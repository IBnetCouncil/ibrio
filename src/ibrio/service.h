// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_SERVICE_H
#define IBRIO_SERVICE_H

#include "base.h"
#include "network.h"
#include "xengine.h"

namespace ibrio
{

class CService : public IService
{
public:
    CService();
    ~CService();
    /* Notify */
    void NotifyBlockChainUpdate(const CBlockChainUpdate& update) override;
    void NotifyNetworkPeerUpdate(const CNetworkPeerUpdate& update) override;
    void NotifyTransactionUpdate(const CTransactionUpdate& update) override;
    /* System */
    void Stop() override;
    /* Network */
    int GetPeerCount() override;
    void GetPeers(std::vector<network::CBbPeerInfo>& vPeerInfo) override;
    bool AddNode(const xengine::CNetHost& node) override;
    bool RemoveNode(const xengine::CNetHost& node) override;
    /* Blockchain & Tx Pool*/
    int GetForkCount() override;
    bool HaveFork(const uint256& hashFork) override;
    int GetForkHeight(const uint256& hashFork) override;
    bool GetForkLastBlock(const uint256& hashFork, int& nLastHeight, uint256& hashLastBlock) override;
    int GetForkType(const uint256& hashFork) override;
    void ListFork(std::vector<std::pair<uint256, CProfile>>& vFork, bool fAll = false) override;
    bool GetForkGenealogy(const uint256& hashFork, std::vector<std::pair<uint256, int>>& vAncestry,
                          std::vector<std::pair<int, uint256>>& vSubline) override;
    bool GetBlockLocation(const uint256& hashBlock, uint256& hashFork, int& nHeight) override;
    int GetBlockCount(const uint256& hashFork) override;
    bool GetBlockHash(const uint256& hashFork, int nHeight, uint256& hashBlock) override;
    bool GetBlockHash(const uint256& hashFork, int nHeight, std::vector<uint256>& vBlockHash) override;
    bool GetBlock(const uint256& hashBlock, CBlock& block, uint256& hashFork, int& nHeight) override;
    bool GetBlockEx(const uint256& hashBlock, CBlockEx& block, uint256& hashFork, int& nHeight) override;
    bool GetLastBlockOfHeight(const uint256& hashFork, const int nHeight, uint256& hashBlock, int64& nTime) override;
    bool GetBlockStatus(const uint256& hashBlock, CBlockStatus& status) override;
    void GetTxPool(const uint256& hashFork, std::vector<std::pair<uint256, std::size_t>>& vTxPool) override;
    void ListTxPool(const uint256& hashFork, const CDestination& dest, std::vector<CTxInfo>& vTxPool, const int64 nGetOffset = 0, const int64 nGetCount = 0) override;
    bool GetTransaction(const uint256& txid, CTransaction& tx, uint256& hashFork, int& nHeight, uint256& hashBlock, CDestination& destIn) override;
    Errno SendTransaction(CTransaction& tx) override;
    //bool RemovePendingTx(const uint256& txid) override;
    bool ListForkUnspent(const uint256& hashFork, const CDestination& dest, uint32 nMax, std::vector<CTxUnspent>& vUnspent) override;
    bool ListForkUnspentBatch(const uint256& hashFork, uint32 nMax, std::map<CDestination, std::vector<CTxUnspent>>& mapUnspent) override;
    Errno ListForkAddressUnspent(const uint256& hashFork, const CDestination& dest, uint32 nMax, int64 nAmount, std::vector<CTxUnspent>& vUnspent, std::string& strErr) override;
    bool GetVotes(const CDestination& destDelegate, int64& nVotes, string& strFailCause) override;
    bool ListDelegate(uint32 nCount, std::multimap<int64, std::pair<CDestination, std::string>>& mapVotes) override;
    int GetLastBlockActivateHeight(const CDestination& dest) override;
    /* Wallet */
    bool HaveKey(const crypto::CPubKey& pubkey, const int32 nVersion = -1) override;
    std::size_t GetPubKeys(std::set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) override;
    bool GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) override;
    boost::optional<std::string> MakeNewKey(const crypto::CCryptoString& strPassphrase, crypto::CPubKey& pubkey) override;
    boost::optional<std::string> AddKey(const crypto::CKey& key) override;
    boost::optional<std::string> RemoveKey(const crypto::CPubKey& pubkey) override;
    bool ImportKey(const std::vector<unsigned char>& vchKey, crypto::CPubKey& pubkey) override;
    bool ExportKey(const crypto::CPubKey& pubkey, std::vector<unsigned char>& vchKey) override;
    bool EncryptKey(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
                    const crypto::CCryptoString& strCurrentPassphrase) override;
    bool Lock(const crypto::CPubKey& pubkey) override;
    bool Unlock(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase, int64 nTimeout) override;
    bool SignSignature(const crypto::CPubKey& pubkey, const uint256& hash, std::vector<unsigned char>& vchSig) override;
    bool SignTransaction(CTransaction& tx, const vector<uint8>& vchDestInData, const vector<uint8>& vchSendToData, const vector<uint8>& vchSignExtraData, bool& fCompleted) override;
    bool HaveTemplate(const CTemplateId& tid) override;
    void GetTemplateIds(std::set<CTemplateId>& setTid, const uint64 nPos, const uint64 nCount) override;
    bool AddTemplate(CTemplatePtr& ptr) override;
    CTemplatePtr GetTemplate(const CTemplateId& tid) override;
    bool RemoveTemplate(const CTemplateId& tid) override;
    bool GetDeFiRelation(const uint256& hashFork, const CDestination& destIn, CDestination& parent) override;
    bool GetBalanceByUnspent(const CDestination& dest, const uint256& hashFork, CWalletBalance& balance) override;
    bool ListTransaction(const uint256& hashFork, const CDestination& dest, const int nPrevHeight, const uint64 nPrevTxSeq, const int64 nOffset, const int64 nCount, std::vector<CTxInfo>& vTx) override;
    boost::optional<std::string> CreateTransactionByUnspent(const uint256& hashFork, const CDestination& destFrom,
                                                            const CDestination& destSendTo, const uint16 nType, const int64 nAmount, const int64 nTxFee, const int nLockHeight,
                                                            const std::vector<unsigned char>& vchData, CTransaction& txNew) override;
    bool SignOfflineTransaction(const CDestination& destIn, CTransaction& tx, const vector<uint8>& vchDestInData, const vector<uint8>& vchSendToData, const vector<uint8>& vchSignExtraData, bool& fCompleted) override;
    Errno SendOfflineSignedTransaction(CTransaction& tx) override;
    bool AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext) override;
    bool AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage) override;
    bool AddMemKey(const uint256& secret, crypto::CPubKey& pubkey) override;
    void RemoveMemKey(const crypto::CPubKey& pubkey) override;
    void GetWalletDestinations(std::set<CDestination>& setDest) override;
    /* Mint */
    bool GetWork(std::vector<unsigned char>& vchWorkData, int& nPrevBlockHeight,
                 uint256& hashPrev, uint32& nPrevTime, int& nAlgo, int& nBits,
                 const CTemplateMintPtr& templMint) override;
    Errno SubmitWork(const std::vector<unsigned char>& vchWorkData, const CTemplateMintPtr& templMint,
                     crypto::CKey& keyMint, uint256& hashBlock) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;

    Errno SelectCoinsByUnspent(const CDestination& dest, const uint256& hashFork, int nForkHeight, const uint256& hashLastBlock,
                               int64 nTxTime, int64 nTargetValue, size_t nMaxInput, vector<CTxUnspent>& vCoins, std::string& strErr);

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    ITxPool* pTxPool;
    IDispatcher* pDispatcher;
    IWallet* pWallet;
    CNetwork* pNetwork;
    IForkManager* pForkManager;
    network::INetChannel* pNetChannel;
    mutable boost::shared_mutex rwForkStatus;
    std::map<uint256, CForkStatus> mapForkStatus;
};

} // namespace ibrio

#endif //IBRIO_SERVICE_H
