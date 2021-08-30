// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "delegate.h"

#include "rpc/auto_protocol.h"
#include "template.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace xengine;
using namespace ibrio::crypto;

//////////////////////////////
// CTemplateDelegate

CTemplateDelegate::CTemplateDelegate(const ibrio::crypto::CPubKey& keyDelegateIn, const CDestination& destOwnerIn)
  : CTemplate(TEMPLATE_DELEGATE), keyDelegate(keyDelegateIn), destOwner(destOwnerIn)
{
}

CTemplateDelegate* CTemplateDelegate::clone() const
{
    return new CTemplateDelegate(*this);
}

bool CTemplateDelegate::GetSignDestination(const CTransaction& tx, const uint256& hashFork, int nHeight, const std::vector<uint8>& vchSig,
                                           std::set<CDestination>& setSubDest, std::vector<uint8>& vchSubSig) const
{
    if (!CTemplate::GetSignDestination(tx, hashFork, nHeight, vchSig, setSubDest, vchSubSig))
    {
        return false;
    }
    setSubDest.clear();
    if (tx.nType == CTransaction::TX_CERT)
    {
        setSubDest.insert(CDestination(keyDelegate));
    }
    else
    {
        setSubDest.insert(destOwner);
    }
    return true;
}

void CTemplateDelegate::GetTemplateData(ibrio::rpc::CTemplateResponse& obj, CDestination&& destInstance) const
{
    obj.delegate.strDelegate = keyDelegate.ToString();
    obj.delegate.strOwner = (destInstance = destOwner).ToString();
}

bool CTemplateDelegate::BuildVssSignature(const uint256& hash, const vector<uint8>& vchDelegateSig, vector<uint8>& vchVssSig)
{
    if (!keyDelegate.Verify(hash, vchDelegateSig))
    {
        return false;
    }
    vchVssSig.clear();
    xengine::CODataStream ods(vchVssSig, vchData.size() * 2 + vchDelegateSig.size());
    ods.Push(vchData.data(), vchData.size());
    ods.Push(vchData.data(), vchData.size());
    ods.Push(vchDelegateSig.data(), vchDelegateSig.size());
    return true;
}

bool CTemplateDelegate::ValidateParam() const
{
    if (!keyDelegate)
    {
        return false;
    }

    if (!IsTxSpendable(destOwner))
    {
        return false;
    }

    return true;
}

bool CTemplateDelegate::SetTemplateData(const std::vector<uint8>& vchDataIn)
{
    CIDataStream is(vchDataIn);
    try
    {
        is >> keyDelegate >> destOwner;
    }
    catch (const std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
        return false;
    }

    return true;
}

bool CTemplateDelegate::SetTemplateData(const ibrio::rpc::CTemplateRequest& obj, CDestination&& destInstance)
{
    if (obj.strType != GetTypeName(TEMPLATE_DELEGATE))
    {
        return false;
    }

    if (!destInstance.SetPubKey(obj.delegate.strDelegate))
    {
        return false;
    }
    keyDelegate = destInstance.GetPubKey();

    if (!destInstance.ParseString(obj.delegate.strOwner))
    {
        return false;
    }
    destOwner = destInstance;

    return true;
}

void CTemplateDelegate::BuildTemplateData()
{
    vchData.clear();
    CODataStream os(vchData);
    os << keyDelegate << destOwner;
}

bool CTemplateDelegate::VerifyTxSignature(const uint256& hash, const uint16 nType, const uint256& hashAnchor, const CDestination& destTo,
                                          const vector<uint8>& vchSig, const int32 nForkHeight, bool& fCompleted) const
{
    if (nType == CTransaction::TX_CERT)
    {
        return CDestination(keyDelegate).VerifyTxSignature(hash, nType, hashAnchor, destTo, vchSig, nForkHeight, fCompleted);
    }
    else
    {
        return destOwner.VerifyTxSignature(hash, nType, hashAnchor, destTo, vchSig, nForkHeight, fCompleted);
    }
}

bool CTemplateDelegate::VerifyBlockSignature(const uint256& hash, const vector<uint8>& vchSig) const
{
    return keyDelegate.Verify(hash, vchSig);
}
