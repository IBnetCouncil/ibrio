// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activate.h"

#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace xengine;
using namespace ibrio::crypto;

//////////////////////////////
// CTemplateActivate

CTemplateActivate::CTemplateActivate(const CDestination& destInviterIn, const CDestination& destOwnerIn)
  : CTemplate(TEMPLATE_ACTIVATE), destInviter(destInviterIn), destOwner(destOwnerIn)
{
}

CTemplateActivate* CTemplateActivate::clone() const
{
    return new CTemplateActivate(*this);
}

bool CTemplateActivate::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const std::vector<uint8>& vchSig,
                                           std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const
{
    if (!CTemplate::GetSignDestination(tx, hashFork, nHeight, vchSig, setSubDest, vchSubSig))
    {
        return false;
    }
    setSubDest.clear();
    return true;
}

void CTemplateActivate::GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const
{
    obj.activate.strInviter = (destInstance = destInviter).ToString();
    obj.activate.strOwner = (destInstance = destOwner).ToString();
}

bool CTemplateActivate::ValidateParam() const
{
    if (!IsTxSpendable(destInviter))
    {
        return false;
    }
    if (!IsTxSpendable(destOwner))
    {
        return false;
    }
    return true;
}

bool CTemplateActivate::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    CIDataStream is(vchDataIn);
    try
    {
        is >> destInviter >> destOwner;
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    return true;
}

bool CTemplateActivate::SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance)
{
    if (obj.strType != GetTypeName(TEMPLATE_ACTIVATE))
    {
        return false;
    }

    if (!destInstance.SetPubKey(obj.activate.strInviter))
    {
        return false;
    }
    destInviter = destInstance;

    if (!destInstance.ParseString(obj.activate.strOwner))
    {
        return false;
    }
    destOwner = destInstance;

    return true;
}

void CTemplateActivate::BuildTemplateData()
{
    vchData.clear();
    CODataStream os(vchData);
    os << destInviter << destOwner;
}

bool CTemplateActivate::VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                          const vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const
{
    return false;
}
