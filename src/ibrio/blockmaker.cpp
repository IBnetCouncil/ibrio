// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockmaker.h"

#include <thread>

#include "address.h"
#include "template/delegate.h"
#include "template/mint.h"
#include "template/proof.h"
#include "util.h"

using namespace std;
using namespace xengine;

#define INITIAL_HASH_RATE (8)
#define WAIT_AGREEMENT_TIME_OFFSET -5
#define WAIT_NEWBLOCK_TIME (BLOCK_TARGET_SPACING + 5)
#define WAIT_LAST_EXTENDED_TIME 0 //(BLOCK_TARGET_SPACING - 10)
#define FORK_LAST_BLOCK_COUNT 30
#define FORK_WAIT_BLOCK_COUNT 60
#define FORK_WAIT_BLOCK_SECT 100000

namespace ibrio
{

//////////////////////////////
// CBlockMakerHashAlgo

class CHashAlgo_Cryptonight : public ibrio::CBlockMakerHashAlgo
{
public:
    CHashAlgo_Cryptonight(int64 nHashRateIn)
      : CBlockMakerHashAlgo("Cryptonight", nHashRateIn) {}
    uint256 Hash(const std::vector<unsigned char>& vchData)
    {
        return crypto::CryptoPowHash(vchData.data(), vchData.size());
    }
};

//////////////////////////////
// CBlockMakerProfile
bool CBlockMakerProfile::BuildTemplate()
{
    crypto::CPubKey pubkey;
    if (destMint.GetPubKey(pubkey) && pubkey == keyMint.GetPubKey())
    {
        return false;
    }
    if (nAlgo == CM_MPVSS)
    {
        templMint = CTemplateMint::CreateTemplatePtr(new CTemplateDelegate(keyMint.GetPubKey(), destMint));
    }
    else if (nAlgo < CM_MAX)
    {
        templMint = CTemplateMint::CreateTemplatePtr(new CTemplateProof(keyMint.GetPubKey(), destMint));
    }
    return (templMint != nullptr);
}

//////////////////////////////
// CBlockMaker

CBlockMaker::CBlockMaker()
  : thrMaker("blockmaker", boost::bind(&CBlockMaker::BlockMakerThreadFunc, this)),
    thrPow("powmaker", boost::bind(&CBlockMaker::PowThreadFunc, this))
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pForkManager = nullptr;
    pTxPool = nullptr;
    pDispatcher = nullptr;
    pConsensus = nullptr;
    pService = nullptr;
    mapHashAlgo[CM_CRYPTONIGHT] = new CHashAlgo_Cryptonight(INITIAL_HASH_RATE);
}

CBlockMaker::~CBlockMaker()
{
    for (map<int, CBlockMakerHashAlgo*>::iterator it = mapHashAlgo.begin(); it != mapHashAlgo.end(); ++it)
    {
        delete ((*it).second);
    }
    mapHashAlgo.clear();
}

bool CBlockMaker::HandleInitialize()
{
    if (!GetObject("coreprotocol", pCoreProtocol))
    {
        Error("Failed to request coreprotocol\n");
        return false;
    }

    if (!GetObject("blockchain", pBlockChain))
    {
        Error("Failed to request blockchain\n");
        return false;
    }

    if (!GetObject("forkmanager", pForkManager))
    {
        Error("Failed to request forkmanager\n");
        return false;
    }

    if (!GetObject("txpool", pTxPool))
    {
        Error("Failed to request txpool\n");
        return false;
    }

    if (!GetObject("dispatcher", pDispatcher))
    {
        Error("Failed to request dispatcher\n");
        return false;
    }

    if (!GetObject("consensus", pConsensus))
    {
        Error("Failed to request consensus\n");
        return false;
    }

    if (!GetObject("service", pService))
    {
        Error("Failed to request service");
        return false;
    }

    if (!MintConfig()->destMpvss.IsNull() && MintConfig()->keyMpvss != 0)
    {
        CBlockMakerProfile profile(CM_MPVSS, MintConfig()->destMpvss, MintConfig()->keyMpvss);
        if (profile.IsValid())
        {
            mapDelegatedProfile.insert(make_pair(profile.GetDestination(), profile));
        }
    }

    if (!MintConfig()->destCryptonight.IsNull() && MintConfig()->keyCryptonight != 0)
    {
        CBlockMakerProfile profile(CM_CRYPTONIGHT, MintConfig()->destCryptonight, MintConfig()->keyCryptonight);
        if (profile.IsValid())
        {
            mapWorkProfile.insert(make_pair(CM_CRYPTONIGHT, profile));
        }
    }

    // print log
    const char* ConsensusMethodName[CM_MAX] = { "mpvss", "cryptonight" };
    Log("Block maker started");
    for (map<int, CBlockMakerProfile>::iterator it = mapWorkProfile.begin(); it != mapWorkProfile.end(); ++it)
    {
        CBlockMakerProfile& profile = (*it).second;
        Log("Profile [%s] : dest=%s,pubkey=%s,pow=%s",
            ConsensusMethodName[(*it).first],
            CAddress(profile.destMint).ToString().c_str(),
            profile.keyMint.GetPubKey().GetHex().c_str(),
            CAddress(profile.templMint->GetTemplateId()).ToString().c_str());
    }
    for (map<CDestination, CBlockMakerProfile>::iterator it = mapDelegatedProfile.begin();
         it != mapDelegatedProfile.end(); ++it)
    {
        CBlockMakerProfile& profile = (*it).second;
        Log("Profile [%s] : dest=%s,pubkey=%s,dpos=%s",
            ConsensusMethodName[CM_MPVSS],
            CAddress(profile.destMint).ToString().c_str(),
            profile.keyMint.GetPubKey().GetHex().c_str(),
            CAddress(profile.templMint->GetTemplateId()).ToString().c_str());
    }

    return true;
}

