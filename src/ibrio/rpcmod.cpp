// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcmod.h"

#include "json/json_spirit_reader_template.h"
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/regex.hpp>
#include <regex>
//#include <algorithm>

#include "address.h"
#include "rpc/auto_protocol.h"
#include "template/activate.h"
#include "template/fork.h"
#include "template/proof.h"
#include "template/template.h"
#include "util.h"
#include "version.h"

using namespace std;
using namespace xengine;
using namespace json_spirit;
using namespace ibrio::rpc;
using namespace ibrio;
namespace fs = boost::filesystem;

#define UNLOCKKEY_RELEASE_DEFAULT_TIME 60

const char* GetGitVersion();

///////////////////////////////
// static function

static int64 AmountFromValue(const double dAmount, const bool fZero = false)
{
    if (IsDoubleEqual(dAmount, -1.0))
    {
        return -1;
    }

    if (dAmount < 0.0 || dAmount > MAX_MONEY)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid amount");
    }
    int64 nAmount = (int64)(dAmount * COIN + 0.5);
    if ((!fZero && nAmount == 0) || !MoneyRange(nAmount))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid amount");
    }
    return nAmount;
}

static CBlockData BlockToJSON(const uint256& hashBlock, const CBlock& block, const uint256& hashFork, int nHeight)
{
    CBlockData data;
    data.strHash = hashBlock.GetHex();
    data.strHashprev = block.hashPrev.GetHex();
    data.nVersion = block.nVersion;
    data.strType = GetBlockTypeStr(block.nType, block.txMint.nType);
    data.nTime = block.GetBlockTime();
    if (block.hashPrev != 0)
    {
        data.strPrev = block.hashPrev.GetHex();
    }
    data.strFork = hashFork.GetHex();
    data.nHeight = nHeight;

    data.strTxmint = block.txMint.GetHash().GetHex();
    for (const CTransaction& tx : block.vtx)
    {
        data.vecTx.push_back(tx.GetHash().GetHex());
    }
    return data;
}

static CTransactionData TxToJSON(const uint256& txid, const CTransaction& tx,
                                 const uint256& hashFork, const uint256& blockHash, int nDepth, const string& fromAddr = string())
{
    CTransactionData ret;
    ret.strTxid = txid.GetHex();
    ret.nVersion = tx.nVersion;
    ret.strType = tx.GetTypeString();
    ret.nTime = tx.nTimeStamp;
    ret.nLockuntil = tx.nLockUntil;
    ret.strAnchor = tx.hashAnchor.GetHex();
    ret.strBlockhash = (!blockHash) ? std::string() : blockHash.GetHex();
    for (const CTxIn& txin : tx.vInput)
    {
        CTransactionData::CVin vin;
        vin.nVout = txin.prevout.n;
        vin.strTxid = txin.prevout.hash.GetHex();
        ret.vecVin.push_back(move(vin));
    }
    ret.strSendfrom = fromAddr;
    ret.strSendto = CAddress(tx.sendTo).ToString();
    ret.dAmount = ValueFromToken(tx.nAmount);
    ret.dTxfee = ValueFromToken(tx.nTxFee);

    std::string str(tx.vchData.begin(), tx.vchData.end());
    if (str.substr(0, 4) == "msg:")
    {
        ret.strData = str;
    }
    else
    {
        ret.strData = xengine::ToHexString(tx.vchData);
    }
    ret.strSig = xengine::ToHexString(tx.vchSig);
    ret.strFork = hashFork.GetHex();
    if (nDepth >= 0)
    {
        ret.nConfirmations = nDepth;
    }

    return ret;
}

static CWalletTxData TxInfoToJSON(const CTxInfo& tx, const bool fSendFromIn)
{
    CWalletTxData data;
    data.strTxid = tx.txid.GetHex();
    data.strFork = tx.hashFork.GetHex();
    data.nBlockheight = tx.nBlockHeight;
    data.nTxseq = tx.nTxSeq;
    data.strType = CTransaction::GetTypeStringStatic(tx.nTxType);
    data.nTime = (boost::int64_t)tx.nTimeStamp;
    data.fSend = fSendFromIn;
    if (!tx.IsRewardTx())
    {
        data.strFrom = CAddress(tx.destFrom).ToString();
    }
    data.strTo = CAddress(tx.destTo).ToString();
    data.dAmount = ValueFromToken(tx.nAmount);
    data.dFee = ValueFromToken(tx.nTxFee);
    data.nLockuntil = (boost::int64_t)tx.nLockUntil;
    return data;
}

static CUnspentData UnspentToJSON(const CTxUnspent& unspent)
{
    CUnspentData data;
    data.strTxid = unspent.hash.ToString();
    data.nOut = unspent.n;
    data.dAmount = ValueFromToken(unspent.output.nAmount);
    data.nHeight = unspent.nHeight;
    data.nTime = unspent.output.nTxTime;
    data.nLockuntil = unspent.output.nLockUntil;
    return data;
}

namespace ibrio
{

///////////////////////////////
// CRPCMod

CRPCMod::CRPCMod()
  : IIOModule("rpcmod")
{
    pHttpServer = nullptr;
    pCoreProtocol = nullptr;
    pService = nullptr;
    pDataStat = nullptr;
    pForkManager = nullptr;

    std::map<std::string, RPCFunc> temp_map = boost::assign::map_list_of
        /* System */
        ("help", &CRPCMod::RPCHelp)
        //
        ("stop", &CRPCMod::RPCStop)
        //
        ("version", &CRPCMod::RPCVersion)
        /* Network */
        ("getpeercount", &CRPCMod::RPCGetPeerCount)
        //
        ("listpeer", &CRPCMod::RPCListPeer)
        //
        ("addnode", &CRPCMod::RPCAddNode)
        //
        ("removenode", &CRPCMod::RPCRemoveNode)
        /* Blockchain & TxPool */
        ("getforkcount", &CRPCMod::RPCGetForkCount)
        //
        ("listfork", &CRPCMod::RPCListFork)
        //
        ("getgenealogy", &CRPCMod::RPCGetForkGenealogy)
        //
        ("getblocklocation", &CRPCMod::RPCGetBlockLocation)
        //
        ("getblockcount", &CRPCMod::RPCGetBlockCount)
        //
        ("getblockhash", &CRPCMod::RPCGetBlockHash)
        //
        ("getblock", &CRPCMod::RPCGetBlock)
        //
        ("getblockdetail", &CRPCMod::RPCGetBlockDetail)
        //
        ("gettxpool", &CRPCMod::RPCGetTxPool)
        //
        ("gettransaction", &CRPCMod::RPCGetTransaction)
        //
        ("sendtransaction", &CRPCMod::RPCSendTransaction)
        //
        ("getforkheight", &CRPCMod::RPCGetForkHeight)
        //
        ("getvotes", &CRPCMod::RPCGetVotes)
        //
        ("listdelegate", &CRPCMod::RPCListDelegate)
        /* Wallet */
        ("listkey", &CRPCMod::RPCListKey)
        //
        ("getnewkey", &CRPCMod::RPCGetNewKey)
        //
        ("encryptkey", &CRPCMod::RPCEncryptKey)
        //
        ("lockkey", &CRPCMod::RPCLockKey)
        //
        ("unlockkey", &CRPCMod::RPCUnlockKey)
        //
        ("removekey", &CRPCMod::RPCRemoveKey)
        //
        ("importprivkey", &CRPCMod::RPCImportPrivKey)
        //
        ("importpubkey", &CRPCMod::RPCImportPubKey)
        //
        ("importkey", &CRPCMod::RPCImportKey)
        //
        ("exportkey", &CRPCMod::RPCExportKey)
        //
        ("addnewtemplate", &CRPCMod::RPCAddNewTemplate)
        //
        ("importtemplate", &CRPCMod::RPCImportTemplate)
        //
        ("exporttemplate", &CRPCMod::RPCExportTemplate)
        //
        ("removetemplate", &CRPCMod::RPCRemoveTemplate)
        //
        ("validateaddress", &CRPCMod::RPCValidateAddress)
        //
        ("getbalance", &CRPCMod::RPCGetBalance)
        //
        ("listtransaction", &CRPCMod::RPCListTransaction)
        //
        ("sendfrom", &CRPCMod::RPCSendFrom)
        //
        ("createtransaction", &CRPCMod::RPCCreateTransaction)
        //
        ("signtransaction", &CRPCMod::RPCSignTransaction)
        //
        ("signmessage", &CRPCMod::RPCSignMessage)
        //
        ("listaddress", &CRPCMod::RPCListAddress)
        //
        ("exportwallet", &CRPCMod::RPCExportWallet)
        //
        ("importwallet", &CRPCMod::RPCImportWallet)
        //
        ("makeorigin", &CRPCMod::RPCMakeOrigin)
        //
        ("signrawtransactionwithwallet", &CRPCMod::RPCSignRawTransactionWithWallet)
        //
        ("sendrawtransaction", &CRPCMod::RPCSendRawTransaction)
        /* Util */
        ("verifymessage", &CRPCMod::RPCVerifyMessage)
        //
        ("makekeypair", &CRPCMod::RPCMakeKeyPair)
        //
        ("getpubkey", &CRPCMod::RPCGetPubKey)
        //
        ("getpubkeyaddress", &CRPCMod::RPCGetPubKeyAddress)
        //
        ("gettemplateaddress", &CRPCMod::RPCGetTemplateAddress)
        //
        ("maketemplate", &CRPCMod::RPCMakeTemplate)
        //
        ("decodetransaction", &CRPCMod::RPCDecodeTransaction)
        //
        ("gettxfee", &CRPCMod::RPCGetTxFee)
        //
        ("makesha256", &CRPCMod::RPCMakeSha256)
        //
        ("aesencrypt", &CRPCMod::RPCAesEncrypt)
        //
        ("aesdecrypt", &CRPCMod::RPCAesDecrypt)
        //
        ("listunspent", &CRPCMod::RPCListUnspent)
        //
        ("getdefirelation", &CRPCMod::RPCGetDeFiRelation)
        //
        ("reversehex", &CRPCMod::RPCReverseHex)
        //
        ("activatesign", &CRPCMod::RPCActivateSign)
        //
        ("getactivatestatus", &CRPCMod::RPCGetActivateStatus)
        /* Mint */
        ("getwork", &CRPCMod::RPCGetWork)
        //
        ("submitwork", &CRPCMod::RPCSubmitWork)
        /* tool */
        ("querystat", &CRPCMod::RPCQueryStat);
    mapRPCFunc = temp_map;
    fWriteRPCLog = true;
}

CRPCMod::~CRPCMod()
{
}

bool CRPCMod::HandleInitialize()
{
    if (!GetObject("httpserver", pHttpServer))
    {
        Error("Failed to request httpserver");
        return false;
    }

    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol");
        return false;
    }

    if (!GetObject("service", pService))
    {
        Error("Failed to request service");
        return false;
    }

    if (!GetObject("datastat", pDataStat))
    {
        Error("Failed to request datastat");
        return false;
    }
    if (!GetObject("forkmanager", pForkManager))
    {
        Error("Failed to request forkmanager");
        return false;
    }
    fWriteRPCLog = RPCServerConfig()->fRPCLogEnable;

    return true;
}

void CRPCMod::HandleDeinitialize()
{
    pHttpServer = nullptr;
    pCoreProtocol = nullptr;
    pService = nullptr;
    pDataStat = nullptr;
    pForkManager = nullptr;
}

bool CRPCMod::HandleEvent(CEventHttpReq& eventHttpReq)
{
    auto lmdMask = [](const string& data) -> string
    {
        //remove all sensible information such as private key
        // or passphrass from log content

        //log for debug mode
        boost::regex ptnSec(R"raw(("privkey"|"passphrase"|"oldpassphrase"|"signsecret"|"privkeyaddress")(\s*:\s*)(".*?"))raw", boost::regex::perl);
        return boost::regex_replace(data, ptnSec, string(R"raw($1$2"***")raw"));
    };

    uint64 nNonce = eventHttpReq.nNonce;

    string strResult;
    try
    {
        // check version
        string strVersion = eventHttpReq.data.mapHeader["url"].substr(1);
        if (!strVersion.empty())
        {
            if (!CheckVersion(strVersion))
            {
                throw CRPCException(RPC_VERSION_OUT_OF_DATE,
                                    string("Out of date version. Server version is v") + VERSION_STR
                                        + ", but client version is v" + strVersion);
            }
        }

        bool fArray;
        CRPCReqVec vecReq = DeserializeCRPCReq(eventHttpReq.data.strContent, fArray);
        CRPCRespVec vecResp;
        for (auto& spReq : vecReq)
        {
            CRPCErrorPtr spError;
            CRPCResultPtr spResult;
            try
            {
                map<string, RPCFunc>::iterator it = mapRPCFunc.find(spReq->strMethod);
                if (it == mapRPCFunc.end())
                {
                    throw CRPCException(RPC_METHOD_NOT_FOUND, "Method not found");
                }

                if (fWriteRPCLog)
                {
                    Debug("request : %s ", lmdMask(spReq->Serialize()).c_str());
                }

                spResult = (this->*(*it).second)(spReq->spParam);
            }
            catch (CRPCException& e)
            {
                spError = CRPCErrorPtr(new CRPCError(e));
            }
            catch (exception& e)
            {
                spError = CRPCErrorPtr(new CRPCError(RPC_MISC_ERROR, e.what()));
            }

            if (spError)
            {
                vecResp.push_back(MakeCRPCRespPtr(spReq->valID, spError));
            }
            else if (spResult)
            {
                vecResp.push_back(MakeCRPCRespPtr(spReq->valID, spResult));
            }
            else
            {
                // no result means no return
            }
        }

        if (fArray)
        {
            strResult = SerializeCRPCResp(vecResp);
        }
        else if (vecResp.size() > 0)
        {
            strResult = vecResp[0]->Serialize();
        }
        else
        {
            // no result means no return
        }
    }
    catch (CRPCException& e)
    {
        auto spError = MakeCRPCErrorPtr(e);
        CRPCResp resp(e.valData, spError);
        strResult = resp.Serialize();
    }
    catch (exception& e)
    {
        cout << "error: " << e.what() << endl;
        auto spError = MakeCRPCErrorPtr(RPC_MISC_ERROR, e.what());
        CRPCResp resp(Value(), spError);
        strResult = resp.Serialize();
    }

    if (fWriteRPCLog)
    {
        Debug("response : %s ", lmdMask(strResult).c_str());
    }

    // no result means no return
    if (!strResult.empty())
    {
        JsonReply(nNonce, strResult);
    }

    return true;
}

