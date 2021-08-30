// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_ENTRY_H
#define IBRIO_ENTRY_H

#include <boost/filesystem.hpp>
#include <xengine.h>

#include "config.h"

namespace ibrio
{

class CBbEntry : public xengine::CEntry
{
public:
    static CBbEntry& GetInstance();

public:
    ~CBbEntry();
    bool Initialize(int argc, char* argv[]);
    bool Run() override;
    void Exit();

protected:
    bool InitializeModules(const EModeType& mode);
    bool AttachModule(xengine::IBase* pBase);

    xengine::CHttpHostConfig GetRPCHostConfig();

    void PurgeStorage();

    boost::filesystem::path GetDefaultDataDir();

    bool SetupEnvironment();
    bool RunInBackground(const boost::filesystem::path& pathData);
    void ExitBackground(const boost::filesystem::path& pathData);

protected:
    CConfig config;
    xengine::CLog log;
    xengine::CDocker docker;

private:
    CBbEntry();
};

} // namespace ibrio

#endif //IBRIO_ENTRY_H