void CBlockMaker::HandleDeinitialize()
{
    pCoreProtocol = nullptr;
    pBlockChain = nullptr;
    pForkManager = nullptr;
    pTxPool = nullptr;
    pDispatcher = nullptr;
    pConsensus = nullptr;
    pService = nullptr;

    mapWorkProfile.clear();
    mapDelegatedProfile.clear();
}

bool CBlockMaker::HandleInvoke()
{
    if (!IBlockMaker::HandleInvoke())
    {
        return false;
    }

    fExit = false;
    if (!ThreadDelayStart(thrMaker))
    {
        return false;
    }

    if (!mapWorkProfile.empty())
    {
        if (!ThreadDelayStart(thrPow))
        {
            return false;
        }
    }

    if (!fExit)
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        pBlockChain->GetLastBlockStatus(pCoreProtocol->GetGenesisBlockHash(), lastStatus);
    }
    return true;
}

void CBlockMaker::HandleHalt()
{
    fExit = true;
    condExit.notify_all();
    condBlock.notify_all();

    thrMaker.Interrupt();
    ThreadExit(thrMaker);

    thrPow.Interrupt();
    ThreadExit(thrPow);

    IBlockMaker::HandleHalt();
}

bool CBlockMaker::HandleEvent(CEventBlockMakerUpdate& eventUpdate)
{
    if (fExit)
    {
        return true;
    }

    {
        uint256 nAgreement;
        size_t nWeight;
        vector<CDestination> vBallot;
        pConsensus->GetAgreement(eventUpdate.data.nBlockHeight, nAgreement, nWeight, vBallot);

        string strMintType = "dpos";
        if (eventUpdate.data.nMintType == CTransaction::TX_WORK)
        {
            strMintType = "pow";
        }

        if (vBallot.size() == 0)
        {
            Log("MakerUpdate: height: %d, consensus: pow, block type: %s, block: %s",
                eventUpdate.data.nBlockHeight, strMintType.c_str(), eventUpdate.data.hashBlock.GetHex().c_str());
        }
        else
        {
            Log("MakerUpdate: height: %d, consensus: dpos, block type: %s, block: %s, ballot address: %s",
                eventUpdate.data.nBlockHeight, strMintType.c_str(), eventUpdate.data.hashBlock.GetHex().c_str(), CAddress(vBallot[0]).ToString().c_str());
        }
    }

    {
        boost::unique_lock<boost::mutex> lock(mutex);

        CBlockMakerUpdate& data = eventUpdate.data;
        lastStatus.hashPrevBlock = data.hashPrevBlock;
        lastStatus.hashBlock = data.hashBlock;
        lastStatus.nBlockTime = data.nBlockTime;
        lastStatus.nBlockHeight = data.nBlockHeight;
        lastStatus.nMintType = data.nMintType;

        condBlock.notify_all();
    }
    return true;
}

bool CBlockMaker::InterruptedPoW(const uint256& hashPrimary)
{
    boost::unique_lock<boost::mutex> lock(mutex);
    return fExit || (hashPrimary != lastStatus.hashBlock);
}

bool CBlockMaker::WaitExit(const long nSeconds)
{
    if (nSeconds <= 0)
    {
        return !fExit;
    }
    boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(nSeconds);
    boost::unique_lock<boost::mutex> lock(mutex);
    while (!fExit)
    {
        if (!condExit.timed_wait(lock, timeout))
        {
            break;
        }
    }
    return !fExit;
}

bool CBlockMaker::WaitUpdateEvent(const long nSeconds)
{
    boost::system_time const timeout = boost::get_system_time() + boost::posix_time::seconds(nSeconds);
    boost::unique_lock<boost::mutex> lock(mutex);
    while (!fExit)
    {
        if (!condBlock.timed_wait(lock, timeout))
        {
            break;
        }
    }
    return !fExit;
}