bool CRPCMod::HandleEvent(CEventHttpBroken& eventHttpBroken)
{
    (void)eventHttpBroken;
    return true;
}

void CRPCMod::JsonReply(uint64 nNonce, const std::string& result)
{
    CEventHttpRsp eventHttpRsp(nNonce);
    eventHttpRsp.data.nStatusCode = 200;
    eventHttpRsp.data.mapHeader["content-type"] = "application/json";
    eventHttpRsp.data.mapHeader["connection"] = "Keep-Alive";
    eventHttpRsp.data.mapHeader["server"] = "ibrio-rpc";
    eventHttpRsp.data.strContent = result + "\n";

    pHttpServer->DispatchEvent(&eventHttpRsp);
}

bool CRPCMod::CheckWalletError(Errno err)
{
    switch (err)
    {
    case ERR_WALLET_NOT_FOUND:
        throw CRPCException(RPC_INVALID_REQUEST, "Missing wallet");
        break;
    case ERR_WALLET_IS_LOCKED:
        throw CRPCException(RPC_WALLET_UNLOCK_NEEDED,
                            "Wallet is locked,enter the wallet passphrase with walletpassphrase first.");
    case ERR_WALLET_IS_UNLOCKED:
        throw CRPCException(RPC_WALLET_ALREADY_UNLOCKED, "Wallet is already unlocked");
        break;
    case ERR_WALLET_IS_ENCRYPTED:
        throw CRPCException(RPC_WALLET_WRONG_ENC_STATE, "Running with an encrypted wallet, "
                                                        "but encryptwallet was called");
        break;
    case ERR_WALLET_IS_UNENCRYPTED:
        throw CRPCException(RPC_WALLET_WRONG_ENC_STATE, "Running with an unencrypted wallet, "
                                                        "but walletpassphrasechange/walletlock was called.");
        break;
    default:
        break;
    }
    return (err == OK);
}

void CRPCMod::ListDestination(vector<CDestination>& vDestination, const uint64 nPage, const uint64 nCount)
{
    set<crypto::CPubKey> setPubKey;
    size_t nPubkeyCount = pService->GetPubKeys(setPubKey, nCount * nPage, nCount);

    vDestination.clear();
    for (const crypto::CPubKey& pubkey : setPubKey)
    {
        vDestination.push_back(CDestination(pubkey));
    }
    if (nCount > 0 && vDestination.size() >= nCount)
    {
        return;
    }

    uint64 nTidStartPos = 0;
    uint64 nTidCount = nCount;
    if (nCount > 0 && nPubkeyCount > 0)
    {
        if (nCount * nPage > nPubkeyCount)
        {
            nTidStartPos = nCount * nPage - nPubkeyCount;
        }
        nTidCount = nCount - vDestination.size();
    }

    set<CTemplateId> setTid;
    pService->GetTemplateIds(setTid, nTidStartPos, nTidCount);
    for (const CTemplateId& tid : setTid)
    {
        vDestination.push_back(CDestination(tid));
    }
}

bool CRPCMod::CheckVersion(string& strVersion)
{
    int nMajor, nMinor, nRevision;
    if (!ResolveVersion(strVersion, nMajor, nMinor, nRevision))
    {
        return false;
    }

    strVersion = FormatVersion(nMajor, nMinor, nRevision);
    if (nMajor != VERSION_MAJOR || nMinor != VERSION_MINOR)
    {
        return false;
    }

    return true;
}

string CRPCMod::GetWidthString(const string& strIn, int nWidth)
{
    string str = strIn;
    int nCurLen = str.size();
    if (nWidth > nCurLen)
    {
        str.append(nWidth - nCurLen, ' ');
    }
    return str;
}

std::string CRPCMod::GetWidthString(uint64 nCount, int nWidth)
{
    char tempbuf[12] = { 0 };
    sprintf(tempbuf, "%2.2d", (int)(nCount % 100));
    return GetWidthString(std::to_string(nCount / 100) + std::string(".") + tempbuf, nWidth);
}

/* System */
CRPCResultPtr CRPCMod::RPCHelp(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CHelpParam>(param);
    string command = spParam->strCommand;
    return MakeCHelpResultPtr(RPCHelpInfo(EModeType::MODE_CONSOLE, command));
}

CRPCResultPtr CRPCMod::RPCStop(CRPCParamPtr param)
{
    pService->Stop();
    return MakeCStopResultPtr("ibrio server stopping");
}

CRPCResultPtr CRPCMod::RPCVersion(CRPCParamPtr param)
{
    string strVersion = string("Ibrio server version is v") + VERSION_STR + string(", git commit id is ") + GetGitVersion();
    return MakeCVersionResultPtr(strVersion);
}

/* Network */
CRPCResultPtr CRPCMod::RPCGetPeerCount(CRPCParamPtr param)
{
    return MakeCGetPeerCountResultPtr(pService->GetPeerCount());
}

