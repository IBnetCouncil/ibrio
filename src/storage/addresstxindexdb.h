// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ADDRESSTXINDEXDB_H
#define STORAGE_ADDRESSTXINDEXDB_H

#include <boost/thread/thread.hpp>

#include "transaction.h"
#include "xengine.h"

namespace ibrio
{
namespace storage
{

//////////////////////////////
// CForkAddressTxIndexDBWalker

class CForkAddressTxIndexDBWalker
{
public:
    virtual bool Walk(const CAddrTxIndex& key, const CAddrTxInfo& value) = 0;
};

//////////////////////////////
// CGetAddressTxIndexWalker

class CGetAddressTxIndexWalker : public CForkAddressTxIndexDBWalker
{
public:
    CGetAddressTxIndexWalker(const int nPrevHeightIn, const uint64 nPrevTxSeqIn, const int64 nOffsetIn, const int64 nCountIn, std::map<CAddrTxIndex, CAddrTxInfo>& mapAddressTxIndexIn)
      : nCurPos(0), nPrevHeight(nPrevHeightIn), nPrevTxSeq(nPrevTxSeqIn), nOffset(nOffsetIn), nCount(nCountIn), mapAddressTxIndex(mapAddressTxIndexIn) {}
    bool Walk(const CAddrTxIndex& key, const CAddrTxInfo& value) override;

public:
    int64 nCurPos;
    int nPrevHeight;
    uint64 nPrevTxSeq;
    int64 nOffset;
    int64 nCount;
    std::map<CAddrTxIndex, CAddrTxInfo>& mapAddressTxIndex;
    std::vector<std::pair<CAddrTxIndex, CAddrTxInfo>> vCacheAddrTxInfo;
};

//////////////////////////////
// CForkAddressTxIndexDB

class CForkAddressTxIndexDB : public xengine::CKVDB
{
    typedef std::map<CAddrTxIndex, CAddrTxInfo> MapType;
    class CDblMap
    {
    public:
        CDblMap()
          : nIdxUpper(0) {}
        MapType& GetUpperMap()
        {
            return mapCache[nIdxUpper];
        }
        MapType& GetLowerMap()
        {
            return mapCache[nIdxUpper ^ 1];
        }
        void Flip()
        {
            MapType& mapLower = mapCache[nIdxUpper ^ 1];
            mapLower.clear();
            nIdxUpper = nIdxUpper ^ 1;
        }
        void Clear()
        {
            mapCache[0].clear();
            mapCache[1].clear();
            nIdxUpper = 0;
        }

    protected:
        MapType mapCache[2];
        int nIdxUpper;
    };

public:
    CForkAddressTxIndexDB(const boost::filesystem::path& pathDB);
    ~CForkAddressTxIndexDB();
    bool RemoveAll();
    bool UpdateAddressTxIndex(const std::vector<std::pair<CAddrTxIndex, CAddrTxInfo>>& vAddNew, const std::vector<CAddrTxIndex>& vRemove);
    bool RepairAddressTxIndex(const std::vector<std::pair<CAddrTxIndex, CAddrTxInfo>>& vAddUpdate, const std::vector<CAddrTxIndex>& vRemove);
    bool WriteAddressTxIndex(const CAddrTxIndex& key, const CAddrTxInfo& value);
    bool ReadAddressTxIndex(const CAddrTxIndex& key, CAddrTxInfo& value);
    int64 RetrieveAddressTxIndex(const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, std::map<CAddrTxIndex, CAddrTxInfo>& mapAddrTxIndex);
    bool RetrieveTxIndex(const CAddrTxIndex& addrTxIndex, CAddrTxInfo& addrTxInfo);
    bool Copy(CForkAddressTxIndexDB& dbAddressTxIndex);
    void SetCache(const CDblMap& dblCacheIn)
    {
        dblCache = dblCacheIn;
    }
    bool WalkThroughAddressTxIndex(CForkAddressTxIndexDBWalker& walker, const CDestination& dest = CDestination(), const int nPrevHeight = -2, const uint64 nPrevTxSeq = -1);
    bool Flush();

protected:
    bool CopyWalker(xengine::CBufStream& ssKey, xengine::CBufStream& ssValue,
                    CForkAddressTxIndexDB& dbAddressTxIndex);
    bool LoadWalker(xengine::CBufStream& ssKey, xengine::CBufStream& ssValue,
                    CForkAddressTxIndexDBWalker& walker, const MapType& mapUpper, const MapType& mapLower);

protected:
    xengine::CRWAccess rwUpper;
    xengine::CRWAccess rwLower;
    CDblMap dblCache;
};

class CAddressTxIndexDB
{
public:
    CAddressTxIndexDB();
    bool Initialize(const boost::filesystem::path& pathData, const bool fFlush = true);
    void Deinitialize();
    bool Exists(const uint256& hashFork)
    {
        return (!!mapAddressDB.count(hashFork));
    }
    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork);
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork);
    void Clear();
    bool UpdateAddressTxIndex(const uint256& hashFork, const std::vector<std::pair<CAddrTxIndex, CAddrTxInfo>>& vAddNew, const std::vector<CAddrTxIndex>& vRemove);
    bool RepairAddressTxIndex(const uint256& hashFork, const std::vector<std::pair<CAddrTxIndex, CAddrTxInfo>>& vAddUpdate, const std::vector<CAddrTxIndex>& vRemove);
    int64 RetrieveAddressTxIndex(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, std::map<CAddrTxIndex, CAddrTxInfo>& mapAddrTxIndex);
    bool RetrieveTxIndex(const uint256& hashFork, const CAddrTxIndex& addrTxIndex, CAddrTxInfo& addrTxInfo);
    bool Copy(const uint256& srcFork, const uint256& destFork);
    bool WalkThrough(const uint256& hashFork, CForkAddressTxIndexDBWalker& walker);
    void Flush(const uint256& hashFork);

protected:
    void FlushProc();

protected:
    boost::filesystem::path pathAddress;
    xengine::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkAddressTxIndexDB>> mapAddressDB;

    boost::mutex mtxFlush;
    boost::condition_variable condFlush;
    boost::thread* pThreadFlush;
    bool fStopFlush;
};

} // namespace storage
} // namespace ibrio

#endif //STORAGE_ADDRESSTXINDEXDB_H
