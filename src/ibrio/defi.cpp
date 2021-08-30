// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "defi.h"

#include "param.h"

using namespace std;
using namespace xengine;
using namespace ibrio::storage;

namespace ibrio
{

//////////////////////////////
// CDeFiForkReward
CDeFiRewardSet CDeFiForkReward::null;

bool CDeFiForkReward::ExistFork(const uint256& forkid) const
{
    return forkReward.count(forkid);
}

bool CDeFiForkReward::IsMinted(const uint256& forkid, const int32 nHeight) const
{
    auto it = forkReward.find(forkid);
    if (it != forkReward.end())
    {
        const CProfile& profile = it->second.profile;
        if (profile.defi.nMintHeight < 0)
        {
            return false;
        }
        else if (profile.defi.nMintHeight == 0)
        {
            return nHeight >= profile.nJointHeight + 2;
        }
        else
        {
            return nHeight >= profile.defi.nMintHeight;
        }
    }

    return false;
}

void CDeFiForkReward::AddFork(const uint256& forkid, const CProfile& profile)
{
    CForkReward fr;
    fr.profile = profile;
    forkReward[forkid] = fr;
}

CProfile CDeFiForkReward::GetForkProfile(const uint256& forkid)
{
    auto it = forkReward.find(forkid);
    return (it == forkReward.end()) ? CProfile() : it->second.profile;
}

int32 CDeFiForkReward::PrevRewardHeight(const uint256& forkid, const int32 nHeight)
{
    auto it = forkReward.find(forkid);
    if (it != forkReward.end())
    {
        CProfile& profile = it->second.profile;
        if (!profile.defi.IsNull() && profile.defi.nMintHeight >= 0)
        {
            int32 nMintHeight = (profile.defi.nMintHeight == 0) ? (profile.nJointHeight + 2) : profile.defi.nMintHeight;
            int32 nRewardCycle = profile.defi.nRewardCycle;
            if (nHeight >= nMintHeight && nRewardCycle > 0)
            {
                return ((nHeight - nMintHeight) / nRewardCycle) * nRewardCycle + nMintHeight - 1;
            }
        }
    }
    return -1;
}

int64 CDeFiForkReward::GetSectionReward(const uint256& forkid, const uint256& section, const int64 nSupply, const int64 nInvalidSupply)
{
    CProfile profile = GetForkProfile(forkid);
    if (profile.IsNull())
    {
        return -1;
    }

    if (profile.defi.nMintHeight < 0)
    {
        return -1;
    }
    int32 nMintHeight = (profile.defi.nMintHeight == 0) ? (profile.nJointHeight + 2) : profile.defi.nMintHeight;

    // begin height
    int32 nSectionHeight = CBlock::GetBlockHeightByHash(section);
    int32 nHeight = PrevRewardHeight(forkid, nSectionHeight) + 1;
    if (nHeight < nMintHeight || (nHeight + profile.defi.nRewardCycle - 1 != nSectionHeight))
    {
        return 0;
    }

    if (profile.defi.nCoinbaseType == FIXED_DEFI_COINBASE_TYPE)
    {
        if (nSupply < 0)
        {
            return GetFixedDecayReward(profile, nHeight);
        }
        else
        {
            return GetFixedDecayRewardWithSupply(profile, nHeight, nSupply, nInvalidSupply);
        }
    }
    else if (profile.defi.nCoinbaseType == SPECIFIC_DEFI_COINBASE_TYPE)
    {
        if (nSupply < 0)
        {
            return GetSpecificDecayReward(profile, nHeight);
        }
        else
        {
            return GetSpecificDecayRewardWithSupply(profile, nHeight, nSupply, nInvalidSupply);
        }
    }
    else
    {
        return 0;
    }
}

bool CDeFiForkReward::ExistForkSection(const uint256& forkid, const uint256& section)
{
    auto it = forkReward.find(forkid);
    if (it != forkReward.end())
    {
        return it->second.reward.count(section);
    }
    return false;
}

CDeFiRewardSet& CDeFiForkReward::GetForkSection(const uint256& forkid, const uint256& section, bool& fIsNull)
{
    auto it = forkReward.find(forkid);
    if (it != forkReward.end())
    {
        auto im = it->second.reward.find(section);
        if (im != it->second.reward.end())
        {
            fIsNull = false;
            return im->second;
        }
    }
    fIsNull = true;
    return null;
}

void CDeFiForkReward::AddForkSection(const uint256& forkid, const uint256& hash, CDeFiRewardSet&& reward)
{
    auto it = forkReward.find(forkid);
    if (it != forkReward.end())
    {
        auto& mapReward = it->second.reward;
        mapReward[hash] = std::move(reward);

        while (mapReward.size() > MAX_REWARD_CACHE)
        {
            if (mapReward.begin()->first != hash)
            {
                mapReward.erase(mapReward.begin());
            }
            else
            {
                break;
            }
        }
    }
}

CDeFiRewardSet CDeFiForkReward::ComputeStakeReward(const int64 nMin, const int64 nReward,
                                                   const std::map<CDestination, int64>& mapAddressAmount)
{
    CDeFiRewardSet rewardSet;

    if (nReward == 0)
    {
        return rewardSet;
    }

    // sort by token
    multimap<int64, pair<CDestination, uint64>> mapRank;
    for (auto& p : mapAddressAmount)
    {
        if (p.second >= nMin)
        {
            mapRank.insert(make_pair(p.second, make_pair(p.first, 0)));
        }
    }

    // tag rank
    uint64 nRank = 1;
    uint64 nPos = 0;
    uint64 nTotal = 0;
    int64 nToken = -1;
    for (auto& p : mapRank)
    {
        ++nPos;
        if (p.first != nToken)
        {
            p.second.second = nPos;
            nRank = nPos;
            nToken = p.first;
        }
        else
        {
            p.second.second = nRank;
        }

        nTotal += p.second.second;
    }

    // reward
    double fUnitReward = (double)nReward / nTotal;
    for (auto& p : mapRank)
    {
        CDeFiReward reward;
        reward.dest = p.second.first;
        reward.nAmount = p.first;
        reward.nRank = p.second.second;
        reward.nStakeReward = fUnitReward * p.second.second;
        reward.nReward = reward.nStakeReward;
        rewardSet.insert(std::move(reward));
    }

    return rewardSet;
}

CDeFiRewardSet CDeFiForkReward::ComputePromotionReward(const int64 nReward,
                                                       const map<CDestination, int64>& mapAddressAmount,
                                                       const std::map<int64, uint32>& mapPromotionTokenTimes,
                                                       CDeFiRelationGraph& relation,
                                                       const std::set<CDestination>& setBlackList)
{
    typedef typename CForest<CDestination, CDeFiRelationRewardNode>::NodePtr NodePtr;

    CDeFiRewardSet rewardSet;

    if (nReward == 0)
    {
        return rewardSet;
    }

    // compute promotion power
    multimap<uint64, tuple<CDestination, int64, int64>> mapPower;
    uint64 nTotal = 0;
    relation.PostorderTraversal([&](NodePtr pNode)
                                {
                                    // blacklist
                                    if (setBlackList.count(pNode->key))
                                    {
                                        pNode->data.nPower = 0;
                                        pNode->data.nAmount = 0;
                                        return true;
                                    }

                                    // amount
                                    auto it = mapAddressAmount.find(pNode->key);
                                    int64 nAmount = (it == mapAddressAmount.end()) ? 0 : (it->second / COIN);

                                    // power
                                    pNode->data.nPower = 0;
                                    pNode->data.nAmount = nAmount;
                                    if (!pNode->setChildren.empty())
                                    {
                                        int64 nMax = -1;
                                        for (auto& p : pNode->setChildren)
                                        {
                                            pNode->data.nAmount += p->data.nAmount;
                                            int64 n = 0;
                                            if (p->data.nAmount <= nMax)
                                            {
                                                n = p->data.nAmount;
                                            }
                                            else
                                            {
                                                n = nMax;
                                                nMax = p->data.nAmount;
                                            }

                                            if (n < 0)
                                            {
                                                continue;
                                            }

                                            uint64 nLastToken = 0;
                                            uint64 nChildPower = 0;
                                            for (auto& tokenTimes : mapPromotionTokenTimes)
                                            {
                                                if (n > tokenTimes.first)
                                                {
                                                    nChildPower += (tokenTimes.first - nLastToken) * tokenTimes.second;
                                                    nLastToken = tokenTimes.first;
                                                }
                                                else
                                                {
                                                    nChildPower += (n - nLastToken) * tokenTimes.second;
                                                    nLastToken = n;
                                                    break;
                                                }
                                            }
                                            nChildPower += (n - nLastToken);
                                            pNode->data.nPower += nChildPower;
                                        }
                                        pNode->data.nPower += llround(pow(nMax, 1.0 / 3));
                                    }

                                    if (pNode->data.nPower > 0)
                                    {
                                        nTotal += pNode->data.nPower;
                                        mapPower.insert(make_pair(pNode->data.nPower, make_tuple(pNode->key, nAmount, pNode->data.nAmount)));
                                    }

                                    return true;
                                });

    // reward
    if (nTotal > 0)
    {
        double fUnitReward = (double)nReward / nTotal;
        for (auto& p : mapPower)
        {
            CDeFiReward reward;
            reward.dest = get<0>(p.second);
            reward.nAmount = get<1>(p.second);
            reward.nAchievement = get<2>(p.second);
            reward.nPower = p.first;
            reward.nPromotionReward = fUnitReward * p.first;
            reward.nReward = reward.nPromotionReward;
            rewardSet.insert(std::move(reward));
        }
    }

    return rewardSet;
}

int64 CDeFiForkReward::GetFixedDecayReward(const CProfile& profile, const int32 nHeight)
{
    if (profile.defi.nMintHeight < 0)
    {
        return -1;
    }
    int32 nMintHeight = (profile.defi.nMintHeight == 0) ? (profile.nJointHeight + 2) : profile.defi.nMintHeight;
    if (nHeight < nMintHeight)
    {
        return -1;
    }

    int64 nMaxSupply = (profile.defi.nMaxSupply < 0) ? MAX_MONEY : profile.defi.nMaxSupply;
    int32 nDecayCycle = profile.defi.nDecayCycle;
    int32 nSupplyCycle = profile.defi.nSupplyCycle;
    int32 nRewardCycle = profile.defi.nRewardCycle;
    uint8 nCoinbaseDecayPercent = profile.defi.nCoinbaseDecayPercent;
    uint32 nInitCoinbasePercent = profile.defi.nInitCoinbasePercent;
    int32 nSupplyCount = (nDecayCycle <= 0) ? 0 : (nDecayCycle / nSupplyCycle);

    // for example:
    // [2] nJoint height
    // [3] origin height
    // [4, 5, 6] the first supply cycle of the first decay cycle
    // [7, 8, 9] the second sypply cycle of the first decay cycle
    // [10, 11, 12] the first supply cycle of the second decay cycle
    // [13, 14, 15] the second supply cycle of the second decay cycle
    int32 nDecayCount = (nDecayCycle <= 0) ? 0 : ((nHeight - nMintHeight) / nDecayCycle);
    int32 nDecayHeight = nDecayCount * nDecayCycle + nMintHeight;
    int32 nCurSupplyCount = (nHeight - nDecayHeight) / nSupplyCycle;
    int64 nSupply = profile.nAmount;
    double fCoinbaseIncreasing = (double)nInitCoinbasePercent / 100;
    for (int i = 0; i <= nDecayCount; i++)
    {
        if (i < nDecayCount)
        {
            for (int j = 0; j < nSupplyCount; j++)
            {
                nSupply *= 1 + fCoinbaseIncreasing;
            }
            fCoinbaseIncreasing = fCoinbaseIncreasing * nCoinbaseDecayPercent / 100;
        }
        else
        {
            for (int i = 0; i < nCurSupplyCount; i++)
            {
                nSupply *= 1 + fCoinbaseIncreasing;
            }
        }
    }

    if (nSupply >= nMaxSupply)
    {
        return 0;
    }

    int64 nNextSupply = nSupply * (1 + fCoinbaseIncreasing);
    if (nNextSupply < nSupply + COIN)
    {
        return 0;
    }
    double fCoinbase = (double)(nNextSupply - nSupply) / nSupplyCycle;

    int32 nSupplyHeight = nDecayHeight + nCurSupplyCount * nSupplyCycle;
    int64 nSupplyReward = (int64)llround(fCoinbase * (nHeight - nSupplyHeight));
    int64 nNextReward = (int64)llround(fCoinbase * nRewardCycle);
    if (nMaxSupply >= nSupply + nSupplyReward + nNextReward)
    {
        return nNextReward;
    }
    else
    {
        int64 nMaxReward = nMaxSupply - nSupply - nSupplyReward;
        return max((int64)0, nMaxReward);
    }
}

int64 CDeFiForkReward::GetSpecificDecayReward(const CProfile& profile, const int32 nHeight)
{
    if (profile.defi.nMintHeight < 0)
    {
        return -1;
    }
    int32 nMintHeight = (profile.defi.nMintHeight == 0) ? (profile.nJointHeight + 2) : profile.defi.nMintHeight;
    if (nHeight < nMintHeight)
    {
        return -1;
    }

    int64 nMaxSupply = (profile.defi.nMaxSupply < 0) ? MAX_MONEY : profile.defi.nMaxSupply;
    int32 nSupplyCycle = profile.defi.nSupplyCycle;
    int32 nRewardCycle = profile.defi.nRewardCycle;
    const map<int32, uint32>& mapCoinbasePercent = profile.defi.mapCoinbasePercent;

    // for example:
    // [2] nJoint height
    // [3] origin height
    // [4, 5] the first supply cycle of the first decay cycle
    // [6, 7] the second sypply cycle of the first decay cycle
    // [8, 9] the third sypply cycle of the first decay cycle
    // [10, 11] the first supply cycle of the second decay cycle
    // [12, 13] the second supply cycle of the second decay cycle
    // [14, 15] the first supply cycle of the third decay cycle
    int64 nSupply = profile.nAmount;
    int32 nLastDecayHeight = 0;
    for (auto it = mapCoinbasePercent.begin(); it != mapCoinbasePercent.end(); it++)
    {
        double fCoinbaseIncreasing = (double)it->second / 100;
        if (nHeight - nMintHeight + 1 > it->first)
        {
            int32 nSupplyCount = (it->first - nLastDecayHeight) / nSupplyCycle;
            for (int j = 0; j < nSupplyCount; j++)
            {
                nSupply *= 1 + fCoinbaseIncreasing;
            }
        }
        else
        {
            int32 nCurSupplyCount = (nHeight - nMintHeight - nLastDecayHeight) / nSupplyCycle;
            for (int j = 0; j < nCurSupplyCount; j++)
            {
                nSupply *= 1 + fCoinbaseIncreasing;
            }

            if (nSupply >= nMaxSupply)
            {
                return 0;
            }

            int64 nNextSupply = nSupply * (1 + fCoinbaseIncreasing);
            if (nNextSupply < nSupply + COIN)
            {
                return 0;
            }
            double fCoinbase = (double)(nNextSupply - nSupply) / nSupplyCycle;

            int32 nSupplyHeight = nMintHeight + nLastDecayHeight + nCurSupplyCount * nSupplyCycle;
            int64 nSupplyReward = (int64)llround(fCoinbase * (nHeight - nSupplyHeight));
            int64 nNextReward = (int64)llround(fCoinbase * nRewardCycle);
            if (nMaxSupply >= nSupply + nSupplyReward + nNextReward)
            {
                return nNextReward;
            }
            else
            {
                int64 nMaxReward = nMaxSupply - nSupply - nSupplyReward;
                return max((int64)0, nMaxReward);
            }
        }

        nLastDecayHeight = it->first;
    }

    return 0;
}

int64 CDeFiForkReward::GetFixedDecayRewardWithSupply(const CProfile& profile, const int32 nHeight, const int64 nSupply, const int64 nInvalidSupply)
{
    if (profile.defi.nMintHeight < 0)
    {
        return -1;
    }
    int32 nMintHeight = (profile.defi.nMintHeight == 0) ? (profile.nJointHeight + 2) : profile.defi.nMintHeight;
    if (nHeight < nMintHeight)
    {
        return -1;
    }

    int64 nMaxSupply = (profile.defi.nMaxSupply < 0) ? MAX_MONEY : profile.defi.nMaxSupply;
    if (nSupply >= nMaxSupply)
    {
        return 0;
    }

    int32 nDecayCycle = profile.defi.nDecayCycle;
    int32 nSupplyCycle = profile.defi.nSupplyCycle;
    int32 nRewardCycle = profile.defi.nRewardCycle;
    int32 nRewardCount = nSupplyCycle / nRewardCycle;
    uint8 nCoinbaseDecayPercent = profile.defi.nCoinbaseDecayPercent;
    uint32 nInitCoinbasePercent = profile.defi.nInitCoinbasePercent;

    // for example:
    // [2] nJoint height
    // [3] origin height
    // [4, 5, 6] the first supply cycle of the first decay cycle
    // [7, 8, 9] the second sypply cycle of the first decay cycle
    // [10, 11, 12] the first supply cycle of the second decay cycle
    // [13, 14, 15] the second supply cycle of the second decay cycle
    int32 nDecayCount = (nDecayCycle <= 0) ? 0 : ((nHeight - nMintHeight) / nDecayCycle);
    double fCoinbaseIncreasing = (double)nInitCoinbasePercent / 100;
    for (int i = 0; i < nDecayCount; i++)
    {
        fCoinbaseIncreasing = fCoinbaseIncreasing * nCoinbaseDecayPercent / 100;
    }

    double fRewardIncreasing = pow(1 + fCoinbaseIncreasing, (double)1 / nRewardCount) - 1;
    int64 nNextReward = (nSupply - nInvalidSupply) * fRewardIncreasing;
    if (nNextReward < COIN)
    {
        return 0;
    }

    return min(nNextReward, nMaxSupply - nSupply);
}

int64 CDeFiForkReward::GetSpecificDecayRewardWithSupply(const CProfile& profile, const int32 nHeight, const int64 nSupply, const int64 nInvalidSupply)
{
    if (profile.defi.nMintHeight < 0)
    {
        return -1;
    }
    int32 nMintHeight = (profile.defi.nMintHeight == 0) ? (profile.nJointHeight + 2) : profile.defi.nMintHeight;
    if (nHeight < nMintHeight)
    {
        return -1;
    }

    int64 nMaxSupply = (profile.defi.nMaxSupply < 0) ? MAX_MONEY : profile.defi.nMaxSupply;
    if (nSupply >= nMaxSupply)
    {
        return 0;
    }

    int32 nSupplyCycle = profile.defi.nSupplyCycle;
    int32 nRewardCycle = profile.defi.nRewardCycle;
    int32 nRewardCount = nSupplyCycle / nRewardCycle;
    const map<int32, uint32>& mapCoinbasePercent = profile.defi.mapCoinbasePercent;

    // for example:
    // [2] nJoint height
    // [3] origin height
    // [4, 5] the first supply cycle of the first decay cycle
    // [6, 7] the second sypply cycle of the first decay cycle
    // [8, 9] the third sypply cycle of the first decay cycle
    // [10, 11] the first supply cycle of the second decay cycle
    // [12, 13] the second supply cycle of the second decay cycle
    // [14, 15] the first supply cycle of the third decay cycle
    for (auto it = mapCoinbasePercent.begin(); it != mapCoinbasePercent.end(); it++)
    {
        double fCoinbaseIncreasing = (double)it->second / 100;
        if (nHeight - nMintHeight + 1 <= it->first)
        {
            double fRewardIncreasing = pow(1 + fCoinbaseIncreasing, (double)1 / nRewardCount) - 1;
            int64 nNextReward = (nSupply - nInvalidSupply) * fRewardIncreasing;
            if (nNextReward < COIN)
            {
                return 0;
            }

            return min(nNextReward, nMaxSupply - nSupply);
        }
    }

    return 0;
}

} // namespace ibrio