CRPCResultPtr CRPCMod::RPCListPeer(CRPCParamPtr param)
{
    vector<network::CBbPeerInfo> vPeerInfo;
    pService->GetPeers(vPeerInfo);

    auto spResult = MakeCListPeerResultPtr();
    for (const network::CBbPeerInfo& info : vPeerInfo)
    {
        CListPeerResult::CPeer peer;
        peer.strAddress = info.strAddress;
        if (info.nService == 0)
        {
            // Handshaking
            peer.strServices = "NON";
        }
        else
        {
            if (info.nService & network::NODE_NETWORK)
            {
                peer.strServices = "NODE_NETWORK";
            }
            if (info.nService & network::NODE_DELEGATED)
            {
                if (peer.strServices.empty())
                {
                    peer.strServices = "NODE_DELEGATED";
                }
                else
                {
                    peer.strServices = peer.strServices + ",NODE_DELEGATED";
                }
            }
            if (peer.strServices.empty())
            {
                peer.strServices = string("OTHER:") + to_string(info.nService);
            }
        }
        peer.strLastsend = GetTimeString(info.nLastSend);
        peer.strLastrecv = GetTimeString(info.nLastRecv);
        peer.strConntime = GetTimeString(info.nActive);
        peer.nPingtime = info.nPingPongTimeDelta;
        peer.strVersion = FormatVersion(info.nVersion);
        peer.strSubver = info.strSubVer;
        peer.fInbound = info.fInBound;
        peer.nHeight = info.nStartingHeight;
        peer.nBanscore = info.nScore;
        spResult->vecPeer.push_back(peer);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCAddNode(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAddNodeParam>(param);
    string strNode = spParam->strNode;

    if (!pService->AddNode(CNetHost(strNode, Config()->nPort)))
    {
        throw CRPCException(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Failed to add node.");
    }

    return MakeCAddNodeResultPtr(string("Add node successfully: ") + strNode);
}

CRPCResultPtr CRPCMod::RPCRemoveNode(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveNodeParam>(param);
    string strNode = spParam->strNode;

    if (!pService->RemoveNode(CNetHost(strNode, Config()->nPort)))
    {
        throw CRPCException(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Failed to remove node.");
    }

    return MakeCRemoveNodeResultPtr(string("Remove node successfully: ") + strNode);
}

CRPCResultPtr CRPCMod::RPCGetForkCount(CRPCParamPtr param)
{
    return MakeCGetForkCountResultPtr(pService->GetForkCount());
}

CRPCResultPtr CRPCMod::RPCListFork(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListForkParam>(param);
    vector<pair<uint256, CProfile>> vFork;
    pService->ListFork(vFork, spParam->fAll);
    auto spResult = MakeCListForkResultPtr();
    for (size_t i = 0; i < vFork.size(); i++)
    {
        const uint256& hashFork = vFork[i].first;
        CProfile& profile = vFork[i].second;
        if (spParam->fAll || pForkManager->IsAllowed(hashFork))
        {
            CForkContext forkContext;
            if (!pForkManager->GetForkContext(hashFork, forkContext))
            {
                StdError("CRPCMod", "RPCListFork: Get fork context fail, fork: %s", hashFork.GetHex().c_str());
                continue;
            }

            int nLastHeight = 0;
            uint256 hashLastBlock;
            CBlockStatus statusBlock;
            if (pService->GetForkLastBlock(hashFork, nLastHeight, hashLastBlock))
            {
                pService->GetBlockStatus(hashLastBlock, statusBlock);
            }

            CListForkResult::CProfile displayProfile;
            displayProfile.strFork = hashFork.GetHex();
            displayProfile.strName = profile.strName;
            displayProfile.strSymbol = profile.strSymbol;
            displayProfile.dAmount = ValueFromToken(profile.nAmount);
            displayProfile.dReward = ValueFromToken(profile.nMintReward);
            displayProfile.nHalvecycle = (uint64)(profile.nHalveCycle);
            displayProfile.fIsolated = profile.IsIsolated();
            displayProfile.fPrivate = profile.IsPrivate();
            displayProfile.fEnclosed = profile.IsEnclosed();
            displayProfile.strOwner = CAddress(profile.destOwner).ToString();
            displayProfile.strCreatetxid = forkContext.txidEmbedded.GetHex();
            displayProfile.nCreateforkheight = forkContext.nJointHeight;
            displayProfile.strParentfork = forkContext.hashParent.GetHex();
            displayProfile.strForktype = profile.nForkType == FORK_TYPE_DEFI ? "defi" : "common";
            displayProfile.nForkheight = nLastHeight;
            displayProfile.strLastblock = hashLastBlock.ToString();
            displayProfile.dMoneysupply = ValueFromToken(statusBlock.nMoneySupply);
            displayProfile.dMoneydestroy = ValueFromToken(statusBlock.nMoneyDestroy);
            if (profile.nForkType == FORK_TYPE_DEFI)
            {
                displayProfile.strForktype = "defi";
                displayProfile.defi.nMintheight = profile.defi.nMintHeight;
                displayProfile.defi.dMaxsupply = ValueFromToken(profile.defi.nMaxSupply);
                displayProfile.defi.nCoinbasetype = profile.defi.nCoinbaseType;
                displayProfile.defi.nDecaycycle = profile.defi.nDecayCycle;
                displayProfile.defi.nCoinbasedecaypercent = profile.defi.nCoinbaseDecayPercent;
                displayProfile.defi.nInitcoinbasepercent = profile.defi.nInitCoinbasePercent;
                displayProfile.defi.nPromotionrewardpercent = profile.defi.nPromotionRewardPercent;
                displayProfile.defi.nRewardcycle = profile.defi.nRewardCycle;
                displayProfile.defi.dStakemintoken = ValueFromToken(profile.defi.nStakeMinToken);
                displayProfile.defi.nStakerewardpercent = profile.defi.nStakeRewardPercent;
                displayProfile.defi.nSupplycycle = profile.defi.nSupplyCycle;

                for (const auto& kv : profile.defi.mapPromotionTokenTimes)
                {
                    CListForkResult::CProfile::CDefi::CMappromotiontokentimes promotiontokentimes(kv.first, kv.second);
                    displayProfile.defi.vecMappromotiontokentimes.push_back(promotiontokentimes);
                }

                for (const auto& kv : profile.defi.mapCoinbasePercent)
                {
                    CListForkResult::CProfile::CDefi::CMapcoinbasepercent coinbasepercent(kv.first, kv.second);
                    displayProfile.defi.vecMapcoinbasepercent.push_back(coinbasepercent);
                }
            }
            else
            {
                displayProfile.strForktype = "common";
            }

            spResult->vecProfile.push_back(displayProfile);
        }
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetForkGenealogy(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetGenealogyParam>(param);

    //getgenealogy (-f="fork")
    uint256 fork;
    if (!GetForkHashOfDef(spParam->strFork, fork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    vector<pair<uint256, int>> vAncestry;
    vector<pair<int, uint256>> vSubline;
    if (!pService->GetForkGenealogy(fork, vAncestry, vSubline))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    auto spResult = MakeCGetGenealogyResultPtr();
    for (int i = vAncestry.size(); i > 0; i--)
    {
        spResult->vecAncestry.push_back({ vAncestry[i - 1].first.GetHex(), vAncestry[i - 1].second });
    }
    for (std::size_t i = 0; i < vSubline.size(); i++)
    {
        spResult->vecSubline.push_back({ vSubline[i].second.GetHex(), vSubline[i].first });
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBlockLocation(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockLocationParam>(param);

    //getblocklocation <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    uint256 fork;
    int height;
    if (!pService->GetBlockLocation(hashBlock, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    auto spResult = MakeCGetBlockLocationResultPtr();
    spResult->strFork = fork.GetHex();
    spResult->nHeight = height;
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBlockCount(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockCountParam>(param);

    //getblockcount (-f="fork")
    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    return MakeCGetBlockCountResultPtr(pService->GetBlockCount(hashFork));
}

CRPCResultPtr CRPCMod::RPCGetBlockHash(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockHashParam>(param);

    //getblockhash <height> (-f="fork")
    int nHeight = spParam->nHeight;

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    vector<uint256> vBlockHash;
    if (!pService->GetBlockHash(hashFork, nHeight, vBlockHash))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Block number out of range.");
    }

    auto spResult = MakeCGetBlockHashResultPtr();
    for (const uint256& hash : vBlockHash)
    {
        spResult->vecHash.push_back(hash.GetHex());
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBlock(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBlockParam>(param);

    //getblock <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    CBlock block;
    uint256 fork;
    int height;
    if (!pService->GetBlock(hashBlock, block, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    return MakeCGetBlockResultPtr(BlockToJSON(hashBlock, block, fork, height));
}

CRPCResultPtr CRPCMod::RPCGetBlockDetail(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CgetblockdetailParam>(param);

    //getblockdetail <"block">
    uint256 hashBlock;
    hashBlock.SetHex(spParam->strBlock);

    CBlockEx block;
    uint256 fork;
    int height;
    if (!pService->GetBlockEx(hashBlock, block, fork, height))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown block");
    }

    Cblockdatadetail data;

    data.strHash = hashBlock.GetHex();
    data.strHashprev = block.hashPrev.GetHex();
    data.nVersion = block.nVersion;
    data.strType = GetBlockTypeStr(block.nType, block.txMint.nType);
    data.nTime = block.GetBlockTime();
    if (block.hashPrev != 0)
    {
        data.strPrev = block.hashPrev.GetHex();
    }
    data.strFork = fork.GetHex();
    data.nHeight = height;
    int nDepth = height < 0 ? 0 : pService->GetForkHeight(fork) - height;
    if (fork != pCoreProtocol->GetGenesisBlockHash())
    {
        nDepth = nDepth * 30;
    }
    data.txmint = TxToJSON(block.txMint.GetHash(), block.txMint, fork, hashBlock, nDepth, CAddress().ToString());
    if (block.IsProofOfWork())
    {
        CProofOfHashWorkCompact proof;
        proof.Load(block.vchProof);
        data.nBits = proof.nBits;
    }
    else
    {
        data.nBits = 0;
    }
    for (int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction& tx = block.vtx[i];
        data.vecTx.push_back(TxToJSON(tx.GetHash(), tx, fork, hashBlock, nDepth, CAddress(block.vTxContxt[i].destIn).ToString()));
    }
    return MakeCgetblockdetailResultPtr(data);
}

CRPCResultPtr CRPCMod::RPCGetTxPool(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTxPoolParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    CAddress address;
    if (spParam->strAddress.IsValid())
    {
        address = CAddress(spParam->strAddress);
        if (address.IsNull())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
        }
    }
    bool fDetail = spParam->fDetail.IsValid() ? bool(spParam->fDetail) : false;
    int64 nGetOffset = spParam->nGetoffset.IsValid() ? int64(spParam->nGetoffset) : 0;
    int64 nGetCount = spParam->nGetcount.IsValid() ? int64(spParam->nGetcount) : 20;

    auto spResult = MakeCGetTxPoolResultPtr();
    if (!fDetail)
    {
        vector<pair<uint256, size_t>> vTxPool;
        pService->GetTxPool(hashFork, vTxPool);

        size_t nTotalSize = 0;
        for (std::size_t i = 0; i < vTxPool.size(); i++)
        {
            nTotalSize += vTxPool[i].second;
        }
        spResult->nCount = vTxPool.size();
        spResult->nSize = nTotalSize;
    }
    else
    {
        vector<CTxInfo> vTxPool;
        pService->ListTxPool(hashFork, address, vTxPool, nGetOffset, nGetCount);

        for (const CTxInfo& txinfo : vTxPool)
        {
            spResult->vecList.push_back({ txinfo.txid.GetHex(), CTransaction::GetTypeStringStatic(txinfo.nTxType), CAddress(txinfo.destFrom).ToString(),
                                          CAddress(txinfo.destTo).ToString(), ValueFromToken(txinfo.nAmount),
                                          ValueFromToken(txinfo.nTxFee), txinfo.nSize });
        }
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetTransaction(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTransactionParam>(param);
    uint256 txid;
    txid.SetHex(spParam->strTxid);
    if (txid == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid txid");
    }
    if (txid == CTransaction().GetHash())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid txid");
    }

    CTransaction tx;
    uint256 hashFork;
    int nHeight;
    uint256 hashBlock;
    CDestination destIn;

    if (!pService->GetTransaction(txid, tx, hashFork, nHeight, hashBlock, destIn))
    {
        throw CRPCException(RPC_INVALID_REQUEST, "No information available about transaction");
    }

    auto spResult = MakeCGetTransactionResultPtr();
    if (spParam->fSerialized)
    {
        CBufStream ss;
        ss << tx;
        spResult->strSerialization = ToHexString((const unsigned char*)ss.GetData(), ss.GetSize());
        return spResult;
    }

    int nDepth = nHeight < 0 ? 0 : pService->GetForkHeight(hashFork) - nHeight;
    if (hashFork != pCoreProtocol->GetGenesisBlockHash())
    {
        nDepth = nDepth * 30;
    }

    spResult->transaction = TxToJSON(txid, tx, hashFork, hashBlock, nDepth, CAddress(destIn).ToString());
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSendTransaction(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSendTransactionParam>(param);

    vector<unsigned char> txData = ParseHexString(spParam->strTxdata);
    CBufStream ss;
    ss.Write((char*)&txData[0], txData.size());
    CTransaction rawTx;
    try
    {
        ss >> rawTx;
    }
    catch (const std::exception& e)
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }
    Errno err = pService->SendTransaction(rawTx);
    if (err != OK)
    {
        throw CRPCException(RPC_TRANSACTION_REJECTED, string("Tx rejected : ")
                                                          + ErrorString(err));
    }

    return MakeCSendTransactionResultPtr(rawTx.GetHash().GetHex());
}

CRPCResultPtr CRPCMod::RPCGetForkHeight(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetForkHeightParam>(param);

    //getforkheight (-f="fork")
    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    return MakeCGetForkHeightResultPtr(pService->GetForkHeight(hashFork));
}

CRPCResultPtr CRPCMod::RPCGetVotes(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetVotesParam>(param);

    CAddress destDelegate(spParam->strAddress);
    if (destDelegate.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
    }

    int64 nVotesToken;
    string strFailCause;
    if (!pService->GetVotes(destDelegate, nVotesToken, strFailCause))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, strFailCause);
    }

    return MakeCGetVotesResultPtr(ValueFromToken(nVotesToken));
}

CRPCResultPtr CRPCMod::RPCListDelegate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListDelegateParam>(param);

    std::multimap<int64, std::pair<CDestination, std::string>> mapVotes;
    if (!pService->ListDelegate(spParam->nCount, mapVotes))
    {
        throw CRPCException(RPC_INTERNAL_ERROR, "Query fail");
    }

    auto spResult = MakeCListDelegateResultPtr();
    for (const auto& d : boost::adaptors::reverse(mapVotes))
    {
        CListDelegateResult::CDelegate delegateData;
        delegateData.strName = d.second.second;
        delegateData.strAddress = CAddress(d.second.first).ToString();
        delegateData.dVotes = ValueFromToken(d.first);
        spResult->vecDelegate.push_back(delegateData);
    }
    return spResult;
}

/* Wallet */
CRPCResultPtr CRPCMod::RPCListKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListKeyParam>(param);

    uint64 nPage = 0;
    uint64 nCount = 30;
    if (spParam->nPage.IsValid())
    {
        nPage = spParam->nPage;
    }
    if (spParam->nCount.IsValid())
    {
        nCount = spParam->nCount;
    }

    set<crypto::CPubKey> setPubKey;
    pService->GetPubKeys(setPubKey, nCount * nPage, nCount);

    auto spResult = MakeCListKeyResultPtr();
    for (const crypto::CPubKey& pubkey : setPubKey)
    {
        int nVersion;
        bool fLocked, fPublic;
        int64 nAutoLockTime;
        if (pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
        {
            CListKeyResult::CPubkey p;
            p.strKey = pubkey.GetHex();
            p.nVersion = nVersion;
            p.fPublic = fPublic;
            p.fLocked = fLocked;
            if (!fLocked && nAutoLockTime > 0)
            {
                p.nTimeout = (nAutoLockTime - GetTime());
            }
            spResult->vecPubkey.push_back(p);
        }
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetNewKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetNewKeyParam>(param);

    if (spParam->strPassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Passphrase must be nonempty");
    }

    crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();
    crypto::CPubKey pubkey;
    auto strErr = pService->MakeNewKey(strPassphrase, pubkey);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_ERROR, std::string("Failed add new key: ") + *strErr);
    }

    return MakeCGetNewKeyResultPtr(pubkey.ToString());
}

CRPCResultPtr CRPCMod::RPCEncryptKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CEncryptKeyParam>(param);

    //encryptkey <"pubkey"> <-new="passphrase"> <-old="oldpassphrase">
    crypto::CPubKey pubkey;
    pubkey.SetHex(spParam->strPubkey);

    if (spParam->strPassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Passphrase must be nonempty");
    }
    crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();

    if (spParam->strOldpassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Old passphrase must be nonempty");
    }
    crypto::CCryptoString strOldPassphrase = spParam->strOldpassphrase.c_str();

    if (!pService->HaveKey(pubkey, crypto::CKey::PRIVATE_KEY))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (!pService->EncryptKey(pubkey, strPassphrase, strOldPassphrase))
    {
        throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "The passphrase entered was incorrect.");
    }

    return MakeCEncryptKeyResultPtr(string("Encrypt key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCLockKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CLockKeyParam>(param);

    CAddress address(spParam->strPubkey);
    if (address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "This method only accepts pubkey or pubkey address as parameter rather than template address you supplied.");
    }

    crypto::CPubKey pubkey;
    if (address.IsPubKey())
    {
        address.GetPubKey(pubkey);
    }
    else
    {
        pubkey.SetHex(spParam->strPubkey);
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (fPublic)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't lock public key");
    }
    if (!fLocked && !pService->Lock(pubkey))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to lock key");
    }
    return MakeCLockKeyResultPtr(string("Lock key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCUnlockKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CUnlockKeyParam>(param);

    CAddress address(spParam->strPubkey);
    if (address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "This method only accepts pubkey or pubkey address as parameter rather than template address you supplied.");
    }

    crypto::CPubKey pubkey;
    if (address.IsPubKey())
    {
        address.GetPubKey(pubkey);
    }
    else
    {
        pubkey.SetHex(spParam->strPubkey);
    }

    if (spParam->strPassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Passphrase must be nonempty");
    }

    crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();
    int64 nTimeout = 0;
    if (spParam->nTimeout.IsValid())
    {
        nTimeout = spParam->nTimeout;
    }
    else if (!RPCServerConfig()->fDebug)
    {
        nTimeout = UNLOCKKEY_RELEASE_DEFAULT_TIME;
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (fPublic)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't unlock public key");
    }
    if (!fLocked)
    {
        throw CRPCException(RPC_WALLET_ALREADY_UNLOCKED, "Key is already unlocked");
    }

    if (!pService->Unlock(pubkey, strPassphrase, nTimeout))
    {
        throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "The passphrase entered was incorrect.");
    }

    return MakeCUnlockKeyResultPtr(string("Unlock key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCRemoveKey(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveKeyParam>(param);
    CAddress address(spParam->strPubkey);
    if (address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "This method only accepts pubkey or pubkey address as parameter rather than template address you supplied.");
    }

    crypto::CPubKey pubkey;
    if (address.IsPubKey())
    {
        address.GetPubKey(pubkey);
    }
    else
    {
        pubkey.SetHex(spParam->strPubkey);
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }

    if (!fPublic)
    {
        pService->Lock(pubkey);
        crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();
        if (!pService->Unlock(pubkey, strPassphrase, UNLOCKKEY_RELEASE_DEFAULT_TIME))
        {
            throw CRPCException(RPC_WALLET_PASSPHRASE_INCORRECT, "Can't remove key with incorrect passphrase");
        }
    }

    auto strErr = pService->RemoveKey(pubkey);
    if (strErr)
    {
        throw CRPCException(RPC_WALLET_REMOVE_KEY_ERROR, *strErr);
    }

    return MakeCRemoveKeyResultPtr(string("Remove key successfully: ") + spParam->strPubkey);
}

CRPCResultPtr CRPCMod::RPCImportPrivKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportPrivKeyParam>(param);

    //importprivkey <"privkey"> <"passphrase">
    uint256 nPriv;
    if (nPriv.SetHex(spParam->strPrivkey) != spParam->strPrivkey.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid private key");
    }

    if (spParam->strPassphrase.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Passphrase must be nonempty");
    }

    crypto::CCryptoString strPassphrase = spParam->strPassphrase.c_str();

    crypto::CKey key;
    if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid private key");
    }
    if (!pService->HaveKey(key.GetPubKey(), crypto::CKey::PRIVATE_KEY))
    {
        if (!strPassphrase.empty())
        {
            key.Encrypt(strPassphrase);
        }
        auto strErr = pService->AddKey(key);
        if (strErr)
        {
            throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
        }
    }

    return MakeCImportPrivKeyResultPtr(key.GetPubKey().GetHex());
}

CRPCResultPtr CRPCMod::RPCImportPubKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportPubKeyParam>(param);

    //importpubkey <"pubkey"> or importpubkey <"pubkeyaddress">
    CAddress address(spParam->strPubkey);
    if (address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Template id is not allowed");
    }

    crypto::CPubKey pubkey;
    if (address.IsPubKey())
    {
        address.GetPubKey(pubkey);
    }
    else if (pubkey.SetHex(spParam->strPubkey) != spParam->strPubkey.size())
    {
        pubkey.SetHex(spParam->strPubkey);
    }

    crypto::CKey key;
    key.Load(pubkey, crypto::CKey::PUBLIC_KEY, crypto::CCryptoCipher());
    if (!pService->HaveKey(key.GetPubKey()))
    {
        auto strErr = pService->AddKey(key);
        if (strErr)
        {
            throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
        }
    }

    CDestination dest(pubkey);
    return MakeCImportPubKeyResultPtr(CAddress(dest).ToString());
}

CRPCResultPtr CRPCMod::RPCImportKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportKeyParam>(param);

    vector<unsigned char> vchKey = ParseHexString(spParam->strPubkey);
    crypto::CKey key;
    if (!key.Load(vchKey))
    {
        throw CRPCException(RPC_INVALID_PARAMS, "Failed to verify serialized key");
    }
    if (key.GetVersion() == crypto::CKey::INIT)
    {
        throw CRPCException(RPC_INVALID_PARAMS, "Can't import the key with empty passphrase");
    }
    if ((key.IsPrivKey() && !pService->HaveKey(key.GetPubKey(), crypto::CKey::PRIVATE_KEY))
        || (key.IsPubKey() && !pService->HaveKey(key.GetPubKey())))
    {
        auto strErr = pService->AddKey(key);
        if (strErr)
        {
            throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
        }
    }

    return MakeCImportKeyResultPtr(key.GetPubKey().GetHex());
}

CRPCResultPtr CRPCMod::RPCExportKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CExportKeyParam>(param);

    crypto::CPubKey pubkey;
    pubkey.SetHex(spParam->strPubkey);

    if (!pService->HaveKey(pubkey))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    vector<unsigned char> vchKey;
    if (!pService->ExportKey(pubkey, vchKey))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to export key");
    }

    return MakeCExportKeyResultPtr(ToHexString(vchKey));
}

CRPCResultPtr CRPCMod::RPCAddNewTemplate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAddNewTemplateParam>(param);
    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(spParam->data, CAddress());
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
    }
    if (!pService->HaveTemplate(ptr->GetTemplateId()))
    {
        if (!pService->AddTemplate(ptr))
        {
            throw CRPCException(RPC_WALLET_ERROR, "Failed to add template");
        }
    }

    return MakeCAddNewTemplateResultPtr(CAddress(ptr->GetTemplateId()).ToString());
}

CRPCResultPtr CRPCMod::RPCImportTemplate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportTemplateParam>(param);
    vector<unsigned char> vchTemplate = ParseHexString(spParam->strData);
    CTemplatePtr ptr = CTemplate::Import(vchTemplate);
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
    }
    if (!pService->HaveTemplate(ptr->GetTemplateId()))
    {
        if (!pService->AddTemplate(ptr))
        {
            throw CRPCException(RPC_WALLET_ERROR, "Failed to add template");
        }
    }

    return MakeCImportTemplateResultPtr(CAddress(ptr->GetTemplateId()).ToString());
}

