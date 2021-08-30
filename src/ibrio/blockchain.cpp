// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockchain.h"

#include "delegatecomm.h"
#include "delegateverify.h"
#include "param.h"
#include "template/activate.h"
#include "template/delegate.h"
#include "template/fork.h"
#include "template/proof.h"
#include "template/vote.h"

using namespace std;
using namespace xengine;

#define ENROLLED_CACHE_COUNT (120)
#define AGREEMENT_CACHE_COUNT (16)
#define NO_DEFI_TEMPLATE_ADDRESS_HEIGHT (500824)

namespace ibrio
{

//////////////////////////////
// CBlockChain

CBlockChain::CBlockChain()
  : cacheEnrolled(ENROLLED_CACHE_COUNT), cacheAgreement(AGREEMENT_CACHE_COUNT)
{
    pCoreProtocol = nullptr;
    pTxPool = nullptr;
    pForkManager = nullptr;
}

CBlockChain::~CBlockChain()
{
}

bool CBlockChain::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("txpool", pTxPool))
    {
        Error("Failed to request txpool");
        return false;
    }

    if (!GetObject("forkmanager", pForkManager))
    {
        Error("Failed to request forkmanager");
        return false;
    }

    return true;
}

void CBlockChain::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pTxPool = nullptr;
    pForkManager = nullptr;
}

bool CBlockChain::HandleInvoke()
{
    CBlock blockGenesis;
    pCoreProtocol->GetGenesisBlock(blockGenesis);

    nMaxBlockRewardTxCount = GetBlockInvestRewardTxMaxCount();
    Log("HandleInvoke: nMaxBlockRewardTxCount: %d", nMaxBlockRewardTxCount);

    if (!cntrBlock.Initialize(Config()->pathData, blockGenesis.GetHash(), Config()->fDebug, Config()->fAddrTxIndex, nMaxBlockRewardTxCount))
    {
        Error("Failed to initialize container");
        return false;
    }

    if (cntrBlock.IsEmpty())
    {
        if (!InsertGenesisBlock(blockGenesis))
        {
            Error("Failed to create genesis block");
            return false;
        }
    }

    InitCheckPoints();

    // Check local block compared to checkpoint
    if (Config()->nMagicNum == MAINNET_MAGICNUM)
    {
        std::map<uint256, CForkStatus> mapForkStatus;
        GetForkStatus(mapForkStatus);
        for (const auto& fork : mapForkStatus)
        {
            CBlock block;
            if (!FindPreviousCheckPointBlock(fork.first, block))
            {
                StdError("BlockChain", "Find CheckPoint on fork %s Error when the node starting, you should purge data(ibrio -purge) to resync blockchain",
                         fork.first.ToString().c_str());
                return false;
            }
        }
    }

    // build defi fork
    multimap<int, CBlockIndex*> mapForkIndex;
    cntrBlock.ListForkIndex(mapForkIndex);
    for (multimap<int, CBlockIndex*>::iterator it = mapForkIndex.begin(); it != mapForkIndex.end(); ++it)
    {
        uint256 hashFork = it->second->GetOriginHash();
        CProfile profile;
        if (!GetForkProfile(hashFork, profile))
        {
            StdError("BlockChain", "Load fork profile fail: %s", hashFork.ToString().c_str());
            return false;
        }

        if (profile.nForkType == FORK_TYPE_DEFI)
        {
            defiReward.AddFork(hashFork, profile);
        }
    }

    return true;
}

void CBlockChain::HandleHalt()
{
    cntrBlock.Deinitialize();
    cacheEnrolled.Clear();
    cacheAgreement.Clear();
}

void CBlockChain::GetForkStatus(map<uint256, CForkStatus>& mapForkStatus)
{
    mapForkStatus.clear();

    multimap<int, CBlockIndex*> mapForkIndex;
    cntrBlock.ListForkIndex(mapForkIndex);

    for (multimap<int, CBlockIndex*>::iterator it = mapForkIndex.begin(); it != mapForkIndex.end(); ++it)
    {
        CBlockIndex* pIndex = (*it).second;
        int nForkHeight = (*it).first;
        uint256 hashFork = pIndex->GetOriginHash();
        uint256 hashParent = pIndex->GetParentHash();

        if (hashParent != 0)
        {
            bool fHave = false;
            multimap<int, uint256>& mapSubline = mapForkStatus[hashParent].mapSubline;
            auto mt = mapSubline.lower_bound(nForkHeight);
            while (mt != mapSubline.upper_bound(nForkHeight))
            {
                if (mt->second == hashFork)
                {
                    fHave = true;
                    break;
                }
                ++mt;
            }
            if (!fHave)
            {
                mapSubline.insert(make_pair(nForkHeight, hashFork));
            }
        }

        map<uint256, CForkStatus>::iterator mi = mapForkStatus.find(hashFork);
        if (mi == mapForkStatus.end())
        {
            CProfile profile;
            GetForkProfile(hashFork, profile);
            mi = mapForkStatus.insert(make_pair(hashFork, CForkStatus(hashFork, hashParent, nForkHeight, profile.nForkType))).first;
        }
        CForkStatus& status = (*mi).second;
        status.hashPrevBlock = pIndex->GetPrevHash();
        status.hashLastBlock = pIndex->GetBlockHash();
        status.nLastBlockTime = pIndex->GetBlockTime();
        status.nLastBlockHeight = pIndex->GetBlockHeight();
        status.nMoneySupply = pIndex->GetMoneySupply();
        status.nMoneyDestroy = pIndex->GetMoneyDestroy();
        status.nMintType = pIndex->nMintType;
    }
}

void CBlockChain::GetValidForkStatus(map<uint256, CForkStatus>& mapForkStatus)
{
    map<uint256, bool> mapValidFork;
    pForkManager->GetValidForkList(mapValidFork);

    GetForkStatus(mapForkStatus);

    map<uint256, CForkStatus>::iterator it = mapForkStatus.begin();
    while (it != mapForkStatus.end())
    {
        const auto nt = mapValidFork.find(it->first);
        if (nt == mapValidFork.end() || !nt->second)
        {
            mapForkStatus.erase(it++);
            continue;
        }
        ++it;
    }
}

bool CBlockChain::GetForkProfile(const uint256& hashFork, CProfile& profile)
{
    return cntrBlock.RetrieveProfile(hashFork, profile);
}

bool CBlockChain::GetForkContext(const uint256& hashFork, CForkContext& ctxt)
{
    return cntrBlock.RetrieveForkContext(hashFork, ctxt);
}

bool CBlockChain::GetForkAncestry(const uint256& hashFork, vector<pair<uint256, uint256>> vAncestry)
{
    return cntrBlock.RetrieveAncestry(hashFork, vAncestry);
}

int CBlockChain::GetBlockCount(const uint256& hashFork)
{
    int nCount = 0;
    CBlockIndex* pIndex = nullptr;
    if (cntrBlock.RetrieveFork(hashFork, &pIndex))
    {
        while (pIndex != nullptr)
        {
            pIndex = pIndex->pPrev;
            ++nCount;
        }
    }
    return nCount;
}

bool CBlockChain::GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }
    hashFork = pIndex->GetOriginHash();
    nHeight = pIndex->GetBlockHeight();
    return true;
}

bool CBlockChain::GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight, uint256& hashNext)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }
    hashFork = pIndex->GetOriginHash();
    nHeight = pIndex->GetBlockHeight();
    if (pIndex->pNext != nullptr)
    {
        hashNext = pIndex->pNext->GetBlockHash();
    }
    else
    {
        hashNext = 0;
    }
    return true;
}

bool CBlockChain::GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex) || pIndex->GetBlockHeight() < nHeight)
    {
        return false;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() > nHeight)
    {
        pIndex = pIndex->pPrev;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() == nHeight && pIndex->IsExtended())
    {
        pIndex = pIndex->pPrev;
    }
    hashBlock = !pIndex ? uint64(0) : pIndex->GetBlockHash();
    return (pIndex != nullptr);
}

bool CBlockChain::GetBlockHash(const uint256& hashFork, int nHeight, vector<uint256>& vBlockHash)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex) || pIndex->GetBlockHeight() < nHeight)
    {
        return false;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() > nHeight)
    {
        pIndex = pIndex->pPrev;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() == nHeight)
    {
        vBlockHash.push_back(pIndex->GetBlockHash());
        pIndex = pIndex->pPrev;
    }
    std::reverse(vBlockHash.begin(), vBlockHash.end());
    return (!vBlockHash.empty());
}

bool CBlockChain::GetBlockStatus(const uint256& hashBlock, CBlockStatus& status)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        return false;
    }

    status.hashPrevBlock = pIndex->GetPrevHash();
    status.hashBlock = pIndex->GetBlockHash();
    status.nBlockTime = pIndex->GetBlockTime();
    status.nBlockHeight = pIndex->GetBlockHeight();
    status.nMoneySupply = pIndex->GetMoneySupply();
    status.nMoneyDestroy = pIndex->GetMoneyDestroy();
    status.nMintType = pIndex->nMintType;

    return true;
}

bool CBlockChain::GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex) || pIndex->GetBlockHeight() < nHeight)
    {
        return false;
    }
    while (pIndex != nullptr && pIndex->GetBlockHeight() > nHeight)
    {
        pIndex = pIndex->pPrev;
    }
    if (pIndex == nullptr || pIndex->GetBlockHeight() != nHeight)
    {
        return false;
    }

    hashBlock = pIndex->GetBlockHash();
    nTime = pIndex->GetBlockTime();

    return true;
}

bool CBlockChain::GetLastBlockStatus(const uint256& hashFork, CBlockStatus& status)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex))
    {
        return false;
    }
    status.hashPrevBlock = pIndex->GetPrevHash();
    status.hashBlock = pIndex->GetBlockHash();
    status.nBlockHeight = pIndex->GetBlockHeight();
    status.nBlockTime = pIndex->GetBlockTime();
    status.nMoneySupply = pIndex->GetMoneySupply();
    status.nMoneyDestroy = pIndex->GetMoneyDestroy();
    status.nMintType = pIndex->nMintType;
    return true;
}

bool CBlockChain::GetLastBlockTime(const uint256& hashFork, int nDepth, vector<int64>& vTime)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveFork(hashFork, &pIndex))
    {
        return false;
    }

    vTime.clear();
    vTime.reserve(nDepth);
    while (nDepth > 0 && pIndex != nullptr)
    {
        vTime.push_back(pIndex->GetBlockTime());
        if (!pIndex->IsExtended())
        {
            nDepth--;
        }
        pIndex = pIndex->pPrev;
    }
    return true;
}

bool CBlockChain::GetBlock(const uint256& hashBlock, CBlock& block)
{
    return cntrBlock.Retrieve(hashBlock, block);
}

bool CBlockChain::GetBlockEx(const uint256& hashBlock, CBlockEx& block)
{
    return cntrBlock.Retrieve(hashBlock, block);
}

bool CBlockChain::GetOrigin(const uint256& hashFork, CBlock& block)
{
    return cntrBlock.RetrieveOrigin(hashFork, block);
}

bool CBlockChain::Exists(const uint256& hashBlock)
{
    return cntrBlock.Exists(hashBlock);
}

bool CBlockChain::GetTransaction(const uint256& txid, CTransaction& tx)
{
    return cntrBlock.RetrieveTx(txid, tx);
}

bool CBlockChain::GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight)
{
    return cntrBlock.RetrieveTx(txid, tx, hashFork, nHeight);
}

bool CBlockChain::ExistsTx(const uint256& txid)
{
    return cntrBlock.ExistsTx(txid);
}

bool CBlockChain::GetTxLocation(const uint256& txid, uint256& hashFork, int& nHeight)
{
    return cntrBlock.RetrieveTxLocation(txid, hashFork, nHeight);
}

bool CBlockChain::GetTxUnspent(const uint256& hashFork, const vector<CTxIn>& vInput, vector<CTxOut>& vOutput)
{
    vOutput.resize(vInput.size());
    storage::CBlockView view;
    if (!cntrBlock.GetForkBlockView(hashFork, view))
    {
        return false;
    }

    for (std::size_t i = 0; i < vInput.size(); i++)
    {
        if (vOutput[i].IsNull())
        {
            view.RetrieveUnspent(vInput[i].prevout, vOutput[i]);
        }
    }
    return true;
}

bool CBlockChain::FilterTx(const uint256& hashFork, CTxFilter& filter)
{
    return cntrBlock.FilterTx(hashFork, filter);
}

bool CBlockChain::FilterTx(const uint256& hashFork, int nDepth, CTxFilter& filter)
{
    return cntrBlock.FilterTx(hashFork, nDepth, filter);
}

