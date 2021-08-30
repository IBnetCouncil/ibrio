// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "investdb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

#include "block.h"

using namespace std;
using namespace xengine;

namespace ibrio
{
namespace storage
{

//////////////////////////////
// CInvestDB

bool CInvestDB::Initialize(const boost::filesystem::path& pathData)
{
    CLevelDBArguments args;
    args.path = (pathData / "invest").string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);

    if (!Open(engine))
    {
        delete engine;
        return false;
    }
    return true;
}

void CInvestDB::Deinitialize()
{
    Close();
}

bool CInvestDB::AddNew(const uint256& hashBlock, const CInvestContext& ctxtInvest)
{
    xengine::CWriteLock wlock(rwData);
    return AddInvest(hashBlock, ctxtInvest);
}

bool CInvestDB::Remove(const uint256& hashBlock)
{
    xengine::CWriteLock wlock(rwData);
    return RemoveInvest(hashBlock);
}

bool CInvestDB::Retrieve(const uint256& hashBlock, CInvestContext& ctxtInvest)
{
    xengine::CReadLock rlock(rwData);
    if (!GetBlockFullInvest(hashBlock, ctxtInvest))
    {
        StdError("CInvestDB", "Retrieve: Get invest fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    return true;
}

bool CInvestDB::AddBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, const std::vector<std::pair<CDestination, int64>>& vNoPackingRewardIn)
{
    xengine::CWriteLock wlock(rwData);

    CInvestContext ctxtInvest;
    if (IsFullInvest(hashBlock))
    {
        if (!GetFullBlockInvest(hashBlock, hashPrev, vTxInvestIn, ctxtInvest))
        {
            StdError("CInvestDB", "Add block invest: Add full invest fail");
            return false;
        }
    }
    else
    {
        if (!GetIncrementBlockInvest(hashBlock, hashPrev, vTxInvestIn, ctxtInvest))
        {
            StdError("CInvestDB", "Add block invest: Add increment invest fail");
            return false;
        }
    }
    ctxtInvest.vNoPackingReward.clear();
    ctxtInvest.vNoPackingReward.assign(vNoPackingRewardIn.begin(), vNoPackingRewardIn.end());

    if (!AddInvest(hashBlock, ctxtInvest))
    {
        StdError("CInvestDB", "Add block invest: Add invest fail");
        return false;
    }
    return true;
}

bool CInvestDB::GetFullInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtFullInvestOut)
{
    xengine::CReadLock rlock(rwData);
    return GetFullBlockInvest(hashBlock, hashPrev, vTxInvestIn, ctxtFullInvestOut);
}

bool CInvestDB::GetBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtInvest)
{
    xengine::CReadLock rlock(rwData);
    if (IsFullInvest(hashBlock))
    {
        if (!GetFullBlockInvest(hashBlock, hashPrev, vTxInvestIn, ctxtInvest))
        {
            return false;
        }
    }
    else
    {
        if (!GetIncrementBlockInvest(hashBlock, hashPrev, vTxInvestIn, ctxtInvest))
        {
            return false;
        }
    }
    return true;
}

bool CInvestDB::RetrieveBlockInvest(const uint256& hashBlock, CInvestContext& ctxtInvest)
{
    xengine::CReadLock rlock(rwData);
    return GetInvest(hashBlock, ctxtInvest);
}

void CInvestDB::Clear()
{
    mapCacheFullInvest.clear();
    mapCacheIncInvest.clear();
    RemoveAll();
}

///////////////////////////////////////////////
bool CInvestDB::IsFullInvest(const uint256& hashBlock)
{
    return ((CBlock::GetBlockHeightByHash(hashBlock) % FULL_HEIGHT) == 0);
}