CRPCResultPtr CRPCMod::RPCExportTemplate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CExportTemplateParam>(param);
    CAddress address(spParam->strAddress);
    if (address.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    CTemplateId tid = address.GetTemplateId();
    if (!tid)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    CTemplatePtr ptr = pService->GetTemplate(tid);
    if (!ptr)
    {
        throw CRPCException(RPC_WALLET_ERROR, "Unkown template");
    }

    vector<unsigned char> vchTemplate = ptr->Export();
    return MakeCExportTemplateResultPtr(ToHexString(vchTemplate));
}

CRPCResultPtr CRPCMod::RPCRemoveTemplate(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CRemoveTemplateParam>(param);
    CAddress address(spParam->strAddress);
    if (address.IsNull() || !address.IsTemplate())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid template address");
    }

    if (!pService->RemoveTemplate(address.GetTemplateId()))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Remove template address fail");
    }

    return MakeCRemoveTemplateResultPtr("Success");
}

CRPCResultPtr CRPCMod::RPCValidateAddress(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CValidateAddressParam>(param);

    CAddress address(spParam->strAddress);
    bool isValid = !address.IsNull();

    auto spResult = MakeCValidateAddressResultPtr();
    spResult->fIsvalid = isValid;
    if (isValid)
    {
        auto& addressData = spResult->addressdata;

        addressData.strAddress = address.ToString();
        if (address.IsPubKey())
        {
            crypto::CPubKey pubkey;
            address.GetPubKey(pubkey);
            bool isMine = pService->HaveKey(pubkey);
            addressData.fIsmine = isMine;
            addressData.strType = "pubkey";
            addressData.strPubkey = pubkey.GetHex();
        }
        else if (address.IsTemplate())
        {
            CTemplateId tid = address.GetTemplateId();
            uint16 nType = tid.GetType();
            CTemplatePtr ptr = pService->GetTemplate(tid);
            addressData.fIsmine = (ptr != nullptr);
            addressData.strType = "template";
            addressData.strTemplate = CTemplate::GetTypeName(nType);
            if (ptr)
            {
                auto& templateData = addressData.templatedata;

                templateData.strHex = ToHexString(ptr->Export());
                templateData.strType = ptr->GetName();
                ptr->GetTemplateData(templateData, CAddress());
            }
        }
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetDeFiRelation(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetDeFiRelationParam>(param);

    //getbalance (-f="fork") (-a="address")
    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    if (hashFork == pCoreProtocol->GetGenesisBlockHash())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "must be sub fork directly inherient from main fork");
    }

    CDestination Dest;
    if (spParam->strAddress.IsValid())
    {
        CAddress address(spParam->strAddress);
        if (address.IsNull())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
        }
        Dest = address;
    }
    else
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    auto spResult = MakeCGetDeFiRelationResultPtr();
    CDestination parentDest;
    if (pService->GetDeFiRelation(hashFork, Dest, parentDest))
    {
        spResult->strParent = CAddress(parentDest).ToString();
    }
    else
    {
        spResult->strParent = "";
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetBalance(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetBalanceParam>(param);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    vector<CDestination> vDest;
    if (spParam->strAddress.IsValid())
    {
        CAddress address(spParam->strAddress);
        if (address.IsNull())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
        }
        vDest.push_back(static_cast<CDestination&>(address));
    }
    else
    {
        uint64 nPage = 0;
        uint64 nCount = 30;
        if (spParam->nPage.IsValid())
        {
            nPage = spParam->nPage;
        }
        if (spParam->nCount.IsValid())
        {
            nCount = spParam->nCount;
        }
        ListDestination(vDest, nPage, nCount);
    }

    auto spResult = MakeCGetBalanceResultPtr();
    for (const CDestination& dest : vDest)
    {
        CWalletBalance balance;
        if (pService->GetBalanceByUnspent(dest, hashFork, balance))
        {
            CGetBalanceResult::CBalance b;
            b.strAddress = CAddress(dest).ToString();
            b.dAvail = ValueFromToken(balance.nAvailable);
            b.dLocked = ValueFromToken(balance.nLocked);
            b.dUnconfirmed = ValueFromToken(balance.nUnconfirmed);
            spResult->vecBalance.push_back(b);
        }
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCListTransaction(CRPCParamPtr param)
{
    if (!BasicConfig()->fAddrTxIndex)
    {
        throw CRPCException(RPC_INVALID_REQUEST, "If you need this function, please set config 'addrtxindex=true' and restart");
    }

    auto spParam = CastParamPtr<CListTransactionParam>(param);

    const CRPCString& strFork = spParam->strFork;
    const CRPCString& strAddress = spParam->strAddress;

    uint256 fork;
    if (!GetForkHashOfDef(strFork, fork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    CAddress address;
    if (!strAddress.empty() && !address.ParseString(strAddress))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address");
    }

    int nCount = GetUint(spParam->nCount, 10);
    int nOffset = GetInt(spParam->nOffset, 0);
    if (nCount <= 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Negative, zero or out of range count");
    }
    if (nOffset < -1)
    {
        nOffset = -1;
    }
    int nPrevHeight = GetInt(spParam->nPrevheight, -2);
    uint64 nPrevTxSeq = GetUint64(spParam->nPrevtxseq, -1);

    vector<CTxInfo> vTx;
    if (!address.IsNull())
    {
        if (!pService->ListTransaction(fork, address, nPrevHeight, nPrevTxSeq, nOffset, nCount, vTx))
        {
            throw CRPCException(RPC_WALLET_ERROR, "Failed to list transactions");
        }
    }
    else
    {
        set<CDestination> setWalletDest;
        pService->GetWalletDestinations(setWalletDest);

        map<pair<uint32, uint64>, CTxInfo> mapTxCache;
        for (const CDestination& dest : setWalletDest)
        {
            vector<CTxInfo> vCache;
            if (!pService->ListTransaction(fork, dest, -2, -1, 0, 0, vCache))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to list transactions");
            }
            for (const auto& vd : vCache)
            {
                mapTxCache.insert(make_pair(make_pair(vd.nBlockHeight, vd.nTxSeq), vd));
            }
            if (nOffset != -1 && mapTxCache.size() >= nOffset + nCount)
            {
                break;
            }
        }
        if (!mapTxCache.empty())
        {
            if (nPrevHeight < -1 || nPrevTxSeq == -1)
            {
                if (nOffset == -1)
                {
                    nOffset = mapTxCache.size() - nCount;
                    if (nOffset < 0)
                    {
                        nOffset = 0;
                    }
                }
                if (mapTxCache.size() > nOffset)
                {
                    vTx.reserve(nCount);
                    int64 nPos = 0;
                    for (const auto& vd : mapTxCache)
                    {
                        if (nPos++ >= nOffset)
                        {
                            vTx.push_back(vd.second);
                            if (vTx.size() >= nCount)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                vTx.reserve(nCount);
                for (const auto& vd : mapTxCache)
                {
                    if (vd.second.nBlockHeight > nPrevHeight
                        || (vd.second.nBlockHeight == nPrevHeight && vd.second.nTxSeq > nPrevTxSeq))
                    {
                        vTx.push_back(vd.second);
                        if (vTx.size() >= nCount)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }

    auto spResult = MakeCListTransactionResultPtr();
    for (const CTxInfo& tx : vTx)
    {
        bool fSendFrom = false;
        if ((tx.destFrom.IsPubKey() && pService->HaveKey(tx.destFrom.GetPubKey()))
            || (tx.destFrom.IsTemplate() && pService->HaveTemplate(tx.destFrom.GetTemplateId())))
        {
            fSendFrom = true;
        }
        spResult->vecTransaction.push_back(TxInfoToJSON(tx, fSendFrom));
    }
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSendFrom(CRPCParamPtr param)
{
    //sendfrom <"from"> <"to"> <$amount$> ($txfee$) (-f="fork") (-d="data")
    auto spParam = CastParamPtr<CSendFromParam>(param);
    CAddress from(spParam->strFrom);
    if (from.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
    }

    CAddress to(spParam->strTo);
    if (to.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
    }

    uint16 nType = (uint16)spParam->nType;
    if (nType != CTransaction::TX_TOKEN && nType != CTransaction::TX_DEFI_RELATION && nType != CTransaction::TX_DEFI_MINT_HEIGHT)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid tx type");
    }

    int64 nAmount = AmountFromValue(spParam->dAmount);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }
    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    int nLockHeight = 0;
    if (spParam->nLockheight.IsValid())
    {
        nLockHeight = (int)(spParam->nLockheight);
        if (nLockHeight < 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid lockheight");
        }
    }

    vector<unsigned char> vchData;
    if (spParam->strData.IsValid())
    {
        auto strDataTmp = spParam->strData;
        if (((std::string)strDataTmp).substr(0, 4) == "msg:")
        {
            auto hex = xengine::ToHexString((const unsigned char*)strDataTmp.c_str(), strlen(strDataTmp.c_str()));
            vchData = ParseHexString(hex);
        }
        else
        {
            vchData = ParseHexString(strDataTmp);
        }
    }
    if (nType == CTransaction::TX_DEFI_MINT_HEIGHT)
    {
        if (vchData.size() > 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "mint height tx can't set customized data");
        }
        if (!spParam->nMintheight.IsValid())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "mint height tx must set mint height param");
        }
        int32 nMintHeight = spParam->nMintheight;
        CODataStream os(vchData);
        os << nMintHeight;
    }

    int64 nTxFee = CalcMinTxFee(vchData.size(), MIN_TX_FEE);
    if (spParam->dTxfee.IsValid())
    {
        int64 nUserTxFee = AmountFromValue(spParam->dTxfee);
        if (nUserTxFee > nTxFee)
        {
            nTxFee = nUserTxFee;
        }
        StdTrace("[SendFrom]", "txudatasize : %d ; mintxfee : %d", vchData.size(), nTxFee);
    }

    CWalletBalance balance;
    if (!pService->GetBalanceByUnspent(from, hashFork, balance))
    {
        throw CRPCException(RPC_WALLET_ERROR, "GetBalanceByUnspent failed");
    }
    if (nAmount == -1)
    {
        if (balance.nAvailable <= nTxFee)
        {
            throw CRPCException(RPC_WALLET_ERROR, "Your amount not enough for txfee");
        }
        nAmount = balance.nAvailable - nTxFee;
    }

    CTransaction txNew;
    auto strErr = pService->CreateTransactionByUnspent(hashFork, from, to, nType, nAmount, nTxFee, nLockHeight, vchData, txNew);
    if (strErr)
    {
        boost::format fmt = boost::format(" Balance: %1% TxFee: %2%") % balance.nAvailable % txNew.nTxFee;
        throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to create transaction: ") + *strErr + fmt.str());
    }

    vector<uint8> vchSignExtraData;
    bool fCompleted = false;
    if (spParam->strSign_M.IsValid() && spParam->strSign_S.IsValid())
    {
        if (from.IsNull() || from.IsPubKey())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address,must be a template address");
        }
        else if (from.IsTemplate())
        {
            CTemplateId tid = from.GetTemplateId();
            uint16 nType = tid.GetType();
            if (nType != TEMPLATE_EXCHANGE && nType != TEMPLATE_DEXMATCH)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address,must be a template address");
            }
            if (spParam->strSign_M == "" || spParam->strSign_S == "")
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Both SS and SM parameter cannot be null");
            }
            vector<unsigned char> vsm = ParseHexString(spParam->strSign_M);
            vector<unsigned char> vss = ParseHexString(spParam->strSign_S);
            if (nType == TEMPLATE_EXCHANGE)
            {
                txNew.vchSig.clear();
                CODataStream ds(txNew.vchSig);
                ds << vsm << vss << hashFork << pService->GetForkHeight(hashFork);
            }
            else
            {
                CODataStream ds(vchSignExtraData);
                ds << vsm << vss;
            }
        }
        else
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
        }
    }

    vector<uint8> vchFromData;
    if (from.IsTemplate() && spParam->strFromdata.IsValid())
    {
        vchFromData = ParseHexString(spParam->strFromdata);
    }

    vector<uint8> vchSendToData;
    if (to.IsTemplate() && spParam->strSendtodata.IsValid())
    {
        vchSendToData = ParseHexString(spParam->strSendtodata);
    }

    bool fMemSig = false;
    crypto::CPubKey memPubkey;
    if (spParam->strSignsecret.IsValid())
    {
        uint256 hashSignsecret;
        if (hashSignsecret.SetHex(spParam->strSignsecret) != spParam->strSignsecret.size())
        {
            throw CRPCException(RPC_WALLET_ERROR, "signsecret error");
        }
        if (pService->AddMemKey(hashSignsecret, memPubkey))
        {
            fMemSig = true;
        }
    }

    if (!pService->SignTransaction(txNew, vchFromData, vchSendToData, vchSignExtraData, fCompleted))
    {
        if (fMemSig)
        {
            pService->RemoveMemKey(memPubkey);
        }
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign transaction");
    }
    if (!fCompleted)
    {
        if (fMemSig)
        {
            pService->RemoveMemKey(memPubkey);
        }
        throw CRPCException(RPC_WALLET_ERROR, "The signature is not completed");
    }

    if (fMemSig)
    {
        pService->RemoveMemKey(memPubkey);
    }

    Errno err = pService->SendTransaction(txNew);
    if (err != OK)
    {
        throw CRPCException(RPC_TRANSACTION_REJECTED, string("Tx rejected : ")
                                                          + ErrorString(err));
    }
    std::stringstream ss;
    for (auto& obj : txNew.vInput)
    {
        ss << (int)obj.prevout.n << ":" << obj.prevout.hash.GetHex().c_str() << ";";
    }

    StdDebug("[SendFrom][DEBUG]", "txNew hash:%s; input:%s", txNew.GetHash().GetHex().c_str(), ss.str().c_str());
    return MakeCSendFromResultPtr(txNew.GetHash().GetHex());
}

CRPCResultPtr CRPCMod::RPCCreateTransaction(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CCreateTransactionParam>(param);

    //createtransaction <"from"> <"to"> <$amount$> ($txfee$) (-f="fork") (-d="data")
    CAddress from(spParam->strFrom);
    if (from.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid from address");
    }

    CAddress to(spParam->strTo);
    if (to.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid to address");
    }

    uint16 nType = (uint16)spParam->nType;
    if (nType != CTransaction::TX_TOKEN && nType != CTransaction::TX_DEFI_RELATION && nType != CTransaction::TX_DEFI_MINT_HEIGHT)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid tx type");
    }

    int64 nAmount = AmountFromValue(spParam->dAmount);

    uint256 hashFork;
    if (!GetForkHashOfDef(spParam->strFork, hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    if (!pService->HaveFork(hashFork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown fork");
    }

    int nLockHeight = 0;
    if (spParam->nLockheight.IsValid())
    {
        nLockHeight = (int)(spParam->nLockheight);
        if (nLockHeight < 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid lockheight");
        }
    }

    vector<unsigned char> vchData;
    if (spParam->strData.IsValid())
    {
        vchData = ParseHexString(spParam->strData);
    }
    if (nType == CTransaction::TX_DEFI_MINT_HEIGHT)
    {
        if (vchData.size() > 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "mint height tx can't set customized data");
        }
        if (!spParam->nMintheight.IsValid())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "mint height tx must set mint height param");
        }
        int32 nMintHeight = spParam->nMintheight;
        CODataStream os(vchData);
        os << nMintHeight;
    }

    int64 nTxFee = CalcMinTxFee(vchData.size(), MIN_TX_FEE);
    if (spParam->dTxfee.IsValid())
    {
        nTxFee = AmountFromValue(spParam->dTxfee);

        int64 nFee = CalcMinTxFee(vchData.size(), MIN_TX_FEE);
        if (nTxFee < nFee)
        {
            nTxFee = nFee;
        }
        StdTrace("[CreateTransaction]", "txudatasize : %d ; mintxfee : %d", vchData.size(), nTxFee);
    }

    CWalletBalance balance;
    if (!pService->GetBalanceByUnspent(from, hashFork, balance))
    {
        throw CRPCException(RPC_WALLET_ERROR, "GetBalanceByUnspent failed");
    }
    if (nAmount == -1)
    {
        if (balance.nAvailable <= nTxFee)
        {
            throw CRPCException(RPC_WALLET_ERROR, "Your amount not enough for txfee");
        }
        nAmount = balance.nAvailable - nTxFee;
    }

    CTemplateId tid;
    if (to.GetTemplateId(tid) && tid.GetType() == TEMPLATE_FORK && nAmount < CTemplateFork::CreatedCoin())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "create transaction nAmount must be at least " + std::to_string(CTemplateFork::CreatedCoin() / COIN) + " for creating fork");
    }

    CTransaction txNew;
    auto strErr = pService->CreateTransactionByUnspent(hashFork, from, to, nType, nAmount, nTxFee, nLockHeight, vchData, txNew);
    if (strErr)
    {
        boost::format fmt = boost::format(" Balance: %1% TxFee: %2%") % balance.nAvailable % txNew.nTxFee;
        throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to create transaction: ") + *strErr + fmt.str());
    }

    CBufStream ss;
    ss << txNew;

    return MakeCCreateTransactionResultPtr(
        ToHexString((const unsigned char*)ss.GetData(), ss.GetSize()));
}

