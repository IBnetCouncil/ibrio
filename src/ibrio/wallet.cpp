// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"

#include "address.h"
#include "param.h"
#include "template/delegate.h"
#include "template/dexmatch.h"
#include "template/exchange.h"
#include "template/fork.h"
#include "template/mint.h"
#include "template/vote.h"

using namespace std;
using namespace xengine;

namespace ibrio
{

//////////////////////////////
// CDBAddressWalker

class CDBAddrWalker : public storage::CWalletDBAddrWalker
{
public:
    CDBAddrWalker(CWallet* pWalletIn)
      : pWallet(pWalletIn) {}
    bool WalkPubkey(const crypto::CPubKey& pubkey, int version, const crypto::CCryptoCipher& cipher) override
    {
        crypto::CKey key;
        key.Load(pubkey, version, cipher);
        return pWallet->LoadKey(key);
    }
    bool WalkTemplate(const CTemplateId& tid, const std::vector<unsigned char>& vchData) override
    {
        CTemplatePtr ptr = CTemplate::CreateTemplatePtr(tid.GetType(), vchData);
        if (ptr)
        {
            return pWallet->LoadTemplate(ptr);
        }
        return false;
    }

protected:
    CWallet* pWallet;
};

//////////////////////////////
// CWallet

CWallet::CWallet()
{
    pBlockChain = nullptr;
}

CWallet::~CWallet()
{
}

bool CWallet::HandleInitialize()
{
    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain");
        return false;
    }
    return true;
}

void CWallet::HandleDeinitialize()
{
    pBlockChain = nullptr;
}

bool CWallet::HandleInvoke()
{
    if (!dbWallet.Initialize(Config()->pathData / "wallet"))
    {
        Error("Failed to initialize wallet database");
        return false;
    }

    if (!LoadDB())
    {
        Error("Failed to load wallet database");
        return false;
    }

    return true;
}

void CWallet::HandleHalt()
{
    dbWallet.Deinitialize();
    Clear();
}

boost::optional<std::string> CWallet::AddKey(const crypto::CKey& key)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    if (!InsertKey(key))
    {
        Warn("AddKey : invalid or duplicated key");
        return std::string("AddKey : invalid or duplicated key");
    }

    if (!dbWallet.UpdateKey(key.GetPubKey(), key.GetVersion(), key.GetCipher()))
    {
        mapKeyStore.erase(key.GetPubKey());
        Warn("AddKey : failed to save key");
        return std::string("AddKey : failed to save key");
    }
    return boost::optional<std::string>{};
}

boost::optional<std::string> CWallet::RemoveKey(const crypto::CPubKey& pubkey)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    map<crypto::CPubKey, CWalletKeyStore>::const_iterator it = mapKeyStore.find(pubkey);
    if (it == mapKeyStore.end())
    {
        Warn("RemoveKey: failed to find key");
        return std::string("RemoveKey: failed to find key");
    }

    const crypto::CKey& key = it->second.key;
    if (!dbWallet.RemoveKey(key.GetPubKey()))
    {
        Warn("RemoveKey: failed to remove key from dbWallet");
        return std::string("RemoveKey: failed to remove key from dbWallet");
    }

    mapKeyStore.erase(key.GetPubKey());

    return boost::optional<std::string>{};
}

bool CWallet::LoadKey(const crypto::CKey& key)
{
    if (!InsertKey(key))
    {
        Error("LoadKey : invalid or duplicated key");
        return false;
    }
    return true;
}

size_t CWallet::GetPubKeys(set<crypto::CPubKey>& setPubKey, const uint64 nPos, const uint64 nCount) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

    uint64 nCurPos = 0;
    for (map<crypto::CPubKey, CWalletKeyStore>::const_iterator it = mapKeyStore.begin();
         it != mapKeyStore.end(); ++it, nCurPos++)
    {
        if (nCount == 0 || nCurPos >= nPos)
        {
            setPubKey.insert((*it).first);
            if (nCount > 0 && setPubKey.size() >= nCount)
            {
                break;
            }
        }
    }

    return mapKeyStore.size();
}

bool CWallet::Have(const crypto::CPubKey& pubkey, const int32 nVersion) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    auto it = mapKeyStore.find(pubkey);
    if (nVersion < 0)
    {
        return it != mapKeyStore.end();
    }
    else
    {
        return (it != mapKeyStore.end() && it->second.key.GetVersion() == nVersion);
    }
}