bool CInvestDB::GetFullBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtInvest)
{
    if (!GetBlockFullInvest(hashPrev, ctxtInvest))
    {
        StdError("CInvestDB", "Get full block: Get prev invest fail");
        return false;
    }
    for (const auto& kv : vTxInvestIn)
    {
        if (kv.second.nInvestAmount > 0)
        {
            auto& invest = ctxtInvest.mapInvest[kv.first];
            if (invest.nCycle == 0)
            {
                invest.nCycle = kv.second.nCycle;
                invest.destReward = kv.second.destReward;
            }
            invest.nInvestAmount += kv.second.nInvestAmount;
            invest.nStartHeight = kv.second.nStartHeight;
        }
        else if (kv.second.nInvestAmount < 0)
        {
            ctxtInvest.mapInvest[kv.first].nInvestAmount += kv.second.nInvestAmount;
        }
    }
    for (auto it = ctxtInvest.mapInvest.begin(); it != ctxtInvest.mapInvest.end();)
    {
        if (it->second.nInvestAmount < 0)
        {
            StdError("CInvestDB", "Get full block: nInvestAmount < 0, nInvestAmount: %ld, ",
                     it->second.nInvestAmount);
            return false;
        }
        if (it->second.nInvestAmount == 0)
        {
            ctxtInvest.mapInvest.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    return true;
}

bool CInvestDB::GetIncrementBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtInvest)
{
    if (!GetInvest(hashPrev, ctxtInvest))
    {
        StdError("CInvestDB", "Get increment: Get invest fail");
        return false;
    }

    std::map<CDestination, CInvest> mapRefFullInvest;
    if (ctxtInvest.IsFull())
    {
        mapRefFullInvest = ctxtInvest.mapInvest;
        ctxtInvest.hashRef = hashPrev;
        ctxtInvest.mapInvest.clear();
    }
    else
    {
        if (!GetInvest(ctxtInvest.hashRef, mapRefFullInvest))
        {
            StdError("CInvestDB", "Get increment: Get invest fail");
            return false;
        }
    }

    for (const auto& kv : vTxInvestIn)
    {
        if (kv.second.nInvestAmount != 0)
        {
            auto it = ctxtInvest.mapInvest.find(kv.first);
            if (it == ctxtInvest.mapInvest.end())
            {
                it = ctxtInvest.mapInvest.insert(make_pair(kv.first, CInvest())).first;
                auto mt = mapRefFullInvest.find(kv.first);
                if (mt != mapRefFullInvest.end())
                {
                    it->second = mt->second;
                }
            }
            auto& invest = it->second;
            if (kv.second.nInvestAmount > 0)
            {
                if (invest.nCycle == 0)
                {
                    invest.nCycle = kv.second.nCycle;
                    invest.destReward = kv.second.destReward;
                }
                invest.nInvestAmount += kv.second.nInvestAmount;
                invest.nStartHeight = kv.second.nStartHeight;
            }
            else
            {
                invest.nInvestAmount += kv.second.nInvestAmount;
                if (invest.nInvestAmount < 0)
                {
                    StdError("CInvestDB", "Get increment: nInvestAmount < 0, nInvestAmount: %ld", invest.nInvestAmount);
                    return false;
                }
            }
        }
    }
    return true;
}

bool CInvestDB::GetBlockFullInvest(const uint256& hashBlock, CInvestContext& ctxtInvest)
{
    if (!GetInvest(hashBlock, ctxtInvest))
    {
        StdError("CInvestDB", "Get block full: Get invest fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (!ctxtInvest.IsFull())
    {
        std::map<CDestination, CInvest> mapIncInvest;
        mapIncInvest = ctxtInvest.mapInvest;
        ctxtInvest.mapInvest.clear();
        if (!GetInvest(ctxtInvest.hashRef, ctxtInvest.mapInvest))
        {
            StdError("CInvestDB", "Get block full: Get invest fail, ref: %s", ctxtInvest.hashRef.GetHex().c_str());
            return false;
        }
        for (const auto& kv : mapIncInvest)
        {
            ctxtInvest.mapInvest[kv.first] = kv.second;
        }
    }
    ctxtInvest.hashRef = 0;

    for (auto it = ctxtInvest.mapInvest.begin(); it != ctxtInvest.mapInvest.end();)
    {
        if (it->second.nInvestAmount < 0)
        {
            StdError("CInvestDB", "Get block full invest: nInvestAmount < 0, nInvestAmount: %ld, ",
                     it->second.nInvestAmount);
            return false;
        }
        if (it->second.nInvestAmount == 0)
        {
            ctxtInvest.mapInvest.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    return true;
}

bool CInvestDB::AddInvest(const uint256& hashBlock, const CInvestContext& ctxtInvest)
{
    if (!Write(hashBlock, ctxtInvest))
    {
        StdError("CInvestDB", "Add invest: Write fail");
        return false;
    }
    AddCache(hashBlock, ctxtInvest);
    return true;
}

bool CInvestDB::RemoveInvest(const uint256& hashBlock)
{
    if (hashBlock == 0)
    {
        return true;
    }
    if (fCache)
    {
        if (IsFullInvest(hashBlock))
        {
            mapCacheFullInvest.erase(hashBlock);
        }
        else
        {
            mapCacheIncInvest.erase(hashBlock);
        }
    }
    return Erase(hashBlock);
}

bool CInvestDB::GetInvest(const uint256& hashBlock, CInvestContext& ctxtInvest)
{
    if (hashBlock == 0)
    {
        return true;
    }
    if (fCache)
    {
        if (IsFullInvest(hashBlock))
        {
            auto it = mapCacheFullInvest.find(hashBlock);
            if (it == mapCacheFullInvest.end())
            {
                if (!Read(hashBlock, ctxtInvest))
                {
                    StdError("CInvestDB", "Get invest: Read fail");
                    return false;
                }
                AddCache(hashBlock, ctxtInvest);
            }
            else
            {
                ctxtInvest = it->second;
            }
        }
        else
        {
            auto it = mapCacheIncInvest.find(hashBlock);
            if (it == mapCacheIncInvest.end())
            {
                if (!Read(hashBlock, ctxtInvest))
                {
                    StdError("CInvestDB", "Get invest: Read fail");
                    return false;
                }
                AddCache(hashBlock, ctxtInvest);
            }
            else
            {
                ctxtInvest = it->second;
            }
        }
    }
    else
    {
        if (!Read(hashBlock, ctxtInvest))
        {
            StdError("CInvestDB", "Get invest: Read fail");
            return false;
        }
    }
    return true;
}

bool CInvestDB::GetInvest(const uint256& hashBlock, map<CDestination, CInvest>& mapInvestOut)
{
    mapInvestOut.clear();
    if (hashBlock == 0)
    {
        return true;
    }
    if (fCache)
    {
        if (IsFullInvest(hashBlock))
        {
            auto it = mapCacheFullInvest.find(hashBlock);
            if (it == mapCacheFullInvest.end())
            {
                CInvestContext ctxtInvest;
                if (!Read(hashBlock, ctxtInvest))
                {
                    StdError("CInvestDB", "Get invest: Read fail");
                    return false;
                }
                AddCache(hashBlock, ctxtInvest);
                mapInvestOut = ctxtInvest.mapInvest;
            }
            else
            {
                mapInvestOut = it->second.mapInvest;
            }
        }
        else
        {
            auto it = mapCacheIncInvest.find(hashBlock);
            if (it == mapCacheIncInvest.end())
            {
                CInvestContext ctxtInvest;
                if (!Read(hashBlock, ctxtInvest))
                {
                    StdError("CInvestDB", "Get invest: Read fail");
                    return false;
                }
                AddCache(hashBlock, ctxtInvest);
                mapInvestOut = ctxtInvest.mapInvest;
            }
            else
            {
                mapInvestOut = it->second.mapInvest;
            }
        }
    }
    else
    {
        CInvestContext ctxtInvest;
        if (!Read(hashBlock, ctxtInvest))
        {
            StdError("CInvestDB", "Get invest: Read fail");
            return false;
        }
        mapInvestOut = ctxtInvest.mapInvest;
    }
    return true;
}

void CInvestDB::AddCache(const uint256& hashBlock, const CInvestContext& ctxtInvest)
{
    if (fCache)
    {
        if (IsFullInvest(hashBlock))
        {
            auto it = mapCacheFullInvest.find(hashBlock);
            if (it == mapCacheFullInvest.end())
            {
                while (mapCacheFullInvest.size() >= MAX_FULL_CACHE_COUNT)
                {
                    mapCacheFullInvest.erase(mapCacheFullInvest.begin());
                }
                mapCacheFullInvest.insert(make_pair(hashBlock, ctxtInvest));
            }
        }
        else
        {
            auto it = mapCacheIncInvest.find(hashBlock);
            if (it == mapCacheIncInvest.end())
            {
                while (mapCacheIncInvest.size() >= MAX_INC_CACHE_COUNT)
                {
                    mapCacheIncInvest.erase(mapCacheIncInvest.begin());
                }
                mapCacheIncInvest.insert(make_pair(hashBlock, ctxtInvest));
            }
        }
    }
}

} // namespace storage
} // namespace ibrio