void CBlockMaker::PrepareBlock(CBlock& block, const uint256& hashPrev, const uint64& nPrevTime,
                               const uint32& nPrevHeight, const CDelegateAgreement& agreement)
{
    block.SetNull();
    block.nType = CBlock::BLOCK_PRIMARY;
    block.hashPrev = hashPrev;
    CProofOfSecretShare proof;
    proof.nWeight = agreement.nWeight;
    proof.nAgreement = agreement.nAgreement;
    proof.Save(block.vchProof);
    if (agreement.nAgreement != 0)
    {
        pConsensus->GetProof(nPrevHeight + 1, block.vchProof);
    }
}

bool CBlockMaker::ArrangeBlockTx(CBlock& block, const uint256& hashFork, const CBlockMakerProfile& profile)
{
    CForkContext forkCtxt;
    bool isDeFi = false;
    if (pBlockChain->GetForkContext(hashFork, forkCtxt))
    {
        isDeFi = (forkCtxt.GetProfile().nForkType == FORK_TYPE_DEFI) ? true : false;
    }

    size_t nRestOfSize = MAX_BLOCK_SIZE - GetSerializeSize(block) - profile.GetSignatureSize();
    size_t rewardTxSize = 0;
    int64 nRewardTxTotalFee = 0;
    if (isDeFi)
    {
        CTransaction txDefault;
        txDefault.SetNull();
        size_t txDefaultSize = GetSerializeSize(txDefault) + 40;
        int32 nMaxTx = nRestOfSize / txDefaultSize;

        list<CDeFiReward> rewards = pBlockChain->GetDeFiReward(hashFork, block.hashPrev, block.GetBlockHeight(), nMaxTx);
        for (const auto& reward : rewards)
        {
            CTransaction txNew;
            txNew.SetNull();
            txNew.hashAnchor = reward.hashAnchor;
            txNew.nType = CTransaction::TX_DEFI_REWARD;
            txNew.nTimeStamp = block.GetBlockTime();
            txNew.nLockUntil = 0;
            txNew.sendTo = reward.dest;
            txNew.nTxFee = MIN_TX_FEE;
            txNew.nAmount = reward.nReward - txNew.nTxFee;
            // save reward detailed
            CODataStream os(txNew.vchData, 40);
            os << reward.nAmount << reward.nRank << reward.nStakeReward << reward.nAchievement << reward.nPower << reward.nPromotionReward;
            if (rewardTxSize + GetSerializeSize(txNew) > nRestOfSize)
            {
                break;
            }

            block.vtx.push_back(txNew);
            nRewardTxTotalFee += txNew.nTxFee;
            rewardTxSize += GetSerializeSize(txNew);
        }
    }

    if (block.IsPrimary())
    {
        vector<CTransaction> vInvestRewardTx;
        if (!pBlockChain->CalcBlockInvestRewardTx(block.hashPrev, block.GetBlockTime(), vInvestRewardTx))
        {
            Error("ArrangeBlockTx CalcBlockInvestRewardTx error, hashPrev: %s", block.hashPrev.ToString().c_str());
            return false;
        }
        for (const CTransaction& tx : vInvestRewardTx)
        {
            size_t nTxSize = GetSerializeSize(tx);
            if (rewardTxSize + nTxSize > nRestOfSize)
            {
                Error("ArrangeBlockTx Reward tx size error, hashPrev: %s", block.hashPrev.ToString().c_str());
                return false;
            }
            block.vtx.push_back(tx);
            nRewardTxTotalFee += tx.nTxFee;
            rewardTxSize += nTxSize;
        }
    }

    size_t nMaxTxSize = nRestOfSize - rewardTxSize;
    int64 nTotalTxFee = nRewardTxTotalFee;
    if (!pTxPool->FetchArrangeBlockTx(hashFork, block.hashPrev, block.GetBlockHeight(), block.GetBlockTime(), nMaxTxSize, block.vtx, nTotalTxFee))
    {
        Error("Fetch arrange block tx error, block: %s", block.GetHash().ToString().c_str());
    }
    block.hashMerkle = block.CalcMerkleTreeRoot();
    block.txMint.nAmount += nTotalTxFee;

    return true;
}

bool CBlockMaker::SignBlock(CBlock& block, const CBlockMakerProfile& profile)
{
    uint256 hashSig = block.GetHash();
    vector<unsigned char> vchMintSig;
    if (!profile.keyMint.Sign(hashSig, vchMintSig))
    {
        StdTrace("blockmaker", "keyMint Sign failed. hashSig: %s", hashSig.ToString().c_str());
        return false;
    }
    return profile.templMint->BuildBlockSignature(hashSig, vchMintSig, block.vchSig);
}

