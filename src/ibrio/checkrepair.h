// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_CHECKREPAIR_H
#define STORAGE_CHECKREPAIR_H

#include "activatedb.h"
#include "address.h"
#include "addressdb.h"
#include "addressunspentdb.h"
#include "block.h"
#include "blockindexdb.h"
#include "core.h"
#include "delegatecomm.h"
#include "delegateverify.h"
#include "investdb.h"
#include "param.h"
#include "struct.h"
#include "template/fork.h"
#include "timeseries.h"
#include "txindexdb.h"
#include "txpooldata.h"
#include "unspentdb.h"
#include "util.h"

using namespace xengine;
using namespace ibrio::storage;
using namespace boost::filesystem;
using namespace std;

namespace ibrio
{

class CCheckTxIndex : public CTxIndex
{
public:
    CCheckTxIndex() {}
    CCheckTxIndex(const int nTxTypeIn, const CTxIndex& txIndexIn)
      : nTxType(nTxTypeIn), CTxIndex(txIndexIn) {}
    CCheckTxIndex(const int nTxTypeIn, const int nBlockHeightIn, const uint32 nFileIn, const uint32 nOffsetIn)
      : nTxType(nTxTypeIn), CTxIndex(nBlockHeightIn, nFileIn, nOffsetIn) {}

public:
    int nTxType;
};

class CCheckTxInfo
{
public:
    CCheckTxInfo() {}
    CCheckTxInfo(const CDestination& destFromIn, const CDestination& destToIn, const int nTxTypeIn, const uint32 nTimeStampIn, const uint32 nLockUntilIn,
                 const int64 nAmountIn, const int64 nTxFeeIn, const int nBlockHeightIn, const int nBlockSeqNoIn, const int nTxSeqNoIn)
      : destFrom(destFromIn), destTo(destToIn), nTxType(nTxTypeIn), nTimeStamp(nTimeStampIn), nLockUntil(nLockUntilIn),
        nAmount(nAmountIn), nTxFee(nTxFeeIn), nBlockHeight(nBlockHeightIn), nBlockSeqNo(nBlockSeqNoIn), nTxSeqNo(nTxSeqNoIn) {}

public:
    CDestination destFrom;
    CDestination destTo;
    int nTxType;
    uint32 nTimeStamp;
    uint32 nLockUntil;
    int64 nAmount;
    int64 nTxFee;
    int nBlockHeight;
    int nBlockSeqNo;
    int nTxSeqNo;
};

class CCheckTxOut : public CTxOut
{
public:
    int nTxType;
    int nHeight;

public:
    CCheckTxOut()
      : nTxType(-1), nHeight(-1) {}
    CCheckTxOut(const CTxOut& txOut, int nTxTypeIn = -1, int nHeightIn = -1)
      : CTxOut(txOut), nTxType(nTxTypeIn), nHeight(nHeightIn) {}

    friend bool operator==(const CCheckTxOut& a, const CCheckTxOut& b)
    {
        return (a.destTo == b.destTo && a.nAmount == b.nAmount && a.nTxTime == b.nTxTime && a.nLockUntil == b.nLockUntil);
    }
    friend bool operator!=(const CCheckTxOut& a, const CCheckTxOut& b)
    {
        return !(a == b);
    }
};

class CCheckForkUnspentWalker : public CForkUnspentDBWalker
{
public:
    CCheckForkUnspentWalker(const map<CTxOutPoint, CCheckTxOut>& mapBlockUnspentIn)
      : mapBlockUnspent(mapBlockUnspentIn) {}

    bool Walk(const CTxOutPoint& txout, const CTxOut& output) override;
    bool CheckForkUnspent();

protected:
    const map<CTxOutPoint, CCheckTxOut>& mapBlockUnspent;
    set<CTxOutPoint> setForkUnspent;

public:
    vector<CTxUnspent> vAddUpdate;
    vector<CTxOutPoint> vRemove;
};

class CCheckAddressUnspentWalker : public CForkAddressUnspentDBWalker
{
public:
    CCheckAddressUnspentWalker(const map<CTxOutPoint, CCheckTxOut>& mapBlockUnspentIn)
      : mapBlockUnspent(mapBlockUnspentIn) {}