CRPCResultPtr CRPCMod::RPCSignTransaction(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSignTransactionParam>(param);

    vector<unsigned char> txData = ParseHexString(spParam->strTxdata);
    CBufStream ss;
    ss.Write((char*)&txData[0], txData.size());
    CTransaction rawTx;
    try
    {
        ss >> rawTx;
    }
    catch (const std::exception& e)
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    vector<uint8> vchSignExtraData;
    if (spParam->strSign_M.IsValid() && spParam->strSign_S.IsValid())
    {
        vector<uint8> vsm = ParseHexString(spParam->strSign_M);
        vector<uint8> vss = ParseHexString(spParam->strSign_S);
        CODataStream ds(vchSignExtraData);
        ds << vsm << vss;
    }

    vector<uint8> vchFromData;
    if (spParam->strFromdata.IsValid())
    {
        vchFromData = ParseHexString(spParam->strFromdata);
    }

    vector<uint8> vchSendToData;
    if (rawTx.sendTo.IsTemplate() && spParam->strSendtodata.IsValid())
    {
        vchSendToData = ParseHexString(spParam->strSendtodata);
    }

    bool fMemSig = false;
    crypto::CPubKey memPubkey;
    if (spParam->strSignsecret.IsValid())
    {
        uint256 hashSignsecret;
        if (hashSignsecret.SetHex(spParam->strSignsecret) != spParam->strSignsecret.size())
        {
            throw CRPCException(RPC_WALLET_ERROR, "signsecret error");
        }
        if (pService->AddMemKey(hashSignsecret, memPubkey))
        {
            fMemSig = true;
        }
    }

    bool fCompleted = false;
    if (!pService->SignTransaction(rawTx, vchFromData, vchSendToData, vchSignExtraData, fCompleted))
    {
        if (fMemSig)
        {
            pService->RemoveMemKey(memPubkey);
        }
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign transaction");
    }

    if (fMemSig)
    {
        pService->RemoveMemKey(memPubkey);
    }

    CBufStream ssNew;
    ssNew << rawTx;

    auto spResult = MakeCSignTransactionResultPtr();
    spResult->strHex = ToHexString((const unsigned char*)ssNew.GetData(), ssNew.GetSize());
    spResult->fCompleted = fCompleted;
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSignMessage(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSignMessageParam>(param);

    crypto::CPubKey pubkey;
    crypto::CKey privkey;
    if (spParam->strPubkey.IsValid())
    {
        if (pubkey.SetHex(spParam->strPubkey) != spParam->strPubkey.size())
        {
            throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid pubkey error");
        }
    }
    else if (spParam->strPrivkey.IsValid())
    {
        uint256 nPriv;
        if (nPriv.SetHex(spParam->strPrivkey) != spParam->strPrivkey.size())
        {
            throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
        }
        privkey.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end()));
    }
    else
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "no pubkey or privkey");
    }

    string strMessage = spParam->strMessage;

    if (!!pubkey)
    {
        int nVersion;
        bool fLocked, fPublic;
        int64 nAutoLockTime;
        if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
        {
            throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
        }
        if (fPublic)
        {
            throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't sign message by public key");
        }
        if (fLocked)
        {
            throw CRPCException(RPC_WALLET_UNLOCK_NEEDED, "Key is locked");
        }
    }

    vector<unsigned char> vchSig;
    if (spParam->strAddr.IsValid())
    {
        CAddress addr(spParam->strMessage);
        std::string ss = addr.ToString();
        if (addr.IsNull() || addr.IsPubKey())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address parameters");
        }
        if (!!pubkey)
        {
            if (!pService->SignSignature(pubkey, addr.GetTemplateId(), vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
        else
        {
            if (!privkey.Sign(addr.GetTemplateId(), vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
    }
    else
    {
        uint256 hashStr;
        if (spParam->fHasprefix)
        {
            CBufStream ss;
            const string strMessageMagic = "Ibrio Signed Message:\n";
            ss << strMessageMagic;
            ss << strMessage;
            hashStr = crypto::CryptoHash(ss.GetData(), ss.GetSize());
        }
        else
        {
            hashStr = crypto::CryptoHash(strMessage.data(), strMessage.size());
        }

        if (!!pubkey)
        {
            if (!pService->SignSignature(pubkey, hashStr, vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
        else
        {
            if (!privkey.Sign(hashStr, vchSig))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
            }
        }
    }
    return MakeCSignMessageResultPtr(ToHexString(vchSig));
}

CRPCResultPtr CRPCMod::RPCListAddress(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CListAddressParam>(param);

    uint64 nPage = 0;
    uint64 nCount = 30;
    if (spParam->nPage.IsValid())
    {
        nPage = spParam->nPage;
    }
    if (spParam->nCount.IsValid())
    {
        nCount = spParam->nCount;
    }

    auto spResult = MakeCListAddressResultPtr();
    vector<CDestination> vDes;
    ListDestination(vDes, nPage, nCount);
    for (const auto& des : vDes)
    {
        CListAddressResult::CAddressdata addressData;
        addressData.strAddress = CAddress(des).ToString();
        if (des.IsPubKey())
        {
            addressData.strType = "pubkey";
            crypto::CPubKey pubkey;
            des.GetPubKey(pubkey);
            addressData.strPubkey = pubkey.GetHex();
        }
        else if (des.IsTemplate())
        {
            addressData.strType = "template";

            CTemplateId tid = des.GetTemplateId();
            uint16 nType = tid.GetType();
            CTemplatePtr ptr = pService->GetTemplate(tid);
            addressData.strTemplate = CTemplate::GetTypeName(nType);

            auto& templateData = addressData.templatedata;
            templateData.strHex = ToHexString(ptr->Export());
            templateData.strType = ptr->GetName();
            ptr->GetTemplateData(templateData, CAddress());
        }
        else
        {
            continue;
        }
        spResult->vecAddressdata.push_back(addressData);
    }

    return spResult;
}

CRPCResultPtr CRPCMod::RPCExportWallet(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CExportWalletParam>(param);

#ifdef BOOST_CYGWIN_FS_PATH
    std::string strCygWinPathPrefix = "/cygdrive";
    std::size_t found = string(spParam->strPath).find(strCygWinPathPrefix);
    if (found != std::string::npos)
    {
        strCygWinPathPrefix = "";
    }
#else
    std::string strCygWinPathPrefix;
#endif

    fs::path pSave(string(strCygWinPathPrefix + spParam->strPath));
    //check if the file name given is available
    if (!pSave.is_absolute())
    {
        throw CRPCException(RPC_WALLET_ERROR, "Must be an absolute path.");
    }
    if (is_directory(pSave))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Cannot export to a folder.");
    }
    if (exists(pSave))
    {
        throw CRPCException(RPC_WALLET_ERROR, "File has been existed.");
    }
    if (pSave.filename() == "." || pSave.filename() == "..")
    {
        throw CRPCException(RPC_WALLET_ERROR, "Cannot export to a folder.");
    }

    if (!exists(pSave.parent_path()) && !create_directories(pSave.parent_path()))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to create directories.");
    }

    Array aAddr;
    vector<CDestination> vDes;
    ListDestination(vDes, 0, 0);
    for (const auto& des : vDes)
    {
        if (des.IsPubKey())
        {
            Object oKey;
            oKey.push_back(Pair("address", CAddress(des).ToString()));

            crypto::CPubKey pubkey;
            des.GetPubKey(pubkey);
            vector<unsigned char> vchKey;
            if (!pService->ExportKey(pubkey, vchKey))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to export key");
            }
            oKey.push_back(Pair("hex", ToHexString(vchKey)));
            aAddr.push_back(oKey);
        }

        if (des.IsTemplate())
        {
            Object oTemp;
            CAddress address(des);

            oTemp.push_back(Pair("address", address.ToString()));

            CTemplateId tid;
            if (!address.GetTemplateId(tid))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid template address");
            }
            CTemplatePtr ptr = pService->GetTemplate(tid);
            if (!ptr)
            {
                throw CRPCException(RPC_WALLET_ERROR, "Unkown template");
            }
            vector<unsigned char> vchTemplate = ptr->Export();

            oTemp.push_back(Pair("hex", ToHexString(vchTemplate)));

            aAddr.push_back(oTemp);
        }
    }
    //output them together to file
    try
    {
        std::ofstream ofs(pSave.string(), std::ios::out);
        if (!ofs)
        {
            throw runtime_error("write error");
        }

        write_stream(Value(aAddr), ofs, pretty_print);
        ofs.close();
    }
    catch (...)
    {
        throw CRPCException(RPC_WALLET_ERROR, "filesystem_error - failed to write.");
    }

    return MakeCExportWalletResultPtr(string("Wallet file has been saved at: ") + pSave.string());
}

CRPCResultPtr CRPCMod::RPCImportWallet(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CImportWalletParam>(param);

#ifdef BOOST_CYGWIN_FS_PATH
    std::string strCygWinPathPrefix = "/cygdrive";
    std::size_t found = string(spParam->strPath).find(strCygWinPathPrefix);
    if (found != std::string::npos)
    {
        strCygWinPathPrefix = "";
    }
#else
    std::string strCygWinPathPrefix;
#endif

    fs::path pLoad(string(strCygWinPathPrefix + spParam->strPath));
    //check if the file name given is available
    if (!pLoad.is_absolute())
    {
        throw CRPCException(RPC_WALLET_ERROR, "Must be an absolute path.");
    }
    if (!exists(pLoad) || is_directory(pLoad))
    {
        throw CRPCException(RPC_WALLET_ERROR, "File name is invalid.");
    }

    Value vWallet;
    try
    {
        fs::ifstream ifs(pLoad);
        if (!ifs)
        {
            throw runtime_error("read error");
        }

        read_stream(ifs, vWallet, RPC_MAX_DEPTH);
        ifs.close();
    }
    catch (...)
    {
        throw CRPCException(RPC_WALLET_ERROR, "Filesystem_error - failed to read.");
    }

    if (array_type != vWallet.type())
    {
        throw CRPCException(RPC_WALLET_ERROR, "Wallet file exported is invalid, check it and try again.");
    }

    Array aAddr;
    uint32 nKey = 0;
    uint32 nTemp = 0;
    for (const auto& oAddr : vWallet.get_array())
    {
        if (oAddr.get_obj()[0].name_ != "address" || oAddr.get_obj()[1].name_ != "hex")
        {
            throw CRPCException(RPC_WALLET_ERROR, "Data format is not correct, check it and try again.");
        }
        string sAddr = oAddr.get_obj()[0].value_.get_str(); //"address" field
        string sHex = oAddr.get_obj()[1].value_.get_str();  //"hex" field

        CAddress addr(sAddr);
        if (addr.IsNull())
        {
            throw CRPCException(RPC_WALLET_ERROR, "Data format is not correct, check it and try again.");
        }

        //import keys
        if (addr.IsPubKey())
        {
            vector<unsigned char> vchKey = ParseHexString(sHex);
            crypto::CKey key;
            if (!key.Load(vchKey))
            {
                throw CRPCException(RPC_INVALID_PARAMS, "Failed to verify serialized key");
            }
            if (key.GetVersion() == crypto::CKey::INIT)
            {
                throw CRPCException(RPC_INVALID_PARAMS, "Can't import the key with empty passphrase");
            }
            if ((key.IsPrivKey() && pService->HaveKey(key.GetPubKey(), crypto::CKey::PRIVATE_KEY))
                || (key.IsPubKey() && pService->HaveKey(key.GetPubKey())))
            {
                continue; //step to next one to continue importing
            }
            auto strErr = pService->AddKey(key);
            if (strErr)
            {
                throw CRPCException(RPC_WALLET_ERROR, std::string("Failed to add key: ") + *strErr);
            }
            aAddr.push_back(key.GetPubKey().GetHex());
            ++nKey;
        }

        //import templates
        if (addr.IsTemplate())
        {
            vector<unsigned char> vchTemplate = ParseHexString(sHex);
            CTemplatePtr ptr = CTemplate::Import(vchTemplate);
            if (ptr == nullptr)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
            }
            if (pService->HaveTemplate(addr.GetTemplateId()))
            {
                continue; //step to next one to continue importing
            }
            if (!pService->AddTemplate(ptr))
            {
                throw CRPCException(RPC_WALLET_ERROR, "Failed to add template");
            }
            aAddr.push_back(CAddress(ptr->GetTemplateId()).ToString());
            ++nTemp;
        }
    }

    return MakeCImportWalletResultPtr(string("Imported ") + std::to_string(nKey)
                                      + string(" keys and ") + std::to_string(nTemp) + string(" templates."));
}

CRPCResultPtr CRPCMod::RPCMakeOrigin(CRPCParamPtr param)
{
#ifdef DISABLE_FORK_CREATETION
    throw CRPCException(RPC_INVALID_REQUEST, "Disable fork creation");
#else
    auto spParam = CastParamPtr<CMakeOriginParam>(param);

    //makeorigin <"prev"> <"owner"> <$amount$> <"name"> <"symbol"> <$reward$> <halvecycle> (-i|-noi*isolated*) (-p|-nop*private*) (-e|-noe*enclosed*)
    uint256 hashPrev;
    hashPrev.SetHex(spParam->strPrev);

    CDestination destOwner = static_cast<CDestination>(CAddress(spParam->strOwner));
    if (destOwner.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid owner");
    }

    int64 nAmount = AmountFromValue(spParam->dAmount, true);
    int64 nMintReward = AmountFromValue(spParam->dReward, true);
    if (!RewardRange(nMintReward))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid reward");
    }

    if (spParam->strName.empty() || spParam->strName.size() > 128
        || spParam->strSymbol.empty() || spParam->strSymbol.size() > 16)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid name or symbol");
    }

    CBlock blockPrev;
    uint256 hashParent;
    int nJointHeight;
    if (!pService->GetBlock(hashPrev, blockPrev, hashParent, nJointHeight))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown prev block");
    }

    if (blockPrev.IsExtended() || blockPrev.IsVacant())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Prev block should not be extended/vacant block");
    }

    int nForkHeight = pService->GetForkHeight(hashParent);
    if (nForkHeight < nJointHeight + MIN_CREATE_FORK_INTERVAL_HEIGHT)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, string("The minimum confirmed height of the previous block is ") + to_string(MIN_CREATE_FORK_INTERVAL_HEIGHT));
    }
    if ((int64)nForkHeight > (int64)nJointHeight + MAX_JOINT_FORK_INTERVAL_HEIGHT)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, string("Maximum fork spacing height is ") + to_string(MAX_JOINT_FORK_INTERVAL_HEIGHT));
    }

    uint256 hashBlockRef;
    int64 nTimeRef;
    if (!pService->GetLastBlockOfHeight(pCoreProtocol->GetGenesisBlockHash(), nJointHeight + 1, hashBlockRef, nTimeRef))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Failed to query main chain reference block");
    }

    CProfile profile;
    profile.strName = spParam->strName;
    profile.strSymbol = spParam->strSymbol;
    profile.destOwner = destOwner;
    profile.hashParent = hashParent;
    profile.nJointHeight = nJointHeight;
    profile.nAmount = nAmount;
    profile.nMintReward = nMintReward;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nHalveCycle = spParam->nHalvecycle;
    profile.SetFlag(spParam->fIsolated, spParam->fPrivate, spParam->fEnclosed);

    if (spParam->strForktype == "defi")
    {
        profile.nForkType = FORK_TYPE_DEFI;
        if (profile.nMintReward != 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi fork mint reward must be zero");
        }

        if (profile.nHalveCycle != 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi fork mint halvecycle must be zero");
        }

        profile.defi.nMintHeight = spParam->defi.nMintheight;
        if (profile.defi.nMintHeight > 0 && profile.defi.nMintHeight < nJointHeight + 2)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param mintheight should be -1 or larger than fork genesis block height");
        }
        else if (profile.defi.nMintHeight < -1)
        {
            profile.defi.nMintHeight = -1;
        }

        profile.defi.nMaxSupply = spParam->defi.nMaxsupply;
        if (profile.defi.nMaxSupply >= 0)
        {
            try
            {
                profile.defi.nMaxSupply = AmountFromValue(profile.defi.nMaxSupply, true);
                if (profile.defi.nMaxSupply < profile.nAmount)
                {
                    throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param maxsupply is less than amount");
                }
            }
            catch (...)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param maxsupply is out of range");
            }
        }

        profile.defi.nRewardCycle = spParam->defi.nRewardcycle;
        if (profile.defi.nRewardCycle <= 0 || profile.defi.nRewardCycle > 100 * YEAR_HEIGHT)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, (string("DeFi param rewardcycle must be [1, ") + to_string(100 * YEAR_HEIGHT) + "]").c_str());
        }

        profile.defi.nSupplyCycle = spParam->defi.nSupplycycle;
        if (profile.defi.nSupplyCycle <= 0 || profile.defi.nSupplyCycle > 100 * YEAR_HEIGHT)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, (string("DeFi param supplycycle must be [1, ") + to_string(100 * YEAR_HEIGHT) + "]").c_str());
        }

        if (profile.defi.nSupplyCycle % profile.defi.nRewardCycle != 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param nSupplyCycle must be divisible by nRewardCycle");
        }

        profile.defi.nCoinbaseType = spParam->defi.nCoinbasetype;
        if (profile.defi.nCoinbaseType == FIXED_DEFI_COINBASE_TYPE)
        {
            profile.defi.nDecayCycle = spParam->defi.nDecaycycle;
            if (profile.defi.nDecayCycle < 0 || profile.defi.nDecayCycle > 100 * YEAR_HEIGHT)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, (string("DeFi param decayCycle must be [0, ") + to_string(100 * YEAR_HEIGHT) + "]").c_str());
            }

            profile.defi.nCoinbaseDecayPercent = spParam->defi.nCoinbasedecaypercent;
            if (profile.defi.nCoinbaseDecayPercent > 100)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param coinbasedecaypercent must be [0, 100]");
            }

            profile.defi.nInitCoinbasePercent = spParam->defi.nInitcoinbasepercent;
            if (profile.defi.nInitCoinbasePercent == 0 || profile.defi.nInitCoinbasePercent > 10000)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param initcoinbasepercent must be [1, 10000]");
            }

            if (profile.defi.nDecayCycle % profile.defi.nSupplyCycle != 0)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param decaycycle must be divisible by supplycycle");
            }
        }
        else if (profile.defi.nCoinbaseType == SPECIFIC_DEFI_COINBASE_TYPE)
        {
            if (spParam->defi.vecMapcoinbasepercent.size() == 0)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param mapcoinbasepercent is empty");
            }

            for (int i = 0; i < spParam->defi.vecMapcoinbasepercent.size(); i++)
            {
                const int32 key = spParam->defi.vecMapcoinbasepercent.at(i).nHeight;
                if (key <= 0)
                {
                    throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param key of mapcoinbasepercent means height, must be larger than 0");
                }
                if (key % profile.defi.nSupplyCycle != 0)
                {
                    throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param key of mapcoinbasePercent must be divisible by supplycycle");
                }

                const uint32 value = spParam->defi.vecMapcoinbasepercent.at(i).nPercent;
                if (value == 0)
                {
                    throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param value of mapcoinbasepercent must be larger than 0");
                }
                profile.defi.mapCoinbasePercent.insert(std::make_pair(key, value));
            }
        }
        else
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param coinbasetype is out of range");
        }

        profile.defi.nStakeRewardPercent = spParam->defi.nStakerewardpercent;
        profile.defi.nPromotionRewardPercent = spParam->defi.nPromotionrewardpercent;
        if (profile.defi.nStakeRewardPercent > 100)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param stakerewardpercent must be [0, 100]");
        }
        if (profile.defi.nPromotionRewardPercent > 100)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param promotionrewardpercent must be [0, 100]");
        }
        if (profile.defi.nStakeRewardPercent + profile.defi.nPromotionRewardPercent > 100)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param (stakerewardpercent + promotionrewardpercent) must be [0, 100]");
        }

        try
        {
            profile.defi.nStakeMinToken = AmountFromValue(spParam->defi.nStakemintoken);
        }
        catch (...)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param stakemintoken is out of range");
        }

        for (int i = 0; i < spParam->defi.vecMappromotiontokentimes.size(); i++)
        {
            const int64 nToken = spParam->defi.vecMappromotiontokentimes.at(i).nToken;
            if (nToken <= 0 || nToken > ValueFromToken(MAX_MONEY))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, (string("DeFi param token of mappromotiontokentimes should be (0, ") + to_string(ValueFromToken(MAX_MONEY)) + "]").c_str());
            }
            const uint32 nTimes = spParam->defi.vecMappromotiontokentimes.at(i).nTimes;
            if (nTimes == 0)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param times of mappromotiontokentimes is equal 0");
            }
            int64 nMaxPower = profile.defi.nMaxSupply / COIN * nTimes;
            if (nMaxPower < (profile.defi.nMaxSupply / COIN))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param times * maxsupply is overflow");
            }
            // precision
            if (to_string(nMaxPower).size() > 14)
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "DeFi param times * maxsupply is more than 15 digits. It will lose precision");
            }
            profile.defi.mapPromotionTokenTimes.insert(std::make_pair(nToken, nTimes));
        }
    }
    else
    {
        profile.nForkType = FORK_TYPE_COMMON;
    }

    CBlock block;
    block.nVersion = 1;
    block.nType = CBlock::BLOCK_ORIGIN;
    block.nTimeStamp = nTimeRef;
    block.hashPrev = hashPrev;
    profile.Save(block.vchProof);

    CTransaction& tx = block.txMint;
    tx.nType = CTransaction::TX_GENESIS;
    tx.nTimeStamp = block.nTimeStamp;
    tx.sendTo = destOwner;
    tx.nAmount = nAmount;
    tx.vchData.assign(profile.strName.begin(), profile.strName.end());

    crypto::CPubKey pubkey;
    if (!destOwner.GetPubKey(pubkey))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Owner' address should be pubkey address");
    }

    int nVersion;
    bool fLocked, fPublic;
    int64 nAutoLockTime;
    if (!pService->GetKeyStatus(pubkey, nVersion, fLocked, nAutoLockTime, fPublic))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Unknown key");
    }
    if (fPublic)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Can't sign origin block by public key");
    }
    if (fLocked)
    {
        throw CRPCException(RPC_WALLET_UNLOCK_NEEDED, "Key is locked");
    }

    uint256 hashBlock = block.GetHash();
    if (!pService->SignSignature(pubkey, hashBlock, block.vchSig))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign message");
    }

    CBufStream ss;
    ss << block;

    auto spResult = MakeCMakeOriginResultPtr();
    spResult->strHash = hashBlock.GetHex();
    spResult->strHex = ToHexString((const unsigned char*)ss.GetData(), ss.GetSize());

    return spResult;
