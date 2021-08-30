// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dexorder.h"

#include "destination.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace xengine;

//////////////////////////////
// CTemplateDexOrder

CTemplateDexOrder::CTemplateDexOrder(const CDestination& destSellerIn, const vector<char>& vCoinPairIn,
                                     uint64 nPriceIn, int nFeeIn, const vector<char>& vRecvDestIn, int nValidHeightIn,
                                     const CDestination& destMatchIn, const vector<char>& vDealDestIn, uint32 nTimeStampIn)
  : CTemplate(TEMPLATE_DEXORDER),
    destSeller(destSellerIn),
    vCoinPair(vCoinPairIn),
    nPrice(nPriceIn),
    nFee(nFeeIn),
    vRecvDest(vRecvDestIn),
    nValidHeight(nValidHeightIn),
    destMatch(destMatchIn),
    vDealDest(vDealDestIn),
    nTimeStamp(nTimeStampIn)
{
}

CTemplateDexOrder* CTemplateDexOrder::clone() const
{
    return new CTemplateDexOrder(*this);
}

bool CTemplateDexOrder::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const vector<uint8>& vchSig,
                                           set<CDestination>& setSubDest, vector<uint8>& vchSubSig) const
{
    if (!CTemplate::GetSignDestination(tx, hashFork, nHeight, vchSig, setSubDest, vchSubSig))
    {
        return false;
    }
    setSubDest.clear();
    if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_DEXMATCH)
    {
        setSubDest.insert(destMatch);
    }
    else
    {
        setSubDest.insert(destSeller);
    }
    return true;
}

void CTemplateDexOrder::GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const
{
    obj.dexorder.strSeller_Address = (destInstance = destSeller).ToString();
    if (!vCoinPair.empty())
    {
        std::string strCoinPairTemp;
        strCoinPairTemp.assign(&(vCoinPair[0]), vCoinPair.size());
        obj.dexorder.strCoinpair = strCoinPairTemp;
    }
    obj.dexorder.dPrice = ((double)nPrice / (double)PRICE_PRECISION);
    obj.dexorder.dFee = FeeDoubleFromInt64(nFee);

    if (!vRecvDest.empty())
    {
        std::string strRecvDestTemp;
        strRecvDestTemp.assign(&(vRecvDest[0]), vRecvDest.size());
        obj.dexorder.strRecv_Address = strRecvDestTemp;
    }

    obj.dexorder.nValid_Height = nValidHeight;
    obj.dexorder.strMatch_Address = (destInstance = destMatch).ToString();

    if (!vDealDest.empty())
    {
        std::string strDestTemp;
        strDestTemp.assign(&(vDealDest[0]), vDealDest.size());
        obj.dexorder.strDeal_Address = strDestTemp;
    }

    obj.dexorder.nTimestamp = nTimeStamp;
}

bool CTemplateDexOrder::ValidateParam() const
{
    if (!IsTxSpendable(destSeller))
    {
        return false;
    }
    if (vCoinPair.empty())
    {
        return false;
    }
    if (nPrice == 0)
    {
        return false;
    }
    if (nFee <= 1 || nFee >= FEE_PRECISION)
    {
        return false;
    }
    if (vRecvDest.empty() || vRecvDest.size() > MAX_STRING_ADDRESS_LEN)
    {
        return false;
    }
    if (nValidHeight <= 0)
    {
        return false;
    }
    if (!IsTxSpendable(destMatch))
    {
        return false;
    }
    if (vDealDest.empty() || vDealDest.size() > MAX_STRING_ADDRESS_LEN)
    {
        return false;
    }
    return true;
}

bool CTemplateDexOrder::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    CIDataStream is(vchDataIn);
    try
    {
        is >> destSeller >> vCoinPair >> nPrice >> nFee >> vRecvDest >> nValidHeight >> destMatch >> vDealDest >> nTimeStamp;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }
    return true;
}

bool CTemplateDexOrder::SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance)
{
    if (obj.strType != GetTypeName(TEMPLATE_DEXORDER))
    {
        return false;
    }

    if (!destInstance.ParseString(obj.dexorder.strSeller_Address))
    {
        return false;
    }
    destSeller = destInstance;

    if (obj.dexorder.strCoinpair.empty())
    {
        return false;
    }
    vCoinPair.assign(obj.dexorder.strCoinpair.c_str(), obj.dexorder.strCoinpair.c_str() + obj.dexorder.strCoinpair.size());

    if (IsDoubleNonPositiveNumber(obj.dexorder.dPrice))
    {
        return false;
    }
    nPrice = (uint64)(obj.dexorder.dPrice * PRICE_PRECISION + 0.5);

    int64 nTempFee = FeeInt64FromDouble(obj.dexorder.dFee);
    if (nTempFee <= 1 || nTempFee >= FEE_PRECISION)
    {
        return false;
    }
    nFee = (int)nTempFee;

    if (obj.dexorder.strRecv_Address.empty() || obj.dexorder.strRecv_Address.size() > MAX_STRING_ADDRESS_LEN)
    {
        return false;
    }
    vRecvDest.assign(obj.dexorder.strRecv_Address.c_str(), obj.dexorder.strRecv_Address.c_str() + obj.dexorder.strRecv_Address.size());

    nValidHeight = obj.dexorder.nValid_Height;

    if (!destInstance.ParseString(obj.dexorder.strMatch_Address))
    {
        return false;
    }
    destMatch = destInstance;

    if (obj.dexorder.strDeal_Address.empty() || obj.dexorder.strDeal_Address.size() > MAX_STRING_ADDRESS_LEN)
    {
        return false;
    }
    vDealDest.assign(obj.dexorder.strDeal_Address.c_str(), obj.dexorder.strDeal_Address.c_str() + obj.dexorder.strDeal_Address.size());

    nTimeStamp = obj.dexorder.nTimestamp;
    return true;
}

void CTemplateDexOrder::BuildTemplateData()
{
    vchData.clear();
    CODataStream os(vchData);
    os << destSeller << vCoinPair << nPrice << nFee << vRecvDest << nValidHeight << destMatch << vDealDest << nTimeStamp;
}

bool CTemplateDexOrder::VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                          const vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const
{
    if (destTo.IsTemplate() && destTo.GetTemplateId().GetType() == TEMPLATE_DEXMATCH)
    {
        return destMatch.VerifyTxSignature(hash, nType, hashAnchor, destTo, vchSig, nForkHeight, fCompleted);
    }
    return destSeller.VerifyTxSignature(hash, nType, hashAnchor, destTo, vchSig, nForkHeight, fCompleted);
}
