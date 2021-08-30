// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_DEFI_H
#define IBRIO_DEFI_H

#include <map>
#include <set>
#include <stack>
#include <type_traits>

#include "address.h"
#include "addressdb.h"
#include "profile.h"
#include "struct.h"
#include "xengine.h"

namespace ibrio
{

class CDeFiRelationRewardNode
{
public:
    CDeFiRelationRewardNode()
      : nPower(0), nAmount(0) {}
    CDeFiRelationRewardNode(const CDestination& parentIn)
      : parent(parentIn), nPower(0), nAmount(0) {}

public:
    CDestination parent;
    uint64 nPower;
    int64 nAmount;
};

typedef xengine::CForest<CDestination, CDeFiRelationRewardNode> CDeFiRelationGraph;

class CDeFiForkReward
{
public:
    enum
    {
        MAX_REWARD_CACHE = 5
    };

    typedef std::map<uint256, CDeFiRewardSet> MapSectionReward;
    struct CForkReward
    {
        MapSectionReward reward;
        CProfile profile;
    };
    typedef std::map<uint256, CForkReward> MapForkReward;

    // return exist fork or not
    bool ExistFork(const uint256& forkid) const;
    // return fork has been minted or not
    bool IsMinted(const uint256& forkid, const int32 nHeight) const;
    // add a fork and its profile
    void AddFork(const uint256& forkid, const CProfile& profile);
    // get a fork profile
    CProfile GetForkProfile(const uint256& forkid);
    // return the last height of previous reward cycle of nHeight
    int32 PrevRewardHeight(const uint256& forkid, const int32 nHeight);
    // return the reward of section. section range is [section_height - nRewardCycle + 1, section_height]
    int64 GetSectionReward(const uint256& forkid, const uint256& section, const int64 nSupply = -1, const int64 nInvalidSupply = 0);
    // return exist section cache of fork or not
    bool ExistForkSection(const uint256& forkid, const uint256& section);
    // return the section reward set. Should use ExistForkSection to determine if it exists first.
    CDeFiRewardSet& GetForkSection(const uint256& forkid, const uint256& section, bool& fIsNull);
    // Add a section reward set of fork
    void AddForkSection(const uint256& forkid, const uint256& hash, CDeFiRewardSet&& reward);

    // compute stake reward
    CDeFiRewardSet ComputeStakeReward(const int64 nMin, const int64 nReward,
                                      const std::map<CDestination, int64>& mapAddressAmount);
    // compute promotion reward
    CDeFiRewardSet ComputePromotionReward(const int64 nReward,
                                          const std::map<CDestination, int64>& mapAddressAmount,
                                          const std::map<int64, uint32>& mapPromotionTokenTimes,
                                          CDeFiRelationGraph& relation,
                                          const std::set<CDestination>& setBlackList);

    // for fixed decay coinbase, return the reward of between [nHeight, nHeight + nRewardCycle), nHeight must be a beginning of nRewardCycle
    int64 GetFixedDecayReward(const CProfile& profile, const int32 nHeight);
    // for specific decay coinbase, return the reward of between [nHeight, nHeight + nRewardCycle), nHeight must be a beginning of nRewardCycle
    int64 GetSpecificDecayReward(const CProfile& profile, const int32 nHeight);
    // for fixed decay coinbase, return the reward of between [nHeight, nHeight + nRewardCycle), nHeight must be a beginning of nRewardCycle
    // Use profile.defi.nSupplyCycle and profile.defi.nRewardCycle to compute increasing rate per nRewardCycle, and multiply nSupply to get reward.
    int64 GetFixedDecayRewardWithSupply(const CProfile& profile, const int32 nHeight, const int64 nSupply, const int64 nInvalidSupply = 0);
    // for specific decay coinbase, return the reward of between [nHeight, nHeight + nRewardCycle), nHeight must be a beginning of nRewardCycle
    // Use profile.defi.nSupplyCycle and profile.defi.nRewardCycle to compute increasing rate per nRewardCycle, and multiply nSupply to get reward.
    int64 GetSpecificDecayRewardWithSupply(const CProfile& profile, const int32 nHeight, const int64 nSupply, const int64 nInvalidSupply = 0);

protected:
    MapForkReward forkReward;
    static CDeFiRewardSet null;
};

} // namespace ibrio

#endif // IBRIO_DEFI_H