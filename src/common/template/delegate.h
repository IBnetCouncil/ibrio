// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_DELEGATE_H
#define COMMON_TEMPLATE_DELEGATE_H

#include "destination.h"
#include "mint.h"

class CTemplateDelegate : virtual public CTemplateMint
{
public:
    CTemplateDelegate(const ibrio::crypto::CPubKey& keyDelegateIn = ibrio::crypto::CPubKey(),
                      const CDestination& destOwnerIn = CDestination());
    virtual CTemplateDelegate* clone() const;
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const std::vector<uint8>& vchSig,
                                    std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const;
    virtual void GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const;

    bool BuildVssSignature(const uint256& hash, const std::vector<uint8>& vchDelegateSig, std::vector<uint8>& vchVssSig);

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance);
    virtual void BuildTemplateData();
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const;
    virtual bool VerifyBlockSignature(const uint256& hash, const std::vector<uint8>& vchSig) const;

public:
    ibrio::crypto::CPubKey keyDelegate;
    CDestination destOwner;
};

#endif // COMMON_TEMPLATE_DELEGATE_H
