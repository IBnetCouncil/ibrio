// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_RECOVERY_H
#define IBRIO_RECOVERY_H

#include "base.h"

namespace ibrio
{

class CRecovery : public IRecovery
{
public:
    CRecovery();
    ~CRecovery();

protected:
    bool HandleInitialize() override;
    void HandleDeinitialize() override;
    bool HandleInvoke() override;

protected:
    IDispatcher* pDispatcher;
};

} // namespace ibrio
#endif // IBRIO_RECOVERY_H