bool CWallet::Export(const crypto::CPubKey& pubkey, vector<unsigned char>& vchKey) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    map<crypto::CPubKey, CWalletKeyStore>::const_iterator it = mapKeyStore.find(pubkey);
    if (it != mapKeyStore.end())
    {
        (*it).second.key.Save(vchKey);
        return true;
    }
    return false;
}

bool CWallet::Import(const vector<unsigned char>& vchKey, crypto::CPubKey& pubkey)
{
    crypto::CKey key;
    if (!key.Load(vchKey))
    {
        return false;
    }
    pubkey = key.GetPubKey();
    return AddKey(key) ? false : true;
}

bool CWallet::Encrypt(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase,
                      const crypto::CCryptoString& strCurrentPassphrase)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    map<crypto::CPubKey, CWalletKeyStore>::iterator it = mapKeyStore.find(pubkey);
    if (it != mapKeyStore.end())
    {
        crypto::CKey& key = (*it).second.key;
        crypto::CKey keyTemp(key);
        if (!keyTemp.Encrypt(strPassphrase, strCurrentPassphrase))
        {
            Error("AddKey : Encrypt fail");
            return false;
        }
        if (!dbWallet.UpdateKey(key.GetPubKey(), keyTemp.GetVersion(), keyTemp.GetCipher()))
        {
            Error("AddKey : failed to update key");
            return false;
        }
        key.Encrypt(strPassphrase, strCurrentPassphrase);
        key.Lock();
        return true;
    }
    return false;
}

bool CWallet::GetKeyStatus(const crypto::CPubKey& pubkey, int& nVersion, bool& fLocked, int64& nAutoLockTime, bool& fPublic) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    map<crypto::CPubKey, CWalletKeyStore>::const_iterator it = mapKeyStore.find(pubkey);
    if (it != mapKeyStore.end())
    {
        const CWalletKeyStore& keystore = (*it).second;
        nVersion = keystore.key.GetVersion();
        fLocked = keystore.key.IsLocked();
        fPublic = keystore.key.IsPubKey();
        nAutoLockTime = (!fLocked && keystore.nAutoLockTime > 0) ? keystore.nAutoLockTime : 0;
        return true;
    }
    return false;
}

bool CWallet::IsLocked(const crypto::CPubKey& pubkey) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    map<crypto::CPubKey, CWalletKeyStore>::const_iterator it = mapKeyStore.find(pubkey);
    if (it != mapKeyStore.end())
    {
        return (*it).second.key.IsLocked();
    }
    return false;
}

bool CWallet::Lock(const crypto::CPubKey& pubkey)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    map<crypto::CPubKey, CWalletKeyStore>::iterator it = mapKeyStore.find(pubkey);
    if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
    {
        CWalletKeyStore& keystore = (*it).second;
        keystore.key.Lock();
        if (keystore.nTimerId)
        {
            CancelTimer(keystore.nTimerId);
            keystore.nTimerId = 0;
            keystore.nAutoLockTime = -1;
            keystore.nAutoDelayTime = 0;
        }
        return true;
    }
    return false;
}

bool CWallet::Unlock(const crypto::CPubKey& pubkey, const crypto::CCryptoString& strPassphrase, int64 nTimeout)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    map<crypto::CPubKey, CWalletKeyStore>::iterator it = mapKeyStore.find(pubkey);
    if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
    {
        CWalletKeyStore& keystore = (*it).second;
        if (!keystore.key.IsLocked() || !keystore.key.Unlock(strPassphrase))
        {
            return false;
        }

        if (nTimeout > 0)
        {
            keystore.nAutoDelayTime = nTimeout;
            keystore.nAutoLockTime = GetTime() + keystore.nAutoDelayTime;
            keystore.nTimerId = SetTimer(nTimeout * 1000, boost::bind(&CWallet::AutoLock, this, _1, (*it).first));
        }
        return true;
    }
    return false;
}

void CWallet::AutoLock(uint32 nTimerId, const crypto::CPubKey& pubkey)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    map<crypto::CPubKey, CWalletKeyStore>::iterator it = mapKeyStore.find(pubkey);
    if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
    {
        CWalletKeyStore& keystore = (*it).second;
        if (keystore.nTimerId == nTimerId)
        {
            keystore.key.Lock();
            keystore.nTimerId = 0;
            keystore.nAutoLockTime = -1;
            keystore.nAutoDelayTime = 0;
        }
    }
}

