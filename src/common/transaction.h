// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TRANSACTION_H
#define COMMON_TRANSACTION_H

#include <set>
#include <stream/datastream.h>
#include <stream/stream.h>

#include "crypto.h"
#include "destination.h"
#include "uint256.h"

class CTxOutPoint
{
    friend class xengine::CStream;

public:
    uint256 hash;
    uint8 n;

public:
    CTxOutPoint()
    {
        SetNull();
    }
    CTxOutPoint(uint256 hashIn, uint8 nIn)
    {
        hash = hashIn;
        n = nIn;
    }
    virtual ~CTxOutPoint() = default;
    virtual void SetNull()
    {
        hash = 0;
        n = -1;
    }
    virtual bool IsNull() const
    {
        return (hash == 0 && n == decltype(n)(-1));
    }
    friend bool operator<(const CTxOutPoint& a, const CTxOutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }

    friend bool operator==(const CTxOutPoint& a, const CTxOutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const CTxOutPoint& a, const CTxOutPoint& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(hash, opt);
        s.Serialize(n, opt);
    }
};

class CTxIn
{
    friend class xengine::CStream;

public:
    CTxOutPoint prevout;
    CTxIn() {}
    CTxIn(const CTxOutPoint& prevoutIn)
      : prevout(prevoutIn) {}
    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout == b.prevout);
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(prevout, opt);
    }
};

class CTransaction
{
    friend class xengine::CStream;

public:
    uint16 nVersion;
    uint16 nType;
    uint32 nTimeStamp;
    uint32 nLockUntil;
    uint256 hashAnchor;
    std::vector<CTxIn> vInput;
    CDestination sendTo;
    int64 nAmount;
    int64 nTxFee;
    uint8 nDataFormat;
    std::vector<uint8> vchData;
    std::vector<uint8> vchSig;
    enum
    {
        TX_TOKEN = 0x0000,            // normal Tx 0
        TX_CERT = 0xff00,             // Enroll Tx 65280
        TX_GENESIS = 0x0100,          // 256
        TX_STAKE = 0x0200,            // DPoS mint Tx 512
        TX_WORK = 0x0300,             // PoW mint Tx 768
        TX_DEFI_REWARD = 0x0001,      // DeFi reward Tx
        TX_DEFI_RELATION = 0x0002,    // DeFi bind relation Tx
        TX_DEFI_MINT_HEIGHT = 0x0003, // setting DeFi mint height Tx
        TX_INVEST_REWARD = 0x0006     // Invest reward Tx
    };
    CTransaction()
    {
        SetNull();
    }
    virtual ~CTransaction() = default;
    virtual void SetNull()
    {
        nVersion = 1;
        nType = 0;
        nTimeStamp = 0;
        nLockUntil = 0;
        hashAnchor = 0;
        vInput.clear();
        sendTo.SetNull();
        nAmount = 0;
        nTxFee = 0;
        nDataFormat = 0;
        vchData.clear();
        vchSig.clear();
    }
    bool IsNull() const
    {
        return (vInput.empty() && sendTo.IsNull());
    }
    bool IsMintTx() const
    {
        return (nType == TX_GENESIS || nType == TX_STAKE || nType == TX_WORK);
    }
    bool IsRewardTx() const
    {
        return (IsMintTx() || nType == TX_DEFI_REWARD || nType == TX_INVEST_REWARD);
    }
    bool IsDeFiRelation() const
    {
        return (nType == TX_DEFI_RELATION);
    }
    static std::string GetTypeStringStatic(uint16 nTxType)
    {
        if (nTxType == TX_TOKEN)
            return std::string("token");
        else if (nTxType == TX_CERT)
            return std::string("certification");
        else if (nTxType == TX_GENESIS)
            return std::string("genesis");
        else if (nTxType == TX_STAKE)
            return std::string("stake");
        else if (nTxType == TX_WORK)
            return std::string("work");
        else if (nTxType == TX_DEFI_REWARD)
            return std::string("defi-reward");
        else if (nTxType == TX_DEFI_RELATION)
            return std::string("defi-relation");
        else if (nTxType == TX_DEFI_MINT_HEIGHT)
            return std::string("defi-mint-height");
        else if (nTxType == TX_INVEST_REWARD)
            return std::string("invest-reward");
        return std::string("undefined");
    }
    std::string GetTypeString() const
    {
        return GetTypeStringStatic(nType);
    }
    int64 GetTxTime() const
    {
        return ((int64)nTimeStamp);
    }
    uint256 GetHash() const
    {
        xengine::CBufStream ss;
        ss << (*this);

        uint256 hash = ibrio::crypto::CryptoHash(ss.GetData(), ss.GetSize());

        return uint256(nTimeStamp, uint224(hash));
    }
    uint256 GetSignatureHash() const
    {
        xengine::CBufStream ss;
        ss << nVersion << nType << nTimeStamp << nLockUntil << hashAnchor << vInput << sendTo << nAmount << nTxFee << nDataFormat << vchData;
        return ibrio::crypto::CryptoHash(ss.GetData(), ss.GetSize());
    }

