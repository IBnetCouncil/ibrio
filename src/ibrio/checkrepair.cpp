// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "checkrepair.h"

#include "param.h"
#include "template/activate.h"
#include "template/vote.h"
#include "util.h"

using namespace std;
using namespace xengine;
using namespace boost::filesystem;

#define BLOCKFILE_PREFIX "block"

namespace ibrio
{

/////////////////////////////////////////////////////////////////////////
// CCheckForkUnspentWalker

bool CCheckForkUnspentWalker::Walk(const CTxOutPoint& txout, const CTxOut& output)
{
    if (!output.IsNull())
    {
        auto it = mapBlockUnspent.find(txout);
        if (it == mapBlockUnspent.end())
        {
            StdLog("check", "Check db unspent: find utxo fail, utxo: [%d] %s.", txout.n, txout.hash.GetHex().c_str());
            vRemove.push_back(txout);
        }
        else
        {
            setForkUnspent.insert(txout);
            if (output != it->second)
            {
                StdLog("check", "Check db unspent: txout error, utxo: [%d] %s.", txout.n, txout.hash.GetHex().c_str());
                vAddUpdate.push_back(CTxUnspent(it->first, static_cast<const CTxOut&>(it->second)));
            }
        }
    }
    else
    {
        StdLog("check", "Leveldb unspent is NULL, utxo: [%d] %s.", txout.n, txout.hash.GetHex().c_str());
    }
    return true;
}

bool CCheckForkUnspentWalker::CheckForkUnspent()
{
    for (auto it = mapBlockUnspent.begin(); it != mapBlockUnspent.end(); ++it)
    {
        if (setForkUnspent.find(it->first) == setForkUnspent.end())
        {
            StdLog("check", "Check block unspent: find utxo fail, utxo: [%d] %s.", it->first.n, it->first.hash.GetHex().c_str());
            vAddUpdate.push_back(CTxUnspent(it->first, static_cast<const CTxOut&>(it->second)));
        }
    }
    return !(vAddUpdate.size() > 0 || vRemove.size() > 0);
}

/////////////////////////////////////////////////////////////////////////
// CCheckAddressUnspentWalker

bool CCheckAddressUnspentWalker::Walk(const CAddrUnspentKey& out, const CUnspentOut& unspent)
{
    if (!unspent.IsNull())
    {
        auto it = mapBlockUnspent.find(out.out);
        if (it == mapBlockUnspent.end())
        {
            StdLog("check", "Check db address unspent: find utxo fail, utxo: [%d] %s.", out.out.n, out.out.hash.GetHex().c_str());
            vRemove.push_back(out);
        }
        else
        {
            setForkUnspent.insert(out.out);

            CUnspentOut unspentBlockOut(static_cast<const CTxOut&>(it->second), it->second.nTxType, it->second.nHeight);
            if (unspent != unspentBlockOut)
            {
                StdLog("check", "Check db address unspent: txout error, utxo: [%d] %s.", out.out.n, out.out.hash.GetHex().c_str());
                vAddUpdate.push_back(make_pair(out, unspentBlockOut));
            }
        }
    }
    return true;
}

bool CCheckAddressUnspentWalker::CheckForkAddressUnspent()
{
    for (auto it = mapBlockUnspent.begin(); it != mapBlockUnspent.end(); ++it)
    {
        if (setForkUnspent.find(it->first) == setForkUnspent.end())
        {
            StdLog("check", "Check block address unspent: find utxo fail, utxo: [%d] %s.", it->first.n, it->first.hash.GetHex().c_str());
            vAddUpdate.push_back(make_pair(CAddrUnspentKey(it->second.destTo, it->first), CUnspentOut(static_cast<const CTxOut&>(it->second), it->second.nTxType, it->second.nHeight)));
        }
    }
    return !(vAddUpdate.size() > 0 || vRemove.size() > 0);
}

/////////////////////////////////////////////////////////////////////////
// CCheckAddressWalker

bool CCheckAddressWalker::Walk(const CDestination& dest, const CAddrInfo& addrInfo)
{
    auto it = mapBlockAddress.find(dest);
    if (it == mapBlockAddress.end())
    {
        StdLog("check", "Check db address: find address fail, address: %s.", CAddress(dest).ToString().c_str());
        vRemove.push_back(dest);
    }
    else
    {
        mapDbAddress.insert(make_pair(dest, addrInfo));
    }
    return true;
}

bool CCheckAddressWalker::CheckAddress()
{
    for (auto it = mapBlockAddress.begin(); it != mapBlockAddress.end(); ++it)
    {
        auto mt = mapDbAddress.find(it->first);
        if (mt == mapDbAddress.end() || mt->second.destParent != it->second.second.destParent)
        {
            if (mt != mapDbAddress.end())
            {
                vRemove.push_back(it->first);
            }
            vAddUpdate.push_back(make_pair(it->first, it->second.second));
        }
    }
    return !(vAddUpdate.size() > 0 || vRemove.size() > 0);
}

/////////////////////////////////////////////////////////////////////////
// CCheckAddressTxIndexWalker

bool CCheckAddressTxIndexWalker::Walk(const CAddrTxIndex& key, const CAddrTxInfo& value)
{
    auto it = mapBlockTxIndex.find(key.txid);
    if (it == mapBlockTxIndex.end())
    {
        StdLog("check", "Check db address txindex: find txid fail, address: %s, txid: %s.",
               CAddress(key.dest).ToString().c_str(), key.txid.GetHex().c_str());
        vRemove.push_back(key);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////
// CCheckForkManager

CCheckForkManager::~CCheckForkManager()
{
    dbFork.Deinitialize();
}

bool CCheckForkManager::Initialize()
{
    for (const auto& vd : mapCheckPointsList)
    {
        mapCheckPoints.insert(make_pair(vd.first, vd.second));
    }

    if (!dbFork.Initialize(path(strDataPath), objParam.hashGenesisBlock))
    {
        StdError("check", "Fork manager set param: Initialize fail");
        return false;
    }
    return true;
}

bool CCheckForkManager::FetchForkStatus()
{
    if (!Initialize())
    {
        StdLog("check", "Fetch fork status: Initialize fail");
        return false;
    }

    bool fCheckGenesisRet = true;
    uint256 hashRefFdBlock;
    map<uint256, int> mapValidFork;
    if (!dbFork.RetrieveValidForkHash(objParam.hashGenesisBlock, hashRefFdBlock, mapValidFork))
    {
        fCheckGenesisRet = false;
    }
    else
    {
        if (hashRefFdBlock != 0)
        {
            fCheckGenesisRet = false;
        }
        else
        {
            const auto it = mapValidFork.find(objParam.hashGenesisBlock);
            if (it == mapValidFork.end() || it->second != 0)
            {
                fCheckGenesisRet = false;
            }
        }
    }
    if (!fCheckGenesisRet)
    {
        StdError("check", "Fetch fork status: Check GenesisBlock valid fork fail");
        if (fOnlyCheck)
        {
            return false;
        }
        mapValidFork.insert(make_pair(objParam.hashGenesisBlock, 0));
        if (!dbFork.AddValidForkHash(objParam.hashGenesisBlock, uint256(), mapValidFork))
        {
            StdError("check", "Fetch fork status: Add valid fork fail");
            return false;
        }
    }

    if (!dbFork.ListActiveFork(mapActiveFork))
    {
        StdError("check", "Fetch fork status: List active fork fail");
        return false;
    }

    return true;
}

bool CCheckForkManager::AddBlockForkContext(const CBlockEx& blockex)
{
    uint256 hashBlock = blockex.GetHash();
    uint256 hashRefFdBlock;
    map<uint256, int> mapValidFork;

    if (hashBlock == objParam.hashGenesisBlock)
    {
        vector<pair<CDestination, CForkContext>> vForkCtxt;
        CProfile profile;
        if (!profile.Load(blockex.vchProof))
        {
            StdLog("check", "Add block fork context: Load genesis %s block Proof failed", objParam.hashGenesisBlock.ToString().c_str());
            return false;
        }
        vForkCtxt.push_back(make_pair(profile.destOwner, CForkContext(objParam.hashGenesisBlock, uint64(0), uint64(0), profile)));

        mapValidFork.insert(make_pair(objParam.hashGenesisBlock, 0));
        if (!AddForkContext(uint256(), objParam.hashGenesisBlock, vForkCtxt, true, hashRefFdBlock, mapValidFork))
        {
            StdLog("check", "Add block fork context: Add fork context fail, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }
    else
    {
        vector<pair<CDestination, CForkContext>> vForkCtxt;
        for (size_t i = 0; i < blockex.vtx.size(); i++)
        {
            const CTransaction& tx = blockex.vtx[i];
            const CTxContxt& txContxt = blockex.vTxContxt[i];
            if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_FORK)
            {
                if (!VerifyBlockForkTx(blockex.hashPrev, tx, vForkCtxt))
                {
                    StdLog("check", "Add block fork context: Verify block fork tx fail, block: %s", hashBlock.ToString().c_str());
                }
            }
            if (txContxt.destIn.IsTemplate() && txContxt.destIn.GetTemplateId().GetType() == TEMPLATE_FORK)
            {
                auto it = vForkCtxt.begin();
                while (it != vForkCtxt.end())
                {
                    if (it->first == txContxt.destIn)
                    {
                        StdLog("check", "Add block fork context: cancel fork, block: %s, fork: %s, dest: %s",
                               hashBlock.ToString().c_str(), it->second.hashFork.ToString().c_str(),
                               CAddress(txContxt.destIn).ToString().c_str());
                        vForkCtxt.erase(it++);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }

        if (!AddForkContext(blockex.hashPrev, hashBlock, vForkCtxt, IsCheckPoint(objParam.hashGenesisBlock, hashBlock), hashRefFdBlock, mapValidFork))
        {
            StdLog("check", "Add block fork context: Add fork context fail, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }

    if (!CheckDbValidFork(hashBlock, hashRefFdBlock, mapValidFork))
    {
        StdLog("check", "Add block fork context: Check db valid fork fail, block: %s", hashBlock.ToString().c_str());
        if (fOnlyCheck)
        {
            return false;
        }
        if (!AddDbValidForkHash(hashBlock, hashRefFdBlock, mapValidFork))
        {
            StdLog("check", "Add block fork context: Add db valid fork fail, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CCheckForkManager::VerifyBlockForkTx(const uint256& hashPrev, const CTransaction& tx, vector<pair<CDestination, CForkContext>>& vForkCtxt)
{
    if (tx.vchData.empty())
    {
        StdLog("check", "Verify block fork tx: invalid vchData, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }

    CBlock block;
    CProfile profile;
    try
    {
        CBufStream ss;
        ss.Write((const char*)tx.vchData.data(), tx.vchData.size());
        ss >> block;
        if (!block.IsOrigin() || block.IsPrimary())
        {
            StdLog("check", "Verify block fork tx: invalid block, tx: %s", tx.GetHash().ToString().c_str());
            return false;
        }
        if (!profile.Load(block.vchProof))
        {
            StdLog("check", "Verify block fork tx: invalid profile, tx: %s", tx.GetHash().ToString().c_str());
            return false;
        }
    }
    catch (...)
    {
        StdLog("check", "Verify block fork tx: invalid vchData, tx: %s", tx.GetHash().ToString().c_str());
        return false;
    }
    uint256 hashNewFork = block.GetHash();

    do
    {
        CForkContext ctxtParent;
        if (!GetForkContext(profile.hashParent, ctxtParent))
        {
            bool fFindParent = false;
            for (const auto& vd : vForkCtxt)
            {
                if (vd.second.hashFork == profile.hashParent)
                {
                    ctxtParent = vd.second;
                    fFindParent = true;
                    break;
                }
            }
            if (!fFindParent)
            {
                StdLog("check", "Verify block fork tx: Retrieve parent context, tx: %s", tx.GetHash().ToString().c_str());
                break;
            }
        }

        CProfile forkProfile;
        if (objParam.ValidateOrigin(block, ctxtParent.GetProfile(), forkProfile) != OK)
        {
            StdLog("check", "Verify block fork tx: Validate origin, tx: %s", tx.GetHash().ToString().c_str());
            break;
        }

        if (!VerifyValidFork(hashPrev, hashNewFork, profile.strName))
        {
            StdLog("check", "Verify block fork tx: verify fork fail, tx: %s", tx.GetHash().ToString().c_str());
            break;
        }
        bool fCheckRet = true;
        for (const auto& vd : vForkCtxt)
        {
            if (vd.second.hashFork == hashNewFork || vd.second.strName == profile.strName)
            {
                StdLog("check", "Verify block fork tx: fork exist, tx: %s", tx.GetHash().ToString().c_str());
                fCheckRet = false;
                break;
            }
        }
        if (!fCheckRet)
        {
            break;
        }

        vForkCtxt.push_back(make_pair(tx.sendTo, CForkContext(block.GetHash(), block.hashPrev, tx.GetHash(), profile)));
    } while (0);

    return true;
}

bool CCheckForkManager::AddForkContext(const uint256& hashPrevBlock, const uint256& hashNewBlock, const vector<pair<CDestination, CForkContext>>& vForkCtxt,
                                       bool fCheckPointBlock, uint256& hashRefFdBlock, map<uint256, int>& mapValidFork)
{
    CCheckValidFdForkId& fd = mapBlockValidFork[hashNewBlock];
    if (fCheckPointBlock)
    {
        fd.mapForkId.clear();
        if (hashPrevBlock != 0)
        {
            if (!GetValidFdForkId(hashPrevBlock, fd.mapForkId))
            {
                StdError("check", "Add fork context: Get Valid forkid fail, prev: %s", hashPrevBlock.GetHex().c_str());
                mapBlockValidFork.erase(hashNewBlock);
                return false;
            }
        }
        fd.hashRefFdBlock = uint256();
    }
    else
    {
        const auto it = mapBlockValidFork.find(hashPrevBlock);
        if (it == mapBlockValidFork.end())
        {
            StdError("check", "Add fork context: Find Valid forkid fail, prev: %s", hashPrevBlock.GetHex().c_str());
            mapBlockValidFork.erase(hashNewBlock);
            return false;
        }
        const CCheckValidFdForkId& prevfd = it->second;
        fd.mapForkId.clear();
        if (prevfd.hashRefFdBlock == 0)
        {
            fd.hashRefFdBlock = hashPrevBlock;
        }
        else
        {
            fd.mapForkId.insert(prevfd.mapForkId.begin(), prevfd.mapForkId.end());
            fd.hashRefFdBlock = prevfd.hashRefFdBlock;
        }
    }
    for (const auto& vd : vForkCtxt)
    {
        const CForkContext& ctxt = vd.second;
        const uint256& hashFork = ctxt.hashFork;
        mapBlockForkSched[hashFork].ctxtFork = ctxt;
        if (ctxt.hashParent != 0)
        {
            mapBlockForkSched[ctxt.hashParent].AddNewJoint(ctxt.hashJoint, hashFork);
        }
        fd.mapForkId.insert(make_pair(hashFork, CBlock::GetBlockHeightByHash(hashNewBlock)));
    }
    hashRefFdBlock = fd.hashRefFdBlock;
    mapValidFork.clear();
    mapValidFork.insert(fd.mapForkId.begin(), fd.mapForkId.end());
    return true;
}

bool CCheckForkManager::GetForkContext(const uint256& hashFork, CForkContext& ctxt)
{
    const auto it = mapBlockForkSched.find(hashFork);
    if (it != mapBlockForkSched.end())
    {
        ctxt = it->second.ctxtFork;
        return true;
    }
    return false;
}

bool CCheckForkManager::VerifyValidFork(const uint256& hashPrevBlock, const uint256& hashFork, const string& strForkName)
{
    map<uint256, int> mapValidFork;
    if (GetValidFdForkId(hashPrevBlock, mapValidFork))
    {
        if (mapValidFork.count(hashFork) > 0)
        {
            StdLog("check", "Verify valid fork: Fork existed, fork: %s", hashFork.GetHex().c_str());
            return false;
        }
        for (const auto& vd : mapValidFork)
        {
            const auto mt = mapBlockForkSched.find(vd.first);
            if (mt != mapBlockForkSched.end() && mt->second.ctxtFork.strName == strForkName)
            {
                StdLog("check", "Verify valid fork: Fork name repeated, new fork: %s, valid fork: %s, name: %s",
                       hashFork.GetHex().c_str(), vd.first.GetHex().c_str(), strForkName.c_str());
                return false;
            }
        }
    }
    return true;
}

bool CCheckForkManager::GetValidFdForkId(const uint256& hashBlock, map<uint256, int>& mapFdForkIdOut)
{
    const auto it = mapBlockValidFork.find(hashBlock);
    if (it != mapBlockValidFork.end())
    {
        if (it->second.hashRefFdBlock != 0)
        {
            const auto mt = mapBlockValidFork.find(it->second.hashRefFdBlock);
            if (mt != mapBlockValidFork.end() && !mt->second.mapForkId.empty())
            {
                mapFdForkIdOut.insert(mt->second.mapForkId.begin(), mt->second.mapForkId.end());
            }
        }
        if (!it->second.mapForkId.empty())
        {
            mapFdForkIdOut.insert(it->second.mapForkId.begin(), it->second.mapForkId.end());
        }
        return true;
    }
    return false;
}

int CCheckForkManager::GetValidForkCreatedHeight(const uint256& hashBlock, const uint256& hashFork)
{
    const auto it = mapBlockValidFork.find(hashBlock);
    if (it != mapBlockValidFork.end())
    {
        int nCreaatedHeight = it->second.GetCreatedHeight(hashFork);
        if (nCreaatedHeight >= 0)
        {
            return nCreaatedHeight;
        }
        if (it->second.hashRefFdBlock != 0)
        {
            const auto mt = mapBlockValidFork.find(it->second.hashRefFdBlock);
            if (mt != mapBlockValidFork.end())
            {
                return mt->second.GetCreatedHeight(hashFork);
            }
        }
    }
    return -1;
}

bool CCheckForkManager::CheckDbValidFork(const uint256& hashBlock, const uint256& hashRefFdBlock, const map<uint256, int>& mapValidFork)
{
    uint256 hashRefFdBlockGet;
    map<uint256, int> mapValidForkGet;
    if (!dbFork.RetrieveValidForkHash(hashBlock, hashRefFdBlockGet, mapValidForkGet))
    {
        StdLog("check", "CheckDbValidFork: RetrieveValidForkHash fail, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    if (hashRefFdBlockGet != hashRefFdBlock || mapValidForkGet.size() != mapValidFork.size())
    {
        StdLog("check", "CheckDbValidFork: hashRefFdBlock or mapValidFork error, block: %s", hashBlock.GetHex().c_str());
        return false;
    }
    for (const auto& vd : mapValidForkGet)
    {
        const auto it = mapValidFork.find(vd.first);
        if (it == mapValidFork.end() || it->second != vd.second)
        {
            StdLog("check", "CheckDbValidFork: mapValidFork error, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CCheckForkManager::AddDbValidForkHash(const uint256& hashBlock, const uint256& hashRefFdBlock, const map<uint256, int>& mapValidFork)
{
    return dbFork.AddValidForkHash(hashBlock, hashRefFdBlock, mapValidFork);
}

bool CCheckForkManager::GetDbForkContext(const uint256& hashFork, CForkContext& ctxt)
{
    return dbFork.RetrieveForkContext(hashFork, ctxt);
}

bool CCheckForkManager::UpdateDbForkContext(const CForkContext& ctxt)
{
    return dbFork.AddNewForkContext(ctxt);
}

bool CCheckForkManager::UpdateDbForkLast(const uint256& hashFork, const uint256& hashLastBlock)
{
    return dbFork.UpdateFork(hashFork, hashLastBlock);
}

bool CCheckForkManager::RemoveDbForkLast(const uint256& hashFork)
{
    return dbFork.RemoveFork(hashFork);
}

bool CCheckForkManager::GetValidForkContext(const uint256& hashPrimaryLastBlock, const uint256& hashFork, CForkContext& ctxt)
{
    if (GetValidForkCreatedHeight(hashPrimaryLastBlock, hashFork) < 0)
    {
        StdLog("check", "GetValidForkContext: find valid fork fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    const auto it = mapBlockForkSched.find(hashFork);
    if (it == mapBlockForkSched.end())
    {
        StdLog("check", "GetValidForkContext: find fork context fail, fork: %s", hashFork.GetHex().c_str());
        return false;
    }
    ctxt = it->second.ctxtFork;
    return true;
}

bool CCheckForkManager::IsCheckPoint(const uint256& hashFork, const uint256& hashBlock)
{
    auto it = mapCheckPoints.find(hashFork);
    if (it == mapCheckPoints.end())
    {
        return false;
    }
    auto mt = it->second.find(CBlock::GetBlockHeightByHash(hashBlock));
    if (mt == it->second.end())
    {
        return false;
    }
    if (mt->second != hashBlock)
    {
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////
// CCheckDelegateDB

bool CCheckDelegateDB::CheckDelegate(const uint256& hashBlock)
{
    CDelegateContext ctxtDelegate;
    return Retrieve(hashBlock, ctxtDelegate);
}

bool CCheckDelegateDB::UpdateDelegate(const uint256& hashBlock, const CBlockEx& block, uint32 nBlockFile, uint32 nBlockOffset)
{
    if (block.IsGenesis())
    {
        CDelegateContext ctxtDelegate;
        if (!AddNew(hashBlock, ctxtDelegate))
        {
            StdLog("check", "Update genesis delegate fail, block: %s", hashBlock.ToString().c_str());
            return false;
        }
        return true;
    }

    CDelegateContext ctxtDelegate;
    map<CDestination, int64>& mapDelegate = ctxtDelegate.mapVote;
    map<int, map<CDestination, CDiskPos>>& mapEnrollTx = ctxtDelegate.mapEnrollTx;
    if (!RetrieveDelegatedVote(block.hashPrev, mapDelegate))
    {
        StdError("check", "Update delegate vote: RetrieveDelegatedVote fail, hashPrev: %s", block.hashPrev.GetHex().c_str());
        return false;
    }

    {
        CTemplateId tid;
        if (block.txMint.sendTo.GetTemplateId(tid) && tid.GetType() == TEMPLATE_DELEGATE)
        {
            mapDelegate[block.txMint.sendTo] += block.txMint.nAmount;
        }
    }

    CBufStream ss;
    CVarInt var(block.vtx.size());
    uint32 nOffset = nBlockOffset + block.GetTxSerializedOffset()
                     + ss.GetSerializeSize(block.txMint)
                     + ss.GetSerializeSize(var);
    for (int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        CDestination destInDelegateTemplate;
        CDestination sendToDelegateTemplate;
        const CTxContxt& txContxt = block.vTxContxt[i];
        if (!CTemplateVote::ParseDelegateDest(txContxt.destIn, tx.sendTo, tx.vchSig, destInDelegateTemplate, sendToDelegateTemplate))
        {
            StdLog("check", "Update delegate vote: parse delegate dest fail, destIn: %s, sendTo: %s, block: %s, txid: %s",
                   CAddress(txContxt.destIn).ToString().c_str(), CAddress(tx.sendTo).ToString().c_str(), hashBlock.GetHex().c_str(), tx.GetHash().GetHex().c_str());
            return false;
        }
        if (!sendToDelegateTemplate.IsNull())
        {
            mapDelegate[sendToDelegateTemplate] += tx.nAmount;
        }
        if (!destInDelegateTemplate.IsNull())
        {
            mapDelegate[destInDelegateTemplate] -= (tx.nAmount + tx.nTxFee);
        }
        if (tx.nType == CTransaction::TX_CERT)
        {
            if (destInDelegateTemplate.IsNull())
            {
                StdLog("check", "Update delegate vote: TX_CERT destInDelegate is null, destInDelegate: %s, block: %s, txid: %s",
                       CAddress(destInDelegateTemplate).ToString().c_str(), hashBlock.GetHex().c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
            int nCertAnchorHeight = 0;
            try
            {
                CIDataStream is(tx.vchData);
                is >> nCertAnchorHeight;
            }
            catch (...)
            {
                StdLog("check", "Update delegate vote: TX_CERT vchData error, destInDelegate: %s, block: %s, txid: %s",
                       CAddress(destInDelegateTemplate).ToString().c_str(), hashBlock.GetHex().c_str(), tx.GetHash().GetHex().c_str());
                return false;
            }
            mapEnrollTx[nCertAnchorHeight].insert(make_pair(destInDelegateTemplate, CDiskPos(nBlockFile, nOffset)));
        }
        nOffset += ss.GetSerializeSize(tx);
    }

    if (!AddNew(hashBlock, ctxtDelegate))
    {
        StdError("check", "Update delegate context failed, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////
// CCheckForkTxPool

bool CCheckForkTxPool::AddTx(const uint256& txid, const CAssembledTx& tx)
{
    for (int i = 0; i < tx.vInput.size(); i++)
    {
        if (!Spent(tx.vInput[i].prevout))
        {
            StdError("check", "TxPool AddTx: Spent fail, txid: %s.", txid.GetHex().c_str());
            return false;
        }
    }
    if (!Unspent(CTxOutPoint(txid, 0), tx.GetOutput(0)))
    {
        StdError("check", "TxPool AddTx: Add unspent 0 fail, txid: %s.", txid.GetHex().c_str());
        return false;
    }
    if (!tx.IsRewardTx())
    {
        if (!Unspent(CTxOutPoint(txid, 1), tx.GetOutput(1)))
        {
            StdError("check", "TxPool AddTx: Add unspent 1 fail, txid: %s.", txid.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CCheckForkTxPool::Spent(const CTxOutPoint& point)
{
    auto it = mapTxPoolUnspent.find(point);
    if (it != mapTxPoolUnspent.end())
    {
        if (it->second.IsNull())
        {
            StdError("check", "TxPool Spent: Utxo has been spent, utxo: [%d] %s", point.n, point.hash.GetHex().c_str());
            return false;
        }
        mapTxPoolUnspent.erase(it);
    }
    else
    {
        if (!mapChainUnspent.count(point))
        {
            StdError("check", "TxPool Spent: Utxo not found on Chain, utxo: [%d] %s", point.n, point.hash.GetHex().c_str());
            return false;
        }
        mapTxPoolUnspent.insert(make_pair(point, CCheckTxOut()));
    }
    return true;
}

bool CCheckForkTxPool::Unspent(const CTxOutPoint& point, const CTxOut& out)
{
    if (!out.IsNull())
    {
        auto it = mapTxPoolUnspent.find(point);
        if (it == mapTxPoolUnspent.end())
        {
            if (mapChainUnspent.count(point))
            {
                StdError("check", "TxPool Unspent: Utxo already exists on chain, utxo: [%d] %s", point.n, point.hash.GetHex().c_str());
                return false;
            }
            mapTxPoolUnspent.insert(make_pair(point, CCheckTxOut(out)));
        }
        else
        {
            if (!it->second.IsNull())
            {
                StdError("check", "TxPool Unspent: Utxo already exists in txpool, utxo: [%d] %s", point.n, point.hash.GetHex().c_str());
                return false;
            }
            if (!mapChainUnspent.count(point))
            {
                StdError("check", "TxPool Unspent: Utxo not found on Chain, utxo: [%d] %s", point.n, point.hash.GetHex().c_str());
                return false;
            }
            mapTxPoolUnspent.erase(it);
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////
// CCheckTxPoolData

bool CCheckTxPoolData::CheckTxPool(const string& strPath, const bool fOnlyCheck)
{
    CTxPoolData datTxPool;
    if (!datTxPool.Initialize(path(strPath)))
    {
        StdLog("check", "TxPool: Failed to initialize txpool data");
        return false;
    }

    bool fCheckRet = true;
    vector<pair<uint256, pair<uint256, CAssembledTx>>> vTx;
    if (!datTxPool.LoadCheck(vTx))
    {
        StdLog("check", "TxPool: Load txpool data failed");
        fCheckRet = false;
    }
    else
    {
        for (int i = 0; i < vTx.size(); i++)
        {
            const uint256& hashFork = vTx[i].first;
            if (hashFork == 0)
            {
                StdError("check", "TxPool: tx fork hash is 0, txid: %s", vTx[i].second.first.GetHex().c_str());
                fCheckRet = false;
                break;
            }
            auto it = mapForkTxPool.find(hashFork);
            if (it == mapForkTxPool.end())
            {
                auto mt = mapLinkCheckFork.find(hashFork);
                if (mt == mapLinkCheckFork.end())
                {
                    StdError("check", "TxPool: tx fork not exists, fork: %s", hashFork.GetHex().c_str());
                    fCheckRet = false;
                    break;
                }
                it = mapForkTxPool.insert(make_pair(hashFork, CCheckForkTxPool(mt->second.mapBlockUnspent))).first;
            }
            if (!it->second.AddTx(vTx[i].second.first, vTx[i].second.second))
            {
                StdError("check", "TxPool: Add tx fail, txid: %s", vTx[i].second.first.GetHex().c_str());
                fCheckRet = false;
                break;
            }
        }
    }
    if (!fCheckRet)
    {
        StdLog("check", "TxPool: Check txpool file failed");
        if (fOnlyCheck)
        {
            return false;
        }
        if (!datTxPool.Remove())
        {
            StdLog("check", "TxPool: Remove file failed");
            return false;
        }
        StdLog("check", "TxPool: Remove file success");
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////
// CCheckBlockFork

bool CCheckBlockFork::AddForkBlock(const CBlockEx& block, CBlockIndex* pBlockIndex)
{
    if (pBlockIndex->IsOrigin())
    {
        if (pOrigin != nullptr)
        {
            StdLog("check", "Add fork block: pOrigin is not NULL");
            return false;
        }
        pOrigin = pBlockIndex;
    }
    if (pLast == nullptr
        || !(pLast->nChainTrust > pBlockIndex->nChainTrust
             || (pLast->nChainTrust == pBlockIndex->nChainTrust && !pBlockIndex->IsEquivalent(pLast))))
    {
        if (pLast && pBlockIndex->pPrev != pLast)
        {
            // rollback
            vector<CBlockIndex*> vNewPath;

            CBlockIndex* pBranch = GetBranch(pLast, pBlockIndex, vNewPath);

            // remove block
            for (CBlockIndex* p = pLast; p != pBranch; p = p->pPrev)
            {
                CBlockEx block;
                if (!tsBlock.Read(block, p->nFile, p->nOffset))
                {
                    StdError("check", "Add fork block: Read remove block fail, block: %s", p->GetBlockHash().GetHex().c_str());
                    return false;
                }
                if (!RemoveBlockData(block, p))
                {
                    StdError("check", "Add fork block: Remove block fail, block: %s", p->GetBlockHash().GetHex().c_str());
                    return false;
                }
            }

            // new block
            for (int64 i = vNewPath.size() - 1; i >= 0; i--)
            {
                CBlockEx block;
                CBlockIndex* pIndex = vNewPath[i];
                if (!tsBlock.Read(block, pIndex->nFile, pIndex->nOffset))
                {
                    StdError("check", "Add fork block: Read new block fail, block: %s", pIndex->GetBlockHash().GetHex().c_str());
                    return false;
                }
                if (!AddBlockData(block, pIndex))
                {
                    StdError("check", "Add fork block: Add block fail, block: %s", pIndex->GetBlockHash().GetHex().c_str());
                    return false;
                }
            }
        }
        else
        {
            if (!AddBlockData(block, pBlockIndex))
            {
                StdError("check", "Add fork block: Add block fail, block: %s", pBlockIndex->GetBlockHash().GetHex().c_str());
                return false;
            }
        }
        pLast = pBlockIndex;
    }
    return true;
}

CBlockIndex* CCheckBlockFork::GetBranch(CBlockIndex* pIndexRef, CBlockIndex* pIndex, vector<CBlockIndex*>& vPath)
{
    vPath.clear();
    while (pIndex != pIndexRef)
    {
        if (pIndexRef->GetBlockTime() > pIndex->GetBlockTime())
        {
            pIndexRef = pIndexRef->pPrev;
        }
        else if (pIndex->GetBlockTime() > pIndexRef->GetBlockTime())
        {
            vPath.push_back(pIndex);
            pIndex = pIndex->pPrev;
        }
        else
        {
            vPath.push_back(pIndex);
            pIndex = pIndex->pPrev;
            pIndexRef = pIndexRef->pPrev;
        }
    }
    return pIndex;
}

bool CCheckBlockFork::AddBlockData(const CBlockEx& block, const CBlockIndex* pBlockIndex)
{
    if (!block.IsNull() && (!block.IsVacant() || !block.txMint.sendTo.IsNull()))
    {
        const uint256& hashFork = pBlockIndex->GetOriginHash();

        int nBlockSeq = 0;
        if (fCheckAddrTxIndex && pBlockIndex->IsExtended())
        {
            nBlockSeq = pBlockIndex->GetExtendedSequence();
        }

        CBufStream ss;
        uint32 nTxOffset = pBlockIndex->nOffset + block.GetTxSerializedOffset();
        if (!AddBlockTx(block.txMint, CTxContxt(), block.GetBlockHeight(), nBlockSeq, 0, hashFork, pBlockIndex->nFile, nTxOffset))
        {
            StdError("check", "Add block data: Add mint tx fail, txid: %s, block: %s",
                     block.txMint.GetHash().GetHex().c_str(), pBlockIndex->GetBlockHash().GetHex().c_str());
            return false;
        }
        nTxOffset += ss.GetSerializeSize(block.txMint);

        CVarInt var(block.vtx.size());
        nTxOffset += ss.GetSerializeSize(var);
        for (size_t i = 0; i < block.vtx.size(); i++)
        {
            if (!AddBlockTx(block.vtx[i], block.vTxContxt[i], block.GetBlockHeight(), nBlockSeq, i + 1, hashFork, pBlockIndex->nFile, nTxOffset))
            {
                StdError("check", "Add block data: Add tx fail, txid: %s, block: %s",
                         block.vtx[i].GetHash().GetHex().c_str(), pBlockIndex->GetBlockHash().GetHex().c_str());
                return false;
            }
            nTxOffset += ss.GetSerializeSize(block.vtx[i]);
        }

        if (fCheckAddrTxIndex)
        {
            if (objForkManager.IsCheckPoint(hashFork, pBlockIndex->GetBlockHash()))
            {
                if (!CheckForkAddressTxIndex(hashFork, pBlockIndex->GetBlockHeight()))
                {
                    StdLog("check", "Check address tx index fail");
                    return false;
                }
            }
            else
            {
                if (mapBlockTxInfo.size() >= MAX_CACHE_BLOCK_TXINFO_COUNT || nCacheTxInfoBlockCount >= MAX_CACHE_BLOCK_COUNT)
                {
                    int nDposBlockCount = 0;
                    int nCheckCount = 0;
                    int nCheckHeight = pBlockIndex->GetBlockHeight() - DEF_CHECK_HEIGHT_DEPTH;
                    if (nCheckHeight < 0)
                    {
                        nCheckHeight = 0;
                    }
                    const CBlockIndex* pIndex = pBlockIndex;
                    while (pIndex)
                    {
                        if ((pIndex->IsPrimary() && !pIndex->IsProofOfWork())
                            || (!pIndex->IsPrimary() && pIndex->IsSubsidiary()))
                        {
                            if (++nDposBlockCount >= DPOS_CONFIRM_HEIGHT)
                            {
                                nCheckHeight = pIndex->GetBlockHeight();
                                break;
                            }
                        }
                        if (++nCheckCount >= nCacheTxInfoBlockCount)
                        {
                            break;
                        }
                        pIndex = pIndex->pPrev;
                    }
                    if (!CheckForkAddressTxIndex(hashFork, nCheckHeight))
                    {
                        StdLog("check", "Check address tx index fail");
                        return false;
                    }
                }
            }
        }
        ++nCacheTxInfoBlockCount;
    }
    return true;
}

bool CCheckBlockFork::RemoveBlockData(const CBlockEx& block, const CBlockIndex* pBlockIndex)
{
    if (!block.IsNull() && (!block.IsVacant() || !block.txMint.sendTo.IsNull()))
    {
        for (int64 i = block.vtx.size() - 1; i >= 0; i--)
        {
            if (!RemoveBlockTx(block.vtx[i], block.vTxContxt[i]))
            {
                StdLog("check", "Remove block data: Remove block tx fail, txid: %s.", block.vtx[i].GetHash().GetHex().c_str());
                return false;
            }
        }
        if (!block.txMint.sendTo.IsNull())
        {
            if (!RemoveBlockTx(block.txMint, CTxContxt()))
            {
                StdLog("check", "Remove block data: Remove block mint tx fail, txid: %s.", block.txMint.GetHash().GetHex().c_str());
                return false;
            }
        }
        --nCacheTxInfoBlockCount;
    }
    return true;
}

bool CCheckBlockFork::AddBlockTx(const CTransaction& txIn, const CTxContxt& contxtIn, const int nHeight, const int nBlockSeqNo, const int nTxSeqNo, const uint256& hashAtForkIn, uint32 nFileNoIn, uint32 nOffsetIn)
{
    const uint256 txid = txIn.GetHash();
    if (!mapBlockTxIndex.insert(make_pair(txid, CCheckTxIndex(txIn.nType, nHeight, nFileNoIn, nOffsetIn))).second)
    {
        StdLog("check", "AddBlockTx: add block tx index fail, txid: %s.", txid.GetHex().c_str());
        return false;
    }
    if (fCheckAddrTxIndex)
    {
        if (!mapBlockTxInfo.insert(make_pair(txid, CCheckTxInfo(contxtIn.destIn, txIn.sendTo, txIn.nType, txIn.nTimeStamp,
                                                                txIn.nLockUntil, txIn.nAmount, txIn.nTxFee, nHeight, nBlockSeqNo, nTxSeqNo)))
                 .second)
        {
            StdError("check", "AddBlockTx: add block tx info fail, txid: %s.", txid.GetHex().c_str());
        }
    }

    for (size_t i = 0; i < txIn.vInput.size(); i++)
    {
        if (!AddBlockSpent(txIn.vInput[i].prevout))
        {
            StdLog("check", "AddBlockTx: block spent fail, txid: %s.", txid.GetHex().c_str());
            return false;
        }
    }
    if (!AddBlockUnspent(CTxOutPoint(txid, 0), CTxOut(txIn), txIn.nType, nHeight))
    {
        StdLog("check", "AddBlockTx: add block unspent 0 fail, txid: %s.", txid.GetHex().c_str());
        return false;
    }
    if (!txIn.IsRewardTx())
    {
        if (!AddBlockUnspent(CTxOutPoint(txid, 1), CTxOut(txIn, contxtIn.destIn, contxtIn.GetValueIn()), txIn.nType, nHeight))
        {
            StdLog("check", "AddBlockTx: add block unspent 1 fail, txid: %s.", txid.GetHex().c_str());
            return false;
        }
    }

    if (txIn.IsDeFiRelation() && txIn.sendTo != contxtIn.destIn)
    {
        if (mapBlockAddress.find(txIn.sendTo) == mapBlockAddress.end())
        {
            mapBlockAddress.insert(make_pair(txIn.sendTo, make_pair(txid, CAddrInfo(CDestination(), contxtIn.destIn))));
        }
    }
    if (txIn.nType == CTransaction::TX_DEFI_MINT_HEIGHT)
    {
        if (txIn.vchData.size() >= sizeof(int32))
        {
            CIDataStream is(txIn.vchData);
            is >> nMintHeight;
            txidMintHeight = txid;
        }
    }
    return true;
}

bool CCheckBlockFork::RemoveBlockTx(const CTransaction& txIn, const CTxContxt& contxtIn)
{
    uint256 txid = txIn.GetHash();

    for (int i = 0; i < txIn.vInput.size(); i++)
    {
        const CTxOutPoint& txpoint = txIn.vInput[i].prevout;
        auto it = mapBlockTxIndex.find(txpoint.hash);
        if (it == mapBlockTxIndex.end())
        {
            it = mapParentForkBlockTxIndex.find(txpoint.hash);
            if (it == mapParentForkBlockTxIndex.end())
            {
                StdLog("check", "Remove Block Tx: Find tx index fail, txid: %s, utxo: [%d] %s.",
                       txid.GetHex().c_str(), txpoint.n, txpoint.hash.GetHex().c_str());
                return false;
            }
        }
        const CTxInContxt& in = contxtIn.vin[i];
        if (!AddBlockUnspent(txpoint, CTxOut(contxtIn.destIn, in.nAmount, in.nTxTime, in.nLockUntil), it->second.nTxType, it->second.nBlockHeight))
        {
            StdLog("check", "Remove Block Tx: Add unspent fail, txid: %s.", txid.GetHex().c_str());
            return false;
        }
    }

    CTxOut output0(txIn);
    if (!output0.IsNull())
    {
        if (!AddBlockSpent(CTxOutPoint(txid, 0)))
        {
            StdLog("check", "Remove Block Tx: Add spent 0 fail, txid: %s.", txid.GetHex().c_str());
            return false;
        }
    }

    if (!txIn.IsRewardTx())
    {
        CTxOut output1(txIn, contxtIn.destIn, contxtIn.GetValueIn());
        if (!output1.IsNull())
        {
            if (!AddBlockSpent(CTxOutPoint(txid, 1)))
            {
                StdLog("check", "Remove Block Tx: Add spent 1 fail, txid: %s.", txid.GetHex().c_str());
                return false;
            }
        }
    }

    mapBlockTxIndex.erase(txid);

    if (fCheckAddrTxIndex)
    {
        auto it = mapBlockTxInfo.find(txid);
        if (it == mapBlockTxInfo.end())
        {
            StdError("check", "Remove Block Tx: Find block tx info fail, txid: %s.", txid.GetHex().c_str());
        }
        else
        {
            mapBlockTxInfo.erase(it);
        }
    }

    if (txIn.IsDeFiRelation() && txIn.sendTo != contxtIn.destIn)
    {
        auto it = mapBlockAddress.find(txIn.sendTo);
        if (it != mapBlockAddress.end() && it->second.first == txid)
        {
            mapBlockAddress.erase(it);
        }
    }

    if (txIn.nType == CTransaction::TX_DEFI_MINT_HEIGHT && txid == txidMintHeight)
    {
        nMintHeight = -1;
    }
    return true;
}

bool CCheckBlockFork::AddBlockUnspent(const CTxOutPoint& txPoint, const CTxOut& txOut, int nTxType, int nHeight)
{
    if (!txOut.IsNull())
    {
        if (!mapBlockUnspent.insert(make_pair(txPoint, CCheckTxOut(txOut, nTxType, nHeight))).second)
        {
            StdLog("check", "Add Block Unspent: Add block unspent fail, utxo: [%d] %s.", txPoint.n, txPoint.hash.GetHex().c_str());
            return false;
        }
    }
    return true;
}

bool CCheckBlockFork::AddBlockSpent(const CTxOutPoint& txPoint)
{
    map<CTxOutPoint, CCheckTxOut>::iterator it = mapBlockUnspent.find(txPoint);
    if (it == mapBlockUnspent.end())
    {
        StdLog("check", "Add Block Spent: utxo find fail, utxo: [%d] %s.", txPoint.n, txPoint.hash.GetHex().c_str());
        return false;
    }
    mapBlockUnspent.erase(it);
    return true;
}

bool CCheckBlockFork::InheritCopyData(const CCheckBlockFork& fromParent, const CBlockIndex* pJointBlockIndex)
{
    mapBlockUnspent.clear();
    mapBlockUnspent.insert(fromParent.mapBlockUnspent.begin(), fromParent.mapBlockUnspent.end());

    mapParentForkBlockTxIndex.clear();
    mapParentForkBlockTxIndex.insert(fromParent.mapParentForkBlockTxIndex.begin(), fromParent.mapParentForkBlockTxIndex.end());

    mapBlockTxIndex.clear();
    mapBlockTxIndex.insert(fromParent.mapBlockTxIndex.begin(), fromParent.mapBlockTxIndex.end());

    mapBlockTxInfo.clear();
    mapBlockTxInfo.insert(fromParent.mapBlockTxInfo.begin(), fromParent.mapBlockTxInfo.end());

    mapBlockAddress.clear();
    mapBlockAddress.insert(fromParent.mapBlockAddress.begin(), fromParent.mapBlockAddress.end());

    // remove block
    for (CBlockIndex* p = fromParent.pLast; p != nullptr && p != pJointBlockIndex; p = p->pPrev)
    {
        CBlockEx block;
        if (!tsBlock.Read(block, p->nFile, p->nOffset))
        {
            StdError("check", "Inherit copy data: Read remove block fail, block: %s", p->GetBlockHash().GetHex().c_str());
            return false;
        }
        if (!RemoveBlockData(block, p))
        {
            StdError("check", "Inherit copy data: Remove block fail, block: %s", p->GetBlockHash().GetHex().c_str());
            return false;
        }
    }

    if (!mapBlockTxIndex.empty())
    {
        mapParentForkBlockTxIndex.insert(mapBlockTxIndex.begin(), mapBlockTxIndex.end());
        mapBlockTxIndex.clear();
    }
    mapBlockTxInfo.clear();
    mapBlockAddress.clear();
    nCacheTxInfoBlockCount = 0;
    return true;
}

bool CCheckBlockFork::CheckForkAddressTxIndex(const uint256& hashFork, const int nCheckHeight)
{
    if (!dbAddressTxIndex.LoadFork(hashFork))
    {
        StdLog("check", "Check fork address tx index: dbAddressTxIndex LoadFork fail");
        return false;
    }

    vector<pair<CAddrTxIndex, CAddrTxInfo>> vAddUpdate;
    vector<CAddrTxIndex> vRemove;
    for (auto nt = mapBlockTxInfo.begin(); nt != mapBlockTxInfo.end();)
    {
        const uint256& txid = nt->first;
        const CCheckTxInfo& checkTxInfo = nt->second;

        if (checkTxInfo.nBlockHeight > nCheckHeight)
        {
            break;
        }

        if (!checkTxInfo.destFrom.IsNull())
        {
            bool fCheckRet = true;
            int nDirection = CAddrTxInfo::TXI_DIRECTION_FROM;
            if (checkTxInfo.destFrom == checkTxInfo.destTo)
            {
                nDirection = CAddrTxInfo::TXI_DIRECTION_TWO;
            }
            CAddrTxInfo txInfo(nDirection, checkTxInfo.destTo, checkTxInfo.nTxType, checkTxInfo.nTimeStamp,
                               checkTxInfo.nLockUntil, checkTxInfo.nAmount, checkTxInfo.nTxFee);
            CAddrTxIndex txIndex(checkTxInfo.destFrom, checkTxInfo.nBlockHeight, checkTxInfo.nBlockSeqNo, checkTxInfo.nTxSeqNo, txid);
            CAddrTxInfo addrTxInfo;
            if (!dbAddressTxIndex.RetrieveTxIndex(hashFork, txIndex, addrTxInfo))
            {
                StdLog("check", "Check address tx index: Retrieve address tx index fail 1, height: %d, tx: %s, destFrom: %s, destTo: %s.",
                       nt->second.nBlockHeight, txid.GetHex().c_str(),
                       CAddress(checkTxInfo.destFrom).ToString().c_str(),
                       CAddress(checkTxInfo.destTo).ToString().c_str());
                fCheckRet = false;
            }
            else if (addrTxInfo != txInfo)
            {
                StdLog("check", "Check address tx index: Address tx info error 1, height: %d, tx: %s, destFrom: %s, destTo: %s.",
                       nt->second.nBlockHeight, txid.GetHex().c_str(),
                       CAddress(checkTxInfo.destFrom).ToString().c_str(),
                       CAddress(checkTxInfo.destTo).ToString().c_str());
                fCheckRet = false;
            }
            if (!fCheckRet)
            {
                vAddUpdate.push_back(make_pair(txIndex, txInfo));
            }
        }

        if (!checkTxInfo.destTo.IsNull() && checkTxInfo.destFrom != checkTxInfo.destTo)
        {
            bool fCheckRet = true;
            CAddrTxInfo txInfo(CAddrTxInfo::TXI_DIRECTION_TO, checkTxInfo.destFrom, checkTxInfo.nTxType, checkTxInfo.nTimeStamp,
                               checkTxInfo.nLockUntil, checkTxInfo.nAmount, checkTxInfo.nTxFee);
            CAddrTxIndex txIndex(checkTxInfo.destTo, checkTxInfo.nBlockHeight, checkTxInfo.nBlockSeqNo, checkTxInfo.nTxSeqNo, txid);
            CAddrTxInfo addrTxInfo;
            if (!dbAddressTxIndex.RetrieveTxIndex(hashFork, txIndex, addrTxInfo))
            {
                StdLog("check", "Check address tx index: Retrieve address tx index fail 2, height: %d, tx: %s, destFrom: %s, destTo: %s.",
                       nt->second.nBlockHeight, txid.GetHex().c_str(),
                       CAddress(checkTxInfo.destFrom).ToString().c_str(),
                       CAddress(checkTxInfo.destTo).ToString().c_str());
                fCheckRet = false;
            }
            else if (addrTxInfo != txInfo)
            {
                StdLog("check", "Check address tx index: Address tx info error 2, height: %d, tx: %s, destFrom: %s, destTo: %s.",
                       nt->second.nBlockHeight, txid.GetHex().c_str(),
                       CAddress(checkTxInfo.destFrom).ToString().c_str(),
                       CAddress(checkTxInfo.destTo).ToString().c_str());
                fCheckRet = false;
            }
            if (!fCheckRet)
            {
                vAddUpdate.push_back(make_pair(txIndex, txInfo));
            }
        }

        mapBlockTxInfo.erase(nt++);
    }

    if (!fOnlyCheck && (!vAddUpdate.empty() || !vRemove.empty()))
    {
        if (!dbAddressTxIndex.RepairAddressTxIndex(hashFork, vAddUpdate, vRemove))
        {
            StdLog("check", "Check address tx index: Repair address tx index update fail");
            return false;
        }
    }

    nCacheTxInfoBlockCount = 0;
    return true;
}

/////////////////////////////////////////////////////////////////////////
// CCheckBlockWalker

CCheckBlockWalker::~CCheckBlockWalker()
{
    Uninitialize();
}

bool CCheckBlockWalker::Initialize(const string& strPath)
{
    strDataPath = strPath;
    if (!objDelegateDB.Initialize(path(strPath)))
    {
        StdError("check", "Block walker: Delegate db initialize fail, path: %s.", strPath.c_str());
        return false;
    }

    if (!objTsBlock.Initialize(path(strPath) / "block", BLOCKFILE_PREFIX))
    {
        StdError("check", "Block walker: TsBlock initialize fail, path: %s.", strPath.c_str());
        return false;
    }

    if (!dbBlockIndex.Initialize(path(strPath)))
    {
        StdLog("check", "dbBlockIndex Initialize fail");
        return false;
    }

    if (fCheckAddrTxIndex)
    {
        if (!dbAddressTxIndex.Initialize(path(strPath), false))
        {
            StdLog("check", "dbAddressTxIndex Initialize fail");
            return false;
        }
    }

    if (!dbInvest.Initialize(path(strPath)))
    {
        StdError("check", "Block walker: Invest db initialize fail, path: %s.", strPath.c_str());
        return false;
    }

    if (!dbActivate.Initialize(path(strPath)))
    {
        StdError("check", "Block walker: Activate db initialize fail, path: %s.", strPath.c_str());
        return false;
    }
    return true;
}

void CCheckBlockWalker::Uninitialize()
{
    ClearBlockIndex();
    dbBlockIndex.Deinitialize();
    objDelegateDB.Deinitialize();
    objTsBlock.Deinitialize();
    if (fCheckAddrTxIndex)
    {
        dbAddressTxIndex.Deinitialize();
    }
    dbInvest.Deinitialize();
    dbActivate.Deinitialize();
}

bool CCheckBlockWalker::Walk(const CBlockEx& block, uint32 nFile, uint32 nOffset)
{
    const uint256 hashBlock = block.GetHash();
    if (mapBlockIndex.count(hashBlock) > 0)
    {
        StdError("check", "Block walk: block exist, hash: %s.", hashBlock.GetHex().c_str());
        return true;
    }

    CBlockIndex* pIndexPrev = nullptr;
    if (block.hashPrev != 0)
    {
        map<uint256, CBlockIndex*>::iterator mt = mapBlockIndex.find(block.hashPrev);
        if (mt != mapBlockIndex.end())
        {
            pIndexPrev = mt->second;
        }
    }
    if (!block.IsGenesis() && pIndexPrev == nullptr)
    {
        StdError("check", "Block walk: Prev block not exist, block: %s.", hashBlock.GetHex().c_str());
        return false;
    }

    if (block.IsPrimary())
    {
        if (!objDelegateDB.CheckDelegate(hashBlock))
        {
            StdError("check", "Block walk: Check delegate vote fail, block: %s", hashBlock.GetHex().c_str());
            if (!fOnlyCheck)
            {
                if (!objDelegateDB.UpdateDelegate(hashBlock, block, nFile, nOffset))
                {
                    StdError("check", "Block walk: Update delegate fail, block: %s.", hashBlock.GetHex().c_str());
                    return false;
                }
            }
        }
        if (!objForkManager.AddBlockForkContext(block))
        {
            StdError("check", "Block walk: Add block fork fail, block: %s", hashBlock.GetHex().c_str());
            return false;
        }
    }

    CBlockIndex* pNewBlockIndex = AddBlockIndex(hashBlock, block, pIndexPrev, nFile, nOffset);
    if (pNewBlockIndex == nullptr)
    {
        StdError("check", "Block walk: Add block index fail, block: %s.", hashBlock.GetHex().c_str());
        return false;
    }
    if (pNewBlockIndex->GetOriginHash() == 0)
    {
        StdError("check", "Block walk: Get block origin hash is 0, block: %s.", hashBlock.GetHex().c_str());
        return false;
    }

    const uint256& hashFork = pNewBlockIndex->GetOriginHash();
    auto nt = mapCheckFork.find(hashFork);
    if (nt == mapCheckFork.end())
    {
        nt = mapCheckFork.insert(make_pair(hashFork, CCheckBlockFork(strDataPath, fOnlyCheck, fCheckAddrTxIndex, objTsBlock, objForkManager, dbAddressTxIndex))).first;
        if (block.IsOrigin() && !block.IsGenesis())
        {
            if (!InheritForkData(block, nt->second))
            {
                StdError("check", "Block walk: Get fork joint block fail, prev block: %s.", block.hashPrev.GetHex().c_str());
                return false;
            }
        }
    }
    CCheckBlockFork& checkBlockFork = nt->second;
    if (!checkBlockFork.AddForkBlock(block, pNewBlockIndex))
    {
        StdError("check", "Block walk: Add fork block fail, block: %s.", hashBlock.GetHex().c_str());
        return false;
    }

    if (!UpdateInvest(hashBlock, block))
    {
        StdError("check", "Block walk: Update invest fail, block: %s.", hashBlock.GetHex().c_str());
        return false;
    }

    if (!UpdateActivate(hashBlock, block))
    {
        StdError("check", "Block walk: Update activate fail, block: %s.", hashBlock.GetHex().c_str());
        return false;
    }

    if (block.IsGenesis())
    {
        if (block.GetHash() != objProofParam.hashGenesisBlock)
        {
            StdError("check", "Block walk: genesis block error, block hash: %s, genesis hash: %s.",
                     hashBlock.GetHex().c_str(), objProofParam.hashGenesisBlock.GetHex().c_str());
            return false;
        }
        hashGenesis = hashBlock;
    }
    nBlockCount++;
    if (nBlockCount % 100000 == 0)
    {
        StdLog("check", "Block walk: Fetch block count: %lu, height: %d.", nBlockCount, block.GetBlockHeight());
    }
    int64 nCurTime = GetTime();
    if (nCurTime - nBlockPrevTime >= 10)
    {
        nBlockPrevTime = nCurTime;
        StdLog("check", "Block walk: Fetch block count: %lu, height: %d.", nBlockCount, block.GetBlockHeight());
    }
    return true;
}

bool CCheckBlockWalker::InheritForkData(const CBlockEx& blockOrigin, CCheckBlockFork& subBlockFork)
{
    uint256 hashNewOriginBlock = blockOrigin.GetHash();

    auto nt = mapCheckFork.find(objProofParam.hashGenesisBlock);
    if (nt == mapCheckFork.end() || nt->second.pLast == nullptr)
    {
        StdError("check", "Inherit fork data: Genesis fork not find, genesis fork: %s.",
                 objProofParam.hashGenesisBlock.GetHex().c_str());
        return false;
    }

    CForkContext ctxt;
    if (!objForkManager.GetValidForkContext(nt->second.pLast->GetBlockHash(), hashNewOriginBlock, ctxt))
    {
        StdError("check", "Inherit fork data: fork not find or invalid, fork: %s.", hashNewOriginBlock.GetHex().c_str());
        return false;
    }
    if (!ctxt.IsIsolated())
    {
        auto it = mapBlockIndex.find(blockOrigin.hashPrev);
        if (it == mapBlockIndex.end() || it->second == nullptr)
        {
            StdError("check", "Inherit fork data: Find fork joint block fail, origin block: %s, prev block: %s.",
                     hashNewOriginBlock.GetHex().c_str(), blockOrigin.hashPrev.GetHex().c_str());
            return false;
        }
        CBlockIndex* pPrevBlockIndex = it->second;
        const uint256& hashParentFork = pPrevBlockIndex->GetOriginHash();

        auto mt = mapCheckFork.find(hashParentFork);
        if (mt == mapCheckFork.end())
        {
            StdError("check", "Inherit fork data: Find parent fork fail, origin block: %s, parent fork: %s.",
                     hashNewOriginBlock.GetHex().c_str(), hashParentFork.GetHex().c_str());
            return false;
        }

        if (!subBlockFork.InheritCopyData(mt->second, pPrevBlockIndex))
        {
            StdError("check", "Inherit fork data: Copy data fail, origin block: %s.", hashNewOriginBlock.GetHex().c_str());
            return false;
        }
    }
    return true;
}

CBlockIndex* CCheckBlockWalker::AddBlockIndex(const uint256& hashBlock, const CBlockEx& block, CBlockIndex* pIndexPrev, uint32 nFile, uint32 nOffset)
{
    if (block.IsSubsidiary() || block.IsExtended())
    {
        CProofOfPiggyback proof;
        if (!proof.Load(block.vchProof) || proof.hashRefBlock == 0)
        {
            StdError("check", "Add block index: subfork vchProof error, block: %s", hashBlock.GetHex().c_str());
            return nullptr;
        }
        mapRefBlock[hashBlock] = proof.hashRefBlock;
    }

    CBlockIndex* pNewBlockIndex = nullptr;
    CBlockOutline blockOutline;
    if (!dbBlockIndex.RetrieveBlock(hashBlock, blockOutline))
    {
        StdLog("check", "Add block index: Check db block index fail, block: %s", hashBlock.GetHex().c_str());
        pNewBlockIndex = AddNewIndex(hashBlock, static_cast<const CBlock&>(block), nFile, nOffset, block.nBlockTrust);
        if (pNewBlockIndex == nullptr)
        {
            StdError("check", "Add block index: Add new block index fail 1, block: %s.", hashBlock.GetHex().c_str());
            return nullptr;
        }

        if (!fOnlyCheck)
        {
            if (!dbBlockIndex.AddNewBlock(CBlockOutline(pNewBlockIndex)))
            {
                StdError("check", "Add block index: Add db block fail, block: %s.", hashBlock.GetHex().c_str());
                return nullptr;
            }
        }
    }
    else
    {
        pNewBlockIndex = AddNewIndex(hashBlock, blockOutline);
        if (pNewBlockIndex == nullptr)
        {
            StdError("check", "Add block index: Add new block index fail 2, block: %s.", hashBlock.GetHex().c_str());
            return nullptr;
        }
        if (pNewBlockIndex->nFile != nFile || pNewBlockIndex->nOffset != nOffset)
        {
            StdLog("check", "Add block index: Block index param error, block: %s, File: %d - %d, Offset: %d - %d.",
                   hashBlock.GetHex().c_str(), pNewBlockIndex->nFile, nFile, pNewBlockIndex->nOffset, nOffset);
            pNewBlockIndex->nFile = nFile;
            pNewBlockIndex->nOffset = nOffset;

            if (!fOnlyCheck)
            {
                if (!dbBlockIndex.AddNewBlock(CBlockOutline(pNewBlockIndex)))
                {
                    StdError("check", "Add block index: Add db block fail, block: %s.", hashBlock.GetHex().c_str());
                    return nullptr;
                }
            }
        }
    }
    return pNewBlockIndex;
}

bool CCheckBlockWalker::GetBlockTrust(const CBlockEx& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev, const CDelegateAgreement& agreement, const CBlockIndex* pIndexRef, size_t nEnrollTrust)
{
    //int32 nHeight = block.GetBlockHeight();
    if (block.IsGenesis())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsVacant())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsPrimary())
    {
        if (block.IsProofOfWork())
        {
            // PoW difficulty = 2 ^ nBits
            CProofOfHashWorkCompact proof;
            proof.Load(block.vchProof);
            uint256 v(1);
            nChainTrust = v << proof.nBits;
        }
        else if (pIndexPrev != nullptr)
        {
            // Get the last PoW block nAlgo
            int nAlgo;
            const CBlockIndex* pIndex = pIndexPrev;
            while (!pIndex->IsProofOfWork() && (pIndex->pPrev != nullptr))
            {
                pIndex = pIndex->pPrev;
            }
            if (!pIndex->IsProofOfWork())
            {
                nAlgo = CM_CRYPTONIGHT;
            }
            else
            {
                nAlgo = pIndex->nProofAlgo;
            }

            int nBits;
            if (GetProofOfWorkTarget(pIndexPrev, nAlgo, nBits))
            {
                if (agreement.nWeight == 0 || nBits <= 0)
                {
                    StdError("check", "GetBlockTrust: nWeight or nBits error, nWeight: %lu, nBits: %d", agreement.nWeight, nBits);
                    return false;
                }
                if (nEnrollTrust <= 0)
                {
                    StdError("check", "GetBlockTrust: nEnrollTrust error, nEnrollTrust: %lu", nEnrollTrust);
                    return false;
                }

                // DPoS difficulty = 2 ^ (nBits + weight)
                nChainTrust = uint256(1) << (int(nEnrollTrust) + nBits + 20);
            }
            else
            {
                StdError("check", "GetBlockTrust: GetProofOfWorkTarget fail");
                return false;
            }
        }
        else
        {
            StdError("check", "GetBlockTrust: Primary pIndexPrev is null");
            return false;
        }
    }
    else if (block.IsOrigin())
    {
        nChainTrust = uint64(0);
    }
    else if ((block.IsSubsidiary() || block.IsExtended()) && (pIndexRef != nullptr))
    {
        if (pIndexRef->pPrev == nullptr)
        {
            StdError("check", "GetBlockTrust: Subsidiary or Extended block pPrev is null, block: %s", block.GetHash().GetHex().c_str());
            return false;
        }
        nChainTrust = pIndexRef->nChainTrust - pIndexRef->pPrev->nChainTrust;
    }
    else
    {
        StdError("check", "GetBlockTrust: block type error");
        return false;
    }
    return true;
}

bool CCheckBlockWalker::GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits)
{
    nBits = 8;
    return true;
}

bool CCheckBlockWalker::GetBlockDelegateAgreement(const uint256& hashBlock, const CBlock& block, CBlockIndex* pIndexPrev, CDelegateAgreement& agreement, size_t& nEnrollTrust)
{
    agreement.Clear();
    if (pIndexPrev->GetBlockHeight() < CONSENSUS_INTERVAL - 1)
    {
        return true;
    }

    CBlockIndex* pIndex = pIndexPrev;
    for (int i = 0; i < CONSENSUS_DISTRIBUTE_INTERVAL; i++)
    {
        if (pIndex == nullptr)
        {
            StdLog("check", "GetBlockDelegateAgreement : pIndex is null, block: %s", hashBlock.ToString().c_str());
            return false;
        }
        pIndex = pIndex->pPrev;
    }

    CDelegateEnrolled enrolled;
    if (!GetBlockDelegateEnrolled(pIndex->GetBlockHash(), pIndex, enrolled))
    {
        StdLog("check", "GetBlockDelegateAgreement : GetBlockDelegateEnrolled fail, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    delegate::CDelegateVerify verifier(enrolled.mapWeight, enrolled.mapEnrollData);
    map<CDestination, size_t> mapBallot;
    if (!verifier.VerifyProof(block.vchProof, agreement.nAgreement, agreement.nWeight, mapBallot, true))
    {
        StdLog("check", "GetBlockDelegateAgreement : Invalid block proof, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    nEnrollTrust = 0;
    for (auto& amount : enrolled.vecAmount)
    {
        if (mapBallot.find(amount.first) != mapBallot.end())
        {
            nEnrollTrust += (size_t)(min(amount.second, objProofParam.nDelegateProofOfStakeEnrollMaximumAmount));
        }
    }
    nEnrollTrust /= objProofParam.nDelegateProofOfStakeEnrollMinimumAmount;
    return true;
}

bool CCheckBlockWalker::GetBlockDelegateEnrolled(const uint256& hashBlock, CBlockIndex* pIndex, CDelegateEnrolled& enrolled)
{
    enrolled.Clear();
    if (pIndex == nullptr)
    {
        StdLog("check", "GetBlockDelegateEnrolled : pIndex is null, block: %s", hashBlock.ToString().c_str());
        return false;
    }

    if (pIndex->GetBlockHeight() < CONSENSUS_ENROLL_INTERVAL)
    {
        return true;
    }

    vector<uint256> vBlockRange;
    for (int i = 0; i < CONSENSUS_ENROLL_INTERVAL; i++)
    {
        if (pIndex == nullptr)
        {
            StdLog("check", "GetBlockDelegateEnrolled : pIndex is null 2, block: %s", hashBlock.ToString().c_str());
            return false;
        }
        vBlockRange.push_back(pIndex->GetBlockHash());
        pIndex = pIndex->pPrev;
    }

    if (!RetrieveAvailDelegate(hashBlock, pIndex->GetBlockHeight(), vBlockRange, objProofParam.nDelegateProofOfStakeEnrollMinimumAmount,
                               enrolled.mapWeight, enrolled.mapEnrollData, enrolled.vecAmount))
    {
        StdLog("check", "GetBlockDelegateEnrolled : Retrieve Avail Delegate Error, block: %s", hashBlock.ToString().c_str());
        return false;
    }
    return true;
}

bool CCheckBlockWalker::RetrieveAvailDelegate(const uint256& hash, int height, const vector<uint256>& vBlockRange,
                                              int64 nMinEnrollAmount,
                                              map<CDestination, size_t>& mapWeight,
                                              map<CDestination, vector<unsigned char>>& mapEnrollData,
                                              vector<pair<CDestination, int64>>& vecAmount)
{
    map<CDestination, int64> mapVote;
    if (!objDelegateDB.RetrieveDelegatedVote(hash, mapVote))
    {
        StdLog("check", "RetrieveAvailDelegate: Retrieve Delegated Vote fail, block: %s", hash.ToString().c_str());
        return false;
    }

    map<CDestination, CDiskPos> mapEnrollTxPos;
    if (!objDelegateDB.RetrieveEnrollTx(height, vBlockRange, mapEnrollTxPos))
    {
        StdLog("check", "RetrieveAvailDelegate: Retrieve Enroll Tx fail, block: %s, height: %d", hash.ToString().c_str(), height);
        return false;
    }

    map<pair<int64, CDiskPos>, pair<CDestination, vector<uint8>>> mapSortEnroll;
    for (map<CDestination, int64>::iterator it = mapVote.begin(); it != mapVote.end(); ++it)
    {
        if ((*it).second >= nMinEnrollAmount)
        {
            const CDestination& dest = (*it).first;
            map<CDestination, CDiskPos>::iterator mi = mapEnrollTxPos.find(dest);
            if (mi != mapEnrollTxPos.end())
            {
                CTransaction tx;
                if (!objTsBlock.Read(tx, (*mi).second))
                {
                    StdLog("check", "RetrieveAvailDelegate: Read tx fail, txid: %s", tx.GetHash().ToString().c_str());
                    return false;
                }

                if (tx.vchData.size() <= sizeof(int))
                {
                    StdLog("check", "RetrieveAvailDelegate: tx.vchData error, txid: %s", tx.GetHash().ToString().c_str());
                    return false;
                }
                std::vector<uint8> vchCertData;
                vchCertData.assign(tx.vchData.begin() + sizeof(int), tx.vchData.end());

                mapSortEnroll.insert(make_pair(make_pair(it->second, mi->second), make_pair(dest, vchCertData)));
            }
        }
    }

    // first 25 destination sorted by amount and sequence
    for (auto it = mapSortEnroll.rbegin(); it != mapSortEnroll.rend() && mapWeight.size() < MAX_DELEGATE_THRESH; it++)
    {
        mapWeight.insert(make_pair(it->second.first, 1));
        mapEnrollData.insert(make_pair(it->second.first, it->second.second));
        vecAmount.push_back(make_pair(it->second.first, it->first.first));
    }
    return true;
}

bool CCheckBlockWalker::UpdateBlockNext()
{
    map<uint256, CCheckBlockFork>::iterator it = mapCheckFork.begin();
    while (it != mapCheckFork.end())
    {
        CCheckBlockFork& checkFork = it->second;
        if (checkFork.pOrigin == nullptr || checkFork.pLast == nullptr)
        {
            if (checkFork.pOrigin == nullptr)
            {
                StdError("check", "UpdateBlockNext: pOrigin is null, fork: %s", it->first.GetHex().c_str());
            }
            else
            {
                StdError("check", "UpdateBlockNext: pLast is null, fork: %s", it->first.GetHex().c_str());
            }
            mapCheckFork.erase(it++);
            continue;
        }
        CBlockIndex* pBlockIndex = checkFork.pLast;
        while (pBlockIndex && !pBlockIndex->IsOrigin())
        {
            if (pBlockIndex->pPrev)
            {
                pBlockIndex->pPrev->pNext = pBlockIndex;
            }
            pBlockIndex = pBlockIndex->pPrev;
        }
        if (it->first == hashGenesis)
        {
            nMainChainHeight = checkFork.pLast->GetBlockHeight();
        }
        ++it;
    }
    return true;
}

bool CCheckBlockWalker::CheckRepairFork()
{
    for (auto it = objForkManager.mapBlockForkSched.begin(); it != objForkManager.mapBlockForkSched.end(); ++it)
    {
        CForkContext& ctxt = it->second.ctxtFork;
        const uint256& hashFork = ctxt.hashFork;

        auto mt = mapCheckFork.find(hashFork);
        if (mt != mapCheckFork.end() && mt->second.nMintHeight >= -1)
        {
            CProfile profile = ctxt.GetProfile();
            profile.defi.nMintHeight = mt->second.nMintHeight;
            ctxt.vchDeFi.clear();
            profile.defi.Save(ctxt.vchDeFi);
        }

        bool fCheckRet = true;
        CForkContext ctxtDb;
        if (!objForkManager.GetDbForkContext(hashFork, ctxtDb))
        {
            StdLog("check", "Check repair fork: Find fork context fail, fork: %s", hashFork.GetHex().c_str());
            fCheckRet = false;
        }
        else if (ctxt != ctxtDb)
        {
            StdLog("check", "Check repair fork: Fork context error, fork: %s", hashFork.GetHex().c_str());
            fCheckRet = false;
        }
        if (!fCheckRet && !fOnlyCheck)
        {
            if (!objForkManager.UpdateDbForkContext(ctxt))
            {
                StdLog("check", "Check repair fork: Add new fork context fail, fork: %s", hashFork.GetHex().c_str());
                return false;
            }
        }
    }

    set<uint256> setLastFork;
    map<uint256, CCheckBlockFork>::iterator it = mapCheckFork.begin();
    while (it != mapCheckFork.end())
    {
        const uint256& hashFork = it->first;
        CCheckBlockFork& checkFork = it->second;
        if (checkFork.pLast == nullptr)
        {
            StdLog("check", "Check repair fork: pLast is null, fork: %s", hashFork.GetHex().c_str());
            return false;
        }
        if (checkFork.fInvalidFork)
        {
            auto mt = objForkManager.mapActiveFork.find(hashFork);
            if (mt != objForkManager.mapActiveFork.end())
            {
                StdLog("check", "Check repair fork: Joint block is non long chain, fork: %s", hashFork.GetHex().c_str());
                if (fOnlyCheck)
                {
                    return false;
                }
                objForkManager.mapActiveFork.erase(mt);
                objForkManager.RemoveDbForkLast(hashFork);
            }
        }
        else
        {
            bool fCheckRet = true;
            const uint256& hashLastBlock = checkFork.pLast->GetBlockHash();
            auto mt = objForkManager.mapActiveFork.find(hashFork);
            if (mt == objForkManager.mapActiveFork.end())
            {
                StdLog("check", "Check repair fork: Find fork fail, fork: %s", hashFork.GetHex().c_str());
                objForkManager.mapActiveFork.insert(make_pair(hashFork, hashLastBlock));
                fCheckRet = false;
            }
            else if (mt->second != hashLastBlock)
            {
                StdLog("check", "Check repair fork: last block error, fork: %s, db last: %s, block last: %s",
                       hashFork.GetHex().c_str(), mt->second.GetHex().c_str(), hashLastBlock.GetHex().c_str());
                mt->second = hashLastBlock;
                fCheckRet = false;
            }
            if (!fCheckRet)
            {
                if (fOnlyCheck)
                {
                    return false;
                }
                if (!objForkManager.UpdateDbForkLast(hashFork, hashLastBlock))
                {
                    StdLog("check", "Check repair fork: Update db fork last fail, fork: %s", hashFork.GetHex().c_str());
                    return false;
                }
            }
            setLastFork.insert(hashFork);
        }
        ++it;
    }

    for (auto it = objForkManager.mapActiveFork.begin(); it != objForkManager.mapActiveFork.end();)
    {
        const uint256& hashFork = it->first;
        if (!setLastFork.count(hashFork))
        {
            StdLog("check", "Check repair fork: Invalid last fork, fork: %s", hashFork.GetHex().c_str());
            if (fOnlyCheck)
            {
                return false;
            }
            objForkManager.mapActiveFork.erase(it++);
            objForkManager.RemoveDbForkLast(hashFork);
        }
        else
        {
            ++it;
        }
    }
    return true;
}

CBlockIndex* CCheckBlockWalker::AddNewIndex(const uint256& hash, const CBlock& block, uint32 nFile, uint32 nOffset, uint256 nChainTrust)
{
    CBlockIndex* pIndexNew = new CBlockIndex(block, nFile, nOffset);
    if (pIndexNew != nullptr)
    {
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.insert(make_pair(hash, pIndexNew)).first;
        if (mi == mapBlockIndex.end())
        {
            StdError("check", "AddNewIndex: insert fail, block: %s", hash.GetHex().c_str());
            return nullptr;
        }
        pIndexNew->phashBlock = &((*mi).first);

        int64 nMoneySupply = block.GetBlockMint();
        int64 nMoneyDestroy = block.GetBlockMoneyDestroy();
        uint64 nRandBeacon = block.GetBlockBeacon();
        CBlockIndex* pIndexPrev = nullptr;
        map<uint256, CBlockIndex*>::iterator miPrev = mapBlockIndex.find(block.hashPrev);
        if (miPrev != mapBlockIndex.end())
        {
            pIndexPrev = (*miPrev).second;
            pIndexNew->pPrev = pIndexPrev;
            if (!pIndexNew->IsOrigin())
            {
                pIndexNew->pOrigin = pIndexPrev->pOrigin;
                nRandBeacon ^= pIndexNew->pOrigin->nRandBeacon;
            }
            nMoneySupply += pIndexPrev->nMoneySupply;
            nMoneyDestroy += pIndexPrev->nMoneyDestroy;
            nChainTrust += pIndexPrev->nChainTrust;
        }
        pIndexNew->nMoneySupply = nMoneySupply;
        pIndexNew->nMoneyDestroy = nMoneyDestroy;
        pIndexNew->nChainTrust = nChainTrust;
        pIndexNew->nRandBeacon = nRandBeacon;
    }
    else
    {
        StdError("check", "AddNewIndex: new fail, block: %s", hash.GetHex().c_str());
    }
    return pIndexNew;
}

CBlockIndex* CCheckBlockWalker::AddNewIndex(const uint256& hash, const CBlockOutline& objBlockOutline)
{
    CBlockIndex* pIndexNew = new CBlockIndex(static_cast<const CBlockIndex&>(objBlockOutline));
    if (pIndexNew != nullptr)
    {
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.insert(make_pair(hash, pIndexNew)).first;
        if (mi == mapBlockIndex.end())
        {
            StdError("check", "AddNewIndex: insert fail, block: %s", hash.GetHex().c_str());
            delete pIndexNew;
            return nullptr;
        }
        pIndexNew->phashBlock = &((*mi).first);

        if (objBlockOutline.hashPrev != 0)
        {
            map<uint256, CBlockIndex*>::iterator miPrev = mapBlockIndex.find(objBlockOutline.hashPrev);
            if (miPrev == mapBlockIndex.end())
            {
                StdError("check", "AddNewIndex: find prev fail, prev: %s", objBlockOutline.hashPrev.GetHex().c_str());
                mapBlockIndex.erase(hash);
                delete pIndexNew;
                return nullptr;
            }
            pIndexNew->pPrev = miPrev->second;
        }

        if (objBlockOutline.hashOrigin != 0)
        {
            map<uint256, CBlockIndex*>::iterator miOri = mapBlockIndex.find(objBlockOutline.hashOrigin);
            if (miOri == mapBlockIndex.end())
            {
                StdError("check", "AddNewIndex: find origin fail, origin: %s", objBlockOutline.hashOrigin.GetHex().c_str());
                mapBlockIndex.erase(hash);
                delete pIndexNew;
                return nullptr;
            }
            pIndexNew->pOrigin = miOri->second;
        }
    }
    else
    {
        StdError("check", "AddNewIndex: new fail, block: %s", hash.GetHex().c_str());
    }
    return pIndexNew;
}

void CCheckBlockWalker::ClearBlockIndex()
{
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin();
    for (; mi != mapBlockIndex.end(); ++mi)
    {
        delete (*mi).second;
    }
    mapBlockIndex.clear();
}

bool CCheckBlockWalker::CheckBlockIndex()
{
    CCheckBlockIndexWalker objBlockIndexWalker(mapBlockIndex);
    dbBlockIndex.WalkThroughBlock(objBlockIndexWalker);
    if (!objBlockIndexWalker.vRemove.empty())
    {
        StdLog("check", "CheckBlockIndex: Remove block index count: %ld", objBlockIndexWalker.vRemove.size());
        if (!fOnlyCheck)
        {
            for (const auto& hash : objBlockIndexWalker.vRemove)
            {
                dbBlockIndex.RemoveBlock(hash);
            }
        }
    }
    return true;
}

bool CCheckBlockWalker::CheckRefBlock()
{
    auto nt = mapCheckFork.find(hashGenesis);
    if (nt == mapCheckFork.end())
    {
        StdError("check", "Check ref block: find primary fork fail, hashGenesis: %s", hashGenesis.GetHex().c_str());
        return false;
    }
    if (nt->second.pLast == nullptr)
    {
        StdError("check", "Check ref block: primary fork last is null, hashGenesis: %s", hashGenesis.GetHex().c_str());
        return false;
    }
    CBlockIndex* pPrimaryLast = nt->second.pLast;

    bool fCheckRet = true;
    map<uint256, CCheckBlockFork>::iterator mt = mapCheckFork.begin();
    for (; mt != mapCheckFork.end(); ++mt)
    {
        if (mt->first == hashGenesis)
        {
            continue;
        }
        const uint256& hashFork = mt->first;
        CCheckBlockFork& checkFork = mt->second;
        if (checkFork.pOrigin == nullptr)
        {
            StdError("check", "Check ref block: pOrigin is null, fork: %s", hashFork.GetHex().c_str());
            return false;
        }
        if (checkFork.pOrigin->pPrev == nullptr)
        {
            StdError("check", "Check ref block: pPrev is null, fork: %s", hashFork.GetHex().c_str());
            return false;
        }
        if (checkFork.pOrigin->pPrev->pNext == nullptr)
        {
            StdLog("check", "Check ref block: Joint block is non long chain, fork: %s", hashFork.GetHex().c_str());
            checkFork.pLast = checkFork.pOrigin;
            checkFork.pLast->pNext = nullptr;
            checkFork.fInvalidFork = true;
            continue;
        }
        CBlockIndex* pBlockIndex = checkFork.pOrigin;
        while (pBlockIndex)
        {
            if (pBlockIndex->IsSubsidiary() || pBlockIndex->IsExtended())
            {
                const uint256& hashBlock = pBlockIndex->GetBlockHash();
                bool fRet = true;
                do
                {
                    auto nt = mapRefBlock.find(hashBlock);
                    if (nt == mapRefBlock.end())
                    {
                        StdError("check", "Check ref block: find ref block fail, block: %s, fork: %s",
                                 hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
                        fRet = false;
                        break;
                    }
                    const uint256& hashRefBlock = nt->second;

                    auto it = mapBlockIndex.find(hashRefBlock);
                    if (it == mapBlockIndex.end())
                    {
                        StdError("check", "Check ref block: find ref index fail, refblock: %s, block: %s, fork: %s",
                                 hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
                        fRet = false;
                        break;
                    }
                    CBlockIndex* pRefIndex = it->second;
                    if (pRefIndex == nullptr)
                    {
                        StdError("check", "Check ref block: ref index is null, refblock: %s, block: %s, fork: %s",
                                 hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
                        fRet = false;
                        break;
                    }
                    if (!pRefIndex->IsPrimary())
                    {
                        StdError("check", "Check ref block: ref block not is primary chain, refblock: %s, block: %s, fork: %s",
                                 hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
                        fRet = false;
                        break;
                    }
                    if (pRefIndex != pPrimaryLast && pRefIndex->pNext == nullptr)
                    {
                        StdError("check", "Check ref block: ref block is short chain, refblock: %s, block: %s, fork: %s",
                                 hashRefBlock.GetHex().c_str(), hashBlock.GetHex().c_str(), hashFork.GetHex().c_str());
                        fRet = false;
                        break;
                    }
                } while (0);

                if (!fRet)
                {
                    fCheckRet = false;
                    checkFork.pLast = pBlockIndex->pPrev;
                    checkFork.pLast->pNext = nullptr;
                    break;
                }
            }
            pBlockIndex = pBlockIndex->pNext;
        }
    }
    if (!fCheckRet && fOnlyCheck)
    {
        return false;
    }
    return true;
}

bool CCheckBlockWalker::CheckSurplusAddressTxIndex(uint64& nTxIndexCount)
{
    map<uint256, CCheckBlockFork>::iterator mt = mapCheckFork.begin();
    for (; mt != mapCheckFork.end(); ++mt)
    {
        nTxIndexCount += mt->second.mapBlockTxInfo.size();
        if (!mt->second.CheckForkAddressTxIndex(mt->first, 0x7FFFFFFF))
        {
            StdLog("check", "Check address tx index: Check fork address txindex fail, fork: %s", mt->first.GetHex().c_str());
            return false;
        }

        CCheckAddressTxIndexWalker walker(mt->second.mapBlockTxIndex);
        if (!dbAddressTxIndex.WalkThrough(mt->first, walker))
        {
            StdLog("check", "Check address tx index: Walk through address txindex fail, fork: %s", mt->first.GetHex().c_str());
            return false;
        }
        if (!walker.vRemove.empty())
        {
            StdLog("check", "Check address tx index: Remove address txindex count: %lu, fork: %s", walker.vRemove.size(), mt->first.GetHex().c_str());
            if (!fOnlyCheck)
            {
                if (!dbAddressTxIndex.RepairAddressTxIndex(mt->first, vector<pair<CAddrTxIndex, CAddrTxInfo>>(), walker.vRemove))
                {
                    StdLog("check", "Check address tx index: RepairAddressTxIndex fail, fork: %s", mt->first.GetHex().c_str());
                    return false;
                }
            }
        }
    }
    return true;
}

bool CCheckBlockWalker::UpdateInvest(const uint256& hashBlock, const CBlockEx& block)
{
    while (mapCacheFullInvest.size() >= 128)
    {
        mapCacheFullInvest.erase(mapCacheFullInvest.begin());
    }
    CInvestContext& ctxtCacheInvest = mapCacheFullInvest[hashBlock];
    auto nt = mapCacheFullInvest.find(block.hashPrev);
    if (nt != mapCacheFullInvest.end())
    {
        ctxtCacheInvest = nt->second;
    }
    else
    {
        if (!dbInvest.Retrieve(block.hashPrev, ctxtCacheInvest))
        {
            StdError("check", "Update invest: Retrieve prev failed, prev: %s", block.hashPrev.GetHex().c_str());
            return false;
        }
    }

    std::vector<std::pair<CDestination, int64>> vBlockInvestReward;
    if (ctxtCacheInvest.vNoPackingReward.size() <= nMaxBlockRewardTxCount)
    {
        vBlockInvestReward = ctxtCacheInvest.vNoPackingReward;
        ctxtCacheInvest.vNoPackingReward.clear();
    }
    else
    {
        vBlockInvestReward.assign(ctxtCacheInvest.vNoPackingReward.begin(), ctxtCacheInvest.vNoPackingReward.begin() + nMaxBlockRewardTxCount);
        ctxtCacheInvest.vNoPackingReward.erase(ctxtCacheInvest.vNoPackingReward.begin(), ctxtCacheInvest.vNoPackingReward.begin() + nMaxBlockRewardTxCount);
    }
    int nPrevHeight = CBlock::GetBlockHeightByHash(block.hashPrev);
    for (const auto& kv : ctxtCacheInvest.mapInvest)
    {
        const CInvest& invest = kv.second;
        if (invest.nInvestAmount > 0 && (nPrevHeight - invest.nStartHeight) == (invest.nCycle * INVEST_DAY_HEIGHT))
        {
            int64 nRewardAmount = CalcInvestReward(invest.nCycle, invest.nInvestAmount);
            if (vBlockInvestReward.size() < nMaxBlockRewardTxCount)
            {
                vBlockInvestReward.push_back(make_pair(invest.destReward, nRewardAmount));
            }
            else
            {
                ctxtCacheInvest.vNoPackingReward.push_back(make_pair(invest.destReward, nRewardAmount));
            }
        }
    }

    if (block.vtx.size() < vBlockInvestReward.size())
    {
        StdError("check", "Update invest: Block invest reward count error, block tx count: %ld, reward count: %ld, block: %s",
                 block.vtx.size(), vBlockInvestReward.size(), hashBlock.ToString().c_str());
        return false;
    }

    vector<pair<CDestination, CInvest>> vTxInvest;
    int nCurBlockHeight = block.GetBlockHeight();
    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        const CTxContxt& txContxt = block.vTxContxt[i];
        if (i < vBlockInvestReward.size())
        {
            const auto& reward = vBlockInvestReward[i];
            int64 nRewardAmount = reward.second * USER_REWARD_RATIO / MINT_REWARD_PER;
            if (tx.nType != CTransaction::TX_INVEST_REWARD
                || tx.sendTo != reward.first
                || tx.nAmount != nRewardAmount)
            {
                StdError("check", "Update invest: Block invest reward error, tx type: %d, tx sendto: %s, tx amount: %f, reward dest: %s, reward amount: %f, block: %s",
                         tx.nType, CAddress(tx.sendTo).ToString().c_str(), ValueFromToken(tx.nAmount),
                         CAddress(reward.first).ToString().c_str(), ValueFromToken(reward.second), hashBlock.ToString().c_str());
                return false;
            }
        }
        else
        {
            if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_VOTE)
            {
                auto ptrVote = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, tx.vchSig);
                if (ptrVote == nullptr)
                {
                    StdError("check", "Update invest: CreateTemplatePtr failed, block: %s", hashBlock.GetHex().c_str());
                    return false;
                }
                auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptrVote);

                CInvest invest;
                invest.nCycle = objVote->nCycle;
                invest.destReward = objVote->destOwner;
                invest.nInvestAmount = tx.nAmount;
                invest.nStartHeight = nCurBlockHeight;

                vTxInvest.push_back(make_pair(tx.sendTo, invest));

                auto& cacheInvest = ctxtCacheInvest.mapInvest[tx.sendTo];
                if (cacheInvest.nCycle == 0)
                {
                    cacheInvest.nCycle = objVote->nCycle;
                    cacheInvest.destReward = objVote->destOwner;
                }
                cacheInvest.nInvestAmount += tx.nAmount;
                cacheInvest.nStartHeight = nCurBlockHeight;
            }
            if (txContxt.destIn.IsTemplate() && txContxt.destIn.GetTemplateId().GetType() == TEMPLATE_VOTE)
            {
                CInvest invest;
                invest.nInvestAmount -= (tx.nAmount + tx.nTxFee);
                vTxInvest.push_back(make_pair(txContxt.destIn, invest));

                auto mt = ctxtCacheInvest.mapInvest.find(txContxt.destIn);
                if (mt == ctxtCacheInvest.mapInvest.end())
                {
                    StdError("check", "Update invest: Redeem find failed, destIn: %s, block: %s",
                             CAddress(txContxt.destIn).ToString().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                mt->second.nInvestAmount -= (tx.nAmount + tx.nTxFee);
                if (mt->second.nInvestAmount < 0)
                {
                    StdError("check", "Update invest: Redeem invest amount less than zero, amount: %ld, destIn: %s, block: %s",
                             mt->second.nInvestAmount, CAddress(txContxt.destIn).ToString().c_str(), hashBlock.ToString().c_str());
                    return false;
                }
                else if (mt->second.nInvestAmount == 0)
                {
                    ctxtCacheInvest.mapInvest.erase(mt);
                }
            }
        }
    }

    bool fUpdate = false;
    CInvestContext ctxtNewInvest;
    do
    {
        if (!dbInvest.GetBlockInvest(hashBlock, block.hashPrev, vTxInvest, ctxtNewInvest))
        {
            fUpdate = true;
            StdLog("check", "Update invest: Get new block invest fail, block: %s", hashBlock.GetHex().c_str());
            break;
        }
        ctxtNewInvest.vNoPackingReward.clear();
        ctxtNewInvest.vNoPackingReward.assign(ctxtCacheInvest.vNoPackingReward.begin(), ctxtCacheInvest.vNoPackingReward.end());

        CInvestContext ctxtBlockInvest;
        if (!dbInvest.Retrieve(hashBlock, ctxtBlockInvest))
        {
            StdError("check", "Update invest: Retrieve failed, block: %s", hashBlock.GetHex().c_str());
            fUpdate = true;
            break;
        }
        if (ctxtCacheInvest.mapInvest != ctxtBlockInvest.mapInvest
            || ctxtCacheInvest.vNoPackingReward != ctxtBlockInvest.vNoPackingReward)
        {
            StdError("check", "Update invest: Block invest error, cache invest size: %ld, db invest size: %ld, cache pack size: %ld, db pack size: %ld, block: %s",
                     ctxtCacheInvest.mapInvest.size(), ctxtBlockInvest.mapInvest.size(),
                     ctxtCacheInvest.vNoPackingReward.size(), ctxtBlockInvest.vNoPackingReward.size(),
                     hashBlock.GetHex().c_str());
            fUpdate = true;
            break;
        }

        CInvestContext ctxtDbInvest;
        if (!dbInvest.RetrieveBlockInvest(hashBlock, ctxtDbInvest))
        {
            fUpdate = true;
            StdLog("check", "Update invest: Get db invest fail, block: %s", hashBlock.GetHex().c_str());
            break;
        }

        if (ctxtNewInvest != ctxtDbInvest)
        {
            fUpdate = true;
            StdLog("check", "Update invest: Invest data error, block: %s", hashBlock.ToString().c_str());
            break;
        }
    } while (0);

    if (fUpdate)
    {
        StdLog("check", "Update invest: Invest check fail, block: %s", hashBlock.ToString().c_str());
        if (fOnlyCheck)
        {
            return false;
        }
        if (!dbInvest.AddNew(hashBlock, ctxtNewInvest))
        {
            StdError("check", "Update invest: Update invest context failed, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }
    return true;
}

bool CCheckBlockWalker::UpdateActivate(const uint256& hashBlock, const CBlockEx& block)
{
    CActivateContext ctxtActivate;
    if (block.hashPrev != 0)
    {
        if (!dbActivate.Retrieve(block.hashPrev, ctxtActivate))
        {
            StdError("check", "Update activate: Retrieve activate context failed, prev block: %s", block.hashPrev.ToString().c_str());
            return false;
        }
    }

    for (size_t i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_ACTIVATE)
        {
            auto ptr = CTemplate::CreateTemplatePtr(TEMPLATE_ACTIVATE, tx.vchSig);
            if (ptr == nullptr)
            {
                StdError("check", "Update activate: CreateTemplatePtr failed, block: %s", hashBlock.ToString().c_str());
                return false;
            }
            auto obj = boost::dynamic_pointer_cast<CTemplateActivate>(ptr);

            int& height = ctxtActivate.mapActivate[obj->destOwner];
            if (height == 0)
            {
                height = block.GetBlockHeight();
            }
        }
    }

    CActivateContext ctxtBlockActivate;
    if (!dbActivate.Retrieve(hashBlock, ctxtBlockActivate) || ctxtBlockActivate != ctxtActivate)
    {
        StdLog("check", "Update activate: activate error, block: %s", hashBlock.ToString().c_str());
        if (fOnlyCheck)
        {
            return false;
        }
        if (!dbActivate.AddNew(hashBlock, ctxtActivate))
        {
            StdError("check", "Update activate: Update activate context failed, block: %s", hashBlock.ToString().c_str());
            return false;
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////
// CCheckRepairData

bool CCheckRepairData::FetchBlockData()
{
    {
        CTimeSeriesCached tsBlock;
        if (!tsBlock.Initialize(path(strDataPath) / "block", BLOCKFILE_PREFIX))
        {
            StdError("check", "tsBlock Initialize fail");
            return false;
        }

        if (!objBlockWalker.Initialize(strDataPath))
        {
            StdError("check", "objBlockWalker Initialize fail");
            tsBlock.Deinitialize();
            return false;
        }

        uint32 nLastFileRet = 0;
        uint32 nLastPosRet = 0;

        StdLog("check", "Fetch block starting");
        if (!tsBlock.WalkThrough(objBlockWalker, nLastFileRet, nLastPosRet, !fOnlyCheck))
        {
            StdError("check", "Fetch block fail.");
            tsBlock.Deinitialize();
            return false;
        }
        StdLog("check", "Fetch block success, count: %ld.", objBlockWalker.nBlockCount);
        tsBlock.Deinitialize();
    }
    objBlockWalker.objDelegateDB.Deinitialize();
    objBlockWalker.objTsBlock.Deinitialize();
    objBlockWalker.dbInvest.Deinitialize();
    objBlockWalker.dbActivate.Deinitialize();

    if (objBlockWalker.nBlockCount > 0)
    {
        if (objBlockWalker.hashGenesis == 0)
        {
            StdError("check", "Not genesis block");
            return false;
        }

        StdLog("check", "Update blockchain starting");
        if (!objBlockWalker.UpdateBlockNext())
        {
            StdError("check", "Fetch block and tx, update block next fail");
            return false;
        }
        StdLog("check", "Update blockchain success, main chain height: %d.", objBlockWalker.nMainChainHeight);

        StdLog("check", "Check refblock starting");
        if (!objBlockWalker.CheckRefBlock())
        {
            StdError("check", "Fetch block and tx, check refblock fail");
            return false;
        }
        StdLog("check", "Check refblock success, main chain height: %d.", objBlockWalker.nMainChainHeight);

        StdLog("check", "Check repair fork starting");
        if (!objBlockWalker.CheckRepairFork())
        {
            StdError("check", "Fetch block and tx, check repair fork");
            return false;
        }
        StdLog("check", "Check repair fork success.");

        if (objBlockWalker.fCheckAddrTxIndex)
        {
            StdLog("check", "Check surplus address txindex starting");
            uint64 nTxIndexCount = 0;
            if (!objBlockWalker.CheckSurplusAddressTxIndex(nTxIndexCount))
            {
                StdError("check", "Check surplus address txindex fail");
                return false;
            }
            StdLog("check", "Check surplus address txindex success, surplus count: %lu.", nTxIndexCount);
        }

        StdLog("check", "Check block index starting");
        if (!objBlockWalker.CheckBlockIndex())
        {
            StdLog("check", "Check block index fail");
            return false;
        }
        StdLog("check", "Check block index complete, count: %lu", objBlockWalker.mapBlockIndex.size());
    }
    objBlockWalker.Uninitialize();
    return true;
}

bool CCheckRepairData::CheckRepairUnspent(uint64& nUnspentCount)
{
    CUnspentDB dbUnspent;
    if (!dbUnspent.Initialize(path(strDataPath), false))
    {
        StdError("check", "Check repair unspent: dbUnspent Initialize fail");
        return false;
    }

    map<uint256, CCheckBlockFork>::iterator it = objBlockWalker.mapCheckFork.begin();
    for (; it != objBlockWalker.mapCheckFork.end(); ++it)
    {
        const uint256& hashFork = it->first;
        if (!dbUnspent.LoadFork(hashFork))
        {
            StdError("check", "Check repair unspent: dbUnspent AddNewFork fail.");
            dbUnspent.Deinitialize();
            return false;
        }
        CCheckForkUnspentWalker forkUnspentWalker(it->second.mapBlockUnspent);
        if (!dbUnspent.WalkThrough(hashFork, forkUnspentWalker))
        {
            StdError("check", "Check repair unspent: dbUnspent WalkThrough fail.");
            dbUnspent.Deinitialize();
            return false;
        }
        if (!forkUnspentWalker.CheckForkUnspent())
        {
            StdLog("check", "Check repair unspent: Check block unspent fail, update: %lu, remove: %lu, fork: %s",
                   forkUnspentWalker.vAddUpdate.size(),
                   forkUnspentWalker.vRemove.size(),
                   hashFork.GetHex().c_str());
            if (!fOnlyCheck)
            {
                if (!dbUnspent.RepairUnspent(hashFork, forkUnspentWalker.vAddUpdate, forkUnspentWalker.vRemove))
                {
                    StdError("check", "Check repair unspent: Repair unspent fail.");
                    dbUnspent.Deinitialize();
                    return false;
                }
            }
        }
        nUnspentCount += it->second.mapBlockUnspent.size();
    }

    dbUnspent.Deinitialize();
    return true;
}

bool CCheckRepairData::CheckRepairAddressUnspent()
{
    CAddressUnspentDB dbAddressUnspent;
    if (!dbAddressUnspent.Initialize(path(strDataPath), false))
    {
        StdError("check", "Check address unspent: dbAddress Initialize fail");
        return false;
    }

    map<uint256, CCheckBlockFork>::iterator it = objBlockWalker.mapCheckFork.begin();
    for (; it != objBlockWalker.mapCheckFork.end(); ++it)
    {
        const uint256& hashFork = it->first;
        if (!dbAddressUnspent.LoadFork(hashFork))
        {
            StdError("check", "Check address unspent: dbAddress AddNewFork fail.");
            dbAddressUnspent.Deinitialize();
            return false;
        }
        CCheckAddressUnspentWalker addressUnspentWalker(it->second.mapBlockUnspent);
        if (!dbAddressUnspent.WalkThrough(hashFork, addressUnspentWalker))
        {
            StdError("check", "Check address unspent: dbAddress WalkThrough fail.");
            dbAddressUnspent.Deinitialize();
            return false;
        }
        if (!addressUnspentWalker.CheckForkAddressUnspent())
        {
            StdLog("check", "Check address unspent: Check block address unspent fail, update: %lu, remove: %lu, fork: %s",
                   addressUnspentWalker.vAddUpdate.size(),
                   addressUnspentWalker.vRemove.size(),
                   hashFork.GetHex().c_str());
            if (!fOnlyCheck)
            {
                if (!dbAddressUnspent.RepairAddressUnspent(hashFork, addressUnspentWalker.vAddUpdate, addressUnspentWalker.vRemove))
                {
                    StdError("check", "Check address unspent: Repair address unspent fail.");
                    dbAddressUnspent.Deinitialize();
                    return false;
                }
            }
        }
        it->second.mapBlockUnspent.clear();
    }

    dbAddressUnspent.Deinitialize();
    return true;
}

bool CCheckRepairData::CheckRepairAddress(uint64& nAddressCount)
{
    CAddressDB dbAddress;
    if (!dbAddress.Initialize(path(strDataPath), false))
    {
        StdError("check", "Check address: dbAddress Initialize fail");
        return false;
    }

    map<uint256, CCheckBlockFork>::iterator it = objBlockWalker.mapCheckFork.begin();
    for (; it != objBlockWalker.mapCheckFork.end(); ++it)
    {
        const uint256& hashFork = it->first;
        if (!dbAddress.LoadFork(hashFork))
        {
            StdError("check", "Check address: dbAddress AddNewFork fail.");
            dbAddress.Deinitialize();
            return false;
        }
        CCheckAddressWalker checkAddressWalker(it->second.mapBlockAddress);
        if (!dbAddress.WalkThrough(hashFork, checkAddressWalker))
        {
            StdError("check", "Check address: dbAddress WalkThrough fail.");
            dbAddress.Deinitialize();
            return false;
        }
        if (!checkAddressWalker.CheckAddress())
        {
            StdLog("check", "Check address fail, update: %lu, remove: %lu, fork: %s",
                   checkAddressWalker.vAddUpdate.size(),
                   checkAddressWalker.vRemove.size(),
                   hashFork.GetHex().c_str());
            if (!fOnlyCheck)
            {
                if (!dbAddress.RepairAddress(hashFork, checkAddressWalker.vAddUpdate, checkAddressWalker.vRemove))
                {
                    StdError("check", "Check address: Repair address fail, update: %lu, remove: %lu, fork: %s",
                             checkAddressWalker.vAddUpdate.size(), checkAddressWalker.vRemove.size(), hashFork.GetHex().c_str());
                    dbAddress.Deinitialize();
                    return false;
                }
            }
        }
        nAddressCount += it->second.mapBlockAddress.size();
        it->second.mapBlockAddress.clear();
    }

    dbAddress.Deinitialize();
    return true;
}

bool CCheckRepairData::CheckTxIndex(uint64& nTxIndexCount)
{
    CTxIndexDB dbTxIndex;
    if (!dbTxIndex.Initialize(path(strDataPath), false))
    {
        StdLog("check", "Check tx index: dbTxIndex Initialize fail");
        return false;
    }

    map<uint256, vector<pair<uint256, CTxIndex>>> mapTxNew;
    map<uint256, CCheckBlockFork>::iterator mt = objBlockWalker.mapCheckFork.begin();
    for (; mt != objBlockWalker.mapCheckFork.end(); ++mt)
    {
        if (!dbTxIndex.LoadFork(mt->first))
        {
            StdLog("check", "Check tx index: dbTxIndex LoadFork fail");
            dbTxIndex.Deinitialize();
            return false;
        }
        const uint256& hashFork = mt->first;
        const auto& mapBlockTxIndex = mt->second.mapBlockTxIndex;
        for (auto nt = mapBlockTxIndex.begin(); nt != mapBlockTxIndex.end(); ++nt)
        {
            CTxIndex txIndex;
            if (!dbTxIndex.Retrieve(hashFork, nt->first, txIndex, true))
            {
                StdLog("check", "Check tx index: Retrieve tx index fail, height: %d, tx: %s.",
                       nt->second.nBlockHeight, nt->first.GetHex().c_str());

                mapTxNew[hashFork].push_back(make_pair(nt->first, CTxIndex(nt->second.nBlockHeight, nt->second.nFile, nt->second.nOffset)));
            }
            else
            {
                if (txIndex.nFile != nt->second.nFile || txIndex.nOffset != nt->second.nOffset)
                {
                    StdLog("check", "Check tx index: nFile or nOffset error, height: %d, tx: %s, db: %d - %d, block: %d - %d.",
                           nt->second.nBlockHeight, nt->first.GetHex().c_str(),
                           txIndex.nFile, txIndex.nOffset, nt->second.nFile, nt->second.nOffset);

                    mapTxNew[hashFork].push_back(make_pair(nt->first, CTxIndex(nt->second.nBlockHeight, nt->second.nFile, nt->second.nOffset)));
                }
            }
            ++nTxIndexCount;
        }
        mt->second.mapBlockTxIndex.clear();
    }

    // repair
    if (!fOnlyCheck && !mapTxNew.empty())
    {
        StdLog("check", "Repair tx index starting");
        int64 nUpdateTxIndexCount = 0;
        for (const auto& fork : mapTxNew)
        {
            if (!dbTxIndex.Update(fork.first, fork.second, vector<uint256>()))
            {
                StdLog("check", "Repair tx index update fail");
                dbTxIndex.Deinitialize();
                return false;
            }
            dbTxIndex.Flush(fork.first);
            nUpdateTxIndexCount += fork.second.size();
        }
        StdLog("check", "Repair tx index success, update: %lu", nUpdateTxIndexCount);
    }

    dbTxIndex.Deinitialize();
    return true;
}

////////////////////////////////////////////////////////////////
bool CCheckRepairData::CheckRepairData()
{
    StdLog("check", "Start check and repair, path: %s", strDataPath.c_str());

    if (!objForkManager.FetchForkStatus())
    {
        StdLog("check", "Fetch fork status fail");
        return false;
    }
    StdLog("check", "Fetch fork status success");

    StdLog("check", "Fetch block data starting");
    if (!FetchBlockData())
    {
        StdLog("check", "Fetch block data fail");
        return false;
    }
    StdLog("check", "Fetch block data success");

    StdLog("check", "Check txpool starting");
    {
        CCheckTxPoolData txpool(objBlockWalker.mapCheckFork);
        if (!txpool.CheckTxPool(strDataPath, fOnlyCheck))
        {
            StdLog("check", "Check txpool fail");
            return false;
        }
    }
    StdLog("check", "Check txpool complete");

    StdLog("check", "Check unspent starting");
    uint64 nUnspentCount = 0;
    if (!CheckRepairUnspent(nUnspentCount))
    {
        StdLog("check", "Check unspent fail");
        return false;
    }
    StdLog("check", "Check unspent success, count: %lu", nUnspentCount);

    StdLog("check", "Check address unspent starting");
    if (!CheckRepairAddressUnspent())
    {
        StdLog("check", "Check address unspent fail");
        return false;
    }
    StdLog("check", "Check address unspent success, count: %lu", nUnspentCount);

    StdLog("check", "Check address starting");
    uint64 nAddressCount = 0;
    if (!CheckRepairAddress(nAddressCount))
    {
        StdLog("check", "Check address fail");
        return false;
    }
    StdLog("check", "Check address success, count: %lu", nAddressCount);

    StdLog("check", "Check tx index starting");
    uint64 nTxIndexCount = 0;
    if (!CheckTxIndex(nTxIndexCount))
    {
        StdLog("check", "Check tx index fail");
        return false;
    }
    StdLog("check", "Check tx index complete, count: %lu", nTxIndexCount);
    return true;
}

} // namespace ibrio
