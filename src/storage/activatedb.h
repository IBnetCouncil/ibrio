// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_ACTIVATEDB_H
#define STORAGE_ACTIVATEDB_H

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

class CActivateDB : public xengine::CKVDB
{
public:
    CActivateDB(const bool fCacheIn = true)
      : fCache(fCacheIn), cacheActivate(MAX_CACHE_COUNT) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    bool AddNew(const uint256& hashBlock, const CActivateContext& ctxtActivate);
    bool Remove(const uint256& hashBlock);
    bool Retrieve(const uint256& hashBlock, CActivateContext& ctxtActivate);
    void Clear();

protected:
    enum
    {
        MAX_CACHE_COUNT = 64,
    };
    xengine::CCache<uint256, CActivateContext> cacheActivate;
    bool fCache;
};

} // namespace storage
} // namespace ibrio

#endif // STORAGE_ACTIVATEDB_H
