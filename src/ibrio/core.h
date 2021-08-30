// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_CORE_H
#define IBRIO_CORE_H

#include "base.h"

namespace ibrio
{

class CCoreProtocol : public ICoreProtocol
{
public:
    CCoreProtocol();
    virtual ~CCoreProtocol();
    virtual void InitializeGenesisBlock() override;
    virtual const uint256& GetGenesisBlockHash() override;
    virtual void GetGenesisBlock(CBlock& block) override;
    virtual Errno ValidateTransaction(const CTransaction& tx, int nHeight) override;
    virtual Errno ValidateBlock(const CBlock& block) override;
    virtual Errno VerifyForkTx(const CTransaction& tx, const CDestination& destIn, const uint256& hashFork, const int nHeight) override;
    virtual Errno VerifyForkRedeem(const CTransaction& tx, const CDestination& destIn, const uint256& hashFork,
                                   const uint256& hashPrevBlock, const vector<uint8>& vchSubSig, const int64 nValueIn) override;
    virtual Errno ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) override;

    virtual Errno VerifyBlock(const CBlock& block, CBlockIndex* pIndexPrev) override;
    virtual Errno VerifyBlockTx(const CTransaction& tx, const CTxContxt& txContxt, CBlockIndex* pIndexPrev, int nBlockHeight, const uint256& fork, const CProfile& profile) override;
    virtual Errno VerifyTransaction(const CTransaction& tx, const std::vector<CTxOut>& vPrevOutput, int nForkHeight, const uint256& fork, const CProfile& profile) override;
    virtual Errno VerifyMintHeightTx(const CTransaction& tx, const CDestination& destIn, const uint256& hashFork, const int nHeight, const CProfile& profile) override;

    virtual Errno VerifyProofOfWork(const CBlock& block, const CBlockIndex* pIndexPrev) override;
    virtual Errno VerifyDelegatedProofOfStake(const CBlock& block, const CBlockIndex* pIndexPrev,
                                              const CDelegateAgreement& agreement) override;
    virtual Errno VerifySubsidiary(const CBlock& block, const CBlockIndex* pIndexPrev, const CBlockIndex* pIndexRef,
                                   const CDelegateAgreement& agreement) override;
    virtual bool GetBlockTrust(const CBlock& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev = nullptr, const CDelegateAgreement& agreement = CDelegateAgreement(), const CBlockIndex* pIndexRef = nullptr, std::size_t nEnrollTrust = 0) override;
    virtual bool GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits, int64& nReward) override;
    virtual void GetDelegatedBallot(const uint256& nAgreement, const std::size_t nWeight, const std::map<CDestination, size_t>& mapBallot,
                                    const std::vector<std::pair<CDestination, int64>>& vecAmount, int64 nMoneySupply, std::vector<CDestination>& vBallot, std::size_t& nEnrollTrust, int nBlockHeight) override;
    virtual int64 MinEnrollAmount() override;
    virtual uint32 DPoSTimestamp(const CBlockIndex* pIndexPrev) override;
    virtual uint32 GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType) override;
    virtual const std::set<CDestination> GetDeFiBlacklist(const uint256& hashFork, const int32 nHeight) override;
    virtual bool VerifyTxDestActivate(const uint256& hashPrev, const CTransaction& tx) override;
    virtual uint256 CreateActivateSignHash(const uint256& hashFork, const CDestination& destActivate) override;

protected:
    bool HandleInitialize() override;
    Errno Debug(const Errno& err, const char* pszFunc, const char* pszFormat, ...);
    bool CheckBlockSignature(const CBlock& block);
    Errno ValidateVacantBlock(const CBlock& block);
    Errno VerifyCertTx(const CTransaction& tx, const CDestination& destIn, const uint256& fork);
    Errno VerifyVoteTx(const CTransaction& tx, const CDestination& destIn, const uint256& hashPrev, const uint256& fork, const bool fBlockTx);
    Errno VerifyDexOrderTx(const CTransaction& tx, const CDestination& destIn, int64 nValueIn, int nHeight);
    Errno VerifyDexMatchTx(const CTransaction& tx, int64 nValueIn, int nHeight);
    Errno VerifyDeFiRelationTx(const CTransaction& tx, const CDestination& destIn, int nHeight, const uint256& fork);
    Errno VerifyActivateTx(const CTransaction& tx, const CDestination& destIn, int nHeight, const uint256& fork);

protected:
    uint256 hashGenesisBlock;
    IBlockChain* pBlockChain;
    IForkManager* pForkManager;
};

class CTestNetCoreProtocol : public CCoreProtocol
{
public:
    CTestNetCoreProtocol();
    void GetGenesisBlock(CBlock& block) override;
};

class CProofOfWorkParam
{
public:
    CProofOfWorkParam(const bool fTestnetIn);
    ~CProofOfWorkParam();

public:
    bool fTestnet;
    int64 nDelegateProofOfStakeEnrollMinimumAmount;
    int64 nDelegateProofOfStakeEnrollMaximumAmount;
    uint256 hashGenesisBlock;

protected:
    ICoreProtocol* pCoreProtocol;

public:
    uint32 GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType);
    Errno ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) const;
};

} // namespace ibrio

#endif //IBRIO_BASE_H