#endif
}

CRPCResultPtr CRPCMod::RPCSignRawTransactionWithWallet(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSignRawTransactionWithWalletParam>(param);

    CAddress addr(spParam->strAddrin);
    crypto::CPubKey pubkey;
    CTemplateId tid;
    bool fPubkey = true;
    if (addr.IsPubKey())
    {
        pubkey = addr.data;
    }
    else if (addr.IsTemplate())
    {
        tid = addr.data;
        fPubkey = false;
    }

    vector<unsigned char> txData = ParseHexString(spParam->strTxdata);
    CBufStream ss;
    ss.Write((char*)&txData[0], txData.size());
    CTransaction rawTx;
    try
    {
        ss >> rawTx;
    }
    catch (const std::exception& e)
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    bool fCompleted = false;
    CDestination destIn;
    if (fPubkey)
    {
        destIn.SetPubKey(pubkey);
    }
    else
    {
        destIn.SetTemplateId(tid);
    }

    vector<uint8> vchSignExtraData;
    if (spParam->strSign_M.IsValid() && spParam->strSign_S.IsValid())
    {
        vector<uint8> vsm = ParseHexString(spParam->strSign_M);
        vector<uint8> vss = ParseHexString(spParam->strSign_S);
        CODataStream ds(vchSignExtraData);
        ds << vsm << vss;
    }

    vector<uint8> vchFromData;
    if (spParam->strFromdata.IsValid())
    {
        vchFromData = ParseHexString(spParam->strFromdata);
    }

    vector<uint8> vchSendToData;
    if (rawTx.sendTo.IsTemplate() && spParam->strSendtodata.IsValid())
    {
        vchSendToData = ParseHexString(spParam->strSendtodata);
    }

    bool fMemSig = false;
    crypto::CPubKey memPubkey;
    if (spParam->strSignsecret.IsValid())
    {
        uint256 hashSignsecret;
        if (hashSignsecret.SetHex(spParam->strSignsecret) != spParam->strSignsecret.size())
        {
            throw CRPCException(RPC_WALLET_ERROR, "signsecret error");
        }
        if (pService->AddMemKey(hashSignsecret, memPubkey))
        {
            fMemSig = true;
        }
    }

    if (!pService->SignOfflineTransaction(destIn, rawTx, vchFromData, vchSendToData, vchSignExtraData, fCompleted))
    {
        if (fMemSig)
        {
            pService->RemoveMemKey(memPubkey);
        }
        throw CRPCException(RPC_WALLET_ERROR, "Failed to sign offline transaction");
    }

    if (fMemSig)
    {
        pService->RemoveMemKey(memPubkey);
    }

    CBufStream ssNew;
    ssNew << rawTx;

    auto spResult = MakeCSignRawTransactionWithWalletResultPtr();
    spResult->strHex = ToHexString((const unsigned char*)ssNew.GetData(), ssNew.GetSize());
    spResult->fCompleted = fCompleted;
    return spResult;
}

