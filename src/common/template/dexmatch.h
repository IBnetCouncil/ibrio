// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMON_TEMPLATE_DEXMATCH_H
#define COMMON_TEMPLATE_DEXMATCH_H

#include "template.h"

class CTemplateDexMatch;
typedef boost::shared_ptr<CTemplateDexMatch> CTemplateDexMatchPtr;

class CTemplateDexMatch : virtual public CTemplate, virtual public CSendToRecordedTemplate
{
public:
    CTemplateDexMatch(const CDestination& destMatchIn = CDestination(), const std::vector<char>& vCoinPairIn = std::vector<char>(), uint64 nFinalPriceIn = 0, int64 nMatchAmountIn = 0, int nFeeIn = 0,
                      const uint256& hashSecretIn = uint256(), const std::vector<uint8>& encSecretIn = std::vector<uint8>(),
                      const CDestination& destSellerOrderIn = CDestination(), const CDestination& destSellerIn = CDestination(),
                      const CDestination& destSellerDealIn = CDestination(), int nSellerValidHeightIn = 0,
                      const CDestination& destBuyerIn = CDestination(), const std::vector<char>& vBuyerAmountIn = std::vector<char>(), const uint256& hashBuyerSecretIn = uint256(), int nBuyerValidHeightIn = 0);
    virtual CTemplateDexMatch* clone() const;
    virtual bool GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const std::vector<uint8>& vchSig,
                                    std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const;
    virtual void GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const;

    bool BuildDexMatchTxSignature(const std::vector<uint8>& vchSignExtraData, const std::vector<uint8>& vchPreSig, std::vector<uint8>& vchSig) const;

protected:
    virtual bool ValidateParam() const;
    virtual bool SetTemplateData(const std::vector<uint8>& vchDataIn);
    virtual bool SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance);
    virtual void BuildTemplateData();
    virtual bool VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                   const std::vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const;

public:
    CDestination destMatch;
    std::vector<char> vCoinPair;
    uint64 nFinalPrice;
    int64 nMatchAmount;
    int nFee;

    uint256 hashSecret;
    std::vector<uint8> encSecret;

    CDestination destSellerOrder;
    CDestination destSeller;
    CDestination destSellerDeal;
    int nSellerValidHeight;

    CDestination destBuyer;
    std::vector<char> vBuyerAmount;
    uint256 hashBuyerSecret;
    int nBuyerValidHeight;
};

#endif // COMMON_TEMPLATE_DEXMATCH_H
