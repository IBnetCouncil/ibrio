// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_MULTISIG_H
#define COMMON_TEMPLATE_MULTISIG_H

#include "weighted.h"

class CTemplateMultiSig : virtual public CTemplateWeighted
{
public:
    CTemplateMultiSig(const uint8 nRequiredIn = 0, const std::map<ibrio::crypto::CPubKey, uint8>& mapPubKeyWeightIn = std::map<ibrio::crypto::CPubKey, uint8>());
    virtual CTemplateMultiSig* clone() const;
    virtual void GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const;

protected:
    virtual bool SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance);
};

#endif // COMMON_TEMPLATE_MULTISIG_H