    int64 GetChange(int64 nValueIn) const
    {
        return (nValueIn - nAmount - nTxFee);
    }
    uint32 GetLockUntil(const uint32 n = 0) const
    {
        if (n == (nLockUntil >> 31))
        {
            return nLockUntil & 0x7FFFFFFF;
        }
        return 0;
    }
    bool SetLockUntil(const uint32 nHeight, const uint32 n = 0)
    {
        if (nHeight >> 31)
        {
            return false;
        }
        nLockUntil = (n << 31) | nHeight;
        return true;
    }
    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return (a.nVersion == b.nVersion && a.nType == b.nType && a.nTimeStamp == b.nTimeStamp && a.nLockUntil == b.nLockUntil && a.hashAnchor == b.hashAnchor && a.vInput == b.vInput && a.sendTo == b.sendTo && a.nAmount == b.nAmount && a.nTxFee == b.nTxFee && a.nDataFormat == b.nDataFormat && a.vchData == b.vchData && a.vchSig == b.vchSig);
    }
    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(nVersion, opt);
        s.Serialize(nType, opt);
        s.Serialize(nTimeStamp, opt);
        s.Serialize(nLockUntil, opt);
        s.Serialize(hashAnchor, opt);
        s.Serialize(vInput, opt);
        s.Serialize(sendTo, opt);
        s.Serialize(nAmount, opt);
        s.Serialize(nTxFee, opt);
        s.Serialize(nDataFormat, opt);
        s.Serialize(vchData, opt);
        s.Serialize(vchSig, opt);
    }
};

class CTxOut
{
    friend class xengine::CStream;

public:
    CDestination destTo;
    int64 nAmount;
    uint32 nTxTime;
    uint32 nLockUntil;

public:
    CTxOut()
    {
        SetNull();
    }
    CTxOut(const CDestination& destToIn, int64 nAmountIn, uint32 nTxTimeIn, uint32 nLockUntilIn)
      : destTo(destToIn), nAmount(nAmountIn), nTxTime(nTxTimeIn), nLockUntil(nLockUntilIn) {}
    CTxOut(const CTransaction& tx)
      : destTo(tx.sendTo)
    {
        nAmount = tx.nAmount;
        nTxTime = tx.nTimeStamp;
        nLockUntil = tx.nLockUntil;
    }
    CTxOut(const CTransaction& tx, const CDestination& destToIn, int64 nValueIn)
      : destTo(destToIn)
    {
        nAmount = tx.GetChange(nValueIn);
        nTxTime = tx.nTimeStamp;
        nLockUntil = 0;
    }
    void SetNull()
    {
        destTo.SetNull();
        nAmount = 0;
        nTxTime = 0;
        nLockUntil = 0;
    }
    bool IsNull() const
    {
        return (destTo.IsNull() || nAmount <= 0);
    }
    bool IsLocked(int nBlockHeight) const
    {
        return (nBlockHeight < (nLockUntil & 0x7FFFFFFF));
    }
    int64 GetTxTime() const
    {
        return nTxTime;
    }
    std::string ToString() const
    {
        std::ostringstream oss;
        oss << "TxOutput : (" << destTo.ToString() << "," << nAmount << "," << nTxTime << "," << nLockUntil << ")";
        return oss.str();
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(destTo, opt);
        s.Serialize(nAmount, opt);
        s.Serialize(nTxTime, opt);
        s.Serialize(nLockUntil, opt);
    }
};