bool CBlockMaker::DispatchBlock(const uint256& hashFork, const CBlock& block)
{
    Debug("Dispatching block: %s, type: %u", block.GetHash().ToString().c_str(), block.nType);
    int nWait = block.nTimeStamp - GetNetTime();
    if (!WaitExit(nWait))
    {
        return false;
    }

    if (!block.IsExtended() && !pBlockChain->VerifyCheckPoint(hashFork, block.GetBlockHeight(), block.GetHash()))
    {
        Error("Fork %s block at height %d does not match checkpoint hash", hashFork.ToString().c_str(), block.GetBlockHeight());
        return false;
    }

    Errno err = pDispatcher->AddNewBlock(block);
    if (err != OK)
    {
        if (err != ERR_ALREADY_HAVE)
        {
            Error("Dispatch new block failed (%d) : %s", err, ErrorString(err));
            return false;
        }
        Debug("Dispatching block: %s already have, type: %u", block.GetHash().ToString().c_str(), block.nType);
    }
    return true;
}

void CBlockMaker::ProcessDelegatedProofOfStake(const CAgreementBlock& consParam)
{
    map<CDestination, CBlockMakerProfile>::iterator it = mapDelegatedProfile.find(consParam.agreement.vBallot[0]);
    if (it != mapDelegatedProfile.end())
    {
        CBlockMakerProfile& profile = (*it).second;

        CBlock block;
        PrepareBlock(block, consParam.hashPrev, consParam.nPrevTime, consParam.nPrevHeight, consParam.agreement);

        // get block time
        block.nTimeStamp = pBlockChain->DPoSTimestamp(block.hashPrev);
        if (block.nTimeStamp == 0)
        {
            Error("Get DPoSTimestamp error, hashPrev: %s", block.hashPrev.ToString().c_str());
            return;
        }

        // create DPoS primary block
        if (!CreateDelegatedBlock(block, pCoreProtocol->GetGenesisBlockHash(), profile))
        {
            Error("CreateDelegatedBlock error, hashPrev: %s", block.hashPrev.ToString().c_str());
            return;
        }

        // dispatch DPoS primary block
        if (DispatchBlock(pCoreProtocol->GetGenesisBlockHash(), block))
        {
            pDispatcher->SetConsensus(consParam);

            pDispatcher->CheckAllSubForkLastBlock();

            // create sub fork blocks
            ProcessSubFork(profile, consParam.agreement, block.GetHash(), block.GetBlockTime(), consParam.nPrevHeight, consParam.nPrevMintType);
        }
    }
}