bool CBlockChain::ListForkContext(vector<CForkContext>& vForkCtxt, map<uint256, CValidForkId>& mapValidForkId)
{
    return cntrBlock.ListForkContext(vForkCtxt, mapValidForkId);
}

Errno CBlockChain::AddNewBlock(const CBlock& block, CBlockChainUpdate& update)
{
    uint256 hash = block.GetHash();
    Errno err = OK;

    if (cntrBlock.Exists(hash))
    {
        Log("AddNewBlock Already Exists : %s ", hash.ToString().c_str());
        return ERR_ALREADY_HAVE;
    }

    err = pCoreProtocol->ValidateBlock(block);
    if (err != OK)
    {
        Log("AddNewBlock Validate Block Error(%s) : %s ", ErrorString(err), hash.ToString().c_str());
        return err;
    }

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        Log("AddNewBlock Retrieve Prev Index Error: %s ", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    uint256 forkid = pIndexPrev->GetOriginHash();

    int64 nReward = 0;
    CDelegateAgreement agreement;
    size_t nEnrollTrust = 0;
    CBlockIndex* pIndexRef = nullptr;
    err = VerifyBlock(hash, block, pIndexPrev, nReward, agreement, nEnrollTrust, &pIndexRef);
    if (err != OK)
    {
        Log("AddNewBlock Verify Block Error(%s) : %s ", ErrorString(err), hash.ToString().c_str());
        return err;
    }

    bool fGetBranchBlock = true;
    if (block.IsVacant())
    {
        do
        {
            if (!block.IsPrimary() && pIndexRef
                && !cntrBlock.VerifyRefBlock(pCoreProtocol->GetGenesisBlockHash(), pIndexRef->GetBlockHash()))
            {
                fGetBranchBlock = false;
                break;
            }

            uint256 nNewChainTrust;
            if (!pCoreProtocol->GetBlockTrust(block, nNewChainTrust, pIndexPrev, agreement, pIndexRef, nEnrollTrust))
            {
                break;
            }
            nNewChainTrust += pIndexPrev->nChainTrust;

            CBlockIndex* pIndexForkLast = nullptr;
            if (cntrBlock.RetrieveFork(forkid, &pIndexForkLast) && pIndexForkLast->nChainTrust > nNewChainTrust)
            {
                fGetBranchBlock = false;
                break;
            }
        } while (0);
    }

    storage::CBlockView view;
    if (!cntrBlock.GetBlockView(block.hashPrev, view, !block.IsOrigin(), fGetBranchBlock))
    {
        Log("AddNewBlock Get Block View Error: %s ", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    if (!block.IsVacant() || !block.txMint.sendTo.IsNull())
    {
        view.AddTx(block.txMint.GetHash(), block.txMint, block.GetBlockHeight(), CTxContxt());
    }

    CBlockEx blockex(block);
    vector<CTxContxt>& vTxContxt = blockex.vTxContxt;

    int64 nTotalFee = 0;
    if (block.vtx.size() > 0)
    {
        vTxContxt.reserve(block.vtx.size());
    }

    // defi
    list<CDeFiReward> listDeFiReward;
    bool fDeFiFork = defiReward.ExistFork(forkid);
    if (!block.IsVacant() && fDeFiFork)
    {
        listDeFiReward = GetDeFiReward(forkid, pIndexPrev->GetBlockHash(), block.GetBlockHeight(), block.vtx.size());
    }
    list<CDeFiReward>::const_iterator itListDeFi = listDeFiReward.begin();

    int64 nIgnoreVerifyTx = 0;
    if (block.IsPrimary())
    {
        if (!VerifyInvestRewardTx(block, nIgnoreVerifyTx))
        {
            Error("AddNewBlock verify invest reward tx fail, block: %s", hash.ToString().c_str());
            return ERR_TRANSACTION_INVALID;
        }
        if (!VerifyBlockInvestVoteTx(block))
        {
            Error("AddNewBlock verify invest vote tx fail, block: %s", hash.ToString().c_str());
            return ERR_TRANSACTION_INVALID;
        }
    }

    // get fork context
    CProfile profile;
    if (!GetForkProfile(forkid, profile))
    {
        Error("AddNewBlock get fork profile error, block: %s, fork: %s", hash.ToString().c_str(), forkid.ToString().c_str());
        return ERR_BLOCK_INVALID_FORK;
    }

    // verify tx
    uint256 mintHeightTxid;
    for (const CTransaction& tx : block.vtx)
    {
        uint256 txid = tx.GetHash();
        CTxContxt txContxt;
        err = GetTxContxt(view, tx, txContxt);
        if (err != OK)
        {
            Log("AddNewBlock Get txContxt Error([%d] %s) : %s ", err, ErrorString(err), txid.ToString().c_str());
            return err;
        }

        if (nIgnoreVerifyTx > 0)
        {
            nIgnoreVerifyTx--;
        }
        else
        {
            // non-defi fork should not exist defi tx
            if (!fDeFiFork && (tx.nType == CTransaction::TX_DEFI_REWARD || tx.nType == CTransaction::TX_DEFI_RELATION || tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT))
            {
                Log("AddNewBlock non-defi fork don't allow reward tx and relation tx");
                return ERR_TRANSACTION_INVALID;
            }

            // defi tx must be next to each other from the beginning
            if (itListDeFi != listDeFiReward.end())
            {
                if (tx.nType != CTransaction::TX_DEFI_REWARD)
                {
                    Log("AddNewBlock should be reward tx at the beginning of vtx of block: %s", hash.ToString().c_str());
                    return ERR_TRANSACTION_INVALID;
                }

                if ((itListDeFi->dest != tx.sendTo) || (itListDeFi->nReward != tx.nAmount + tx.nTxFee) || (itListDeFi->hashAnchor != tx.hashAnchor))
                {
                    Log("AddNewBlock Check defi reward tx error, block: %s, tx: %s. dest: %s, should: %s. reward: %ld, should: %ld. anchor: %s, should: %s",
                        hash.ToString().c_str(), txid.ToString().c_str(), CAddress(tx.sendTo).ToString().c_str(), CAddress(itListDeFi->dest).ToString().c_str(),
                        tx.nAmount + tx.nTxFee, itListDeFi->nReward, tx.hashAnchor.ToString().c_str(), itListDeFi->hashAnchor.ToString().c_str());
                    return ERR_TRANSACTION_INVALID;
                }
                ++itListDeFi;
            }
            else
            {
                if (tx.nType == CTransaction::TX_DEFI_REWARD)
                {
                    Log("AddNewBlock reward tx is redundant, txid: %s, block: %s", txid.ToString().c_str(), hash.ToString().c_str());
                    return ERR_TRANSACTION_INVALID;
                }
            }

            if (tx.nType != CTransaction::TX_DEFI_REWARD)
            {
                err = pCoreProtocol->VerifyBlockTx(tx, txContxt, pIndexPrev, block.GetBlockHeight(), forkid, profile);
                if (err != OK)
                {
                    Log("AddNewBlock Verify BlockTx Error(%s) : %s ", ErrorString(err), txid.ToString().c_str());
                    return err;
                }
            }

            // check mint height tx uniqueness
            if (tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT)
            {
                if (!mintHeightTxid)
                {
                    mintHeightTxid = txid;
                }
                else
                {
                    Log("AddNewBlock there are too many mint height tx in block: %s, tx1: %s, tx2: %s", hash.ToString().c_str(), mintHeightTxid.ToString().c_str(), txid.ToString().c_str());
                    return ERR_TRANSACTION_TOO_MANY_MINTHEIGHT_TX;
                }
            }

            if (tx.nTimeStamp > block.nTimeStamp)
            {
                Log("AddNewBlock Verify BlockTx time fail: tx time: %d, block time: %d, tx: %s, block: %s",
                    tx.nTimeStamp, block.nTimeStamp, txid.ToString().c_str(), hash.GetHex().c_str());
                return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
            }
        }

        vTxContxt.push_back(txContxt);
        if (!view.AddTx(txid, tx, block.GetBlockHeight(), txContxt))
        {
            Log("AddNewBlock Add block view tx error, txid: %s", txid.ToString().c_str());
            return ERR_BLOCK_TRANSACTIONS_INVALID;
        }

        StdTrace("BlockChain", "AddNewBlock: verify tx success, new tx: %s, new block: %s", txid.GetHex().c_str(), hash.GetHex().c_str());

        nTotalFee += tx.nTxFee;
    }
    view.AddBlock(hash, blockex);

    if (block.txMint.nAmount > nTotalFee + nReward)
    {
        Log("AddNewBlock Mint tx amount invalid : (%ld > %ld + %ld)", block.txMint.nAmount, nTotalFee, nReward);
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }

    // Get block trust
    uint256 nChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nChainTrust, pIndexPrev, agreement, pIndexRef, nEnrollTrust))
    {
        Log("AddNewBlock get block trust fail, block: %s", hash.GetHex().c_str());
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }
    StdTrace("BlockChain", "AddNewBlock block chain trust: %s", nChainTrust.GetHex().c_str());

    blockex.nBlockTrust = nChainTrust;

    CBlockIndex* pIndexNew;
    if (!cntrBlock.AddNew(hash, blockex, &pIndexNew, nChainTrust, pCoreProtocol->MinEnrollAmount()))
    {
        Log("AddNewBlock Storage AddNew Error : %s ", hash.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    Log("AddNew Block : %s", pIndexNew->ToString().c_str());

    if (pIndexNew->IsPrimary() && !AddBlockForkContext(blockex))
    {
        Log("AddNewBlock add block fork fail, block: %s", hash.ToString().c_str());
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }

    if (!pIndexNew->IsPrimary() && pIndexRef
        && !cntrBlock.VerifyRefBlock(pCoreProtocol->GetGenesisBlockHash(), pIndexRef->GetBlockHash()))
    {
        Log("AddNew Block: Ref block short chain, refblock: %s, new block: %s, fork: %s",
            pIndexRef->GetBlockHash().GetHex().c_str(), hash.GetHex().c_str(), pIndexNew->GetOriginHash().GetHex().c_str());
        return OK;
    }

    CBlockIndex* pIndexFork = nullptr;
    if (cntrBlock.RetrieveFork(pIndexNew->GetOriginHash(), &pIndexFork)
        && (pIndexFork->nChainTrust > pIndexNew->nChainTrust
            || (pIndexFork->nChainTrust == pIndexNew->nChainTrust && !pIndexNew->IsEquivalent(pIndexFork))))
    {
        Log("AddNew Block : Short chain, new block height: %d, block type: %s, block: %s, fork chain trust: %s, fork last block: %s, fork: %s",
            pIndexNew->GetBlockHeight(), GetBlockTypeStr(block.nType, block.txMint.nType).c_str(), hash.GetHex().c_str(),
            pIndexFork->nChainTrust.GetHex().c_str(), pIndexFork->GetBlockHash().GetHex().c_str(), pIndexFork->GetOriginHash().GetHex().c_str());
        return OK;
    }

    if (!cntrBlock.CommitBlockView(view, pIndexNew))
    {
        Log("AddNewBlock Storage Commit BlockView Error : %s ", hash.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    // update profile in defiReward
    if (!!mintHeightTxid)
    {
        if (!GetForkProfile(forkid, profile))
        {
            Error("AddNewBlock get fork profile error after commit, block: %s, fork: %s", hash.ToString().c_str(), forkid.ToString().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }
        defiReward.AddFork(forkid, profile);
    }

    update = CBlockChainUpdate(pIndexNew, profile.nForkType);
    view.GetTxUpdated(update.setTxUpdate);
    view.GetBlockChanges(update.vBlockAddNew, update.vBlockRemove);

    StdLog("BlockChain", "AddNewBlock: Commit blockchain success, height: %d, block type: %s, add block: %ld, remove block: %ld, block tx count: %ld, block: %s, fork: %s",
           block.GetBlockHeight(), GetBlockTypeStr(block.nType, block.txMint.nType).c_str(),
           update.vBlockAddNew.size(), update.vBlockRemove.size(),
           block.vtx.size(), hash.GetHex().c_str(), pIndexFork->GetOriginHash().GetHex().c_str());

    if (!update.vBlockRemove.empty())
    {
        uint32 nTxAdd = 0;
        for (const auto& b : update.vBlockAddNew)
        {
            Log("Chain rollback occur[added block]: height: %u hash: %s time: %u",
                b.GetBlockHeight(), b.GetHash().ToString().c_str(), b.nTimeStamp);
            Log("Chain rollback occur[added mint tx]: %s", b.txMint.GetHash().ToString().c_str());
            ++nTxAdd;
            for (const auto& t : b.vtx)
            {
                Log("Chain rollback occur[added tx]: %s", t.GetHash().ToString().c_str());
                ++nTxAdd;
            }
        }
        uint32 nTxDel = 0;
        for (const auto& b : update.vBlockRemove)
        {
            Log("Chain rollback occur[removed block]: height: %u hash: %s time: %u",
                b.GetBlockHeight(), b.GetHash().ToString().c_str(), b.nTimeStamp);
            Log("Chain rollback occur[removed mint tx]: %s", b.txMint.GetHash().ToString().c_str());
            ++nTxDel;
            for (const auto& t : b.vtx)
            {
                Log("Chain rollback occur[removed tx]: %s", t.GetHash().ToString().c_str());
                ++nTxDel;
            }
        }
        Log("Chain rollback occur, [height]: %u [hash]: %s "
            "[nBlockAdd]: %u [nBlockDel]: %u [nTxAdd]: %u [nTxDel]: %u",
            pIndexNew->GetBlockHeight(), pIndexNew->GetBlockHash().ToString().c_str(),
            update.vBlockAddNew.size(), update.vBlockRemove.size(), nTxAdd, nTxDel);
    }

    return OK;
}

Errno CBlockChain::AddNewOrigin(const CBlock& block, CBlockChainUpdate& update)
{
    uint256 hash = block.GetHash();
    Errno err = OK;

    if (cntrBlock.Exists(hash))
    {
        Log("AddNewOrigin Already Exists : %s ", hash.ToString().c_str());
        return ERR_ALREADY_HAVE;
    }

    err = pCoreProtocol->ValidateBlock(block);
    if (err != OK)
    {
        Log("AddNewOrigin Validate Block Error(%s) : %s ", ErrorString(err), hash.ToString().c_str());
        return err;
    }

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        Log("AddNewOrigin Retrieve Prev Index Error: %s ", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    if (pIndexPrev->IsExtended() || pIndexPrev->IsVacant())
    {
        Log("Prev block should not be extended/vacant block");
        return ERR_BLOCK_TYPE_INVALID;
    }

    uint256 hashBlockRef;
    int64 nTimeRef;
    if (!GetLastBlockOfHeight(pCoreProtocol->GetGenesisBlockHash(), block.GetBlockHeight(), hashBlockRef, nTimeRef))
    {
        Log("Failed to query main chain reference block");
        return ERR_SYS_STORAGE_ERROR;
    }
    if (block.GetBlockTime() != nTimeRef)
    {
        Log("Invalid origin block time");
        return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
    }

    CProfile parent;
    if (!cntrBlock.RetrieveProfile(pIndexPrev->GetOriginHash(), parent))
    {
        Log("AddNewOrigin Retrieve parent profile Error: %s ", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }
    CProfile profile;
    err = pCoreProtocol->ValidateOrigin(block, parent, profile);
    if (err != OK)
    {
        Log("AddNewOrigin Validate Origin Error(%s): %s ", ErrorString(err), hash.ToString().c_str());
        return err;
    }

    CBlockIndex* pIndexDuplicated;
    if (cntrBlock.RetrieveFork(profile.strName, &pIndexDuplicated))
    {
        Log("AddNewOrigin Validate Origin Error(duplated fork name): %s, \nexisted: %s",
            hash.ToString().c_str(), pIndexDuplicated->GetOriginHash().GetHex().c_str());
        return ERR_ALREADY_HAVE;
    }

    storage::CBlockView view;

    if (profile.IsIsolated())
    {
        if (!cntrBlock.GetBlockView(view))
        {
            Log("AddNewOrigin Get Block View Error: %s ", block.hashPrev.ToString().c_str());
            return ERR_SYS_STORAGE_ERROR;
        }
    }
    else
    {
        if (!cntrBlock.GetBlockView(block.hashPrev, view, false))
        {
            Log("AddNewOrigin Get Block View Error: %s ", block.hashPrev.ToString().c_str());
            return ERR_SYS_STORAGE_ERROR;
        }
    }

    if (block.txMint.nAmount != 0)
    {
        view.AddTx(block.txMint.GetHash(), block.txMint, block.GetBlockHeight(), CTxContxt());
    }

    // Get block trust
    uint256 nChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nChainTrust, pIndexPrev))
    {
        Log("AddNewOrigin get block trust fail, block: %s", hash.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    CBlockIndex* pIndexNew;
    CBlockEx blockex(block, std::vector<CTxContxt>(), nChainTrust);
    if (!cntrBlock.AddNew(hash, blockex, &pIndexNew, nChainTrust, pCoreProtocol->MinEnrollAmount()))
    {
        Log("AddNewOrigin Storage AddNew Error : %s ", hash.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    Log("AddNew Origin Block : %s ", hash.ToString().c_str());
    Log("    %s", pIndexNew->ToString().c_str());

    if (!cntrBlock.CommitBlockView(view, pIndexNew))
    {
        Log("AddNewOrigin Storage Commit BlockView Error : %s ", hash.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    update = CBlockChainUpdate(pIndexNew, profile.nForkType);
    view.GetTxUpdated(update.setTxUpdate);
    update.vBlockAddNew.push_back(blockex);

    // add defi fork
    if (profile.nForkType == FORK_TYPE_DEFI)
    {
        defiReward.AddFork(hash, profile);
    }

    return OK;
}

bool CBlockChain::GetProofOfWorkTarget(const uint256& hashPrev, int nAlgo, int& nBits, int64& nReward)
{
    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev))
    {
        Log("GetProofOfWorkTarget : Retrieve Prev Index Error: %s ", hashPrev.ToString().c_str());
        return false;
    }
    if (!pIndexPrev->IsPrimary())
    {
        Log("GetProofOfWorkTarget : Previous is not primary: %s ", hashPrev.ToString().c_str());
        return false;
    }
    if (!pCoreProtocol->GetProofOfWorkTarget(pIndexPrev, nAlgo, nBits, nReward))
    {
        Log("GetProofOfWorkTarget : Unknown proof-of-work algo: %s ", hashPrev.ToString().c_str());
        return false;
    }
    return true;
}

bool CBlockChain::GetBlockMintReward(const uint256& hashPrev, int64& nReward)
{
    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev))
    {
        Log("Get block reward: Retrieve Prev Index Error, hashPrev: %s", hashPrev.ToString().c_str());
        return false;
    }

    if (pIndexPrev->IsPrimary())
    {
        //nReward = pCoreProtocol->GetPrimaryMintWorkReward(pIndexPrev);
        nReward = GetBlockInvestTotalReward(hashPrev);
        if (nReward < 0)
        {
            Log("Get block reward: Get block invest total reward fail, hashPrev: %s", hashPrev.ToString().c_str());
            return false;
        }
    }
    else
    {
        CProfile profile;
        if (!GetForkProfile(pIndexPrev->GetOriginHash(), profile))
        {
            Log("Get block reward: Get fork profile fail, hashPrev: %s", hashPrev.ToString().c_str());
            return false;
        }
        if (profile.nHalveCycle == 0)
        {
            nReward = profile.nMintReward;
        }
        else
        {
            nReward = profile.nMintReward / pow(2, (pIndexPrev->GetBlockHeight() + 1 - profile.nJointHeight) / profile.nHalveCycle);
        }
    }
    return true;
}

bool CBlockChain::GetBlockLocator(const uint256& hashFork, CBlockLocator& locator, uint256& hashDepth, int nIncStep)
{
    return cntrBlock.GetForkBlockLocator(hashFork, locator, hashDepth, nIncStep);
}

bool CBlockChain::GetBlockInv(const uint256& hashFork, const CBlockLocator& locator, vector<uint256>& vBlockHash, size_t nMaxCount)
{
    return cntrBlock.GetForkBlockInv(hashFork, locator, vBlockHash, nMaxCount);
}

bool CBlockChain::ListForkUnspent(const uint256& hashFork, const CDestination& dest, uint32 nMax, std::vector<CTxUnspent>& vUnspent)
{
    return cntrBlock.ListForkUnspent(hashFork, dest, nMax, vUnspent);
}

bool CBlockChain::ListForkUnspentBatch(const uint256& hashFork, uint32 nMax, std::map<CDestination, std::vector<CTxUnspent>>& mapUnspent)
{
    return cntrBlock.ListForkUnspentBatch(hashFork, nMax, mapUnspent);
}

bool CBlockChain::GetVotes(const CDestination& destDelegate, int64& nVotes)
{
    return cntrBlock.GetVotes(pCoreProtocol->GetGenesisBlockHash(), destDelegate, nVotes);
}

bool CBlockChain::ListDelegatePayment(uint32 height, CBlock& block, std::multimap<int64, CDestination>& mapVotes)
{
    std::vector<uint256> vBlockHash;
    if (!GetBlockHash(pCoreProtocol->GetGenesisBlockHash(), height, vBlockHash) || vBlockHash.size() == 0)
    {
        return false;
    }
    cntrBlock.GetDelegatePaymentList(vBlockHash[0], mapVotes);
    if (!GetBlock(vBlockHash[0], block))
    {
        return false;
    }
    return true;
}

bool CBlockChain::ListDelegate(uint32 nCount, std::multimap<int64, std::pair<CDestination, std::string>>& mapVotes)
{
    return cntrBlock.GetDelegateList(pCoreProtocol->GetGenesisBlockHash(), nCount, mapVotes);
}

bool CBlockChain::VerifyRepeatBlock(const uint256& hashFork, const CBlock& block, const uint256& hashBlockRef)
{
    uint32 nRefTimeStamp = 0;
    if (hashBlockRef != 0 && (block.IsSubsidiary() || block.IsExtended()))
    {
        CBlockIndex* pIndexRef;
        if (!cntrBlock.RetrieveIndex(hashBlockRef, &pIndexRef))
        {
            StdLog("CBlockChain", "VerifyRepeatBlock: RetrieveIndex fail, hashBlockRef: %s, block: %s",
                   hashBlockRef.GetHex().c_str(), block.GetHash().GetHex().c_str());
            return false;
        }
        if (block.IsSubsidiary())
        {
            if (block.GetBlockTime() != pIndexRef->GetBlockTime())
            {
                StdLog("CBlockChain", "VerifyRepeatBlock: Subsidiary block time error, block time: %ld, ref block time: %ld, hashBlockRef: %s, block: %s",
                       block.GetBlockTime(), pIndexRef->GetBlockTime(), hashBlockRef.GetHex().c_str(), block.GetHash().GetHex().c_str());
                return false;
            }
        }
        else
        {
            if (block.GetBlockTime() <= pIndexRef->GetBlockTime()
                || block.GetBlockTime() >= pIndexRef->GetBlockTime() + BLOCK_TARGET_SPACING
                || ((block.GetBlockTime() - pIndexRef->GetBlockTime()) % EXTENDED_BLOCK_SPACING) != 0)
            {
                StdLog("CBlockChain", "VerifyRepeatBlock: Extended block time error, block time: %ld, ref block time: %ld, hashBlockRef: %s, block: %s",
                       block.GetBlockTime(), pIndexRef->GetBlockTime(), hashBlockRef.GetHex().c_str(), block.GetHash().GetHex().c_str());
                return false;
            }
        }
        nRefTimeStamp = pIndexRef->nTimeStamp;
    }
    return cntrBlock.VerifyRepeatBlock(hashFork, block.GetBlockHeight(), block.txMint.sendTo, block.nType, block.nTimeStamp, nRefTimeStamp, EXTENDED_BLOCK_SPACING);
}

bool CBlockChain::GetBlockDelegateVote(const uint256& hashBlock, map<CDestination, int64>& mapVote)
{
    return cntrBlock.GetBlockDelegateVote(hashBlock, mapVote);
}

int64 CBlockChain::GetDelegateMinEnrollAmount(const uint256& hashBlock)
{
    return pCoreProtocol->MinEnrollAmount();
}

bool CBlockChain::GetDelegateCertTxCount(const uint256& hashLastBlock, map<CDestination, int>& mapVoteCert)
{
    CBlockIndex* pLastIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashLastBlock, &pLastIndex))
    {
        StdLog("CBlockChain", "GetDelegateCertTxCount: RetrieveIndex fail, block: %s", hashLastBlock.GetHex().c_str());
        return false;
    }
    if (pLastIndex->GetBlockHeight() <= 0)
    {
        return true;
    }

    int nMinHeight = pLastIndex->GetBlockHeight() - CONSENSUS_ENROLL_INTERVAL + 2;
    if (nMinHeight < 1)
    {
        nMinHeight = 1;
    }

    CBlockIndex* pIndex = pLastIndex;
    for (int i = 0; i < CONSENSUS_ENROLL_INTERVAL - 1 && pIndex != nullptr; i++)
    {
        std::map<int, std::set<CDestination>> mapEnrollDest;
        if (cntrBlock.GetBlockDelegatedEnrollTx(pIndex->GetBlockHash(), mapEnrollDest))
        {
            for (const auto& t : mapEnrollDest)
            {
                if (t.first >= nMinHeight)
                {
                    for (const auto& m : t.second)
                    {
                        map<CDestination, int>::iterator it = mapVoteCert.find(m);
                        if (it == mapVoteCert.end())
                        {
                            mapVoteCert.insert(make_pair(m, 1));
                        }
                        else
                        {
                            it->second++;
                        }
                    }
                }
            }
        }
        pIndex = pIndex->pPrev;
    }

    int nMaxCertCount = CONSENSUS_ENROLL_INTERVAL * 4 / 3;
    if (nMaxCertCount > pLastIndex->GetBlockHeight())
    {
        nMaxCertCount = pLastIndex->GetBlockHeight();
    }
    for (auto& v : mapVoteCert)
    {
        v.second = nMaxCertCount - v.second;
        if (v.second < 0)
        {
            v.second = 0;
        }
    }
    return true;
}

bool CBlockChain::GetBlockDelegateEnrolled(const uint256& hashBlock, CDelegateEnrolled& enrolled)
{
    // Log("CBlockChain::GetBlockDelegateEnrolled enter .... height: %d, hashBlock: %s", CBlock::GetBlockHeightByHash(hashBlock), hashBlock.ToString().c_str());
    enrolled.Clear();

    if (cacheEnrolled.Retrieve(hashBlock, enrolled))
    {
        return true;
    }

    CBlockIndex* pIndex;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        Log("GetBlockDelegateEnrolled : Retrieve block Index Error: %s \n", hashBlock.ToString().c_str());
        return false;
    }
    int64 nMinEnrollAmount = pCoreProtocol->MinEnrollAmount();

    if (pIndex->GetBlockHeight() < CONSENSUS_ENROLL_INTERVAL)
    {
        return true;
    }
    vector<uint256> vBlockRange;
    for (int i = 0; i < CONSENSUS_ENROLL_INTERVAL; i++)
    {
        vBlockRange.push_back(pIndex->GetBlockHash());
        pIndex = pIndex->pPrev;
    }

    if (!cntrBlock.RetrieveAvailDelegate(hashBlock, pIndex->GetBlockHeight(), vBlockRange, nMinEnrollAmount,
                                         enrolled.mapWeight, enrolled.mapEnrollData, enrolled.vecAmount))
    {
        Log("GetBlockDelegateEnrolled : Retrieve Avail Delegate Error: %s \n", hashBlock.ToString().c_str());
        return false;
    }

    cacheEnrolled.AddNew(hashBlock, enrolled);

    return true;
}

bool CBlockChain::GetBlockDelegateAgreement(const uint256& hashBlock, CDelegateAgreement& agreement)
{
    agreement.Clear();

    if (cacheAgreement.Retrieve(hashBlock, agreement))
    {
        return true;
    }

    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex))
    {
        Log("GetBlockDelegateAgreement : Retrieve block Index Error: %s \n", hashBlock.ToString().c_str());
        return false;
    }

    // is not primary or PoW, return.
    if (!pIndex->IsPrimary() || pIndex->IsProofOfWork())
    {
        return true;
    }

    CBlockIndex* pIndexRef = pIndex;
    if (pIndex->GetBlockHeight() < CONSENSUS_INTERVAL)
    {
        return true;
    }

    CBlock block;
    if (!cntrBlock.Retrieve(pIndex, block))
    {
        Log("GetBlockDelegateAgreement : Retrieve block Error: %s \n", hashBlock.ToString().c_str());
        return false;
    }

    for (int i = 0; i < CONSENSUS_DISTRIBUTE_INTERVAL + 1; i++)
    {
        pIndex = pIndex->pPrev;
    }

    CDelegateEnrolled enrolled;
    if (!GetBlockDelegateEnrolled(pIndex->GetBlockHash(), enrolled))
    {
        Log("GetBlockDelegateAgreement : Get delegate enrolled fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    delegate::CDelegateVerify verifier(enrolled.mapWeight, enrolled.mapEnrollData);
    map<CDestination, size_t> mapBallot;
    if (!verifier.VerifyProof(block.vchProof, agreement.nAgreement, agreement.nWeight, mapBallot, true))
    {
        Log("GetBlockDelegateAgreement : Invalid block proof : %s \n", hashBlock.ToString().c_str());
        return false;
    }

    size_t nEnrollTrust = 0;
    pCoreProtocol->GetDelegatedBallot(agreement.nAgreement, agreement.nWeight, mapBallot, enrolled.vecAmount,
                                      pIndex->GetMoneySupply(), agreement.vBallot, nEnrollTrust, pIndexRef->GetBlockHeight());

    cacheAgreement.AddNew(hashBlock, agreement);

    return true;
}

int64 CBlockChain::GetBlockMoneySupply(const uint256& hashBlock)
{
    CBlockIndex* pIndex = nullptr;
    if (!cntrBlock.RetrieveIndex(hashBlock, &pIndex) || pIndex == nullptr)
    {
        return -1;
    }
    return pIndex->GetMoneySupply();
}

uint32 CBlockChain::DPoSTimestamp(const uint256& hashPrev)
{
    CBlockIndex* pIndexPrev = nullptr;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev) || pIndexPrev == nullptr)
    {
        return 0;
    }
    return pCoreProtocol->DPoSTimestamp(pIndexPrev);
}

Errno CBlockChain::VerifyPowBlock(const CBlock& block, bool& fLongChain)
{
    uint256 hash = block.GetHash();
    Errno err = OK;

    if (cntrBlock.Exists(hash))
    {
        Log("VerifyPowBlock Already Exists : %s ", hash.ToString().c_str());
        return ERR_ALREADY_HAVE;
    }

    err = pCoreProtocol->ValidateBlock(block);
    if (err != OK)
    {
        Log("VerifyPowBlock Validate Block Error(%s) : %s ", ErrorString(err), hash.ToString().c_str());
        return err;
    }

    CBlockIndex* pIndexPrev;
    if (!cntrBlock.RetrieveIndex(block.hashPrev, &pIndexPrev))
    {
        Log("VerifyPowBlock Retrieve Prev Index Error: %s ", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    int64 nReward = 0;
    CDelegateAgreement agreement;
    size_t nEnrollTrust = 0;
    CBlockIndex* pIndexRef = nullptr;
    err = VerifyBlock(hash, block, pIndexPrev, nReward, agreement, nEnrollTrust, &pIndexRef);
    if (err != OK)
    {
        Log("VerifyPowBlock Verify Block Error(%s) : %s ", ErrorString(err), hash.ToString().c_str());
        return err;
    }

    storage::CBlockView view;
    if (!cntrBlock.GetBlockView(block.hashPrev, view, !block.IsOrigin()))
    {
        Log("VerifyPowBlock Get Block View Error: %s ", block.hashPrev.ToString().c_str());
        return ERR_SYS_STORAGE_ERROR;
    }

    if (!block.IsVacant() || !block.txMint.sendTo.IsNull())
    {
        view.AddTx(block.txMint.GetHash(), block.txMint, block.GetBlockHeight(), CTxContxt());
    }

    CBlockEx blockex(block);
    vector<CTxContxt>& vTxContxt = blockex.vTxContxt;

    int64 nTotalFee = 0;
    if (block.vtx.size() > 0)
    {
        vTxContxt.reserve(block.vtx.size());
    }

    int64 nIgnoreVerifyTx = 0;
    if (block.IsPrimary())
    {
        if (!VerifyInvestRewardTx(block, nIgnoreVerifyTx))
        {
            Error("VerifyPowBlock verify invest reward tx fail, block: %s", hash.ToString().c_str());
            return ERR_TRANSACTION_INVALID;
        }
        if (!VerifyBlockInvestVoteTx(block))
        {
            Error("VerifyPowBlock verify invest vote tx fail, block: %s", hash.ToString().c_str());
            return ERR_TRANSACTION_INVALID;
        }
    }

    // get fork context
    CProfile profile;
    uint256 forkid = pIndexPrev->GetOriginHash();
    if (!GetForkProfile(forkid, profile))
    {
        Error("VerifyPowBlock get fork profile error, block: %s, fork: %s", hash.ToString().c_str(), forkid.ToString().c_str());
        return ERR_BLOCK_INVALID_FORK;
    }

    for (const CTransaction& tx : block.vtx)
    {
        uint256 txid = tx.GetHash();
        CTxContxt txContxt;
        err = GetTxContxt(view, tx, txContxt);
        if (err != OK)
        {
            Log("VerifyPowBlock Get txContxt Error([%d] %s) : %s ", err, ErrorString(err), txid.ToString().c_str());
            return err;
        }

        if (nIgnoreVerifyTx > 0)
        {
            nIgnoreVerifyTx--;
        }
        else
        {
            err = pCoreProtocol->VerifyBlockTx(tx, txContxt, pIndexPrev, block.GetBlockHeight(), pIndexPrev->GetOriginHash(), profile);
            if (err != OK)
            {
                Log("VerifyPowBlock Verify BlockTx Error(%s) : %s ", ErrorString(err), txid.ToString().c_str());
                return err;
            }
            if (tx.nTimeStamp > block.nTimeStamp)
            {
                Log("VerifyPowBlock Verify BlockTx time fail: tx time: %d, block time: %d, tx: %s, block: %s",
                    tx.nTimeStamp, block.nTimeStamp, txid.ToString().c_str(), hash.GetHex().c_str());
                return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
            }
        }

        vTxContxt.push_back(txContxt);
        if (!view.AddTx(txid, tx, block.GetBlockHeight(), txContxt))
        {
            Log("VerifyPowBlock Add block view tx error, txid: %s", txid.ToString().c_str());
            return ERR_BLOCK_TRANSACTIONS_INVALID;
        }

        StdTrace("BlockChain", "VerifyPowBlock: verify tx success, new tx: %s, new block: %s", txid.GetHex().c_str(), hash.GetHex().c_str());

        nTotalFee += tx.nTxFee;
    }

    if (block.txMint.nAmount > nTotalFee + nReward)
    {
        Log("VerifyPowBlock Mint tx amount invalid : (%ld > %ld + %ld)", block.txMint.nAmount, nTotalFee, nReward);
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }

    // Get block trust
    uint256 nNewBlockChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nNewBlockChainTrust, pIndexPrev, agreement, pIndexRef, nEnrollTrust))
    {
        Log("VerifyPowBlock get block trust fail, block: %s", hash.GetHex().c_str());
        return ERR_BLOCK_TRANSACTIONS_INVALID;
    }
    nNewBlockChainTrust += pIndexPrev->nChainTrust;

    CBlockIndex* pIndexFork = nullptr;
    if (cntrBlock.RetrieveFork(pIndexPrev->GetOriginHash(), &pIndexFork)
        && pIndexFork->nChainTrust > nNewBlockChainTrust)
    {
        Log("VerifyPowBlock : Short chain, new block height: %d, block: %s, fork chain trust: %s, fork last block: %s",
            block.GetBlockHeight(), hash.GetHex().c_str(), pIndexFork->nChainTrust.GetHex().c_str(), pIndexFork->GetBlockHash().GetHex().c_str());
        fLongChain = false;
    }
    else
    {
        fLongChain = true;
    }

    return OK;
}

bool CBlockChain::VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, vector<pair<CDestination, CForkContext>>& vForkCtxt)
{
#ifdef DISABLE_FORK_CREATETION
    Log("Verify block fork tx: Disable fork creation, tx: %s", tx.GetHash().ToString().c_str());
    return false;
#else
    if (tx.vchData.empty())
    {
        Log("Verify block fork tx: invalid vchData, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }

    CBlock block;
    CProfile profile;
    try
    {
        CBufStream ss;
        ss.Write((const char*)tx.vchData.data(), tx.vchData.size());
        ss >> block;
        if (!block.IsOrigin() || block.IsPrimary())
        {
            Log("Verify block fork tx: invalid block, tx: %s", tx.GetHash().ToString().c_str());
            return false;
        }
        if (!profile.Load(block.vchProof))
        {
            Log("Verify block fork tx: invalid profile, tx: %s", tx.GetHash().ToString().c_str());
            return false;
        }
    }
    catch (...)
    {
        Log("Verify block fork tx: invalid vchData, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }
    uint256 hashNewFork = block.GetHash();

    do
    {
        CForkContext ctxtParent;
        if (!cntrBlock.RetrieveForkContext(profile.hashParent, ctxtParent))
        {
            bool fFindParent = false;
            for (const auto& vd : vForkCtxt)
            {
                if (vd.second.hashFork == profile.hashParent)
                {
                    ctxtParent = vd.second;
                    fFindParent = true;
                    break;
                }
            }
            if (!fFindParent)
            {
                Log("Verify block fork tx: Retrieve parent context, tx: %s", tx.GetHash().ToString().c_str());
                break;
            }
        }

        CProfile forkProfile;
        Errno err = pCoreProtocol->ValidateOrigin(block, ctxtParent.GetProfile(), forkProfile);
        if (err != OK)
        {
            Log("Verify block fork tx: Validate origin Error(%s), tx: %s", ErrorString(err), tx.GetHash().ToString().c_str());
            break;
        }

        if (!pForkManager->VerifyFork(hashPrev, hashNewFork, profile.strName))
        {
            Log("Verify block fork tx: verify fork fail, tx: %s", tx.GetHash().ToString().c_str());
            break;
        }
        bool fCheckRet = true;
        for (const auto& vd : vForkCtxt)
        {
            if (vd.second.hashFork == hashNewFork || vd.second.strName == profile.strName)
            {
                Log("Verify block fork tx: fork existed or name repeated, tx: %s, new fork: %s, name: %s",
                    tx.GetHash().ToString().c_str(), hashNewFork.GetHex().c_str(), vd.second.strName.c_str());
                fCheckRet = false;
                break;
            }
        }
        if (!fCheckRet)
        {
            break;
        }

        vForkCtxt.push_back(make_pair(tx.sendTo, CForkContext(block.GetHash(), block.hashPrev, tx.GetHash(), profile)));
        Log("Verify block fork tx success: valid fork: %s, tx: %s", hashNewFork.GetHex().c_str(), tx.GetHash().ToString().c_str());
    } while (0);

    return true;
#endif
}

bool CBlockChain::CheckForkValidLast(const uint256& hashFork, CBlockChainUpdate& update)
{
    CBlockIndex* pValidLastIndex = cntrBlock.GetForkValidLast(pCoreProtocol->GetGenesisBlockHash(), hashFork /*, pCoreProtocol->GetRefVacantHeight()*/);
    if (pValidLastIndex == nullptr)
    {
        return false;
    }

    storage::CBlockView view;
    if (!cntrBlock.GetBlockView(pValidLastIndex->GetBlockHash(), view, true))
    {
        StdLog("BlockChain", "CheckForkValidLast: Get Block View Error, last block: %s, fork: %s",
               pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    if (!cntrBlock.CommitBlockView(view, pValidLastIndex))
    {
        StdLog("BlockChain", "CheckForkValidLast: Storage Commit BlockView Error, last block: %s, fork: %s",
               pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }

    StdLog("BlockChain", "CheckForkValidLast: Repair fork last success, last block: %s, fork: %s",
           pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());

    CProfile profile;
    if (!GetForkProfile(hashFork, profile))
    {
        Error("CheckForkValidLast: get fork profile error, last block: %s, fork: %s",
              pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    update = CBlockChainUpdate(pValidLastIndex, profile.nForkType);
    view.GetTxUpdated(update.setTxUpdate);
    view.GetBlockChanges(update.vBlockAddNew, update.vBlockRemove);

    if (update.IsNull())
    {
        StdLog("BlockChain", "CheckForkValidLast: update is null, last block: %s, fork: %s",
               pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
    }
    if (update.vBlockAddNew.empty())
    {
        StdLog("BlockChain", "CheckForkValidLast: vBlockAddNew is empty, last block: %s, fork: %s",
               pValidLastIndex->GetBlockHash().ToString().c_str(), hashFork.GetHex().c_str());
    }
    return true;
}

bool CBlockChain::VerifyForkRefLongChain(const uint256& hashFork, const uint256& hashForkBlock, const uint256& hashPrimaryBlock)
{
    uint256 hashRefBlock;
    bool fOrigin = false;
    if (!cntrBlock.GetLastRefBlockHash(hashFork, hashForkBlock, hashRefBlock, fOrigin))
    {
        StdLog("BlockChain", "VerifyForkRefLongChain: Get ref block fail, last block: %s, fork: %s",
               hashForkBlock.GetHex().c_str(), hashFork.GetHex().c_str());
        return false;
    }
    if (!fOrigin)
    {
        if (!cntrBlock.VerifySameChain(hashRefBlock, hashPrimaryBlock))
        {
            StdLog("BlockChain", "VerifyForkRefLongChain: Fork does not refer to long chain, fork last: %s, ref block: %s, primayr block: %s, fork: %s",
                   hashForkBlock.GetHex().c_str(), hashRefBlock.GetHex().c_str(),
                   hashPrimaryBlock.GetHex().c_str(), hashFork.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockChain::GetPrimaryHeightBlockTime(const uint256& hashLastBlock, int nHeight, uint256& hashBlock, int64& nTime)
{
    return cntrBlock.GetPrimaryHeightBlockTime(hashLastBlock, nHeight, hashBlock, nTime);
}

bool CBlockChain::IsVacantBlockBeforeCreatedForkHeight(const uint256& hashFork, const CBlock& block)
{
    int nCreatedHeight = pForkManager->GetForkCreatedHeight(hashFork);
    if (nCreatedHeight < 0)
    {
        return true;
    }

    int nOriginHeight = CBlock::GetBlockHeightByHash(hashFork);
    int nTargetHeight = block.GetBlockHeight();

    if (nTargetHeight < nCreatedHeight)
    {
        Log("Target Block Is Vacant %s at height %d in range of (%d, %d)", block.IsVacant() ? "true" : "false", nTargetHeight, nOriginHeight, nCreatedHeight);
        return block.IsVacant();
    }

    return true;
}

bool CBlockChain::CheckContainer()
{
    if (cntrBlock.IsEmpty())
    {
        return true;
    }
    if (!cntrBlock.Exists(pCoreProtocol->GetGenesisBlockHash()))
    {
        return false;
    }
    return cntrBlock.CheckConsistency(StorageConfig()->nCheckLevel, StorageConfig()->nCheckDepth);
}

bool CBlockChain::RebuildContainer()
{
    return false;
}

bool CBlockChain::InsertGenesisBlock(CBlock& block)
{
    uint256 nChainTrust;
    if (!pCoreProtocol->GetBlockTrust(block, nChainTrust))
    {
        return false;
    }
    return cntrBlock.Initiate(block.GetHash(), block, nChainTrust);
}

Errno CBlockChain::GetTxContxt(storage::CBlockView& view, const CTransaction& tx, CTxContxt& txContxt)
{
    txContxt.SetNull();
    for (const CTxIn& txin : tx.vInput)
    {
        CTxOut output;
        if (!view.RetrieveUnspent(txin.prevout, output))
        {
            Log("GetTxContxt: RetrieveUnspent fail, prevout: [%d]:%s", txin.prevout.n, txin.prevout.hash.GetHex().c_str());
            return ERR_MISSING_PREV;
        }
        if (txContxt.destIn.IsNull())
        {
            txContxt.destIn = output.destTo;
        }
        else if (txContxt.destIn != output.destTo)
        {
            Log("GetTxContxt: destIn error, destIn: %s, destTo: %s",
                txContxt.destIn.ToString().c_str(), output.destTo.ToString().c_str());
            return ERR_TRANSACTION_INVALID;
        }
        txContxt.vin.push_back(CTxInContxt(output));
    }
    return OK;
}

bool CBlockChain::GetBlockChanges(const CBlockIndex* pIndexNew, const CBlockIndex* pIndexFork,
                                  vector<CBlockEx>& vBlockAddNew, vector<CBlockEx>& vBlockRemove)
{
    while (pIndexNew != pIndexFork)
    {
        int64 nLastBlockTime = pIndexFork ? pIndexFork->GetBlockTime() : -1;
        if (pIndexNew->GetBlockTime() >= nLastBlockTime)
        {
            CBlockEx block;
            if (!cntrBlock.Retrieve(pIndexNew, block))
            {
                return false;
            }
            vBlockAddNew.push_back(block);
            pIndexNew = pIndexNew->pPrev;
        }
        else
        {
            CBlockEx block;
            if (!cntrBlock.Retrieve(pIndexFork, block))
            {
                return false;
            }
            vBlockRemove.push_back(block);
            pIndexFork = pIndexFork->pPrev;
        }
    }
    return true;
}

bool CBlockChain::GetBlockDelegateAgreement(const uint256& hashBlock, const CBlock& block, const CBlockIndex* pIndexPrev,
                                            CDelegateAgreement& agreement, size_t& nEnrollTrust)
{
    agreement.Clear();

    if (pIndexPrev->GetBlockHeight() < CONSENSUS_INTERVAL - 1)
    {
        return true;
    }

    const CBlockIndex* pIndex = pIndexPrev;
    for (int i = 0; i < CONSENSUS_DISTRIBUTE_INTERVAL; i++)
    {
        pIndex = pIndex->pPrev;
    }

    CDelegateEnrolled enrolled;
    if (!GetBlockDelegateEnrolled(pIndex->GetBlockHash(), enrolled))
    {
        Log("GetBlockDelegateAgreement : GetBlockDelegateEnrolled fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    delegate::CDelegateVerify verifier(enrolled.mapWeight, enrolled.mapEnrollData);
    map<CDestination, size_t> mapBallot;
    if (!verifier.VerifyProof(block.vchProof, agreement.nAgreement, agreement.nWeight, mapBallot, true))
    {
        Log("GetBlockDelegateAgreement : Invalid block proof : %s", hashBlock.ToString().c_str());
        return false;
    }

    pCoreProtocol->GetDelegatedBallot(agreement.nAgreement, agreement.nWeight, mapBallot, enrolled.vecAmount,
                                      pIndex->GetMoneySupply(), agreement.vBallot, nEnrollTrust, pIndexPrev->GetBlockHeight() + 1);

    cacheAgreement.AddNew(hashBlock, agreement);

    return true;
}

Errno CBlockChain::VerifyBlock(const uint256& hashBlock, const CBlock& block, CBlockIndex* pIndexPrev,
                               int64& nReward, CDelegateAgreement& agreement, size_t& nEnrollTrust, CBlockIndex** ppIndexRef)
{
    nReward = 0;
    if (block.IsOrigin())
    {
        Log("Verify block : Is origin, block: %s", hashBlock.GetHex().c_str());
        return ERR_BLOCK_INVALID_FORK;
    }

    if (block.IsPrimary())
    {
        if (!pIndexPrev->IsPrimary())
        {
            Log("Verify block : Prev block not is primary, prev: %s, block: %s",
                pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        if (!VerifyBlockCertTx(block))
        {
            Log("Verify block : Verify cert tx fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_CERTTX_OUT_OF_BOUND;
        }

        if (!GetBlockDelegateAgreement(hashBlock, block, pIndexPrev, agreement, nEnrollTrust))
        {
            Log("Verify block : Get agreement fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!GetBlockMintReward(block.hashPrev, nReward))
        {
            Log("Verify block : Get mint reward fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_COINBASE_INVALID;
        }

        /*if (!pCoreProtocol->IsDposHeight(pIndexPrev->GetBlockHeight() + 1))
        {
            if (!agreement.IsProofOfWork())
            {
                Log("Verify block : POW stage not is pow block, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            return pCoreProtocol->VerifyProofOfWork(block, pIndexPrev);
        }
        else
        {*/
        if (agreement.IsProofOfWork())
        {
            return pCoreProtocol->VerifyProofOfWork(block, pIndexPrev);
        }
        else
        {
            return pCoreProtocol->VerifyDelegatedProofOfStake(block, pIndexPrev, agreement);
        }
        //}
    }
    else if (block.IsSubsidiary() || block.IsExtended())
    {
        if (pIndexPrev->IsPrimary())
        {
            Log("Verify block : SubFork prev not is primary, prev: %s, block: %s",
                pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        CProofOfPiggyback proof;
        if (!proof.Load(block.vchProof) || proof.hashRefBlock == 0)
        {
            Log("Verify block : SubFork load proof fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_INVALID_FORK;
        }

        CDelegateAgreement agreement;
        if (!GetBlockDelegateAgreement(proof.hashRefBlock, agreement))
        {
            Log("Verify block : SubFork get agreement fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (agreement.nAgreement != proof.nAgreement || agreement.nWeight != proof.nWeight
            || agreement.IsProofOfWork())
        {
            Log("Verify block : SubFork agreement error, ref agreement: %s, block agreement: %s, ref weight: %d, block weight: %d, type: %s, block: %s",
                agreement.nAgreement.GetHex().c_str(), proof.nAgreement.GetHex().c_str(),
                agreement.nWeight, proof.nWeight, (agreement.IsProofOfWork() ? "pow" : "dpos"),
                hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!cntrBlock.RetrieveIndex(proof.hashRefBlock, ppIndexRef) || *ppIndexRef == nullptr || !(*ppIndexRef)->IsPrimary())
        {
            Log("Verify block : SubFork retrieve ref index fail, ref block: %s, block: %s",
                proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        CProofOfPiggyback proofPrev;
        if (!pIndexPrev->IsOrigin() /*&& (!pIndexPrev->IsVacant() || pCoreProtocol->IsRefVacantHeight(pIndexPrev->GetBlockHeight()))*/)
        {
            CBlock blockPrev;
            if (!cntrBlock.Retrieve(pIndexPrev, blockPrev))
            {
                Log("Verify block : SubFork retrieve prev index fail, block: %s", hashBlock.GetHex().c_str());
                return ERR_MISSING_PREV;
            }
            if (!proofPrev.Load(blockPrev.vchProof) || proofPrev.hashRefBlock == 0)
            {
                Log("Verify block : SubFork load prev proof fail, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            if (proof.hashRefBlock != proofPrev.hashRefBlock
                && !cntrBlock.VerifySameChain(proofPrev.hashRefBlock, proof.hashRefBlock))
            {
                Log("Verify block : SubFork verify same chain fail, prev ref: %s, block ref: %s, block: %s",
                    proofPrev.hashRefBlock.GetHex().c_str(), proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
        }

        if (block.IsExtended())
        {
            if (pIndexPrev->IsOrigin() || pIndexPrev->IsVacant())
            {
                Log("Verify block : SubFork extended prev is origin or vacant, prev: %s, block: %s",
                    pIndexPrev->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_MISSING_PREV;
            }
            if (proof.nAgreement != proofPrev.nAgreement || proof.nWeight != proofPrev.nWeight)
            {
                Log("Verify block : SubFork extended agreement error, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            nReward = 0;
        }
        else
        {
            if (!GetBlockMintReward(block.hashPrev, nReward))
            {
                Log("Verify block : SubFork get mint reward error, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
        }

        return pCoreProtocol->VerifySubsidiary(block, pIndexPrev, *ppIndexRef, agreement);
    }
    else if (block.IsVacant())
    {
        /*if (!pCoreProtocol->IsRefVacantHeight(block.GetBlockHeight()))
        {
            if (block.GetBlockTime() < pIndexPrev->GetBlockTime())
            {
                Log("Verify block : Vacant time error, block time: %d, prev time: %d, block: %s",
                    block.GetBlockTime(), pIndexPrev->GetBlockTime(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
            }
        }
        else
        {*/
        CProofOfPiggyback proof;
        if (!proof.Load(block.vchProof) || proof.hashRefBlock == 0)
        {
            Log("Verify block : Vacant load proof error, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        CDelegateAgreement agreement;
        if (!GetBlockDelegateAgreement(proof.hashRefBlock, agreement))
        {
            Log("Verify block : Vacant get agreement fail, block: %s", hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (agreement.nAgreement != proof.nAgreement || agreement.nWeight != proof.nWeight
            || agreement.IsProofOfWork())
        {
            Log("Verify block : Vacant agreement error, ref agreement: %s, block agreement: %s, ref weight: %d, block weight: %d, type: %s, block: %s",
                agreement.nAgreement.GetHex().c_str(), proof.nAgreement.GetHex().c_str(),
                agreement.nWeight, proof.nWeight, (agreement.IsProofOfWork() ? "pow" : "dpos"),
                hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (block.txMint.sendTo != agreement.GetBallot(0))
        {
            Log("Verify block : Vacant sendTo error, sendTo: %s, ballot: %s, block: %s",
                CAddress(block.txMint.sendTo).ToString().c_str(),
                CAddress(agreement.GetBallot(0)).ToString().c_str(),
                hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (block.txMint.nTimeStamp != block.GetBlockTime())
        {
            Log("Verify block : Vacant txMint timestamp error, mint tx time: %d, block time: %d, block: %s",
                block.txMint.nTimeStamp, block.GetBlockTime(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!cntrBlock.RetrieveIndex(proof.hashRefBlock, ppIndexRef) || *ppIndexRef == nullptr || !(*ppIndexRef)->IsPrimary())
        {
            Log("Verify block : Vacant retrieve ref index fail, ref: %s, block: %s",
                proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
            return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
        }

        if (!pIndexPrev->IsOrigin() /*&& (!pIndexPrev->IsVacant() || pCoreProtocol->IsRefVacantHeight(pIndexPrev->GetBlockHeight()))*/)
        {
            CBlock blockPrev;
            if (!cntrBlock.Retrieve(pIndexPrev, blockPrev))
            {
                Log("Verify block : Vacant retrieve prev index fail, block: %s", hashBlock.GetHex().c_str());
                return ERR_MISSING_PREV;
            }
            CProofOfPiggyback proofPrev;
            if (!proofPrev.Load(blockPrev.vchProof) || proofPrev.hashRefBlock == 0)
            {
                Log("Verify block : Vacant load prev proof fail, block: %s", hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            if (proof.hashRefBlock != proofPrev.hashRefBlock
                && !cntrBlock.VerifySameChain(proofPrev.hashRefBlock, proof.hashRefBlock))
            {
                Log("Verify block : Vacant verify same chain fail, prev ref: %s, block ref: %s, block: %s",
                    proofPrev.hashRefBlock.GetHex().c_str(), proof.hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
        }

        if (!cntrBlock.VerifyPrimaryHeightRefBlockTime(block.GetBlockHeight(), block.GetBlockTime()))
        {
            uint256 hashPrimaryBlock;
            int64 nPrimaryTime = 0;
            if (!cntrBlock.GetPrimaryHeightBlockTime((*ppIndexRef)->GetBlockHash(), block.GetBlockHeight(), hashPrimaryBlock, nPrimaryTime))
            {
                Log("Verify block : Vacant get height time, block ref: %s, block: %s",
                    (*ppIndexRef)->GetBlockHash().GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
            }
            if (block.GetBlockTime() != nPrimaryTime)
            {
                Log("Verify block : Vacant time error, block time: %d, primary time: %d, ref block: %s, same height block: %s, block: %s",
                    block.GetBlockTime(), nPrimaryTime, (*ppIndexRef)->GetBlockHash().GetHex().c_str(),
                    hashPrimaryBlock.GetHex().c_str(), hashBlock.GetHex().c_str());
                return ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE;
            }
        }
        //}
    }
    else
    {
        Log("Verify block : block type error, nType: %d, block: %s", block.nType, hashBlock.GetHex().c_str());
        return ERR_BLOCK_PROOF_OF_STAKE_INVALID;
    }

    return OK;
}

bool CBlockChain::VerifyBlockCertTx(const CBlock& block)
{
    map<CDestination, int> mapBlockCert;
    for (const auto& d : block.vtx)
    {
        if (d.nType == CTransaction::TX_CERT)
        {
            ++mapBlockCert[d.sendTo];
        }
    }
    if (!mapBlockCert.empty())
    {
        map<CDestination, int64> mapVote;
        if (!GetBlockDelegateVote(block.hashPrev, mapVote))
        {
            StdError("CBlockChain", "VerifyBlockCertTx: GetBlockDelegateVote fail");
            return false;
        }
        map<CDestination, int> mapVoteCert;
        if (!GetDelegateCertTxCount(block.hashPrev, mapVoteCert))
        {
            StdError("CBlockChain", "VerifyBlockCertTx: GetBlockDelegateVote fail");
            return false;
        }
        int64 nMinAmount = pCoreProtocol->MinEnrollAmount();
        for (const auto& d : mapBlockCert)
        {
            const CDestination& dest = d.first;
            map<CDestination, int64>::iterator mt = mapVote.find(dest);
            if (mt == mapVote.end() || mt->second < nMinAmount)
            {
                StdLog("CBlockChain", "VerifyBlockCertTx: not enough votes, votes: %ld, dest: %s",
                       (mt == mapVote.end() ? 0 : mt->second), CAddress(dest).ToString().c_str());
                return false;
            }
            map<CDestination, int>::iterator it = mapVoteCert.find(dest);
            if (it != mapVoteCert.end() && d.second > it->second)
            {
                StdLog("CBlockChain", "VerifyBlockCertTx: more than votes, block cert count: %d, available cert count: %d, dest: %s",
                       d.second, it->second, CAddress(dest).ToString().c_str());
                return false;
            }
        }
    }
    return true;
}

void CBlockChain::InitCheckPoints(const uint256& hashFork, const map<int, uint256>& mapCheckPointsIn)
{
    MapCheckPointsType& mapCheckPoints = mapForkCheckPoints[hashFork];
    for (const auto& vd : mapCheckPointsIn)
    {
        mapCheckPoints.insert(std::make_pair(vd.first, CCheckPoint(vd.first, vd.second)));
    }
}

void CBlockChain::InitCheckPoints()
{
    if (Config()->nMagicNum == MAINNET_MAGICNUM)
    {
        for (const auto& vd : mapCheckPointsList)
        {
            InitCheckPoints(vd.first, vd.second);
        }
    }
}

bool CBlockChain::AddBlockForkContext(const CBlockEx& blockex)
{
    uint256 hashBlock = blockex.GetHash();
    vector<pair<CDestination, CForkContext>> vForkCtxt;
    for (size_t i = 0; i < blockex.vtx.size(); i++)
    {
        const CTransaction& tx = blockex.vtx[i];
        const CTxContxt& txContxt = blockex.vTxContxt[i];
        if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_FORK)
        {
            if (!VerifyBlockForkTx(blockex.hashPrev, tx, vForkCtxt))
            {
                StdLog("CBlockChain", "Add block fork context: Verify block fork tx fail, block: %s", hashBlock.ToString().c_str());
            }
        }
        if (txContxt.destIn.IsTemplate() && txContxt.destIn.GetTemplateId().GetType() == TEMPLATE_FORK)
        {
            auto it = vForkCtxt.begin();
            while (it != vForkCtxt.end())
            {
                if (it->first == txContxt.destIn)
                {
                    StdLog("CBlockChain", "Add block fork context: cancel fork, block: %s, fork: %s, dest: %s",
                           hashBlock.ToString().c_str(), it->second.hashFork.ToString().c_str(),
                           CAddress(txContxt.destIn).ToString().c_str());
                    vForkCtxt.erase(it++);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    for (const auto& vd : vForkCtxt)
    {
        if (!cntrBlock.AddNewForkContext(vd.second))
        {
            StdLog("CBlockChain", "Add block fork context: AddNewForkContext fail, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }

    bool fCheckPointBlock = false;
    const auto mt = mapForkCheckPoints.find(pCoreProtocol->GetGenesisBlockHash());
    if (mt != mapForkCheckPoints.end())
    {
        const auto it = mt->second.find(CBlock::GetBlockHeightByHash(hashBlock));
        if (it != mt->second.end() && it->second.nBlockHash == hashBlock)
        {
            fCheckPointBlock = true;
        }
    }

    uint256 hashRefFdBlock;
    map<uint256, int> mapValidFork;
    if (!pForkManager->AddValidForkContext(blockex.hashPrev, hashBlock, vForkCtxt, fCheckPointBlock, hashRefFdBlock, mapValidFork))
    {
        StdLog("CBlockChain", "Add block fork context: Add valid fork context fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    if (!cntrBlock.AddValidForkHash(hashBlock, hashRefFdBlock, mapValidFork))
    {
        StdLog("CBlockChain", "Add block fork context: AddValidForkHash fail, block: %s", hashBlock.ToString().c_str());
        pForkManager->RemoveValidForkContext(hashBlock);
        return false;
    }
    return true;
}

bool CBlockChain::HasCheckPoints(const uint256& hashFork) const
{
    auto iter = mapForkCheckPoints.find(hashFork);
    if (iter != mapForkCheckPoints.end())
    {
        return iter->second.size() > 0;
    }
    else
    {
        return false;
    }
}

bool CBlockChain::GetCheckPointByHeight(const uint256& hashFork, int nHeight, CCheckPoint& point)
{
    auto iter = mapForkCheckPoints.find(hashFork);
    if (iter != mapForkCheckPoints.end())
    {
        if (iter->second.count(nHeight) == 0)
        {
            return false;
        }
        else
        {
            point = iter->second[nHeight];
            return true;
        }
    }
    else
    {
        return false;
    }
}

std::vector<IBlockChain::CCheckPoint> CBlockChain::CheckPoints(const uint256& hashFork) const
{
    auto iter = mapForkCheckPoints.find(hashFork);
    if (iter != mapForkCheckPoints.end())
    {
        std::vector<IBlockChain::CCheckPoint> points;
        for (const auto& kv : iter->second)
        {
            points.push_back(kv.second);
        }

        return points;
    }

    return std::vector<IBlockChain::CCheckPoint>();
}

IBlockChain::CCheckPoint CBlockChain::LatestCheckPoint(const uint256& hashFork) const
{
    if (!HasCheckPoints(hashFork))
    {
        return IBlockChain::CCheckPoint();
    }
    return mapForkCheckPoints.at(hashFork).rbegin()->second;
}

IBlockChain::CCheckPoint CBlockChain::UpperBoundCheckPoint(const uint256& hashFork, int nHeight) const
{
    if (!HasCheckPoints(hashFork))
    {
        return IBlockChain::CCheckPoint();
    }

    auto& forkCheckPoints = mapForkCheckPoints.at(hashFork);
    auto iter = forkCheckPoints.upper_bound(nHeight);
    return (iter != forkCheckPoints.end()) ? IBlockChain::CCheckPoint(iter->second) : IBlockChain::CCheckPoint();
}

bool CBlockChain::VerifyCheckPoint(const uint256& hashFork, int nHeight, const uint256& nBlockHash)
{
    if (!HasCheckPoints(hashFork))
    {
        return true;
    }

    CCheckPoint point;
    if (!GetCheckPointByHeight(hashFork, nHeight, point))
    {
        return true;
    }

    if (nBlockHash != point.nBlockHash)
    {
        return false;
    }

    Log("HashFork %s Verified checkpoint at height %d/block %s", hashFork.ToString().c_str(), point.nHeight, point.nBlockHash.ToString().c_str());

    return true;
}

bool CBlockChain::FindPreviousCheckPointBlock(const uint256& hashFork, CBlock& block)
{
    if (!HasCheckPoints(hashFork))
    {
        return true;
    }

    const auto& points = CheckPoints(hashFork);
    int numCheckpoints = points.size();
    for (int i = numCheckpoints - 1; i >= 0; i--)
    {
        const CCheckPoint& point = points[i];

        uint256 hashBlock;
        if (!GetBlockHash(hashFork, point.nHeight, hashBlock))
        {
            StdTrace("BlockChain", "HashFork %s CheckPoint(%d, %s) doest not exists and continuely try to get previous checkpoint",
                     hashFork.ToString().c_str(), point.nHeight, point.nBlockHash.ToString().c_str());

            continue;
        }

        if (hashBlock != point.nBlockHash)
        {
            StdError("BlockChain", "CheckPoint(%d, %s)  does not match block hash %s",
                     point.nHeight, point.nBlockHash.ToString().c_str(), hashBlock.ToString().c_str());
            return false;
        }

        return GetBlock(hashBlock, block);
    }

    return true;
}

bool CBlockChain::IsSameBranch(const uint256& hashFork, const CBlock& block)
{
    uint256 bestChainBlockHash;
    if (!GetBlockHash(hashFork, block.GetBlockHeight(), bestChainBlockHash))
    {
        return true;
    }

    return block.GetHash() == bestChainBlockHash;
}

list<CDeFiReward> CBlockChain::GetDeFiReward(const uint256& forkid, const uint256& hashPrev, const int32 nHeight, const int32 nMax)
{
    list<CDeFiReward> listReward;
    if (!defiReward.ExistFork(forkid) || !defiReward.IsMinted(forkid, nHeight))
    {
        return listReward;
    }

    CBlockIndex* pIndexPrev = nullptr;
    if (!cntrBlock.RetrieveIndex(hashPrev, &pIndexPrev))
    {
        Error("GetDeFiReward retrieve prev block index fail: %s", hashPrev.ToString().c_str());
        return listReward;
    }

    uint256 nLastSection;
    CDeFiReward lastReward;
    list<uint256> listSection = GetDeFiSectionList(forkid, pIndexPrev, nHeight, nLastSection, lastReward);

    for (const uint256& section : listSection)
    {
        bool fIsNull;
        CDeFiRewardSet& s = defiReward.GetForkSection(forkid, section, fIsNull);

        // generate section reward
        if (fIsNull)
        {
            CProfile profile = defiReward.GetForkProfile(forkid);
            CDeFiRewardSet st = ComputeDeFiSection(forkid, section, profile);
            defiReward.AddForkSection(forkid, section, std::move(st));
            s = defiReward.GetForkSection(forkid, section, fIsNull);
        }

        const CDeFiRewardSetByReward& idxByReward = s.get<1>();
        CDeFiRewardSetByReward::iterator it = idxByReward.begin();
        if (section == nLastSection)
        {
            auto itLower = idxByReward.lower_bound(lastReward.nReward);
            auto itUpper = idxByReward.upper_bound(lastReward.nReward);
            for (it = itLower; it != itUpper; ++it)
            {
                if (it->dest == lastReward.dest)
                {
                    ++it;
                    break;
                }
            }
        }

        for (; it != idxByReward.end() && (nMax < 0 || listReward.size() < nMax); ++it)
        {
            listReward.push_back(*it);
        }

        if (listReward.size() >= nMax)
        {
            break;
        }
    }

    return listReward;
}

list<uint256> CBlockChain::GetDeFiSectionList(const uint256& forkid, const CBlockIndex* pIndexPrev, const int32 nHeight, uint256& nLastSection, CDeFiReward& lastReward)
{
    list<uint256> listSection;

    // find the last non-vacant block
    int32 prevHeight = defiReward.PrevRewardHeight(forkid, nHeight);
    if (prevHeight <= 0)
    {
        return listSection;
    }

    if (pIndexPrev->GetBlockHeight() == prevHeight)
    {
        listSection.push_front(pIndexPrev->GetBlockHash());
        prevHeight = defiReward.PrevRewardHeight(forkid, pIndexPrev->GetBlockHeight());
    }

    // find all prev vacant
    const CBlockIndex* pIndexLast = pIndexPrev;
    while (pIndexLast->IsVacant())
    {
        if (pIndexLast->GetBlockHeight() == prevHeight + 1)
        {
            listSection.push_front(pIndexLast->pPrev->GetBlockHash());
            prevHeight = defiReward.PrevRewardHeight(forkid, prevHeight);
        }
        pIndexLast = pIndexLast->pPrev;
    }

    // find the last section and last destination
    if (!pIndexLast->IsOrigin())
    {
        CBlockEx block;
        cntrBlock.Retrieve(pIndexLast, block);
        const CTransaction& lastTx = block.vtx.back();
        if (!block.vtx.empty() && lastTx.nType == CTransaction::TX_DEFI_REWARD)
        {
            nLastSection = lastTx.hashAnchor;
            lastReward.dest = lastTx.sendTo;
            lastReward.nReward = lastTx.nAmount + lastTx.nTxFee;

            CBlockIndex* pIndexLastSection = nullptr;
            if (!cntrBlock.RetrieveIndex(nLastSection, &pIndexLastSection))
            {
                Error("GetDeFiSectionList retrieve last section block index fail: %s", nLastSection.ToString().c_str());
                return listSection;
            }

            while (pIndexLast != pIndexLastSection->pPrev)
            {
                if (pIndexLast->GetBlockHeight() == prevHeight)
                {
                    listSection.push_front(pIndexLast->GetBlockHash());
                    prevHeight = defiReward.PrevRewardHeight(forkid, pIndexLast->GetBlockHeight());
                }
                pIndexLast = pIndexLast->pPrev;
            }
        }
    }

    CProfile profile = defiReward.GetForkProfile(forkid);
    for (auto it = listSection.begin(); it != listSection.end(); it++)
    {
        const uint256& section = *it;
        if (!defiReward.ExistForkSection(forkid, section))
        {
            // check max supply
            CBlockIndex* pIndexSection = nullptr;
            if (!cntrBlock.RetrieveIndex(section, &pIndexSection))
            {
                Error("GetDeFiSectionList retrieve section index error, section: %s", section.ToString().c_str());
            }
            if (!cntrBlock.RetrieveIndex(section, &pIndexSection) || (profile.defi.nMaxSupply >= 0 && pIndexSection->GetMoneySupply() >= profile.defi.nMaxSupply))
            {
                listSection.erase(it, listSection.end());
                break;
            }
        }
    }

    return listSection;
}

CDeFiRewardSet CBlockChain::ComputeDeFiSection(const uint256& forkid, const uint256& hash, const CProfile& profile)
{
    CDeFiRewardSet s;

    int32 nHeight = CBlock::GetBlockHeightByHash(hash);

    storage::CBlockView view;
    if (!cntrBlock.GetBlockView(hash, view))
    {
        Error("ComputeDeFiSection get block view error, fork: %s, hash: %s", forkid.ToString().c_str(), hash.ToString().c_str());
        return s;
    }

    map<CDestination, int64> mapAddressAmount;
    if (!cntrBlock.ListForkAllAddressAmount(forkid, view, mapAddressAmount) || mapAddressAmount.empty())
    {
        Error("ComputeDeFiSection ListForkAllAddressAmount error, fork: %s, hash: %s", forkid.ToString().c_str(), hash.ToString().c_str());
        return s;
    }

    int64 nSupply = 0;
    for (auto& addrAmount : mapAddressAmount)
    {
        nSupply += addrAmount.second;
    }

    int64 nInvalidSupply = 0;
    // unspendable address
    if (nHeight > NO_DEFI_TEMPLATE_ADDRESS_HEIGHT)
    {
        auto iter = mapAddressAmount.begin();
        for (; iter != mapAddressAmount.end();)
        {
            const CDestination& destTo = iter->first;
            if (!CTemplate::IsTxSpendable(destTo))
            {
                nInvalidSupply += iter->second;
                mapAddressAmount.erase(iter++);
            }
            else
            {
                ++iter;
            }
        }
    }

    // blacklist
    const set<CDestination> setBlacklist = pCoreProtocol->GetDeFiBlacklist(forkid, nHeight);
    for (auto& dest : setBlacklist)
    {
        auto iter = mapAddressAmount.find(dest);
        if (iter != mapAddressAmount.end())
        {
            nInvalidSupply += iter->second;
            mapAddressAmount.erase(iter);
        }
    }

    int64 nReward = 0;
    //if (pCoreProtocol->IsNewDeFiRewardHeight(nHeight))
    //{
    nReward = defiReward.GetSectionReward(forkid, hash, nSupply, nInvalidSupply);
    //}
    //else
    //{
    //    nReward = defiReward.GetSectionReward(forkid, hash);
    //}
    if (nReward <= 0)
    {
        return s;
    }

    int64 nStakeReward = nReward * profile.defi.nStakeRewardPercent / 100;
    CDeFiRewardSet stakeReward = defiReward.ComputeStakeReward(profile.defi.nStakeMinToken, nStakeReward, mapAddressAmount);

    // get invitation relation
    CDeFiRelationGraph relation;
    if (!cntrBlock.ListDeFiRelation(forkid, view, relation, [](const CTransaction& tx, const CDestination& parentIn)
                                    { return CDeFiRelationRewardNode(parentIn); }))
    {
        Error("ComputeDeFiSection ListDeFiRelation error, fork: %s, hash: %s", forkid.ToString().c_str(), hash.ToString().c_str());
        return s;
    }

    int64 nPromotionReward = nReward * profile.defi.nPromotionRewardPercent / 100;
    CDeFiRewardSet promotionReward = defiReward.ComputePromotionReward(nPromotionReward, mapAddressAmount, profile.defi.mapPromotionTokenTimes, relation, setBlacklist);

    CDeFiRewardSetByDest& destIdx = promotionReward.get<0>();
    for (auto& stake : stakeReward)
    {
        CDeFiReward reward = stake;
        auto it = destIdx.find(stake.dest);
        if (it != destIdx.end())
        {
            reward.nAchievement = it->nAchievement;
            reward.nPower = it->nPower;
            reward.nPromotionReward = it->nPromotionReward;
            reward.nReward += it->nPromotionReward;
            destIdx.erase(it);
        }

        // check reward > txfee
        if (reward.nReward > MIN_TX_FEE)
        {
            reward.hashAnchor = hash;
            s.insert(move(reward));
        }
    }

    for (auto& promotion : promotionReward)
    {
        // check reward > txfee
        if (promotion.nReward > MIN_TX_FEE)
        {
            CDeFiReward reward = promotion;
            reward.hashAnchor = hash;
            s.insert(move(reward));
        }
    }

    return s;
}

bool CBlockChain::GetDeFiRelation(const uint256& hashFork, const CDestination& destIn, CDestination& parent)
{
    storage::CAddrInfo addrInfo;
    if (cntrBlock.GetDeFiRelation(hashFork, destIn, addrInfo))
    {
        parent = addrInfo.destParent;
        return true;
    }

    return false;
}

bool CBlockChain::ListDeFiRelation(const uint256& hashFork, xengine::CForest<CDestination, CDestination>& relation)
{
    return cntrBlock.ListDeFiRelation(hashFork, storage::CBlockView(), relation, [](const CTransaction& tx, const CDestination& parentIn)
                                      { return parentIn; });
}

bool CBlockChain::InitDeFiRelation(const uint256& hashFork)
{
    return cntrBlock.InitDeFiRelation(hashFork);
}

bool CBlockChain::CheckAddDeFiRelation(const uint256& hashFork, const CDestination& dest, const CDestination& parent)
{
    return cntrBlock.CheckAddDeFiRelation(hashFork, dest, parent);
}

bool CBlockChain::GetAddressUnspent(const uint256& hashFork, const CDestination& dest, map<CTxOutPoint, CUnspentOut>& mapUnspent, uint256& hashLastBlockOut)
{
    return cntrBlock.RetrieveAddressUnspent(hashFork, dest, mapUnspent, hashLastBlockOut);
}

int64 CBlockChain::GetAddressTxList(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, vector<CTxInfo>& vTx)
{
    return cntrBlock.RetrieveAddressTxList(hashFork, dest, nPrevHeight, nPrevTxSeq, nOffset, nCount, vTx);
}

bool CBlockChain::CalcBlockInvestRewardTx(const uint256& hashPrev, const uint32 nBlockTime, vector<CTransaction>& vInvestRewardTx)
{
    CInvestContext ctxtInvest;
    if (!cntrBlock.RetrieveInvestContext(hashPrev, ctxtInvest))
    {
        StdLog("BlockChain", "Calculate block invest reward: Retrieve invest context fail, prev block: %s", hashPrev.GetHex().c_str());
        return false;
    }

    int nPrevHeight = CBlock::GetBlockHeightByHash(hashPrev);
    for (const auto& vd : ctxtInvest.vNoPackingReward)
    {
        CTransaction tx;
        tx.hashAnchor = pCoreProtocol->GetGenesisBlockHash();
        tx.nType = CTransaction::TX_INVEST_REWARD;
        tx.nTimeStamp = nBlockTime;
        tx.sendTo = vd.first;
        tx.nAmount = vd.second * USER_REWARD_RATIO / MINT_REWARD_PER;

        xengine::CODataStream ds(tx.vchData);
        ds << (uint32)(vInvestRewardTx.size());

        //StdTrace("BlockChain", "Invest reward tx<p>: height: %d, amount: %f, dest: %s, tx: %s",
        //         nPrevHeight + 1, ValueFromToken(tx.nAmount), CAddress(tx.sendTo).ToString().c_str(), tx.GetHash().GetHex().c_str());

        vInvestRewardTx.push_back(tx);
        if (vInvestRewardTx.size() >= nMaxBlockRewardTxCount)
        {
            break;
        }
    }

    if (vInvestRewardTx.size() < nMaxBlockRewardTxCount)
    {
        for (const auto& kv : ctxtInvest.mapInvest)
        {
            if ((nPrevHeight - kv.second.nStartHeight) == (kv.second.nCycle * INVEST_DAY_HEIGHT))
            {
                CTransaction tx;
                tx.hashAnchor = pCoreProtocol->GetGenesisBlockHash();
                tx.nType = CTransaction::TX_INVEST_REWARD;
                tx.nTimeStamp = nBlockTime;
                tx.sendTo = kv.second.destReward;
                int64 nRewardAmount = CalcInvestReward(kv.second.nCycle, kv.second.nInvestAmount);
                if (nRewardAmount < 0)
                {
                    StdError("BlockChain", "Calculate block invest reward: Calc invest reward fail, prev block: %s", hashPrev.GetHex().c_str());
                    return false;
                }
                tx.nAmount = nRewardAmount * USER_REWARD_RATIO / MINT_REWARD_PER;

                xengine::CODataStream ds(tx.vchData);
                ds << (uint32)(vInvestRewardTx.size());

                //StdTrace("BlockChain", "Invest reward tx: height: %d, amount: %f, dest: %s, tx: %s",
                //         nPrevHeight + 1, ValueFromToken(tx.nAmount), CAddress(tx.sendTo).ToString().c_str(), tx.GetHash().GetHex().c_str());

                vInvestRewardTx.push_back(tx);
                if (vInvestRewardTx.size() >= nMaxBlockRewardTxCount)
                {
                    break;
                }
            }
        }
    }
    return true;
}

bool CBlockChain::CheckDestInvestAmount(const uint256& hashPrev, const CDestination& dest, const bool fCheckTxPool)
{
    CInvestContext ctxtInvest;
    if (!cntrBlock.RetrieveInvestContext(hashPrev, ctxtInvest))
    {
        StdLog("BlockChain", "Check dest invest amount: Retrieve invest context fail, prev block: %s", hashPrev.GetHex().c_str());
        return false;
    }
    auto it = ctxtInvest.mapInvest.find(dest);
    if (it != ctxtInvest.mapInvest.end())
    {
        if (it->second.nInvestAmount > 0)
        {
            StdLog("BlockChain", "Check dest invest amount: There is data on the chain, dest: %s, prev block: %s",
                   CAddress(dest).ToString().c_str(), hashPrev.GetHex().c_str());
            return false;
        }
    }
    if (fCheckTxPool)
    {
        int nTxPoolCount = pTxPool->GetDestTxpoolTxCount(dest);
        if (nTxPoolCount > 0)
        {
            StdLog("BlockChain", "Check dest invest amount: Txpool has data, tx pool tx count: %d, dest: %s, prev block: %s",
                   nTxPoolCount, CAddress(dest).ToString().c_str(), hashPrev.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockChain::VerifyDestInvestRedeem(const uint256& hashPrev, const CDestination& dest, const bool fCheckTxpool)
{
    CInvestContext ctxtInvest;
    if (!cntrBlock.RetrieveInvestContext(hashPrev, ctxtInvest))
    {
        StdLog("BlockChain", "Verify dest invest redeem: Retrieve invest context fail, prev block: %s", hashPrev.GetHex().c_str());
        return false;
    }
    auto it = ctxtInvest.mapInvest.find(dest);
    if (it == ctxtInvest.mapInvest.end())
    {
        if (fCheckTxpool)
        {
            int nTxPoolCount = pTxPool->GetDestTxpoolTxCount(dest);
            if (nTxPoolCount > 0)
            {
                StdLog("BlockChain", "Verify dest invest redeem: Txpool has data, tx pool tx count: %d, prev block: %s, dest: %s",
                       nTxPoolCount, hashPrev.GetHex().c_str(), CAddress(dest).ToString().c_str());
                return false;
            }
        }
    }
    else
    {
        if ((CBlock::GetBlockHeightByHash(hashPrev) - it->second.nStartHeight) <= (it->second.nCycle * INVEST_DAY_HEIGHT))
        {
            StdLog("BlockChain", "Verify dest invest redeem: The height is not enough, prev height: %d, start height: %d, cycle: %d, prev block: %s, dest: %s",
                   CBlock::GetBlockHeightByHash(hashPrev), it->second.nStartHeight, it->second.nCycle, hashPrev.GetHex().c_str(), CAddress(dest).ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CBlockChain::CheckDestActivate(const uint256& hashPrev, const CDestination& dest)
{
    int nActivateHeight = GetBlockActivateHeight(hashPrev, dest);
    if (nActivateHeight < 0)
    {
        StdLog("BlockChain", "Check dest activate: Get activate height fail, prev block: %s, dest: %s",
               hashPrev.GetHex().c_str(), CAddress(dest).ToString().c_str());
        return false;
    }
    if (CBlock::GetBlockHeightByHash(hashPrev) + 1 - nActivateHeight < ACTIVATE_CONFIRM_HEIGHT)
    {
        StdLog("BlockChain", "Check dest activate: Confirm that the height is not enough, block height: %d, activate height: %d, dest: %s",
               CBlock::GetBlockHeightByHash(hashPrev) + 1, nActivateHeight, CAddress(dest).ToString().c_str());
        return false;
    }
    return true;
}

int CBlockChain::GetBlockActivateHeight(const uint256& hashBlock, const CDestination& dest)
{
    CActivateContext ctxtActivate;
    if (!cntrBlock.RetrieveActivateContext(hashBlock, ctxtActivate))
    {
        StdLog("BlockChain", "Get dest activate: Retrieve activate fail, block: %s", hashBlock.GetHex().c_str());
        return -1;
    }
    auto it = ctxtActivate.mapActivate.find(dest);
    if (it == ctxtActivate.mapActivate.end())
    {
        StdLog("BlockChain", "Get dest activate: Find dest fail, size: %ld, block: %s, dest: %s",
               ctxtActivate.mapActivate.size(), hashBlock.GetHex().c_str(), CAddress(dest).ToString().c_str());
        return -1;
    }
    return it->second;
}

int64 CBlockChain::GetBlockInvestRewardTxMaxCount()
{
    CBlock block;

    ibrio::crypto::CPubKey pubkey(uint256("d1e1a33b30ec21b3675608679b7750ef2dd38d9bb8847ddf8a8e5c19273357ac"));
    const CDestination dest = CDestination(ibrio::crypto::CPubKey(uint256("2ae5c9621cd4ca80653a6fa4438ad8f31b240c29344c9655e6e64c88c213ee10")));

    CTemplatePtr ptrDelegate = CTemplate::CreateTemplatePtr(new CTemplateDelegate(pubkey, dest));
    CTemplatePtr ptrProof = CTemplateMint::CreateTemplatePtr(new CTemplateProof(pubkey, dest));

    size_t nMintDataSize = ptrDelegate->GetTemplateData().size();
    if (nMintDataSize < ptrProof->GetTemplateData().size())
    {
        nMintDataSize = ptrProof->GetTemplateData().size();
    }

    size_t nSigSize = nMintDataSize + 64 + 2;
    size_t nProofSize = 680;
    size_t nMaxTxSize = MAX_BLOCK_SIZE - GetSerializeSize(block) - nSigSize - nProofSize;
    nMaxTxSize = nMaxTxSize * 9 / 10;

    CTransaction txReward;
    xengine::CODataStream ds(txReward.vchData);
    uint32 nSeq = 0;
    ds << nSeq;

    return (nMaxTxSize / GetSerializeSize(txReward));
}

int64 CBlockChain::GetBlockInvestTotalReward(const uint256& hashPrev)
{
    CInvestContext ctxtInvest;
    if (!cntrBlock.RetrieveInvestContext(hashPrev, ctxtInvest))
    {
        StdLog("BlockChain", "Get block invest reward: Retrieve invest context fail, prev block: %s", hashPrev.GetHex().c_str());
        return -1;
    }
    int nPrevHeight = CBlock::GetBlockHeightByHash(hashPrev);
    int64 nTotalReward = 0;
    for (const auto& kv : ctxtInvest.mapInvest)
    {
        if ((nPrevHeight - kv.second.nStartHeight) < (kv.second.nCycle * INVEST_DAY_HEIGHT))
        {
            nTotalReward += ((kv.second.nInvestAmount / INVEST_UNIT_AMOUNT) * CalcHeightInvestReward(kv.second.nCycle));
        }
    }
    return (nTotalReward * MINT_REWARD_RATIO / MINT_REWARD_PER);
}

bool CBlockChain::VerifyInvestRewardTx(const CBlock& block, int64& nRewardTxCount)
{
    vector<CTransaction> vInvestRewardTx;
    if (!CalcBlockInvestRewardTx(block.hashPrev, block.GetBlockTime(), vInvestRewardTx))
    {
        return false;
    }
    if (block.vtx.size() < vInvestRewardTx.size())
    {
        return false;
    }
    for (size_t i = 0; i < vInvestRewardTx.size(); i++)
    {
        if (vInvestRewardTx[i] != block.vtx[i])
        {
            return false;
        }
    }
    nRewardTxCount = vInvestRewardTx.size();
    return true;
}

bool CBlockChain::VerifyBlockInvestVoteTx(const CBlock& block)
{
    map<CDestination, int64> mapDestAmount;
    for (const CTransaction& tx : block.vtx)
    {
        if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_VOTE)
        {
            int64& nBlockAmount = mapDestAmount[tx.sendTo];
            if (nBlockAmount > 0 || !CheckDestInvestAmount(block.hashPrev, tx.sendTo, false))
            {
                Log("Verify invest vote tx: Address transferred, txid: %s", tx.GetHash().GetHex().c_str());
                return false;
            }
            nBlockAmount += tx.nAmount;
        }
    }
    return true;
}

} // namespace ibrio