class CUnspentOut
{
    friend class xengine::CStream;

public:
    int64 nAmount;
    int nTxType;
    uint32 nTxTime;
    uint32 nLockUntil;
    int nHeight;

public:
    CUnspentOut()
    {
        SetNull();
    }
    CUnspentOut(int64 nAmountIn, int nTxTypeIn, uint32 nTxTimeIn, uint32 nLockUntilIn, int nHeightIn)
      : nAmount(nAmountIn), nTxType(nTxTypeIn), nTxTime(nTxTimeIn), nLockUntil(nLockUntilIn), nHeight(nHeightIn) {}
    CUnspentOut(const CTxOut& out, int nTxTypeIn, int nHeightIn)
      : nAmount(out.nAmount), nTxType(nTxTypeIn), nTxTime(out.nTxTime), nLockUntil(out.nLockUntil), nHeight(nHeightIn) {}

    void SetNull()
    {
        nAmount = 0;
        nTxType = -1;
        nTxTime = 0;
        nLockUntil = 0;
        nHeight = -1;
    }
    bool IsNull() const
    {
        return (nAmount <= 0);
    }
    bool IsLocked(int nBlockHeight) const
    {
        return (nBlockHeight < (nLockUntil & 0x7FFFFFFF));
    }
    int64 GetTxTime() const
    {
        return nTxTime;
    }
    friend bool operator==(const CUnspentOut& a, const CUnspentOut& b)
    {
        return (a.nAmount == b.nAmount && a.nTxType == b.nTxType && a.nTxTime == b.nTxTime && a.nLockUntil == b.nLockUntil && a.nHeight == b.nHeight);
    }
    friend bool operator!=(const CUnspentOut& a, const CUnspentOut& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(nAmount, opt);
        s.Serialize(nTxType, opt);
        s.Serialize(nTxTime, opt);
        s.Serialize(nLockUntil, opt);
        s.Serialize(nHeight, opt);
    }
};

class CTxUnspent : public CTxOutPoint
{
public:
    CTxOut output;
    int nTxType;
    int nHeight;

public:
    CTxUnspent()
    {
        SetNull();
    }
    CTxUnspent(const CTxOutPoint& out, const CTxOut& outputIn, int nTxTypeIn = -1, int nHeightIn = -1)
      : CTxOutPoint(out), output(outputIn), nTxType(nTxTypeIn), nHeight(nHeightIn) {}
    virtual ~CTxUnspent() = default;
    void SetNull() override
    {
        CTxOutPoint::SetNull();
        output.SetNull();
        nTxType = -1;
        nHeight = -1;
    }
    bool IsNull() const override
    {
        return (CTxOutPoint::IsNull() || output.IsNull());
    }
};

