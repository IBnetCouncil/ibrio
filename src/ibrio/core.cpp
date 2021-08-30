// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"

#include "address.h"
#include "crypto.h"
#include "param.h"
#include "template/activate.h"
#include "template/delegate.h"
#include "template/dexmatch.h"
#include "template/dexorder.h"
#include "template/exchange.h"
#include "template/fork.h"
#include "template/mint.h"
#include "template/vote.h"
#include "wallet.h"

using namespace std;
using namespace xengine;

#define DEBUG(err, ...) Debug((err), __FUNCTION__, __VA_ARGS__)

// DeFi fork blacklist
static const std::map<uint256, std::map<int, std::set<CDestination>>> mapDeFiBlacklist = {
    {
        uint256(),
        {
            {
                0,
                {
                    ibrio::CAddress("100000000000000000000000000000000000000000000000000000000"),
                },
            },
        },
    },
};

namespace ibrio
{
///////////////////////////////
// CCoreProtocol

CCoreProtocol::CCoreProtocol()
{
    pBlockChain = nullptr;
    pForkManager = nullptr;
}

CCoreProtocol::~CCoreProtocol()
{
}

bool CCoreProtocol::HandleInitialize()
{
    InitializeGenesisBlock();
    if (!GetObject("blockchain", pBlockChain))
    {
        return false;
    }
    if (!GetObject("forkmanager", pForkManager))
    {
        return false;
    }
    return true;
}

Errno CCoreProtocol::Debug(const Errno& err, const char* pszFunc, const char* pszFormat, ...)
{
    string strFormat(pszFunc);
    strFormat += string(", ") + string(ErrorString(err)) + string(" : ") + string(pszFormat);
    va_list ap;
    va_start(ap, pszFormat);
    VDebug(strFormat.c_str(), ap);
    va_end(ap);
    return err;
}

void CCoreProtocol::InitializeGenesisBlock()
{
    CBlock block;
    GetGenesisBlock(block);
    hashGenesisBlock = block.GetHash();
}

const uint256& CCoreProtocol::GetGenesisBlockHash()
{
    return hashGenesisBlock;
}

/*
Address: 1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws
PubKey : ac9a2f4b438a270fcdfe33305db1da885dc53de8e4299bbba765c4207338c310
Secret : 9ae89671cc1a74e9e404a16982ae48d21c56d4ad8278bc9755235a68fc841271
*/
void CCoreProtocol::GetGenesisBlock(CBlock& block)
{
    const CDestination destOwner = CDestination(ibrio::crypto::CPubKey(uint256("ac9a2f4b438a270fcdfe33305db1da885dc53de8e4299bbba765c4207338c310")));

    block.SetNull();

    block.nVersion = 1;
    block.nType = CBlock::BLOCK_GENESIS;
    block.nTimeStamp = 1621322325;
    block.hashPrev = 0;

    CTransaction& tx = block.txMint;
    tx.nType = CTransaction::TX_GENESIS;
    tx.nTimeStamp = block.nTimeStamp;
    tx.sendTo = destOwner;
    tx.nAmount = NPSP_TOKEN_INIT; // initial number of token

    CProfile profile;
    profile.strName = "IBR Core";
    profile.strSymbol = "IBR";
    profile.destOwner = destOwner;
    profile.nAmount = tx.nAmount;
    profile.nMintReward = 0;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nHalveCycle = 0;
    profile.SetFlag(true, false, false);

    profile.Save(block.vchProof);
}

Errno CCoreProtocol::ValidateTransaction(const CTransaction& tx, int nHeight)
{
    // Basic checks that don't depend on any context
    if (tx.nType == CTransaction::TX_TOKEN
        && (tx.sendTo.IsPubKey()
            || (tx.sendTo.IsTemplate()
                && (tx.sendTo.GetTemplateId().GetType() == TEMPLATE_WEIGHTED || tx.sendTo.GetTemplateId().GetType() == TEMPLATE_MULTISIG)))
        && !tx.vchData.empty())
    {
        if (tx.vchData.size() < 21)
        { //vchData must contain 3 fields of UUID, timestamp, szDescription at least
            return DEBUG(ERR_TRANSACTION_INVALID, "tx vchData is less than 21 bytes.");
        }
        //check description field
        uint16 nPos = 20;
        uint8 szDesc = tx.vchData[nPos];
        if (szDesc > 0)
        {
            if ((nPos + 1 + szDesc) > tx.vchData.size())
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "tx vchData is overflow.");
            }
            std::string strDescEncodedBase64(tx.vchData.begin() + nPos + 1, tx.vchData.begin() + nPos + 1 + szDesc);
            xengine::CHttpUtil util;
            std::string strDescDecodedBase64;
            if (!util.Base64Decode(strDescEncodedBase64, strDescDecodedBase64))
            {
                return DEBUG(ERR_TRANSACTION_INVALID, "tx vchData description base64 is not available.");
            }
        }
    }
    if (tx.nType == CTransaction::TX_DEFI_REWARD && (tx.vchData.size() > 48))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "DeFi reward tx data length is not 48");
    }
    if (!tx.vchData.empty() && (tx.nType == CTransaction::TX_WORK || tx.nType == CTransaction::TX_STAKE))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "tx data is not empty, tx type: %s", tx.GetTypeString().c_str());
    }
    if (tx.nType == CTransaction::TX_INVEST_REWARD)
    {
        vector<uint8> vTempData;
        xengine::CODataStream ds(vTempData);
        uint32 n = 0;
        ds << n;
        if (tx.vchData.size() != vTempData.size())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "tx data is not empty, tx type: %s", tx.GetTypeString().c_str());
        }
    }
    if (tx.vInput.empty()
        && (tx.nType != CTransaction::TX_GENESIS
            && tx.nType != CTransaction::TX_WORK
            && tx.nType != CTransaction::TX_STAKE
            && tx.nType != CTransaction::TX_DEFI_REWARD
            && tx.nType != CTransaction::TX_INVEST_REWARD))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "tx vin is empty");
    }
    if (!tx.vInput.empty()
        && (tx.nType == CTransaction::TX_GENESIS
            || tx.nType == CTransaction::TX_WORK
            || tx.nType == CTransaction::TX_STAKE
            || tx.nType == CTransaction::TX_DEFI_REWARD
            || tx.nType == CTransaction::TX_INVEST_REWARD))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "tx vin is not empty for genesis or work tx");
    }
    if (!tx.vchSig.empty() && tx.IsRewardTx())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid signature");
    }
    if (tx.sendTo.IsNull())
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "send to null address");
    }
    if (!MoneyRange(tx.nAmount))
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "amount overflow %ld", tx.nAmount);
    }

    if (!MoneyRange(tx.nTxFee))
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "txfee invalid %ld", tx.nTxFee);
    }
    if (tx.nTxFee != 0
        && (tx.nType != CTransaction::TX_TOKEN
            && tx.nType != CTransaction::TX_DEFI_REWARD
            && tx.nType != CTransaction::TX_DEFI_RELATION
            && tx.nType != CTransaction::TX_DEFI_MINT_HEIGHT
            && tx.nType != CTransaction::TX_INVEST_REWARD))
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "txfee invalid %ld", tx.nTxFee);
    }
    if ((tx.nType == CTransaction::TX_TOKEN
         || tx.nType == CTransaction::TX_DEFI_RELATION
         || tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT)
        && !(tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_ACTIVATE)
        && tx.nTxFee < CalcMinTxFee(tx.vchData.size(), MIN_TX_FEE))
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "txfee invalid %ld", tx.nTxFee);
    }
    if (tx.nType == CTransaction::TX_DEFI_REWARD && tx.nTxFee != MIN_TX_FEE)
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "txfee invalid %ld", tx.nTxFee);
    }
    if (tx.nType == CTransaction::TX_INVEST_REWARD && tx.nTxFee != 0)
    {
        return DEBUG(ERR_TRANSACTION_OUTPUT_INVALID, "txfee invalid %ld", tx.nTxFee);
    }

    set<CTxOutPoint> setInOutPoints;
    for (const CTxIn& txin : tx.vInput)
    {
        if (txin.prevout.IsNull() || txin.prevout.n > 1)
        {
            return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "prevout invalid");
        }
        if (!setInOutPoints.insert(txin.prevout).second)
        {
            return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "duplicate inputs");
        }
    }

    if (GetSerializeSize(tx) > MAX_TX_SIZE)
    {
        return DEBUG(ERR_TRANSACTION_OVERSIZE, "%u", GetSerializeSize(tx));
    }

    return OK;
}