bool CWallet::Sign(const crypto::CPubKey& pubkey, const uint256& hash, vector<uint8>& vchSig)
{
    set<crypto::CPubKey> setSignedKey;
    bool ret;
    {
        boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
        ret = SignPubKey(pubkey, hash, vchSig, setSignedKey);
    }

    if (ret)
    {
        UpdateAutoLock(setSignedKey);
    }
    return ret;
}

bool CWallet::AesEncrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vMessage, std::vector<uint8>& vCiphertext)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

    auto it = mapKeyStore.find(pubkeyLocal);
    if (it == mapKeyStore.end())
    {
        StdError("CWallet", "Aes encrypt: find privkey fail, pubkey: %s", pubkeyLocal.GetHex().c_str());
        return false;
    }
    if (!it->second.key.IsPrivKey())
    {
        StdError("CWallet", "Aes encrypt: can't encrypt by non-privkey, pubkey: %s", pubkeyLocal.GetHex().c_str());
        return false;
    }
    if (!it->second.key.AesEncrypt(pubkeyRemote, vMessage, vCiphertext))
    {
        StdError("CWallet", "Aes encrypt: encrypt fail, pubkey: %s", pubkeyLocal.GetHex().c_str());
        return false;
    }
    return true;
}

bool CWallet::AesDecrypt(const crypto::CPubKey& pubkeyLocal, const crypto::CPubKey& pubkeyRemote, const std::vector<uint8>& vCiphertext, std::vector<uint8>& vMessage)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

    auto it = mapKeyStore.find(pubkeyLocal);
    if (it == mapKeyStore.end())
    {
        StdError("CWallet", "Aes decrypt: find privkey fail, pubkey: %s", pubkeyLocal.GetHex().c_str());
        return false;
    }
    if (!it->second.key.IsPrivKey())
    {
        StdError("CWallet", "Aes decrypt: can't decrypt by non-privkey, pubkey: %s", pubkeyLocal.GetHex().c_str());
        return false;
    }
    if (!it->second.key.AesDecrypt(pubkeyRemote, vCiphertext, vMessage))
    {
        StdError("CWallet", "Aes decrypt: decrypt fail, pubkey: %s", pubkeyLocal.GetHex().c_str());
        return false;
    }
    return true;
}

bool CWallet::LoadTemplate(CTemplatePtr ptr)
{
    if (ptr != nullptr)
    {
        return mapTemplatePtr.insert(make_pair(ptr->GetTemplateId(), ptr)).second;
    }
    return false;
}

void CWallet::GetTemplateIds(set<CTemplateId>& setTemplateId, const uint64 nPos, const uint64 nCount) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

    uint64 nCurPos = 0;
    for (map<CTemplateId, CTemplatePtr>::const_iterator it = mapTemplatePtr.begin();
         it != mapTemplatePtr.end(); ++it, nCurPos++)
    {
        if (nCount == 0 || nCurPos >= nPos)
        {
            setTemplateId.insert((*it).first);
            if (nCount > 0 && setTemplateId.size() >= nCount)
            {
                break;
            }
        }
    }
}

bool CWallet::Have(const CTemplateId& tid) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    return (!!mapTemplatePtr.count(tid));
}

bool CWallet::AddTemplate(CTemplatePtr& ptr)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    if (ptr != nullptr)
    {
        CTemplateId tid = ptr->GetTemplateId();
        if (mapTemplatePtr.insert(make_pair(tid, ptr)).second)
        {
            const vector<unsigned char>& vchData = ptr->GetTemplateData();
            return dbWallet.UpdateTemplate(tid, vchData);
        }
    }
    return false;
}

CTemplatePtr CWallet::GetTemplate(const CTemplateId& tid) const
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
    map<CTemplateId, CTemplatePtr>::const_iterator it = mapTemplatePtr.find(tid);
    if (it != mapTemplatePtr.end())
    {
        return (*it).second;
    }
    return nullptr;
}

bool CWallet::RemoveTemplate(const CTemplateId& tid)
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    map<CTemplateId, CTemplatePtr>::const_iterator it = mapTemplatePtr.find(tid);
    if (it != mapTemplatePtr.end())
    {
        if (!dbWallet.RemoveTemplate(tid))
        {
            return false;
        }
        mapTemplatePtr.erase(it);
        return true;
    }
    return false;
}