class CAssembledTx : public CTransaction
{
    friend class xengine::CStream;

public:
    CDestination destIn;
    int64 nValueIn;
    int nBlockHeight;

public:
    CAssembledTx()
    {
        SetNull();
    }
    CAssembledTx(const CTransaction& tx, int nBlockHeightIn, const CDestination& destInIn = CDestination(), int64 nValueInIn = 0)
      : CTransaction(tx), destIn(destInIn), nValueIn(nValueInIn), nBlockHeight(nBlockHeightIn)
    {
    }
    virtual ~CAssembledTx() = default;
    void SetNull() override
    {
        CTransaction::SetNull();
        destIn.SetNull();
        nValueIn = 0;
    }
    int64 GetChange() const
    {
        return (nValueIn - nAmount - nTxFee);
    }
    const CTxOut GetOutput(int n = 0) const
    {
        if (n == 0)
        {
            return CTxOut(sendTo, nAmount, nTimeStamp, GetLockUntil(0));
        }
        else if (n == 1)
        {
            return CTxOut(destIn, GetChange(), nTimeStamp, GetLockUntil(1));
        }
        return CTxOut();
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        CTransaction::Serialize(s, opt);
        s.Serialize(destIn, opt);
        s.Serialize(nValueIn, opt);
        s.Serialize(nBlockHeight, opt);
    }
};

class CTxInContxt
{
    friend class xengine::CStream;

public:
    int64 nAmount;
    uint32 nTxTime;
    uint32 nLockUntil;

public:
    CTxInContxt()
    {
        nAmount = 0;
        nTxTime = 0;
        nLockUntil = 0;
    }
    CTxInContxt(const CTxOut& prevOutput)
    {
        nAmount = prevOutput.nAmount;
        nTxTime = prevOutput.nTxTime;
        nLockUntil = prevOutput.nLockUntil;
    }
    bool IsLocked(int nBlockHeight) const
    {
        return (nBlockHeight < (nLockUntil & 0x7FFFFFFF));
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(nAmount, opt);
        s.Serialize(nTxTime, opt);
        s.Serialize(nLockUntil, opt);
    }
};

class CTxContxt
{
    friend class xengine::CStream;

public:
    CDestination destIn;
    std::vector<CTxInContxt> vin;
    int64 GetValueIn() const
    {
        int64 nValueIn = 0;
        for (std::size_t i = 0; i < vin.size(); i++)
        {
            nValueIn += vin[i].nAmount;
        }
        return nValueIn;
    }
    void SetNull()
    {
        destIn.SetNull();
        vin.clear();
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(destIn, opt);
        s.Serialize(vin, opt);
    }
};

class CTxIndex
{
public:
    int nBlockHeight;
    uint32 nFile;
    uint32 nOffset;

public:
    CTxIndex()
    {
        SetNull();
    }
    CTxIndex(int nBlockHeightIn, uint32 nFileIn, uint32 nOffsetIn)
    {
        nBlockHeight = nBlockHeightIn;
        nFile = nFileIn;
        nOffset = nOffsetIn;
    }
    void SetNull()
    {
        nBlockHeight = 0;
        nFile = 0;
        nOffset = 0;
    }
    bool IsNull() const
    {
        return (nFile == 0);
    };
};

class CTxFilter
{
public:
    std::set<CDestination> setDest;

public:
    CTxFilter(const CDestination& destIn)
    {
        setDest.insert(destIn);
    }
    CTxFilter(const std::set<CDestination>& setDestIn)
      : setDest(setDestIn) {}
    virtual bool FoundTx(const uint256& hashFork, const CAssembledTx& tx) = 0;
};

class CTxId : public uint256
{
public:
    CTxId(const uint256& txid = uint256())
      : uint256(txid) {}
    int64 GetTxTime() const
    {
        return ((int64)Get32(7));
    }
    uint224 GetTxHash() const
    {
        return uint224(*this);
    }
};

