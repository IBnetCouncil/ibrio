// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockbase.h"

#include <boost/timer/timer.hpp>
#include <cstdio>

#include "../ibrio/address.h"
#include "delegatecomm.h"
#include "template/activate.h"
#include "template/delegate.h"
#include "template/template.h"
#include "template/vote.h"
#include "util.h"

using namespace std;
using namespace boost::filesystem;
using namespace xengine;

#define BLOCKFILE_PREFIX "block"
#define LOGFILE_NAME "storage.log"
namespace ibrio
{
namespace storage
{

//////////////////////////////
// CBlockBaseDBWalker

class CBlockWalker : public CBlockDBWalker
{
public:
    CBlockWalker(CBlockBase* pBaseIn)
      : pBase(pBaseIn) {}
    bool Walk(CBlockOutline& outline) override
    {
        return pBase->LoadIndex(outline);
    }

public:
    CBlockBase* pBase;
};

//////////////////////////////
// CBlockView

CBlockView::CBlockView()
  : pBlockBase(nullptr), hashFork(uint64(0)), fCommittable(false)
{
}

CBlockView::~CBlockView()
{
    Deinitialize();
}

void CBlockView::Initialize(CBlockBase* pBlockBaseIn, boost::shared_ptr<CBlockFork> spForkIn,
                            const uint256& hashForkIn, bool fCommittableIn)
{
    Deinitialize();

    pBlockBase = pBlockBaseIn;
    spFork = spForkIn;
    hashFork = hashForkIn;
    fCommittable = fCommittableIn;
    if (pBlockBase && spFork)
    {
        if (fCommittable)
        {
            spFork->UpgradeLock();
        }
        else
        {
            spFork->ReadLock();
        }
    }
    vTxRemove.clear();
    vAddrTxRemove.clear();
    vTxAddNew.clear();
}

void CBlockView::Deinitialize()
{
    if (pBlockBase)
    {
        if (spFork)
        {
            if (fCommittable)
            {
                spFork->UpgradeUnlock();
            }
            else
            {
                spFork->ReadUnlock();
            }
            spFork = nullptr;
        }
        pBlockBase = nullptr;
        hashFork = 0;
        mapTx.clear();
        mapUnspent.clear();
    }
}

bool CBlockView::ExistsTx(const uint256& txid) const
{
    map<uint256, CTransaction>::const_iterator it = mapTx.find(txid);
    if (it != mapTx.end())
    {
        return (!(*it).second.IsNull());
    }
    return (!!(pBlockBase->ExistsTx(txid)));
}

bool CBlockView::RetrieveTx(const uint256& txid, CTransaction& tx)
{
    map<uint256, CTransaction>::const_iterator it = mapTx.find(txid);
    if (it != mapTx.end())
    {
        tx = (*it).second;
        return (!tx.IsNull());
    }
    return pBlockBase->RetrieveTx(txid, tx);
}

bool CBlockView::RetrieveUnspent(const CTxOutPoint& out, CTxOut& unspent)
{
    map<CTxOutPoint, CViewUnspent>::const_iterator it = mapUnspent.find(out);
    if (it != mapUnspent.end())
    {
        if ((*it).second.IsSpent())
        {
            StdTrace("CBlockView", "RetrieveUnspent: unspent is spent, unspent: [%d]:%s", out.n, out.hash.GetHex().c_str());
            return false;
        }
        unspent = it->second.output;
    }
    else
    {
        if (!pBlockBase->GetTxUnspent(hashFork, out, unspent))
        {
            StdTrace("CBlockView", "RetrieveUnspent: Blockchain unspent don't exist, unspent: [%d]:%s", out.n, out.hash.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockView::AddTx(const uint256& txid, const CTransaction& tx, int nHeight, const CTxContxt& txContxt)
{
    mapTx[txid] = tx;
    vTxAddNew.push_back(txid);

    const CDestination& destIn = txContxt.destIn;
    int64 nValueIn = txContxt.GetValueIn();

    for (int i = 0; i < tx.vInput.size(); i++)
    {
        const CTxInContxt& txin = txContxt.vin[i];
        mapUnspent[tx.vInput[i].prevout].Disable(CTxOut(destIn, txin.nAmount, txin.nTxTime, txin.nLockUntil), -1, -1);
    }
    CTxOut output0(tx);
    if (!output0.IsNull())
    {
        mapUnspent[CTxOutPoint(txid, 0)].Enable(output0, tx.nType, nHeight);
    }
    CTxOut output1(tx, destIn, nValueIn);
    if (!output1.IsNull())
    {
        mapUnspent[CTxOutPoint(txid, 1)].Enable(output1, tx.nType, nHeight);
    }

    // relation
    if (tx.IsDeFiRelation())
    {
        CDestination root;
        if (!relationAddNew.CheckInsert(tx.sendTo, destIn, root))
        {
            StdLog("CBlockView", "AddTx relationAddNew.CheckInsert fail, addr: %s, parent: %s", tx.sendTo.ToString().c_str(), destIn.ToString().c_str());
            return false;
        }

        if (!spFork->GetRelation().CheckInsert(tx.sendTo, root, root, relationRemove))
        {
            StdLog("CBlockView", "AddTx spFork->GetRelation().CheckInsert fail, addr: %s, parent: %s", tx.sendTo.ToString().c_str(), destIn.ToString().c_str());
            return false;
        }

        relationAddNew.Insert(tx.sendTo, destIn, destIn, false);
    }

    return true;
}

void CBlockView::RemoveTx(const uint256& txid, const CTransaction& tx, const int nHeight, const int nBlockSeq, const int nTxSeq, const CTxContxt& txContxt, const bool fAddrTxIndexIn)
{
    mapTx[txid].SetNull();
    vTxRemove.push_back(txid);

    if (fAddrTxIndexIn)
    {
        if (tx.IsRewardTx())
        {
            vAddrTxRemove.push_back(CAddrTxIndex(tx.sendTo, nHeight, nBlockSeq, nTxSeq, txid));
        }
        else if (txContxt.destIn == tx.sendTo)
        {
            vAddrTxRemove.push_back(CAddrTxIndex(txContxt.destIn, nHeight, nBlockSeq, nTxSeq, txid));
        }
        else
        {
            if (!txContxt.destIn.IsNull())
            {
                vAddrTxRemove.push_back(CAddrTxIndex(txContxt.destIn, nHeight, nBlockSeq, nTxSeq, txid));
            }
            vAddrTxRemove.push_back(CAddrTxIndex(tx.sendTo, nHeight, nBlockSeq, nTxSeq, txid));
        }
    }

    for (int i = 0; i < tx.vInput.size(); i++)
    {
        const CTxInContxt& in = txContxt.vin[i];
        mapUnspent[tx.vInput[i].prevout].Enable(CTxOut(txContxt.destIn, in.nAmount, in.nTxTime, in.nLockUntil), -1, -1);
    }

    CTxOut output0(tx);
    if (!output0.IsNull())
    {
        mapUnspent[CTxOutPoint(txid, 0)].Disable(output0, tx.nType, nHeight);
    }
    CTxOut output1(tx, txContxt.destIn, txContxt.GetValueIn());
    if (!output1.IsNull())
    {
        mapUnspent[CTxOutPoint(txid, 1)].Disable(output1, tx.nType, nHeight);
    }

    // relation
    if (tx.IsDeFiRelation())
    {
        relationRemove.insert(tx.sendTo);
    }
}

void CBlockView::AddBlock(const uint256& hash, const CBlockEx& block)
{
    InsertBlockList(hash, block, vBlockAddNew);
}

void CBlockView::RemoveBlock(const uint256& hash, const CBlockEx& block)
{
    InsertBlockList(hash, block, vBlockRemove);
}

void CBlockView::GetUnspentChanges(vector<CTxUnspent>& vAddNew, vector<CTxOutPoint>& vRemove)
{
    vAddNew.reserve(mapUnspent.size());
    vRemove.reserve(mapUnspent.size());

    for (map<CTxOutPoint, CViewUnspent>::iterator it = mapUnspent.begin(); it != mapUnspent.end(); ++it)
    {
        const CTxOutPoint& out = (*it).first;
        CViewUnspent& unspent = (*it).second;
        if (unspent.IsModified())
        {
            if (!unspent.IsSpent())
            {
                vAddNew.push_back(CTxUnspent(out, unspent.output, unspent.nTxType, unspent.nHeight));
            }
            else
            {
                vRemove.push_back(out);
            }
        }
    }
}

void CBlockView::GetUnspentChanges(vector<CTxUnspent>& vAddNewUnspent, vector<CTxUnspent>& vRemoveUnspent)
{
    vAddNewUnspent.reserve(mapUnspent.size());
    vRemoveUnspent.reserve(mapUnspent.size());

    for (map<CTxOutPoint, CViewUnspent>::iterator it = mapUnspent.begin(); it != mapUnspent.end(); ++it)
    {
        const CTxOutPoint& out = (*it).first;
        CViewUnspent& unspent = (*it).second;
        if (unspent.IsModified())
        {
            if (!unspent.IsSpent())
            {
                if (unspent.nTxType == -1 || unspent.nHeight == -1)
                {
                    CTransaction tx;
                    uint256 hashFork;
                    int nHeight;
                    if (pBlockBase->RetrieveTx(out.hash, tx, hashFork, nHeight))
                    {
                        unspent.nTxType = tx.nType;
                        unspent.nHeight = nHeight;
                    }
                    else
                    {
                        StdError("CBlockView", "Get Unspent Changes: RetrieveTx fail, tx: %s", out.hash.GetHex().c_str());
                    }
                }
                vAddNewUnspent.push_back(CTxUnspent(out, unspent.output, unspent.nTxType, unspent.nHeight));
            }
            else
            {
                vRemoveUnspent.push_back(CTxUnspent(out, unspent.output, unspent.nTxType, unspent.nHeight));
            }
        }
    }
}

void CBlockView::GetTxUpdated(set<uint256>& setUpdate)
{
    for (int i = 0; i < vTxRemove.size(); i++)
    {
        const uint256& txid = vTxRemove[i];
        if (!mapTx[txid].IsNull())
        {
            setUpdate.insert(txid);
        }
    }
}

void CBlockView::GetTxRemoved(vector<uint256>& vRemove, vector<CAddrTxIndex>& vAddrTxIndexRemove, const bool fAddrTxIndexIn)
{
    vRemove.reserve(vTxRemove.size());
    for (size_t i = 0; i < vTxRemove.size(); i++)
    {
        const uint256& txid = vTxRemove[i];
        if (mapTx[txid].IsNull())
        {
            vRemove.push_back(txid);
        }
    }
    if (fAddrTxIndexIn)
    {
        vAddrTxIndexRemove.reserve(vAddrTxRemove.size());
        for (size_t i = 0; i < vAddrTxRemove.size(); i++)
        {
            const CAddrTxIndex& addrTxIndex = vAddrTxRemove[i];
            if (mapTx[addrTxIndex.txid].IsNull())
            {
                vAddrTxIndexRemove.push_back(addrTxIndex);
            }
        }
    }
}

void CBlockView::GetBlockChanges(vector<CBlockEx>& vAdd, vector<CBlockEx>& vRemove) const
{
    vAdd.clear();
    vAdd.reserve(vBlockAddNew.size());
    for (auto& pair : vBlockAddNew)
    {
        vAdd.push_back(pair.second);
    }

    vRemove.clear();
    vRemove.reserve(vBlockRemove.size());
    for (auto& pair : vBlockRemove)
    {
        vRemove.push_back(pair.second);
    }
}

void CBlockView::InsertBlockList(const uint256& hash, const CBlockEx& block, list<pair<uint256, CBlockEx>>& blockList)
{
    // store reserve block order
    auto pair = make_pair(hash, block);
    if (blockList.empty())
    {
        blockList.push_back(pair);
    }
    else if (block.hashPrev == blockList.front().first)
    {
        blockList.push_front(pair);
    }
    else if (blockList.back().second.hashPrev == hash)
    {
        blockList.push_back(pair);
    }
    else
    {
        StdError("CBlockView", "InsertBlockList error, no prev and next of block: %s", hash.ToString().c_str());
    }
}

//////////////////////////////
// CForkHeightIndex

void CForkHeightIndex::AddHeightIndex(uint32 nHeight, const uint256& hashBlock, uint32 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock)
{
    mapHeightIndex[nHeight][hashBlock] = CBlockHeightIndex(nBlockTimeStamp, destMint, hashRefBlock);
}

void CForkHeightIndex::RemoveHeightIndex(uint32 nHeight, const uint256& hashBlock)
{
    auto it = mapHeightIndex.find(nHeight);
    if (it != mapHeightIndex.end())
    {
        it->second.erase(hashBlock);
    }
}

void CForkHeightIndex::UpdateBlockRef(int nHeight, const uint256& hashBlock, const uint256& hashRefBlock)
{
    auto it = mapHeightIndex.find(nHeight);
    if (it != mapHeightIndex.end())
    {
        auto mt = it->second.find(hashBlock);
        if (mt != it->second.end())
        {
            mt->second.hashRefBlock = hashRefBlock;
        }
    }
}

map<uint256, CBlockHeightIndex>* CForkHeightIndex::GetBlockMintList(uint32 nHeight)
{
    auto it = mapHeightIndex.find(nHeight);
    if (it != mapHeightIndex.end())
    {
        return &(it->second);
    }
    return nullptr;
}

//////////////////////////////
// CBlockBase

CBlockBase::CBlockBase()
  : fDebugLog(false), fCfgAddrTxIndex(false)
{
}

CBlockBase::~CBlockBase()
{
    dbBlock.Deinitialize();
    tsBlock.Deinitialize();
}

bool CBlockBase::Initialize(const path& pathDataLocation, const uint256& hashGenesisBlockIn, const bool fDebug, const bool fAddrTxIndexIn, const uint32 nMaxBlockRewardTxCountIn, const bool fRenewDB)
{
    fCfgAddrTxIndex = fAddrTxIndexIn;
    nMaxBlockRewardTxCount = nMaxBlockRewardTxCountIn;
    hashGenesisBlock = hashGenesisBlockIn;

    if (!SetupLog(pathDataLocation, fDebug))
    {
        return false;
    }

    Log("B", "Initializing... (Path : %s)", pathDataLocation.string().c_str());

    if (!dbBlock.Initialize(pathDataLocation, hashGenesisBlockIn, fAddrTxIndexIn))
    {
        Error("B", "Failed to initialize block db");
        return false;
    }

    if (!tsBlock.Initialize(pathDataLocation / "block", BLOCKFILE_PREFIX))
    {
        dbBlock.Deinitialize();
        Error("B", "Failed to initialize block tsfile");
        return false;
    }

    if (fRenewDB)
    {
        Clear();
    }
    else if (!LoadDB())
    {
        dbBlock.Deinitialize();
        tsBlock.Deinitialize();
        {
            CWriteLock wlock(rwAccess);

            ClearCache();
        }
        Error("B", "Failed to load block db");
        return false;
    }
    Log("B", "Initialized");
    return true;
}

void CBlockBase::Deinitialize()
{
    dbBlock.Deinitialize();
    tsBlock.Deinitialize();
    {
        CWriteLock wlock(rwAccess);

        ClearCache();
    }
    Log("B", "Deinitialized");
}

bool CBlockBase::Exists(const uint256& hash) const
{
    CReadLock rlock(rwAccess);

    return (!!mapIndex.count(hash));
}

bool CBlockBase::ExistsTx(const uint256& txid)
{
    uint256 hashFork;
    CTxIndex txIndex;
    return dbBlock.RetrieveTxIndex(txid, txIndex, hashFork);
}

bool CBlockBase::IsEmpty() const
{
    CReadLock rlock(rwAccess);

    return mapIndex.empty();
}

void CBlockBase::Clear()
{
    CWriteLock wlock(rwAccess);

    dbBlock.RemoveAll();
    ClearCache();
}

bool CBlockBase::Initiate(const uint256& hashGenesis, const CBlock& blockGenesis, const uint256& nChainTrust)
{
    if (!IsEmpty())
    {
        StdTrace("BlockBase", "Is not empty");
        return false;
    }
    uint32 nFile, nOffset;
    if (!tsBlock.Write(CBlockEx(blockGenesis, std::vector<CTxContxt>(), nChainTrust), nFile, nOffset))
    {
        StdTrace("BlockBase", "Write genesis %s block failed", hashGenesis.ToString().c_str());
        return false;
    }

    uint32 nTxOffset = nOffset + blockGenesis.GetTxSerializedOffset();
    uint256 txidMintTx = blockGenesis.txMint.GetHash();

    vector<pair<uint256, CTxIndex>> vTxNew;
    vector<pair<CAddrTxIndex, CAddrTxInfo>> vAddrTxNew;
    vTxNew.push_back(make_pair(txidMintTx, CTxIndex(0, nFile, nTxOffset)));

    CAddrTxInfo txInfo(CAddrTxInfo::TXI_DIRECTION_TO, CDestination(), blockGenesis.txMint);
    vAddrTxNew.push_back(make_pair(CAddrTxIndex(blockGenesis.txMint.sendTo, 0, 0, 0, txidMintTx), txInfo));

    vector<CTxUnspent> vAddNew;
    vAddNew.push_back(CTxUnspent(CTxOutPoint(txidMintTx, 0), CTxOut(blockGenesis.txMint), blockGenesis.txMint.nType, blockGenesis.GetBlockHeight()));

    {
        CWriteLock wlock(rwAccess);
        CBlockIndex* pIndexNew = AddNewIndex(hashGenesis, blockGenesis, nFile, nOffset, nChainTrust);
        if (pIndexNew == nullptr)
        {
            StdTrace("BlockBase", "Add New Index %s block failed", hashGenesis.ToString().c_str());
            return false;
        }

        if (!dbBlock.AddNewBlock(CBlockOutline(pIndexNew)))
        {
            StdTrace("BlockBase", "Add New genesis Block %s block failed", hashGenesis.ToString().c_str());
            return false;
        }

        CDelegateContext ctxtDelegate;
        if (!dbBlock.UpdateDelegateContext(hashGenesis, ctxtDelegate))
        {
            StdTrace("BlockBase", "Update Delegate Contetxt %s block failed", hashGenesis.ToString().c_str());
            return false;
        }

        CInvestContext ctxtInvest;
        if (!dbBlock.UpdateInvestContext(hashGenesis, ctxtInvest))
        {
            StdTrace("BlockBase", "Update Invest Contetxt %s block failed", hashGenesis.ToString().c_str());
            return false;
        }

        CActivateContext ctxtActivate;
        if (!dbBlock.UpdateActivateContext(hashGenesis, ctxtActivate))
        {
            StdTrace("BlockBase", "Update Activate Contetxt %s block failed", hashGenesis.ToString().c_str());
            return false;
        }

        CProfile profile;
        if (!profile.Load(blockGenesis.vchProof))
        {
            StdTrace("BlockBase", "Load genesis %s block Proof failed", hashGenesis.ToString().c_str());
            return false;
        }

        CForkContext ctxt(hashGenesis, uint64(0), uint64(0), profile);
        if (!dbBlock.AddNewForkContext(ctxt))
        {
            StdTrace("BlockBase", "Add New Fork COntext %s block failed", hashGenesis.ToString().c_str());
            return false;
        }

        bool fCheckValidForkResult = true;
        uint256 hashRefFdBlock;
        map<uint256, int> mapValidFork;
        if (!dbBlock.RetrieveValidForkHash(hashGenesisBlock, hashRefFdBlock, mapValidFork))
        {
            fCheckValidForkResult = false;
        }
        else
        {
            if (hashRefFdBlock != 0)
            {
                fCheckValidForkResult = false;
            }
            else
            {
                const auto it = mapValidFork.find(hashGenesisBlock);
                if (it == mapValidFork.end() || it->second != 0)
                {
                    fCheckValidForkResult = false;
                }
            }
        }
        if (!fCheckValidForkResult)
        {
            map<uint256, int> mapValidFork;
            mapValidFork.insert(make_pair(hashGenesis, 0));
            if (!dbBlock.AddValidForkHash(hashGenesis, uint256(), mapValidFork))
            {
                StdTrace("BlockBase", "Add valid genesis fork fail");
                return false;
            }
        }

        if (!dbBlock.AddNewFork(hashGenesis))
        {
            StdTrace("BlockBase", "Add New Fork %s  failed", hashGenesis.ToString().c_str());
            return false;
        }

        boost::shared_ptr<CBlockFork> spFork = AddNewFork(profile, pIndexNew);
        if (spFork != nullptr)
        {
            CWriteLock wForkLock(spFork->GetRWAccess());

            if (!dbBlock.UpdateFork(hashGenesis, hashGenesis, uint64(0), vTxNew, vector<uint256>(), vAddrTxNew, vector<CAddrTxIndex>(), vAddNew, vector<CTxUnspent>()))
            {
                StdTrace("BlockBase", "Update Fork %s failed", hashGenesis.ToString().c_str());
                return false;
            }
            spFork->UpdateLast(pIndexNew);
        }
        else
        {
            StdTrace("BlockBase", "Add New Fork profile  %s  failed", hashGenesis.ToString().c_str());
            return false;
        }

        Log("B", "Initiate genesis %s", hashGenesis.ToString().c_str());
    }
    return true;
}

bool CBlockBase::AddNew(const uint256& hash, CBlockEx& block, CBlockIndex** ppIndexNew, const uint256& nChainTrust, int64 nMinEnrollAmount)
{
    if (Exists(hash))
    {
        StdTrace("BlockBase", "Add new block: Exist Block: %s", hash.ToString().c_str());
        return false;
    }

    CDelegateContext ctxtDelegate;
    if (block.IsPrimary())
    {
        if (!VerifyDelegateVote(hash, block, nMinEnrollAmount, ctxtDelegate))
        {
            StdError("BlockBase", "Add new block: Verify delegate vote fail, block: %s", hash.ToString().c_str());
            return false;
        }
    }

    uint32 nFile, nOffset;
    if (!tsBlock.Write(block, nFile, nOffset))
    {
        StdError("BlockBase", "Add new block: write block failed, block: %s", hash.ToString().c_str());
        return false;
    }
    {
        CWriteLock wlock(rwAccess);

        CBlockIndex* pIndexNew = AddNewIndex(hash, block, nFile, nOffset, nChainTrust);
        if (pIndexNew == nullptr)
        {
            StdError("BlockBase", "Add new block: AddNewIndex faild, block: %s", hash.ToString().c_str());
            return false;
        }

        if (!dbBlock.AddNewBlock(CBlockOutline(pIndexNew)))
        {
            StdError("BlockBase", "Add new block: AddNewBlock failed, block: %s", hash.ToString().c_str());
            //mapIndex.erase(hash);
            RemoveBlockIndex(pIndexNew->GetOriginHash(), hash);
            delete pIndexNew;
            return false;
        }

        if (pIndexNew->IsPrimary())
        {
            if (!UpdateDelegate(hash, block, CDiskPos(nFile, nOffset), ctxtDelegate))
            {
                StdTrace("BlockBase", "Add new block: Update delegate failed, block: %s", hash.ToString().c_str());
                dbBlock.RemoveBlock(hash);
                RemoveBlockIndex(pIndexNew->GetOriginHash(), hash);
                delete pIndexNew;
                return false;
            }
            if (!UpdateInvest(hash, block))
            {
                StdTrace("BlockBase", "Add new block: Update invest failed, block: %s", hash.ToString().c_str());
                dbBlock.RemoveBlock(hash);
                RemoveBlockIndex(pIndexNew->GetOriginHash(), hash);
                delete pIndexNew;
                return false;
            }
            if (!UpdateActivate(hash, block))
            {
                StdTrace("BlockBase", "Add new block: Update activate failed, block: %s", hash.ToString().c_str());
                dbBlock.RemoveBlock(hash);
                RemoveBlockIndex(pIndexNew->GetOriginHash(), hash);
                delete pIndexNew;
                return false;
            }
        }

        *ppIndexNew = pIndexNew;
    }

    Log("B", "AddNew block, hash=%s", hash.ToString().c_str());
    return true;
}

bool CBlockBase::AddNewForkContext(const CForkContext& ctxt)
{
    if (!dbBlock.AddNewForkContext(ctxt))
    {
        Error("F", "Failed to addnew forkcontext in %s", ctxt.hashFork.GetHex().c_str());
        return false;
    }
    Log("F", "AddNew forkcontext,hash=%s", ctxt.hashFork.GetHex().c_str());
    return true;
}

bool CBlockBase::AddValidForkHash(const uint256& hashBlock, const uint256& hashRefFdBlock, const map<uint256, int>& mapValidFork)
{
    return dbBlock.AddValidForkHash(hashBlock, hashRefFdBlock, mapValidFork);
}

bool CBlockBase::Retrieve(const uint256& hash, CBlock& block)
{
    block.SetNull();

    CBlockIndex* pIndex;
    {
        CReadLock rlock(rwAccess);

        if (!(pIndex = GetIndex(hash)))
        {
            StdTrace("BlockBase", "Retrieve::GetIndex %s block failed", hash.ToString().c_str());
            return false;
        }
    }
    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset, false))
    {
        StdTrace("BlockBase", "Retrieve::Read %s block failed", hash.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::Retrieve(const CBlockIndex* pIndex, CBlock& block)
{
    block.SetNull();

    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset, false))
    {
        StdTrace("BlockBase", "RetrieveFromIndex::Read %s block failed, File: %d, Offset: %d",
                 pIndex->GetBlockHash().ToString().c_str(), pIndex->nFile, pIndex->nOffset);
        return false;
    }
    return true;
}

bool CBlockBase::Retrieve(const uint256& hash, CBlockEx& block)
{
    block.SetNull();

    CBlockIndex* pIndex;
    {
        CReadLock rlock(rwAccess);

        if (!(pIndex = GetIndex(hash)))
        {
            StdTrace("BlockBase", "RetrieveBlockEx::GetIndex %s block failed", hash.ToString().c_str());
            return false;
        }
    }
    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset))
    {
        StdTrace("BlockBase", "RetrieveBlockEx::Read %s block failed", hash.ToString().c_str());

        return false;
    }
    return true;
}

bool CBlockBase::Retrieve(const CBlockIndex* pIndex, CBlockEx& block)
{
    block.SetNull();

    if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset))
    {
        StdTrace("BlockBase", "RetrieveFromIndex::GetIndex %s block failed", pIndex->GetBlockHash().ToString().c_str());

        return false;
    }
    return true;
}

bool CBlockBase::RetrieveIndex(const uint256& hash, CBlockIndex** ppIndex)
{
    CReadLock rlock(rwAccess);

    *ppIndex = GetIndex(hash);
    return (*ppIndex != nullptr);
}

bool CBlockBase::RetrieveFork(const uint256& hash, CBlockIndex** ppIndex)
{
    CReadLock rlock(rwAccess);

    boost::shared_ptr<CBlockFork> spFork = GetFork(hash);
    if (spFork != nullptr)
    {
        CReadLock rForkLock(spFork->GetRWAccess());

        *ppIndex = spFork->GetLast();

        return true;
    }

    return false;
}

bool CBlockBase::RetrieveFork(const string& strName, CBlockIndex** ppIndex)
{
    CReadLock rlock(rwAccess);

    boost::shared_ptr<CBlockFork> spFork = GetFork(strName);
    if (spFork != nullptr)
    {
        CReadLock rForkLock(spFork->GetRWAccess());

        *ppIndex = spFork->GetLast();

        return true;
    }

    return false;
}

bool CBlockBase::RetrieveProfile(const uint256& hash, CProfile& profile)
{
    CReadLock rlock(rwAccess);

    boost::shared_ptr<CBlockFork> spFork = GetFork(hash);
    if (spFork == nullptr)
    {
        return false;
    }

    profile = spFork->GetProfile();

    return true;
}

bool CBlockBase::RetrieveForkContext(const uint256& hash, CForkContext& ctxt)
{
    return dbBlock.RetrieveForkContext(hash, ctxt);
}

bool CBlockBase::RetrieveAncestry(const uint256& hash, vector<pair<uint256, uint256>> vAncestry)
{
    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hash, ctxt))
    {
        StdTrace("BlockBase", "Ancestry Retrieve hashFork %s failed", hash.ToString().c_str());
        return false;
    }

    while (ctxt.hashParent != 0)
    {
        vAncestry.push_back(make_pair(ctxt.hashParent, ctxt.hashJoint));
        if (!dbBlock.RetrieveForkContext(ctxt.hashParent, ctxt))
        {
            return false;
        }
    }

    std::reverse(vAncestry.begin(), vAncestry.end());
    return true;
}

