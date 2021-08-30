// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "service.h"

#include "event.h"
#include "template/delegate.h"
#include "template/exchange.h"
#include "template/fork.h"
#include "template/vote.h"

using namespace std;
using namespace xengine;

extern void Shutdown();

namespace ibrio
{

//////////////////////////////
// CService

CService::CService()
  : pCoreProtocol(nullptr), pBlockChain(nullptr), pTxPool(nullptr), pDispatcher(nullptr), pWallet(nullptr), pNetwork(nullptr), pForkManager(nullptr), pNetChannel(nullptr)
{
}

CService::~CService()
{
}

bool CService::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }

    if (!GetObject("txpool", pTxPool))
    {
        Error("Failed to request txpool");
        return false;
    }

    if (!GetObject("dispatcher", pDispatcher))
    {
        Error("Failed to request dispatcher");
        return false;
    }

    if (!GetObject("wallet", pWallet))
    {
        Error("Failed to request wallet");
        return false;
    }

    if (!GetObject("peernet", pNetwork))
    {
        Error("Failed to request network");
        return false;
    }

    if (!GetObject("forkmanager", pForkManager))
    {
        Error("Failed to request forkmanager");
        return false;
    }

    if (!GetObject("netchannel", pNetChannel))
    {
        Error("Failed to request netchannel");
        return false;
    }

    return true;
}

void CService::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pTxPool = nullptr;
    pDispatcher = nullptr;
    pWallet = nullptr;
    pNetwork = nullptr;
    pForkManager = nullptr;
    pNetChannel = nullptr;
}

bool CService::HandleInvoke()
{
    {
        boost::unique_lock<boost::shared_mutex> wlock(rwForkStatus);
        pBlockChain->GetForkStatus(mapForkStatus);
    }
    return true;
}

void CService::HandleHalt()
{
    {
        boost::unique_lock<boost::shared_mutex> wlock(rwForkStatus);
        mapForkStatus.clear();
    }
}

void CService::NotifyBlockChainUpdate(const CBlockChainUpdate& update)
{
    map<uint256, bool> mapFork;
    pForkManager->GetValidForkList(mapFork);

    {
        boost::unique_lock<boost::shared_mutex> wlock(rwForkStatus);
        map<uint256, CForkStatus>::iterator it = mapForkStatus.find(update.hashFork);
        if (it == mapForkStatus.end())
        {
            it = mapForkStatus.insert(make_pair(update.hashFork, CForkStatus(update.hashFork, update.hashParent, update.nOriginHeight, update.nForkType))).first;
            if (it == mapForkStatus.end())
            {
                StdError("CService", "NotifyBlockChainUpdate: Insert fork status fail, fork: %s", update.hashFork.GetHex().c_str());
                return;
            }
            if (update.hashParent != 0)
            {
                mapForkStatus[update.hashParent].mapSubline.insert(make_pair(update.nOriginHeight, update.hashFork));
            }
        }

        CForkStatus& status = (*it).second;
        status.hashPrevBlock = update.hashPrevBlock;
        status.hashLastBlock = update.hashLastBlock;
        status.nLastBlockTime = update.nLastBlockTime;
        status.nLastBlockHeight = update.nLastBlockHeight;
        status.nMoneySupply = update.nMoneySupply;
        status.nMoneyDestroy = update.nMoneyDestroy;
        status.nMintType = update.nLastMintType;

        map<uint256, CForkStatus>::iterator mt = mapForkStatus.begin();
        while (mt != mapForkStatus.end())
        {
            auto nt = mapFork.find(mt->first);
            if (nt == mapFork.end() || !nt->second)
            {
                mapForkStatus.erase(mt++);
                continue;
            }
            ++mt;
        }
    }
}

void CService::NotifyNetworkPeerUpdate(const CNetworkPeerUpdate& update)
{
    (void)update;
}

void CService::NotifyTransactionUpdate(const CTransactionUpdate& update)
{
    (void)update;
}

void CService::Stop()
{
    Shutdown();
}

int CService::GetPeerCount()
{
    CEventPeerNetGetCount eventGetPeerCount(0);
    if (pNetwork->DispatchEvent(&eventGetPeerCount))
    {
        return eventGetPeerCount.result;
    }
    return 0;
}

void CService::GetPeers(vector<network::CBbPeerInfo>& vPeerInfo)
{
    vPeerInfo.clear();
    CEventPeerNetGetPeers eventGetPeers(0);
    if (pNetwork->DispatchEvent(&eventGetPeers))
    {
        vPeerInfo.reserve(eventGetPeers.result.size());
        for (unsigned int i = 0; i < eventGetPeers.result.size(); i++)
        {
            vPeerInfo.push_back(static_cast<network::CBbPeerInfo&>(eventGetPeers.result[i]));
        }
    }
}

bool CService::AddNode(const CNetHost& node)
{
    CEventPeerNetAddNode eventAddNode(0);
    eventAddNode.data = node;
    return pNetwork->DispatchEvent(&eventAddNode);
}

bool CService::RemoveNode(const CNetHost& node)
{
    CEventPeerNetRemoveNode eventRemoveNode(0);
    eventRemoveNode.data = node;
    return pNetwork->DispatchEvent(&eventRemoveNode);
}

int CService::GetForkCount()
{
    return mapForkStatus.size();
}

bool CService::HaveFork(const uint256& hashFork)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);

    map<uint256, CForkStatus>::iterator it = mapForkStatus.find(hashFork);
    if (it != mapForkStatus.end())
    {
        return true;
    }

    return false;
}