void CWallet::GetDestinations(set<CDestination>& setDest)
{
    boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);

    for (map<crypto::CPubKey, CWalletKeyStore>::const_iterator it = mapKeyStore.begin();
         it != mapKeyStore.end(); ++it)
    {
        setDest.insert(CDestination((*it).first));
    }

    for (map<CTemplateId, CTemplatePtr>::const_iterator it = mapTemplatePtr.begin();
         it != mapTemplatePtr.end(); ++it)
    {
        setDest.insert(CDestination((*it).first));
    }
}

bool CWallet::SignTransaction(const CDestination& destIn, CTransaction& tx, const vector<uint8>& vchDestInData, const vector<uint8>& vchSendToData, const vector<uint8>& vchSignExtraData, const uint256& hashFork, const int32 nForkHeight, bool& fCompleted)
{
    vector<uint8> vchSig;
    CTemplateId tid;
    if (destIn.GetTemplateId(tid) && tid.GetType() == TEMPLATE_ACTIVATE)
    {
        return false;
    }

    if (!CTemplate::VerifyDestRecorded(tx, nForkHeight + 1, vchSig))
    {
        Error("Sign Transaction: Parse dest fail, txid: %s", tx.GetHash().GetHex().c_str());
        return false;
    }

    set<crypto::CPubKey> setSignedKey;
    {
        boost::shared_lock<boost::shared_mutex> rlock(rwKeyStore);
        if (!SignDestination(destIn, tx, vchDestInData, tx.GetSignatureHash(), vchSig, vchSignExtraData, hashFork, nForkHeight + 1, setSignedKey, fCompleted))
        {
            Error("Sign transaction: Sign destination fail, destIn: %s, txid: %s",
                  destIn.ToString().c_str(), tx.GetHash().GetHex().c_str());
            return false;
        }
    }

    UpdateAutoLock(setSignedKey);

    vector<uint8> vchDestData;
    if (!GetSendToDestRecorded(tx, nForkHeight + 1, vchSendToData, vchDestData))
    {
        Error("Sign transaction: Get SendTo DestRecorded fail, destIn: %s, txid: %s",
              destIn.ToString().c_str(), tx.GetHash().GetHex().c_str());
        return false;
    }
    if (!vchDestData.empty())
    {
        tx.vchSig = move(vchDestData);
        tx.vchSig.insert(tx.vchSig.end(), vchSig.begin(), vchSig.end());
    }
    else
    {
        tx.vchSig = move(vchSig);
    }
    return true;
}

bool CWallet::AddMemKey(const uint256& secret, crypto::CPubKey& pubkey)
{
    crypto::CKey key;
    if (!key.SetSecret(crypto::CCryptoKeyData(secret.begin(), secret.end())))
    {
        return false;
    }
    pubkey = key.GetPubKey();
    if (Have(pubkey, crypto::CKey::PRIVATE_KEY))
    {
        return false;
    }
    return mapMemSignKey.insert(make_pair(pubkey, secret)).second;
}

void CWallet::RemoveMemKey(const crypto::CPubKey& pubkey)
{
    mapMemSignKey.erase(pubkey);
}

bool CWallet::GetSendToDestRecorded(const CTransaction& tx, const int nHeight, const vector<uint8>& vchSendToData, vector<uint8>& vchDestData)
{
    CTemplateId tid;
    if (tx.sendTo.GetTemplateId(tid) && CTemplate::IsDestInRecorded(tx.sendTo))
    {
        if (!vchSendToData.empty())
        {
            CTemplatePtr tempPtr = CTemplate::Import(vchSendToData);
            if (tempPtr != nullptr && tempPtr->GetTemplateId() == tid)
            {
                vchDestData = tempPtr->GetTemplateData();
                return true;
            }
        }
        CTemplatePtr tempPtr = GetTemplate(tid);
        if (tempPtr != nullptr)
        {
            vchDestData = tempPtr->GetTemplateData();
            return true;
        }
        return false;
    }
    return true;
}

bool CWallet::LoadDB()
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);

    CDBAddrWalker walker(this);
    if (!dbWallet.WalkThroughAddress(walker))
    {
        StdLog("CWallet", "LoadDB: WalkThroughAddress fail.");
        return false;
    }
    return true;
}