bool CBlockBase::RetrieveOrigin(const uint256& hash, CBlock& block)
{
    block.SetNull();

    CForkContext ctxt;
    if (!dbBlock.RetrieveForkContext(hash, ctxt))
    {
        StdTrace("BlockBase", "RetrieveOrigin::RetrieveForkContext %s block failed", hash.ToString().c_str());
        return false;
    }

    CTransaction tx;
    if (!RetrieveTx(ctxt.txidEmbedded, tx))
    {
        StdTrace("BlockBase", "RetrieveOrigin::RetrieveTx %s tx failed", ctxt.txidEmbedded.ToString().c_str());
        return false;
    }

    try
    {
        CBufStream ss;
        ss.Write((const char*)tx.vchData.data(), tx.vchData.size());
        ss >> block;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveTx(const uint256& txid, CTransaction& tx)
{
    tx.SetNull();
    uint256 hashFork;
    CTxIndex txIndex;
    if (!dbBlock.RetrieveTxIndex(txid, txIndex, hashFork))
    {
        StdTrace("BlockBase", "RetrieveTx::RetrieveTxIndex %s tx failed", txid.ToString().c_str());
        return false;
    }

    if (!tsBlock.Read(tx, txIndex.nFile, txIndex.nOffset))
    {
        StdTrace("BlockBase", "RetrieveTx::Read %s tx failed", txid.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveTx(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight)
{
    tx.SetNull();
    CTxIndex txIndex;
    if (!dbBlock.RetrieveTxIndex(txid, txIndex, hashFork))
    {
        StdTrace("BlockBase", "RetrieveTx::RetrieveTxIndex %s tx failed", txid.ToString().c_str());
        return false;
    }
    if (!tsBlock.Read(tx, txIndex.nFile, txIndex.nOffset))
    {
        StdTrace("BlockBase", "RetrieveTx::Read %s tx failed", txid.ToString().c_str());
        return false;
    }
    nHeight = txIndex.nBlockHeight;
    return true;
}

bool CBlockBase::RetrieveTx(const uint256& hashFork, const uint256& txid, CTransaction& tx)
{
    tx.SetNull();

    CTxIndex txIndex;
    if (!dbBlock.RetrieveTxIndex(hashFork, txid, txIndex))
    {
        StdTrace("BlockBase", "RetrieveTxFromFork::RetrieveTxIndex fork:%s txid: %s tx failed",
                 hashFork.ToString().c_str(), txid.ToString().c_str());
        return false;
    }

    if (!tsBlock.Read(tx, txIndex.nFile, txIndex.nOffset))
    {
        StdTrace("BlockBase", "RetrieveTxFromFork::Read %s tx failed",
                 txid.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::RetrieveTxLocation(const uint256& txid, uint256& hashFork, int& nHeight)
{
    CTxIndex txIndex;
    if (!dbBlock.RetrieveTxIndex(txid, txIndex, hashFork))
    {
        StdTrace("BlockBase", "RetrieveTxLocation::RetrieveTxIndex %s tx failed",
                 txid.ToString().c_str());
        return false;
    }

    nHeight = txIndex.nBlockHeight;
    return true;
}

bool CBlockBase::RetrieveAvailDelegate(const uint256& hash, int height, const vector<uint256>& vBlockRange,
                                       int64 nMinEnrollAmount,
                                       map<CDestination, size_t>& mapWeight,
                                       map<CDestination, vector<unsigned char>>& mapEnrollData,
                                       vector<pair<CDestination, int64>>& vecAmount)
{
    map<CDestination, int64> mapVote;
    if (!dbBlock.RetrieveDelegate(hash, mapVote))
    {
        StdTrace("BlockBase", "RetrieveAvailDelegate::RetrieveDelegate %s block failed",
                 hash.ToString().c_str());
        return false;
    }
    // for (const auto d : mapVote)
    // {
    //     StdTrace("BlockBase", "RetrieveAvailDelegate mapVote: height: %d, dest: %s, vote: %.6f",
    //              height, CAddress(d.first).ToString().c_str(), ValueFromToken(d.second));
    // }

    map<CDestination, CDiskPos> mapEnrollTxPos;
    if (!dbBlock.RetrieveEnroll(height, vBlockRange, mapEnrollTxPos))
    {
        StdTrace("BlockBase", "RetrieveAvailDelegate::RetrieveEnroll block %s height %d failed",
                 hash.ToString().c_str(), height);
        return false;
    }
    // for (const auto d : mapEnrollTxPos)
    // {
    //     StdTrace("BlockBase", "RetrieveAvailDelegate mapEnrollTxPos: height: %d, dest: %s",
    //              height, CAddress(d.first).ToString().c_str());
    // }

    map<pair<int64, CDiskPos>, pair<CDestination, vector<uint8>>> mapSortEnroll;
    for (map<CDestination, int64>::iterator it = mapVote.begin(); it != mapVote.end(); ++it)
    {
        // StdTrace("BlockBase", "RetrieveAvailDelegate mapVote dest: %s, amount: %llu, minAmount: %llu, txpos find: %d",
        //          CAddress(it->first).ToString().c_str(), it->second, nMinEnrollAmount, mapEnrollTxPos.find(it->first) == mapEnrollTxPos.end());
        if ((*it).second >= nMinEnrollAmount)
        {
            const CDestination& dest = (*it).first;
            map<CDestination, CDiskPos>::iterator mi = mapEnrollTxPos.find(dest);
            if (mi != mapEnrollTxPos.end())
            {
                CTransaction tx;
                if (!tsBlock.Read(tx, (*mi).second))
                {
                    StdLog("BlockBase", "RetrieveAvailDelegate::Read %s tx failed", tx.GetHash().ToString().c_str());
                    return false;
                }

                if (tx.vchData.size() <= sizeof(int))
                {
                    StdLog("CBlockBase", "RetrieveAvailDelegate: tx.vchData error, txid: %s", tx.GetHash().ToString().c_str());
                    return false;
                }
                std::vector<uint8> vchCertData;
                vchCertData.assign(tx.vchData.begin() + sizeof(int), tx.vchData.end());

                mapSortEnroll.insert(make_pair(make_pair(it->second, mi->second), make_pair(dest, vchCertData)));
            }
        }
    }
    // for (const auto d : mapSortEnroll)
    // {
    //     StdTrace("BlockBase", "RetrieveAvailDelegate mapSortEnroll dest: %s, amount: %llu, data: %s",
    //              CAddress(d.second.first).ToString().c_str(), d.first.first, xengine::ToHexString(d.second.second).c_str());
    // }
    // first 25 destination sorted by amount and sequence
    for (auto it = mapSortEnroll.rbegin(); it != mapSortEnroll.rend() && mapWeight.size() < MAX_DELEGATE_THRESH; it++)
    {
        mapWeight.insert(make_pair(it->second.first, 1));
        mapEnrollData.insert(make_pair(it->second.first, it->second.second));
        vecAmount.push_back(make_pair(it->second.first, it->first.first));
    }
    for (const auto& d : vecAmount)
    {
        StdTrace("BlockBase", "RetrieveAvailDelegate: dest: %s, amount: %.6f",
                 CAddress(d.first).ToString().c_str(), ValueFromToken(d.second));
    }
    return true;
}

void CBlockBase::ListForkIndex(multimap<int, CBlockIndex*>& mapForkIndex)
{
    CReadLock rlock(rwAccess);

    mapForkIndex.clear();
    for (map<uint256, boost::shared_ptr<CBlockFork>>::iterator it = mapFork.begin(); it != mapFork.end(); ++it)
    {
        CReadLock rForkLock((*it).second->GetRWAccess());

        CBlockIndex* pIndex = (*it).second->GetLast();
        mapForkIndex.insert(make_pair(pIndex->pOrigin->GetBlockHeight() - 1, pIndex));
    }
}

bool CBlockBase::GetBlockView(CBlockView& view)
{
    boost::shared_ptr<CBlockFork> spFork;
    view.Initialize(this, spFork, uint64(0), false);
    return true;
}

bool CBlockBase::GetBlockView(const uint256& hash, CBlockView& view, bool fCommitable, bool fGetBranchBlock)
{
    CBlockIndex* pIndex = nullptr;
    uint256 hashOrigin;
    boost::shared_ptr<CBlockFork> spFork;

    {
        CReadLock rlock(rwAccess);
        pIndex = GetIndex(hash);
        if (pIndex == nullptr)
        {
            StdTrace("BlockBase", "GetBlockView::GetIndex %s block failed", hash.ToString().c_str());
            return false;
        }

        hashOrigin = pIndex->GetOriginHash();
        spFork = GetFork(hashOrigin);
        if (spFork == nullptr)
        {
            StdTrace("BlockBase", "GetBlockView::GetFork %s  failed", hashOrigin.ToString().c_str());
            return false;
        }
    }

    view.Initialize(this, spFork, hashOrigin, fCommitable);

    if (fGetBranchBlock)
    {
        CReadLock rlock(rwAccess);
        CBlockIndex* pForkLast = spFork->GetLast();

        vector<CBlockIndex*> vPath;
        CBlockIndex* pBranch = GetBranch(pForkLast, pIndex, vPath);

        uint64 nBlockRemoved = 0;
        uint64 nTxRemoved = 0;
        for (CBlockIndex* p = pForkLast; p != pBranch; p = p->pPrev)
        {
            // remove block tx;
            StdTrace("BlockBase",
                     "Chain rollback attempt[removed block]: height: %u hash: %s time: %u supply: %u algo: %u bits: %u trust: %s",
                     p->nHeight, p->GetBlockHash().ToString().c_str(), p->nTimeStamp,
                     p->nMoneySupply, p->nProofAlgo, p->nProofBits, p->nChainTrust.ToString().c_str());
            ++nBlockRemoved;
            CBlockEx block;
            if (!tsBlock.Read(block, p->nFile, p->nOffset))
            {
                StdTrace("BlockBase",
                         "Chain rollback attempt[remove]: Failed to read block`%s` from file",
                         p->GetBlockHash().ToString().c_str());
                return false;
            }
            int nBlockSeq = 0;
            if (fCfgAddrTxIndex && p->IsExtended())
            {
                nBlockSeq = p->GetExtendedSequence();
            }
            for (int j = block.vtx.size() - 1; j >= 0; j--)
            {
                StdTrace("BlockBase",
                         "Chain rollback attempt[removed tx]: %s",
                         block.vtx[j].GetHash().ToString().c_str());
                view.RemoveTx(block.vtx[j].GetHash(), block.vtx[j], block.GetBlockHeight(), nBlockSeq, j + 1, block.vTxContxt[j], fCfgAddrTxIndex);
                ++nTxRemoved;
            }
            if (!block.txMint.sendTo.IsNull())
            {
                StdTrace("BlockBase",
                         "Chain rollback attempt[removed mint tx]: %s",
                         block.txMint.GetHash().ToString().c_str());
                view.RemoveTx(block.txMint.GetHash(), block.txMint, block.GetBlockHeight(), nBlockSeq, 0, CTxContxt(), fCfgAddrTxIndex);
                ++nTxRemoved;
            }
            view.RemoveBlock(p->GetBlockHash(), block);
        }
        StdTrace("BlockBase",
                 "Chain rollback attempt[removed block amount]: %lu, [removed tx amount]: %lu",
                 nBlockRemoved, nTxRemoved);

        uint64 nBlockAdded = 0;
        uint64 nTxAdded = 0;
        for (int i = vPath.size() - 1; i >= 0; i--)
        {
            // add block tx;
            StdTrace("BlockBase",
                     "Chain rollback attempt[added block]: height: %u hash: %s time: %u supply: %u algo: %u bits: %u trust: %s",
                     vPath[i]->nHeight, vPath[i]->GetBlockHash().ToString().c_str(),
                     vPath[i]->nTimeStamp, vPath[i]->nMoneySupply, vPath[i]->nProofAlgo,
                     vPath[i]->nProofBits, vPath[i]->nChainTrust.ToString().c_str());
            ++nBlockAdded;
            CBlockEx block;
            if (!tsBlock.Read(block, vPath[i]->nFile, vPath[i]->nOffset))
            {
                StdTrace("BlockBase",
                         "Chain rollback attempt[add]: Failed to read block`%s` from file",
                         vPath[i]->GetBlockHash().ToString().c_str());
                return false;
            }
            if (!block.txMint.sendTo.IsNull())
            {
                view.AddTx(block.txMint.GetHash(), block.txMint, block.GetBlockHeight(), CTxContxt());
            }
            ++nTxAdded;
            for (int j = 0; j < block.vtx.size(); j++)
            {
                StdTrace("BlockBase",
                         "Chain rollback attempt[added tx]: %s",
                         block.vtx[j].GetHash().ToString().c_str());
                const CTxContxt& txContxt = block.vTxContxt[j];
                view.AddTx(block.vtx[j].GetHash(), block.vtx[j], block.GetBlockHeight(), txContxt);
                ++nTxAdded;
            }
            view.AddBlock(vPath[i]->GetBlockHash(), block);
        }
        StdTrace("BlockBase",
                 "Chain rollback attempt[added block amount]: %lu, [added tx amount]: %lu",
                 nBlockAdded, nTxAdded);
    }
    return true;
}

bool CBlockBase::GetForkBlockView(const uint256& hashFork, CBlockView& view)
{
    boost::shared_ptr<CBlockFork> spFork;
    {
        CReadLock rlock(rwAccess);
        spFork = GetFork(hashFork);
        if (spFork == nullptr)
        {
            return false;
        }
    }
    view.Initialize(this, spFork, hashFork, false);
    return true;
}

bool CBlockBase::CommitBlockView(CBlockView& view, CBlockIndex* pIndexNew)
{
    const uint256 hashFork = pIndexNew->GetOriginHash();

    boost::shared_ptr<CBlockFork> spFork;

    if (hashFork == view.GetForkHash())
    {
        if (!view.IsCommittable())
        {
            StdTrace("BlockBase", "CommitBlockView Is not COmmitable");
            return false;
        }
        spFork = view.GetFork();
    }
    else
    {
        CProfile profile;
        if (!LoadForkProfile(pIndexNew->pOrigin, profile))
        {
            StdTrace("BlockBase", "CommitBlockView::LoadForkProfile %s block failed", pIndexNew->pOrigin->GetBlockHash().ToString().c_str());
            return false;
        }
        if (!dbBlock.AddNewFork(hashFork))
        {
            StdTrace("BlockBase", "CommitBlockView::AddNewFork %s  failed", hashFork.ToString().c_str());
            return false;
        }
        spFork = AddNewFork(profile, pIndexNew);
    }

    vector<pair<uint256, CTxIndex>> vTxNew;
    vector<pair<CAddrTxIndex, CAddrTxInfo>> vAddrTxNew;
    if (!GetTxNewIndex(view, pIndexNew, vTxNew, vAddrTxNew))
    {
        StdTrace("BlockBase", "CommitBlockView: Get tx new index failed");
        return false;
    }

    vector<uint256> vTxDel;
    vector<CAddrTxIndex> vAddrTxDel;
    view.GetTxRemoved(vTxDel, vAddrTxDel, fCfgAddrTxIndex);

    vector<CTxUnspent> vAddNewUnspent;
    vector<CTxUnspent> vRemoveUnspent;
    view.GetUnspentChanges(vAddNewUnspent, vRemoveUnspent);

    if (hashFork == view.GetForkHash())
    {
        spFork->UpgradeToWrite();
    }

    if (!dbBlock.UpdateFork(hashFork, pIndexNew->GetBlockHash(), view.GetForkHash(), vTxNew, vTxDel, vAddrTxNew, vAddrTxDel, vAddNewUnspent, vRemoveUnspent))
    {
        StdTrace("BlockBase", "CommitBlockView::Update fork %s  failed", hashFork.ToString().c_str());
        return false;
    }
    spFork->UpdateLast(pIndexNew);

    if (spFork->GetProfile().nForkType == FORK_TYPE_DEFI)
    {
        vector<CBlockEx> vAdd;
        vector<CBlockEx> vRemove;
        view.GetBlockChanges(vAdd, vRemove);

        if (!AddDeFiRelation(hashFork, spFork, vAdd, vRemove))
        {
            StdLog("CBlockBase", "CommitBlockView: AddDeFiRelation fail, fork: %s", hashFork.ToString().c_str());
            return false;
        }

        if (!UpdateDeFiMintHeight(hashFork, spFork, vAdd, vRemove))
        {
            StdLog("CBlockBase", "CommitBlockView: UpdateDeFiMintHeight fail, fork: %s", hashFork.ToString().c_str());
            return false;
        }
    }

    Log("B", "Update fork %s, last block hash=%s", hashFork.ToString().c_str(),
        pIndexNew->GetBlockHash().ToString().c_str());
    return true;
}

bool CBlockBase::LoadIndex(CBlockOutline& outline)
{
    uint256 hash = outline.GetBlockHash();
    CBlockIndex* pIndexNew = nullptr;

    map<uint256, CBlockIndex*>::iterator mi = mapIndex.find(hash);
    if (mi != mapIndex.end())
    {
        pIndexNew = (*mi).second;
        *pIndexNew = static_cast<CBlockIndex&>(outline);
    }
    else
    {
        pIndexNew = new CBlockIndex(static_cast<CBlockIndex&>(outline));
        if (pIndexNew == nullptr)
        {
            Log("B", "LoadIndex: new CBlockIndex fail");
            return false;
        }
        mi = mapIndex.insert(make_pair(hash, pIndexNew)).first;
    }

    pIndexNew->phashBlock = &((*mi).first);
    pIndexNew->pPrev = nullptr;
    pIndexNew->pOrigin = pIndexNew;

    if (outline.hashPrev != 0)
    {
        pIndexNew->pPrev = GetOrCreateIndex(outline.hashPrev);
        if (pIndexNew->pPrev == nullptr)
        {
            Log("B", "LoadIndex: GetOrCreateIndex prev block index fail");
            return false;
        }
    }

    if (!pIndexNew->IsOrigin())
    {
        pIndexNew->pOrigin = GetOrCreateIndex(outline.hashOrigin);
        if (pIndexNew->pOrigin == nullptr)
        {
            Log("B", "LoadIndex: GetOrCreateIndex origin block index fail");
            return false;
        }
    }

    UpdateBlockHeightIndex(pIndexNew->GetOriginHash(), hash, pIndexNew->nTimeStamp, CDestination(), uint256());
    return true;
}

bool CBlockBase::LoadTx(CTransaction& tx, uint32 nTxFile, uint32 nTxOffset, uint256& hashFork)
{
    tx.SetNull();
    if (!tsBlock.Read(tx, nTxFile, nTxOffset))
    {
        StdTrace("BlockBase", "LoadTx::Read %s block failed", tx.GetHash().ToString().c_str());
        return false;
    }
    CBlockIndex* pIndex = (tx.hashAnchor != 0 ? GetIndex(tx.hashAnchor) : GetOriginIndex(tx.GetHash()));
    if (pIndex == nullptr)
    {
        return false;
    }
    hashFork = pIndex->GetOriginHash();
    return true;
}

bool CBlockBase::FilterTx(const uint256& hashFork, CTxFilter& filter)
{
    CReadLock rlock(rwAccess);

    boost::shared_ptr<CBlockFork> spFork = GetFork(hashFork);
    if (spFork == nullptr)
    {
        StdTrace("BlockBase", "FilterTx::GetFork %s  failed", hashFork.ToString().c_str());
        return false;
    }

    CReadLock rForkLock(spFork->GetRWAccess());

    for (CBlockIndex* pIndex = spFork->GetOrigin(); pIndex != nullptr; pIndex = pIndex->pNext)
    {
        CBlockEx block;
        if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset))
        {
            StdLog("BlockBase", "FilterTx: Block read fail, nFile: %d, nOffset: %d.", pIndex->nFile, pIndex->nOffset);
            return false;
        }
        int nBlockHeight = pIndex->GetBlockHeight();
        if (block.txMint.nAmount > 0 && filter.setDest.count(block.txMint.sendTo))
        {
            if (!filter.FoundTx(hashFork, CAssembledTx(block.txMint, nBlockHeight)))
            {
                StdLog("BlockBase", "FilterTx: FoundTx mint tx fail, txid: %s.", block.txMint.GetHash().GetHex().c_str());
                return false;
            }
        }
        for (int i = 0; i < block.vtx.size(); i++)
        {
            CTransaction& tx = block.vtx[i];
            CTxContxt& ctxt = block.vTxContxt[i];

            if (filter.setDest.count(tx.sendTo) || filter.setDest.count(ctxt.destIn))
            {
                if (!filter.FoundTx(hashFork, CAssembledTx(tx, nBlockHeight, ctxt.destIn, ctxt.GetValueIn())))
                {
                    StdLog("BlockBase", "FilterTx: FoundTx tx fail, txid: %s.", tx.GetHash().GetHex().c_str());
                    return false;
                }
            }
        }
    }
    return true;
}

bool CBlockBase::FilterTx(const uint256& hashFork, int nDepth, CTxFilter& filter)
{
    CReadLock rlock(rwAccess);

    boost::shared_ptr<CBlockFork> spFork = GetFork(hashFork);
    if (spFork == nullptr)
    {
        StdTrace("BlockBase", "FilterTx2::GetFork %s  failed", hashFork.ToString().c_str());
        return false;
    }

    CReadLock rForkLock(spFork->GetRWAccess());

    int nCount = 0;
    for (CBlockIndex* pIndex = spFork->GetLast(); pIndex != nullptr && nCount++ < nDepth; pIndex = pIndex->pPrev)
    {
        CBlockEx block;
        if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset))
        {
            StdLog("BlockBase", "FilterTx2: Block read fail, nFile: %d, nOffset: %d, block: %s.",
                   pIndex->nFile, pIndex->nOffset, pIndex->GetBlockHash().GetHex().c_str());
            return false;
        }
        int nBlockHeight = pIndex->GetBlockHeight();
        if (block.txMint.nAmount > 0 && filter.setDest.count(block.txMint.sendTo))
        {
            if (!filter.FoundTx(hashFork, CAssembledTx(block.txMint, nBlockHeight)))
            {
                StdLog("BlockBase", "FilterTx2: FoundTx mint tx fail, height: %d, txid: %s, block: %s, fork: %s.",
                       nBlockHeight, block.txMint.GetHash().GetHex().c_str(), pIndex->GetBlockHash().GetHex().c_str(), pIndex->GetOriginHash().GetHex().c_str());
                return false;
            }
        }
        for (int i = 0; i < block.vtx.size(); i++)
        {
            CTransaction& tx = block.vtx[i];
            CTxContxt& ctxt = block.vTxContxt[i];

            if (filter.setDest.count(tx.sendTo) || filter.setDest.count(ctxt.destIn))
            {
                if (!filter.FoundTx(hashFork, CAssembledTx(tx, nBlockHeight, ctxt.destIn, ctxt.GetValueIn())))
                {
                    StdLog("BlockBase", "FilterTx2: FoundTx tx fail, height: %d, txid: %s, block: %s, fork: %s.",
                           nBlockHeight, tx.GetHash().GetHex().c_str(), pIndex->GetBlockHash().GetHex().c_str(), pIndex->GetOriginHash().GetHex().c_str());
                    return false;
                }
            }
        }
    }
    return true;
}

bool CBlockBase::ListForkContext(std::vector<CForkContext>& vForkCtxt, map<uint256, CValidForkId>& mapValidForkId)
{
    return dbBlock.ListForkContext(vForkCtxt, mapValidForkId);
}

bool CBlockBase::GetForkBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep)
{
    CReadLock rlock(rwAccess);

    boost::shared_ptr<CBlockFork> spFork = GetFork(hashFork);
    if (spFork == nullptr)
    {
        StdTrace("BlockBase", "GetForkBlockLocator GetFork failed, hashFork: %s", hashFork.ToString().c_str());
        return false;
    }

    CBlockIndex* pIndex = nullptr;
    {
        CReadLock rForkLock(spFork->GetRWAccess());
        pIndex = spFork->GetLast();
        if (pIndex == nullptr)
        {
            StdTrace("BlockBase", "GetForkBlockLocator GetLast failed, hashFork: %s", hashFork.ToString().c_str());
            return false;
        }
    }

    if (hashDepth != 0)
    {
        CBlockIndex* pStartIndex = GetIndex(hashDepth);
        if (pStartIndex != nullptr && pStartIndex->pNext != nullptr)
        {
            pIndex = pStartIndex;
        }
    }

    while (pIndex)
    {
        if (pIndex->GetOriginHash() != hashFork)
        {
            hashDepth = 0;
            break;
        }
        locator.vBlockHash.push_back(pIndex->GetBlockHash());
        if (pIndex->IsOrigin())
        {
            hashDepth = 0;
            break;
        }
        if (locator.vBlockHash.size() >= nIncStep / 2)
        {
            pIndex = pIndex->pPrev;
            if (pIndex == nullptr)
            {
                hashDepth = 0;
            }
            else
            {
                hashDepth = pIndex->GetBlockHash();
            }
            break;
        }
        for (int i = 0; i < nIncStep && !pIndex->IsOrigin(); i++)
        {
            pIndex = pIndex->pPrev;
            if (pIndex == nullptr)
            {
                hashDepth = 0;
                break;
            }
        }
    }

    return true;
}

bool CBlockBase::GetForkBlockInv(const uint256& hashFork, const CBlockLocator& locator, vector<uint256>& vBlockHash, size_t nMaxCount)
{
    CReadLock rlock(rwAccess);

    boost::shared_ptr<CBlockFork> spFork = GetFork(hashFork);
    if (spFork == nullptr)
    {
        StdTrace("BlockBase", "GetForkBlockInv::GetFork %s failed", hashFork.ToString().c_str());
        return false;
    }

    CReadLock rForkLock(spFork->GetRWAccess());
    CBlockIndex* pIndexLast = spFork->GetLast();
    CBlockIndex* pIndex = nullptr;
    for (const uint256& hash : locator.vBlockHash)
    {
        pIndex = GetIndex(hash);
        if (pIndex != nullptr && (pIndex == pIndexLast || pIndex->pNext != nullptr))
        {
            if (pIndex->GetOriginHash() != hashFork)
            {
                StdTrace("BlockBase", "GetForkBlockInv GetOriginHash error, fork: %s", hashFork.ToString().c_str());
                return false;
            }
            break;
        }
        pIndex = nullptr;
    }

    if (pIndex != nullptr)
    {
        pIndex = pIndex->pNext;
        while (pIndex != nullptr && vBlockHash.size() < nMaxCount)
        {
            vBlockHash.push_back(pIndex->GetBlockHash());
            pIndex = pIndex->pNext;
        }
    }
    return true;
}

bool CBlockBase::CheckConsistency(int nCheckLevel, int nCheckDepth)
{
    boost::timer::cpu_timer t_lock;
    t_lock.start();

    CReadLock rlock(rwAccess);

    Log("B", "Getting lock duration ===> %s.", t_lock.format().c_str());

    boost::timer::cpu_timer t_check;
    t_check.start();

    Log("B", "Check consistency with parameters check-level:%d and check-depth:%d.", nCheckLevel, nCheckDepth);

    int nLevel = nCheckLevel;
    if (nCheckLevel < 0)
    {
        nLevel = 0;
    }
    if (nCheckLevel > 3)
    {
        nLevel = 3;
    }
    int32 nDepth = nCheckDepth;

    Log("B", "Consistency checking level is %d", nLevel);

    vector<pair<uint256, uint256>> vFork;
    if (!dbBlock.ListFork(vFork))
    {
        Error("B", "List fork failed.");
        return false;
    }

    for (const auto& fork : vFork)
    {
        boost::timer::cpu_timer t_fork;
        t_fork.start();

        //checking of level 0: fork/block

        //check field refblock of table fork must be in rows in table block
        CBlockIndex* pBlockRefIndex = GetIndex(fork.second);
        if (!pBlockRefIndex)
        {
            Error("B", "Get referenced block index failed.");
            return false;
        }

        boost::shared_ptr<CBlockFork> spFork = GetFork(fork.first);
        if (nullptr == spFork)
        {
            Error("B", "Get fork failed.");
            return false;
        }
        CBlockIndex* pLastBlock = spFork->GetLast();
        if (nullptr == pLastBlock)
        {
            Error("B", "Get last block index of current fork failed.");
            return false;
        }

        bool fIsMainFork = spFork->GetOrigin()->IsPrimary();

        if (0 == nDepth || pLastBlock->nHeight < nDepth)
        {
            nDepth = pLastBlock->nHeight;
            Log("B", "Consistency checking depth is {%d} for fork:{%s}", nDepth, fork.first.ToString().c_str());
        }

        CBlockIndex* pIndex = pLastBlock;
        map<CDestination, int64> mapNextBlockDelegate;

        map<CTxOutPoint, CTxUnspent> mapUnspentUTXO;
        vector<CTxOutPoint> vSpentUTXO;

        while (pIndex && pLastBlock->nHeight - pIndex->nHeight < nDepth)
        {
            //be able to read from block files
            CBlockEx block;
            if (!tsBlock.ReadDirect(block, pIndex->nFile, pIndex->nOffset))
            {
                Error("B", "Retrieve block from file directly failed.");
                return false;
            }

            //consistent between database and block file
            if (!(pIndex->GetBlockHash() == block.GetHash()
                  && pIndex->pPrev->GetBlockHash() == block.hashPrev
                  && pIndex->nVersion == block.nVersion
                  && pIndex->nType == block.nType
                  && pIndex->nTimeStamp == block.nTimeStamp
                  && ((pIndex->nMintType == 0) ? block.IsVacant()
                                               : (!block.IsVacant() && pIndex->txidMint == block.txMint.GetHash() && pIndex->nMintType == block.txMint.nType))))
            {
                Error("B", "Block info are not consistent in db and file.");
                return false;
            }

            //checking of level 1: transaction
            if (nLevel >= 1 && !pIndex->IsVacant())
            {
                auto lmdChkTx = [&](const uint256& txid, const CTxIndex& pTxIndex) -> bool
                {
                    CTransaction tx;
                    if (!tsBlock.ReadDirect(tx, pTxIndex.nFile, pTxIndex.nOffset))
                    {
                        Error("B", "Retrieve tx from file directly failed.");
                        return false;
                    }

                    //consistent between database and block file
                    if (txid != tx.GetHash() || pTxIndex.nBlockHeight != pIndex->nHeight)
                    {
                        return false;
                    }

                    return true;
                };

                CTxIndex pTxIdx;
                uint256 fk;
                if (!dbBlock.RetrieveTxIndex(pIndex->txidMint, pTxIdx, fk))
                {
                    Error("B", "Retrieve mint tx index from db failed.");
                    return false;
                }
                if (!lmdChkTx(pIndex->txidMint, pTxIdx))
                {
                    Error("B", "Mint tx info are not consistent in db and file.");
                    return false;
                }

                for (auto const& tx : block.vtx)
                {
                    pTxIdx.SetNull();
                    const uint256& txid = tx.GetHash();
                    if (!dbBlock.RetrieveTxIndex(txid, pTxIdx, fk))
                    {
                        Error("B", "Retrieve token tx index from db failed.");
                        return false;
                    }
                    if (!lmdChkTx(txid, pTxIdx))
                    {
                        Error("B", "Token tx info are not consistent in db and file.");
                        return false;
                    }
                }
            }

            //checking of level 2: delegate/enroll
            if (nLevel >= 2 && fIsMainFork)
            {
                static bool fIsLastBlock = true;

                if (fIsLastBlock)
                {
                    if (!dbBlock.RetrieveDelegate(block.GetHash(), mapNextBlockDelegate))
                    {
                        Error("B", "Retrieve the latest delegate record from db failed.");
                        return false;
                    }
                    fIsLastBlock = false;
                }
                else
                { //compare delegate in this iteration with the previous one
                    map<CDestination, int64> mapPrevBlockDelegate;
                    if (!dbBlock.RetrieveDelegate(block.GetHash(), mapPrevBlockDelegate))
                    {
                        Error("B", "Retrieve the following previous delegate record from db failed.");
                        return false;
                    }
                    if (mapNextBlockDelegate != mapPrevBlockDelegate)
                    {
                        Error("B", "Delegate records followed one by one do not match.");
                        return false;
                    }
                    mapNextBlockDelegate = mapPrevBlockDelegate;
                }

                if (block.txMint.nType == CTransaction::TX_STAKE)
                {
                    mapNextBlockDelegate[block.txMint.sendTo] -= block.txMint.nAmount;
                }

                map<pair<uint256, CDestination>, tuple<uint256, uint32, uint32>> mapEnrollRanged;
                for (int i = 0; i < block.vtx.size(); i++)
                {
                    const CTransaction& tx = block.vtx[i];
                    {
                        CTemplateId tid;
                        if (tx.sendTo.GetTemplateId(tid) && tid.GetType() == TEMPLATE_DELEGATE)
                        {
                            mapNextBlockDelegate[tx.sendTo] -= tx.nAmount;
                        }
                    }

                    const CTxContxt& txContxt = block.vTxContxt[i];
                    {
                        CTemplateId tid;
                        if (txContxt.destIn.GetTemplateId(tid) && tid.GetType() == TEMPLATE_DELEGATE)
                        {
                            mapNextBlockDelegate[txContxt.destIn] += tx.nAmount + tx.nTxFee;
                        }
                    }

                    if (tx.nType == CTransaction::TX_CERT)
                    {
                        const uint256& anchor = tx.hashAnchor;
                        const CDestination& dest = tx.sendTo;
                        const uint256& blk = block.GetHash();
                        CTxIndex txIdx;
                        uint256 fk;
                        if (!dbBlock.RetrieveTxIndex(tx.GetHash(), txIdx, fk))
                        {
                            Error("B", "Retrieve enroll tx index from table transaction failed.");
                            return false;
                        }
                        const uint32& nFile = txIdx.nFile;
                        const uint32& nOffset = txIdx.nOffset;
                        mapEnrollRanged[make_pair(anchor, dest)] = make_tuple(blk, nFile, nOffset);
                    }
                }

                vector<CDestination> vDestNull;
                for (const auto& delegate : mapNextBlockDelegate)
                {
                    if (delegate.second < 0)
                    {
                        Error("B", "Amount on delegate template address must not be less than zero.");
                        return false;
                    }
                    if (delegate.second == 0)
                    {
                        vDestNull.push_back(delegate.first);
                    }
                }
                for (const auto& dest : vDestNull)
                {
                    mapNextBlockDelegate.erase(dest);
                }

                //compare enroll ranged in argument of nDepth with table enroll
                vector<uint256> vBlockRange;
                vBlockRange.push_back(block.GetHash());
                map<CDestination, CDiskPos> mapRes;
                if (!dbBlock.RetrieveEnroll(GetIndex(block.hashPrev)->GetBlockHeight(), vBlockRange, mapRes))
                {
                    Error("B", "Retrieve enroll tx records from table enroll failed.");
                    return false;
                }
                map<CDestination, CDiskPos> mapResComp;
                for (const auto& enroll : mapEnrollRanged)
                {
                    const CDestination& dest = enroll.first.second;
                    const tuple<uint256, uint32, uint32>& pos = enroll.second;
                    const uint32& file = get<1>(pos);
                    const uint32& offset = get<2>(pos);
                    mapResComp.insert(make_pair(dest, CDiskPos(file, offset)));
                }
                if (mapRes != mapResComp)
                {
                    Error("B", "Enroll transactions in tables enroll and transaction do not match.");
                    return false;
                }
            }

            //checking of level 3: unspent
            if (nLevel >= 3)
            {
                CTransaction txMint;
                if (!RetrieveTx(block.txMint.GetHash(), txMint))
                {
                    return false;
                }
                mapUnspentUTXO.insert(make_pair(CTxOutPoint(block.txMint.GetHash(), 0), CTxUnspent(CTxOutPoint(block.txMint.GetHash(), 0), CTxOut(txMint.sendTo, txMint.nAmount, txMint.nTimeStamp, txMint.nLockUntil))));

                for (int i = 0; i < block.vtx.size(); ++i)
                {
                    const CTransaction& tx = block.vtx[i];
                    const CTxContxt& txCtxt = block.vTxContxt[i];
                    int64 nChange = txCtxt.GetValueIn() - tx.nAmount - tx.nTxFee;
                    mapUnspentUTXO.insert(make_pair(
                        CTxOutPoint(tx.GetHash(), 0),
                        CTxUnspent(CTxOutPoint(tx.GetHash(), 0), CTxOut(tx.sendTo, tx.nAmount, tx.nTimeStamp, tx.nLockUntil))));
                    if (nChange > 0)
                    {
                        Log("B", "Tx(%s) with a change(%s) on height(%d): to prepare to check.", tx.GetHash().ToString().c_str(), to_string(nChange).c_str(), pIndex->nHeight);
                        if (!CheckInputSingleAddressForTxWithChange(tx.GetHash()))
                        {
                            Error("B", "Tx(%s) with a change(%s) on height(%d): input must be a single address.", tx.GetHash().ToString().c_str(), to_string(nChange).c_str(), pIndex->nHeight);
                            return false;
                        }
                        else
                        {
                            mapUnspentUTXO.insert(make_pair(
                                CTxOutPoint(tx.GetHash(), 1),
                                CTxUnspent(CTxOutPoint(tx.GetHash(), 1), CTxOut(txCtxt.destIn, nChange, tx.nTimeStamp, tx.nLockUntil))));
                        }
                    }
                    for (const auto& txin : tx.vInput)
                    {
                        vSpentUTXO.push_back(txin.prevout);
                    }
                }

                vector<CTxOutPoint> vRemovedUTXO;
                for (const auto& spent : vSpentUTXO)
                {
                    if (mapUnspentUTXO.find(spent) != mapUnspentUTXO.end())
                    {
                        mapUnspentUTXO.erase(spent);
                        vRemovedUTXO.push_back(spent);
                    }
                }

                for (const auto& txDel : vRemovedUTXO)
                {
                    const auto& pos = find(vSpentUTXO.begin(), vSpentUTXO.end(), txDel);
                    vSpentUTXO.erase(pos);
                }
            }

            pIndex = pIndex->pPrev;
        }
        Log("B", "Checking duration before comparing unspent ===> %s", t_fork.format().c_str());
        if (nLevel >= 3)
        {
            //compare unspent with transaction
            CForkUnspentCheckWalker walker(mapUnspentUTXO);
            if (!dbBlock.WalkThroughUnspent(fork.first, walker))
            {
                Error("B", "{%d} ranged unspent records failed to walk through.", mapUnspentUTXO.size());
                return false;
            }

            if (walker.nMatch != mapUnspentUTXO.size())
            {
                Error("B", "{%d} ranged unspent records do not match with full collection of unspent.", mapUnspentUTXO.size());
                return false;
            }
        }

        Log("B", "Checking duration of fork{%s} ===> %s", fork.first.ToString().c_str(), t_fork.format().c_str());
    }

    Log("B", "Checking duration ===> %s", t_check.format().c_str());

    Log("B", "Data consistency verified.");

    return true;
}

bool CBlockBase::CheckInputSingleAddressForTxWithChange(const uint256& txid)
{
    CTransaction tx;
    if (!RetrieveTx(txid, tx))
    {
        Error("B", "[CBlockBase::CheckInputSingleAddressForTxWithChange](%s): Failed to call to RetrieveTx.", txid.ToString().c_str());
        return false;
    }

    //get all inputs whose index is 0 if any
    vector<CDestination> vDestNoChange;
    vector<uint256> vTxExistChange;
    for (const auto& i : tx.vInput)
    {
        if (i.prevout.n == 0)
        {
            CTransaction prevTx;
            if (!RetrieveTx(i.prevout.hash, prevTx))
            {
                Error("B", "[CBlockBase::CheckInputSingleAddressForTxWithChange](%s): Failed to call to RetrieveTxIndex.", txid.ToString().c_str());
                return false;
            }
            vDestNoChange.push_back(prevTx.sendTo);
        }
        else
        {
            vTxExistChange.push_back(i.prevout.hash);
        }
    }

    sort(vDestNoChange.begin(), vDestNoChange.end());
    auto end_iter = unique(vDestNoChange.begin(), vDestNoChange.end());
    vDestNoChange.erase(end_iter, vDestNoChange.end());

    //if destinations are not equal, return false
    if (vDestNoChange.size() > 1)
    {
        Error("B", "[CBlockBase::CheckInputSingleAddressForTxWithChange](%s): {vDestNoChange.size() > 1}.", txid.ToString().c_str());
        return false;
    }

    //if destination from input is not equal to output, return false
    if (vDestNoChange.size() == 1)
    {
        CDestination dest = *(vDestNoChange.begin());
        if (dest != tx.sendTo)
        {
            Error("B", "[CBlockBase::CheckInputSingleAddressForTxWithChange](%s): {dest != txIdx.sendTo}.", txid.ToString().c_str());
            return false;
        }
    }

    //if exist output index is 1, recur the process
    vector<bool> vRes;
    for (const auto& i : vTxExistChange)
    {
        vRes.push_back(CheckInputSingleAddressForTxWithChange(i));
    }

    if (!vRes.empty())
    {
        int nFalse = count(vRes.begin(), vRes.end(), false);
        if (nFalse <= 0)
        {
            return true;
        }
        else
        {
            Error("B", "Tx(%s) one or more preout validate failed.", txid.ToString().c_str());
            return false;
        }
        //        return count(vRes.begin(), vRes.end(), false) <= 0;
    }
    else
    {
        return true;
    }
}

bool CBlockBase::ListForkUnspent(const uint256& hashFork, const CDestination& dest, uint32 nMax, std::vector<CTxUnspent>& vUnspent)
{
    vUnspent.clear();
    CListUnspentWalker walker(hashFork, dest, nMax);
    dbBlock.WalkThroughUnspent(hashFork, walker);
    vUnspent = walker.vUnspent;
    return true;
}

bool CBlockBase::ListForkUnspentBatch(const uint256& hashFork, uint32 nMax, std::map<CDestination, std::vector<CTxUnspent>>& mapUnspent)
{
    CListUnspentBatchWalker walker(hashFork, mapUnspent, nMax);
    dbBlock.WalkThroughUnspent(hashFork, walker);
    return true;
}

bool CBlockBase::RetrieveAddressUnspent(const uint256& hashFork, const CDestination& dest, map<CTxOutPoint, CUnspentOut>& mapUnspent, uint256& hashLastBlockOut)
{
    return dbBlock.RetrieveAddressUnspent(hashFork, dest, mapUnspent, hashLastBlockOut);
}

int64 CBlockBase::RetrieveAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, vector<CTxInfo>& vTx)
{
    map<CAddrTxIndex, CAddrTxInfo> mapAddrTxIndex;
    int64 nGetEndPos = dbBlock.RetrieveAddressTxList(hashFork, dest, nPrevHeight, nPrevTxSeq, nOffset, nCount, mapAddrTxIndex);
    if (nGetEndPos < 0)
    {
        return nGetEndPos;
    }
    for (const auto& vd : mapAddrTxIndex)
    {
        if (vd.second.nDirection == CAddrTxInfo::TXI_DIRECTION_TO)
        {
            vTx.push_back(CTxInfo(vd.first.txid, hashFork, vd.second.nTxType, vd.second.nTimeStamp,
                                  vd.second.nLockUntil, vd.first.GetHeight(), vd.first.GetSeq(), vd.second.destPeer, vd.first.dest,
                                  vd.second.nAmount, vd.second.nTxFee, 0));
        }
        else
        {
            vTx.push_back(CTxInfo(vd.first.txid, hashFork, vd.second.nTxType, vd.second.nTimeStamp,
                                  vd.second.nLockUntil, vd.first.GetHeight(), vd.first.GetSeq(), vd.first.dest, vd.second.destPeer,
                                  vd.second.nAmount, vd.second.nTxFee, 0));
        }
    }
    return nGetEndPos;
}

bool CBlockBase::ListForkAllAddressAmount(const uint256& hashFork, CBlockView& view, std::map<CDestination, int64>& mapAddressAmount)
{
    std::vector<CTxUnspent> vAddNew;
    std::vector<CTxOutPoint> vRemove;
    view.GetUnspentChanges(vAddNew, vRemove);

    CListAddressUnspentWalker walker(vRemove);
    if (!dbBlock.WalkThroughUnspent(hashFork, walker))
    {
        return false;
    }
    for (const CTxUnspent& unspent : vAddNew)
    {
        walker.mapAddressAmount[unspent.output.destTo] += unspent.output.nAmount;
    }
    mapAddressAmount = walker.mapAddressAmount;
    return true;
}

bool CBlockBase::AddDeFiRelation(const uint256& hashFork, boost::shared_ptr<CBlockFork> spFork, const vector<CBlockEx>& vAdd, const vector<CBlockEx>& vRemove)
{
    vector<pair<CDestination, CAddrInfo>> vNewAddress;
    vector<CDestination> vRemoveAddress;

    for (const CBlockEx& block : vRemove)
    {
        for (int i = block.vtx.size() - 1; i >= 0; --i)
        {
            const CTransaction& tx = block.vtx[i];
            if (tx.IsDeFiRelation())
            {
                vRemoveAddress.push_back(tx.sendTo);
            }
        }
    }

    for (const CBlockEx& block : boost::adaptors::reverse(vAdd))
    {
        for (std::size_t i = 0; i < block.vtx.size(); i++)
        {
            const CTransaction& tx = block.vtx[i];
            const CTxContxt& txContxt = block.vTxContxt[i];
            if (tx.IsDeFiRelation() && tx.sendTo != txContxt.destIn)
            {
                vNewAddress.push_back(make_pair(tx.sendTo, CAddrInfo(CDestination(), txContxt.destIn)));
            }
        }
    }

    if (!dbBlock.UpdateAddressInfo(hashFork, vNewAddress, vRemoveAddress))
    {
        StdWarn("CBlockBase", "AddDeFiRelation: UpdateAddressInfo fail, fork: %s", hashFork.ToString().c_str());
        return false;
    }

    // update CBlockFork::relation
    auto& relation = spFork->GetRelation();
    for (auto& addr : vRemoveAddress)
    {
        relation.RemoveRelation(addr);
        StdDebug("CBlockBase", "Remove relation from memory, key: %s", addr.GetPubKey().ToString().c_str());
    }

    for (auto& addr : vNewAddress)
    {
        if (!relation.Insert(addr.first, addr.second.destParent, addr.second.destParent))
        {
            // reconstruct memory
            StdError("CBlockBase", "AddDeFiRelation memory is not equal DB");
            InitDeFiRelation(spFork);
            break;
        }
        StdDebug("CBlockBase", "Add relation in memory, key: %s", addr.first.GetPubKey().ToString().c_str());
    }

    return true;
}

bool CBlockBase::GetDeFiRelation(const uint256& hashFork, const CDestination& destIn, CAddrInfo& addrInfo)
{
    CAddrInfo addressInfo;
    return dbBlock.GetAddressInfo(hashFork, destIn, addrInfo);
}

bool CBlockBase::InitDeFiRelation(const uint256& hashFork)
{
    boost::shared_ptr<CBlockFork> spFork;
    {
        CReadLock rlock(rwAccess);

        spFork = GetFork(hashFork);
        if (!spFork)
        {
            return false;
        }
    }

    if (spFork->GetProfile().nForkType != FORK_TYPE_DEFI)
    {
        return true;
    }

    spFork->WriteLock();
    bool ret = InitDeFiRelation(spFork);
    spFork->WriteUnlock();

    return ret;
}

bool CBlockBase::InitDeFiRelation(boost::shared_ptr<CBlockFork> spFork)
{
    auto& relation = spFork->GetRelation();
    relation.Clear();

    CListAddressWalker walker;
    if (!dbBlock.WalkThroughAddress(spFork->GetOrigin()->GetBlockHash(), walker))
    {
        StdError("CBlockBase", "InitDeFiRelation: WalkThroughAddress fail, fork: %s", spFork->GetOrigin()->GetBlockHash().ToString().c_str());
        return false;
    }

    for (auto& r : walker.mapAddress)
    {
        if (!relation.Insert(r.first, r.second.destParent, r.second.destParent))
        {
            StdError("CBlockBase", "InitDeFiRelation: Insert relation fail, fork: %s, addr: %s, parent: %s",
                     spFork->GetOrigin()->GetBlockHash().ToString().c_str(), r.first.ToString().c_str(), r.second.destParent.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockBase::CheckAddDeFiRelation(const uint256& hashFork, const CDestination& dest, const CDestination& parent)
{
    boost::shared_ptr<CBlockFork> spFork;
    {
        CReadLock rlock(rwAccess);
        spFork = GetFork(hashFork);
        if (!spFork)
        {
            return false;
        }
    }

    if (spFork->GetProfile().nForkType != FORK_TYPE_DEFI)
    {
        return false;
    }

    auto& relation = spFork->GetRelation();

    CDestination root;
    return relation.CheckInsert(dest, parent, root);
}

bool CBlockBase::UpdateDeFiMintHeight(const uint256& hashFork, boost::shared_ptr<CBlockFork> spFork, const vector<CBlockEx>& vAdd, const vector<CBlockEx>& vRemove)
{
    const CProfile& profile = spFork->GetProfile();
    int nMintHeight = profile.defi.nMintHeight;
    {
        bool fUpdate = false;
        for (const CBlockEx& block : vRemove)
        {
            for (int i = block.vtx.size() - 1; i >= 0; --i)
            {
                const CTransaction& tx = block.vtx[i];
                if (tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT)
                {
                    nMintHeight = -1;
                    fUpdate = true;
                    break;
                }
            }
            if (fUpdate)
            {
                break;
            }
        }
    }

    {
        bool fUpdate = false;
        for (const CBlockEx& block : boost::adaptors::reverse(vAdd))
        {
            for (std::size_t i = 0; i < block.vtx.size(); i++)
            {
                const CTransaction& tx = block.vtx[i];
                if (tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT)
                {
                    CIDataStream is(tx.vchData);
                    is >> nMintHeight;
                    fUpdate = true;
                    break;
                }
            }
            if (fUpdate)
            {
                break;
            }
        }
    }

    if (nMintHeight != profile.defi.nMintHeight)
    {
        CProfile newProfile = profile;
        newProfile.defi.nMintHeight = nMintHeight;

        // update fork context
        CForkContext ctxt;
        RetrieveForkContext(hashFork, ctxt);
        ctxt.vchDeFi.clear();
        newProfile.defi.Save(ctxt.vchDeFi);
        if (!dbBlock.AddNewForkContext(ctxt))
        {
            StdError("sht", "add new error!");
            return false;
        }
        spFork->SetProfile(newProfile);
    }

    return true;
}

bool CBlockBase::GetVotes(const uint256& hashGenesis, const CDestination& destDelegate, int64& nVotes)
{
    CBlockIndex* pForkLastIndex = nullptr;
    if (!RetrieveFork(hashGenesis, &pForkLastIndex))
    {
        return false;
    }
    std::map<CDestination, int64> mapVote;
    if (!dbBlock.RetrieveDelegate(pForkLastIndex->GetBlockHash(), mapVote))
    {
        return false;
    }
    nVotes = mapVote[destDelegate];
    return true;
}

bool CBlockBase::GetDelegatePaymentList(const uint256& block_hash, std::multimap<int64, CDestination>& mapVotes)
{
    std::map<CDestination, int64> mapVote;
    if (!dbBlock.RetrieveDelegate(block_hash, mapVote))
    {
        return false;
    }
    for (const auto& d : mapVote)
    {
        mapVotes.insert(std::make_pair(d.second, d.first));
    }
    std::size_t nGetVotesCount = mapVotes.size();
    std::multimap<int64, CDestination>::iterator it = mapVotes.begin();
    while (it != mapVotes.end() && nGetVotesCount > MAX_DELEGATE_THRESH)
    {
        mapVotes.erase(it++);
        --nGetVotesCount;
    }
    return true;
}

bool CBlockBase::GetDelegateList(const uint256& hashGenesis, uint32 nCount, std::multimap<int64, std::pair<CDestination, std::string>>& mapVotes)
{
    CBlockIndex* pForkLastIndex = nullptr;
    if (!RetrieveFork(hashGenesis, &pForkLastIndex))
    {
        return false;
    }
    CDelegateContext ctxtDelegate;
    if (!dbBlock.RetrieveDelegateContext(pForkLastIndex->GetBlockHash(), ctxtDelegate))
    {
        return false;
    }
    for (const auto& d : ctxtDelegate.mapVote)
    {
        std::string strName;
        auto mt = ctxtDelegate.mapDelegateName.find(d.first);
        if (mt != ctxtDelegate.mapDelegateName.end())
        {
            strName = mt->second;
        }
        mapVotes.insert(std::make_pair(d.second, std::make_pair(d.first, strName)));
    }
    if (nCount > 0)
    {
        while (mapVotes.size() > nCount)
        {
            mapVotes.erase(mapVotes.begin());
        }
    }
    return true;
}

bool CBlockBase::VerifyRepeatBlock(const uint256& hashFork, uint32 height, const CDestination& destMint, uint16 nBlockType,
                                   uint32 nBlockTimeStamp, uint32 nRefBlockTimeStamp, uint32 nExtendedBlockSpacing)
{
    CWriteLock wlock(rwAccess);

    map<uint256, CForkHeightIndex>::iterator it = mapForkHeightIndex.find(hashFork);
    if (it != mapForkHeightIndex.end())
    {
        map<uint256, CBlockHeightIndex>* pBlockMint = it->second.GetBlockMintList(height);
        if (pBlockMint != nullptr)
        {
            for (auto& mt : *pBlockMint)
            {
                if (mt.second.destMint.IsNull())
                {
                    CBlockIndex* pBlockIndex = GetIndex(mt.first);
                    if (pBlockIndex)
                    {
                        if (pBlockIndex->IsVacant())
                        {
                            CBlock block;
                            if (Retrieve(pBlockIndex, block) && !block.txMint.sendTo.IsNull())
                            {
                                mt.second.destMint = block.txMint.sendTo;
                            }
                        }
                        else
                        {
                            CTransaction tx;
                            if (RetrieveTx(pBlockIndex->txidMint, tx))
                            {
                                mt.second.destMint = tx.sendTo;
                            }
                        }
                    }
                }
                if (mt.second.destMint == destMint)
                {
                    if (nBlockType == CBlock::BLOCK_SUBSIDIARY || nBlockType == CBlock::BLOCK_EXTENDED)
                    {
                        if ((nBlockTimeStamp - nRefBlockTimeStamp) / nExtendedBlockSpacing
                            == (mt.second.nTimeStamp - nRefBlockTimeStamp) / nExtendedBlockSpacing)
                        {
                            StdTrace("CBlockBase", "VerifyRepeatBlock: subsidiary or extended repeat block, block time: %d, cache block time: %d, ref block time: %d, destMint: %s",
                                     nBlockTimeStamp, mt.second.nTimeStamp, mt.second.nTimeStamp, CAddress(destMint).ToString().c_str());
                            return false;
                        }
                    }
                    else
                    {
                        StdTrace("CBlockBase", "VerifyRepeatBlock: repeat block: %s, destMint: %s", mt.first.GetHex().c_str(), CAddress(destMint).ToString().c_str());
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool CBlockBase::GetBlockDelegateVote(const uint256& hashBlock, map<CDestination, int64>& mapVote)
{
    return dbBlock.RetrieveDelegate(hashBlock, mapVote);
}

bool CBlockBase::GetDelegateEnrollTx(int height, const vector<uint256>& vBlockRange, map<CDestination, CDiskPos>& mapEnrollTxPos)
{
    return dbBlock.RetrieveEnroll(height, vBlockRange, mapEnrollTxPos);
}

bool CBlockBase::GetBlockDelegatedEnrollTx(const uint256& hashBlock, map<int, set<CDestination>>& mapEnrollDest)
{
    map<int, map<CDestination, CDiskPos>> mapEnrollTxPos;
    if (!dbBlock.RetrieveEnroll(hashBlock, mapEnrollTxPos))
    {
        return false;
    }
    for (const auto& d : mapEnrollTxPos)
    {
        set<CDestination>& destEnroll = mapEnrollDest[d.first];
        for (const auto& m : d.second)
        {
            destEnroll.insert(m.first);
        }
    }
    return true;
}

bool CBlockBase::VerifyRefBlock(const uint256& hashGenesis, const uint256& hashRefBlock)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pIndexGenesisLast = nullptr;
    boost::shared_ptr<CBlockFork> spForkGenesis;
    spForkGenesis = GetFork(hashGenesis);
    if (spForkGenesis == nullptr)
    {
        return false;
    }
    pIndexGenesisLast = spForkGenesis->GetLast();
    if (pIndexGenesisLast == nullptr)
    {
        return false;
    }
    return IsValidBlock(pIndexGenesisLast, hashRefBlock);
}

CBlockIndex* CBlockBase::GetForkValidLast(const uint256& hashGenesis, const uint256& hashFork /*, int nRefVacantHeight*/)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pIndexGenesisLast = nullptr;
    boost::shared_ptr<CBlockFork> spForkGenesis;
    spForkGenesis = GetFork(hashGenesis);
    if (spForkGenesis == nullptr)
    {
        return nullptr;
    }
    pIndexGenesisLast = spForkGenesis->GetLast();
    if (pIndexGenesisLast == nullptr)
    {
        return nullptr;
    }

    CBlockIndex* pForkLast = nullptr;
    boost::shared_ptr<CBlockFork> spFork;
    spFork = GetFork(hashFork);
    if (spFork == nullptr)
    {
        return nullptr;
    }
    pForkLast = spFork->GetLast();
    if (pForkLast == nullptr || pForkLast->IsOrigin() /*|| pForkLast->GetBlockHeight() <= nRefVacantHeight*/)
    {
        return nullptr;
    }

    set<uint256> setInvalidHash;
    CBlockIndex* pIndex = pForkLast;
    while (pIndex && !pIndex->IsOrigin())
    {
        /*if (pIndex->GetBlockHeight() <= nRefVacantHeight)
        {
            break;
        }*/
        if (VerifyValidBlock(pIndexGenesisLast, pIndex))
        {
            break;
        }
        setInvalidHash.insert(pIndex->GetBlockHash());
        pIndex = pIndex->pPrev;
    }
    if (pIndex == nullptr)
    {
        pIndex = GetIndex(hashFork);
    }
    if (pIndex == pForkLast)
    {
        return nullptr;
    }
    CBlockIndex* pIndexValidLast = GetLongChainLastBlock(hashFork, pIndex->GetBlockHeight(), pIndexGenesisLast, setInvalidHash);
    if (pIndexValidLast == nullptr)
    {
        return pIndex;
    }
    return pIndexValidLast;
}

bool CBlockBase::VerifySameChain(const uint256& hashPrevBlock, const uint256& hashAfterBlock)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pPrevIndex = GetIndex(hashPrevBlock);
    if (pPrevIndex == nullptr)
    {
        return false;
    }
    CBlockIndex* pAfterIndex = GetIndex(hashAfterBlock);
    if (pAfterIndex == nullptr)
    {
        return false;
    }
    while (pAfterIndex->GetBlockHeight() >= pPrevIndex->GetBlockHeight())
    {
        if (pAfterIndex == pPrevIndex)
        {
            return true;
        }
        pAfterIndex = pAfterIndex->pPrev;
    }
    return false;
}

bool CBlockBase::GetLastRefBlockHash(const uint256& hashFork, const uint256& hashBlock, uint256& hashRefBlock, bool& fOrigin)
{
    hashRefBlock = 0;
    fOrigin = false;
    CBlockIndex* pIndexUpdateRef = nullptr;

    {
        CReadLock rlock(rwAccess);

        auto it = mapForkHeightIndex.find(hashFork);
        if (it == mapForkHeightIndex.end())
        {
            return false;
        }
        std::map<uint256, CBlockHeightIndex>* pHeightIndex = it->second.GetBlockMintList(CBlock::GetBlockHeightByHash(hashBlock));
        if (pHeightIndex)
        {
            auto mt = pHeightIndex->find(hashBlock);
            if (mt != pHeightIndex->end() && mt->second.hashRefBlock != 0)
            {
                hashRefBlock = mt->second.hashRefBlock;
                return true;
            }
        }

        CBlockIndex* pIndex = GetIndex(hashBlock);
        while (pIndex)
        {
            if (pIndex->IsOrigin())
            {
                fOrigin = true;
                return true;
            }
            CBlockEx block;
            if (!Retrieve(pIndex, block))
            {
                return false;
            }
            if (!block.vchProof.empty())
            {
                CProofOfPiggyback proof;
                if (proof.Load(block.vchProof) && proof.hashRefBlock != 0)
                {
                    hashRefBlock = proof.hashRefBlock;
                    pIndexUpdateRef = pIndex;
                    break;
                }
            }
            pIndex = pIndex->pPrev;
        }
    }

    if (pIndexUpdateRef)
    {
        CWriteLock wlock(rwAccess);
        UpdateBlockRef(pIndexUpdateRef->GetOriginHash(), pIndexUpdateRef->GetBlockHash(), hashRefBlock);
        return true;
    }
    return false;
}

bool CBlockBase::GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime)
{
    CReadLock rlock(rwAccess);

    CBlockIndex* pIndex = GetIndex(hashLastBlock);
    if (pIndex == nullptr || !pIndex->IsPrimary())
    {
        return false;
    }
    while (pIndex && pIndex->GetBlockHeight() >= nHeight)
    {
        if (pIndex->GetBlockHeight() == nHeight)
        {
            hashBlock = pIndex->GetBlockHash();
            nTime = pIndex->GetBlockTime();
            return true;
        }
        pIndex = pIndex->pPrev;
    }
    return false;
}

bool CBlockBase::VerifyPrimaryHeightRefBlockTime(const int nHeight, const int64 nTime)
{
    CReadLock rlock(rwAccess);

    const std::map<uint256, CBlockHeightIndex>* pMapHeight = mapForkHeightIndex[hashGenesisBlock].GetBlockMintList(nHeight);
    if (pMapHeight == nullptr)
    {
        return false;
    }
    for (const auto& vd : *pMapHeight)
    {
        if (vd.second.nTimeStamp != nTime)
        {
            return false;
        }
    }
    return true;
}

bool CBlockBase::RetrieveInvestContext(const uint256& hashBlock, CInvestContext& ctxtInvest)
{
    return dbBlock.RetrieveInvestContext(hashBlock, ctxtInvest);
}

bool CBlockBase::RetrieveActivateContext(const uint256& hashBlock, CActivateContext& ctxtActivate)
{
    return dbBlock.RetrieveActivateContext(hashBlock, ctxtActivate);
}

CBlockIndex* CBlockBase::GetIndex(const uint256& hash) const
{
    map<uint256, CBlockIndex*>::const_iterator mi = mapIndex.find(hash);
    return (mi != mapIndex.end() ? (*mi).second : nullptr);
}

CBlockIndex* CBlockBase::GetOrCreateIndex(const uint256& hash)
{
    map<uint256, CBlockIndex*>::const_iterator mi = mapIndex.find(hash);
    if (mi == mapIndex.end())
    {
        CBlockIndex* pIndexNew = new CBlockIndex();
        mi = mapIndex.insert(make_pair(hash, pIndexNew)).first;
        if (mi == mapIndex.end())
        {
            return nullptr;
        }
        pIndexNew->phashBlock = &((*mi).first);
    }
    return ((*mi).second);
}

CBlockIndex* CBlockBase::GetBranch(CBlockIndex* pIndexRef, CBlockIndex* pIndex, vector<CBlockIndex*>& vPath)
{
    vPath.clear();
    while (pIndex != pIndexRef)
    {
        if (pIndexRef->GetBlockTime() > pIndex->GetBlockTime())
        {
            pIndexRef = pIndexRef->pPrev;
        }
        else if (pIndex->GetBlockTime() > pIndexRef->GetBlockTime())
        {
            vPath.push_back(pIndex);
            pIndex = pIndex->pPrev;
        }
        else
        {
            vPath.push_back(pIndex);
            pIndex = pIndex->pPrev;
            pIndexRef = pIndexRef->pPrev;
        }
    }
    return pIndex;
}

CBlockIndex* CBlockBase::GetOriginIndex(const uint256& txidMint) const
{
    for (map<uint256, boost::shared_ptr<CBlockFork>>::const_iterator mi = mapFork.begin(); mi != mapFork.end(); ++mi)
    {
        CBlockIndex* pIndex = (*mi).second->GetOrigin();
        if (pIndex->txidMint == txidMint)
        {
            return pIndex;
        }
    }
    return nullptr;
}

void CBlockBase::UpdateBlockHeightIndex(const uint256& hashFork, const uint256& hashBlock, uint32 nBlockTimeStamp, const CDestination& destMint, const uint256& hashRefBlock)
{
    mapForkHeightIndex[hashFork].AddHeightIndex(CBlock::GetBlockHeightByHash(hashBlock), hashBlock, nBlockTimeStamp, destMint, hashRefBlock);
}

void CBlockBase::RemoveBlockIndex(const uint256& hashFork, const uint256& hashBlock)
{
    std::map<uint256, CForkHeightIndex>::iterator it = mapForkHeightIndex.find(hashFork);
    if (it != mapForkHeightIndex.end())
    {
        it->second.RemoveHeightIndex(CBlock::GetBlockHeightByHash(hashBlock), hashBlock);
    }
    mapIndex.erase(hashBlock);
}

void CBlockBase::UpdateBlockRef(const uint256& hashFork, const uint256& hashBlock, const uint256& hashRefBlock)
{
    std::map<uint256, CForkHeightIndex>::iterator it = mapForkHeightIndex.find(hashFork);
    if (it != mapForkHeightIndex.end())
    {
        it->second.UpdateBlockRef(CBlock::GetBlockHeightByHash(hashBlock), hashBlock, hashRefBlock);
    }
}

CBlockIndex* CBlockBase::AddNewIndex(const uint256& hash, const CBlock& block, uint32 nFile, uint32 nOffset, uint256 nChainTrust)
{
    CBlockIndex* pIndexNew = new CBlockIndex(block, nFile, nOffset);
    if (pIndexNew != nullptr)
    {
        map<uint256, CBlockIndex*>::iterator mi = mapIndex.insert(make_pair(hash, pIndexNew)).first;
        pIndexNew->phashBlock = &((*mi).first);

        int64 nMoneySupply = block.GetBlockMint();
        int64 nMoneyDestroy = block.GetBlockMoneyDestroy();
        uint64 nRandBeacon = block.GetBlockBeacon();
        CBlockIndex* pIndexPrev = nullptr;
        map<uint256, CBlockIndex*>::iterator miPrev = mapIndex.find(block.hashPrev);
        if (miPrev != mapIndex.end())
        {
            pIndexPrev = (*miPrev).second;
            pIndexNew->pPrev = pIndexPrev;
            if (!pIndexNew->IsOrigin())
            {
                pIndexNew->pOrigin = pIndexPrev->pOrigin;
                nRandBeacon ^= pIndexNew->pOrigin->nRandBeacon;
            }
            nMoneySupply += pIndexPrev->nMoneySupply;
            nMoneyDestroy += pIndexPrev->nMoneyDestroy;
            nChainTrust += pIndexPrev->nChainTrust;
        }
        pIndexNew->nMoneySupply = nMoneySupply;
        pIndexNew->nMoneyDestroy = nMoneyDestroy;
        pIndexNew->nChainTrust = nChainTrust;
        pIndexNew->nRandBeacon = nRandBeacon;

        uint256 hashRefBlock;
        if (!block.IsPrimary() && !block.vchProof.empty()
            && (block.IsSubsidiary() || block.IsExtended() || (block.IsVacant() && !block.txMint.sendTo.IsNull())))
        {
            CProofOfPiggyback proof;
            if (proof.Load(block.vchProof) && proof.hashRefBlock != 0)
            {
                hashRefBlock = proof.hashRefBlock;
            }
        }

        UpdateBlockHeightIndex(pIndexNew->GetOriginHash(), hash, block.nTimeStamp, block.txMint.sendTo, hashRefBlock);
    }
    return pIndexNew;
}

boost::shared_ptr<CBlockFork> CBlockBase::GetFork(const uint256& hash)
{
    map<uint256, boost::shared_ptr<CBlockFork>>::iterator mi = mapFork.find(hash);
    return (mi != mapFork.end() ? (*mi).second : nullptr);
}

boost::shared_ptr<CBlockFork> CBlockBase::GetFork(const std::string& strName)
{
    for (map<uint256, boost::shared_ptr<CBlockFork>>::iterator mi = mapFork.begin(); mi != mapFork.end(); ++mi)
    {
        const CProfile& profile = (*mi).second->GetProfile();
        if (profile.strName == strName)
        {
            return ((*mi).second);
        }
    }
    return nullptr;
}

boost::shared_ptr<CBlockFork> CBlockBase::AddNewFork(const CProfile& profileIn, CBlockIndex* pIndexLast)
{
    boost::shared_ptr<CBlockFork> spFork = boost::shared_ptr<CBlockFork>(new CBlockFork(profileIn, pIndexLast));
    if (spFork != nullptr)
    {
        spFork->UpdateNext();
        mapFork.insert(make_pair(pIndexLast->GetOriginHash(), spFork));
    }

    return spFork;
}

bool CBlockBase::LoadForkProfile(const CBlockIndex* pIndexOrigin, CProfile& profile)
{
    CForkContext ctxt;
    if (!RetrieveForkContext(pIndexOrigin->GetBlockHash(), ctxt))
    {
        return false;
    }
    profile = ctxt.GetProfile();
    return true;
}

bool CBlockBase::VerifyDelegateVote(const uint256& hash, CBlockEx& block, int64 nMinEnrollAmount, CDelegateContext& ctxtDelegate)
{
    StdTrace("CBlockBase", "VerifyDelegateVote: height: %d, block: %s", block.GetBlockHeight(), hash.GetHex().c_str());

    //map<CDestination, int64>& mapDelegate = ctxtDelegate.mapVote;
    //map<int, map<CDestination, CDiskPos>>& mapEnrollTx = ctxtDelegate.mapEnrollTx;
    //if (!dbBlock.RetrieveDelegate(block.hashPrev, mapDelegate))
    //{
    //    StdError("CBlockBase", "Verify delegate vote: RetrieveDelegate fail");
    //    return false;
    //}
    if (!dbBlock.RetrieveDelegateContext(block.hashPrev, ctxtDelegate))
    {
        StdError("CBlockBase", "Verify delegate vote: Retrieve delegate context fail, prev: %s", block.hashPrev.GetHex().c_str());
        return false;
    }
    map<CDestination, int64>& mapDelegate = ctxtDelegate.mapVote;
    map<int, map<CDestination, CDiskPos>>& mapEnrollTx = ctxtDelegate.mapEnrollTx;
    mapEnrollTx.clear();

    vector<pair<CDestination, int64>> vDestVote;
    {
        CTemplateId tid;
        if (block.txMint.sendTo.GetTemplateId(tid) && tid.GetType() == TEMPLATE_DELEGATE)
        {
            vDestVote.push_back(make_pair(block.txMint.sendTo, block.txMint.nAmount));
        }
    }

    CBufStream ss;
    CVarInt var(block.vtx.size());
    uint32 nOffset = block.GetTxSerializedOffset()
                     + ss.GetSerializeSize(block.txMint)
                     + ss.GetSerializeSize(var);
    for (int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        const CTxContxt& txContxt = block.vTxContxt[i];

        CDestination destInDelegateTemplate;
        CDestination sendToDelegateTemplate;
        if (!CTemplateVote::ParseDelegateDest(txContxt.destIn, tx.sendTo, tx.vchSig, destInDelegateTemplate, sendToDelegateTemplate))
        {
            StdLog("CBlockBase", "Verify delegate vote: parse delegate dest fail, destIn: %s, sendTo: %s, block: %s, txid: %s",
                   CAddress(txContxt.destIn).ToString().c_str(), CAddress(tx.sendTo).ToString().c_str(), hash.GetHex().c_str(), tx.GetHash().GetHex().c_str());
            return false;
        }
        if (!sendToDelegateTemplate.IsNull())
        {
            vDestVote.push_back(make_pair(sendToDelegateTemplate, tx.nAmount));
        }
        if (!destInDelegateTemplate.IsNull())
        {
            vDestVote.push_back(make_pair(destInDelegateTemplate, 0 - (tx.nAmount + tx.nTxFee)));
        }

        if (tx.nType == CTransaction::TX_CERT)
        {
            if (destInDelegateTemplate.IsNull())
            {
                StdLog("CBlockBase", "Verify delegate vote: TX_CERT destInDelegate is null, destInDelegate: %s, destIn: %s, sendTo: %s, block: %s, txid: %s",
                       CAddress(destInDelegateTemplate).ToString().c_str(),
                       CAddress(txContxt.destIn).ToString().c_str(),
                       CAddress(tx.sendTo).ToString().c_str(),
                       hash.GetHex().c_str(),
                       tx.GetHash().GetHex().c_str());
                return false;
            }
            int64 nDelegateVote = mapDelegate[destInDelegateTemplate];
            if (nDelegateVote < nMinEnrollAmount)
            {
                StdLog("CBlockBase", "Verify delegate vote: TX_CERT not enough votes, destInDelegate: %s, delegate vote: %.6f, weight ratio: %.6f, txid: %s",
                       CAddress(destInDelegateTemplate).ToString().c_str(), ValueFromToken(nDelegateVote), ValueFromToken(nMinEnrollAmount), tx.GetHash().GetHex().c_str());
                return false;
            }

            int nCertAnchorHeight = 0;
            try
            {
                CIDataStream is(tx.vchData);
                is >> nCertAnchorHeight;
            }
            catch (...)
            {
                StdLog("CBlockBase", "Verify delegate vote: TX_CERT vchData error, destInDelegate: %s, delegate vote: %.6f, weight ratio: %.6f, txid: %s",
                       CAddress(destInDelegateTemplate).ToString().c_str(), ValueFromToken(nDelegateVote), ValueFromToken(nMinEnrollAmount), tx.GetHash().GetHex().c_str());
                return false;
            }
            mapEnrollTx[nCertAnchorHeight].insert(make_pair(destInDelegateTemplate, CDiskPos(0, nOffset)));
            StdTrace("CBlockBase", "VerifyDelegateVote: Enroll cert tx, anchor height: %d, nAmount: %.6f, vote: %.6f, destInDelegate: %s, txid: %s",
                     nCertAnchorHeight, ValueFromToken(tx.nAmount), ValueFromToken(nDelegateVote), CAddress(destInDelegateTemplate).ToString().c_str(), tx.GetHash().GetHex().c_str());
        }

        if (tx.nType == CTransaction::TX_TOKEN && tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_DELEGATE)
        {
            auto ptrDelegate = CTemplate::CreateTemplatePtr(TEMPLATE_DELEGATE, tx.vchSig);
            if (ptrDelegate == nullptr)
            {
                StdError("BlockBase", "VerifyDelegateVote: CreateTemplatePtr failed, block: %s", hash.ToString().c_str());
                return false;
            }
            auto objDelegate = boost::dynamic_pointer_cast<CTemplateDelegate>(ptrDelegate);
            if (txContxt.destIn == objDelegate->destOwner && tx.vchData.size() > 0)
            {
                std::string strName(tx.vchData.begin(), tx.vchData.end());
                ctxtDelegate.mapDelegateName[tx.sendTo] = strName;
            }
        }

        nOffset += ss.GetSerializeSize(tx);
    }
    for (const auto& d : vDestVote)
    {
        mapDelegate[d.first] += d.second;
        /*if (d.second > 0)
        {
            StdTrace("CBlockBase", "VerifyDelegateVote: sendToDelegate: %s, nAmount: %.6f, AddUp: %.6f",
                     CAddress(d.first).ToString().c_str(), ValueFromToken(d.second), ValueFromToken(mapDelegate[d.first]));
        }
        else
        {
            StdTrace("CBlockBase", "VerifyDelegateVote: destInDelegate: %s, nAmount+nTxFee: %.6f, AddUp: %.6f",
                     CAddress(d.first).ToString().c_str(), ValueFromToken(0 - d.second), ValueFromToken(mapDelegate[d.first]));
        }*/
    }
    /*{
        for (auto it = mapDelegate.begin(); it != mapDelegate.end(); ++it)
        {
            StdTrace("CBlockBase", "VerifyDelegateVote: destDelegate: %s, votes: %.6f",
                     CAddress(it->first).ToString().c_str(), ValueFromToken(it->second));
        }
    }*/
    return true;
}

bool CBlockBase::UpdateDelegate(const uint256& hash, CBlockEx& block, const CDiskPos& posBlock, CDelegateContext& ctxtDelegate)
{
    for (auto& dEnrollTx : ctxtDelegate.mapEnrollTx)
    {
        for (auto& dDest : dEnrollTx.second)
        {
            dDest.second.nFile = posBlock.nFile;
            dDest.second.nOffset += posBlock.nOffset;
        }
    }
    if (!dbBlock.UpdateDelegateContext(hash, ctxtDelegate))
    {
        StdError("BlockBase", "Update delegate context failed, block: %s", hash.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateInvest(const uint256& hash, const CBlockEx& block)
{
    vector<pair<CDestination, CInvest>> vTxInvest;
    int nCurBlockHeight = block.GetBlockHeight();
    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        const CTxContxt& txContxt = block.vTxContxt[i];
        if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_VOTE)
        {
            auto ptrVote = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, tx.vchSig);
            if (ptrVote == nullptr)
            {
                StdError("BlockBase", "Update invest: CreateTemplatePtr failed, block: %s", hash.ToString().c_str());
                return false;
            }
            auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptrVote);

            CInvest invest;
            invest.nCycle = objVote->nCycle;
            invest.destReward = objVote->destOwner;
            invest.nInvestAmount = tx.nAmount;
            invest.nStartHeight = nCurBlockHeight;

            vTxInvest.push_back(make_pair(tx.sendTo, invest));
        }
        if (txContxt.destIn.IsTemplate() && txContxt.destIn.GetTemplateId().GetType() == TEMPLATE_VOTE)
        {
            CInvest invest;
            invest.nInvestAmount -= (tx.nAmount + tx.nTxFee);
            vTxInvest.push_back(make_pair(txContxt.destIn, invest));
        }
    }

    CInvestContext ctxtInvest;
    if (!dbBlock.GetFullInvest(hash, block.hashPrev, vTxInvest, ctxtInvest))
    {
        StdError("BlockBase", "Update invest: Get full invest failed, block: %s", hash.ToString().c_str());
        return false;
    }

    int64 nBlockRewardTxCount = ctxtInvest.vNoPackingReward.size();
    if (nBlockRewardTxCount <= nMaxBlockRewardTxCount)
    {
        ctxtInvest.vNoPackingReward.clear();
    }
    else
    {
        ctxtInvest.vNoPackingReward.erase(ctxtInvest.vNoPackingReward.begin(), ctxtInvest.vNoPackingReward.begin() + nMaxBlockRewardTxCount);
    }
    int nPrevHeight = CBlock::GetBlockHeightByHash(block.hashPrev);
    for (auto it = ctxtInvest.mapInvest.begin(); it != ctxtInvest.mapInvest.end();)
    {
        CInvest& invest = it->second;
        if (invest.nInvestAmount > 0)
        {
            if ((nPrevHeight - invest.nStartHeight) == (invest.nCycle * INVEST_DAY_HEIGHT))
            {
                int64 nRewardAmount = CalcInvestReward(invest.nCycle, invest.nInvestAmount);
                //StdTrace("BlockBase", "Update invest: Invest reward: Rc: %ld:%d nRewardAmount: %ld, destReward: %s",
                //         nBlockRewardTxCount, nMaxBlockRewardTxCount, nRewardAmount, CAddress(invest.destReward).ToString().c_str());
                if (nBlockRewardTxCount++ >= nMaxBlockRewardTxCount)
                {
                    ctxtInvest.vNoPackingReward.push_back(make_pair(invest.destReward, nRewardAmount));
                }
            }
        }
        ++it;
    }

    if (!dbBlock.AddBlockInvest(hash, block.hashPrev, vTxInvest, ctxtInvest.vNoPackingReward))
    {
        StdError("BlockBase", "Update invest: Update block invest context failed, block: %s", hash.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::UpdateActivate(const uint256& hash, const CBlockEx& block)
{
    CActivateContext ctxtActivate;
    if (block.hashPrev != 0)
    {
        if (!dbBlock.RetrieveActivateContext(block.hashPrev, ctxtActivate))
        {
            StdError("BlockBase", "Update activate: Retrieve activate context failed, prev block: %s", block.hashPrev.ToString().c_str());
            return false;
        }
    }

    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_ACTIVATE)
        {
            auto ptr = CTemplate::CreateTemplatePtr(TEMPLATE_ACTIVATE, tx.vchSig);
            if (ptr == nullptr)
            {
                StdError("BlockBase", "Update activate: CreateTemplatePtr failed, block: %s", hash.ToString().c_str());
                return false;
            }
            auto obj = boost::dynamic_pointer_cast<CTemplateActivate>(ptr);

            int& height = ctxtActivate.mapActivate[obj->destOwner];
            if (height == 0)
            {
                height = block.GetBlockHeight();
            }
        }
    }

    if (!dbBlock.UpdateActivateContext(hash, ctxtActivate))
    {
        StdError("BlockBase", "Update activate: Update activate context failed, block: %s", hash.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockBase::GetTxUnspent(const uint256 fork, const CTxOutPoint& out, CTxOut& unspent)
{
    return dbBlock.RetrieveTxUnspent(fork, out, unspent);
}

bool CBlockBase::GetTxNewIndex(CBlockView& view, CBlockIndex* pIndexNew, vector<pair<uint256, CTxIndex>>& vTxNew, vector<pair<CAddrTxIndex, CAddrTxInfo>>& vAddrTxNew)
{
    vector<CBlockIndex*> vPath;
    if (view.GetFork() != nullptr && view.GetFork()->GetLast() != nullptr)
    {
        GetBranch(view.GetFork()->GetLast(), pIndexNew, vPath);
    }
    else
    {
        vPath.push_back(pIndexNew);
    }

    CBufStream ss;
    for (int i = vPath.size() - 1; i >= 0; i--)
    {
        CBlockIndex* pIndex = vPath[i];
        CBlockEx block;
        if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset))
        {
            return false;
        }
        int nHeight = pIndex->GetBlockHeight();
        uint32 nOffset = pIndex->nOffset + block.GetTxSerializedOffset();

        int nBlockSeq = 0;
        if (fCfgAddrTxIndex && pIndex->IsExtended())
        {
            nBlockSeq = pIndex->GetExtendedSequence();
        }

        if (!block.txMint.sendTo.IsNull())
        {
            CTxIndex txIndex(nHeight, pIndex->nFile, nOffset);
            vTxNew.push_back(make_pair(block.txMint.GetHash(), txIndex));

            if (fCfgAddrTxIndex)
            {
                CAddrTxInfo txInfo(CAddrTxInfo::TXI_DIRECTION_TO, CDestination(), block.txMint);
                vAddrTxNew.push_back(make_pair(CAddrTxIndex(block.txMint.sendTo, nHeight, nBlockSeq, 0, block.txMint.GetHash()), txInfo));
            }
        }
        nOffset += ss.GetSerializeSize(block.txMint);

        CVarInt var(block.vtx.size());
        nOffset += ss.GetSerializeSize(var);
        for (int i = 0; i < block.vtx.size(); i++)
        {
            CTransaction& tx = block.vtx[i];
            uint256 txid = tx.GetHash();
            CTxIndex txIndex(nHeight, pIndex->nFile, nOffset);
            vTxNew.push_back(make_pair(txid, txIndex));

            if (fCfgAddrTxIndex)
            {
                const CTxContxt& txContxt = block.vTxContxt[i];
                if (tx.nType == CTransaction::TX_DEFI_REWARD || tx.nType == CTransaction::TX_INVEST_REWARD)
                {
                    CAddrTxInfo txToInfo(CAddrTxInfo::TXI_DIRECTION_TO, CDestination(), tx);
                    vAddrTxNew.push_back(make_pair(CAddrTxIndex(tx.sendTo, nHeight, nBlockSeq, i + 1, txid), txToInfo));
                }
                else if (tx.sendTo == txContxt.destIn)
                {
                    CAddrTxInfo txInfo(CAddrTxInfo::TXI_DIRECTION_TWO, tx.sendTo, tx);
                    vAddrTxNew.push_back(make_pair(CAddrTxIndex(txContxt.destIn, nHeight, nBlockSeq, i + 1, txid), txInfo));
                }
                else
                {
                    if (!txContxt.destIn.IsNull())
                    {
                        CAddrTxInfo txFromInfo(CAddrTxInfo::TXI_DIRECTION_FROM, tx.sendTo, tx);
                        vAddrTxNew.push_back(make_pair(CAddrTxIndex(txContxt.destIn, nHeight, nBlockSeq, i + 1, txid), txFromInfo));
                    }
                    CAddrTxInfo txToInfo(CAddrTxInfo::TXI_DIRECTION_TO, txContxt.destIn, tx);
                    vAddrTxNew.push_back(make_pair(CAddrTxIndex(tx.sendTo, nHeight, nBlockSeq, i + 1, txid), txToInfo));
                }
            }
            nOffset += ss.GetSerializeSize(tx);
        }
    }
    return true;
}

bool CBlockBase::IsValidBlock(CBlockIndex* pForkLast, const uint256& hashBlock)
{
    if (hashBlock != 0)
    {
        int nBlockHeight = CBlock::GetBlockHeightByHash(hashBlock);
        CBlockIndex* pIndex = pForkLast;
        while (pIndex && pIndex->GetBlockHeight() >= nBlockHeight)
        {
            if (pIndex->GetBlockHeight() == nBlockHeight && pIndex->GetBlockHash() == hashBlock)
            {
                return true;
            }
            pIndex = pIndex->pPrev;
        }
    }
    return false;
}

bool CBlockBase::VerifyValidBlock(CBlockIndex* pIndexGenesisLast, const CBlockIndex* pIndex)
{
    if (pIndex->IsOrigin())
    {
        return true;
    }

    uint256 hashRefBlock;
    auto it = mapForkHeightIndex.find(pIndex->GetOriginHash());
    if (it == mapForkHeightIndex.end())
    {
        return false;
    }
    std::map<uint256, CBlockHeightIndex>* pHeightIndex = it->second.GetBlockMintList(pIndex->GetBlockHeight());
    if (pHeightIndex)
    {
        auto mt = pHeightIndex->find(pIndex->GetBlockHash());
        if (mt != pHeightIndex->end() && mt->second.hashRefBlock != 0)
        {
            hashRefBlock = mt->second.hashRefBlock;
        }
    }
    if (hashRefBlock == 0)
    {
        CBlockEx block;
        if (!Retrieve(pIndex, block))
        {
            return false;
        }
        if (!block.vchProof.empty())
        {
            CProofOfPiggyback proof;
            if (proof.Load(block.vchProof) && proof.hashRefBlock != 0)
            {
                hashRefBlock = proof.hashRefBlock;
            }
        }
        if (hashRefBlock == 0)
        {
            return false;
        }
    }
    return IsValidBlock(pIndexGenesisLast, hashRefBlock);
}

CBlockIndex* CBlockBase::GetLongChainLastBlock(const uint256& hashFork, int nStartHeight, CBlockIndex* pIndexGenesisLast, const std::set<uint256>& setInvalidHash)
{
    auto it = mapForkHeightIndex.find(hashFork);
    if (it == mapForkHeightIndex.end())
    {
        return nullptr;
    }
    CForkHeightIndex& indexHeight = it->second;
    CBlockIndex* pMaxTrustIndex = nullptr;
    while (1)
    {
        std::map<uint256, CBlockHeightIndex>* pHeightIndex = indexHeight.GetBlockMintList(nStartHeight);
        if (pHeightIndex == nullptr)
        {
            break;
        }
        auto mt = pHeightIndex->begin();
        for (; mt != pHeightIndex->end(); ++mt)
        {
            const uint256& hashBlock = mt->first;
            if (setInvalidHash.count(hashBlock) == 0)
            {
                CBlockIndex* pIndex;
                if (!(pIndex = GetIndex(hashBlock)))
                {
                    StdError("BlockBase", "GetLongChainLastBlock GetIndex failed, block: %s", hashBlock.ToString().c_str());
                }
                else if (!pIndex->IsOrigin())
                {
                    if (VerifyValidBlock(pIndexGenesisLast, pIndex))
                    {
                        if (pMaxTrustIndex == nullptr)
                        {
                            pMaxTrustIndex = pIndex;
                        }
                        else if (!(pMaxTrustIndex->nChainTrust > pIndex->nChainTrust
                                   || (pMaxTrustIndex->nChainTrust == pIndex->nChainTrust && !pIndex->IsEquivalent(pMaxTrustIndex))))
                        {
                            pMaxTrustIndex = pIndex;
                        }
                    }
                }
            }
        }
        nStartHeight++;
    }
    return pMaxTrustIndex;
}

void CBlockBase::ClearCache()
{
    map<uint256, CBlockIndex*>::iterator mi;
    for (mi = mapIndex.begin(); mi != mapIndex.end(); ++mi)
    {
        delete (*mi).second;
    }
    mapIndex.clear();
    mapForkHeightIndex.clear();
    mapFork.clear();
}

bool CBlockBase::LoadDB()
{
    CWriteLock wlock(rwAccess);

    ClearCache();
    CBlockWalker walker(this);
    if (!dbBlock.WalkThroughBlock(walker))
    {
        StdLog("CBlockBase", "LoadDB: WalkThroughBlock fail");
        ClearCache();
        return false;
    }

    vector<pair<uint256, uint256>> vFork;
    if (!dbBlock.ListFork(vFork))
    {
        StdLog("CBlockBase", "LoadDB: ListFork fail");
        ClearCache();
        return false;
    }
    for (int i = 0; i < vFork.size(); i++)
    {
        CBlockIndex* pIndex = GetIndex(vFork[i].second);
        if (pIndex == nullptr)
        {
            StdLog("CBlockBase", "LoadDB: GetIndex fail, forkid: %s, lastblock: %s", vFork[i].first.GetHex().c_str(), vFork[i].second.GetHex().c_str());
            ClearCache();
            return false;
        }
        CProfile profile;
        if (!LoadForkProfile(pIndex->pOrigin, profile))
        {
            StdLog("CBlockBase", "LoadDB: LoadForkProfile fail, forkid: %s", vFork[i].first.GetHex().c_str());
            return false;
        }
        boost::shared_ptr<CBlockFork> spFork = AddNewFork(profile, pIndex);
        if (spFork == nullptr)
        {
            StdLog("CBlockBase", "LoadDB: AddNewFork fail, forkid: %s", vFork[i].first.GetHex().c_str());
            return false;
        }
    }

    return true;
}

bool CBlockBase::SetupLog(const path& pathLocation, bool fDebug)
{

    if (!log.SetLogFilePath((pathLocation / LOGFILE_NAME).string()))
    {
        return false;
    }
    fDebugLog = fDebug;
    return true;
}

} // namespace storage
} // namespace ibrio