int CService::GetForkHeight(const uint256& hashFork)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);

    map<uint256, CForkStatus>::iterator it = mapForkStatus.find(hashFork);
    if (it != mapForkStatus.end())
    {
        return ((*it).second.nLastBlockHeight);
    }
    return 0;
}

bool CService::GetForkLastBlock(const uint256& hashFork, int& nLastHeight, uint256& hashLastBlock)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);

    map<uint256, CForkStatus>::iterator it = mapForkStatus.find(hashFork);
    if (it != mapForkStatus.end())
    {
        nLastHeight = it->second.nLastBlockHeight;
        hashLastBlock = it->second.hashLastBlock;
        return true;
    }
    return false;
}

int CService::GetForkType(const uint256& hashFork)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);

    map<uint256, CForkStatus>::iterator it = mapForkStatus.find(hashFork);
    if (it != mapForkStatus.end())
    {
        return ((*it).second.nForkType);
    }
    return FORK_TYPE_COMMON;
}

void CService::ListFork(std::vector<std::pair<uint256, CProfile>>& vFork, bool fAll)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);
    if (fAll)
    {
        map<uint256, bool> mapFork;
        pForkManager->GetValidForkList(mapFork);
        vFork.reserve(mapFork.size());
        for (const auto& vd : mapFork)
        {
            CForkContext ctx;
            if (pBlockChain->GetForkContext(vd.first, ctx))
            {
                vFork.push_back(make_pair(vd.first, ctx.GetProfile()));
            }
        }
    }
    else
    {
        vFork.reserve(mapForkStatus.size());
        for (map<uint256, CForkStatus>::iterator it = mapForkStatus.begin(); it != mapForkStatus.end(); ++it)
        {
            CProfile profile;
            if (pBlockChain->GetForkProfile((*it).first, profile))
            {
                vFork.push_back(make_pair((*it).first, profile));
            }
        }
    }
}

bool CService::GetForkGenealogy(const uint256& hashFork, vector<pair<uint256, int>>& vAncestry, vector<pair<int, uint256>>& vSubline)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);

    uint256 hashParent, hashJoint;
    int nJointHeight;
    if (!pForkManager->GetJoint(hashFork, hashParent, hashJoint, nJointHeight))
    {
        return false;
    }

    while (hashParent != 0)
    {
        vAncestry.push_back(make_pair(hashParent, nJointHeight));
        pForkManager->GetJoint(hashParent, hashParent, hashJoint, nJointHeight);
    }

    pForkManager->GetSubline(hashFork, vSubline);
    return true;
}

bool CService::GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlockLocation(hashBlock, hashFork, nHeight);
}

int CService::GetBlockCount(const uint256& hashFork)
{
    if (hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        return (GetForkHeight(hashFork) + 1);
    }
    return pBlockChain->GetBlockCount(hashFork);
}

bool CService::GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock)
{
    if (nHeight < 0)
    {
        boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);
        map<uint256, CForkStatus>::iterator it = mapForkStatus.find(hashFork);
        if (it == mapForkStatus.end())
        {
            return false;
        }
        hashBlock = (*it).second.hashLastBlock;
        return true;
    }
    return pBlockChain->GetBlockHash(hashFork, nHeight, hashBlock);
}

bool CService::GetBlockHash(const uint256& hashFork, int nHeight, vector<uint256>& vBlockHash)
{
    return pBlockChain->GetBlockHash(hashFork, nHeight, vBlockHash);
}

bool CService::GetBlock(const uint256& hashBlock, CBlock& block, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlock(hashBlock, block)
           && pBlockChain->GetBlockLocation(hashBlock, hashFork, nHeight);
}

bool CService::GetBlockEx(const uint256& hashBlock, CBlockEx& block, uint256& hashFork, int& nHeight)
{
    return pBlockChain->GetBlockEx(hashBlock, block)
           && pBlockChain->GetBlockLocation(hashBlock, hashFork, nHeight);
}

bool CService::GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime)
{
    return pBlockChain->GetLastBlockOfHeight(hashFork, nHeight, hashBlock, nTime);
}

bool CService::GetBlockStatus(const uint256& hashBlock, CBlockStatus& status)
{
    return pBlockChain->GetBlockStatus(hashBlock, status);
}

void CService::GetTxPool(const uint256& hashFork, vector<pair<uint256, size_t>>& vTxPool)
{
    vTxPool.clear();
    pTxPool->ListTx(hashFork, vTxPool);
}

void CService::ListTxPool(const uint256& hashFork, const CDestination& dest, vector<CTxInfo>& vTxPool, const int64 nGetOffset, const int64 nGetCount)
{
    vTxPool.clear();
    pTxPool->ListTx(hashFork, dest, vTxPool, nGetOffset, nGetCount);
}

