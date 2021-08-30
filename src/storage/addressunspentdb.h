// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ADDRESSUNSPENTDB_H
#define STORAGE_ADDRESSUNSPENTDB_H

#include <boost/thread/thread.hpp>

#include "transaction.h"
#include "xengine.h"

namespace ibrio
{
namespace storage
{

//////////////////////////////
// CAddrUnspentKey

class CAddrUnspentKey
{
    friend class xengine::CStream;

public:
    CDestination dest;
    CTxOutPoint out;

public:
    CAddrUnspentKey() {}
    CAddrUnspentKey(const CDestination& dest_in, const CTxOutPoint& out_in)
      : dest(dest_in), out(out_in) {}

    friend bool operator==(const CAddrUnspentKey& a, const CAddrUnspentKey& b)
    {
        return (a.dest == b.dest && a.out == b.out);
    }
    friend bool operator!=(const CAddrUnspentKey& a, const CAddrUnspentKey& b)
    {
        return !(a == b);
    }
    friend bool operator<(const CAddrUnspentKey& a, const CAddrUnspentKey& b)
    {
        return (a.dest < b.dest || (a.dest == b.dest && a.out < b.out));
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(dest, opt);
        s.Serialize(out, opt);
    }
};

//////////////////////////////
// CForkAddressUnspentDBWalker

class CForkAddressUnspentDBWalker
{
public:
    virtual bool Walk(const CAddrUnspentKey& out, const CUnspentOut& unspent) = 0;
};

//////////////////////////////
// CGetAddressUnspentWalker

class CGetAddressUnspentWalker : public CForkAddressUnspentDBWalker
{
public:
    CGetAddressUnspentWalker() {}
    bool Walk(const CAddrUnspentKey& out, const CUnspentOut& unspent) override
    {
        mapAddressUnspent[out] = unspent;
        return true;
    }

public:
    std::map<CAddrUnspentKey, CUnspentOut> mapAddressUnspent;
};

//////////////////////////////
// CGetSingleAddressUnspentWalker

class CGetSingleAddressUnspentWalker : public CForkAddressUnspentDBWalker
{
public:
    CGetSingleAddressUnspentWalker(std::map<CTxOutPoint, CUnspentOut>& mapAddressUnspentIn)
      : mapAddressUnspent(mapAddressUnspentIn) {}
    bool Walk(const CAddrUnspentKey& out, const CUnspentOut& unspent) override
    {
        mapAddressUnspent[out.out] = unspent;
        return true;
    }

public:
    std::map<CTxOutPoint, CUnspentOut>& mapAddressUnspent;
};

//////////////////////////////
// CForkAddressUnspentDB

class CForkAddressUnspentDB : public xengine::CKVDB
{
    typedef std::map<CAddrUnspentKey, CUnspentOut> MapType;
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
    CForkAddressUnspentDB(const boost::filesystem::path& pathDB, const uint256& hashLastBlockIn);
    ~CForkAddressUnspentDB();
    bool RemoveAll();
    bool UpdateAddressUnspent(const uint256& hashLastBlockIn, const std::vector<CTxUnspent>& vAddNew, const std::vector<CTxUnspent>& vRemove);
    bool RepairAddressUnspent(const std::vector<std::pair<CAddrUnspentKey, CUnspentOut>>& vAddUpdate, const std::vector<CAddrUnspentKey>& vRemove);
    bool WriteAddressUnspent(const CAddrUnspentKey& out, const CUnspentOut& unspent);
    bool ReadAddressUnspent(const CAddrUnspentKey& out, CUnspentOut& unspent);
    bool RetrieveAddressUnspent(const CDestination& dest, std::map<CTxOutPoint, CUnspentOut>& mapUnspent, uint256& hashLastBlockOut);
    bool Copy(CForkAddressUnspentDB& dbAddressUnspent);
    void SetCache(const CDblMap& dblCacheIn)
    {
        dblCache = dblCacheIn;
    }
    bool WalkThroughAddressUnspent(CForkAddressUnspentDBWalker& walker, const CDestination& dest, uint256& hashLastBlockOut);
    bool Flush();

protected:
    bool CopyWalker(xengine::CBufStream& ssKey, xengine::CBufStream& ssValue,
                    CForkAddressUnspentDB& dbAddressUnspent);
    bool LoadWalker(xengine::CBufStream& ssKey, xengine::CBufStream& ssValue,
                    CForkAddressUnspentDBWalker& walker, const MapType& mapUpper, const MapType& mapLower);

protected:
    xengine::CRWAccess rwUpper;
    xengine::CRWAccess rwLower;
    CDblMap dblCache;
    uint256 hashLastBlock;
};

class CAddressUnspentDB
{
public:
    CAddressUnspentDB();
    bool Initialize(const boost::filesystem::path& pathData, const bool fFlush = true);
    void Deinitialize();
    bool Exists(const uint256& hashFork)
    {
        return (!!mapAddressDB.count(hashFork));
    }
    bool ExistFork(const uint256& hashFork);
    bool LoadFork(const uint256& hashFork, const uint256& hashLastBlock = uint256());
    void RemoveFork(const uint256& hashFork);
    bool AddNewFork(const uint256& hashFork, const uint256& hashLastBlock = uint256());
    void Clear();
    bool UpdateAddressUnspent(const uint256& hashFork, const uint256& hashLastBlockIn, const std::vector<CTxUnspent>& vAddNew, const std::vector<CTxUnspent>& vRemove);
    bool RepairAddressUnspent(const uint256& hashFork, const std::vector<std::pair<CAddrUnspentKey, CUnspentOut>>& vAddUpdate, const std::vector<CAddrUnspentKey>& vRemove);
    bool RetrieveAddressUnspent(const uint256& hashFork, const CDestination& dest, std::map<CTxOutPoint, CUnspentOut>& mapUnspent, uint256& hashLastBlockOut);
    bool Copy(const uint256& srcFork, const uint256& destFork);
    bool WalkThrough(const uint256& hashFork, CForkAddressUnspentDBWalker& walker);
    void Flush(const uint256& hashFork);

protected:
    void FlushProc();

protected:
    boost::filesystem::path pathAddress;
    xengine::CRWAccess rwAccess;
    std::map<uint256, std::shared_ptr<CForkAddressUnspentDB>> mapAddressDB;

    boost::mutex mtxFlush;
    boost::condition_variable condFlush;
    boost::thread* pThreadFlush;
    bool fStopFlush;
};

} // namespace storage
} // namespace ibrio

#endif //STORAGE_ADDRESSUNSPENTDB_H