    bool Walk(const CAddrUnspentKey& out, const CUnspentOut& unspent) override;
    bool CheckForkAddressUnspent();

protected:
    const map<CTxOutPoint, CCheckTxOut>& mapBlockUnspent;
    set<CTxOutPoint> setForkUnspent;

public:
    vector<pair<CAddrUnspentKey, CUnspentOut>> vAddUpdate;
    vector<CAddrUnspentKey> vRemove;
};

class CCheckAddressWalker : public CForkAddressDBWalker
{
public:
    CCheckAddressWalker(const map<CDestination, pair<uint256, CAddrInfo>>& mapBlockAddressIn)
      : mapBlockAddress(mapBlockAddressIn) {}

    bool Walk(const CDestination& dest, const CAddrInfo& addrInfo) override;
    bool CheckAddress();

protected:
    const map<CDestination, pair<uint256, CAddrInfo>>& mapBlockAddress;
    map<CDestination, CAddrInfo> mapDbAddress;

public:
    vector<pair<CDestination, CAddrInfo>> vAddUpdate;
    vector<CDestination> vRemove;
};

class CCheckAddressTxIndexWalker : public CForkAddressTxIndexDBWalker
{
public:
    CCheckAddressTxIndexWalker(const map<uint256, CCheckTxIndex>& mapBlockTxIndexIn)
      : mapBlockTxIndex(mapBlockTxIndexIn) {}

    bool Walk(const CAddrTxIndex& key, const CAddrTxInfo& value) override;

protected:
    const map<uint256, CCheckTxIndex>& mapBlockTxIndex;

public:
    vector<CAddrTxIndex> vRemove;
};

/////////////////////////////////////////////////////////////////////////
// CCheckForkStatus & CCheckForkManager

class CCheckForkStatus
{
public:
    CCheckForkStatus() {}

    void InsertSubline(int nHeight, const uint256& hashSubline)
    {
        auto it = mapSubline.lower_bound(nHeight);
        while (it != mapSubline.upper_bound(nHeight))
        {
            if (it->second == hashSubline)
            {
                return;
            }
            ++it;
        }
        mapSubline.insert(std::make_pair(nHeight, hashSubline));
    }

public:
    CForkContext ctxt;
    uint256 hashLastBlock;
    std::multimap<int, uint256> mapSubline;
};

class CCheckForkSchedule
{
public:
    CCheckForkSchedule() {}
    void AddNewJoint(const uint256& hashJoint, const uint256& hashFork)
    {
        std::multimap<uint256, uint256>::iterator it = mapJoint.lower_bound(hashJoint);
        while (it != mapJoint.upper_bound(hashJoint))
        {
            if (it->second == hashFork)
            {
                return;
            }
            ++it;
        }
        mapJoint.insert(std::make_pair(hashJoint, hashFork));
    }

public:
    CForkContext ctxtFork;
    std::multimap<uint256, uint256> mapJoint;
};

class CCheckValidFdForkId
{
public:
    CCheckValidFdForkId() {}
    CCheckValidFdForkId(const uint256& hashRefFdBlockIn, const std::map<uint256, int>& mapForkIdIn)
    {
        hashRefFdBlock = hashRefFdBlockIn;
        mapForkId.clear();
        mapForkId.insert(mapForkIdIn.begin(), mapForkIdIn.end());
    }
    int GetCreatedHeight(const uint256& hashFork)
    {
        const auto it = mapForkId.find(hashFork);
        if (it != mapForkId.end())
        {
            return it->second;
        }
        return -1;
    }

public:
    uint256 hashRefFdBlock;
    std::map<uint256, int> mapForkId; // When hashRefFdBlock == 0, it is the total quantity, otherwise it is the increment
};

class CCheckForkManager
{
public:
    CCheckForkManager(const string& strDataPathIn, bool fTestnetIn, bool fOnlyCheckIn, const CProofOfWorkParam& objParamIn)
      : strDataPath(strDataPathIn), fTestnet(fTestnetIn), fOnlyCheck(fOnlyCheckIn), objParam(objParamIn) {}
    ~CCheckForkManager();

    bool Initialize();
    bool FetchForkStatus();

