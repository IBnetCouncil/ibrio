// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_TIMESERIES_H
#define STORAGE_TIMESERIES_H

#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <xengine.h>

#include "crc24q.h"
#include "uint256.h"

namespace ibrio
{
namespace storage
{

class CDiskPos
{
    friend class xengine::CStream;

public:
    uint32 nFile;
    uint32 nOffset;

public:
    CDiskPos(uint32 nFileIn = 0, uint32 nOffsetIn = 0)
      : nFile(nFileIn), nOffset(nOffsetIn) {}
    bool IsNull() const
    {
        return (nFile == 0);
    }
    bool operator==(const CDiskPos& b) const
    {
        return (nFile == b.nFile && nOffset == b.nOffset);
    }
    bool operator!=(const CDiskPos& b) const
    {
        return (nFile != b.nFile || nOffset != b.nOffset);
    }
    bool operator<(const CDiskPos& b) const
    {
        return (nFile < b.nFile || (nFile == b.nFile && nOffset < b.nOffset));
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(nFile, opt);
        s.Serialize(nOffset, opt);
    }
};

template <typename T>
class CTSWalker
{
public:
    virtual bool Walk(const T& t, uint32 nFile, uint32 nOffset) = 0;
};

class CTimeSeriesBase
{
public:
    CTimeSeriesBase();
    ~CTimeSeriesBase();
    virtual bool Initialize(const boost::filesystem::path& pathLocationIn, const std::string& strPrefixIn);
    virtual void Deinitialize();

protected:
    bool CheckDiskSpace();
    const std::string FileName(uint32 nFile);
    bool GetFilePath(uint32 nFile, std::string& strPath);
    bool GetLastFilePath(uint32& nFile, std::string& strPath, const uint32 nWriteDataSize);
    bool RemoveFollowUpFile(uint32 nBeginFile);
    bool TruncateFile(const std::string& pathFile, uint32 nOffset);
    bool RepairFile(uint32 nFile, uint32 nOffset);

protected:
    enum
    {
        MAX_FILE_SIZE = 0x7F000000,
        MAX_CHUNK_SIZE = 0x200000
    };
    boost::filesystem::path pathLocation;
    std::string strPrefix;
    uint32 nLastFile;
};

class CTimeSeriesCached : public CTimeSeriesBase
{
public:
    CTimeSeriesCached();
    ~CTimeSeriesCached();
    bool Initialize(const boost::filesystem::path& pathLocationIn, const std::string& strPrefixIn);
    void Deinitialize();
    template <typename T>
    bool Write(const T& t, uint32& nFile, uint32& nOffset, bool fWriteCache = true)
    {
        boost::unique_lock<boost::mutex> lock(mtxCache);

        xengine::CBufStream ss;
        ss << t;

        std::string pathFile;
        if (!GetLastFilePath(nFile, pathFile, ss.GetSize()))
        {
            return false;
        }
        try
        {
            xengine::CFileStream fs(pathFile.c_str());
            fs.SeekToEnd();
            uint32 nSize = ss.GetSize();
            uint32 nCrc = ibrio::crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
            fs << nMagicNum << nSize << nCrc;
            nOffset = fs.GetCurPos();
            fs.Write(ss.GetData(), ss.GetSize());
        }
        catch (std::exception& e)
        {
            xengine::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        if (fWriteCache)
        {
            if (!WriteToCache(ss.GetData(), ss.GetSize(), CDiskPos(nFile, nOffset)))
            {
                ResetCache();
            }
        }
        return true;
    }
    template <typename T>
    bool Write(const T& t, CDiskPos& pos, bool fWriteCache = true)
    {
        boost::unique_lock<boost::mutex> lock(mtxCache);

        xengine::CBufStream ss;
        ss << t;

        std::string pathFile;
        if (!GetLastFilePath(pos.nFile, pathFile, ss.GetSize()))
        {
            return false;
        }
        try
        {
            xengine::CFileStream fs(pathFile.c_str());
            fs.SeekToEnd();
            uint32 nSize = ss.GetSize();
            uint32 nCrc = ibrio::crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
            fs << nMagicNum << nSize << nCrc;
            pos.nOffset = fs.GetCurPos();
            fs.Write(ss.GetData(), ss.GetSize());
        }
        catch (std::exception& e)
        {
            xengine::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        if (fWriteCache)
        {
            if (!WriteToCache(ss.GetData(), ss.GetSize(), pos))
            {
                ResetCache();
            }
        }
        return true;
    }
    template <typename T>
    bool Read(T& t, uint32 nFile, uint32 nOffset, bool fWriteCache = true)
    {
        boost::unique_lock<boost::mutex> lock(mtxCache);

        if (ReadFromCache(t, CDiskPos(nFile, nOffset)))
        {
            return true;
        }

        std::string pathFile;
        if (!GetFilePath(nFile, pathFile))
        {
            return false;
        }
        try
        {
            // Open history file to read
            xengine::CFileStream fs(pathFile.c_str());
            fs.Seek(nOffset);
            fs >> t;
        }
        catch (std::exception& e)
        {
            xengine::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }

        if (fWriteCache)
        {
            if (!WriteToCache(t, CDiskPos(nFile, nOffset)))
            {
                ResetCache();
            }
        }
        return true;
    }
    template <typename T>
    bool Read(T& t, const CDiskPos& pos, bool fWriteCache = true)
    {
        boost::unique_lock<boost::mutex> lock(mtxCache);

        if (ReadFromCache(t, pos))
        {
            return true;
        }

        std::string pathFile;
        if (!GetFilePath(pos.nFile, pathFile))
        {
            return false;
        }
        try
        {
            // Open history file to read
            xengine::CFileStream fs(pathFile.c_str());
            fs.Seek(pos.nOffset);
            fs >> t;
        }
        catch (std::exception& e)
        {
            xengine::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }

        if (fWriteCache)
        {
            if (!WriteToCache(t, pos))
            {
                ResetCache();
            }
        }
        return true;
    }
    template <typename T>
    bool WalkThrough(CTSWalker<T>& walker, uint32& nLastFileRet, uint32& nLastPosRet, bool fRepairFile)
    {
        bool fRet = true;
        uint32 nFile = 1;
        uint32 nOffset = 0;
        nLastFileRet = 0;
        nLastPosRet = 0;
        std::string pathFile;
        uint8* pReadBuf = nullptr;
        uint32 nReadBufSize = 0;
        const uint32 nHeadSize = sizeof(uint32) + sizeof(uint32) + sizeof(uint32);

        while (GetFilePath(nFile, pathFile) && fRet)
        {
            nLastFileRet = nFile;
            bool fFileDataError = false;
            try
            {
                xengine::CFileStream fs(pathFile.c_str());
                fs.Seek(0);
                nOffset = 0;
                std::size_t nFileSize = fs.GetSize();
                if (nFileSize > MAX_FILE_SIZE)
                {
                    xengine::StdError("TimeSeriesCached", "WalkThrough: File size error, nFile: %d, size: %lu", nFile, nFileSize);
                    fFileDataError = true;
                }
                else
                {
                    while (!fs.IsEOF() && fRet && nOffset < (uint32)nFileSize)
                    {
                        if (nOffset + nHeadSize > (uint32)nFileSize)
                        {
                            xengine::StdError("TimeSeriesCached", "WalkThrough: (nOffset + %d) error, nFile: %d, nFileSize: %lu, nOffset: %d", nHeadSize, nFile, nFileSize, nOffset);
                            fFileDataError = true;
                            break;
                        }
                        uint32 nMagic, nSize, nCrc;
                        try
                        {
                            fs >> nMagic >> nSize >> nCrc;
                        }
                        catch (std::exception& e)
                        {
                            xengine::StdError("TimeSeriesCached", "WalkThrough: Read nMagic and nSize error, nFile: %d, msg: %s", nFile, e.what());
                            fFileDataError = true;
                            break;
                        }
                        if (nMagic != nMagicNum)
                        {
                            xengine::StdError("TimeSeriesCached", "WalkThrough: nMagic error, nFile: %d, nOffset: %d, nMagic: %x, right magic: %x",
                                              nFile, nOffset, nMagic, nMagicNum);
                            fFileDataError = true;
                            break;
                        }
                        if (nOffset + nHeadSize + nSize > (uint32)nFileSize)
                        {
                            xengine::StdError("TimeSeriesCached", "WalkThrough: (nOffset + %d + nSize) error, nFile: %d, nFileSize: %lu, nOffset: %d, nSize: %d",
                                              nHeadSize, nFile, nFileSize, nOffset, nSize);
                            fFileDataError = true;
                            break;
                        }
                        T t;
                        if (nSize > 0)
                        {
                            if (nReadBufSize < nSize)
                            {
                                if (pReadBuf)
                                {
                                    free(pReadBuf);
                                    pReadBuf = nullptr;
                                }
                                nReadBufSize = nSize;
                                pReadBuf = (uint8*)malloc(nReadBufSize);
                                if (pReadBuf == nullptr)
                                {
                                    xengine::StdError("TimeSeriesCached", "WalkThrough: malloc error, nFile: %d", nFile);
                                    fFileDataError = true;
                                    break;
                                }
                            }
                            try
                            {
                                fs.Read((char*)pReadBuf, nSize);
                            }
                            catch (std::exception& e)
                            {
                                xengine::StdError("TimeSeriesCached", "WalkThrough: Read data error, nFile: %d, msg: %s", nFile, e.what());
                                fFileDataError = true;
                                break;
                            }
                            if (nCrc != ibrio::crypto::crc24q(pReadBuf, nSize))
                            {
                                xengine::StdError("TimeSeriesCached", "WalkThrough: crc error, nFile: %d", nFile);
                                fFileDataError = true;
                                break;
                            }
                            try
                            {
                                xengine::CBufStream bs;
                                bs.Write((const char*)pReadBuf, nSize);
                                bs >> t;
                                if (bs.size() > 0)
                                {
                                    xengine::StdError("TimeSeriesCached", "WalkThrough: data size error, nFile: %d", nFile);
                                    fFileDataError = true;
                                    break;
                                }
                            }
                            catch (std::exception& e)
                            {
                                xengine::StdError("TimeSeriesCached", "WalkThrough: Read t error, nFile: %d, msg: %s", nFile, e.what());
                                fFileDataError = true;
                                break;
                            }
                        }
                        if ((fs.GetCurPos() - nOffset - nHeadSize) != nSize)
                        {
                            xengine::StdError("TimeSeriesCached", "WalkThrough: Read size error, nFile: %d, GetCurPos: %lu, nOffset: %d, nSize: %d",
                                              nFile, fs.GetCurPos(), nOffset, nSize);
                            fFileDataError = true;
                            break;
                        }
                        if (!walker.Walk(t, nFile, nOffset + nHeadSize))
                        {
                            xengine::StdLog("TimeSeriesCached", "WalkThrough: Walk fail");
                            fRet = false;
                            break;
                        }
                        nOffset = fs.GetCurPos();
                    }
                    if (fRet && !fFileDataError)
                    {
                        if (nOffset != (uint32)nFileSize)
                        {
                            xengine::StdLog("TimeSeriesCached", "WalkThrough: nOffset error, nOffset: %d, nFileSize: %lu", nOffset, nFileSize);
                        }
                    }
                }
            }
            catch (std::exception& e)
            {
                xengine::StdError("TimeSeriesCached", "WalkThrough: catch error, nFile: %d, msg: %s", nFile, e.what());
                fRet = false;
                break;
            }
            if (fFileDataError)
            {
                if (fRepairFile)
                {
                    if (!RepairFile(nFile, nOffset))
                    {
                        xengine::StdError("TimeSeriesCached", "WalkThrough: RepairFile fail");
                        fRet = false;
                    }
                    xengine::StdLog("TimeSeriesCached", "WalkThrough: RepairFile success");
                }
                break;
            }
            nFile++;
        }
        nLastPosRet = nOffset;
        if (pReadBuf)
        {
            free(pReadBuf);
            pReadBuf = nullptr;
        }
        return fRet;
    }
    template <typename T>
    bool ReadDirect(T& t, uint32 nFile, uint32 nOffset)
    {
        std::string pathFile;
        if (!GetFilePath(nFile, pathFile))
        {
            return false;
        }
        try
        {
            xengine::CFileStream fs(pathFile.c_str());
            fs.Seek(nOffset);
            fs >> t;
        }
        catch (...)
        {
            return false;
        }
        return true;
    }
    size_t GetSize(const uint32 nFile = -1)
    {
        uint32 nFileNo = (nFile == -1) ? 1 : nFile;
        size_t nOffset = 0;
        std::string pathFile;
        while (GetFilePath(nFileNo, pathFile))
        {
            try
            {
                xengine::CFileStream fs(pathFile.c_str());
                nOffset += fs.GetSize();
            }
            catch (std::exception& e)
            {
                xengine::StdError("TimeSeriesCached", "GetSize: catch error, nFile: %d, msg: %s", nFile, e.what());
                break;
            }

            if (nFile == -1)
            {
                nFileNo++;
            }
            else
            {
                break;
            }
        }
        return nOffset;
    }

protected:
    void ResetCache();
    bool VacateCache(uint32 nNeeded);
    template <typename T>
    bool WriteToCache(const T& t, const CDiskPos& diskpos)
    {
        xengine::CBufStream ss;
        ss << t;
        return WriteToCache(ss.GetData(), ss.GetSize(), diskpos);
    }
    bool WriteToCache(const char* pData, const uint32 nSize, const CDiskPos& diskpos)
    {
        if (mapCachePos.count(diskpos))
        {
            return true;
        }
        if (!VacateCache(nSize))
        {
            return false;
        }
        try
        {
            std::size_t nPos;
            cacheStream << diskpos << nSize;
            nPos = cacheStream.GetWritePos();
            cacheStream.Write(pData, nSize);
            mapCachePos.insert(std::make_pair(diskpos, nPos));
            return true;
        }
        catch (std::exception& e)
        {
            xengine::StdError(__PRETTY_FUNCTION__, e.what());
        }
        return false;
    }
    template <typename T>
    bool ReadFromCache(T& t, const CDiskPos& diskpos)
    {
        std::map<CDiskPos, size_t>::iterator it = mapCachePos.find(diskpos);
        if (it != mapCachePos.end())
        {
            if (cacheStream.Seek((*it).second))
            {
                try
                {
                    cacheStream >> t;
                    return true;
                }
                catch (std::exception& e)
                {
                    xengine::StdError(__PRETTY_FUNCTION__, e.what());
                }
            }
            ResetCache();
        }
        return false;
    }

protected:
    enum
    {
        FILE_CACHE_SIZE = 0x2000000
    };
    boost::mutex mtxCache;
    xengine::CCircularStream cacheStream;
    std::map<CDiskPos, std::size_t> mapCachePos;
    static const uint32 nMagicNum;
};

class CTimeSeriesChunk : public CTimeSeriesBase
{
public:
    CTimeSeriesChunk();
    ~CTimeSeriesChunk();
    template <typename T>
    bool Write(const T& t, CDiskPos& pos)
    {
        boost::unique_lock<boost::mutex> lock(mtxWriter);

        xengine::CBufStream ss;
        ss << t;

        std::string pathFile;
        if (!GetLastFilePath(pos.nFile, pathFile, ss.GetSize()))
        {
            return false;
        }
        try
        {
            xengine::CFileStream fs(pathFile.c_str());
            fs.SeekToEnd();
            uint32 nSize = ss.GetSize();
            uint32 nCrc = ibrio::crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
            fs << nMagicNum << nSize << nCrc;
            pos.nOffset = fs.GetCurPos();
            fs.Write(ss.GetData(), ss.GetSize());
        }
        catch (std::exception& e)
        {
            xengine::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        return true;
    }
    template <typename T>
    bool WriteBatch(const typename std::vector<T>& vBatch, std::vector<CDiskPos>& vPos)
    {
        boost::unique_lock<boost::mutex> lock(mtxWriter);

        size_t n = 0;
        const uint32 nHeadSize = sizeof(uint32) + sizeof(uint32) + sizeof(uint32);

        while (n < vBatch.size())
        {
            xengine::CBufStream ss;
            ss << vBatch[n];

            uint32 nFile, nOffset;
            std::string pathFile;
            if (!GetLastFilePath(nFile, pathFile, ss.GetSize()))
            {
                return false;
            }
            try
            {
                xengine::CFileStream fs(pathFile.c_str());
                fs.SeekToEnd();
                do
                {
                    uint32 nSize = ss.GetSize();
                    uint32 nCrc = ibrio::crypto::crc24q((const unsigned char*)(ss.GetData()), (int)(ss.GetSize()));
                    fs << nMagicNum << nSize << nCrc;
                    nOffset = fs.GetCurPos();
                    fs.Write(ss.GetData(), ss.GetSize());
                    vPos.push_back(CDiskPos(nFile, nOffset));
                    if (++n >= vBatch.size())
                    {
                        break;
                    }
                    ss.Clear();
                    ss << vBatch[n];
                } while (fs.GetCurPos() + ss.GetSize() + nHeadSize <= MAX_FILE_SIZE);
            }
            catch (std::exception& e)
            {
                xengine::StdError(__PRETTY_FUNCTION__, e.what());
                return false;
            }
        }

        return true;
    }
    template <typename T>
    bool Read(T& t, const CDiskPos& pos)
    {
        std::string pathFile;
        if (!GetFilePath(pos.nFile, pathFile))
        {
            return false;
        }
        try
        {
            // Open history file to read
            xengine::CFileStream fs(pathFile.c_str());
            fs.Seek(pos.nOffset);
            fs >> t;
        }
        catch (std::exception& e)
        {
            xengine::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        return true;
    }

protected:
    boost::mutex mtxWriter;
    static const uint32 nMagicNum;
};

} // namespace storage
} // namespace ibrio

#endif //STORAGE_TIMESERIES_H
