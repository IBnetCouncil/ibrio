// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STORAGE_PURGER_H
#define STORAGE_PURGER_H

#include <boost/filesystem.hpp>

#include "uint256.h"

namespace ibrio
{
namespace storage
{

class CPurger
{
public:
    bool ResetDB(const boost::filesystem::path& pathDataLocation, const uint256& hashGenesisBlockIn) const;
    bool RemoveBlockFile(const boost::filesystem::path& pathDataLocation) const;
    bool operator()(const boost::filesystem::path& pathDataLocation, const uint256& hashGenesisBlockIn) const
    {
        return (ResetDB(pathDataLocation, hashGenesisBlockIn) && RemoveBlockFile(pathDataLocation));
    }
};

} // namespace storage
} // namespace ibrio

#endif //STORAGE_PURGER_H
