// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_MODE_NETWORK_CONFIG_H
#define IBRIO_MODE_NETWORK_CONFIG_H

#include <string>
#include <vector>

#include "mode/basic_config.h"

namespace ibrio
{
class CNetworkConfig : virtual public CBasicConfig, virtual public CNetworkConfigOption
{
public:
    CNetworkConfig();
    virtual ~CNetworkConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

public:
    unsigned short nPort;
    unsigned int nMaxInBounds;
    unsigned int nMaxOutBounds;
};

} // namespace ibrio

#endif // IBRIO_MODE_NETWORK_CONFIG_H