void CWallet::Clear()
{
    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    mapKeyStore.clear();
    mapTemplatePtr.clear();
}

bool CWallet::InsertKey(const crypto::CKey& key)
{
    if (!key.IsNull())
    {
        auto it = mapKeyStore.find(key.GetPubKey());
        if (it != mapKeyStore.end())
        {
            // privkey update pubkey
            if (it->second.key.IsPubKey() && key.IsPrivKey())
            {
                it->second = CWalletKeyStore(key);
                it->second.key.Lock();
                return true;
            }
            return false;
        }
        else
        {
            auto ret = mapKeyStore.insert(make_pair(key.GetPubKey(), CWalletKeyStore(key)));
            ret.first->second.key.Lock();
            return true;
        }
    }
    return false;
}

bool CWallet::SignPubKey(const crypto::CPubKey& pubkey, const uint256& hash, vector<uint8>& vchSig, std::set<crypto::CPubKey>& setSignedKey)
{
    auto it = mapKeyStore.find(pubkey);
    if (it == mapKeyStore.end())
    {
        auto mt = mapMemSignKey.find(pubkey);
        if (mt != mapMemSignKey.end())
        {
            crypto::CKey::MemSign(mt->second, pubkey, hash, vchSig);
            return true;
        }
        StdError("CWallet", "SignPubKey: find privkey fail, pubkey: %s", pubkey.GetHex().c_str());
        return false;
    }
    if (!it->second.key.IsPrivKey())
    {
        auto mt = mapMemSignKey.find(pubkey);
        if (mt != mapMemSignKey.end())
        {
            crypto::CKey::MemSign(mt->second, pubkey, hash, vchSig);
            return true;
        }
        StdError("CWallet", "SignPubKey: can't sign tx by non-privkey, pubkey: %s", pubkey.GetHex().c_str());
        return false;
    }
    if (!it->second.key.Sign(hash, vchSig))
    {
        StdError("CWallet", "SignPubKey: sign fail, pubkey: %s", pubkey.GetHex().c_str());
        return false;
    }

    setSignedKey.insert(pubkey);
    return true;
}

bool CWallet::SignMultiPubKey(const set<crypto::CPubKey>& setPubKey, const uint256& hash,
                              const uint256& hashAnchor, vector<uint8>& vchSig,
                              std::set<crypto::CPubKey>& setSignedKey)
{
    bool fSigned = false;
    for (auto& pubkey : setPubKey)
    {
        auto it = mapKeyStore.find(pubkey);
        if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
        {
            fSigned |= it->second.key.MultiSign(setPubKey, hash, vchSig);
            setSignedKey.insert(pubkey);
        }
    }
    return fSigned;
}

