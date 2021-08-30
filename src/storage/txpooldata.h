// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_TXPOOLDATA_H
#define STORAGE_TXPOOLDATA_H

#include <boost/filesystem.hpp>

#include "transaction.h"
#include "xengine.h"

namespace ibrio
{
namespace storage
{

class CTxPoolData
{
public:
    CTxPoolData();
    ~CTxPoolData();
    bool Initialize(const boost::filesystem::path& pathData);
    bool Remove();
    bool Save(const std::vector<std::pair<uint256, std::pair<uint256, CAssembledTx>>>& vTx);
    bool Load(std::vector<std::pair<uint256, std::pair<uint256, CAssembledTx>>>& vTx);
    bool LoadCheck(std::vector<std::pair<uint256, std::pair<uint256, CAssembledTx>>>& vTx);

protected:
    boost::filesystem::path pathTxPoolFile;
};

} // namespace storage
} // namespace ibrio

#endif //STORAGE_TXPOOLDATA_H
