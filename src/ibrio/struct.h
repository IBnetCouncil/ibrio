/// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_STRUCT_H
#define IBRIO_STRUCT_H

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <map>
#include <set>
#include <vector>

#include "block.h"
#include "proto.h"
#include "transaction.h"
#include "uint256.h"

namespace ibrio
{

inline int64 CalcMinTxFee(const uint32 nVchData, const uint32 nMinFee)
{
    if (0 == nVchData)
    {
        return nMinFee;
    }
    uint32_t multiplier = nVchData / 200;
    if (nVchData % 200 > 0)
    {
        multiplier++;
    }
    if (multiplier > 5)
    {
        return nMinFee + nMinFee * 10 + (multiplier - 5) * nMinFee * 4;
    }
    else
    {
        return nMinFee + multiplier * nMinFee * 2;
    }
}

// Status
class CBlockStatus
{
public:
    CBlockStatus()
      : nBlockTime(0), nBlockHeight(-1), nMoneySupply(0), nMoneyDestroy(0), nMintType(-1) {}

public:
    uint256 hashPrevBlock;
    uint256 hashBlock;
    int64 nBlockTime;
    int nBlockHeight;
    int64 nMoneySupply;
    int64 nMoneyDestroy;
    uint16 nMintType;
};

class CForkStatus
{
public:
    CForkStatus() {}
    CForkStatus(const uint256& hashOriginIn, const uint256& hashParentIn, int nOriginHeightIn, int nForkTypeIn)
      : hashOrigin(hashOriginIn),
        hashParent(hashParentIn),
        nOriginHeight(nOriginHeightIn),
        nForkType(nForkTypeIn),
        nLastBlockTime(0),
        nLastBlockHeight(-1),
        nMoneySupply(0),
        nMoneyDestroy(0),
        nMintType(-1)
    {
    }

    bool IsNull()
    {
        return nLastBlockTime == 0;
    }

public:
    uint256 hashOrigin;
    uint256 hashParent;
    int nOriginHeight;
    int nForkType;

    uint256 hashPrevBlock;
    uint256 hashLastBlock;
    int64 nLastBlockTime;
    int nLastBlockHeight;
    int64 nMoneySupply;
    int64 nMoneyDestroy;
    uint16 nMintType;
    std::multimap<int, uint256> mapSubline;
};

class CWalletBalance
{
public:
    int64 nAvailable;
    int64 nLocked;
    int64 nUnconfirmed;

public:
    void SetNull()
    {
        nAvailable = 0;
        nLocked = 0;
        nUnconfirmed = 0;
    }
};

// Notify
class CBlockChainUpdate
{
public:
    CBlockChainUpdate()
    {
        SetNull();
    }
    CBlockChainUpdate(CBlockIndex* pIndex, int nForkTypeIn)
    {
        hashFork = pIndex->GetOriginHash();
        hashParent = pIndex->GetParentHash();
        nOriginHeight = pIndex->pOrigin->GetBlockHeight() - 1;
        hashPrevBlock = pIndex->GetPrevHash();
        hashLastBlock = pIndex->GetBlockHash();
        nLastBlockTime = pIndex->GetBlockTime();
        nLastBlockHeight = pIndex->GetBlockHeight();
        nLastMintType = pIndex->nMintType;
        nMoneySupply = pIndex->GetMoneySupply();
        nMoneyDestroy = pIndex->GetMoneyDestroy();
        nForkType = nForkTypeIn;
    }
    void SetNull()
    {
        hashFork = 0;
        nOriginHeight = -1;
        nLastBlockHeight = -1;
        nLastMintType = 0;
        nForkType = -1;
    }
    bool IsNull() const
    {
        return (hashFork == 0);
    }

public:
    uint256 hashFork;
    uint256 hashParent;
    int nOriginHeight;
    int nForkType;
    uint256 hashPrevBlock;
    uint256 hashLastBlock;
    int64 nLastBlockTime;
    int nLastBlockHeight;
    uint16 nLastMintType;
    int64 nMoneySupply;
    int64 nMoneyDestroy;
    std::set<uint256> setTxUpdate;
    std::vector<CBlockEx> vBlockAddNew;
    std::vector<CBlockEx> vBlockRemove;
};

class CTxSetChange
{
public:
    uint256 hashFork;
    std::map<uint256, int> mapTxUpdate;
    std::vector<CAssembledTx> vTxAddNew;
    std::vector<std::pair<uint256, std::vector<CTxIn>>> vTxRemove;
};

class CNetworkPeerUpdate
{
public:
    bool fActive;
    uint64 nPeerNonce;
    network::CAddress addrPeer;
};

class CTransactionUpdate
{
public:
    CTransactionUpdate()
    {
        SetNull();
    }
    void SetNull()
    {
        hashFork = 0;
    }
    bool IsNull() const
    {
        return (hashFork == 0);
    }

public:
    uint256 hashFork;
    int64 nChange;
    CTransaction txUpdate;
};

class CDelegateRoutine
{
public:
    std::vector<std::pair<uint256, std::map<CDestination, size_t>>> vEnrolledWeight;

