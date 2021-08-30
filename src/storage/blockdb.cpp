// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockdb.h"

#include "stream/datastream.h"

using namespace std;

namespace ibrio
{
namespace storage
{

//////////////////////////////
// CBlockDB

CBlockDB::CBlockDB()
  : fDbCfgAddrTxIndex(false)
{
}

CBlockDB::~CBlockDB()
{
}

bool CBlockDB::Initialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn, const bool fAddrTxIndexIn)
{
    fDbCfgAddrTxIndex = fAddrTxIndexIn;
    if (!dbFork.Initialize(pathData, hashGenesisBlockIn))
    {
        return false;
    }

    if (!dbBlockIndex.Initialize(pathData))
    {
        return false;
    }

    if (!dbTxIndex.Initialize(pathData))
    {
        return false;
    }

    if (!dbUnspent.Initialize(pathData))
    {
        return false;
    }

    if (!dbDelegate.Initialize(pathData))
    {
        return false;
    }

    if (!dbAddress.Initialize(pathData))
    {
        return false;
    }

    if (!dbAddressUnspent.Initialize(pathData))
    {
        return false;
    }

    if (fDbCfgAddrTxIndex)
    {
        if (!dbAddressTxIndex.Initialize(pathData))
        {
            return false;
        }
    }

    if (!dbInvest.Initialize(pathData))
    {
        return false;
    }

    if (!dbActivate.Initialize(pathData))
    {
        return false;
    }
    return LoadFork();
}

void CBlockDB::Deinitialize()
{
    dbActivate.Deinitialize();
    dbInvest.Deinitialize();
    dbAddress.Deinitialize();
    dbAddressUnspent.Deinitialize();
    if (fDbCfgAddrTxIndex)
    {
        dbAddressTxIndex.Deinitialize();
    }
    dbDelegate.Deinitialize();
    dbUnspent.Deinitialize();
    dbTxIndex.Deinitialize();
    dbBlockIndex.Deinitialize();
    dbFork.Deinitialize();
}

bool CBlockDB::RemoveAll()
{
    dbActivate.Clear();
    dbInvest.Clear();
    dbAddress.Clear();
    dbAddressUnspent.Clear();
    if (fDbCfgAddrTxIndex)
    {
        dbAddressTxIndex.Clear();
    }
    dbDelegate.Clear();
    dbUnspent.Clear();
    dbTxIndex.Clear();
    dbBlockIndex.Clear();
    dbFork.Clear();

    return true;
}

bool CBlockDB::AddNewForkContext(const CForkContext& ctxt)
{
    return dbFork.AddNewForkContext(ctxt);
}

bool CBlockDB::RetrieveForkContext(const uint256& hash, CForkContext& ctxt)
{
    ctxt.SetNull();
    return dbFork.RetrieveForkContext(hash, ctxt);
}

bool CBlockDB::ListForkContext(vector<CForkContext>& vForkCtxt, map<uint256, CValidForkId>& mapValidForkId)
{
    vForkCtxt.clear();
    return dbFork.ListForkContext(vForkCtxt, mapValidForkId);
}

bool CBlockDB::AddValidForkHash(const uint256& hashBlock, const uint256& hashRefFdBlock, const map<uint256, int>& mapValidFork)
{
    return dbFork.AddValidForkHash(hashBlock, hashRefFdBlock, mapValidFork);
}

bool CBlockDB::RetrieveValidForkHash(const uint256& hashBlock, uint256& hashRefFdBlock, map<uint256, int>& mapValidFork)
{
    return dbFork.RetrieveValidForkHash(hashBlock, hashRefFdBlock, mapValidFork);
}

bool CBlockDB::AddNewFork(const uint256& hash)
{
    if (!dbFork.UpdateFork(hash))
    {
        return false;
    }

    if (!dbTxIndex.AddNewFork(hash))
    {
        RemoveFork(hash);
        return false;
    }

    if (!dbUnspent.AddNewFork(hash))
    {
        RemoveFork(hash);
        return false;
    }

    if (!dbAddress.AddNewFork(hash))
    {
        RemoveFork(hash);
        return false;
    }

    if (!dbAddressUnspent.AddNewFork(hash))
    {
        RemoveFork(hash);
        return false;
    }

    if (fDbCfgAddrTxIndex)
    {
        if (!dbAddressTxIndex.AddNewFork(hash))
        {
            RemoveFork(hash);
            return false;
        }
    }

    return true;
}

bool CBlockDB::RemoveFork(const uint256& hash)
{
    dbTxIndex.RemoveFork(hash);
    dbUnspent.RemoveFork(hash);
    dbAddress.RemoveFork(hash);
    dbAddressUnspent.RemoveFork(hash);
    if (fDbCfgAddrTxIndex)
    {
        dbAddressTxIndex.RemoveFork(hash);
    }
    return dbFork.RemoveFork(hash);
}

bool CBlockDB::ListFork(vector<pair<uint256, uint256>>& vFork)
{
    vFork.clear();
    return dbFork.ListFork(vFork);
}

bool CBlockDB::UpdateFork(const uint256& hash, const uint256& hashRefBlock, const uint256& hashForkBased,
                          const vector<pair<uint256, CTxIndex>>& vTxNew, const vector<uint256>& vTxDel,
                          const vector<pair<CAddrTxIndex, CAddrTxInfo>>& vAddrTxNew, const vector<CAddrTxIndex>& vAddrTxDel,
                          const vector<CTxUnspent>& vAddNewUnspent, const vector<CTxUnspent>& vRemoveUnspent)
{
    if (!dbUnspent.Exists(hash))
    {
        return false;
    }

    bool fIgnoreTxDel = false;
    if (hashForkBased != hash && hashForkBased != 0)
    {
        if (!dbUnspent.Copy(hashForkBased, hash))
        {
            return false;
        }
        if (!dbAddressUnspent.Copy(hashForkBased, hash))
        {
            return false;
        }
        fIgnoreTxDel = true;
    }

    if (!dbFork.UpdateFork(hash, hashRefBlock))
    {
        return false;
    }

    if (!dbTxIndex.Update(hash, vTxNew, fIgnoreTxDel ? vector<uint256>() : vTxDel))
    {
        return false;
    }

    if (!dbUnspent.Update(hash, vAddNewUnspent, vRemoveUnspent))
    {
        return false;
    }

    if (!dbAddressUnspent.UpdateAddressUnspent(hash, hashRefBlock, vAddNewUnspent, vRemoveUnspent))
    {
        return false;
    }

    if (fDbCfgAddrTxIndex)
    {
        if (!dbAddressTxIndex.UpdateAddressTxIndex(hash, vAddrTxNew, fIgnoreTxDel ? vector<CAddrTxIndex>() : vAddrTxDel))
        {
            return false;
        }
    }

    return true;
}

bool CBlockDB::AddNewBlock(const CBlockOutline& outline)
{
    return dbBlockIndex.AddNewBlock(outline);
}

bool CBlockDB::RemoveBlock(const uint256& hash)
{
    return dbBlockIndex.RemoveBlock(hash);
}

bool CBlockDB::UpdateDelegateContext(const uint256& hash, const CDelegateContext& ctxtDelegate)
{
    return dbDelegate.AddNew(hash, ctxtDelegate);
}

bool CBlockDB::UpdateAddressInfo(const uint256& hashFork, const vector<pair<CDestination, CAddrInfo>>& vNewAddress,
                                 const vector<CDestination>& vRemoveAddress)
{
    return dbAddress.Update(hashFork, vNewAddress, vRemoveAddress);
}

bool CBlockDB::GetAddressInfo(const uint256& hashFork, const CDestination& destIn, CAddrInfo& addrInfo)
{
    return dbAddress.Retrieve(hashFork, destIn, addrInfo);
}

bool CBlockDB::WalkThroughBlock(CBlockDBWalker& walker)
{
    return dbBlockIndex.WalkThroughBlock(walker);
}

bool CBlockDB::RetrieveTxIndex(const uint256& txid, CTxIndex& txIndex, uint256& fork)
{
    txIndex.SetNull();
    return dbTxIndex.Retrieve(txid, txIndex, fork);
}

bool CBlockDB::RetrieveTxIndex(const uint256& fork, const uint256& txid, CTxIndex& txIndex)
{
    txIndex.SetNull();
    return dbTxIndex.Retrieve(fork, txid, txIndex);
}

bool CBlockDB::RetrieveTxUnspent(const uint256& fork, const CTxOutPoint& out, CTxOut& unspent)
{
    return dbUnspent.Retrieve(fork, out, unspent);
}

bool CBlockDB::WalkThroughUnspent(const uint256& hashFork, CForkUnspentDBWalker& walker)
{
    return dbUnspent.WalkThrough(hashFork, walker);
}

bool CBlockDB::WalkThroughAddress(const uint256& hashFork, CForkAddressDBWalker& walker)
{
    return dbAddress.WalkThrough(hashFork, walker);
}

bool CBlockDB::RetrieveDelegateContext(const uint256& hashBlock, CDelegateContext& ctxtDelegate)
{
    return dbDelegate.Retrieve(hashBlock, ctxtDelegate);
}

bool CBlockDB::RetrieveDelegate(const uint256& hash, map<CDestination, int64>& mapDelegate)
{
    return dbDelegate.RetrieveDelegatedVote(hash, mapDelegate);
}

bool CBlockDB::RetrieveEnroll(const uint256& hash, std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTxPos)
{
    return dbDelegate.RetrieveDelegatedEnrollTx(hash, mapEnrollTxPos);
}

bool CBlockDB::RetrieveEnroll(int height, const vector<uint256>& vBlockRange,
                              map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    return dbDelegate.RetrieveEnrollTx(height, vBlockRange, mapEnrollTxPos);
}

bool CBlockDB::RetrieveAddressUnspent(const uint256& hashFork, const CDestination& dest, map<CTxOutPoint, CUnspentOut>& mapUnspent, uint256& hashLastBlockOut)
{
    return dbAddressUnspent.RetrieveAddressUnspent(hashFork, dest, mapUnspent, hashLastBlockOut);
}

int64 CBlockDB::RetrieveAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, map<CAddrTxIndex, CAddrTxInfo>& mapAddrTxIndex)
{
    if (fDbCfgAddrTxIndex)
    {
        return dbAddressTxIndex.RetrieveAddressTxIndex(hashFork, dest, nPrevHeight, nPrevTxSeq, nOffset, nCount, mapAddrTxIndex);
    }
    return -1;
}

