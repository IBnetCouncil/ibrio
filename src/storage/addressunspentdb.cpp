// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressunspentdb.h"

#include <boost/bind.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace xengine;

namespace ibrio
{
namespace storage
{

#define ADDRESS_UNSPENT_FLUSH_INTERVAL (600)

//////////////////////////////
// CForkAddressUnspentDB

CForkAddressUnspentDB::CForkAddressUnspentDB(const boost::filesystem::path& pathDB, const uint256& hashLastBlockIn)
{
    CLevelDBArguments args;
    args.path = pathDB.string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);

    if (!CKVDB::Open(engine))
    {
        delete engine;
    }
    hashLastBlock = hashLastBlockIn;
}

CForkAddressUnspentDB::~CForkAddressUnspentDB()
{
    Close();
    dblCache.Clear();
}

bool CForkAddressUnspentDB::RemoveAll()
{
    if (!CKVDB::RemoveAll())
    {
        return false;
    }
    dblCache.Clear();
    return true;
}

bool CForkAddressUnspentDB::UpdateAddressUnspent(const uint256& hashLastBlockIn, const vector<CTxUnspent>& vAddNew, const vector<CTxUnspent>& vRemove)
{
    xengine::CWriteLock wlock(rwUpper);

    MapType& mapUpper = dblCache.GetUpperMap();

    for (const auto& vd : vAddNew)
    {
        mapUpper[CAddrUnspentKey(vd.output.destTo, static_cast<const CTxOutPoint&>(vd))] = CUnspentOut(vd.output, vd.nTxType, vd.nHeight);
    }

    for (const auto& vd : vRemove)
    {
        mapUpper[CAddrUnspentKey(vd.output.destTo, static_cast<const CTxOutPoint&>(vd))].SetNull();
    }

    hashLastBlock = hashLastBlockIn;
    return true;
}

bool CForkAddressUnspentDB::RepairAddressUnspent(const std::vector<std::pair<CAddrUnspentKey, CUnspentOut>>& vAddUpdate, const std::vector<CAddrUnspentKey>& vRemove)
{
    if (!TxnBegin())
    {
        return false;
    }

    for (const auto& vd : vAddUpdate)
    {
        Write(vd.first, vd.second);
    }

    for (const auto& out : vRemove)
    {
        Erase(out);
    }

    if (!TxnCommit())
    {
        return false;
    }
    return true;
}

bool CForkAddressUnspentDB::WriteAddressUnspent(const CAddrUnspentKey& out, const CUnspentOut& unspent)
{
    return Write(out, unspent);
}

bool CForkAddressUnspentDB::ReadAddressUnspent(const CAddrUnspentKey& out, CUnspentOut& unspent)
{
    {
        xengine::CReadLock rlock(rwUpper);

        MapType& mapUpper = dblCache.GetUpperMap();
        typename MapType::iterator it = mapUpper.find(out);
        if (it != mapUpper.end())
        {
            if (!it->second.IsNull())
            {
                unspent = it->second;
                return true;
            }
            return false;
        }
    }

    {
        xengine::CReadLock rlock(rwLower);

        MapType& mapLower = dblCache.GetLowerMap();
        typename MapType::iterator it = mapLower.find(out);
        if (it != mapLower.end())
        {
            if (!it->second.IsNull())
            {
                unspent = it->second;
                return true;
            }
            return false;
        }
    }

    return Read(out, unspent);
}

bool CForkAddressUnspentDB::RetrieveAddressUnspent(const CDestination& dest, map<CTxOutPoint, CUnspentOut>& mapUnspent, uint256& hashLastBlockOut)
{
    if (dest.IsNull())
    {
        return false;
    }
    CGetSingleAddressUnspentWalker walker(mapUnspent);
    return WalkThroughAddressUnspent(walker, dest, hashLastBlockOut);
}

bool CForkAddressUnspentDB::Copy(CForkAddressUnspentDB& dbAddressUnspent)
{
    if (!dbAddressUnspent.RemoveAll())
    {
        return false;
    }

    try
    {
        xengine::CReadLock rdlock(rwLower);
        xengine::CReadLock rulock(rwUpper);

        if (!WalkThrough(boost::bind(&CForkAddressUnspentDB::CopyWalker, this, _1, _2, boost::ref(dbAddressUnspent))))
        {
            return false;
        }

        dbAddressUnspent.SetCache(dblCache);
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressUnspentDB::WalkThroughAddressUnspent(CForkAddressUnspentDBWalker& walker, const CDestination& dest, uint256& hashLastBlockOut)
{
    try
    {
        xengine::CReadLock rdlock(rwLower);
        xengine::CReadLock rulock(rwUpper);

        MapType& mapUpper = dblCache.GetUpperMap();
        MapType& mapLower = dblCache.GetLowerMap();

        if (dest.IsNull())
        {
            if (!WalkThrough(boost::bind(&CForkAddressUnspentDB::LoadWalker, this, _1, _2, boost::ref(walker),
                                         boost::ref(mapUpper), boost::ref(mapLower))))
            {
                return false;
            }
        }
        else
        {
            if (!WalkThrough(boost::bind(&CForkAddressUnspentDB::LoadWalker, this, _1, _2, boost::ref(walker),
                                         boost::ref(mapUpper), boost::ref(mapLower)),
                             dest, true))
            {
                return false;
            }
        }

        for (MapType::iterator it = mapLower.begin(); it != mapLower.end(); ++it)
        {
            const CAddrUnspentKey& out = (*it).first;
            const CUnspentOut& unspent = (*it).second;
            if ((dest.IsNull() || out.dest == dest) && !mapUpper.count(out) && !unspent.IsNull())
            {
                if (!walker.Walk(out, unspent))
                {
                    return false;
                }
            }
        }
        for (MapType::iterator it = mapUpper.begin(); it != mapUpper.end(); ++it)
        {
            const CAddrUnspentKey& out = (*it).first;
            const CUnspentOut& unspent = (*it).second;
            if ((dest.IsNull() || out.dest == dest) && !unspent.IsNull())
            {
                if (!walker.Walk(out, unspent))
                {
                    return false;
                }
            }
        }
        hashLastBlockOut = hashLastBlock;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkAddressUnspentDB::CopyWalker(CBufStream& ssKey, CBufStream& ssValue,
                                       CForkAddressUnspentDB& dbAddressUnspent)
{
    CAddrUnspentKey out;
    CUnspentOut unspent;
    ssKey >> out;
    ssValue >> unspent;

    return dbAddressUnspent.WriteAddressUnspent(out, unspent);
}

bool CForkAddressUnspentDB::LoadWalker(CBufStream& ssKey, CBufStream& ssValue,
                                       CForkAddressUnspentDBWalker& walker, const MapType& mapUpper, const MapType& mapLower)
{
    CAddrUnspentKey out;
    CUnspentOut unspent;
    ssKey >> out;

    if (mapUpper.count(out) || mapLower.count(out))
    {
        return true;
    }

    ssValue >> unspent;

    return walker.Walk(out, unspent);
}

bool CForkAddressUnspentDB::Flush()
{
    xengine::CUpgradeLock ulock(rwLower);

    vector<pair<CAddrUnspentKey, CUnspentOut>> vAddNew;
    vector<CAddrUnspentKey> vRemove;

    MapType& mapLower = dblCache.GetLowerMap();
    for (typename MapType::iterator it = mapLower.begin(); it != mapLower.end(); ++it)
    {
        if (it->second.IsNull())
        {
            vRemove.push_back(it->first);
        }
        else
        {
            vAddNew.push_back(*it);
        }
    }

    if (!TxnBegin())
    {
        return false;
    }

    for (const auto& addr : vAddNew)
    {
        Write(addr.first, addr.second);
    }

    for (int i = 0; i < vRemove.size(); i++)
    {
        Erase(vRemove[i]);
    }

    if (!TxnCommit())
    {
        return false;
    }

    ulock.Upgrade();

    {
        xengine::CWriteLock wlock(rwUpper);
        dblCache.Flip();
    }

    return true;
}

//////////////////////////////
// CAddressUnspentDB

CAddressUnspentDB::CAddressUnspentDB()
{
    pThreadFlush = nullptr;
    fStopFlush = true;
}

bool CAddressUnspentDB::Initialize(const boost::filesystem::path& pathData, const bool fFlush)
{
    pathAddress = pathData / "addressunspent";

    if (!boost::filesystem::exists(pathAddress))
    {
        boost::filesystem::create_directories(pathAddress);
    }

    if (!boost::filesystem::is_directory(pathAddress))
    {
        return false;
    }

    if (fFlush)
    {
        fStopFlush = false;
        pThreadFlush = new boost::thread(boost::bind(&CAddressUnspentDB::FlushProc, this));
        if (pThreadFlush == nullptr)
        {
            fStopFlush = true;
            return false;
        }
    }
    return true;
}

void CAddressUnspentDB::Deinitialize()
{
    if (pThreadFlush)
    {
        {
            boost::unique_lock<boost::mutex> lock(mtxFlush);
            fStopFlush = true;
        }
        condFlush.notify_all();
        pThreadFlush->join();
        delete pThreadFlush;
        pThreadFlush = nullptr;

        {
            CWriteLock wlock(rwAccess);

            for (map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.begin();
                 it != mapAddressDB.end(); ++it)
            {
                std::shared_ptr<CForkAddressUnspentDB> spAddress = (*it).second;

                spAddress->Flush();
                spAddress->Flush();
            }
            mapAddressDB.clear();
        }
    }
    else
    {
        mapAddressDB.clear();
    }
}

bool CAddressUnspentDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return mapAddressDB.find(hashFork) != mapAddressDB.end();
}

bool CAddressUnspentDB::LoadFork(const uint256& hashFork, const uint256& hashLastBlock)
{
    CWriteLock wlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkAddressUnspentDB> spAddress(new CForkAddressUnspentDB(pathAddress / hashFork.GetHex(), hashLastBlock));
    if (spAddress == nullptr || !spAddress->IsValid())
    {
        return false;
    }
    mapAddressDB.insert(make_pair(hashFork, spAddress));
    return true;
}

void CAddressUnspentDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        (*it).second->RemoveAll();
        mapAddressDB.erase(it);
    }

    boost::filesystem::path forkPath = pathAddress / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CAddressUnspentDB::AddNewFork(const uint256& hashFork, const uint256& hashLastBlock)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork, hashLastBlock);
}

void CAddressUnspentDB::Clear()
{
    CWriteLock wlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.begin();
    while (it != mapAddressDB.end())
    {
        (*it).second->RemoveAll();
        mapAddressDB.erase(it++);
    }
}

bool CAddressUnspentDB::UpdateAddressUnspent(const uint256& hashFork, const uint256& hashLastBlockIn, const vector<CTxUnspent>& vAddNew, const vector<CTxUnspent>& vRemove)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.find(hashFork);
    if (it == mapAddressDB.end())
    {
        StdLog("CAddressUnspentDB", "UpdateAddressUnspent: find fork fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    return it->second->UpdateAddressUnspent(hashLastBlockIn, vAddNew, vRemove);
}

bool CAddressUnspentDB::RepairAddressUnspent(const uint256& hashFork, const vector<pair<CAddrUnspentKey, CUnspentOut>>& vAddUpdate, const vector<CAddrUnspentKey>& vRemove)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.find(hashFork);
    if (it == mapAddressDB.end())
    {
        StdLog("CAddressUnspentDB", "RepairAddressUnspent: find fork fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    return it->second->RepairAddressUnspent(vAddUpdate, vRemove);
}

bool CAddressUnspentDB::RetrieveAddressUnspent(const uint256& hashFork, const CDestination& dest, map<CTxOutPoint, CUnspentOut>& mapUnspent, uint256& hashLastBlockOut)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.find(hashFork);
    if (it == mapAddressDB.end())
    {
        StdLog("CAddressUnspentDB", "RetrieveAddressUnspent: find fork fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    return it->second->RetrieveAddressUnspent(dest, mapUnspent, hashLastBlockOut);
}

bool CAddressUnspentDB::Copy(const uint256& srcFork, const uint256& destFork)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator itSrc = mapAddressDB.find(srcFork);
    if (itSrc == mapAddressDB.end())
    {
        return false;
    }

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator itDest = mapAddressDB.find(destFork);
    if (itDest == mapAddressDB.end())
    {
        return false;
    }

    return ((*itSrc).second->Copy(*(*itDest).second));
}

bool CAddressUnspentDB::WalkThrough(const uint256& hashFork, CForkAddressUnspentDBWalker& walker)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        uint256 hashLastBlock;
        return (*it).second->WalkThroughAddressUnspent(walker, CDestination(), hashLastBlock);
    }
    return false;
}

void CAddressUnspentDB::Flush(const uint256& hashFork)
{
    boost::unique_lock<boost::mutex> lock(mtxFlush);
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.find(hashFork);
    if (it != mapAddressDB.end())
    {
        (*it).second->Flush();
    }
}

void CAddressUnspentDB::FlushProc()
{
    SetThreadName("AddressUnspentDB");
    boost::system_time timeout = boost::get_system_time();
    boost::unique_lock<boost::mutex> lock(mtxFlush);
    while (!fStopFlush)
    {
        timeout += boost::posix_time::seconds(ADDRESS_UNSPENT_FLUSH_INTERVAL);

        while (!fStopFlush)
        {
            if (!condFlush.timed_wait(lock, timeout))
            {
                break;
            }
        }

        if (!fStopFlush)
        {
            CReadLock rlock(rwAccess);

            for (map<uint256, std::shared_ptr<CForkAddressUnspentDB>>::iterator it = mapAddressDB.begin();
                 it != mapAddressDB.end(); ++it)
            {
                it->second->Flush();
            }
        }
    }
}

} // namespace storage
} // namespace ibrio