    bool AddBlockForkContext(const CBlockEx& blockex);
    bool VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, vector<pair<CDestination, CForkContext>>& vForkCtxt);
    bool AddForkContext(const uint256& hashPrevBlock, const uint256& hashNewBlock, const vector<pair<CDestination, CForkContext>>& vForkCtxt,
                        bool fCheckPointBlock, uint256& hashRefFdBlock, map<uint256, int>& mapValidFork);
    bool GetForkContext(const uint256& hashFork, CForkContext& ctxt);
    bool VerifyValidFork(const uint256& hashPrevBlock, const uint256& hashFork, const string& strForkName);
    bool GetValidFdForkId(const uint256& hashBlock, map<uint256, int>& mapFdForkIdOut);
    int GetValidForkCreatedHeight(const uint256& hashBlock, const uint256& hashFork);

    bool CheckDbValidFork(const uint256& hashBlock, const uint256& hashRefFdBlock, const map<uint256, int>& mapValidFork);
    bool AddDbValidForkHash(const uint256& hashBlock, const uint256& hashRefFdBlock, const map<uint256, int>& mapValidFork);

    bool GetDbForkContext(const uint256& hashFork, CForkContext& ctxt);
    bool UpdateDbForkContext(const CForkContext& ctxt);

    bool UpdateDbForkLast(const uint256& hashFork, const uint256& hashLastBlock);
    bool RemoveDbForkLast(const uint256& hashFork);

    bool GetValidForkContext(const uint256& hashPrimaryLastBlock, const uint256& hashFork, CForkContext& ctxt);
    bool IsCheckPoint(const uint256& hashFork, const uint256& hashBlock);

public:
    string strDataPath;
    bool fTestnet;
    bool fOnlyCheck;
    const CProofOfWorkParam& objParam;
    CForkDB dbFork;
    map<uint256, uint256> mapActiveFork;
    map<uint256, map<int, uint256>> mapCheckPoints;
    std::map<uint256, CCheckForkSchedule> mapBlockForkSched;
    std::map<uint256, CCheckValidFdForkId> mapBlockValidFork;
};

/////////////////////////////////////////////////////////////////////////
// CCheckDelegateDB

class CCheckDelegateDB : public CDelegateDB
{
public:
    CCheckDelegateDB() {}
    ~CCheckDelegateDB()
    {
        Deinitialize();
    }

public:
    bool CheckDelegate(const uint256& hashBlock);
    bool UpdateDelegate(const uint256& hashBlock, const CBlockEx& block, uint32 nBlockFile, uint32 nBlockOffset);
};

/////////////////////////////////////////////////////////////////////////
// CCheckTsBlock

class CCheckTsBlock : public CTimeSeriesCached
{
public:
    CCheckTsBlock() {}
    ~CCheckTsBlock()
    {
        Deinitialize();
    }
};

/////////////////////////////////////////////////////////////////////////
// CCheckBlockIndexWalker

class CCheckBlockIndexWalker : public CBlockDBWalker
{
public:
    CCheckBlockIndexWalker(const map<uint256, CBlockIndex*>& mapBlockIndexIn)
      : mapBlockIndex(mapBlockIndexIn) {}

    bool Walk(CBlockOutline& outline) override
    {
        if (!mapBlockIndex.count(outline.GetBlockHash()))
        {
            vRemove.push_back(outline.GetBlockHash());
        }
        return true;
    }

protected:
    const map<uint256, CBlockIndex*>& mapBlockIndex;

public:
    vector<uint256> vRemove;
};

/////////////////////////////////////////////////////////////////////////
// CCheckDbTxPool
class CCheckForkTxPool
{
public:
    CCheckForkTxPool(const map<CTxOutPoint, CCheckTxOut>& mapUnspent)
      : mapChainUnspent(mapUnspent) {}

    bool AddTx(const uint256& txid, const CAssembledTx& tx);

protected:
    bool Spent(const CTxOutPoint& point);
    bool Unspent(const CTxOutPoint& point, const CTxOut& out);

protected:
    const map<CTxOutPoint, CCheckTxOut>& mapChainUnspent;
    map<CTxOutPoint, CCheckTxOut> mapTxPoolUnspent;
};

class CCheckBlockFork;

class CCheckTxPoolData
{
public:
    CCheckTxPoolData(const map<uint256, CCheckBlockFork>& mapCheckForkIn)
      : mapLinkCheckFork(mapCheckForkIn) {}