Errno CCoreProtocol::ValidateBlock(const CBlock& block)
{
    // These are checks that are independent of context
    // Only allow CBlock::BLOCK_PRIMARY type in v1.0.0
    /*if (block.nType != CBlock::BLOCK_PRIMARY)
    {
        return DEBUG(ERR_BLOCK_TYPE_INVALID, "Block type error");
    }*/
    // Check timestamp
    if (block.GetBlockTime() > GetNetTime() + MAX_CLOCK_DRIFT)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "%ld", block.GetBlockTime());
    }

    // validate vacant block
    if (block.nType == CBlock::BLOCK_VACANT)
    {
        /*if (!IsRefVacantHeight(block.GetBlockHeight()))
        {
            return ValidateVacantBlock(block);
        }*/
        if (block.txMint.nAmount != 0 || block.txMint.nTxFee != 0 || block.txMint.nType != CTransaction::TX_STAKE
            || block.txMint.nTimeStamp == 0 || block.txMint.sendTo.IsNull())
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid mint tx, nAmount: %lu, nTxFee: %lu, nType: %d, nTimeStamp: %d, sendTo: %s",
                         block.txMint.nAmount, block.txMint.nTxFee, block.txMint.nType, block.txMint.nTimeStamp,
                         (block.txMint.sendTo.IsNull() ? "null" : CAddress(block.txMint.sendTo).ToString().c_str()));
        }
        if (block.hashMerkle != 0 || !block.vtx.empty())
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "vacant block vtx is not empty");
        }
    }

    // Validate mint tx
    if (!block.txMint.IsMintTx() || ValidateTransaction(block.txMint, block.GetBlockHeight()) != OK)
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid mint tx, tx type: %d", block.txMint.nType);
    }

    size_t nBlockSize = GetSerializeSize(block);
    if (nBlockSize > MAX_BLOCK_SIZE)
    {
        return DEBUG(ERR_BLOCK_OVERSIZE, "size overflow size=%u vtx=%u", nBlockSize, block.vtx.size());
    }

    if (block.nType == CBlock::BLOCK_ORIGIN && !block.vtx.empty())
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "origin block vtx is not empty");
    }

    vector<uint256> vMerkleTree;
    if (block.hashMerkle != block.BuildMerkleTree(vMerkleTree))
    {
        return DEBUG(ERR_BLOCK_TXHASH_MISMATCH, "tx merkeroot mismatched");
    }

    set<uint256> setTx;
    setTx.insert(vMerkleTree.begin(), vMerkleTree.begin() + block.vtx.size());
    if (setTx.size() != block.vtx.size())
    {
        return DEBUG(ERR_BLOCK_DUPLICATED_TRANSACTION, "duplicate tx");
    }

    for (const CTransaction& tx : block.vtx)
    {
        if (tx.IsMintTx() || ValidateTransaction(tx, block.GetBlockHeight()) != OK)
        {
            return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "invalid tx %s", tx.GetHash().GetHex().c_str());
        }
    }

    if (!CheckBlockSignature(block))
    {
        return DEBUG(ERR_BLOCK_SIGNATURE_INVALID, "Check block signature fail");
    }
    return OK;
}

Errno CCoreProtocol::VerifyForkTx(const CTransaction& tx, const CDestination& destIn, const uint256& hashFork, const int nHeight)
{
#ifdef DISABLE_FORK_CREATETION
    return DEBUG(ERR_BLOCK_INVALID_FORK, "Disable fork creation");
#else
    if (hashFork != GetGenesisBlockHash())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork");
    }
    if (tx.sendTo == destIn)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "It is not allowed to change from self to self");
    }
    if (tx.vchData.empty())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid vchData");
    }
    if (tx.nAmount < CTemplateFork::CreatedCoin())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid nAmount");
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
            return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid block");
        }
        if (!profile.Load(block.vchProof))
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
        }
    }
    catch (...)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork vchData");
    }

    if (profile.IsNull())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
    }
    if (!MoneyRange(profile.nAmount))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork amount");
    }
    if (!RewardRange(profile.nMintReward))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork reward");
    }
    if (block.txMint.sendTo != profile.destOwner)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork sendTo");
    }

    if (ValidateBlock(block) != OK)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid block");
    }

    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(TEMPLATE_FORK, tx.vchSig);
    if (!ptr)
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid vchSig");
    }
    if (ptr->GetTemplateId() != tx.sendTo.GetTemplateId())
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid template id");
    }
    CDestination destRedeem;
    uint256 hashForkRedeem;
    boost::dynamic_pointer_cast<CLockedCoinTemplate>(ptr)->GetForkParam(destRedeem, hashForkRedeem);
    if (hashForkRedeem != block.GetHash())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid forkid");
    }
    return OK;
#endif
}

Errno CCoreProtocol::VerifyForkRedeem(const CTransaction& tx, const CDestination& destIn, const uint256& hashFork,
                                      const uint256& hashPrevBlock, const vector<uint8>& vchSubSig, const int64 nValueIn)
{
    if (hashFork != GetGenesisBlockHash())
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid fork");
    }
    if (tx.sendTo == destIn)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "It is not allowed to change from self to self");
    }
    CTemplatePtr ptr = CTemplate::CreateTemplatePtr(destIn.GetTemplateId().GetType(), vchSubSig);
    if (!ptr)
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid locked coin template destination");
    }
    CDestination destRedeemLocked;
    uint256 hashForkLocked;
    boost::dynamic_pointer_cast<CLockedCoinTemplate>(ptr)->GetForkParam(destRedeemLocked, hashForkLocked);
    int64 nLockedCoin = pForkManager->ForkLockedCoin(hashForkLocked, hashPrevBlock);
    if (nLockedCoin < 0)
    {
        nLockedCoin = 0;
    }
    // locked coin template: nValueIn >= tx.nAmount + tx.nTxFee + nLockedCoin
    if (nValueIn < tx.nAmount + tx.nTxFee + nLockedCoin)
    {
        return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "valuein is not enough to locked coin (%ld : %ld)",
                     nValueIn, tx.nAmount + tx.nTxFee + nLockedCoin);
    }
    return OK;
}

Errno CCoreProtocol::ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile)
{
    if (!forkProfile.Load(block.vchProof))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "load profile error");
    }
    if (forkProfile.IsNull())
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid profile");
    }
    if (!MoneyRange(forkProfile.nAmount))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork amount");
    }
    if (!RewardRange(forkProfile.nMintReward))
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork reward");
    }
    if (block.txMint.sendTo != forkProfile.destOwner)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "invalid fork sendTo");
    }
    if (parentProfile.IsPrivate())
    {
        if (!forkProfile.IsPrivate() || parentProfile.destOwner != forkProfile.destOwner)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "permission denied");
        }
    }
    // check defi param
    if (forkProfile.nForkType == FORK_TYPE_DEFI)
    {
        return DEBUG(ERR_BLOCK_INVALID_FORK, "Disable FORK creation of DEFI");
        /*if (forkProfile.nMintReward != 0)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi fork mint reward must be zero");
        }
        if (forkProfile.nHalveCycle != 0)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi fork mint halvecycle must be zero");
        }

        const CDeFiProfile& defi = forkProfile.defi;
        if ((defi.nMintHeight < -1) || (defi.nMintHeight > 0 && forkProfile.defi.nMintHeight < forkProfile.nJointHeight + 2))
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param mintheight should be -1 or 0 or larger than fork genesis block height");
        }
        if (defi.nMaxSupply >= 0 && !MoneyRange(defi.nMaxSupply))
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nMaxSupply is out of range");
        }
        if (defi.nRewardCycle <= 0 || defi.nRewardCycle > 100 * YEAR_HEIGHT)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nRewardCycle must be [1, %ld]", 100 * YEAR_HEIGHT);
        }
        if (defi.nSupplyCycle <= 0 || defi.nSupplyCycle > 100 * YEAR_HEIGHT)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nSupplyCycle must be [1, %ld]", 100 * YEAR_HEIGHT);
        }
        if (defi.nSupplyCycle % defi.nRewardCycle != 0)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nSupplyCycle must be divisible by nRewardCycle");
        }
        if (defi.nCoinbaseType == FIXED_DEFI_COINBASE_TYPE)
        {
            if (defi.nInitCoinbasePercent == 0 || defi.nInitCoinbasePercent > 10000)
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nInitCoinbasePercent must be [1, 10000]");
            }
            if (defi.nCoinbaseDecayPercent > 100)
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nCoinbaseDecayPercent must be [0, 100]");
            }
            if (defi.nDecayCycle < 0 || defi.nDecayCycle > 100 * YEAR_HEIGHT)
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nDecayCycle must be [0, %ld]", 100 * YEAR_HEIGHT);
            }
            if (defi.nDecayCycle % defi.nSupplyCycle != 0)
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nDecayCycle must be divisible by nSupplyCycle");
            }
        }
        else if (defi.nCoinbaseType == SPECIFIC_DEFI_COINBASE_TYPE)
        {
            if (defi.mapCoinbasePercent.size() == 0)
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param mapCoinbasePercent is empty");
            }
            for (auto it = defi.mapCoinbasePercent.begin(); it != defi.mapCoinbasePercent.end(); it++)
            {
                if (it->first <= 0)
                {
                    return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param key of mapCoinbasePercent must be larger than 0");
                }
                if (it->first % defi.nSupplyCycle != 0)
                {
                    return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param key of mapCoinbasePercent must be divisible by nSupplyCycle");
                }
                if (it->second == 0)
                {
                    return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param value of mapCoinbasePercent must be larger than 0");
                }
            }
        }
        else
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nCoinbaseType is out of range");
        }
        if (defi.nStakeRewardPercent > 100)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nStakeRewardPercent must be [0, 100]");
        }
        if (defi.nPromotionRewardPercent > 100)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nPromotionRewardPercent must be [0, 100]");
        }
        if (defi.nStakeRewardPercent + defi.nPromotionRewardPercent > 100)
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param (nStakeRewardPercent + nPromotionRewardPercent) must be [0, 100]");
        }
        if (!MoneyRange(defi.nStakeMinToken))
        {
            return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param nStakeMinToken is out of range");
        }
        for (auto& times : defi.mapPromotionTokenTimes)
        {
            if (times.first <= 0 || times.first > (MAX_MONEY / COIN))
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param key of mapPromotionTokenTimes should be (0, %ld]", (MAX_MONEY / COIN));
            }
            if (times.second == 0)
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param times of mappromotiontokentimes is equal 0");
            }
            // precision
            int64 nMaxPower = defi.nMaxSupply / COIN * times.second;
            if (nMaxPower < (defi.nMaxSupply / COIN))
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param times * maxsupply is overflow");
            }
            if (to_string(nMaxPower).size() > 14)
            {
                return DEBUG(ERR_BLOCK_INVALID_FORK, "DeFi param times * maxsupply is more than 15 digits. It will lose precision");
            }
        }*/
    }
    return OK;
}