bool CService::GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight, uint256& hashBlock, CDestination& destIn)
{
    CAssembledTx txTemp;
    if (pTxPool->Get(txid, txTemp))
    {
        int nAnchorHeight;
        if (!pBlockChain->GetBlockLocation(txTemp.hashAnchor, hashFork, nAnchorHeight))
        {
            StdLog("CService", "GetTransaction: BlockChain GetBlockLocation fail, txid: %s, hashAnchor: %s", txid.GetHex().c_str(), txTemp.hashAnchor.GetHex().c_str());
            return false;
        }
        tx = txTemp;
        nHeight = -1;
        destIn = txTemp.destIn;
    }
    else
    {
        if (!pBlockChain->GetTransaction(txid, tx, hashFork, nHeight))
        {
            StdLog("CService", "GetTransaction: BlockChain GetTransaction fail, txid: %s", txid.GetHex().c_str());
            return false;
        }

        std::vector<uint256> vHashBlock;
        if (!GetBlockHash(hashFork, nHeight, vHashBlock))
        {
            StdLog("CService", "GetTransaction: GetBlockHash fail, txid: %s, fork: %s, height: %d",
                   txid.GetHex().c_str(), hashFork.GetHex().c_str(), nHeight);
            return false;
        }
        for (const auto& hash : vHashBlock)
        {
            CBlockEx block;
            uint256 tempHashFork;
            int tempHeight = 0;
            if (!GetBlockEx(hash, block, tempHashFork, tempHeight))
            {
                StdLog("CService", "GetTransaction: GetBlockEx fail, txid: %s, block: %s",
                       txid.GetHex().c_str(), hash.GetHex().c_str());
                return false;
            }
            if (txid == block.txMint.GetHash())
            {
                hashBlock = hash;
                break;
            }
            for (int i = 0; i < block.vtx.size(); i++)
            {
                if (txid == block.vtx[i].GetHash())
                {
                    hashBlock = hash;
                    destIn = block.vTxContxt[i].destIn;
                    break;
                }
            }
            if (hashBlock != 0)
            {
                break;
            }
        }
    }
    return true;
}

Errno CService::SendTransaction(CTransaction& tx)
{
    return pDispatcher->AddNewTx(tx);
}

/*bool CService::RemovePendingTx(const uint256& txid)
{
    if (!pTxPool->Exists(txid))
    {
        return false;
    }
    pTxPool->Pop(txid);
    return true;
}*/

bool CService::ListForkUnspent(const uint256& hashFork, const CDestination& dest, uint32 nMax, std::vector<CTxUnspent>& vUnspent)
{
    std::vector<CTxUnspent> vUnspentOnChain;
    if (pBlockChain->ListForkUnspent(hashFork, dest, nMax, vUnspentOnChain) && pTxPool->ListForkUnspent(hashFork, dest, nMax, vUnspentOnChain, vUnspent))
    {
        return true;
    }

    return false;
}

bool CService::ListForkUnspentBatch(const uint256& hashFork, uint32 nMax, std::map<CDestination, std::vector<CTxUnspent>>& mapUnspent)
{
    std::map<CDestination, std::vector<CTxUnspent>> mapUnspentOnChain(mapUnspent);
    if (pBlockChain->ListForkUnspentBatch(hashFork, nMax, mapUnspentOnChain) && pTxPool->ListForkUnspentBatch(hashFork, nMax, mapUnspentOnChain, mapUnspent))
    {
        return true;
    }

    return false;
}

Errno CService::ListForkAddressUnspent(const uint256& hashFork, const CDestination& dest, uint32 nMax, int64 nAmount, vector<CTxUnspent>& vUnspent, string& strErr)
{
    if (nAmount <= 0)
    {
        map<CTxOutPoint, CUnspentOut> mapUnspent;
        if (!pDispatcher->FetchAddressUnspent(hashFork, dest, mapUnspent))
        {
            StdError("CService", "ListForkAddressUnspent: Fetch address unspent fail, fork: %s", hashFork.GetHex().c_str());
            strErr = "Fetch address unspent fail";
            return ERR_WALLET_NOT_FOUND;
        }

        vUnspent.reserve(mapUnspent.size());
        for (const auto& vd : mapUnspent)
        {
            vUnspent.push_back(CTxUnspent(vd.first, CTxOut(dest, vd.second.nAmount, vd.second.nTxTime, vd.second.nLockUntil), vd.second.nTxType, vd.second.nHeight));
            if (nMax != 0 && vUnspent.size() >= nMax)
            {
                break;
            }
        }
        return OK;
    }

    int nForkHeight = 0;
    uint256 hashLastBlock;
    {
        boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);
        map<uint256, CForkStatus>::iterator it = mapForkStatus.find(hashFork);
        if (it == mapForkStatus.end())
        {
            StdError("CService", "ListForkAddressUnspent: Find fork fail, fork: %s", hashFork.GetHex().c_str());
            strErr = "Find fork fail";
            return ERR_BLOCK_INVALID_FORK;
        }
        nForkHeight = it->second.nLastBlockHeight;
        hashLastBlock = it->second.hashLastBlock;
    }

    int64 nTxTime = GetNetTime();
    int64 nTargetValue = nAmount;
    size_t nMaxTxCount = 0;
    if (nMax == 0 || nMax > MAX_TX_INPUT_COUNT)
    {
        nMaxTxCount = MAX_TX_INPUT_COUNT;
    }
    else
    {
        nMaxTxCount = nMax;
    }

    return SelectCoinsByUnspent(dest, hashFork, nForkHeight, hashLastBlock, nTxTime, nTargetValue, nMaxTxCount, vUnspent, strErr);
}

bool CService::GetVotes(const CDestination& destDelegate, int64& nVotes, string& strFailCause)
{
    CTemplateId tid;
    if (!destDelegate.GetTemplateId(tid)
        || (tid.GetType() != TEMPLATE_DELEGATE && tid.GetType() != TEMPLATE_VOTE))
    {
        strFailCause = "Not a delegate or vote template address";
        return false;
    }
    if (tid.GetType() == TEMPLATE_DELEGATE)
    {
        if (!pBlockChain->GetVotes(destDelegate, nVotes))
        {
            strFailCause = "Query failed";
            return false;
        }
    }
    else
    {
        CTemplatePtr ptr = pWallet->GetTemplate(tid);
        if (ptr == nullptr)
        {
            strFailCause = "Vote template address not imported";
            return false;
        }
        CDestination destDelegateTemplateOut;
        CDestination destOwnerOut;
        boost::dynamic_pointer_cast<CTemplateVote>(ptr)->GetDelegateOwnerDestination(destDelegateTemplateOut, destOwnerOut);
        if (destDelegateTemplateOut.IsNull())
        {
            strFailCause = "Vote template address not imported";
            return false;
        }
        if (!pBlockChain->GetVotes(destDelegateTemplateOut, nVotes))
        {
            strFailCause = "Query failed";
            return false;
        }
    }
    return true;
}

