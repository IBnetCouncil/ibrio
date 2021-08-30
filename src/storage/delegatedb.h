// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_DELEGATEDB_H
#define STORAGE_DELEGATEDB_H

#include <map>

#include "destination.h"
#include "timeseries.h"
#include "uint256.h"
#include "xengine.h"

namespace ibrio
{
namespace storage
{

class CDelegateContext
{
    friend class xengine::CStream;

public:
    std::map<CDestination, int64> mapVote;
    std::map<int, std::map<CDestination, CDiskPos>> mapEnrollTx;
    std::map<CDestination, std::string> mapDelegateName;

protected:
    template <typename O>
    void Serialize(xengine::CStream& s, O& opt)
    {
        s.Serialize(mapVote, opt);
        s.Serialize(mapEnrollTx, opt);
        s.Serialize(mapDelegateName, opt);
    }
};

class CDelegateDB : public xengine::CKVDB
{
public:
    CDelegateDB()
      : cacheDelegate(MAX_CACHE_COUNT) {}
    bool Initialize(const boost::filesystem::path& pathData);
    void Deinitialize();
    void Clear();
    bool AddNew(const uint256& hashBlock, const CDelegateContext& ctxtDelegate);
    bool Remove(const uint256& hashBlock);
    bool Retrieve(const uint256& hashBlock, CDelegateContext& ctxtDelegate);
    bool RetrieveDelegatedVote(const uint256& hashBlock, std::map<CDestination, int64>& mapVote);
    bool RetrieveDelegatedEnrollTx(const uint256& hashBlock, std::map<int, std::map<CDestination, CDiskPos>>& mapEnrollTxPos);
    bool RetrieveEnrollTx(int height, const std::vector<uint256>& vBlockRange, std::map<CDestination, CDiskPos>& mapEnrollTxPos);

protected:
    enum
    {
        MAX_CACHE_COUNT = 64,
    };
    xengine::CCache<uint256, CDelegateContext> cacheDelegate;
};

} // namespace storage
} // namespace ibrio

#endif //STORAGE_DELEGATEDB_H
