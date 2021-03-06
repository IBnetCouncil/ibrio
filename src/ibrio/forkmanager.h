// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_FORKMANAGER_H
#define IBRIO_FORKMANAGER_H

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <stack>

#include "base.h"
#include "peernet.h"

namespace ibrio
{

class CForkSchedule
{
public:
    CForkSchedule(bool fAllowedIn = false)
      : fAllowed(fAllowedIn)
    {
    }
    bool IsAllowed() const
    {
        return fAllowed;
    }
    bool IsJointEmpty() const
    {
        return mapJoint.empty();
    }
    void AddNewJoint(const uint256& hashJoint, const uint256& hashFork)
    {
        std::multimap<uint256, uint256>::iterator it = mapJoint.lower_bound(hashJoint);
        while (it != mapJoint.upper_bound(hashJoint))
        {
            if (it->second == hashFork)
            {
                return;
            }
            ++it;
        }
        mapJoint.insert(std::make_pair(hashJoint, hashFork));
    }
    void GetJointFork(const uint256& hashJoint, std::vector<uint256>& vFork)
    {
        std::multimap<uint256, uint256>::iterator it = mapJoint.lower_bound(hashJoint);
        while (it != mapJoint.upper_bound(hashJoint))
        {
            vFork.push_back((*it).second);
            ++it;
        }
    }

public:
    bool fAllowed;
    CForkContext ctxtFork;
    std::multimap<uint256, uint256> mapJoint;
};

class CForkManager : public IForkManager
{
public:
    CForkManager();
    ~CForkManager();
    bool IsAllowed(const uint256& hashFork) const override;
    bool GetJoint(const uint256& hashFork, uint256& hashParent, uint256& hashJoint, int& nHeight) const override;
    bool LoadForkContext(const uint256& hashPrimaryLastBlockIn, const std::vector<CForkContext>& vForkCtxt,
                         const std::map<uint256, CValidForkId>& mapValidForkId, std::vector<uint256>& vActive) override;
    void SetPrimaryLastBlock(const uint256& hashPrimaryLastBlockIn) override;
    bool VerifyFork(const uint256& hashPrevBlock, const uint256& hashFork, const std::string& strForkName) override;
    bool AddValidForkContext(const uint256& hashPrevBlock, const uint256& hashNewBlock, const std::vector<std::pair<CDestination, CForkContext>>& vForkCtxt,
                             bool fCheckPointBlock, uint256& hashRefFdBlock, std::map<uint256, int>& mapValidFork) override;
    void RemoveValidForkContext(const uint256& hashBlock) override;
    void ForkUpdate(const CBlockChainUpdate& update, std::vector<uint256>& vActive, std::vector<uint256>& vDeactive) override;
    void GetValidForkList(std::map<uint256, bool>& mapFork) override;
    bool GetSubline(const uint256& hashFork, std::vector<std::pair<int, uint256>>& vSubline) const override;
    int64 ForkLockedCoin(const uint256& hashFork, const uint256& hashBlock) override;
    int GetForkCreatedHeight(const uint256& hashFork) override;
    bool GetForkContext(const uint256& hashFork, CForkContext& forkContext) override;

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;
    void HandleHalt() override;
    bool IsAllowedFork(const uint256& hashFork, const uint256& hashParent) const;
    int GetValidForkCreatedHeight(const uint256& hashBlock, const uint256& hashFork);
    bool AddDbForkContext(const CForkContext& ctxt, std::vector<uint256>& vActive);
    bool GetValidFdForkId(const uint256& hashBlock, std::map<uint256, int>& mapFdForkIdOut);
    void ListValidFork(std::map<uint256, bool>& mapFork);

protected:
    mutable boost::shared_mutex rwAccess;
    ICoreProtocol* pCoreProtocol;
    IBlockChain* pBlockChain;
    bool fAllowAnyFork;
    uint256 hashPrimaryLastBlock;
    std::set<uint256> setForkAllowed;
    std::set<uint256> setGroupAllowed;
    std::set<uint256> setForkExcluded;
    std::map<uint256, CForkSchedule> mapForkSched;
    std::map<uint256, CValidForkId> mapBlockValidFork;
    std::set<uint256> setCurValidFork;
};

} // namespace ibrio

#endif //IBRIO_FORKMANAGER_H