void CBlockMaker::ProcessSubFork(const CBlockMakerProfile& profile, const CDelegateAgreement& agreement,
                                 const uint256& hashRefBlock, int64 nRefBlockTime, const int32 nPrevHeight, const uint16 nPrevMintType)
{
    map<uint256, CForkStatus> mapForkStatus;
    pBlockChain->GetValidForkStatus(mapForkStatus);

    // create subsidiary task
    multimap<int64, pair<uint256, CBlock>> mapBlocks;
    for (map<uint256, CForkStatus>::iterator it = mapForkStatus.begin(); it != mapForkStatus.end(); ++it)
    {
        const uint256& hashFork = it->first;
        if (hashFork != pCoreProtocol->GetGenesisBlockHash() && pForkManager->IsAllowed(hashFork))
        {
            CBlock block;
            PreparePiggyback(block, agreement, hashRefBlock, nRefBlockTime, nPrevHeight, it->second, nPrevMintType);
            mapBlocks.insert(make_pair(nRefBlockTime, make_pair(hashFork, block)));
        }
    }

    while (!mapBlocks.empty())
    {
        auto it = mapBlocks.begin();
        int64 nSeconds = it->first - GetNetTime();
        const uint256 hashFork = it->second.first;
        CBlock block = it->second.second;
        mapBlocks.erase(it);

        if (!WaitExit(nSeconds))
        {
            break;
        }

        bool fCreateExtendedTask = false;
        uint256 hashExtendedPrevBlock;
        int64 nExtendedPrevTime = 0;

        if (block.IsSubsidiary())
        {
            // query previous last extended block
            if (block.hashPrev == 0)
            {
                CBlockStatus status;
                if (pBlockChain->GetLastBlockStatus(hashFork, status))
                {
                    if (!pBlockChain->VerifyForkRefLongChain(hashFork, status.hashBlock, hashRefBlock))
                    {
                        Error("ProcessSubFork fork does not refer to long chain, fork: %s", hashFork.ToString().c_str());
                    }
                    else if (status.nBlockHeight > nPrevHeight)
                    {
                        if (pBlockChain->GetLastBlockOfHeight(hashFork, nPrevHeight, status.hashBlock, status.nBlockTime))
                        {
                            block.hashPrev = status.hashBlock;
                        }
                        else
                        {
                            Error("ProcessSubFork get last block error, fork: %s", hashFork.ToString().c_str());
                        }
                    }
                    else if (status.nBlockHeight == nPrevHeight)
                    {
                        if (nPrevMintType != CTransaction::TX_STAKE
                            || status.nBlockTime + EXTENDED_BLOCK_SPACING == nRefBlockTime
                            || GetNetTime() - nRefBlockTime >= WAIT_LAST_EXTENDED_TIME)
                        {
                            block.hashPrev = status.hashBlock;
                        }
                        else
                        {
                            mapBlocks.insert(make_pair(GetNetTime() + 1, make_pair(hashFork, block)));
                        }
                    }
                    else
                    {
                        if (status.nBlockHeight != CBlock::GetBlockHeightByHash(hashFork)
                            && status.nBlockHeight < nPrevHeight + 1 - FORK_LAST_BLOCK_COUNT)
                        {
                            auto& forkStat = mapForkLastStat[hashFork];
                            if (forkStat.first != status.nBlockHeight)
                            {
                                forkStat.first = status.nBlockHeight;
                                forkStat.second = nPrevHeight + 1;
                            }
                            if (nPrevHeight + 1 - forkStat.second >= FORK_WAIT_BLOCK_COUNT * ((nPrevHeight + 1 - status.nBlockHeight) / FORK_WAIT_BLOCK_SECT + 1))
                            {
                                if (ReplenishSubForkVacant(hashFork, status.nBlockHeight, status.hashBlock, profile, agreement, hashRefBlock, nPrevHeight))
                                {
                                    block.hashPrev = status.hashBlock;
                                }
                                else
                                {
                                    Error("ProcessSubFork replenish vacant error, fork: %s", hashFork.ToString().c_str());
                                }
                            }
                            else
                            {
                                Error("ProcessSubFork fork is missing too many blocks, primary height: %d, fork height: %d, fork: %s",
                                      nPrevHeight + 1, status.nBlockHeight, hashFork.ToString().c_str());
                            }
                        }
                        else
                        {
                            if (nPrevMintType != CTransaction::TX_STAKE || GetNetTime() - nRefBlockTime >= WAIT_LAST_EXTENDED_TIME)
                            {
                                if (ReplenishSubForkVacant(hashFork, status.nBlockHeight, status.hashBlock, profile, agreement, hashRefBlock, nPrevHeight))
                                {
                                    block.hashPrev = status.hashBlock;
                                }
                                else
                                {
                                    Error("ProcessSubFork replenish vacant error, fork: %s", hashFork.ToString().c_str());
                                }
                            }
                            else
                            {
                                mapBlocks.insert(make_pair(GetNetTime() + 1, make_pair(hashFork, block)));
                            }
                        }
                    }
                }
                else
                {
                    Error("ProcessSubFork GetLastBlock fail, fork: %s", hashFork.ToString().c_str());
                }
            }

            // make subsidiary block
            if (block.hashPrev != 0)
            {
                if (CreateDelegatedBlock(block, hashFork, profile))
                {
                    if (DispatchBlock(hashFork, block))
                    {
                        fCreateExtendedTask = true;
                        hashExtendedPrevBlock = block.GetHash();
                        nExtendedPrevTime = block.GetBlockTime();
                    }
                    else
                    {
                        Error("ProcessSubFork dispatch subsidiary block error, fork: %s, block: %s", hashFork.ToString().c_str(), block.GetHash().ToString().c_str());
                    }
                }
                else
                {
                    Error("ProcessSubFork CreateDelegatedBlock error, fork: %s, block: %s", hashFork.ToString().c_str(), block.GetHash().ToString().c_str());
                }
            }
        }
        else
        {
            // make extended block
            if (!ArrangeBlockTx(block, hashFork, profile))
            {
                Error("ProcessSubFork ArrangeBlockTx fail, fork: %s, block: %s",
                      hashFork.ToString().c_str(), block.GetHash().ToString().c_str());
            }
            if (block.vtx.size() == 0)
            {
                fCreateExtendedTask = true;
                hashExtendedPrevBlock = block.hashPrev;
                nExtendedPrevTime = block.GetBlockTime();
            }
            else if (!SignBlock(block, profile))
            {
                Error("ProcessSubFork extended block sign error, fork: %s, block: %s, seq: %d",
                      hashFork.ToString().c_str(), block.GetHash().ToString().c_str(),
                      ((int64)(block.nTimeStamp) - nRefBlockTime) / EXTENDED_BLOCK_SPACING);
            }
            else if (!DispatchBlock(hashFork, block))
            {
                Error("ProcessSubFork dispatch subsidiary block error, fork: %s, block: %s",
                      hashFork.ToString().c_str(), block.GetHash().ToString().c_str());
            }
            else
            {
                fCreateExtendedTask = true;
                hashExtendedPrevBlock = block.GetHash();
                nExtendedPrevTime = block.GetBlockTime();
            }
        }

        // create next extended task
        if (fCreateExtendedTask && nExtendedPrevTime + EXTENDED_BLOCK_SPACING < nRefBlockTime + BLOCK_TARGET_SPACING)
        {
            CBlock extended;
            CreateExtended(extended, profile, agreement, hashRefBlock, hashFork, hashExtendedPrevBlock, nExtendedPrevTime + EXTENDED_BLOCK_SPACING);
            mapBlocks.insert(make_pair(extended.nTimeStamp, make_pair(hashFork, extended)));
        }
    }
}