class CTxInfo
{
public:
    CTxInfo() {}
    CTxInfo(const uint256& txidIn, const uint256& hashForkIn, const int nTxTypeIn, const uint32 nTimeStampIn,
            const uint32& nLockUntilIn, const int nBlockHeightIn, const uint64 nTxSeqIn, const CDestination& destFromIn, const CDestination& destToIn,
            const int64 nAoumtIn, const int64 nTxFeeIn, const uint64 nSizeIn)
      : txid(txidIn), hashFork(hashForkIn), nTxType(nTxTypeIn), nTimeStamp(nTimeStampIn), nLockUntil(nLockUntilIn),
        nBlockHeight(nBlockHeightIn), nTxSeq(nTxSeqIn), destFrom(destFromIn), destTo(destToIn), nAmount(nAoumtIn), nTxFee(nTxFeeIn), nSize(nSizeIn) {}

    bool IsRewardTx() const
    {
        return (nTxType == CTransaction::TX_GENESIS || nTxType == CTransaction::TX_STAKE || nTxType == CTransaction::TX_WORK
                || nTxType == CTransaction::TX_DEFI_REWARD || nTxType == CTransaction::TX_INVEST_REWARD);
    }

public:
    uint256 txid;
    uint256 hashFork;
    int nTxType;
    uint32 nTimeStamp;
    uint32 nLockUntil;
    int nBlockHeight;
    uint64 nTxSeq;
    CDestination destFrom;
    CDestination destTo;
    int64 nAmount;
    int64 nTxFee;
    uint64 nSize;
};

class CAddrTxIndex
{
    friend class xengine::CStream;

public:
    CDestination dest;
    int64 nHeightSeq;
    uint256 txid;

public:
    CAddrTxIndex() {}
    CAddrTxIndex(const CDestination& destIn, const int64 nHeightSeqIn, const uint256& txidIn)
      : dest(destIn), nHeightSeq(nHeightSeqIn), txid(txidIn) {}
    CAddrTxIndex(const CDestination& destIn, const int nHeightIn, const int nBlockSeqIn, const int nTxSeqIn, const uint256& txidIn)
      : dest(destIn), nHeightSeq(((int64)nHeightIn << 32) | ((nBlockSeqIn << 24) | nTxSeqIn)), txid(txidIn) {}

    int GetHeight() const
    {
        return (int)(nHeightSeq >> 32);
    }
    int GetSeq() const
    {
        return (int)(nHeightSeq & 0xFFFFFFFFL);
    }

    friend bool operator==(const CAddrTxIndex& a, const CAddrTxIndex& b)
    {
        return (a.dest == b.dest && a.nHeightSeq == b.nHeightSeq && a.txid == b.txid);
    }
    friend bool operator!=(const CAddrTxIndex& a, const CAddrTxIndex& b)
    {
        return !(a == b);
    }
    friend bool operator<(const CAddrTxIndex& a, const CAddrTxIndex& b)
    {
        if (a.dest < b.dest)
        {
            return true;
        }
        else if (a.dest == b.dest)
        {
            if (a.nHeightSeq < b.nHeightSeq)
            {
                return true;
            }
            else if (a.nHeightSeq == b.nHeightSeq && a.txid < b.txid)
            {
                return true;
            }
        }
        return false;
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(dest, opt);
        s.Serialize(nHeightSeq, opt);
        s.Serialize(txid, opt);
    }
};

class CAddrTxInfo
{
    friend class xengine::CStream;

public:
    int nDirection;
    CDestination destPeer;
    int nTxType;
    uint32 nTimeStamp;
    uint32 nLockUntil;
    int64 nAmount;
    int64 nTxFee;

    enum
    {
        TXI_DIRECTION_NULL = 0,
        TXI_DIRECTION_FROM = 1,
        TXI_DIRECTION_TO = 2,
        TXI_DIRECTION_TWO = 3
    };

public:
    CAddrTxInfo()
    {
        SetNull();
    }
    CAddrTxInfo(const int nDirectionIn, const CDestination& destPeerIn, const int nTxTypeIn, const uint32 nTimeStampIn,
                const uint32 nLockUntilIn, const int64 nAmountIn, const int64 nTxFeeIn)
      : nDirection(nDirectionIn), destPeer(destPeerIn), nTxType(nTxTypeIn), nTimeStamp(nTimeStampIn),
        nLockUntil(nLockUntilIn), nAmount(nAmountIn), nTxFee(nTxFeeIn)
    {
    }
    CAddrTxInfo(const int nDirectionIn, const CDestination& destPeerIn, const CTransaction& tx)
      : nDirection(nDirectionIn), destPeer(destPeerIn), nTxType(tx.nType), nTimeStamp(tx.nTimeStamp),
        nLockUntil(tx.nLockUntil), nAmount(tx.nAmount), nTxFee(tx.nTxFee)
    {
    }

