// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "unspentdb.h"

#include <boost/bind.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace xengine;

namespace ibrio
{
namespace storage
{

#define UNSPENT_FLUSH_INTERVAL (60)

//////////////////////////////
// CForkUnspentDB

CForkUnspentDB::CForkUnspentDB(const boost::filesystem::path& pathDB)
{
    CLevelDBArguments args;
    args.path = pathDB.string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);

    if (!CKVDB::Open(engine))
    {
        delete engine;
    }
}

CForkUnspentDB::~CForkUnspentDB()
{
    Close();
    dblCache.Clear();
}

bool CForkUnspentDB::RemoveAll()
{
    if (!CKVDB::RemoveAll())
    {
        return false;
    }
    dblCache.Clear();
    return true;
}

bool CForkUnspentDB::UpdateUnspent(const vector<CTxUnspent>& vAddNew, const vector<CTxUnspent>& vRemove)
{
    xengine::CWriteLock wlock(rwUpper);

    MapType& mapUpper = dblCache.GetUpperMap();

    for (const CTxUnspent& unspent : vAddNew)
    {
        mapUpper[static_cast<const CTxOutPoint&>(unspent)] = unspent.output;
    }

    for (const CTxUnspent& unspent : vRemove)
    {
        mapUpper[static_cast<const CTxOutPoint&>(unspent)].SetNull();
    }

    return true;
}

bool CForkUnspentDB::RepairUnspent(const std::vector<CTxUnspent>& vAddUpdate, const std::vector<CTxOutPoint>& vRemove)
{
    if (!TxnBegin())
    {
        return false;
    }

    for (const CTxUnspent& unspent : vAddUpdate)
    {
        Write(static_cast<const CTxOutPoint&>(unspent), unspent.output);
    }

    for (const CTxOutPoint& txout : vRemove)
    {
        Erase(txout);
    }

    if (!TxnCommit())
    {
        return false;
    }
    return true;
}

bool CForkUnspentDB::WriteUnspent(const CTxOutPoint& txout, const CTxOut& output)
{
    return Write(txout, output);
}

bool CForkUnspentDB::ReadUnspent(const CTxOutPoint& txout, CTxOut& output)
{
    {
        xengine::CReadLock rlock(rwUpper);

        MapType& mapUpper = dblCache.GetUpperMap();
        typename MapType::iterator it = mapUpper.find(txout);
        if (it != mapUpper.end())
        {
            if (!(*it).second.IsNull())
            {
                output = (*it).second;
                return true;
            }
            return false;
        }
    }

    {
        xengine::CReadLock rlock(rwLower);
        MapType& mapLower = dblCache.GetLowerMap();
        typename MapType::iterator it = mapLower.find(txout);
        if (it != mapLower.end())
        {
            if (!(*it).second.IsNull())
            {
                output = (*it).second;
                return true;
            }
            return false;
        }
    }

    return Read(txout, output);
}

bool CForkUnspentDB::Copy(CForkUnspentDB& dbUnspent)
{
    if (!dbUnspent.RemoveAll())
    {
        return false;
    }

    try
    {
        xengine::CReadLock rdlock(rwLower);
        xengine::CReadLock rulock(rwUpper);

        if (!WalkThrough(boost::bind(&CForkUnspentDB::CopyWalker, this, _1, _2, boost::ref(dbUnspent))))
        {
            return false;
        }

        dbUnspent.SetCache(dblCache);
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkUnspentDB::WalkThroughUnspent(CForkUnspentDBWalker& walker)
{
    try
    {
        xengine::CReadLock rdlock(rwLower);
        xengine::CReadLock rulock(rwUpper);

        MapType& mapUpper = dblCache.GetUpperMap();
        MapType& mapLower = dblCache.GetLowerMap();

        if (!WalkThrough(boost::bind(&CForkUnspentDB::LoadWalker, this, _1, _2, boost::ref(walker),
                                     boost::ref(mapUpper), boost::ref(mapLower))))
        {
            return false;
        }

        for (MapType::iterator it = mapLower.begin(); it != mapLower.end(); ++it)
        {
            const CTxOutPoint& txout = (*it).first;
            const CTxOut& output = (*it).second;
            if (!mapUpper.count(txout) && !output.IsNull())
            {
                if (!walker.Walk(txout, output))
                {
                    return false;
                }
            }
        }
        for (MapType::iterator it = mapUpper.begin(); it != mapUpper.end(); ++it)
        {
            const CTxOutPoint& txout = (*it).first;
            const CTxOut& output = (*it).second;
            if (!output.IsNull())
            {
                if (!walker.Walk(txout, output))
                {
                    return false;
                }
            }
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CForkUnspentDB::CopyWalker(CBufStream& ssKey, CBufStream& ssValue,
                                CForkUnspentDB& dbUnspent)
{
    CTxOutPoint txout;
    CTxOut output;
    ssKey >> txout;
    ssValue >> output;

    return dbUnspent.WriteUnspent(txout, output);
}

bool CForkUnspentDB::LoadWalker(CBufStream& ssKey, CBufStream& ssValue,
                                CForkUnspentDBWalker& walker, const MapType& mapUpper, const MapType& mapLower)
{
    CTxOutPoint txout;
    CTxOut output;
    ssKey >> txout;

    if (mapUpper.count(txout) || mapLower.count(txout))
    {
        return true;
    }

    ssValue >> output;

    return walker.Walk(txout, output);
}

bool CForkUnspentDB::Flush()
{
    xengine::CUpgradeLock ulock(rwLower);

    vector<pair<CTxOutPoint, CTxOut>> vAddNew;
    vector<CTxOutPoint> vRemove;

    MapType& mapLower = dblCache.GetLowerMap();
    for (typename MapType::iterator it = mapLower.begin(); it != mapLower.end(); ++it)
    {
        CTxOut& output = (*it).second;
        if (!output.IsNull())
        {
            vAddNew.push_back(*it);
        }
        else
        {
            vRemove.push_back((*it).first);
        }
    }

    if (!TxnBegin())
    {
        return false;
    }

    for (int i = 0; i < vAddNew.size(); i++)
    {
        Write(vAddNew[i].first, vAddNew[i].second);
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
// CUnspentDB

CUnspentDB::CUnspentDB()
{
    pThreadFlush = nullptr;
    fStopFlush = true;
}

bool CUnspentDB::Initialize(const boost::filesystem::path& pathData, const bool fFlush)
{
    pathUnspent = pathData / "unspent";

    if (!boost::filesystem::exists(pathUnspent))
    {
        boost::filesystem::create_directories(pathUnspent);
    }

    if (!boost::filesystem::is_directory(pathUnspent))
    {
        return false;
    }

    if (fFlush)
    {
        fStopFlush = false;
        pThreadFlush = new boost::thread(boost::bind(&CUnspentDB::FlushProc, this));
        if (pThreadFlush == nullptr)
        {
            fStopFlush = true;
            return false;
        }
    }
    return true;
}

void CUnspentDB::Deinitialize()
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

            for (map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.begin();
                 it != mapUnspentDB.end(); ++it)
            {
                std::shared_ptr<CForkUnspentDB> spUnspent = (*it).second;

                spUnspent->Flush();
                spUnspent->Flush();
            }
            mapUnspentDB.clear();
        }
    }
    else
    {
        CWriteLock wlock(rwAccess);
        mapUnspentDB.clear();
    }
}

bool CUnspentDB::ExistFork(const uint256& hashFork)
{
    CReadLock rlock(rwAccess);
    return mapUnspentDB.find(hashFork) != mapUnspentDB.end();
}

bool CUnspentDB::LoadFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.find(hashFork);
    if (it != mapUnspentDB.end())
    {
        return true;
    }

    std::shared_ptr<CForkUnspentDB> spUnspent(new CForkUnspentDB(pathUnspent / hashFork.GetHex()));
    if (spUnspent == nullptr || !spUnspent->IsValid())
    {
        return false;
    }
    mapUnspentDB.insert(make_pair(hashFork, spUnspent));
    return true;
}

void CUnspentDB::RemoveFork(const uint256& hashFork)
{
    CWriteLock wlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.find(hashFork);
    if (it != mapUnspentDB.end())
    {
        (*it).second->RemoveAll();
        mapUnspentDB.erase(it);
    }

    boost::filesystem::path forkPath = pathUnspent / hashFork.GetHex();
    if (boost::filesystem::exists(forkPath))
    {
        boost::filesystem::remove_all(forkPath);
    }
}

bool CUnspentDB::AddNewFork(const uint256& hashFork)
{
    RemoveFork(hashFork);
    return LoadFork(hashFork);
}

void CUnspentDB::Clear()
{
    CWriteLock wlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.begin();
    while (it != mapUnspentDB.end())
    {
        (*it).second->RemoveAll();
        mapUnspentDB.erase(it++);
    }
}

bool CUnspentDB::Update(const uint256& hashFork,
                        const vector<CTxUnspent>& vAddNew, const vector<CTxUnspent>& vRemove)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.find(hashFork);
    if (it != mapUnspentDB.end())
    {
        return (*it).second->UpdateUnspent(vAddNew, vRemove);
    }
    return false;
}

bool CUnspentDB::RepairUnspent(const uint256& hashFork, const std::vector<CTxUnspent>& vAddUpdate, const std::vector<CTxOutPoint>& vRemove)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.find(hashFork);
    if (it != mapUnspentDB.end())
    {
        return (*it).second->RepairUnspent(vAddUpdate, vRemove);
    }
    return false;
}

bool CUnspentDB::Retrieve(const uint256& hashFork, const CTxOutPoint& txout, CTxOut& output)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.find(hashFork);
    if (it != mapUnspentDB.end())
    {
        return (*it).second->ReadUnspent(txout, output);
    }
    return false;
}

bool CUnspentDB::Copy(const uint256& srcFork, const uint256& destFork)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator itSrc = mapUnspentDB.find(srcFork);
    if (itSrc == mapUnspentDB.end())
    {
        return false;
    }

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator itDest = mapUnspentDB.find(destFork);
    if (itDest == mapUnspentDB.end())
    {
        return false;
    }

    return ((*itSrc).second->Copy(*(*itDest).second));
}

bool CUnspentDB::WalkThrough(const uint256& hashFork, CForkUnspentDBWalker& walker)
{
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.find(hashFork);
    if (it != mapUnspentDB.end())
    {
        return (*it).second->WalkThroughUnspent(walker);
    }
    return false;
}

void CUnspentDB::Flush(const uint256& hashFork)
{
    boost::unique_lock<boost::mutex> lock(mtxFlush);
    CReadLock rlock(rwAccess);

    map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.find(hashFork);
    if (it != mapUnspentDB.end())
    {
        (*it).second->Flush();
    }
}

void CUnspentDB::FlushProc()
{
    SetThreadName("UnspentDB");
    boost::system_time timeout = boost::get_system_time();
    boost::unique_lock<boost::mutex> lock(mtxFlush);
    while (!fStopFlush)
    {
        timeout += boost::posix_time::seconds(UNSPENT_FLUSH_INTERVAL);

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

            for (map<uint256, std::shared_ptr<CForkUnspentDB>>::iterator it = mapUnspentDB.begin();
                 it != mapUnspentDB.end(); ++it)
            {
                it->second->Flush();
            }
        }
    }
}

} // namespace storage
} // namespace ibrio