bool CBlockMaker::CreateDelegatedBlock(CBlock& block, const uint256& hashFork, const CBlockMakerProfile& profile)
{
    CDestination destSendTo = profile.GetDestination();

    int64 nReward;
    if (!pBlockChain->GetBlockMintReward(block.hashPrev, nReward))
    {
        Error("GetBlockMintReward error, hashPrev: %s", block.hashPrev.ToString().c_str());
        return false;
    }

    CTransaction& txMint = block.txMint;
    txMint.nType = CTransaction::TX_STAKE;
    txMint.nTimeStamp = block.nTimeStamp;
    txMint.hashAnchor = hashFork;
    txMint.sendTo = destSendTo;
    txMint.nAmount = nReward;
    txMint.nTxFee = 0;

    if (!ArrangeBlockTx(block, hashFork, profile))
    {
        Error("ArrangeBlockTx fail, hashPrev: %s", block.hashPrev.ToString().c_str());
        return false;
    }

    return SignBlock(block, profile);
}

void CBlockMaker::PreparePiggyback(CBlock& block, const CDelegateAgreement& agreement, const uint256& hashRefBlock,
                                   int64 nRefBlockTime, const int32 nPrevHeight, const CForkStatus& status, const uint16 nPrevMintType)
{
    CProofOfPiggyback proof;
    proof.nWeight = agreement.nWeight;
    proof.nAgreement = agreement.nAgreement;
    proof.hashRefBlock = hashRefBlock;

    block.nType = CBlock::BLOCK_SUBSIDIARY;
    block.nTimeStamp = nRefBlockTime;
    proof.Save(block.vchProof);
    /*if (status.nLastBlockHeight == nPrevHeight && status.nLastBlockTime < nRefBlockTime)
    {
        // last is PoW or last extended or timeouot
        if (nPrevMintType != CTransaction::TX_STAKE
            || status.nLastBlockTime + EXTENDED_BLOCK_SPACING == nRefBlockTime
            || GetNetTime() - nRefBlockTime >= WAIT_LAST_EXTENDED_TIME)
        {
            block.hashPrev = status.hashLastBlock;
        }
    }*/
}

void CBlockMaker::CreateExtended(CBlock& block, const CBlockMakerProfile& profile, const CDelegateAgreement& agreement,
                                 const uint256& hashRefBlock, const uint256& hashFork, const uint256& hashLastBlock, int64 nTime)
{
    CProofOfPiggyback proof;
    proof.nWeight = agreement.nWeight;
    proof.nAgreement = agreement.nAgreement;
    proof.hashRefBlock = hashRefBlock;

    block.nType = CBlock::BLOCK_EXTENDED;
    block.nTimeStamp = nTime;
    block.hashPrev = hashLastBlock;
    proof.Save(block.vchProof);

    CTransaction& txMint = block.txMint;
    txMint.nType = CTransaction::TX_STAKE;
    txMint.nTimeStamp = block.nTimeStamp;
    txMint.hashAnchor = hashFork;
    txMint.sendTo = profile.GetDestination();
    txMint.nAmount = 0;
    txMint.nTxFee = 0;
}

bool CBlockMaker::CreateVacant(CBlock& block, const CBlockMakerProfile& profile, const CDelegateAgreement& agreement,
                               const uint256& hashRefBlock, const uint256& hashFork, const uint256& hashLastBlock, int64 nTime)
{
    block.SetNull();

    /*if (!pCoreProtocol->IsRefVacantHeight(CBlock::GetBlockHeightByHash(hashLastBlock) + 1))
    {
        block.nType = CBlock::BLOCK_VACANT;
        block.hashPrev = hashLastBlock;
        block.nTimeStamp = nTime;
        return true;
    }*/

    CProofOfPiggyback proof;
    proof.nWeight = agreement.nWeight;
    proof.nAgreement = agreement.nAgreement;
    proof.hashRefBlock = hashRefBlock;

    block.nType = CBlock::BLOCK_VACANT;
    block.nTimeStamp = nTime;
    block.hashPrev = hashLastBlock;
    proof.Save(block.vchProof);

    CTransaction& txMint = block.txMint;
    txMint.nType = CTransaction::TX_STAKE;
    txMint.nTimeStamp = block.nTimeStamp;
    txMint.hashAnchor = hashFork;
    txMint.sendTo = profile.GetDestination();
    txMint.nAmount = 0;
    txMint.nTxFee = 0;

    return SignBlock(block, profile);
}