Errno CCoreProtocol::VerifyProofOfWork(const CBlock& block, const CBlockIndex* pIndexPrev)
{
    if (block.vchProof.size() < CProofOfHashWorkCompact::PROOFHASHWORK_SIZE)
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "vchProof size error.");
    }

    uint32 nNextTimestamp = GetNextBlockTimeStamp(pIndexPrev->nMintType, pIndexPrev->GetBlockTime(), block.txMint.nType);
    if (block.GetBlockTime() < nNextTimestamp)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Verify proof work: Timestamp out of range 2, height: %d, block time: %d, next time: %d, prev minttype: 0x%x, prev time: %d, block: %s.",
                     block.GetBlockHeight(), block.GetBlockTime(), nNextTimestamp,
                     pIndexPrev->nMintType, pIndexPrev->GetBlockTime(), block.GetHash().GetHex().c_str());
    }

    CProofOfHashWorkCompact proof;
    proof.Load(block.vchProof);

    int nBits = 0;
    int64 nReward = 0;
    if (!GetProofOfWorkTarget(pIndexPrev, proof.nAlgo, nBits, nReward))
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "get target fail.");
    }

    if (nBits != proof.nBits || proof.nAlgo != CM_CRYPTONIGHT)
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "algo or bits error, nAlgo: %d, nBits: %d, vchProof size: %ld.", proof.nAlgo, proof.nBits, block.vchProof.size());
    }
    if (proof.destMint != block.txMint.sendTo)
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "destMint error, destMint: %s.", proof.destMint.ToString().c_str());
    }

    uint256 hashTarget = (~uint256(uint64(0)) >> nBits);

    vector<unsigned char> vchProofOfWork;
    block.GetSerializedProofOfWorkData(vchProofOfWork);
    uint256 hash = crypto::CryptoPowHash(&vchProofOfWork[0], vchProofOfWork.size());

    if (hash > hashTarget)
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_WORK_INVALID, "hash error: proof[%s] vs. target[%s] with bits[%d]",
                     hash.ToString().c_str(), hashTarget.ToString().c_str(), nBits);
    }

    return OK;
}

Errno CCoreProtocol::VerifyDelegatedProofOfStake(const CBlock& block, const CBlockIndex* pIndexPrev,
                                                 const CDelegateAgreement& agreement)
{
    uint32 nTime = DPoSTimestamp(pIndexPrev);
    if (block.GetBlockTime() != nTime)
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Timestamp out of range. block time %d is not equal %u", block.GetBlockTime(), nTime);
    }
    if (agreement.vBallot.size() == 0 || block.txMint.sendTo != agreement.vBallot[0])
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint sendTo error.");
    }
    if (block.txMint.nTimeStamp != block.GetBlockTime())
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint timestamp error.");
    }
    return OK;
}

Errno CCoreProtocol::VerifySubsidiary(const CBlock& block, const CBlockIndex* pIndexPrev, const CBlockIndex* pIndexRef,
                                      const CDelegateAgreement& agreement)
{
    if (block.GetBlockTime() <= pIndexPrev->GetBlockTime())
    {
        return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Timestamp out of range.");
    }

    if (block.IsSubsidiary())
    {
        if (block.GetBlockTime() != pIndexRef->GetBlockTime())
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Subsidiary timestamp out of range.");
        }
    }
    else
    {
        if (block.GetBlockTime() <= pIndexRef->GetBlockTime()
            || block.GetBlockTime() >= pIndexRef->GetBlockTime() + BLOCK_TARGET_SPACING
            /*|| block.GetBlockTime() != pIndexPrev->GetBlockTime() + EXTENDED_BLOCK_SPACING*/)
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Extended timestamp out of range.");
        }
        if (((block.GetBlockTime() - pIndexPrev->GetBlockTime()) % EXTENDED_BLOCK_SPACING) != 0)
        {
            return DEBUG(ERR_BLOCK_TIMESTAMP_OUT_OF_RANGE, "Extended timestamp error.");
        }
    }

    if (block.txMint.sendTo != agreement.GetBallot(0))
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint sendTo error.");
    }

    if (block.txMint.nTimeStamp != block.GetBlockTime())
    {
        return DEBUG(ERR_BLOCK_PROOF_OF_STAKE_INVALID, "txMint timestamp error.");
    }
    return OK;
}

Errno CCoreProtocol::VerifyBlock(const CBlock& block, CBlockIndex* pIndexPrev)
{
    (void)block;
    (void)pIndexPrev;
    return OK;
}

Errno CCoreProtocol::VerifyBlockTx(const CTransaction& tx, const CTxContxt& txContxt, CBlockIndex* pIndexPrev,
                                   int nBlockHeight, const uint256& fork, const CProfile& profile)
{
    Errno err = OK;
    const CDestination& destIn = txContxt.destIn;
    int64 nValueIn = 0;
    for (const CTxInContxt& inctxt : txContxt.vin)
    {
        if (inctxt.nTxTime > tx.nTimeStamp)
        {
            return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "tx time is ahead of input tx");
        }
        if (inctxt.IsLocked(pIndexPrev->GetBlockHeight()))
        {
            return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "input is still locked");
        }
        nValueIn += inctxt.nAmount;
    }

    if (!MoneyRange(nValueIn))
    {
        return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "valuein invalid %ld", nValueIn);
    }
    if (nValueIn < tx.nAmount + tx.nTxFee)
    {
        return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "valuein is not enough (%ld : %ld)", nValueIn, tx.nAmount + tx.nTxFee);
    }

    if ((tx.nType == CTransaction::TX_DEFI_REWARD || tx.nType == CTransaction::TX_DEFI_RELATION || tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT) && profile.nForkType != FORK_TYPE_DEFI)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "DeFi tx must be in DeFi fork");
    }
    if (tx.nType == CTransaction::TX_DEFI_RELATION && VerifyDeFiRelationTx(tx, destIn, nBlockHeight, fork) != OK)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid DeFi relation tx");
    }

    if (tx.nType == CTransaction::TX_CERT)
    {
        if (VerifyCertTx(tx, destIn, fork) != OK)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid cert tx");
        }
    }
    else if (tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT)
    {
        if (VerifyMintHeightTx(tx, destIn, fork, nBlockHeight, profile) != OK)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid mint height tx");
        }
    }

    uint16 nDestInTemplateType = 0;
    uint16 nSendToTemplateType = 0;
    CTemplateId tid;
    if (destIn.GetTemplateId(tid))
    {
        nDestInTemplateType = tid.GetType();
    }
    if (tx.sendTo.GetTemplateId(tid))
    {
        nSendToTemplateType = tid.GetType();
    }

    if (nDestInTemplateType == TEMPLATE_VOTE || nSendToTemplateType == TEMPLATE_VOTE)
    {
        if (VerifyVoteTx(tx, destIn, pIndexPrev->GetBlockHash(), fork, true) != OK)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid vote tx");
        }
    }

    switch (nDestInTemplateType)
    {
    case TEMPLATE_DEXORDER:
        err = VerifyDexOrderTx(tx, destIn, nValueIn, nBlockHeight);
        if (err != OK)
        {
            return DEBUG(err, "invalid dex order tx");
        }
        break;
    case TEMPLATE_DEXMATCH:
        err = VerifyDexMatchTx(tx, nValueIn, nBlockHeight);
        if (err != OK)
        {
            return DEBUG(err, "invalid dex match tx");
        }
        break;
    }

    if (nSendToTemplateType == TEMPLATE_DEXMATCH && nDestInTemplateType != TEMPLATE_DEXORDER)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid sendto dex match tx");
    }

    if (nSendToTemplateType == TEMPLATE_ACTIVATE)
    {
        Errno err = VerifyActivateTx(tx, destIn, nBlockHeight, fork);
        if (err != OK)
        {
            return DEBUG(err, "invalid activate tx");
        }
    }

    vector<uint8> vchSig;
    if (!CTemplate::VerifyDestRecorded(tx, nBlockHeight, vchSig))
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid recoreded destination");
    }

    if (nDestInTemplateType == TEMPLATE_FORK)
    {
        err = VerifyForkRedeem(tx, destIn, fork, pIndexPrev->GetBlockHash(), vchSig, nValueIn);
        if (err != OK)
        {
            return DEBUG(err, "Verify fork redeem fail");
        }
    }

    if (nSendToTemplateType == TEMPLATE_FORK)
    {
        err = VerifyForkTx(tx, destIn, fork, nBlockHeight);
        if (err != OK)
        {
            //if (nBlockHeight > VALID_FORK_VERIFY_HEIGHT)
            //{
            return DEBUG(err, "Verify fork tx fail");
            //}
        }
    }

    /*if (nDestInTemplateType == TEMPLATE_DEXMATCH && nBlockHeight < (int)MATCH_VERIFY_ERROR_HEIGHT)
    {
        nBlockHeight -= 1;
    }*/

    if (!destIn.VerifyTxSignature(tx.GetSignatureHash(), tx.nType, tx.hashAnchor, tx.sendTo, vchSig, nBlockHeight, fork))
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid signature");
    }

    return OK;
}