    void SetNull()
    {
        nDirection = TXI_DIRECTION_NULL;
        destPeer.SetNull();
        nTxType = 0;
        nTimeStamp = 0;
        nLockUntil = 0;
        nAmount = 0;
        nTxFee = 0;
    }
    bool IsNull() const
    {
        return (nDirection == TXI_DIRECTION_NULL);
    };

    friend bool operator==(const CAddrTxInfo& a, const CAddrTxInfo& b)
    {
        return (a.nDirection == b.nDirection && a.destPeer == b.destPeer && a.nTxType == b.nTxType && a.nTimeStamp == b.nTimeStamp
                && a.nLockUntil == b.nLockUntil && a.nAmount == b.nAmount && a.nTxFee == b.nTxFee);
    }
    friend bool operator!=(const CAddrTxInfo& a, const CAddrTxInfo& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(nDirection, opt);
        s.Serialize(destPeer, opt);
        s.Serialize(nTxType, opt);
        s.Serialize(nTimeStamp, opt);
        s.Serialize(nLockUntil, opt);
        s.Serialize(nAmount, opt);
        s.Serialize(nTxFee, opt);
    }
};

class CInvest
{
    friend class xengine::CStream;

public:
    CInvest()
      : nInvestAmount(0), nCycle(0), nStartHeight(0)
    {
    }

public:
    int64 nInvestAmount;
    uint32 nCycle;
    uint32 nStartHeight;
    CDestination destReward;

public:
    friend bool operator==(const CInvest& a, const CInvest& b)
    {
        return (a.nInvestAmount == b.nInvestAmount
                && a.nCycle == b.nCycle
                && a.nStartHeight == b.nStartHeight
                && a.destReward == b.destReward);
    }
    friend bool operator!=(const CInvest& a, const CInvest& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(nInvestAmount, opt);
        s.Serialize(nCycle, opt);
        s.Serialize(nStartHeight, opt);
        s.Serialize(destReward, opt);
    }
};

class CInvestContext
{
    friend class xengine::CStream;

public:
    uint256 hashRef;
    std::map<CDestination, CInvest> mapInvest;
    std::vector<std::pair<CDestination, int64>> vNoPackingReward;

public:
    bool IsFull() const
    {
        return (hashRef == 0);
    }
    void Clear()
    {
        hashRef = 0;
        mapInvest.clear();
        vNoPackingReward.clear();
    }

    friend bool operator==(const CInvestContext& a, const CInvestContext& b)
    {
        return (a.hashRef == b.hashRef
                && a.mapInvest == b.mapInvest
                && a.vNoPackingReward == b.vNoPackingReward);
    }
    friend bool operator!=(const CInvestContext& a, const CInvestContext& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(hashRef, opt);
        s.Serialize(mapInvest, opt);
        s.Serialize(vNoPackingReward, opt);
    }
};

class CActivateContext
{
    friend class xengine::CStream;

public:
    std::map<CDestination, int> mapActivate;

public:
    friend bool operator==(const CActivateContext& a, const CActivateContext& b)
    {
        return (a.mapActivate == b.mapActivate);
    }
    friend bool operator!=(const CActivateContext& a, const CActivateContext& b)
    {
        return !(a == b);
    }

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(mapActivate, opt);
    }
};

#endif //COMMON_TRANSACTION_H