bool CBlockMaker::ReplenishSubForkVacant(const uint256& hashFork, int nLastBlockHeight, uint256& hashLastBlock, const CBlockMakerProfile& profile,
                                         const CDelegateAgreement& agreement, const uint256& hashRefBlock, const int32 nPrevHeight)
{
    vector<int64> vTime;
    int nPrimaryHeight = nPrevHeight + 1;
    int nNextHeight = nLastBlockHeight + 1;
    int nDepth = nPrimaryHeight - nLastBlockHeight;
    if (!pBlockChain->GetLastBlockTime(pCoreProtocol->GetGenesisBlockHash(), nDepth, vTime))
    {
        StdError("blockmaker", "Replenish vacant: GetLastBlockTime fail");
        return false;
    }
    while (nNextHeight < nPrimaryHeight)
    {
        CBlock block;
        if (!CreateVacant(block, profile, agreement, hashRefBlock, hashFork, hashLastBlock, vTime[nPrimaryHeight - nNextHeight]))
        {
            StdError("blockmaker", "Replenish vacant: create vacane fail");
            return false;
        }
        if (!DispatchBlock(hashFork, block))
        {
            StdError("blockmaker", "Replenish vacant: dispatch block fail");
            return false;
        }
        StdTrace("blockmaker", "Replenish vacant: height: %d, block: %s, fork: %s",
                 block.GetBlockHeight(), block.GetHash().GetHex().c_str(), hashFork.GetHex().c_str());
        hashLastBlock = block.GetHash();
        nNextHeight++;
    }
    return true;
}

bool CBlockMaker::CreateProofOfWork()
{
    int nConsensus = CM_CRYPTONIGHT;
    map<int, CBlockMakerProfile>::iterator it = mapWorkProfile.find(nConsensus);
    if (it == mapWorkProfile.end())
    {
        StdError("blockmaker", "did not find Work profile");
        return false;
    }
    CBlockMakerProfile& profile = (*it).second;
    CBlockMakerHashAlgo* pHashAlgo = mapHashAlgo[profile.nAlgo];
    if (pHashAlgo == nullptr)
    {
        StdError("blockmaker", "pHashAlgo is null");
        return false;
    }

    vector<unsigned char> vchWorkData;
    int nPrevBlockHeight = 0;
    uint256 hashPrev;
    uint32 nPrevTime = 0;
    int nAlgo = 0, nBits = 0;
    if (!pService->GetWork(vchWorkData, nPrevBlockHeight, hashPrev, nPrevTime, nAlgo, nBits, profile.templMint))
    {
        //StdTrace("blockmaker", "GetWork fail");
        return false;
    }

    uint32& nTime = *((uint32*)&vchWorkData[4]);
    uint64_t& nNonce = *((uint64_t*)&vchWorkData[vchWorkData.size() - sizeof(uint64_t)]);
    nNonce = (GetTime() % 0xFFFFFF) << 40;

    int64& nHashRate = pHashAlgo->nHashRate;
    int64 nHashComputeCount = 0;
    int64 nHashComputeBeginTime = GetTime();
    int64 nTimeDiff = (int64)nTime - GetNetTime();

    Log("Proof-of-work: start hash compute, target height: %d, difficulty bits: (%d)", nPrevBlockHeight + 1, nBits);

    uint256 hashTarget = (~uint256(uint64(0)) >> nBits);
    while (!InterruptedPoW(hashPrev))
    {
        if (nHashRate == 0)
        {
            nHashRate = 1;
        }
        for (int i = 0; i < nHashRate; i++)
        {
            uint256 hash = pHashAlgo->Hash(vchWorkData);
            nHashComputeCount++;
            if (hash <= hashTarget)
            {
                int64 nDuration = GetTime() - nHashComputeBeginTime;
                int nCompHashRate = ((nDuration <= 0) ? 0 : (nHashComputeCount / nDuration));

                Log("Proof-of-work: block found (%s), target height: %d, compute: (rate:%ld, count:%ld, duration:%lds, hashrate:%ld), difficulty bits: (%d)\nhash :   %s\ntarget : %s",
                    pHashAlgo->strAlgo.c_str(), nPrevBlockHeight + 1, nHashRate, nHashComputeCount, nDuration, nCompHashRate, nBits,
                    hash.GetHex().c_str(), hashTarget.GetHex().c_str());

                uint256 hashBlock;
                Errno err = pService->SubmitWork(vchWorkData, profile.templMint, profile.keyMint, hashBlock);
                if (err != OK)
                {
                    return false;
                }
                return true;
            }
            nNonce++;
        }

        int64 nNetTime = GetNetTime() + nTimeDiff;
        if (nTime + 1 < nNetTime)
        {
            nHashRate /= (nNetTime - nTime);
            nTime = nNetTime;
        }
        else if (nTime == nNetTime)
        {
            nHashRate *= 2;
        }
    }
    Log("Proof-of-work: target height: %d, compute interrupted.", nPrevBlockHeight + 1);
    return false;
}