CRPCResultPtr CRPCMod::RPCSendRawTransaction(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSendRawTransactionParam>(param);

    vector<unsigned char> txData = ParseHexString(spParam->strTxdata);
    CBufStream ss;
    ss.Write((char*)&txData[0], txData.size());
    CTransaction rawTx;
    try
    {
        ss >> rawTx;
    }
    catch (const std::exception& e)
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "Signed offline raw tx decode failed");
    }

    Errno err = pService->SendOfflineSignedTransaction(rawTx);
    if (err != OK)
    {
        throw CRPCException(RPC_TRANSACTION_REJECTED, string("Tx rejected : ")
                                                          + ErrorString(err));
    }

    return MakeCSendRawTransactionResultPtr(rawTx.GetHash().GetHex());
}

/* Util */
CRPCResultPtr CRPCMod::RPCVerifyMessage(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CVerifyMessageParam>(param);

    //verifymessage <"pubkey"> <"message"> <"sig">
    crypto::CPubKey pubkey;
    if (pubkey.SetHex(spParam->strPubkey) != spParam->strPubkey.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }

    string strMessage = spParam->strMessage;

    if (spParam->strSig.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid sig");
    }
    vector<unsigned char> vchSig = ParseHexString(spParam->strSig);
    if (vchSig.size() == 0)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid sig");
    }
    if (spParam->strAddr.IsValid())
    {
        CAddress addr(spParam->strMessage);
        std::string ss = addr.ToString();
        if (addr.IsNull() || addr.IsPubKey())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address parameters");
        }
        return MakeCVerifyMessageResultPtr(
            pubkey.Verify(addr.GetTemplateId(), vchSig));
    }
    else
    {
        uint256 hashStr;
        if (spParam->fHasprefix)
        {
            CBufStream ss;
            const string strMessageMagic = "Ibrio Signed Message:\n";
            ss << strMessageMagic;
            ss << strMessage;
            hashStr = crypto::CryptoHash(ss.GetData(), ss.GetSize());
        }
        else
        {
            hashStr = crypto::CryptoHash(strMessage.data(), strMessage.size());
        }
        return MakeCVerifyMessageResultPtr(pubkey.Verify(hashStr, vchSig));
    }
}

CRPCResultPtr CRPCMod::RPCMakeKeyPair(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeKeyPairParam>(param);

    crypto::CCryptoKey key;
    crypto::CryptoMakeNewKey(key);

    auto spResult = MakeCMakeKeyPairResultPtr();
    spResult->strPrivkey = key.secret.GetHex();
    spResult->strPubkey = key.pubkey.GetHex();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCGetPubKey(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetPubkeyParam>(param);
    crypto::CPubKey pubkey;
    {
        CAddress address(spParam->strPrivkeyaddress);
        if (!address.IsNull())
        {
            if (!address.GetPubKey(pubkey))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey address");
            }
            return MakeCGetPubkeyResultPtr(pubkey.ToString());
        }
    }
    {
        uint256 nPriv;
        if (nPriv.SetHex(spParam->strPrivkeyaddress) == spParam->strPrivkeyaddress.size())
        {
            crypto::CKey key;
            if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
            {
                throw CRPCException(RPC_INVALID_PARAMETER, "Get pubkey by privkey error");
            }
            return MakeCGetPubkeyResultPtr(key.GetPubKey().ToString());
        }
    }

    throw CRPCException(RPC_INVALID_PARAMETER, "Invalid address or privkey");
}

CRPCResultPtr CRPCMod::RPCGetPubKeyAddress(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetPubkeyAddressParam>(param);
    crypto::CPubKey pubkey;
    if (pubkey.SetHex(spParam->strPubkey) != spParam->strPubkey.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid pubkey");
    }
    CDestination dest(pubkey);
    return MakeCGetPubkeyAddressResultPtr(CAddress(dest).ToString());
}

CRPCResultPtr CRPCMod::RPCGetTemplateAddress(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTemplateAddressParam>(param);
    CTemplateId tid;
    if (tid.SetHex(spParam->strTid) != spParam->strTid.size())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid tid");
    }

    CDestination dest(tid);

    return MakeCGetTemplateAddressResultPtr(CAddress(dest).ToString());
}

CRPCResultPtr CRPCMod::RPCMakeTemplate(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeTemplateParam>(param);
    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(spParam->data, CAddress());
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid parameters,failed to make template");
    }

    auto spResult = MakeCMakeTemplateResultPtr();
    vector<unsigned char> vchTemplate = ptr->Export();
    spResult->strHex = ToHexString(vchTemplate);
    spResult->strAddress = CAddress(ptr->GetTemplateId()).ToString();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCDecodeTransaction(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CDecodeTransactionParam>(param);
    vector<unsigned char> txData(ParseHexString(spParam->strTxdata));
    CBufStream ss;
    ss.Write((char*)&txData[0], txData.size());
    CTransaction rawTx;
    try
    {
        ss >> rawTx;
    }
    catch (const std::exception& e)
    {
        throw CRPCException(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    uint256 hashFork = rawTx.hashAnchor;
    /*int nHeight;
    if (!pService->GetBlockLocation(rawTx.hashAnchor, hashFork, nHeight))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Unknown anchor block");
    }*/

    return MakeCDecodeTransactionResultPtr(TxToJSON(rawTx.GetHash(), rawTx, hashFork, uint256(), -1, string()));
}

CRPCResultPtr CRPCMod::RPCGetTxFee(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetTransactionFeeParam>(param);
    int64 nTxFee = CalcMinTxFee(ParseHexString(spParam->strHexdata).size(), MIN_TX_FEE);
    auto spResult = MakeCGetTransactionFeeResultPtr();
    spResult->dTxfee = ValueFromToken(nTxFee);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCMakeSha256(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CMakeSha256Param>(param);
    vector<unsigned char> vData;
    if (spParam->strHexdata.IsValid())
    {
        vData = ParseHexString(spParam->strHexdata);
    }
    else
    {
        uint256 u;
        crypto::CryptoGetRand256(u);
        vData.assign(u.begin(), u.end());
    }

    uint256 hash = crypto::CryptoSHA256(&(vData[0]), vData.size());

    auto spResult = MakeCMakeSha256ResultPtr();
    spResult->strHexdata = ToHexString(vData);
    spResult->strSha256 = hash.GetHex();
    return spResult;
}

CRPCResultPtr CRPCMod::RPCAesEncrypt(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAesEncryptParam>(param);

    CAddress addressLocal(spParam->strLocaladdress);
    if (addressLocal.IsNull() || !addressLocal.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid local address");
    }

    CAddress addressRemote(spParam->strRemoteaddress);
    if (addressRemote.IsNull() || !addressRemote.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid remote address");
    }

    crypto::CPubKey pubkeyLocal;
    addressLocal.GetPubKey(pubkeyLocal);

    crypto::CPubKey pubkeyRemote;
    addressRemote.GetPubKey(pubkeyRemote);

    vector<uint8> vMessage = ParseHexString(spParam->strMessage);
    if (vMessage.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid message");
    }

    vector<uint8> vCiphertext;
    if (!pService->AesEncrypt(pubkeyLocal, pubkeyRemote, vMessage, vCiphertext))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Encrypt fail");
    }

    auto spResult = MakeCAesEncryptResultPtr();
    spResult->strResult = ToHexString(vCiphertext);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCAesDecrypt(rpc::CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CAesDecryptParam>(param);

    CAddress addressLocal(spParam->strLocaladdress);
    if (addressLocal.IsNull() || !addressLocal.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid local address");
    }

    CAddress addressRemote(spParam->strRemoteaddress);
    if (addressRemote.IsNull() || !addressRemote.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid remote address");
    }

    crypto::CPubKey pubkeyLocal;
    addressLocal.GetPubKey(pubkeyLocal);

    crypto::CPubKey pubkeyRemote;
    addressRemote.GetPubKey(pubkeyRemote);

    vector<uint8> vCiphertext = ParseHexString(spParam->strCiphertext);
    if (vCiphertext.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid ciphertext");
    }

    vector<uint8> vMessage;
    if (!pService->AesDecrypt(pubkeyLocal, pubkeyRemote, vCiphertext, vMessage))
    {
        throw CRPCException(RPC_WALLET_ERROR, "Decrypt fail");
    }

    auto spResult = MakeCAesDecryptResultPtr();
    spResult->strResult = ToHexString(vMessage);
    return spResult;
}

CRPCResultPtr CRPCMod::RPCListUnspent(CRPCParamPtr param)
{
    auto lmdImport = [](const string& pathFile, vector<CAddress>& addresses) -> bool
    {
        ifstream inFile(pathFile);

        if (!inFile)
        {
            return false;
        }

        // iterate addresses from input file
        const uint32 MAX_LISTUNSPENT_INPUT = 10000;
        uint32 nCount = 1;
        string strAddr;
        while (getline(inFile, strAddr) && nCount <= MAX_LISTUNSPENT_INPUT)
        {
            boost::trim(strAddr);
            if (strAddr.size() != CAddress::ADDRESS_LEN)
            {
                continue;
            }

            CAddress addr(strAddr);
            if (!addr.IsNull())
            {
                addresses.emplace_back(addr);
                ++nCount;
            }
        }

        auto last = unique(addresses.begin(), addresses.end());
        addresses.erase(last, addresses.end());

        return true;
    };

    auto spParam = CastParamPtr<CListUnspentParam>(param);

    uint256 fork;
    if (!GetForkHashOfDef(spParam->strFork, fork))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
    }

    vector<CAddress> vAddr;

    CAddress addr(spParam->strAddress);
    if (!addr.IsNull())
    {
        vAddr.emplace_back(addr);
    }

    if (spParam->strFile.IsValid() && !lmdImport(spParam->strFile, vAddr))
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid import file");
    }

    if (vAddr.empty())
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Available address as argument should be provided.");
    }

    std::map<CDestination, std::vector<CTxUnspent>> mapDest;
    for (const auto& i : vAddr)
    {
        mapDest.emplace(std::make_pair(static_cast<CDestination>(i), std::vector<CTxUnspent>()));
    }

    int64 nAmount = 0;
    if (!IsDoubleNonPositiveNumber(spParam->dAmount))
    {
        nAmount = AmountFromValue(spParam->dAmount);
    }
    for (int i = 0; i < vAddr.size(); i++)
    {
        string strErr;
        Errno err = pService->ListForkAddressUnspent(fork, static_cast<CDestination&>(vAddr[i]), spParam->nMax,
                                                     nAmount, mapDest[static_cast<CDestination>(vAddr[i])], strErr);
        if (err != OK)
        {
            if (err == ERR_WALLET_INSUFFICIENT_FUNDS)
            {
                throw CRPCException(RPC_WALLET_INSUFFICIENT_FUNDS, strErr);
            }
            else
            {
                throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, strErr);
            }
        }
    }

    auto spResult = MakeCListUnspentResultPtr();
    double dTotal = 0.0f;
    for (auto& iAddr : mapDest)
    {
        CAddress dest(iAddr.first);

        typename CListUnspentResult::CAddresses a;
        a.strAddress = dest.ToString();

        double dSum = 0.0f;
        for (const auto& unspent : iAddr.second)
        {
            CUnspentData data = UnspentToJSON(unspent);
            a.vecUnspents.push_back(data);
            dSum += data.dAmount;
        }

        a.dSum = dSum;

        spResult->vecAddresses.push_back(a);

        dTotal += dSum;
    }

    spResult->dTotal = dTotal;

    return spResult;
}

CRPCResultPtr CRPCMod::RPCReverseHex(rpc::CRPCParamPtr param)
{
    // reversehex <"hex">
    auto spParam = CastParamPtr<CReverseHexParam>(param);

    string strHex = spParam->strHex;
    if (strHex.empty() || (strHex.size() % 2 != 0))
    {
        throw CRPCException(RPC_INVALID_PARAMS, "hex string size is not even");
    }

    regex r(R"([^0-9a-fA-F]+)");
    smatch sm;
    if (regex_search(strHex, sm, r))
    {
        throw CRPCException(RPC_INVALID_PARAMS, string("invalid hex string: ") + sm.str());
    }

    for (auto itBegin = strHex.begin(), itEnd = strHex.end() - 2; itBegin < itEnd; itBegin += 2, itEnd -= 2)
    {
        swap_ranges(itBegin, itBegin + 2, itEnd);
    }

    return MakeCReverseHexResultPtr(strHex);
}