    std::vector<CTransaction> vEnrollTx;
    std::vector<std::pair<uint256, std::map<CDestination, std::vector<unsigned char>>>> vDistributeData;
    std::map<CDestination, std::vector<unsigned char>> mapPublishData;
    uint256 hashDistributeOfPublish;
    bool fPublishCompleted;

public:
    CDelegateRoutine()
      : fPublishCompleted(false) {}
};

class CDelegateEnrolled
{
public:
    CDelegateEnrolled() {}
    void Clear()
    {
        mapWeight.clear();
        mapEnrollData.clear();
        vecAmount.clear();
    }

public:
    std::map<CDestination, std::size_t> mapWeight;
    std::map<CDestination, std::vector<unsigned char>> mapEnrollData;
    std::vector<std::pair<CDestination, int64>> vecAmount;
};

class CDelegateAgreement
{
public:
    CDelegateAgreement()
    {
        nAgreement = 0;
        nWeight = 0;
    }
    bool IsProofOfWork() const
    {
        return (vBallot.empty());
    }
    const CDestination GetBallot(int nIndex) const
    {
        if (vBallot.empty())
        {
            return CDestination();
        }
        return vBallot[nIndex % vBallot.size()];
    }
    bool operator==(const CDelegateAgreement& other)
    {
        return (nAgreement == other.nAgreement && nWeight == other.nWeight);
    }
    void Clear()
    {
        nAgreement = 0;
        nWeight = 0;
        vBallot.clear();
    }

public:
    uint256 nAgreement;
    std::size_t nWeight;
    std::vector<CDestination> vBallot;
};

class CAgreementBlock
{
public:
    CAgreementBlock()
      : nPrevTime(0), nPrevHeight(0), nPrevMintType(0), nWaitTime(0), fCompleted(false), ret(false) {}

    uint256 hashPrev;
    int64 nPrevTime;
    int nPrevHeight;
    uint16 nPrevMintType;
    CDelegateAgreement agreement;
    int64 nWaitTime;
    bool fCompleted;
    bool ret;
};

/* Protocol & Event */
class CNil
{
    friend class xengine::CStream;

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
    }
};

class CBlockMakerUpdate
{
public:
    uint256 hashParent;
    int nOriginHeight;

    uint256 hashPrevBlock;
    uint256 hashBlock;
    int64 nBlockTime;
    int nBlockHeight;
    uint256 nAgreement;
    std::size_t nWeight;
    uint16 nMintType;
};

/* Net Channel */
class CPeerKnownTx
{
public:
    CPeerKnownTx() {}
    CPeerKnownTx(const uint256& txidIn)
      : txid(txidIn), nTime(xengine::GetTime()) {}

public:
    uint256 txid;
    int64 nTime;
};

typedef boost::multi_index_container<
    CPeerKnownTx,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::member<CPeerKnownTx, uint256, &CPeerKnownTx::txid>>,
        boost::multi_index::ordered_non_unique<boost::multi_index::member<CPeerKnownTx, int64, &CPeerKnownTx::nTime>>>>
    CPeerKnownTxSet;

typedef CPeerKnownTxSet::nth_index<0>::type CPeerKnownTxSetById;
typedef CPeerKnownTxSet::nth_index<1>::type CPeerKnownTxSetByTime;

typedef boost::multi_index_container<
    uint256,
    boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::ordered_unique<boost::multi_index::identity<uint256>>>>
    CUInt256List;
typedef CUInt256List::nth_index<1>::type CUInt256ByValue;

/* CStatItemBlockMaker & CStatItemP2pSyn */
class CStatItemBlockMaker
{
public:
    CStatItemBlockMaker()
      : nTimeValue(0), nPOWBlockCount(0), nDPOSBlockCount(0), nBlockTPS(0), nTxTPS(0) {}

    uint32 nTimeValue;

    uint64 nPOWBlockCount;
    uint64 nDPOSBlockCount;
    uint64 nBlockTPS;
    uint64 nTxTPS;
};

class CStatItemP2pSyn
{
public:
    CStatItemP2pSyn()
      : nRecvBlockCount(0), nRecvTxTPS(0), nSendBlockCount(0), nSendTxTPS(0), nSynRecvTxTPS(0), nSynSendTxTPS(0) {}

    uint32 nTimeValue;

    uint64 nRecvBlockCount;
    uint64 nRecvTxTPS;
    uint64 nSendBlockCount;
    uint64 nSendTxTPS;

    uint64 nSynRecvTxTPS;
    uint64 nSynSendTxTPS;
};

struct CDeFiReward
{
    CDestination dest;
    int64 nReward;
    int64 nAmount;
    uint64 nRank;
    int64 nStakeReward;
    int64 nAchievement; // all children amount
    uint64 nPower;
    int64 nPromotionReward;
    uint256 hashAnchor;

    CDeFiReward()
      : nReward(0), nAmount(0), nRank(0), nStakeReward(0), nAchievement(0), nPower(0), nPromotionReward(0)
    {
    }
};

typedef boost::multi_index_container<
    CDeFiReward,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::member<CDeFiReward, CDestination, &CDeFiReward::dest>>,
        boost::multi_index::ordered_non_unique<boost::multi_index::member<CDeFiReward, int64, &CDeFiReward::nReward>, std::greater<int64>>>>
    CDeFiRewardSet;

typedef CDeFiRewardSet::nth_index<0>::type CDeFiRewardSetByDest;
typedef CDeFiRewardSet::nth_index<1>::type CDeFiRewardSetByReward;

} // namespace ibrio

#endif // IBRIO_STRUCT_H