bool CService::ListDelegate(uint32 nCount, std::multimap<int64, std::pair<CDestination, std::string>>& mapVotes)
{
    return pBlockChain->ListDelegate(nCount, mapVotes);
}

int CService::GetLastBlockActivateHeight(const CDestination& dest)
{
    CBlockStatus status;
    if (!pBlockChain->GetLastBlockStatus(pCoreProtocol->GetGenesisBlockHash(), status))
    {
        return -1;
    }
    return pBlockChain->GetBlockActivateHeight(status.hashBlock, dest);
}

bool CService::HaveKey(const crypto::CPubKey& pubkey, const int32 nVersion)
{
    return pWallet->Have(pubkey, nVersion);
}

size_t CService::GetPubKeys(set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount)
{
    return pWallet->GetPubKeys(setPubKey, nPos, nCount);
}

bool CService::GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic)
{
    return pWallet->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic);
}

boost::optional<std::string> CService::MakeNewKey(const crypto::CCryptoString& strPassphrase, crypto::CPubKey& pubkey)
{
    crypto::CKey key;
    if (!key.Renew())
    {
        return std::string("Renew Key Failed");
    }
    if (!strPassphrase.empty())
    {
        if (!key.Encrypt(strPassphrase))
        {
            return std::string("Encrypt Key failed");
        }
        key.Lock();
    }
    pubkey = key.GetPubKey();
    return pWallet->AddKey(key);
}

boost::optional<std::string> CService::AddKey(const crypto::CKey& key)
{
    return pWallet->AddKey(key);
}

boost::optional<std::string> CService::RemoveKey(const crypto::CPubKey& pubkey)
{
    return pWallet->RemoveKey(pubkey);
}

bool CService::ImportKey(const vector<unsigned char>& vchKey, crypto::CPubKey& pubkey)
{
    return pWallet->Import(vchKey, pubkey);
}

bool CService::ExportKey(const crypto::CPubKey& pubkey, vector<unsigned char>& vchKey)
{
    return pWallet->Export(pubkey, vchKey);
}

bool CService::EncryptKey(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
                          const crypto::CCryptoString& strCurrentPassphrase)
{
    return pWallet->Encrypt(pubkey, strPassphrase, strCurrentPassphrase);
}

bool CService::Lock(const crypto::CPubKey& pubkey)
{
    return pWallet->Lock(pubkey);
}

bool CService::Unlock(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase, int64 nTimeout)
{
    return pWallet->Unlock(pubkey, strPassphrase, nTimeout);
}

bool CService::SignSignature(const crypto::CPubKey& pubkey, const uint256& hash, vector<unsigned char>& vchSig)
{
    return pWallet->Sign(pubkey, hash, vchSig);
}

bool CService::SignTransaction(CTransaction& tx, const vector<uint8>& vchDestInData, const vector<uint8>& vchSendToData, const vector<uint8>& vchSignExtraData, bool& fCompleted)
{
    uint256 hashFork;
    int nHeight;
    if (!pBlockChain->GetBlockLocation(tx.hashAnchor, hashFork, nHeight))
    {
        StdError("CService", "Sign Transaction: GetBlockLocation fail, txid: %s, hashAnchor: %s", tx.GetHash().GetHex().c_str(), tx.hashAnchor.GetHex().c_str());
        return false;
    }
    vector<CTxOut> vUnspent;
    if (!pTxPool->FetchInputs(hashFork, tx, vUnspent) || vUnspent.empty())
    {
        StdError("CService", "Sign Transaction: FetchInputs fail or vUnspent is empty, txid: %s", tx.GetHash().GetHex().c_str());
        return false;
    }

    const CDestination& destIn = vUnspent[0].destTo;
    int32 nForkHeight;
    uint256 hashLastBlock;
    if (!GetForkLastBlock(hashFork, nForkHeight, hashLastBlock))
    {
        StdError("CService", "SignTransaction: GetForkLastBlock fail, txid: %s", tx.GetHash().GetHex().c_str());
        return false;
    }
    if (hashFork == pCoreProtocol->GetGenesisBlockHash()
        && tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_FORK)
    {
        vector<pair<CDestination, CForkContext>> vForkCtxt;
        if (!pBlockChain->VerifyBlockForkTx(hashLastBlock, tx, vForkCtxt) || vForkCtxt.empty())
        {
            StdError("CService", "SignTransaction: Verify block fork tx fail, txid: %s", tx.GetHash().GetHex().c_str());
            return false;
        }
    }
    if (!pWallet->SignTransaction(destIn, tx, vchDestInData, vchSendToData, vchSignExtraData, hashFork, nForkHeight, fCompleted))
    {
        StdError("CService", "Sign ransaction: Sign Transaction fail, txid: %s, destIn: %s", tx.GetHash().GetHex().c_str(), CAddress(destIn).ToString().c_str());
        return false;
    }
    return true;
}