Errno CCoreProtocol::VerifyTransaction(const CTransaction& tx, const vector<CTxOut>& vPrevOutput,
                                       int nForkHeight, const uint256& fork, const CProfile& profile)
{
    Errno err = OK;
    CDestination destIn = vPrevOutput[0].destTo;
    int64 nValueIn = 0;
    for (const CTxOut& output : vPrevOutput)
    {
        if (destIn != output.destTo)
        {
            return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "input destination mismatched");
        }
        if (output.nTxTime > tx.nTimeStamp)
        {
            return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "tx time is ahead of input tx");
        }
        if (output.IsLocked(nForkHeight))
        {
            return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "input is still locked");
        }
        nValueIn += output.nAmount;
    }
    if (!MoneyRange(nValueIn))
    {
        return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "valuein invalid %ld", nValueIn);
    }
    if (nValueIn < tx.nAmount + tx.nTxFee)
    {
        return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "valuein is not enough (%ld : %ld)", nValueIn, tx.nAmount + tx.nTxFee);
    }

    if ((tx.nType == CTransaction::TX_DEFI_REWARD || tx.nType == CTransaction::TX_DEFI_RELATION || tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT) && profile.nForkType != FORK_TYPE_DEFI)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "DeFi tx must be in DeFi fork");
    }
    if (tx.nType == CTransaction::TX_DEFI_RELATION && VerifyDeFiRelationTx(tx, destIn, nForkHeight + 1, fork) != OK)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid DeFi relation tx");
    }

    if (tx.nType == CTransaction::TX_CERT)
    {
        if (VerifyCertTx(tx, destIn, fork) != OK)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid cert tx");
        }
    }
    else if (tx.nType == CTransaction::TX_DEFI_MINT_HEIGHT)
    {
        if (VerifyMintHeightTx(tx, destIn, fork, nForkHeight + 1, profile) != OK)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid mint height tx");
        }
    }

    uint16 nDestInTemplateType = 0;
    uint16 nSendToTemplateType = 0;
    CTemplateId tid;
    if (destIn.GetTemplateId(tid))
    {
        nDestInTemplateType = tid.GetType();
    }
    if (tx.sendTo.GetTemplateId(tid))
    {
        nSendToTemplateType = tid.GetType();
    }

    if (nDestInTemplateType == TEMPLATE_VOTE || nSendToTemplateType == TEMPLATE_VOTE)
    {
        CBlockStatus status;
        if (!pBlockChain->GetLastBlockStatus(fork, status))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "get last fail");
        }
        if (VerifyVoteTx(tx, destIn, status.hashBlock, fork, false) != OK)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid vote tx");
        }
    }

    switch (nDestInTemplateType)
    {
    case TEMPLATE_DEXORDER:
        err = VerifyDexOrderTx(tx, destIn, nValueIn, nForkHeight + 1);
        if (err != OK)
        {
            return DEBUG(err, "invalid dex order tx");
        }
        break;
    case TEMPLATE_DEXMATCH:
        err = VerifyDexMatchTx(tx, nValueIn, nForkHeight + 1);
        if (err != OK)
        {
            return DEBUG(err, "invalid dex match tx");
        }
        break;
    }

    if (nSendToTemplateType == TEMPLATE_DEXMATCH && nDestInTemplateType != TEMPLATE_DEXORDER)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "invalid sendto dex match tx");
    }

    if (nSendToTemplateType == TEMPLATE_ACTIVATE)
    {
        Errno err = VerifyActivateTx(tx, destIn, nForkHeight + 1, fork);
        if (err != OK)
        {
            return DEBUG(err, "invalid activate tx");
        }
    }

    // record destIn in vchSig
    vector<uint8> vchSig;
    if (!CTemplate::VerifyDestRecorded(tx, nForkHeight + 1, vchSig))
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid recoreded destination");
    }

    if (!destIn.VerifyTxSignature(tx.GetSignatureHash(), tx.nType, tx.hashAnchor, tx.sendTo, vchSig, nForkHeight + 1, fork))
    {
        return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid signature");
    }

    // locked coin template: nValueIn >= tx.nAmount + tx.nTxFee + nLockedCoin
    if (nDestInTemplateType == TEMPLATE_FORK)
    {
        if (fork != GetGenesisBlockHash())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "invalid fork");
        }
        if (tx.sendTo == destIn)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "It is not allowed to change from self to self");
        }
        CBlockStatus status;
        if (!pBlockChain->GetLastBlockStatus(GetGenesisBlockHash(), status))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "Failed to get last block");
        }
        CTemplatePtr ptr = CTemplate::CreateTemplatePtr(destIn.GetTemplateId(), vchSig);
        if (!ptr)
        {
            return DEBUG(ERR_TRANSACTION_SIGNATURE_INVALID, "invalid locked coin template destination");
        }
        CDestination destRedeemLocked;
        uint256 hashForkLocked;
        boost::dynamic_pointer_cast<CLockedCoinTemplate>(ptr)->GetForkParam(destRedeemLocked, hashForkLocked);
        int64 nLockedCoin = pForkManager->ForkLockedCoin(hashForkLocked, status.hashBlock);
        if (nLockedCoin < 0)
        {
            bool fTxAtTxPool = false;
            for (int i = 0; i < tx.vInput.size(); i++)
            {
                uint256 hashFork;
                int nHeight;
                if (!pBlockChain->GetTxLocation(tx.vInput[i].prevout.hash, hashFork, nHeight))
                {
                    fTxAtTxPool = true;
                    break;
                }
            }
            nLockedCoin = CTemplateFork::CreatedCoin();
            if (!fTxAtTxPool)
            {
                nLockedCoin = 0;
            }
        }
        if (nValueIn < tx.nAmount + tx.nTxFee + nLockedCoin)
        {
            return DEBUG(ERR_TRANSACTION_INPUT_INVALID, "valuein is not enough to locked coin (%ld : %ld)", nValueIn, tx.nAmount + tx.nTxFee + nLockedCoin);
        }
    }

    if (nSendToTemplateType == TEMPLATE_FORK)
    {
        err = VerifyForkTx(tx, destIn, fork, nForkHeight + 1);
        if (err != OK)
        {
            return DEBUG(err, "Verify fork tx fail");
        }
    }

    return OK;
}

Errno CCoreProtocol::VerifyMintHeightTx(const CTransaction& tx, const CDestination& destIn, const uint256& hashFork, const int nHeight, const CProfile& profile)
{
    if (profile.nForkType != FORK_TYPE_DEFI)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "mint height Tx must be on DeFi fork");
    }
    if (profile.defi.nMintHeight >= 0)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "already has vald mint height in fork: %s", hashFork.ToString().c_str());
    }
    if (destIn != profile.destOwner || tx.sendTo != profile.destOwner)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "mint height Tx must be from owner to owner");
    }
    if (tx.vchData.size() != 4)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "mint height Tx must save 4 bytes height of mint in vchData");
    }
    int32 nMintHeight;
    CIDataStream is(tx.vchData);
    is >> nMintHeight;
    if (nMintHeight <= nHeight)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "mint height [%d] must be larger than current block chain height [%d]", nMintHeight, nHeight);
    }

    return OK;
}