bool CWallet::SignDestination(const CDestination& destIn, const CTransaction& tx, const vector<uint8>& vchDestInData, const uint256& hash,
                              vector<uint8>& vchSig, const vector<uint8>& vchSignExtraData, const uint256& hashFork, const int32 nForkHeight,
                              std::set<crypto::CPubKey>& setSignedKey, bool& fCompleted)
{
    if (destIn.IsPubKey())
    {
        fCompleted = SignPubKey(destIn.GetPubKey(), hash, vchSig, setSignedKey);
        if (!fCompleted)
        {
            StdError("CWallet", "Sign destination: PubKey SignPubKey fail, txid: %s, destIn: %s",
                     tx.GetHash().GetHex().c_str(), CAddress(destIn).ToString().c_str());
        }
        return fCompleted;
    }

    if (destIn.IsTemplate())
    {
        CTemplatePtr ptr = nullptr;
        if (!vchDestInData.empty())
        {
            ptr = CTemplate::Import(vchDestInData);
            if (!ptr)
            {
                StdError("CWallet", "Sign destination: Import address fail, txid: %s, destIn: %s, vchDestInData: %s",
                         tx.GetHash().GetHex().c_str(), CAddress(destIn).ToString().c_str(),
                         ToHexString(&(vchDestInData[0]), vchDestInData.size()).c_str());
            }
        }
        if (ptr == nullptr || ptr->GetTemplateId() != destIn.GetTemplateId())
        {
            ptr = GetTemplate(destIn.GetTemplateId());
        }
        if (!ptr)
        {
            StdError("CWallet", "Sign destination: GetTemplate fail, txid: %s, destIn: %s", tx.GetHash().GetHex().c_str(), CAddress(destIn).ToString().c_str());
            return false;
        }

        set<CDestination> setSubDest;
        vector<uint8> vchSubSig;
        if (!ptr->GetSignDestination(tx, hashFork, nForkHeight, vchSig, setSubDest, vchSubSig))
        {
            StdError("CWallet", "Sign destination: Get sign destination fail, txid: %s", tx.GetHash().GetHex().c_str());
            return false;
        }

        if (setSubDest.empty())
        {
            StdError("CWallet", "Sign destination: setSubDest is empty, txid: %s", tx.GetHash().GetHex().c_str());
            return false;
        }
        else if (setSubDest.size() == 1)
        {
            vector<uint8> vchSubDestInData;
            size_t nTemplateDataSize = ptr->GetTemplateData().size() + 2; //2 bytes is template type
            if (nTemplateDataSize < vchDestInData.size())
            {
                vchSubDestInData.assign(vchDestInData.begin() + nTemplateDataSize, vchDestInData.end());
            }
            if (!SignDestination(*setSubDest.begin(), tx, vchSubDestInData, hash, vchSubSig, vchSignExtraData, hashFork, nForkHeight, setSignedKey, fCompleted))
            {
                StdError("CWallet", "Sign destination: Sign destination fail, txid: %s", tx.GetHash().GetHex().c_str());
                return false;
            }
        }
        else
        {
            set<crypto::CPubKey> setPubKey;
            for (const CDestination& dest : setSubDest)
            {
                if (!dest.IsPubKey())
                {
                    StdError("CWallet", "Sign destination: dest not is pubkey, txid: %s, dest: %s",
                             tx.GetHash().GetHex().c_str(), CAddress(dest).ToString().c_str());
                    return false;
                }
                setPubKey.insert(dest.GetPubKey());
            }

            if (!SignMultiPubKey(setPubKey, hash, tx.hashAnchor, vchSubSig, setSignedKey))
            {
                StdError("CWallet", "Sign destination: SignMultiPubKey fail, txid: %s", tx.GetHash().GetHex().c_str());
                return false;
            }
        }

        if (ptr->GetTemplateType() == TEMPLATE_EXCHANGE)
        {
            CTemplateExchangePtr pe = boost::dynamic_pointer_cast<CTemplateExchange>(ptr);
            vchSig = tx.vchSig;
            return pe->BuildTxSignature(hash, tx.nType, tx.hashAnchor, tx.sendTo, vchSubSig, vchSig);
        }
        else if (ptr->GetTemplateType() == TEMPLATE_DEXMATCH)
        {
            CTemplateDexMatchPtr pe = boost::dynamic_pointer_cast<CTemplateDexMatch>(ptr);
            return pe->BuildDexMatchTxSignature(vchSignExtraData, vchSubSig, vchSig);
        }
        else
        {
            if (!ptr->BuildTxSignature(hash, tx.nType, tx.hashAnchor, tx.sendTo, nForkHeight, vchSubSig, vchSig, fCompleted))
            {
                StdError("CWallet", "Sign destination: BuildTxSignature fail, txid: %s", tx.GetHash().GetHex().c_str());
                return false;
            }
        }

        return true;
    }

    StdError("CWallet", "Sign destination: destIn type error, txid: %s, destIn: %s",
             tx.GetHash().GetHex().c_str(), CAddress(destIn).ToString().c_str());
    return false;
}

void CWallet::UpdateAutoLock(const std::set<crypto::CPubKey>& setSignedKey)
{
    if (setSignedKey.empty())
    {
        return;
    }

    boost::unique_lock<boost::shared_mutex> wlock(rwKeyStore);
    for (auto& key : setSignedKey)
    {
        map<crypto::CPubKey, CWalletKeyStore>::iterator it = mapKeyStore.find(key);
        if (it != mapKeyStore.end() && it->second.key.IsPrivKey())
        {
            CWalletKeyStore& keystore = (*it).second;
            if (keystore.nAutoDelayTime > 0)
            {
                CancelTimer(keystore.nTimerId);
                keystore.nAutoLockTime = GetTime() + keystore.nAutoDelayTime;
                keystore.nTimerId = SetTimer(keystore.nAutoDelayTime * 1000, boost::bind(&CWallet::AutoLock, this, _1, keystore.key.GetPubKey()));
            }
        }
    }
}

} // namespace ibrio
