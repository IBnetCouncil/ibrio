// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "forkdb.h"

#include <boost/bind.hpp>

#include "leveldbeng.h"

using namespace std;
using namespace xengine;

namespace ibrio
{
namespace storage
{

//////////////////////////////
// CForkDB

bool CForkDB::Initialize(const boost::filesystem::path& pathData, const uint256& hashGenesisBlockIn)
{
    hashGenesisBlock = hashGenesisBlockIn;

    CLevelDBArguments args;
    args.path = (pathData / "fork").string();
    args.syncwrite = false;
    args.files = 16;
    args.cache = 2 << 20;

    CLevelDBEngine* engine = new CLevelDBEngine(args);

    if (!Open(engine))
    {
        delete engine;
        return false;
    }

    return true;
}

void CForkDB::Deinitialize()
{
    Close();
}

bool CForkDB::AddNewForkContext(const CForkContext& ctxt)
{
    return Write(make_pair(string("ctxt"), ctxt.hashFork), ctxt);
}

bool CForkDB::RemoveForkContext(const uint256& hashFork)
{
    return Erase(make_pair(string("ctxt"), hashFork));
}

bool CForkDB::RetrieveForkContext(const uint256& hashFork, CForkContext& ctxt)
{
    if (!Read(make_pair(string("ctxt"), hashFork), ctxt))
    {
        StdLog("CForkDB", "Read ctxt fail, forkid: %s", hashFork.GetHex().c_str());
        return false;
    }

    return true;
}

bool CForkDB::ListForkContext(vector<CForkContext>& vForkCtxt, map<uint256, CValidForkId>& mapValidForkId)
{
    if (!WalkThrough(boost::bind(&CForkDB::LoadCtxtWalker, this, _1, _2, boost::ref(vForkCtxt)), string("ctxt"), true))
    {
        StdError("CForkDB", "ListForkContext: Walk through ctxt fail");
        return false;
    }
    if (!WalkThrough(boost::bind(&CForkDB::LoadValidForkWalker, this, _1, _2, boost::ref(mapValidForkId)), string("valid"), true))
    {
        StdError("CForkDB", "ListForkContext: Walk through valid fail");
        return false;
    }
    return true;
}

bool CForkDB::UpdateFork(const uint256& hashFork, const uint256& hashLastBlock)
{
    return Write(make_pair(string("active"), hashFork), hashLastBlock);
}

bool CForkDB::RemoveFork(const uint256& hashFork)
{
    return Erase(make_pair(string("active"), hashFork));
}

bool CForkDB::RetrieveFork(const uint256& hashFork, uint256& hashLastBlock)
{
    return Read(make_pair(string("active"), hashFork), hashLastBlock);
}

bool CForkDB::ListFork(vector<pair<uint256, uint256>>& vFork)
{
    uint256 hashLastBlock;
    if (!RetrieveFork(hashGenesisBlock, hashLastBlock))
    {
        hashLastBlock = hashGenesisBlock;
    }

    map<uint256, int> mapValidFork;
    if (hashLastBlock == hashGenesisBlock)
    {
        mapValidFork.insert(make_pair(hashGenesisBlock, 0));
    }
    else
    {
        uint256 hashRefFdBlock;
        if (RetrieveValidForkHash(hashLastBlock, hashRefFdBlock, mapValidFork))
        {
            if (hashRefFdBlock != 0)
            {
                uint256 hashTempBlock;
                map<uint256, int> mapTempValidFork;
                if (RetrieveValidForkHash(hashRefFdBlock, hashTempBlock, mapTempValidFork) && !mapTempValidFork.empty())
                {
                    mapValidFork.insert(mapTempValidFork.begin(), mapTempValidFork.end());
                }
            }
        }
        if (mapValidFork.empty())
        {
            mapValidFork.insert(make_pair(hashGenesisBlock, 0));
        }
    }

    vector<CForkContext> vForkCtxt;
    map<uint256, uint256> mapActiveFork;

    if (!WalkThrough(boost::bind(&CForkDB::LoadCtxtWalker, this, _1, _2, boost::ref(vForkCtxt)), string("ctxt"), true))
    {
        StdError("CForkDB", "ListFork: Walk through ctxt fail");
        return false;
    }

    if (!WalkThrough(boost::bind(&CForkDB::LoadActiveForkWalker, this, _1, _2, boost::ref(mapActiveFork)), string("active"), true))
    {
        StdError("CForkDB", "ListFork: Walk through active fail");
        return false;
    }

    vFork.reserve(vForkCtxt.size());
    for (auto ctxt : vForkCtxt)
    {
        map<uint256, uint256>::iterator mi = mapActiveFork.find(ctxt.hashFork);
        if (mi != mapActiveFork.end() && mapValidFork.count(ctxt.hashFork))
        {
            vFork.push_back(*mi);
        }
    }
    return true;
}

bool CForkDB::AddValidForkHash(const uint256& hashBlock, const uint256& hashRefFdBlock, const map<uint256, int>& mapValidFork)
{
    return Write(make_pair(string("valid"), hashBlock), CValidForkId(hashRefFdBlock, mapValidFork));
}

bool CForkDB::RetrieveValidForkHash(const uint256& hashBlock, uint256& hashRefFdBlock, map<uint256, int>& mapValidFork)
{
    CValidForkId validForkId;
    if (!Read(make_pair(string("valid"), hashBlock), validForkId))
    {
        //StdError("CForkDB", "RetrieveValidForkHash: Read fail");
        return false;
    }
    hashRefFdBlock = validForkId.hashRefFdBlock;
    mapValidFork.clear();
    mapValidFork.insert(validForkId.mapForkId.begin(), validForkId.mapForkId.end());
    return true;
}

bool CForkDB::ListActiveFork(map<uint256, uint256>& mapActiveFork)
{
    if (!WalkThrough(boost::bind(&CForkDB::LoadActiveForkWalker, this, _1, _2, boost::ref(mapActiveFork)), string("active"), true))
    {
        StdError("CForkDB", "ListActiveFork: Walk through active fail");
        return false;
    }
    return true;
}

void CForkDB::Clear()
{
    RemoveAll();
}

bool CForkDB::LoadCtxtWalker(CBufStream& ssKey, CBufStream& ssValue, vector<CForkContext>& vForkCtxt)
{
    string strPrefix;
    uint256 hashFork;
    ssKey >> strPrefix >> hashFork;

    if (strPrefix == "ctxt")
    {
        CForkContext ctxt;
        ssValue >> ctxt;
        vForkCtxt.push_back(ctxt);
        return true;
    }
    StdError("CForkDB", "LoadCtxtWalker: strPrefix error, strPrefix: %s", strPrefix.c_str());
    return false;
}

bool CForkDB::LoadActiveForkWalker(CBufStream& ssKey, CBufStream& ssValue, map<uint256, uint256>& mapFork)
{
    string strPrefix;
    uint256 hashFork;
    ssKey >> strPrefix >> hashFork;

    if (strPrefix == "active")
    {
        uint256 hashLastBlock;
        ssValue >> hashLastBlock;
        mapFork.insert(make_pair(hashFork, hashLastBlock));
        return true;
    }
    StdError("CForkDB", "LoadActiveForkWalker: strPrefix error, strPrefix: %s", strPrefix.c_str());
    return false;
}

bool CForkDB::LoadValidForkWalker(CBufStream& ssKey, CBufStream& ssValue, map<uint256, CValidForkId>& mapBlockForkId)
{
    string strPrefix;
    uint256 hashBlock;
    ssKey >> strPrefix >> hashBlock;

    if (strPrefix == "valid")
    {
        CValidForkId& validForkId = mapBlockForkId[hashBlock];
        validForkId.Clear();
        ssValue >> validForkId;
        return true;
    }
    StdError("CForkDB", "LoadValidForkWalker: strPrefix error, strPrefix: %s", strPrefix.c_str());
    return false;
}

} // namespace storage
} // namespace ibrio