bool CCoreProtocol::GetBlockTrust(const CBlock& block, uint256& nChainTrust, const CBlockIndex* pIndexPrev, const CDelegateAgreement& agreement, const CBlockIndex* pIndexRef, size_t nEnrollTrust)
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
            /*if (!IsDposHeight(nHeight))
            {
                StdError("CCoreProtocol", "GetBlockTrust: not dpos height, height: %d", nHeight);
                return false;
            }*/

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
            int64 nReward;
            if (GetProofOfWorkTarget(pIndexPrev, nAlgo, nBits, nReward))
            {
                if (agreement.nWeight == 0 || nBits <= 0)
                {
                    StdError("CCoreProtocol", "GetBlockTrust: nWeight or nBits error, nWeight: %lu, nBits: %d", agreement.nWeight, nBits);
                    return false;
                }
                if (nEnrollTrust <= 0)
                {
                    StdError("CCoreProtocol", "GetBlockTrust: nEnrollTrust error, nEnrollTrust: %lu", nEnrollTrust);
                    return false;
                }

                /*if (!IsDPoSNewTrustHeight(nHeight))
                {
                    // DPoS difficulty = weight * (2 ^ nBits)
                    nChainTrust = uint256(uint64(nEnrollTrust)) << nBits;
                }
                else
                {*/
                // DPoS difficulty = 2 ^ (nBits + weight)
                nChainTrust = uint256(1) << (int(nEnrollTrust) + nBits + 20);
                //}
            }
            else
            {
                StdError("CCoreProtocol", "GetBlockTrust: GetProofOfWorkTarget fail");
                return false;
            }
        }
        else
        {
            StdError("CCoreProtocol", "GetBlockTrust: Primary pIndexPrev is null");
            return false;
        }
    }
    else if (block.IsOrigin())
    {
        nChainTrust = uint64(0);
    }
    else if (block.IsSubsidiary() || block.IsExtended())
    {
        if (pIndexRef == nullptr)
        {
            StdError("CCoreProtocol", "GetBlockTrust: pIndexRef is null, block: %s", block.GetHash().GetHex().c_str());
            return false;
        }
        if (pIndexRef->pPrev == nullptr)
        {
            StdError("CCoreProtocol", "GetBlockTrust: Subsidiary or Extended block pPrev is null, block: %s", block.GetHash().GetHex().c_str());
            return false;
        }
        nChainTrust = pIndexRef->nChainTrust - pIndexRef->pPrev->nChainTrust;
    }
    else
    {
        StdError("CCoreProtocol", "GetBlockTrust: block type error");
        return false;
    }
    return true;
}

bool CCoreProtocol::GetProofOfWorkTarget(const CBlockIndex* pIndexPrev, int nAlgo, int& nBits, int64& nReward)
{
    nBits = 8;
    nReward = 0;
    return true;
}

void CCoreProtocol::GetDelegatedBallot(const uint256& nAgreement, const size_t nWeight, const map<CDestination, size_t>& mapBallot,
                                       const vector<pair<CDestination, int64>>& vecAmount, int64 nMoneySupply, vector<CDestination>& vBallot, size_t& nEnrollTrust, int nBlockHeight)
{
    vBallot.clear();
    if (nAgreement == 0 || mapBallot.size() == 0)
    {
        StdTrace("Core", "Get delegated ballot: height: %d, nAgreement: %s, mapBallot.size: %ld", nBlockHeight, nAgreement.GetHex().c_str(), mapBallot.size());
        return;
    }
    if (nMoneySupply < 0)
    {
        StdTrace("Core", "Get delegated ballot: nMoneySupply < 0");
        return;
    }
    if (vecAmount.size() != mapBallot.size())
    {
        StdError("Core", "Get delegated ballot: dest ballot size %llu is not equal amount size %llu", mapBallot.size(), vecAmount.size());
    }

    int nSelected = 0;
    for (const unsigned char* p = nAgreement.begin(); p != nAgreement.end(); ++p)
    {
        nSelected ^= *p;
    }

    map<CDestination, size_t> mapSelectBallot;
    //size_t nMaxWeight = std::min(nMoneySupply, DELEGATE_PROOF_OF_STATE_ENROLL_MAXIMUM_TOTAL_AMOUNT) / DELEGATE_PROOF_OF_STAKE_UNIT_AMOUNT;
    size_t nEnrollWeight = 0;
    nEnrollTrust = 0;
    for (auto& amount : vecAmount)
    {
        StdTrace("Core", "Get delegated ballot: height: %d, vote dest: %s, amount: %lld",
                 nBlockHeight, CAddress(amount.first).ToString().c_str(), amount.second);
        if (mapBallot.find(amount.first) != mapBallot.end())
        {
            size_t nDestWeight = (size_t)(min(amount.second, DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT) / DELEGATE_PROOF_OF_STAKE_UNIT_AMOUNT);
            mapSelectBallot[amount.first] = nDestWeight;
            nEnrollWeight += nDestWeight;
            nEnrollTrust += (size_t)(min(amount.second, DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT));
            StdTrace("Core", "Get delegated ballot: height: %d, ballot dest: %s, weight: %lld",
                     nBlockHeight, CAddress(amount.first).ToString().c_str(), nDestWeight);
        }
    }
    nEnrollTrust /= DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT;
    StdTrace("Core", "Get delegated ballot: trust height: %d, ballot dest count is %llu, enroll trust: %llu", nBlockHeight, mapSelectBallot.size(), nEnrollTrust);

    /*size_t nWeightWork = ((nMaxWeight - nEnrollWeight) * (nMaxWeight - nEnrollWeight) * (nMaxWeight - nEnrollWeight))
                         / (nMaxWeight * nMaxWeight);
    // new DPoS & PoW mint rate
    if (nBlockHeight >= CHANGE_MINT_RATE_HEIGHT)
    {
        nWeightWork /= 10;
    }*/
    size_t nWeightWork = 0;
    StdTrace("Core", "Get delegated ballot: weight height: %d, nRandomDelegate: %llu, nRandomWork: %llu, nWeightDelegate: %llu, nWeightWork: %llu",
             nBlockHeight, nSelected, (nWeightWork * 256 / (nWeightWork + nEnrollWeight)), nEnrollWeight, nWeightWork);

    if (nSelected >= nWeightWork * 256 / (nWeightWork + nEnrollWeight))
    {
        size_t total = nEnrollWeight;
        size_t n = (nSelected * DELEGATE_PROOF_OF_STAKE_MAXIMUM_TIMES) % total;
        for (map<CDestination, size_t>::const_iterator it = mapSelectBallot.begin(); it != mapSelectBallot.end(); ++it)
        {
            if (n < it->second)
            {
                vBallot.push_back(it->first);
                break;
            }
            n -= (*it).second;
        }
    }

    StdTrace("Core", "Get delegated ballot: height: %d, consensus: %s, ballot dest: %s",
             nBlockHeight, (vBallot.size() > 0 ? "dpos" : "pow"), (vBallot.size() > 0 ? CAddress(vBallot[0]).ToString().c_str() : ""));
}

int64 CCoreProtocol::MinEnrollAmount()
{
    return DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT;
}

uint32 CCoreProtocol::DPoSTimestamp(const CBlockIndex* pIndexPrev)
{
    if (pIndexPrev == nullptr || !pIndexPrev->IsPrimary())
    {
        return 0;
    }
    return pIndexPrev->GetBlockTime() + BLOCK_TARGET_SPACING;
}

uint32 CCoreProtocol::GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType)
{
    if (nPrevMintType == CTransaction::TX_WORK || nPrevMintType == CTransaction::TX_GENESIS)
    {
        if (nTargetMintType == CTransaction::TX_STAKE)
        {
            return nPrevTimeStamp + BLOCK_TARGET_SPACING;
        }
        return nPrevTimeStamp + PROOF_OF_WORK_BLOCK_SPACING;
    }
    return nPrevTimeStamp + BLOCK_TARGET_SPACING;
}

/*bool CCoreProtocol::IsRefVacantHeight(uint32 nBlockHeight)
{
    if (nBlockHeight < REF_VACANT_HEIGHT)
    {
        return false;
    }
    return true;
}

int CCoreProtocol::GetRefVacantHeight()
{
    return REF_VACANT_HEIGHT;
}*/

const std::set<CDestination> CCoreProtocol::GetDeFiBlacklist(const uint256& hashFork, const int32 nHeight)
{
    auto f = [](const uint256& hashFork, const int32 nHeight, const map<uint256, map<int, set<CDestination>>>& mapDeFiBlacklist) -> set<CDestination>
    {
        auto it = mapDeFiBlacklist.find(hashFork);
        if (it != mapDeFiBlacklist.end())
        {
            for (auto& list : boost::adaptors::reverse(it->second))
            {
                if (nHeight >= list.first)
                {
                    return list.second;
                }
            }
        }
        return set<CDestination>();
    };

    set<CDestination> commonBlacklist = f(uint256(), nHeight, mapDeFiBlacklist);
    set<CDestination> forkBlacklist = f(hashFork, nHeight, mapDeFiBlacklist);

    for (auto& dest : commonBlacklist)
    {
        forkBlacklist.insert(dest);
    }

    return forkBlacklist;
}

