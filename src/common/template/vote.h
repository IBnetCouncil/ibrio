// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_VOTE_H
#define COMMON_TEMPLATE_VOTE_H

#include "destination.h"
#include "mint.h"

class CTemplateVote : virtual public CTemplate, virtual public CSendToRecordedTemplate
{
public:
    CTemplateVote(const CDestination& destDelegateIn = CDestination(),
                  const CDestination& destOwnerIn = CDestination(),
                  const uint32 nCycleIn = 0, const uint64 nNonceIn = 0);
    virtual CTemplateVote* clone() const;
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const std::vector<uint8>& vchSig,
                                    std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const;
    virtual void GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const;

    bool GetDelegateOwnerDestination(CDestination& destDelegateOut, CDestination& destOwnerOut) const;

    static bool ParseDelegateDest(const CDestination& destIn, const CDestination& sendTo, const std::vector<uint8>& vchSigIn,
                                  CDestination& destInDelegateOut, CDestination& sendToDelegateOut);

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance);
    virtual void BuildTemplateData();
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const;

public:
    CDestination destDelegate;
    CDestination destOwner;
    uint32 nCycle;
    uint64 nNonce;
};

#endif // COMMON_TEMPLATE_VOTE_H