void CBlockMaker::BlockMakerThreadFunc()
{
    uint256 hashLastBlock;
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        hashLastBlock = lastStatus.hashBlock;
    }
    bool fCachePow = false;
    int64 nWaitTime = 1;
    while (!fExit)
    {
        if (nWaitTime < 1)
        {
            nWaitTime = 1;
        }
        if (!WaitUpdateEvent(nWaitTime))
        {
            break;
        }

        switch (MintConfig()->nPeerType)
        {
        case NODE_TYPE_COMMON:
        case NODE_TYPE_SUPER:
        {
            CAgreementBlock consParam;
            if (!pConsensus->GetNextConsensus(consParam))
            {
                StdDebug("BlockMaker", "BlockMakerThreadFunc: GetNextConsensus fail, target height: %d, wait time: %ld, last height: %d, prev block: %s",
                         consParam.nPrevHeight + 1, consParam.nWaitTime, lastStatus.nBlockHeight, consParam.hashPrev.GetHex().c_str());
                nWaitTime = consParam.nWaitTime;
                continue;
            }
            // StdDebug("BlockMaker", "BlockMakerThreadFunc: GetNextConsensus success, target height: %d, wait time: %ld, last height: %d, prev block: %s",
            //          consParam.nPrevHeight + 1, consParam.nWaitTime, lastStatus.nLastBlockHeight, consParam.hashPrev.GetHex().c_str());
            nWaitTime = consParam.nWaitTime;

            if (hashLastBlock != consParam.hashPrev || fCachePow != consParam.agreement.IsProofOfWork())
            {
                hashLastBlock = consParam.hashPrev;
                fCachePow = consParam.agreement.IsProofOfWork();
                if (consParam.agreement.IsProofOfWork())
                {
                    Log("GetAgreement: height: %d, consensus: pow", consParam.nPrevHeight + 1);
                }
                else
                {
                    Log("GetAgreement: height: %d, consensus: dpos, ballot address: %s", consParam.nPrevHeight + 1, CAddress(consParam.agreement.vBallot[0]).ToString().c_str());
                }
            }

            try
            {
                if (!consParam.agreement.IsProofOfWork())
                {
                    ProcessDelegatedProofOfStake(consParam);
                }
                pDispatcher->SetConsensus(consParam);
            }
            catch (exception& e)
            {
                Error("Block maker error: %s", e.what());
                break;
            }
            break;
        }
        case NODE_TYPE_FORK:
        {
            uint256 hashPrevBlock;
            int64 nLastTime;
            // wait for new main fork block
            {
                boost::unique_lock<boost::mutex> lock(mutex);
                // no new block
                if (hashLastBlock == lastStatus.hashBlock)
                {
                    continue;
                }
                // too early block
                else if (lastStatus.nBlockTime < GetNetTime() - BLOCK_TARGET_SPACING)
                {
                    continue;
                }
                else
                {
                    hashLastBlock = lastStatus.hashBlock;
                    hashPrevBlock = lastStatus.hashPrevBlock;
                    nLastTime = lastStatus.nBlockTime;
                }
            }

            CDelegateAgreement agreement;
            if (pBlockChain->GetBlockDelegateAgreement(hashLastBlock, agreement)) // && !agreement.IsProofOfWork())
            {
                if (!agreement.IsProofOfWork())
                {
                    map<CDestination, CBlockMakerProfile>::iterator it = mapDelegatedProfile.find(agreement.vBallot[0]);
                    if (it != mapDelegatedProfile.end())
                    {
                        CBlockStatus status;
                        if (pBlockChain->GetBlockStatus(hashPrevBlock, status))
                        {
                            // create sub fork blocks
                            ProcessSubFork(it->second, agreement, hashLastBlock, nLastTime, status.nBlockHeight, status.nMintType);
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
    Log("Block maker exited");
}

void CBlockMaker::PowThreadFunc()
{
    if (!WaitExit(5))
    {
        Log("Pow exited non");
        return;
    }
    int64 nPrevBlockTime = 0;
    int64 nSynBeginTime = 0;
    while (WaitExit(1))
    {
        bool fWork = false;
        {
            boost::unique_lock<boost::mutex> lock(mutex);
            if (lastStatus.nBlockHeight == 0)
            {
                fWork = true;
            }
            else
            {
                int64 nCurTime = GetNetTime();
                if (nSynBeginTime == 0 || nPrevBlockTime != lastStatus.nBlockTime)
                {
                    nPrevBlockTime = lastStatus.nBlockTime;
                    nSynBeginTime = nCurTime;
                }
                int64 nStakeTime = 0;
                if (lastStatus.nMintType == CTransaction::TX_STAKE)
                {
                    nStakeTime = 10;
                }
                if (nCurTime >= lastStatus.nBlockTime + BLOCK_TARGET_SPACING + nStakeTime && nCurTime - nSynBeginTime >= BLOCK_TARGET_SPACING / 2)
                {
                    fWork = true;
                }
            }
        }
        if (fWork)
        {
            CreateProofOfWork();
        }
    }
    Log("Pow exited");
}

} // namespace ibrio
