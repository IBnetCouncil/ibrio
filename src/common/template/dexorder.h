// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_DEXORDER_H
#define COMMON_TEMPLATE_DEXORDER_H

#include "template.h"

class CTemplateDexOrder : virtual public CTemplate, virtual public CSendToRecordedTemplate
{
public:
    CTemplateDexOrder(const CDestination& destSellerIn = CDestination(), const std::vector<char>& vCoinPairIn = std::vector<char>(),
                      uint64 nPriceIn = 0, int nFeeIn = 0, const std::vector<char>& vRecvDestIn = std::vector<char>(), int nValidHeightIn = 0,
                      const CDestination& destMatchIn = CDestination(), const std::vector<char>& vDealDestIn = std::vector<char>(), uint32 nTimeStampIn = 0);
    virtual CTemplateDexOrder* clone() const;
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const std::vector<uint8>& vchSig,
                                    std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const;
    virtual void GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const;

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance);
    virtual void BuildTemplateData();
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const;

public:
    CDestination destSeller;
    std::vector<char> vCoinPair;
    uint64 nPrice;
    int nFee;
    std::vector<char> vRecvDest;
    int nValidHeight;
    CDestination destMatch;
    std::vector<char> vDealDest;
    uint32 nTimeStamp;
};

#endif // COMMON_TEMPLATE_DEXORDER_H
