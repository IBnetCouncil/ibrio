// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexmatch.h"

#include "destination.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace xengine;

//////////////////////////////
// CTemplateDexMatch

CTemplateDexMatch::CTemplateDexMatch(const CDestination& destMatchIn, const vector<char>& vCoinPairIn, uint64 nFinalPriceIn, int64 nMatchAmountIn, int nFeeIn,
                                     const uint256& hashSecretIn, const vector<uint8>& encSecretIn,
                                     const CDestination& destSellerOrderIn, const CDestination& destSellerIn,
                                     const CDestination& destSellerDealIn, int nSellerValidHeightIn,
                                     const CDestination& destBuyerIn, const vector<char>& vBuyerAmountIn, const uint256& hashBuyerSecretIn, int nBuyerValidHeightIn)
  : CTemplate(TEMPLATE_DEXMATCH),
    destMatch(destMatchIn),
    nFinalPrice(nFinalPriceIn),
    vCoinPair(vCoinPairIn),
    nMatchAmount(nMatchAmountIn),
    nFee(nFeeIn),
    hashSecret(hashSecretIn),
    encSecret(encSecretIn),
    destSellerOrder(destSellerOrderIn),
    destSeller(destSellerIn),
    destSellerDeal(destSellerDealIn),
    nSellerValidHeight(nSellerValidHeightIn),
    destBuyer(destBuyerIn),
    vBuyerAmount(vBuyerAmountIn),
    hashBuyerSecret(hashBuyerSecretIn),
    nBuyerValidHeight(nBuyerValidHeightIn)
{
}

CTemplateDexMatch* CTemplateDexMatch::clone() const
{
    return new CTemplateDexMatch(*this);
}

bool CTemplateDexMatch::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const vector<uint8>& vchSig,
                                           set<CDestination>& setSubDest, vector<uint8>& vchSubSig) const
{
    if (!CTemplate::GetSignDestination(tx, hashFork, nHeight, vchSig, setSubDest, vchSubSig))
    {
        return false;
    }
    setSubDest.clear();
    if (nHeight <= nSellerValidHeight)
    {
        setSubDest.insert(destSellerDeal);
    }
    else
    {
        setSubDest.insert(destSeller);
    }
    return true;
}

void CTemplateDexMatch::GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const
{
    obj.dexmatch.strMatch_Address = (destInstance = destMatch).ToString();
    if (!vCoinPair.empty())
    {
        std::string strCoinPairTemp;
        strCoinPairTemp.assign(&(vCoinPair[0]), vCoinPair.size());
        obj.dexmatch.strCoinpair = strCoinPairTemp;
    }
    obj.dexmatch.dFinal_Price = ((double)nFinalPrice / (double)PRICE_PRECISION);
    obj.dexmatch.dMatch_Amount = ValueFromToken(nMatchAmount);
    obj.dexmatch.dFee = FeeDoubleFromInt64(nFee);

    obj.dexmatch.strSecret_Hash = hashSecret.GetHex();
    obj.dexmatch.strSecret_Enc = ToHexString(encSecret);

    obj.dexmatch.strSeller_Order_Address = (destInstance = destSellerOrder).ToString();
    obj.dexmatch.strSeller_Address = (destInstance = destSeller).ToString();
    obj.dexmatch.strSeller_Deal_Address = (destInstance = destSellerDeal).ToString();

    obj.dexmatch.nSeller_Valid_Height = nSellerValidHeight;

    obj.dexmatch.strBuyer_Address = (destInstance = destBuyer).ToString();

    if (!vBuyerAmount.empty())
    {
        std::string strTemp;
        strTemp.assign(&(vBuyerAmount[0]), vBuyerAmount.size());
        obj.dexmatch.strBuyer_Amount = strTemp;
    }

    obj.dexmatch.strBuyer_Secret_Hash = hashBuyerSecret.GetHex();
    obj.dexmatch.nBuyer_Valid_Height = nBuyerValidHeight;
}

bool CTemplateDexMatch::ValidateParam() const
{
    if (!IsTxSpendable(destMatch))
    {
        return false;
    }
    if (vCoinPair.empty())
    {
        return false;
    }
    if (nFinalPrice == 0)
    {
        return false;
    }
    if (nMatchAmount <= 0 || nMatchAmount > TNS_DEX_MAX_MATCH_TOKEN * COIN)
    {
        return false;
    }
    if (nFee <= 1 || nFee >= FEE_PRECISION)
    {
        return false;
    }
    if (hashSecret == 0)
    {
        return false;
    }
    if (encSecret.empty())
    {
        return false;
    }
    if (!destSellerOrder.IsTemplate() || destSellerOrder.GetTemplateId().GetType() != TEMPLATE_DEXORDER)
    {
        return false;
    }
    if (!IsTxSpendable(destSeller))
    {
        return false;
    }
    if (!IsTxSpendable(destSellerDeal))
    {
        return false;
    }
    if (nSellerValidHeight <= 0)
    {
        return false;
    }
    if (!IsTxSpendable(destBuyer))
    {
        return false;
    }
    if (vBuyerAmount.empty() || vBuyerAmount.size() > MAX_STRING_AMOUNT_LEN)
    {
        return false;
    }
    if (hashBuyerSecret == 0)
    {
        return false;
    }
    if (nBuyerValidHeight <= 0)
    {
        return false;
    }
    return true;
}

