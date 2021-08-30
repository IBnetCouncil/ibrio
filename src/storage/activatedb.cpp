// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activatedb.h"

#include <boost/range/adaptor/reversed.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace xengine;

namespace ibrio
{
namespace storage
{

//////////////////////////////
// CActivateDB

bool CActivateDB::Initialize(const boost::filesystem::path& pathData)
{
    CLevelDBArguments args;
    args.path = (pathData / "activate").string();
    args.syncwrite = false;
    CLevelDBEngine* engine = new CLevelDBEngine(args);

    if (!Open(engine))
    {
        delete engine;
        return false;
    }
    return true;
}

void CActivateDB::Deinitialize()
{
    cacheActivate.Clear();
    Close();
}

bool CActivateDB::AddNew(const uint256& hashBlock, const CActivateContext& ctxtActivate)
{
    if (!Write(hashBlock, ctxtActivate))
    {
        return false;
    }
    if (fCache)
    {
        cacheActivate.AddNew(hashBlock, ctxtActivate);
    }
    return true;
}

bool CActivateDB::Remove(const uint256& hashBlock)
{
    if (fCache)
    {
        cacheActivate.Remove(hashBlock);
    }
    return Erase(hashBlock);
}

bool CActivateDB::Retrieve(const uint256& hashBlock, CActivateContext& ctxtActivate)
{
    if (fCache)
    {
        if (cacheActivate.Retrieve(hashBlock, ctxtActivate))
        {
            return true;
        }
        if (!Read(hashBlock, ctxtActivate))
        {
            return false;
        }
        cacheActivate.AddNew(hashBlock, ctxtActivate);
    }
    else
    {
        if (!Read(hashBlock, ctxtActivate))
        {
            return false;
        }
    }
    return true;
}

void CActivateDB::Clear()
{
    cacheActivate.Clear();
    RemoveAll();
}

} // namespace storage
} // namespace ibrio
