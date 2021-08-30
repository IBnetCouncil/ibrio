// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_INVESTDB_H
#define STORAGE_INVESTDB_H

#include <map>

#include "destination.h"
#include "timeseries.h"
#include "transaction.h"
#include "uint256.h"
#include "xengine.h"

namespace ibrio
{
namespace storage
{

class CInvestDB : public xengine::CKVDB
{
public:
    CInvestDB(const bool fCacheIn = true)
      : fCache(fCacheIn) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool AddNew(const uint256& hashBlock, const CInvestContext& ctxtInvest);
    bool Remove(const uint256& hashBlock);
    bool Retrieve(const uint256& hashBlock, CInvestContext& ctxtInvest);
    bool AddBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, const std::vector<std::pair<CDestination, int64>>& vNoPackingRewardIn);
    bool GetFullInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtFullInvestOut);
    bool GetBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtInvest);
    bool RetrieveBlockInvest(const uint256& hashBlock, CInvestContext& ctxtInvest);
    void Clear();

protected:
    bool IsFullInvest(const uint256& hashBlock);
    bool GetFullBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtInvest);
    bool GetIncrementBlockInvest(const uint256& hashBlock, const uint256& hashPrev, const std::vector<std::pair<CDestination, CInvest>>& vTxInvestIn, CInvestContext& ctxtInvest);

    bool GetBlockFullInvest(const uint256& hashPrev, CInvestContext& ctxtInvest);
    bool AddInvest(const uint256& hashBlock, const CInvestContext& ctxtInvest);
    bool RemoveInvest(const uint256& hashBlock);
    bool GetInvest(const uint256& hashBlock, CInvestContext& ctxtInvest);
    bool GetInvest(const uint256& hashBlock, std::map<CDestination, CInvest>& mapInvestOut);
    void AddCache(const uint256& hashBlock, const CInvestContext& ctxtInvest);

protected:
    enum
    {
        FULL_HEIGHT = 720, //2880,
        MAX_FULL_CACHE_COUNT = 16,
        MAX_INC_CACHE_COUNT = 860 //2880 * 2
    };
    bool fCache;
    xengine::CRWAccess rwData;
    std::map<uint256, CInvestContext> mapCacheFullInvest;
    std::map<uint256, CInvestContext> mapCacheIncInvest;
};

} // namespace storage
} // namespace ibrio

#endif //STORAGE_INVESTDB_H