bool CCoreProtocol::VerifyTxDestActivate(const uint256& hashPrev, const CTransaction& tx)
{
    if (!tx.sendTo.IsTemplate() || tx.sendTo.GetTemplateId().GetType() != TEMPLATE_VOTE)
    {
        Log("Verify Tx Dest Activate: sendTo not VOTE, sendTo: %s, tx: %s", CAddress(tx.sendTo).ToString().c_str(), tx.GetHash().GetHex().c_str());
        return false;
    }
    auto ptrVote = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, tx.vchSig);
    if (ptrVote == nullptr)
    {
        Log("Verify Tx Dest Activate: CreateTemplatePtr fail, tx: %s", tx.GetHash().GetHex().c_str());
        return false;
    }
    auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptrVote);
    if (!pBlockChain->CheckDestActivate(hashPrev, objVote->destOwner))
    {
        Log("Verify Tx Dest Activate: Check dest activate fail, tx: %s", tx.GetHash().GetHex().c_str());
        return false;
    }
    if (!pBlockChain->CheckDestInvestAmount(hashPrev, tx.sendTo, false))
    {
        Log("Verify Tx Dest Activate: Check dest invest amount fail, tx: %s", tx.GetHash().GetHex().c_str());
        return false;
    }
    return true;
}

uint256 CCoreProtocol::CreateActivateSignHash(const uint256& hashFork, const CDestination& destActivate)
{
    //'activate'+forkid+destActivate
    std::string strSignData = std::string("activate") + hashFork.ToString() + CAddress(destActivate).ToString();
    uint256 hashSigData = crypto::CryptoHash(strSignData.data(), strSignData.size());
    return hashSigData;
}

bool CCoreProtocol::CheckBlockSignature(const CBlock& block)
{
    if (block.GetHash() != GetGenesisBlockHash())
    {
        return block.txMint.sendTo.VerifyBlockSignature(block.GetHash(), block.vchSig);
    }
    return true;
}

Errno CCoreProtocol::ValidateVacantBlock(const CBlock& block)
{
    if (block.hashMerkle != 0 || block.txMint != CTransaction() || !block.vtx.empty())
    {
        return DEBUG(ERR_BLOCK_TRANSACTIONS_INVALID, "vacant block tx is not empty.");
    }

    if (!block.vchProof.empty() || !block.vchSig.empty())
    {
        return DEBUG(ERR_BLOCK_SIGNATURE_INVALID, "vacant block proof or signature is not empty.");
    }

    return OK;
}

