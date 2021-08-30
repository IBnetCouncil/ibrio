// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_FORKDB_H
#define STORAGE_FORKDB_H

#include <map>

#include "forkcontext.h"
#include "uint256.h"
#include "xengine.h"

namespace ibrio
{
namespace storage
{

class CForkDB : public xengine::CKVDB
{
public:
    CForkDB() {}
    bool Initialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn);
    void Deinitialize();
    bool AddNewForkContext(const CForkContext& ctxt);
    bool RemoveForkContext(const uint256& hashFork);
    bool RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt);
    bool ListForkContext(std::vector<CForkContext>& vForkCtxt, std::map<uint256, CValidForkId>& mapValidForkId);
    bool UpdateFork(const uint256& hashFork, const uint256& hashLastBlock = uint256());
    bool RemoveFork(const uint256& hashFork);
    bool RetrieveFork(const uint256& hashFork, uint256& hashLastBlock);
    bool ListFork(std::vector<std::pair<uint256, uint256>>& vFork);
    bool AddValidForkHash(const uint256& hashBlock, const uint256& hashRefFdBlock, const std::map<uint256, int>& mapValidFork);
    bool RetrieveValidForkHash(const uint256& hashBlock, uint256& hashRefFdBlock, std::map<uint256, int>& mapValidFork);
    bool ListActiveFork(std::map<uint256, uint256>& mapActiveFork);
    void Clear();

protected:
    bool LoadCtxtWalker(xengine::CBufStream& ssKey, xengine::CBufStream& ssValue, std::vector<CForkContext>& vForkCtxt);
    bool LoadActiveForkWalker(xengine::CBufStream& ssKey, xengine::CBufStream& ssValue, std::map<uint256, uint256>& mapFork);
    bool LoadValidForkWalker(xengine::CBufStream& ssKey, xengine::CBufStream& ssValue, std::map<uint256, CValidForkId>& mapBlockForkId);

protected:
    uint256 hashGenesisBlock;
};

} // namespace storage
} // namespace ibrio

#endif //STORAGE_FORKDB_H