    bool CheckTxPool(const string& strPath, const bool fOnlyCheck);

protected:
    const map<uint256, CCheckBlockFork>& mapLinkCheckFork;

public:
    map<uint256, CCheckForkTxPool> mapForkTxPool;
};

/////////////////////////////////////////////////////////////////////////
// CCheckBlockFork

class CCheckBlockFork
{
    enum
    {
        MAX_CACHE_BLOCK_COUNT = 1000,
        MAX_CACHE_BLOCK_TXINFO_COUNT = 200000,
        DEF_CHECK_HEIGHT_DEPTH = 120,
        DPOS_CONFIRM_HEIGHT = 6
    };

public:
    CCheckBlockFork(const string& strPathIn, const bool fOnlyCheckIn, const bool fAddrTxIndexIn, CCheckTsBlock& tsBlockIn,
                    CCheckForkManager& objForkManagerIn, CAddressTxIndexDB& dbAddressTxIndexIn)
      : pOrigin(nullptr), pLast(nullptr), fInvalidFork(false), strDataPath(strPathIn), fOnlyCheck(fOnlyCheckIn), fCheckAddrTxIndex(fAddrTxIndexIn),
        tsBlock(tsBlockIn), objForkManager(objForkManagerIn), dbAddressTxIndex(dbAddressTxIndexIn), nCacheTxInfoBlockCount(0), nMintHeight(-2) {}

    bool AddForkBlock(const CBlockEx& block, CBlockIndex* pBlockIndex);
    CBlockIndex* GetBranch(CBlockIndex* pIndexRef, CBlockIndex* pIndex, vector<CBlockIndex*>& vPath);
    bool AddBlockData(const CBlockEx& block, const CBlockIndex* pBlockIndex);
    bool RemoveBlockData(const CBlockEx& block, const CBlockIndex* pBlockIndex);

    bool AddBlockTx(const CTransaction& txIn, const CTxContxt& contxtIn, const int nHeight, const int nBlockSeqNo, const int nTxSeqNo, const uint256& hashAtForkIn, uint32 nFileNoIn, uint32 nOffsetIn);
    bool RemoveBlockTx(const CTransaction& txIn, const CTxContxt& contxtIn);
    bool AddBlockUnspent(const CTxOutPoint& txPoint, const CTxOut& txOut, int nTxType, int nHeight);
    bool AddBlockSpent(const CTxOutPoint& txPoint);

    bool InheritCopyData(const CCheckBlockFork& fromParent, const CBlockIndex* pJointBlockIndex);
    bool CheckForkAddressTxIndex(const uint256& hashFork, const int nCheckHeight);

public:
    string strDataPath;
    bool fOnlyCheck;
    bool fCheckAddrTxIndex;
    CCheckTsBlock& tsBlock;
    CCheckForkManager& objForkManager;
    CAddressTxIndexDB& dbAddressTxIndex;
    CBlockIndex* pOrigin;
    CBlockIndex* pLast;
    bool fInvalidFork;
    map<uint256, CCheckTxIndex> mapParentForkBlockTxIndex;
    map<uint256, CCheckTxIndex> mapBlockTxIndex;
    map<uint256, CCheckTxInfo> mapBlockTxInfo;
    map<CTxOutPoint, CCheckTxOut> mapBlockUnspent;
    map<CDestination, pair<uint256, CAddrInfo>> mapBlockAddress;
    uint64 nCacheTxInfoBlockCount;
    int32 nMintHeight;
    uint256 txidMintHeight;
};

/////////////////////////////////////////////////////////////////////////
// CCheckBlockWalker
class CCheckBlockWalker : public CTSWalker<CBlockEx>
{
public:
    CCheckBlockWalker(const bool fTestnetIn, const bool fOnlyCheckIn, const bool fAddrTxIndexIn, const int64& nMaxBlockRewardTxCountIn, CCheckForkManager& objForkManagerIn)
      : nBlockCount(0), nMainChainHeight(0), objProofParam(fTestnetIn), fOnlyCheck(fOnlyCheckIn),
        fCheckAddrTxIndex(fAddrTxIndexIn), nMaxBlockRewardTxCount(nMaxBlockRewardTxCountIn), objForkManager(objForkManagerIn), dbInvest(false), dbActivate(false), nBlockPrevTime(0) {}
    ~CCheckBlockWalker();

    bool Initialize(const string& strPath);
    void Uninitialize();

