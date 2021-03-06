// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_DISPATCHER_H
#define IBRIO_DISPATCHER_H

#include "base.h"
#include "peernet.h"

namespace ibrio
{

class CDispatcher : public IDispatcher
{
public:
    CDispatcher();
    ~CDispatcher();
    Errno AddNewBlock(const CBlock& block, uint64 nNonce = 0) override;
    Errno AddNewTx(const CTransaction& tx, uint64 nNonce = 0) override;
    bool AddNewDistribute(const uint256& hashAnchor, const CDestination& dest,
                          const std::vector<unsigned char>& vchDistribute) override;
    bool AddNewPublish(const uint256& hashAnchor, const CDestination& dest,
                       const std::vector<unsigned char>& vchPublish) override;
    void SetConsensus(const CAgreementBlock& agreeBlock) override;
    void CheckAllSubForkLastBlock() override;
    bool FetchAddressUnspent(const uint256& hashFork, const CDestination& dest, std::map<CTxOutPoint, CUnspentOut>& mapUnspent) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    void UpdatePrimaryBlock(const CBlock& block, const CBlockChainUpdate& updateBlockChain, const CTxSetChange& changeTxSet, const uint64& nNonce);
    void ActivateFork(const uint256& hashFork, const uint64& nNonce);
    bool ProcessForkTx(const uint256& txid, const CTransaction& tx);
    void CheckSubForkLastBlock(const uint256& hashFork);

protected:
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    ITxPool* pTxPool;
    IForkManager* pForkManager;
    IConsensus* pConsensus;
    IService* pService;
    IBlockMaker* pBlockMaker;
    network::INetChannel* pNetChannel;
    network::IDelegatedChannel* pDelegatedChannel;
    IDataStat* pDataStat;
    std::string strCmd;
};

} // namespace ibrio

#endif //IBRIO_DISPATCHER_H