CRPCResultPtr CRPCMod::RPCActivateSign(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CActivateSignParam>(param);

    CAddress destInviter(spParam->strInviter);
    if (destInviter.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMS, "Invalid inviter");
    }

    CAddress destOwner(spParam->strOwner);
    if (destOwner.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMS, "Invalid owner");
    }

    auto ptr = CTemplate::CreateTemplatePtr(new CTemplateActivate(destInviter, destOwner));
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_PARAMS, "Invalid inviter or owner");
    }

    uint256 hashSign = pCoreProtocol->CreateActivateSignHash(pCoreProtocol->GetGenesisBlockHash(), CDestination(ptr->GetTemplateId()));
    std::vector<uint8> vSignData;
    if (!pService->SignSignature(destOwner.GetPubKey(), hashSign, vSignData))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Sign fail");
    }

    return MakeCActivateSignResultPtr(ToHexString(vSignData));
}

CRPCResultPtr CRPCMod::RPCGetActivateStatus(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CGetActivateStatusParam>(param);

    CAddress dest(spParam->strAddress);
    if (dest.IsNull())
    {
        throw CRPCException(RPC_INVALID_PARAMS, "Invalid address");
    }

    auto spResult = MakeCGetActivateStatusResultPtr();

    int nActivateHeight = pService->GetLastBlockActivateHeight(dest);
    if (nActivateHeight < 0)
    {
        spResult->fActivate = false;
        spResult->nHeight = 0;
    }
    else
    {
        spResult->fActivate = true;
        spResult->nHeight = nActivateHeight;
    }
    return spResult;
}

// /* Mint */
CRPCResultPtr CRPCMod::RPCGetWork(CRPCParamPtr param)
{
    //getwork <"spent"> <"privkey"> ("prev")
    auto spParam = CastParamPtr<CGetWorkParam>(param);

    CAddress addrSpent(spParam->strSpent);
    uint256 nPriv(spParam->strPrivkey);
    if (addrSpent.IsNull() || !addrSpent.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid spent address");
    }
    crypto::CKey key;
    if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    }
    crypto::CPubKey pubkeySpent;
    if (addrSpent.GetPubKey(pubkeySpent) && pubkeySpent == key.GetPubKey())
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid spent address or private key");
    }
    CTemplateMintPtr ptr = CTemplateMint::CreateTemplatePtr(new CTemplateProof(key.GetPubKey(), static_cast<CDestination&>(addrSpent)));
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid mint template");
    }

    auto spResult = MakeCGetWorkResultPtr();

    vector<unsigned char> vchWorkData;
    int nPrevBlockHeight;
    uint256 hashPrev;
    uint32 nPrevTime;
    int nAlgo, nBits;
    if (!pService->GetWork(vchWorkData, nPrevBlockHeight, hashPrev, nPrevTime, nAlgo, nBits, ptr))
    {
        spResult->fResult = false;
        return spResult;
    }

    spResult->fResult = true;

    spResult->work.nPrevblockheight = nPrevBlockHeight;
    spResult->work.strPrevblockhash = hashPrev.GetHex();
    spResult->work.nPrevblocktime = nPrevTime;
    spResult->work.nAlgo = nAlgo;
    spResult->work.nBits = nBits;
    spResult->work.strData = ToHexString(vchWorkData);

    return spResult;
}

CRPCResultPtr CRPCMod::RPCSubmitWork(CRPCParamPtr param)
{
    auto spParam = CastParamPtr<CSubmitWorkParam>(param);
    vector<unsigned char> vchWorkData(ParseHexString(spParam->strData));
    CAddress addrSpent(spParam->strSpent);
    uint256 nPriv(spParam->strPrivkey);
    if (addrSpent.IsNull() || !addrSpent.IsPubKey())
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid spent address");
    }
    crypto::CKey key;
    if (!key.SetSecret(crypto::CCryptoKeyData(nPriv.begin(), nPriv.end())))
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    }

    CTemplateMintPtr ptr = CTemplateMint::CreateTemplatePtr(new CTemplateProof(key.GetPubKey(), static_cast<CDestination&>(addrSpent)));
    if (ptr == nullptr)
    {
        throw CRPCException(RPC_INVALID_ADDRESS_OR_KEY, "Invalid mint template");
    }
    uint256 hashBlock;
    Errno err = pService->SubmitWork(vchWorkData, ptr, key, hashBlock);
    if (err != OK)
    {
        throw CRPCException(RPC_INVALID_PARAMETER, string("Block rejected : ") + ErrorString(err));
    }

    return MakeCSubmitWorkResultPtr(hashBlock.GetHex());
}

CRPCResultPtr CRPCMod::RPCQueryStat(rpc::CRPCParamPtr param)
{
    enum
    {
        TYPE_NON,
        TYPE_MAKER,
        TYPE_P2PSYN
    } eType
        = TYPE_NON;
    uint32 nDefQueryCount = 20;
    uint256 hashFork;
    uint32 nBeginTimeValue = ((GetTime() - 60 * nDefQueryCount) % (24 * 60 * 60)) / 60;
    uint32 nGetCount = nDefQueryCount;
    bool fSetBegin = false;

    auto spParam = CastParamPtr<CQueryStatParam>(param);
    if (spParam->strType.empty())
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid type: is empty");
    }
    if (spParam->strType == "maker")
    {
        eType = TYPE_MAKER;
    }
    else if (spParam->strType == "p2psyn")
    {
        eType = TYPE_P2PSYN;
    }
    else
    {
        throw CRPCException(RPC_INVALID_PARAMETER, "Invalid type");
    }
    if (!spParam->strFork.empty())
    {
        if (hashFork.SetHex(spParam->strFork) != spParam->strFork.size())
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid fork");
        }
    }
    if (!spParam->strBegin.empty() && spParam->strBegin.size() <= 8)
    {
        //HH:MM:SS
        std::string strTempTime = spParam->strBegin;
        std::size_t found_hour = strTempTime.find(":");
        if (found_hour != std::string::npos && found_hour > 0)
        {
            std::size_t found_min = strTempTime.find(":", found_hour + 1);
            if (found_min != std::string::npos && found_min > found_hour + 1)
            {
                int hour = std::stoi(strTempTime.substr(0, found_hour));
                int minute = std::stoi(strTempTime.substr(found_hour + 1, found_min - (found_hour + 1)));
                if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59)
                {
                    nBeginTimeValue = hour * 60 + minute;
                    int64 nTimeOffset = (GetTime() - GetLocalTimeSeconds()) / 60;
                    nTimeOffset += nBeginTimeValue;
                    if (nTimeOffset >= 0)
                    {
                        nBeginTimeValue = nTimeOffset % (24 * 60);
                    }
                    else
                    {
                        nBeginTimeValue = nTimeOffset + (24 * 60);
                    }
                    fSetBegin = true;
                }
            }
        }
    }
    if (spParam->nCount.IsValid())
    {
        nGetCount = GetUint(spParam->nCount, nDefQueryCount);
        if (nGetCount == 0)
        {
            throw CRPCException(RPC_INVALID_PARAMETER, "Invalid count");
        }
        if (nGetCount > 24 * 60)
        {
            nGetCount = 24 * 60;
        }
    }
    if (!fSetBegin && nGetCount != nDefQueryCount)
    {
        nBeginTimeValue = ((GetTime() - 60 * nGetCount) % (24 * 60 * 60)) / 60;
    }
    else
    {
        uint32 nTempCurTimeValue = (GetTime() % (24 * 60 * 60)) / 60;
        if (nTempCurTimeValue == nBeginTimeValue)
        {
            nGetCount = 0;
        }
        else
        {
            uint32 nTempCount = 0;
            if (nTempCurTimeValue > nBeginTimeValue)
            {
                nTempCount = nTempCurTimeValue - nBeginTimeValue;
            }
            else
            {
                nTempCount = (24 * 60) - (nBeginTimeValue - nTempCurTimeValue);
            }
            if (nGetCount > nTempCount)
            {
                nGetCount = nTempCount;
            }
        }
    }

    switch (eType)
    {
    case TYPE_MAKER:
    {
        std::vector<CStatItemBlockMaker> vStatData;
        if (nGetCount > 0)
        {
            if (!pDataStat->GetBlockMakerStatData(hashFork, nBeginTimeValue, nGetCount, vStatData))
            {
                throw CRPCException(RPC_INTERNAL_ERROR, "query error");
            }
        }

        int nTimeWidth = 8 + 2;                                 //hh:mm:ss + two spaces
        int nPowBlocksWidth = string("powblocks").size() + 2;   //+ two spaces
        int nDposBlocksWidth = string("dposblocks").size() + 2; //+ two spaces
        int nTxTPSWidth = string("tps").size() + 2;
        for (const CStatItemBlockMaker& item : vStatData)
        {
            int nTempValue;
            nTempValue = to_string(item.nPOWBlockCount).size() + 2; //+ two spaces (not decimal point)
            if (nTempValue > nPowBlocksWidth)
            {
                nPowBlocksWidth = nTempValue;
            }
            nTempValue = to_string(item.nDPOSBlockCount).size() + 2; //+ two spaces (not decimal point)
            if (nTempValue > nDposBlocksWidth)
            {
                nDposBlocksWidth = nTempValue;
            }
            nTempValue = to_string(item.nTxTPS).size() + 3; //+ one decimal point + two spaces
            if (nTempValue > nTxTPSWidth)
            {
                nTxTPSWidth = nTempValue;
            }
        }

        int64 nTimeOffset = GetLocalTimeSeconds() - GetTime();

        string strResult = "";
        strResult += GetWidthString("time", nTimeWidth);
        strResult += GetWidthString("powblocks", nPowBlocksWidth);
        strResult += GetWidthString("dposblocks", nDposBlocksWidth);
        strResult += GetWidthString("tps", nTxTPSWidth);
        strResult += string("\r\n");
        for (const CStatItemBlockMaker& item : vStatData)
        {
            int nLocalTimeValue = item.nTimeValue * 60 + nTimeOffset;
            if (nLocalTimeValue >= 0)
            {
                nLocalTimeValue %= (24 * 3600);
            }
            else
            {
                nLocalTimeValue += (24 * 3600);
            }
            char sTimeBuf[128] = { 0 };
            sprintf(sTimeBuf, "%2.2d:%2.2d:59", nLocalTimeValue / 3600, nLocalTimeValue % 3600 / 60);
            strResult += GetWidthString(sTimeBuf, nTimeWidth);
            strResult += GetWidthString(to_string(item.nPOWBlockCount), nPowBlocksWidth);
            strResult += GetWidthString(to_string(item.nDPOSBlockCount), nDposBlocksWidth);
            strResult += GetWidthString(item.nTxTPS, nTxTPSWidth);
            strResult += string("\r\n");
        }
        return MakeCQueryStatResultPtr(strResult);
    }
    case TYPE_P2PSYN:
    {
        std::vector<CStatItemP2pSyn> vStatData;
        if (nGetCount > 0)
        {
            if (!pDataStat->GetP2pSynStatData(hashFork, nBeginTimeValue, nGetCount, vStatData))
            {
                throw CRPCException(RPC_INTERNAL_ERROR, "query error");
            }
        }

        int nTimeWidth = 8 + 2;                                   //hh:mm:ss + two spaces
        int nRecvBlockTPSWidth = string("recvblocks").size() + 2; //+ two spaces
        int nRecvTxTPSWidth = string("recvtps").size() + 2;
        int nSendBlockTPSWidth = string("sendblocks").size() + 2;
        int nSendTxTPSWidth = string("sendtps").size() + 2;
        for (const CStatItemP2pSyn& item : vStatData)
        {
            int nTempValue;
            nTempValue = to_string(item.nRecvBlockCount).size() + 2; //+ two spaces (not decimal point)
            if (nTempValue > nRecvBlockTPSWidth)
            {
                nRecvBlockTPSWidth = nTempValue;
            }
            nTempValue = to_string(item.nSynRecvTxTPS).size() + 3; //+ one decimal point + two spaces
            if (nTempValue > nRecvTxTPSWidth)
            {
                nRecvTxTPSWidth = nTempValue;
            }
            nTempValue = to_string(item.nSendBlockCount).size() + 2; //+ two spaces (not decimal point)
            if (nTempValue > nSendBlockTPSWidth)
            {
                nSendBlockTPSWidth = nTempValue;
            }
            nTempValue = to_string(item.nSynSendTxTPS).size() + 3; //+ one decimal point + two spaces
            if (nTempValue > nSendTxTPSWidth)
            {
                nSendTxTPSWidth = nTempValue;
            }
        }

        int64 nTimeOffset = GetLocalTimeSeconds() - GetTime();

        string strResult;
        strResult += GetWidthString("time", nTimeWidth);
        strResult += GetWidthString("recvblocks", nRecvBlockTPSWidth);
        strResult += GetWidthString("recvtps", nRecvTxTPSWidth);
        strResult += GetWidthString("sendblocks", nSendBlockTPSWidth);
        strResult += GetWidthString("sendtps", nSendTxTPSWidth);
        strResult += string("\r\n");
        for (const CStatItemP2pSyn& item : vStatData)
        {
            int nLocalTimeValue = item.nTimeValue * 60 + nTimeOffset;
            if (nLocalTimeValue >= 0)
            {
                nLocalTimeValue %= (24 * 3600);
            }
            else
            {
                nLocalTimeValue += (24 * 3600);
            }
            char sTimeBuf[128] = { 0 };
            sprintf(sTimeBuf, "%2.2d:%2.2d:59", nLocalTimeValue / 3600, nLocalTimeValue % 3600 / 60);
            strResult += GetWidthString(sTimeBuf, nTimeWidth);
            strResult += GetWidthString(to_string(item.nRecvBlockCount), nRecvBlockTPSWidth);
            strResult += GetWidthString(item.nSynRecvTxTPS, nRecvTxTPSWidth);
            strResult += GetWidthString(to_string(item.nSendBlockCount), nSendBlockTPSWidth);
            strResult += GetWidthString(item.nSynSendTxTPS, nSendTxTPSWidth);
            strResult += string("\r\n");
        }
        return MakeCQueryStatResultPtr(strResult);
    }
    default:
        break;
    }

    return MakeCQueryStatResultPtr(string("error"));
}

} // namespace ibrio
