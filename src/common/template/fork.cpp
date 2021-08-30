// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "fork.h"

#include "destination.h"
#include "rpc/auto_protocol.h"
#include "template.h"
#include "util.h"

using namespace std;
using namespace xengine;

//////////////////////////////
// CTemplateFork

CTemplateFork::CTemplateFork(const CDestination& destRedeemIn, const uint256& hashForkIn)
  : CTemplate(TEMPLATE_FORK), destRedeem(destRedeemIn), hashFork(hashForkIn)
{
}

CTemplateFork* CTemplateFork::clone() const
{
    return new CTemplateFork(*this);
}

bool CTemplateFork::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const std::vector<uint8>& vchSig,
                                       std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const
{
    if (!CTemplate::GetSignDestination(tx, hashFork, nHeight, vchSig, setSubDest, vchSubSig))
    {
        return false;
    }

    setSubDest.clear();
    setSubDest.insert(destRedeem);
    return true;
}

void CTemplateFork::GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const
{
    obj.fork.strFork = hashFork.GetHex();
    obj.fork.strRedeem = (destInstance = destRedeem).ToString();
}

int64 CTemplateFork::CreatedCoin()
{
    return MORTGAGE_BASE;
}

int64 CTemplateFork::LockedCoin(const int32 nHeight)
{
    return (int64)(MORTGAGE_BASE * pow(MORTGAGE_DECAY_QUANTITY, nHeight / MORTGAGE_DECAY_CYCLE));
}
/*
int64 CTemplateFork::LockedCoin(const CDestination& destTo, const int32 nForkHeight) const
{
    return (int64)(MORTGAGE_BASE * pow(MORTGAGE_DECAY_QUANTITY, nForkHeight / MORTGAGE_DECAY_CYCLE));
}
*/
void CTemplateFork::GetForkParam(CDestination& destRedeemOut, uint256& hashForkOut)
{
    destRedeemOut = destRedeem;
    hashForkOut = hashFork;
}

bool CTemplateFork::ValidateParam() const
{
    if (!IsTxSpendable(destRedeem))
    {
        return false;
    }

    if (!hashFork)
    {
        return false;
    }

    return true;
}

bool CTemplateFork::SetTemplateData(const vector<uint8>& vchDataIn)
{
    CIDataStream is(vchDataIn);
    try
    {
        is >> destRedeem >> hashFork;
    }
    catch (exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    return true;
}

bool CTemplateFork::SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance)
{
    if (obj.strType != GetTypeName(TEMPLATE_FORK))
    {
        return false;
    }

    if (!destInstance.ParseString(obj.fork.strRedeem))
    {
        return false;
    }
    destRedeem = destInstance;

    if (hashFork.SetHex(obj.fork.strFork) != obj.fork.strFork.size())
    {
        return false;
    }

    return true;
}

void CTemplateFork::BuildTemplateData()
{
    vchData.clear();
    CODataStream os(vchData);
    os << destRedeem << hashFork;
}

bool CTemplateFork::VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                      const vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const
{
    return destRedeem.VerifyTxSignature(hash, nType, hashAnchor, destTo, vchSig, nForkHeight, fCompleted);
}