bool CTemplateDexMatch::SetTemplateData(const vector<uint8>& vchDataIn)
{
    CIDataStream is(vchDataIn);
    try
    {
        is >> destMatch >> vCoinPair >> nFinalPrice >> nMatchAmount >> nFee >> hashSecret >> encSecret >> destSellerOrder >> destSeller >> destSellerDeal >> nSellerValidHeight >> destBuyer >> vBuyerAmount >> hashBuyerSecret >> nBuyerValidHeight;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplateDexMatch::SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance)
{
    if (obj.strType != GetTypeName(TEMPLATE_DEXMATCH))
    {
        return false;
    }

    if (!destInstance.ParseString(obj.dexmatch.strMatch_Address))
    {
        return false;
    }
    destMatch = destInstance;

    if (obj.dexmatch.strCoinpair.empty())
    {
        return false;
    }
    vCoinPair.assign(obj.dexmatch.strCoinpair.c_str(), obj.dexmatch.strCoinpair.c_str() + obj.dexmatch.strCoinpair.size());

    if (IsDoubleNonPositiveNumber(obj.dexmatch.dFinal_Price))
    {
        return false;
    }
    nFinalPrice = (uint64)(obj.dexmatch.dFinal_Price * PRICE_PRECISION + 0.5);

    if (IsDoubleNonPositiveNumber(obj.dexmatch.dMatch_Amount))
    {
        return false;
    }
    if (obj.dexmatch.dMatch_Amount > TNS_DEX_MAX_MATCH_TOKEN)
    {
        return false;
    }
    nMatchAmount = (int64)(obj.dexmatch.dMatch_Amount * COIN + 0.5);

    int64 nTempFee = FeeInt64FromDouble(obj.dexmatch.dFee);
    if (nTempFee <= 1 || nTempFee >= FEE_PRECISION)
    {
        return false;
    }
    nFee = (int)nTempFee;

    if (hashSecret.SetHex(obj.dexmatch.strSecret_Hash) != obj.dexmatch.strSecret_Hash.size())
    {
        return false;
    }

    encSecret = ParseHexString(obj.dexmatch.strSecret_Enc);

    if (!destInstance.ParseString(obj.dexmatch.strSeller_Order_Address))
    {
        return false;
    }
    destSellerOrder = destInstance;

    if (!destInstance.ParseString(obj.dexmatch.strSeller_Address))
    {
        return false;
    }
    destSeller = destInstance;

    if (!destInstance.ParseString(obj.dexmatch.strSeller_Deal_Address))
    {
        return false;
    }
    destSellerDeal = destInstance;

    nSellerValidHeight = obj.dexmatch.nSeller_Valid_Height;

    if (!destInstance.ParseString(obj.dexmatch.strBuyer_Address))
    {
        return false;
    }
    destBuyer = destInstance;

    if (obj.dexmatch.strBuyer_Amount.empty() || obj.dexmatch.strBuyer_Amount.size() > MAX_STRING_AMOUNT_LEN)
    {
        return false;
    }
    vBuyerAmount.assign(obj.dexmatch.strBuyer_Amount.c_str(), obj.dexmatch.strBuyer_Amount.c_str() + obj.dexmatch.strBuyer_Amount.size());

    if (hashBuyerSecret.SetHex(obj.dexmatch.strBuyer_Secret_Hash) != obj.dexmatch.strBuyer_Secret_Hash.size())
    {
        return false;
    }
    nBuyerValidHeight = obj.dexmatch.nBuyer_Valid_Height;
    return true;
}

void CTemplateDexMatch::BuildTemplateData()
{
    vchData.clear();
    CODataStream os(vchData);
    os << destMatch << vCoinPair << nFinalPrice << nMatchAmount << nFee << hashSecret << encSecret << destSellerOrder << destSeller << destSellerDeal << nSellerValidHeight << destBuyer << vBuyerAmount << hashBuyerSecret << nBuyerValidHeight;
}

bool CTemplateDexMatch::VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                          const vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const
{
    vector<uint8> vchSigSub;
    if (vchSig.size() <= 64)
    {
        vchSigSub = vchSig;
    }
    else
    {
        xengine::CIDataStream ds(vchSig);
        try
        {
            vector<unsigned char> vsm;
            vector<unsigned char> vss;
            ds >> vsm >> vss >> vchSigSub;
        }
        catch (const std::exception& e)
        {
            StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
    }
    if (nForkHeight <= nSellerValidHeight)
    {
        return destSellerDeal.VerifyTxSignature(hash, nType, hashAnchor, destTo, vchSigSub, nForkHeight, fCompleted);
    }
    return destSeller.VerifyTxSignature(hash, nType, hashAnchor, destTo, vchSigSub, nForkHeight, fCompleted);
}

bool CTemplateDexMatch::BuildDexMatchTxSignature(const std::vector<uint8>& vchSignExtraData, const std::vector<uint8>& vchPreSig, std::vector<uint8>& vchSig) const
{
    vector<unsigned char> vsm;
    vector<unsigned char> vss;

    if (!vchSignExtraData.empty())
    {
        xengine::CIDataStream ds(vchSignExtraData);
        try
        {
            ds >> vsm >> vss;
        }
        catch (const std::exception& e)
        {
            StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
    }

    std::vector<uint8_t> vchTempSigData;
    xengine::CODataStream odsStream(vchTempSigData);
    odsStream << vsm << vss << vchPreSig;

    vchSig = vchData;
    vchSig.insert(vchSig.end(), vchTempSigData.begin(), vchTempSigData.end());
    return true;
}