Errno CCoreProtocol::VerifyCertTx(const CTransaction& tx, const CDestination& destIn, const uint256& fork)
{
    // CERT transaction must be on the main chain
    if (fork != GetGenesisBlockHash())
    {
        Log("VerifyCertTx CERT tx is not on the main chain, fork: %s", fork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    // the `from` address must be equal to the `to` address of cert tx
    if (destIn != tx.sendTo)
    {
        Log("VerifyCertTx the `from` address is not equal the `to` address of CERT tx, from: %s, to: %s",
            CAddress(destIn).ToString().c_str(), CAddress(tx.sendTo).ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }
    // the `to` address must be delegate template address
    if (tx.sendTo.GetTemplateId().GetType() != TEMPLATE_DELEGATE)
    {
        Log("VerifyCertTx the `to` address of CERT tx is not a delegate template address, to: %s", CAddress(tx.sendTo).ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    return OK;
}

Errno CCoreProtocol::VerifyVoteTx(const CTransaction& tx, const CDestination& destIn, const uint256& hashPrev, const uint256& fork, const bool fBlockTx)
{
    uint256 txid = tx.GetHash();

    // VOTE transaction must be on the main chain
    if (fork != GetGenesisBlockHash())
    {
        Log("Verify Vote Tx: From or to vote template address tx is not on the main chain, txid: %s, fork: %s",
            txid.GetHex().c_str(), fork.ToString().c_str());
        return ERR_TRANSACTION_INVALID;
    }

    if (tx.sendTo.IsTemplate() && tx.sendTo.GetTemplateId().GetType() == TEMPLATE_VOTE)
    {
        if (tx.sendTo == destIn)
        {
            Log("Verify Vote Tx: From and to addresses cannot be the same, txid: %s", txid.GetHex().c_str());
            return ERR_TRANSACTION_INVALID;
        }

        if (tx.nAmount <= 0 || (tx.nAmount % INVEST_UNIT_AMOUNT) != 0)
        {
            Log("Verify Vote Tx: Transfer quantity is not a multiple of 100, nAmount: %ld, txid: %s", tx.nAmount, txid.GetHex().c_str());
            return ERR_TRANSACTION_INVALID;
        }

        // verify template data
        auto ptr = CTemplate::CreateTemplatePtr(TEMPLATE_VOTE, tx.vchSig);
        if (ptr == nullptr)
        {
            Log("Verify Vote Tx: Create template fail, txid: %s", txid.GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }

        if (!fBlockTx)
        {
            if (!pBlockChain->CheckDestInvestAmount(hashPrev, tx.sendTo, true))
            {
                Log("Verify Vote Tx: Address transferred, txid: %s", txid.GetHex().c_str());
                return ERR_TRANSACTION_INVALID;
            }
        }

        auto objVote = boost::dynamic_pointer_cast<CTemplateVote>(ptr);
        if (!pBlockChain->CheckDestActivate(hashPrev, objVote->destOwner))
        {
            Log("Verify Vote Tx: Check activate failed, txid: %s", txid.GetHex().c_str());
            return ERR_TRANSACTION_INVALID;
        }
    }

    if (destIn.IsTemplate() && destIn.GetTemplateId().GetType() == TEMPLATE_VOTE)
    {
        if (!pBlockChain->VerifyDestInvestRedeem(hashPrev, destIn, !fBlockTx))
        {
            Log("Verify Vote Tx: Verify redeem fail, dest: %s, txid: %s", CAddress(destIn).ToString().c_str(), txid.GetHex().c_str());
            return ERR_TRANSACTION_INVALID;
        }
    }
    return OK;
}

Errno CCoreProtocol::VerifyDexOrderTx(const CTransaction& tx, const CDestination& destIn, int64 nValueIn, int nHeight)
{
    uint16 nSendToTemplateType = 0;
    if (tx.sendTo.IsTemplate())
    {
        nSendToTemplateType = tx.sendTo.GetTemplateId().GetType();
    }

    vector<uint8> vchSig;
    if (!CTemplate::VerifyDestRecorded(tx, nHeight, vchSig))
    {
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }

    auto ptrOrder = CTemplate::CreateTemplatePtr(TEMPLATE_DEXORDER, vchSig);
    if (ptrOrder == nullptr)
    {
        Log("Verify dexorder tx: Create order template fail, tx: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }
    auto objOrder = boost::dynamic_pointer_cast<CTemplateDexOrder>(ptrOrder);
    if (nSendToTemplateType == TEMPLATE_DEXMATCH)
    {
        CTemplatePtr ptrMatch = CTemplate::CreateTemplatePtr(nSendToTemplateType, tx.vchSig);
        if (!ptrMatch)
        {
            Log("Verify dexorder tx: Create match template fail, tx: %s", tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }
        auto objMatch = boost::dynamic_pointer_cast<CTemplateDexMatch>(ptrMatch);

        set<CDestination> setSubDest;
        vector<uint8> vchSubSig;
        if (!objOrder->GetSignDestination(tx, uint256(), nHeight, tx.vchSig, setSubDest, vchSubSig))
        {
            Log("Verify dexorder tx: GetSignDestination fail, tx: %s", tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }
        if (setSubDest.empty() || objMatch->destMatch != *setSubDest.begin())
        {
            Log("Verify dexorder tx: destMatch error, tx: %s", tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }

        if (objMatch->destSellerOrder != destIn)
        {
            Log("Verify dexorder tx: destSellerOrder error, tx: %s", tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }
        if (objMatch->destSeller != objOrder->destSeller)
        {
            Log("Verify dexorder tx: destSeller error, tx: %s", tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }
        /*if (objMatch->nSellerValidHeight != objOrder->nValidHeight)
        {
            Log("Verify dexorder tx: nSellerValidHeight error, match nSellerValidHeight: %d, order nValidHeight: %d, tx: %s",
                objMatch->nSellerValidHeight, objOrder->nValidHeight, tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }*/
        if ((tx.nAmount != objMatch->nMatchAmount) || (tx.nAmount < (TNS_DEX_MIN_TX_FEE * 3 + TNS_DEX_MIN_MATCH_AMOUNT)))
        {
            Log("Verify dexorder tx: nAmount error, match nMatchAmount: %lu, tx amount: %lu, tx: %s",
                objMatch->nMatchAmount, tx.nAmount, tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }
        if (objMatch->nFee != objOrder->nFee)
        {
            Log("Verify dexorder tx: nFee error, match fee: %ld, order fee: %ld, tx: %s",
                objMatch->nFee, objMatch->nFee, tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }
    }
    return OK;
}

Errno CCoreProtocol::VerifyDexMatchTx(const CTransaction& tx, int64 nValueIn, int nHeight)
{
    vector<uint8> vchSig;
    if (!CTemplate::VerifyDestRecorded(tx, nHeight, vchSig))
    {
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }

    auto ptrMatch = CTemplate::CreateTemplatePtr(TEMPLATE_DEXMATCH, vchSig);
    if (ptrMatch == nullptr)
    {
        Log("Verify dex match tx: Create match template fail, tx: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }
    auto objMatch = boost::dynamic_pointer_cast<CTemplateDexMatch>(ptrMatch);
    if (nHeight <= objMatch->nSellerValidHeight)
    {
        if (tx.sendTo == objMatch->destBuyer)
        {
            int64 nBuyerAmount = ((uint64)(objMatch->nMatchAmount - TNS_DEX_MIN_TX_FEE * 3) * (FEE_PRECISION - objMatch->nFee)) / FEE_PRECISION;
            if (nValueIn != objMatch->nMatchAmount)
            {
                Log("Verify dex match tx: Send buyer nValueIn error, nValueIn: %lu, nMatchAmount: %lu, tx: %s",
                    nValueIn, objMatch->nMatchAmount, tx.GetHash().GetHex().c_str());
                return ERR_TRANSACTION_SIGNATURE_INVALID;
            }
            if (tx.nAmount != nBuyerAmount)
            {
                Log("Verify dex match tx: Send buyer tx nAmount error, nAmount: %lu, need amount: %lu, nMatchAmount: %lu, nFee: %ld, nTxFee: %lu, tx: %s",
                    tx.nAmount, nBuyerAmount, objMatch->nMatchAmount, objMatch->nFee, tx.nTxFee, tx.GetHash().GetHex().c_str());
                return ERR_TRANSACTION_SIGNATURE_INVALID;
            }
            if (tx.nTxFee != TNS_DEX_MIN_TX_FEE)
            {
                Log("Verify dex match tx: Send buyer tx nTxFee error, nAmount: %lu, need amount: %lu, nMatchAmount: %lu, nFee: %ld, nTxFee: %lu, tx: %s",
                    tx.nAmount, nBuyerAmount, objMatch->nMatchAmount, objMatch->nFee, tx.nTxFee, tx.GetHash().GetHex().c_str());
                return ERR_TRANSACTION_SIGNATURE_INVALID;
            }
        }
        else if (tx.sendTo == objMatch->destMatch)
        {
            int64 nBuyerAmount = ((uint64)(objMatch->nMatchAmount - TNS_DEX_MIN_TX_FEE * 3) * (FEE_PRECISION - objMatch->nFee)) / FEE_PRECISION;
            int64 nRewardAmount = ((uint64)(objMatch->nMatchAmount - TNS_DEX_MIN_TX_FEE * 3) * (objMatch->nFee / 2)) / FEE_PRECISION;
            if (nValueIn != (objMatch->nMatchAmount - nBuyerAmount - TNS_DEX_MIN_TX_FEE))
            {
                Log("Verify dex match tx: Send match nValueIn error, nValueIn: %lu, need amount: %lu, nMatchAmount: %lu, nFee: %ld, nTxFee: %lu, tx: %s",
                    nValueIn, objMatch->nMatchAmount - nBuyerAmount, objMatch->nMatchAmount, objMatch->nFee, tx.nTxFee, tx.GetHash().GetHex().c_str());
                return ERR_TRANSACTION_SIGNATURE_INVALID;
            }
            if (tx.nAmount != nRewardAmount)
            {
                Log("Verify dex match tx: Send match tx nAmount error, nAmount: %lu, need amount: %lu, nMatchAmount: %lu, nRewardAmount: %lu, nFee: %ld, nTxFee: %lu, tx: %s",
                    tx.nAmount, nRewardAmount, objMatch->nMatchAmount, nRewardAmount, objMatch->nFee, tx.nTxFee, tx.GetHash().GetHex().c_str());
                return ERR_TRANSACTION_SIGNATURE_INVALID;
            }
            if (tx.nTxFee != TNS_DEX_MIN_TX_FEE)
            {
                Log("Verify dex match tx: Send match tx nTxFee error, nAmount: %lu, need amount: %lu, nMatchAmount: %lu, nRewardAmount: %lu, nFee: %ld, nTxFee: %lu, tx: %s",
                    tx.nAmount, nRewardAmount, objMatch->nMatchAmount, nRewardAmount, objMatch->nFee, tx.nTxFee, tx.GetHash().GetHex().c_str());
                return ERR_TRANSACTION_SIGNATURE_INVALID;
            }
        }
        else
        {
            set<CDestination> setSubDest;
            vector<uint8> vchSubSig;
            if (!objMatch->GetSignDestination(tx, uint256(), nHeight, tx.vchSig, setSubDest, vchSubSig))
            {
                Log("Verify dex match tx: GetSignDestination fail, tx: %s", tx.GetHash().GetHex().c_str());
                return ERR_TRANSACTION_SIGNATURE_INVALID;
            }
            if (tx.sendTo == *setSubDest.begin())
            {
                int64 nBuyerAmount = ((uint64)(objMatch->nMatchAmount - TNS_DEX_MIN_TX_FEE * 3) * (FEE_PRECISION - objMatch->nFee)) / FEE_PRECISION;
                int64 nRewardAmount = ((uint64)(objMatch->nMatchAmount - TNS_DEX_MIN_TX_FEE * 3) * (objMatch->nFee / 2)) / FEE_PRECISION;
                if (nValueIn != (objMatch->nMatchAmount - nBuyerAmount - nRewardAmount - TNS_DEX_MIN_TX_FEE * 2))
                {
                    Log("Verify dex match tx: Send deal nValueIn error, nValueIn: %lu, need amount: %lu, nMatchAmount: %lu, nRewardAmount: %lu, nFee: %ld, nTxFee: %lu, tx: %s",
                        nValueIn, objMatch->nMatchAmount - nBuyerAmount - nRewardAmount, objMatch->nMatchAmount, nRewardAmount, objMatch->nFee, tx.nTxFee, tx.GetHash().GetHex().c_str());
                    return ERR_TRANSACTION_SIGNATURE_INVALID;
                }
                if (tx.nAmount != (nValueIn - TNS_DEX_MIN_TX_FEE))
                {
                    Log("Verify dex match tx: Send deal tx nAmount error, nAmount: %lu, need amount: %lu, nMatchAmount: %lu, nRewardAmount: %lu, nFee: %ld, nTxFee: %lu, tx: %s",
                        tx.nAmount, nValueIn - TNS_DEX_MIN_TX_FEE, objMatch->nMatchAmount, nRewardAmount, objMatch->nFee, tx.nTxFee, tx.GetHash().GetHex().c_str());
                    return ERR_TRANSACTION_SIGNATURE_INVALID;
                }
                if (tx.nTxFee != TNS_DEX_MIN_TX_FEE)
                {
                    Log("Verify dex match tx: Send deal tx nTxFee error, nAmount: %lu, need amount: %lu, nMatchAmount: %lu, nRewardAmount: %lu, nFee: %ld, nTxFee: %lu, tx: %s",
                        tx.nAmount, nValueIn - TNS_DEX_MIN_TX_FEE, objMatch->nMatchAmount, nRewardAmount, objMatch->nFee, tx.nTxFee, tx.GetHash().GetHex().c_str());
                    return ERR_TRANSACTION_SIGNATURE_INVALID;
                }
            }
            else
            {
                Log("Verify dex match tx: sendTo error, tx: %s", tx.GetHash().GetHex().c_str());
                return ERR_TRANSACTION_SIGNATURE_INVALID;
            }
        }

        set<CDestination> setSubDest;
        vector<uint8> vchSigOut;
        if (!ptrMatch->GetSignDestination(tx, uint256(), 0, vchSig, setSubDest, vchSigOut))
        {
            Log("Verify dex match tx: get sign data fail, tx: %s", tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }

        vector<uint8> vms;
        vector<uint8> vss;
        vector<uint8> vchSigSub;
        try
        {
            vector<uint8> head;
            xengine::CIDataStream is(vchSigOut);
            is >> vms >> vss >> vchSigSub;
        }
        catch (std::exception& e)
        {
            Log("Verify dex match tx: get vms and vss fail, tx: %s", tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }

        if (crypto::CryptoSHA256(&(vss[0]), vss.size()) != objMatch->hashBuyerSecret)
        {
            Log("Verify dex match tx: hashBuyerSecret error, vss: %s, secret: %s, tx: %s",
                ToHexString(vss).c_str(), objMatch->hashBuyerSecret.GetHex().c_str(), tx.GetHash().GetHex().c_str());
            return ERR_TRANSACTION_SIGNATURE_INVALID;
        }
    }
    return OK;
}

Errno CCoreProtocol::VerifyDeFiRelationTx(const CTransaction& tx, const CDestination& destIn, int nHeight, const uint256& fork)
{
    if (destIn == tx.sendTo)
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "DeFi relation tx from address must be not equal to sendto address");
    }

    const set<CDestination>& setBlacklist = GetDeFiBlacklist(fork, nHeight);
    if (setBlacklist.count(tx.sendTo) || setBlacklist.count(destIn))
    {
        return DEBUG(ERR_TRANSACTION_INVALID, "DeFi tx sendto Address or destIn is in blacklist");
    }

    // new relation type
    //if (nHeight >= NEW_DEFI_RELATION_TX_HEIGHT)
    {
        if (!tx.sendTo.IsPubKey() || !destIn.IsPubKey())
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "DeFi tx sendto Address and destIn must be public key address");
        }

        // vchData: shared_pubkey + sub_sig + parent_sig
        if (tx.vchData.size() != 160)
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "DeFi tx length of vchData is not 160");
        }
        uint256 sharedPubKey(vector<uint8>(tx.vchData.begin(), tx.vchData.begin() + 32));
        vector<uint8> subSign(tx.vchData.begin() + 32, tx.vchData.begin() + 96);
        vector<uint8> parentSign(tx.vchData.begin() + 96, tx.vchData.end());
        StdTrace("CCoreProtocol", "VerifyDeFiRelationTx sharedPubKey: %s, subSign: %s, parentSign: %s",
                 sharedPubKey.ToString().c_str(), ToHexString(subSign).c_str(), ToHexString(parentSign).c_str());

        // sub_sign: sign blake2b(DeFiRelation + forkid + shared_pubkey) with sendto
        crypto::CPubKey subKey = tx.sendTo.GetPubKey();
        string subSignStr = string("DeFiRelation") + fork.ToString() + sharedPubKey.ToString();
        uint256 subSignHashStr = crypto::CryptoHash(subSignStr.data(), subSignStr.size());
        StdTrace("CCoreProtocol", "VerifyDeFiRelationTx subSignStr: %s, subSignHashStr: %s", subSignStr.c_str(), ToHexString(subSignHashStr.begin(), subSignHashStr.size()).c_str());
        if (!crypto::CryptoVerify(subKey, subSignHashStr.begin(), subSignHashStr.size(), subSign))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "DeFi tx sub signature in vchData is not currect");
        }

        // parent_sign: sign blake2b(DeFiRelation + parent_pubkey) with sharedPubKey
        crypto::CPubKey parentKey = destIn.GetPubKey();
        string parentSignStr = string("DeFiRelation") + parentKey.ToString();
        uint256 parentSignHashStr = crypto::CryptoHash(parentSignStr.data(), parentSignStr.size());
        StdTrace("CCoreProtocol", "VerifyDeFiRelationTx parentSignStr: %s, parentSignHashStr: %s", parentSignStr.c_str(), ToHexString(parentSignHashStr.begin(), parentSignHashStr.size()).c_str());
        if (!crypto::CryptoVerify(sharedPubKey, parentSignHashStr.begin(), parentSignHashStr.size(), parentSign))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "DeFi tx parent signature in vchData is not currect");
        }
    }
    /*else
    {
        if ((!CTemplate::IsTxSpendable(tx.sendTo) || !CTemplate::IsTxSpendable(destIn)))
        {
            return DEBUG(ERR_TRANSACTION_INVALID, "DeFi tx sendto Address and destIn must be spendable");
        }
    }*/

    return OK;
}

Errno CCoreProtocol::VerifyActivateTx(const CTransaction& tx, const CDestination& destIn, int nHeight, const uint256& fork)
{
    if (tx.nAmount != MIN_ACTIVATE_AMOUNT)
    {
        Log("Verify activate tx: nAmount error, nAmount: %f, tx: %s", ValueFromToken(tx.nAmount), tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }
    auto ptrActivate = CTemplate::CreateTemplatePtr(TEMPLATE_ACTIVATE, tx.vchSig);
    if (ptrActivate == nullptr)
    {
        Log("Verify activate tx: Create template fail, tx: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }
    if (tx.sendTo != CDestination(ptrActivate->GetTemplateId()))
    {
        Log("Verify activate tx: sendTo error, sendTo: %s, sig address: %s, tx: %s",
            CAddress(tx.sendTo).ToString().c_str(), CAddress(ptrActivate->GetTemplateId()).ToString().c_str(), tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }
    auto objActivate = boost::dynamic_pointer_cast<CTemplateActivate>(ptrActivate);
    if (destIn != objActivate->destInviter)
    {
        Log("Verify activate tx: destIn error, destIn: %s, destInviter: %s, tx: %s",
            CAddress(destIn).ToString().c_str(), CAddress(objActivate->destInviter).ToString().c_str(), tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }
    if (tx.vchData.size() != 64)
    {
        Log("Verify activate tx: vchData error, tx: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }
    uint256 hashSigData = CreateActivateSignHash(fork, tx.sendTo);
    if (!crypto::CryptoVerify(objActivate->destOwner.GetPubKey(), hashSigData.begin(), hashSigData.size(), tx.vchData))
    {
        Log("Verify activate tx: Verify activate sign fail, tx: %s", tx.GetHash().GetHex().c_str());
        return ERR_TRANSACTION_SIGNATURE_INVALID;
    }
    return OK;
}

///////////////////////////////
// CTestNetCoreProtocol

CTestNetCoreProtocol::CTestNetCoreProtocol()
{
}

/*
Address: 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb
PubKey : f3afc3a42a31836c9111acc4f65d3bf512e10124cb04a4137c7a6ce87d6f1329
Secret : 141a6728ded4f83f767ea770e3582be497c5088fcc3b9ca248751887534f5197
*/
void CTestNetCoreProtocol::GetGenesisBlock(CBlock& block)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;
    const CDestination destOwner = CDestination(ibrio::crypto::CPubKey(uint256("f3afc3a42a31836c9111acc4f65d3bf512e10124cb04a4137c7a6ce87d6f1329")));

    block.SetNull();

    block.nVersion = 1;
    block.nType = CBlock::BLOCK_GENESIS;
    block.nTimeStamp = 1621322325;
    block.hashPrev = 0;

    CTransaction& tx = block.txMint;
    tx.nType = CTransaction::TX_GENESIS;
    tx.nTimeStamp = block.nTimeStamp;
    tx.sendTo = destOwner;
    tx.nAmount = NPSP_TOKEN_INIT; // initial number of token

    CProfile profile;
    profile.strName = "IBR Core Test";
    profile.strSymbol = "IBRT";
    profile.destOwner = destOwner;
    profile.nAmount = tx.nAmount;
    profile.nMintReward = 0;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nHalveCycle = 0;
    profile.SetFlag(true, false, false);

    profile.Save(block.vchProof);
}

///////////////////////////////
// CProofOfWorkParam

CProofOfWorkParam::CProofOfWorkParam(const bool fTestnetIn)
{
    fTestnet = fTestnetIn;
    nDelegateProofOfStakeEnrollMinimumAmount = DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT;
    nDelegateProofOfStakeEnrollMaximumAmount = DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT;

    if (fTestnet)
    {
        pCoreProtocol = (ICoreProtocol*)new CTestNetCoreProtocol();
    }
    else
    {
        pCoreProtocol = (ICoreProtocol*)new CCoreProtocol();
    }
    pCoreProtocol->InitializeGenesisBlock();

    hashGenesisBlock = pCoreProtocol->GetGenesisBlockHash();
}

CProofOfWorkParam::~CProofOfWorkParam()
{
    if (pCoreProtocol)
    {
        if (fTestnet)
        {
            delete (CTestNetCoreProtocol*)pCoreProtocol;
        }
        else
        {
            delete (CCoreProtocol*)pCoreProtocol;
        }
        pCoreProtocol = nullptr;
    }
}

uint32 CProofOfWorkParam::GetNextBlockTimeStamp(uint16 nPrevMintType, uint32 nPrevTimeStamp, uint16 nTargetMintType)
{
    if (nPrevMintType == CTransaction::TX_WORK || nPrevMintType == CTransaction::TX_GENESIS)
    {
        if (nTargetMintType == CTransaction::TX_STAKE)
        {
            return nPrevTimeStamp + BLOCK_TARGET_SPACING;
        }
        return nPrevTimeStamp + PROOF_OF_WORK_BLOCK_SPACING;
    }
    return nPrevTimeStamp + BLOCK_TARGET_SPACING;
}

Errno CProofOfWorkParam::ValidateOrigin(const CBlock& block, const CProfile& parentProfile, CProfile& forkProfile) const
{
    return pCoreProtocol->ValidateOrigin(block, parentProfile, forkProfile);
}

} // namespace ibrio