    bool Walk(const CBlockEx& block, uint32 nFile, uint32 nOffset) override;

    bool InheritForkData(const CBlockEx& blockOrigin, CCheckBlockFork& subBlockFork);
    CBlockIndex* AddBlockIndex(const uint256& hashBlock, const CBlockEx& block, CBlockIndex* pIndexPrev, uint32 nFile, uint32 nOffset);
    bool GetBlockTrust(const CBlockEx& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev = nullptr, const CDelegateAgreement& agreement = CDelegateAgreement(), const CBlockIndex* pIndexRef = nullptr, std::size_t nEnrollTrust = 0);
    bool GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits);
    bool GetBlockDelegateAgreement(const uint256& hashBlock, const CBlock& block, CBlockIndex* pIndexPrev, CDelegateAgreement& agreement, size_t& nEnrollTrust);
    bool GetBlockDelegateEnrolled(const uint256& hashBlock, CBlockIndex* pIndex, CDelegateEnrolled& enrolled);
    bool RetrieveAvailDelegate(const uint256& hash, int height, const vector<uint256>& vBlockRange, int64 nMinEnrollAmount,
                               map<CDestination, size_t>& mapWeight, map<CDestination, vector<unsigned char>>& mapEnrollData,
                               vector<pair<CDestination, int64>>& vecAmount);

    bool UpdateBlockNext();
    bool CheckRepairFork();
    CBlockIndex* AddNewIndex(const uint256& hash, const CBlock& block, uint32 nFile, uint32 nOffset, uint256 nChainTrust);
    CBlockIndex* AddNewIndex(const uint256& hash, const CBlockOutline& objBlockOutline);
    void ClearBlockIndex();
    bool CheckBlockIndex();
    bool CheckRefBlock();
    bool CheckSurplusAddressTxIndex(uint64& nTxIndexCount);
    bool UpdateInvest(const uint256& hashBlock, const CBlockEx& block);
    bool UpdateActivate(const uint256& hashBlock, const CBlockEx& block);

public:
    bool fOnlyCheck;
    bool fCheckAddrTxIndex;
    string strDataPath;
    int64 nBlockCount;
    uint32 nMainChainHeight;
    uint256 hashGenesis;
    int64 nMaxBlockRewardTxCount;
    CProofOfWorkParam objProofParam;
    CCheckForkManager& objForkManager;
    map<uint256, CCheckBlockFork> mapCheckFork;
    map<uint256, CBlockIndex*> mapBlockIndex;
    map<uint256, uint256> mapRefBlock;
    CBlockIndexDB dbBlockIndex;
    CCheckDelegateDB objDelegateDB;
    CCheckTsBlock objTsBlock;
    CAddressTxIndexDB dbAddressTxIndex;
    CInvestDB dbInvest;
    CActivateDB dbActivate;
    std::map<uint256, CInvestContext> mapCacheFullInvest;
    int64 nBlockPrevTime;
};

/////////////////////////////////////////////////////////////////////////
// CCheckRepairData

class CCheckRepairData
{
public:
    CCheckRepairData(const string& strPath, const bool fTestnetIn, const bool fOnlyCheckIn, const bool fAddrTxIndexIn, const int64& nMaxBlockRewardTxCountIn)
      : strDataPath(strPath), fTestnet(fTestnetIn), fOnlyCheck(fOnlyCheckIn),
        objProofOfWorkParam(fTestnetIn), objForkManager(strPath, fTestnetIn, fOnlyCheckIn, objProofOfWorkParam),
        objBlockWalker(fTestnetIn, fOnlyCheckIn, fAddrTxIndexIn, nMaxBlockRewardTxCountIn, objForkManager)
    {
    }

protected:
    bool FetchBlockData();
    bool CheckRepairUnspent(uint64& nUnspentCount);
    bool CheckRepairAddressUnspent();
    bool CheckRepairAddress(uint64& nAddressCount);
    bool CheckTxIndex(uint64& nTxIndexCount);

public:
    bool CheckRepairData();

protected:
    string strDataPath;
    bool fTestnet;
    bool fOnlyCheck;

    CProofOfWorkParam objProofOfWorkParam;
    CCheckForkManager objForkManager;
    CCheckBlockWalker objBlockWalker;
};

} // namespace ibrio

#endif //STORAGE_CHECKREPAIR_H
