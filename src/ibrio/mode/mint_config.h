// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_MODE_MINT_CONFIG_H
#define IBRIO_MODE_MINT_CONFIG_H

#include <string>

#include "destination.h"
#include "mode/basic_config.h"
#include "uint256.h"

namespace ibrio
{

enum
{
    NODE_TYPE_COMMON,
    NODE_TYPE_SUPER,
    NODE_TYPE_FORK
};

class CMintConfig : virtual public CBasicConfig, virtual public CMintConfigOption
{
public:
    CMintConfig();
    virtual ~CMintConfig();
    virtual bool PostLoad();
    virtual std::string ListConfig() const;
    virtual std::string Help() const;

protected:
    void ExtractMintParamPair(const std::string& strAddress,
                              const std::string& strKey, CDestination& dest,
                              uint256& privkey);

public:
    CDestination destMpvss;
    uint256 keyMpvss;
    CDestination destCryptonight;
    uint256 keyCryptonight;
    int nPeerType;
};

} // namespace ibrio

#endif // IBRIO_MODE_MINT_CONFIG_H