bool CBlockDB::UpdateInvestContext(const uint256& hashBlock, const CInvestContext& ctxtInvest)
{
    return dbInvest.AddNew(hashBlock, ctxtInvest);
}

bool CBlockDB::RetrieveInvestContext(const uint256& hashBlock, CInvestContext& ctxtInvest)
{
    return dbInvest.Retrieve(hashBlock, ctxtInvest);
}

bool CBlockDB::AddBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, const std::vector<std::pair<CDestination, int64>>& vNoPackingRewardIn)
{
    return dbInvest.AddBlockInvest(hashBlock, hashPrev, vTxInvestIn, vNoPackingRewardIn);
}

bool CBlockDB::GetFullInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtFullInvestOut)
{
    return dbInvest.GetFullInvest(hashBlock, hashPrev, vTxInvestIn, ctxtFullInvestOut);
}

bool CBlockDB::UpdateActivateContext(const uint256& hashBlock, const CActivateContext& ctxtActivate)
{
    return dbActivate.AddNew(hashBlock, ctxtActivate);
}

bool CBlockDB::RetrieveActivateContext(const uint256& hashBlock, CActivateContext& ctxtActivate)
{
    return dbActivate.Retrieve(hashBlock, ctxtActivate);
}

bool CBlockDB::LoadFork()
{
    vector<pair<uint256, uint256>> vFork;
    if (!dbFork.ListFork(vFork))
    {
        return false;
    }

    for (int i = 0; i < vFork.size(); i++)
    {
        if (!dbTxIndex.LoadFork(vFork[i].first))
        {
            return false;
        }

        if (!dbUnspent.LoadFork(vFork[i].first))
        {
            return false;
        }

        if (!dbAddress.LoadFork(vFork[i].first))
        {
            return false;
        }

        if (!dbAddressUnspent.LoadFork(vFork[i].first, vFork[i].second))
        {
            return false;
        }

        if (fDbCfgAddrTxIndex)
        {
            if (!dbAddressTxIndex.LoadFork(vFork[i].first))
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace storage
} // namespace ibrio
