// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_MODE_MODULE_TYPE_H
#define IBRIO_MODE_MODULE_TYPE_H

namespace ibrio
{
// module type
enum class EModuleType
{
    LOCK,             // lock file
    BLOCKMAKER,       // CBlockMaker
    COREPROTOCOL,     // CCoreProtocol
    DISPATCHER,       // CDispatcher
    HTTPGET,          // CHttpGet
    HTTPSERVER,       // CHttpServer
    MINER,            // CMiner
    NETCHANNEL,       // CNetChannel
    DELEGATEDCHANNEL, // CDelegatedChannel
    NETWORK,          // CNetwork
    RPCCLIENT,        // CRPCClient
    RPCMODE,          // CRPCMod
    SERVICE,          // CService
    TXPOOL,           // CTxPool
    WALLET,           // CWallet
    BLOCKCHAIN,       // CBlockChain
    CONSENSUS,        // CConsensus
    FORKMANAGER,      // CForkManager
    DATASTAT,         // CDataStat
    RECOVERY,         // CRecovery
};

} // namespace ibrio

#endif // IBRIO_MODE_MODULE_TYPE_H