bool CService::HaveTemplate(const CTemplateId& tid)
{
    return pWallet->Have(tid);
}

void CService::GetTemplateIds(set<CTemplateId>& setTid, const uint64 nPos, const uint64 nCount)
{
    pWallet->GetTemplateIds(setTid, nPos, nCount);
}

bool CService::AddTemplate(CTemplatePtr& ptr)
{
    return pWallet->AddTemplate(ptr);
}

CTemplatePtr CService::GetTemplate(const CTemplateId& tid)
{
    return pWallet->GetTemplate(tid);
}

bool CService::RemoveTemplate(const CTemplateId& tid)
{
    return pWallet->RemoveTemplate(tid);
}

bool CService::GetDeFiRelation(const uint256& hashFork, const CDestination& destIn, CDestination& parent)
{
    return pBlockChain->GetDeFiRelation(hashFork, destIn, parent);
}

bool CService::GetBalanceByUnspent(const CDestination& dest, const uint256& hashFork, CWalletBalance& balance)
{
    int32 nForkHeight;
    uint256 hashLastBlock;
    if (!GetForkLastBlock(hashFork, nForkHeight, hashLastBlock))
    {
        StdError("CService", "GetBalanceByUnspent: GetForkLastBlock fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    map<CTxOutPoint, CUnspentOut> mapUnspent;
    if (!pDispatcher->FetchAddressUnspent(hashFork, dest, mapUnspent))
    {
        StdError("CService", "GetBalanceByUnspent: Fetch address unspent fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }

    balance.SetNull();
    int64 nTotalValue = 0;
    for (const auto& vd : mapUnspent)
    {
        nTotalValue += vd.second.nAmount;
        if (vd.second.IsLocked(nForkHeight)
            || (dest.IsTemplate() && dest.GetTemplateId().GetType() == TEMPLATE_VOTE
                && !pBlockChain->VerifyDestInvestRedeem(hashLastBlock, dest, true)))
        {
            balance.nLocked += vd.second.nAmount;
        }
        else if (vd.second.nHeight < 0)
        {
            balance.nUnconfirmed += vd.second.nAmount;
        }
    }

    // locked fork template
    if (dest.IsTemplate() && dest.GetTemplateId().GetType() == TEMPLATE_FORK
        && hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        int64 nLockedCoin = 0;
        CTemplatePtr ptr = GetTemplate(dest.GetTemplateId());
        if (!ptr)
        {
            StdLog("CService", "GetBalanceByUnspent: GetTemplate fail, destIn: %s", dest.ToString().c_str());
            nLockedCoin = CTemplateFork::CreatedCoin();
        }
        else
        {
            CDestination destRedeemLocked;
            uint256 hashForkLocked;
            boost::dynamic_pointer_cast<CLockedCoinTemplate>(ptr)->GetForkParam(destRedeemLocked, hashForkLocked);
            nLockedCoin = pForkManager->ForkLockedCoin(hashForkLocked, hashLastBlock);
            if (nLockedCoin < 0)
            {
                nLockedCoin = 0;
                for (const auto& vd : mapUnspent)
                {
                    if (vd.second.nHeight < 0)
                    {
                        nLockedCoin = CTemplateFork::CreatedCoin();
                        break;
                    }
                }
            }
        }
        balance.nLocked += nLockedCoin;
        if (balance.nLocked > nTotalValue)
        {
            balance.nLocked = nTotalValue;
        }
    }

    balance.nAvailable = nTotalValue - balance.nLocked;
    return true;
}

bool CService::ListTransaction(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, vector<CTxInfo>& vTx)
{
    if (nPrevHeight < -1 || nPrevTxSeq == -1)
    {
        // nOffset is valid
        if (nOffset == -1)
        {
            // last count transactions
            if (dest.IsNull())
            {
                return false;
            }
            vector<CTxInfo> vTxCache;
            if (!pTxPool->ListTx(hashFork, dest, vTxCache, -1, nCount))
            {
                return false;
            }
            if (nCount <= 0 || vTxCache.size() < nCount)
            {
                if (pBlockChain->GetAddressTxList(hashFork, dest, -2, -1, -1, ((nCount > 0) ? (nCount - vTxCache.size()) : 0), vTx) < 0)
                {
                    return false;
                }
                if (!vTxCache.empty())
                {
                    vTx.insert(vTx.end(), vTxCache.begin(), vTxCache.end());
                }
            }
            else
            {
                vTx.assign(vTxCache.begin(), vTxCache.end());
            }
        }
        else
        {
            // positive sequence
            int64 nGetEndOffset = pBlockChain->GetAddressTxList(hashFork, dest, -2, -1, nOffset, nCount, vTx);
            if (nGetEndOffset < 0)
            {
                return false;
            }
            if (vTx.size() == 0)
            {
                int64 nTxPoolOffset = nOffset - nGetEndOffset;
                if (nTxPoolOffset < 0)
                {
                    nTxPoolOffset = 0;
                }
                if (!pTxPool->ListTx(hashFork, dest, vTx, nTxPoolOffset, nCount))
                {
                    return false;
                }
            }
            else if (nCount <= 0 || vTx.size() < nCount)
            {
                if (!pTxPool->ListTx(hashFork, dest, vTx, 0, ((nCount > 0) ? (nCount - vTx.size()) : 0)))
                {
                    return false;
                }
            }
        }
    }
    else if (nPrevHeight == -1)
    {
        // nPrevHeight at txpool
        if (!pTxPool->ListTxOfSeq(hashFork, dest, vTx, nPrevTxSeq, nCount))
        {
            return false;
        }
    }
    else
    {
        // nPrevHeight and nPrevTxSeq is valid
        if (pBlockChain->GetAddressTxList(hashFork, dest, nPrevHeight, nPrevTxSeq, nOffset, nCount, vTx) < 0)
        {
            return false;
        }
        if (vTx.size() == 0)
        {
            if (!pTxPool->ListTx(hashFork, dest, vTx, 0, nCount))
            {
                return false;
            }
        }
        else if (nCount <= 0 || vTx.size() < nCount)
        {
            if (!pTxPool->ListTx(hashFork, dest, vTx, 0, ((nCount > 0) ? (nCount - vTx.size()) : 0)))
            {
                return false;
            }
        }
    }
    return true;
}

boost::optional<std::string> CService::CreateTransactionByUnspent(const uint256& hashFork, const CDestination& destFrom,
                                                                  const CDestination& destSendTo, const uint16 nType, const int64 nAmount, const int64 nTxFee, const int nLockHeight,
                                                                  const vector<unsigned char>& vchData, CTransaction& txNew)
{
    int nForkHeight = 0;
    uint256 hashLastBlock;
    txNew.SetNull();
    {
        boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);
        map<uint256, CForkStatus>::iterator it = mapForkStatus.find(hashFork);
        if (it == mapForkStatus.end())
        {
            StdError("CService", "CreateTransactionByUnspent: find fork fail, fork: %s", hashFork.GetHex().c_str());
            return std::string("find fork fail, fork: ") + hashFork.GetHex();
        }
        nForkHeight = it->second.nLastBlockHeight;
        hashLastBlock = it->second.hashLastBlock;
        txNew.hashAnchor = hashFork;
    }
    txNew.nType = nType;
    txNew.nTimeStamp = GetNetTime();
    txNew.nLockUntil = 0;
    txNew.sendTo = destSendTo;
    txNew.nAmount = nAmount;
    txNew.nTxFee = nTxFee;
    txNew.vchData = vchData;

    if (nLockHeight > 0)
    {
        txNew.SetLockUntil(nLockHeight);
    }

    if (txNew.nType == CTransaction::TX_DEFI_MINT_HEIGHT)
    {
        CProfile profile;
        if (!pBlockChain->GetForkProfile(hashFork, profile))
        {
            return std::string("Get fork profile failed");
        }
        if (pCoreProtocol->VerifyMintHeightTx(txNew, destFrom, hashFork, nForkHeight + 1, profile) != OK)
        {
            return std::string("Mint height Tx failed. Attention: "
                               "1. Fork must be DeFi; "
                               "2. From dest & to dest must be fork owner; "
                               "3. Not set mint height or set -1 when creating fork; "
                               "4. This tx is the first mint height tx; "
                               "5. Param mintheight must be larger than block height; ");
        }
    }

    string strErr;
    vector<CTxUnspent> vCoins;
    Errno err = SelectCoinsByUnspent(destFrom, hashFork, nForkHeight, hashLastBlock, txNew.GetTxTime(), txNew.nAmount + txNew.nTxFee, MAX_TX_INPUT_COUNT, vCoins, strErr);
    if (err != OK)
    {
        StdError("CService", "CreateTransactionByUnspent: Select coin not enough, destIn: %s", CAddress(destFrom).ToString().c_str());
        return strErr;
    }

    txNew.vInput.clear();
    txNew.vInput.reserve(vCoins.size());
    for (const CTxUnspent& unspent : vCoins)
    {
        txNew.vInput.emplace_back(CTxIn(static_cast<const CTxOutPoint&>(unspent)));
    }

    return boost::optional<std::string>{};
}

Errno CService::SelectCoinsByUnspent(const CDestination& dest, const uint256& hashFork, int nForkHeight, const uint256& hashLastBlock,
                                     int64 nTxTime, int64 nTargetValue, size_t nMaxInput, vector<CTxUnspent>& vCoins, string& strErr)
{
    map<CTxOutPoint, CUnspentOut> mapUnspent;
    if (!pDispatcher->FetchAddressUnspent(hashFork, dest, mapUnspent))
    {
        StdError("CService", "SelectCoinsByUnspent: Fetch address unspent fail, dest: %s", CAddress(dest).ToString().c_str());
        strErr = "Fetch address unspent fail";
        return ERR_WALLET_NOT_FOUND;
    }

    // locked fork template
    CTemplateId tid;
    if (dest.GetTemplateId(tid) && tid.GetType() == TEMPLATE_FORK
        && hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        int64 nLockedCoin = 0;
        CTemplatePtr ptr = GetTemplate(tid);
        if (!ptr)
        {
            StdLog("CService", "SelectCoinsByUnspent: GetTemplate fail, dest: %s", CAddress(dest).ToString().c_str());
            nLockedCoin = CTemplateFork::CreatedCoin();
        }
        else
        {
            CDestination destRedeemLocked;
            uint256 hashForkLocked;
            boost::dynamic_pointer_cast<CLockedCoinTemplate>(ptr)->GetForkParam(destRedeemLocked, hashForkLocked);
            nLockedCoin = pForkManager->ForkLockedCoin(hashForkLocked, hashLastBlock);
            if (nLockedCoin < 0)
            {
                nLockedCoin = 0;
                for (const auto& vd : mapUnspent)
                {
                    if (vd.second.nHeight < 0)
                    {
                        nLockedCoin = CTemplateFork::CreatedCoin();
                        break;
                    }
                }
            }
        }
        nTargetValue += nLockedCoin;
    }

    vCoins.clear();

    pair<int64, CTxUnspent> coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<int64>::max();
    int64 nTotalLower = 0;
    uint16 nDestTemplateType = 0;
    if (dest.IsTemplate())
    {
        nDestTemplateType = dest.GetTemplateId().GetType();
    }

    multimap<int64, CTxUnspent> mapValue;

    for (const auto& vd : mapUnspent)
    {
        const CUnspentOut& out = vd.second;
        if (out.IsLocked(nForkHeight) || out.GetTxTime() > nTxTime
            || (out.nTxType == CTransaction::TX_CERT && vd.first.n == 0))
        {
            continue;
        }
        if (out.nTxType == CTransaction::TX_DEFI_REWARD && nDestTemplateType == TEMPLATE_DEXMATCH)
        {
            continue;
        }

        int64 nValue = out.nAmount;
        pair<int64, CTxUnspent> coin = make_pair(nValue, CTxUnspent(vd.first, CTxOut(dest, out.nAmount, out.nTxTime, out.nLockUntil), vd.second.nTxType, vd.second.nHeight));

        if (nValue == nTargetValue)
        {
            vCoins.push_back(coin.second);
            return OK;
        }
        else if (nValue < nTargetValue)
        {
            mapValue.insert(coin);
            nTotalLower += nValue;
            while (mapValue.size() > nMaxInput)
            {
                multimap<int64, CTxUnspent>::iterator mi = mapValue.begin();
                nTotalLower -= (*mi).first;
                mapValue.erase(mi);
            }
            if (nTotalLower >= nTargetValue)
            {
                break;
            }
        }
        else if (nValue < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    }

    int64 nValueRet = 0;
    if (nTotalLower >= nTargetValue)
    {
        while (nValueRet < nTargetValue)
        {
            int64 nShortage = nTargetValue - nValueRet;
            multimap<int64, CTxUnspent>::iterator it = mapValue.lower_bound(nShortage);
            if (it == mapValue.end())
            {
                --it;
            }
            vCoins.push_back(it->second);
            nValueRet += (*it).first;
            mapValue.erase(it);
        }
    }
    else if (!coinLowestLarger.second.IsNull())
    {
        vCoins.push_back(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        multimap<int64, CTxUnspent>::iterator it = mapValue.begin();
        for (int i = 0; i < 3 && it != mapValue.end(); i++, ++it)
        {
            vCoins.push_back(it->second);
            nValueRet += (*it).first;
        }
    }

    if (nValueRet < nTargetValue)
    {
        StdLog("CService", "SelectCoinsByUnspent: Not enough funds in wallet or account, balance: %lu, need: %lu, dest: %s",
               nValueRet, nTargetValue, CAddress(dest).ToString().c_str());
        strErr = string("Not enough funds in wallet or account, balance: ")
                 + to_string(nValueRet)
                 + string(", need: ")
                 + to_string(nTargetValue);
        return ERR_WALLET_INSUFFICIENT_FUNDS;
    }

    return OK;
}

bool CService::SignOfflineTransaction(const CDestination& destIn, CTransaction& tx, const vector<uint8>& vchDestInData, const vector<uint8>& vchSendToData, const vector<uint8>& vchSignExtraData, bool& fCompleted)
{
    uint256 hashFork;
    int nHeight;
    if (!pBlockChain->GetBlockLocation(tx.hashAnchor, hashFork, nHeight))
    {
        StdError("CService", "SignOfflineTransaction: GetBlockLocation fail, txid: %s, hashAnchor: %s", tx.GetHash().GetHex().c_str(), tx.hashAnchor.GetHex().c_str());
        return false;
    }

    int32 nForkHeight = GetForkHeight(hashFork);
    if (!pWallet->SignTransaction(destIn, tx, vchDestInData, vchSendToData, vchSignExtraData, hashFork, nForkHeight, fCompleted))
    {
        StdError("CService", "SignOfflineTransaction: Sign Transaction fail, txid: %s, destIn: %s", tx.GetHash().GetHex().c_str(), destIn.ToString().c_str());
        return false;
    }

    return true;
}

Errno CService::SendOfflineSignedTransaction(CTransaction& tx)
{
    return pDispatcher->AddNewTx(tx, 0);
}

bool CService::AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext)
{
    return pWallet->AesEncrypt(pubkeyLocal, pubkeyRemote, vMessage, vCiphertext);
}

bool CService::AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage)
{
    return pWallet->AesDecrypt(pubkeyLocal, pubkeyRemote, vCiphertext, vMessage);
}

bool CService::AddMemKey(const uint256& secret, crypto::CPubKey& pubkey)
{
    return pWallet->AddMemKey(secret, pubkey);
}

void CService::RemoveMemKey(const crypto::CPubKey& pubkey)
{
    return pWallet->RemoveMemKey(pubkey);
}

void CService::GetWalletDestinations(std::set<CDestination>& setDest)
{
    pWallet->GetDestinations(setDest);
}

bool CService::GetWork(vector<unsigned char>& vchWorkData, int& nPrevBlockHeight,
                       uint256& hashPrev, uint32& nPrevTime, int& nAlgo,
                       int& nBits, const CTemplateMintPtr& templMint)
{
    CBlock block;
    block.nType = CBlock::BLOCK_PRIMARY;

    {
        boost::shared_lock<boost::shared_mutex> rlock(rwForkStatus);
        map<uint256, CForkStatus>::iterator it = mapForkStatus.find(pCoreProtocol->GetGenesisBlockHash());
        if (it == mapForkStatus.end())
        {
            StdError("CService", "GetWork: mapForkStatus find fail");
            return false;
        }
        hashPrev = it->second.hashLastBlock;
        nPrevTime = it->second.nLastBlockTime;
        nPrevBlockHeight = it->second.nLastBlockHeight;
        block.hashPrev = hashPrev;

        nPrevTime = pCoreProtocol->GetNextBlockTimeStamp(it->second.nMintType, it->second.nLastBlockTime, CTransaction::TX_WORK);
        block.nTimeStamp = std::max((uint32)GetNetTime(), nPrevTime);
    }

    bool fIsDpos = false;
    if (pNetChannel->IsLocalCachePowBlock(nPrevBlockHeight + 1, fIsDpos))
    {
        StdTrace("CService", "GetWork: IsLocalCachePowBlock pow exist");
        return false;
    }

    nAlgo = CM_CRYPTONIGHT;
    int64 nReward;
    if (!pBlockChain->GetProofOfWorkTarget(block.hashPrev, nAlgo, nBits, nReward))
    {
        StdError("CService", "GetWork: GetProofOfWorkTarget fail");
        return false;
    }

    CProofOfHashWork proof;
    proof.nWeight = 0;
    proof.nAgreement = 0;
    proof.nAlgo = nAlgo;
    proof.nBits = nBits;
    proof.destMint = CDestination(templMint->GetTemplateId());
    proof.nNonce = 0;
    proof.Save(block.vchProof);

    block.GetSerializedProofOfWorkData(vchWorkData);

    if (fIsDpos)
    {
        nAlgo = CM_MPVSS;
    }
    return true;
}

Errno CService::SubmitWork(const vector<unsigned char>& vchWorkData,
                           const CTemplateMintPtr& templMint, crypto::CKey& keyMint, uint256& hashBlock)
{
    if (vchWorkData.empty())
    {
        StdError("CService", "SubmitWork: vchWorkData is empty");
        return FAILED;
    }
    CBlock block;
    CProofOfHashWorkCompact proof;
    CBufStream ss;
    ss.Write((const char*)&vchWorkData[0], vchWorkData.size());
    try
    {
        ss >> block.nVersion >> block.nType >> block.nTimeStamp >> block.hashPrev >> block.vchProof;
        proof.Load(block.vchProof);
        if (proof.nAlgo != CM_CRYPTONIGHT)
        {
            StdError("CService", "SubmitWork: nAlgo error");
            return ERR_BLOCK_PROOF_OF_WORK_INVALID;
        }
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return FAILED;
    }
    int nBits;
    int64 nReward;
    if (!pBlockChain->GetProofOfWorkTarget(block.hashPrev, proof.nAlgo, nBits, nReward))
    {
        StdError("CService", "SubmitWork: GetProofOfWorkTarget fail");
        return FAILED;
    }

    CTransaction& txMint = block.txMint;
    txMint.nType = CTransaction::TX_WORK;
    txMint.nTimeStamp = block.nTimeStamp;
    txMint.hashAnchor = block.hashPrev;
    txMint.sendTo = CDestination(templMint->GetTemplateId());
    txMint.nAmount = pBlockChain->GetBlockInvestTotalReward(block.hashPrev);
    if (txMint.nAmount < 0)
    {
        StdError("CService", "SubmitWork: Get block invest total reward fail");
        return FAILED;
    }

    size_t nSigSize = templMint->GetTemplateData().size() + 64 + 2;
    size_t nMaxTxSize = MAX_BLOCK_SIZE - GetSerializeSize(block) - nSigSize;
    int64 nTotalTxFee = 0;

    vector<CTransaction> vInvestRewardTx;
    if (!pBlockChain->CalcBlockInvestRewardTx(block.hashPrev, block.GetBlockTime(), vInvestRewardTx))
    {
        StdError("CService", "SubmitWork: Get block invest reward fail");
        return FAILED;
    }
    for (const CTransaction& tx : vInvestRewardTx)
    {
        size_t nTxSize = GetSerializeSize(tx);
        if (nMaxTxSize < nTxSize)
        {
            StdError("CService", "SubmitWork: Invest reward tx size error");
            return FAILED;
        }
        block.vtx.push_back(tx);
        nTotalTxFee += tx.nTxFee;
        nMaxTxSize -= nTxSize;
    }

    if (!pTxPool->FetchArrangeBlockTx(pCoreProtocol->GetGenesisBlockHash(), block.hashPrev, block.GetBlockHeight(), block.nTimeStamp, nMaxTxSize, block.vtx, nTotalTxFee))
    {
        StdError("CService", "SubmitWork: Fetch arrange block tx fail");
        return FAILED;
    }
    block.hashMerkle = block.CalcMerkleTreeRoot();
    block.txMint.nAmount += nTotalTxFee;

    hashBlock = block.GetHash();
    vector<unsigned char> vchMintSig;
    if (!keyMint.Sign(hashBlock, vchMintSig)
        || !templMint->BuildBlockSignature(hashBlock, vchMintSig, block.vchSig))
    {
        StdError("CService", "SubmitWork: Sign fail");
        return ERR_BLOCK_SIGNATURE_INVALID;
    }

    Errno err = pCoreProtocol->ValidateBlock(block);
    if (err != OK)
    {
        StdError("CService", "SubmitWork: ValidateBlock fail");
        return err;
    }

    if (!pNetChannel->AddCacheLocalPowBlock(block))
    {
        StdError("CService", "SubmitWork: AddCacheLocalPowBlock fail");
        return FAILED;
    }
    return OK;
}

} // namespace ibrio
