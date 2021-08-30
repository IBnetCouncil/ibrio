// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_FORK_H
#define COMMON_TEMPLATE_FORK_H

#include "template.h"

class CTemplateFork : virtual public CTemplate, virtual public CLockedCoinTemplate, virtual public CSendToRecordedTemplate
{
public:
    static int64 CreatedCoin();
    static int64 LockedCoin(const int32 nHeight);

public:
    CTemplateFork(const CDestination& destRedeemIn = CDestination(), const uint256& hashForkIn = uint256());
    virtual CTemplateFork* clone() const;
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const std::vector<uint8>& vchSig,
                                    std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const;
    virtual void GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const;
    //virtual int64 LockedCoin(const CDestination& destTo, const int32 nForkHeight) const;
    virtual void GetForkParam(CDestination& destRedeemOut, uint256& hashForkOut);

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance);
    virtual void BuildTemplateData();
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const;

protected:
    CDestination destRedeem;
    uint256 hashFork;
};

#endif // COMMON_TEMPLATE_FORK_H
