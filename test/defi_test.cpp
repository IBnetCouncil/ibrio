// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>
#include <map>
#include <set>
#include <sodium.h>

#include "crypto.h"
#include "defi.h"
#include "forkcontext.h"
#include "param.h"
#include "profile.h"
#include "test_big.h"

using namespace std;
using namespace xengine;
using namespace ibrio;
using namespace storage;
using namespace ibrio::crypto;

BOOST_FIXTURE_TEST_SUITE(defi_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(common_profile)
{
    CProfile profile;
    profile.strName = "IBR Test";
    profile.strSymbol = "IBRA";
    profile.nVersion = 1;
    profile.nMinTxFee = 100;
    profile.nMintReward = 1000;
    profile.nAmount = 100000;

    std::vector<uint8> vchProfile;
    BOOST_CHECK(profile.Save(vchProfile));

    CProfile profileLoad;
    try
    {
        BOOST_CHECK(profileLoad.Load(vchProfile));
    }
    catch (const std::exception& e)
    {
        BOOST_FAIL(e.what());
    }

    BOOST_CHECK(profileLoad.strName == profile.strName);
    BOOST_CHECK(profileLoad.strSymbol == profile.strSymbol);
    BOOST_CHECK(profileLoad.nVersion == profile.nVersion);
    BOOST_CHECK(profileLoad.nMinTxFee == profile.nMinTxFee);
    BOOST_CHECK(profileLoad.nMintReward == profile.nMintReward);
    BOOST_CHECK(profileLoad.nAmount == profile.nAmount);

    CForkContext forkContextWrite(uint256(), uint256(), uint256(), profile);
    CBufStream ss;
    ss << forkContextWrite;
    CForkContext forkContextRead;
    ss >> forkContextRead;

    BOOST_CHECK(forkContextRead.GetProfile().strName == profile.strName);
    BOOST_CHECK(forkContextRead.GetProfile().strSymbol == profile.strSymbol);
    BOOST_CHECK(forkContextRead.GetProfile().nVersion == profile.nVersion);
    BOOST_CHECK(forkContextRead.GetProfile().nMinTxFee == profile.nMinTxFee);
    BOOST_CHECK(forkContextRead.GetProfile().nMintReward == profile.nMintReward);
    BOOST_CHECK(forkContextRead.GetProfile().nAmount == profile.nAmount);
}

BOOST_AUTO_TEST_CASE(defi_profile)
{
    CProfile profile;
    profile.strName = "IBR Test";
    profile.strSymbol = "IBRA";
    profile.nVersion = 1;
    profile.nMinTxFee = 100;
    profile.nMintReward = 1000;
    profile.nAmount = 100000;
    profile.nForkType = FORK_TYPE_DEFI;
    profile.defi.nMintHeight = -1;
    profile.defi.nMaxSupply = 5;
    profile.defi.nCoinbaseType = SPECIFIC_DEFI_COINBASE_TYPE;
    profile.defi.nDecayCycle = 10;
    profile.defi.nCoinbaseDecayPercent = 50;
    profile.defi.nInitCoinbasePercent = 15;
    profile.defi.nPromotionRewardPercent = 20;
    profile.defi.nRewardCycle = 25;
    profile.defi.nSupplyCycle = 55;
    profile.defi.nStakeMinToken = 100;
    profile.defi.nStakeRewardPercent = 30;
    profile.defi.mapPromotionTokenTimes.insert(std::make_pair(10, 11));
    profile.defi.mapPromotionTokenTimes.insert(std::make_pair(11, 12));
    profile.defi.mapCoinbasePercent.insert(std::make_pair(5, 10));
    profile.defi.mapCoinbasePercent.insert(std::make_pair(15, 11));

    std::vector<uint8> vchProfile;
    BOOST_CHECK(profile.Save(vchProfile));

    CProfile profileLoad;
    try
    {
        BOOST_CHECK(profileLoad.Load(vchProfile));
    }
    catch (const std::exception& e)
    {
        BOOST_FAIL(e.what());
    }

    BOOST_CHECK(profileLoad.strName == profile.strName);
    BOOST_CHECK(profileLoad.strSymbol == profile.strSymbol);
    BOOST_CHECK(profileLoad.nVersion == profile.nVersion);
    BOOST_CHECK(profileLoad.nMinTxFee == profile.nMinTxFee);
    BOOST_CHECK(profileLoad.nMintReward == profile.nMintReward);
    BOOST_CHECK(profileLoad.nAmount == profile.nAmount);

    BOOST_CHECK(profileLoad.nForkType == profile.nForkType);
    BOOST_CHECK(profileLoad.defi.nMintHeight == profile.defi.nMintHeight);
    BOOST_CHECK(profileLoad.defi.nMaxSupply == profile.defi.nMaxSupply);
    BOOST_CHECK(profileLoad.defi.nCoinbaseType == profile.defi.nCoinbaseType);
    BOOST_CHECK(profileLoad.defi.nDecayCycle == profile.defi.nDecayCycle);
    BOOST_CHECK(profileLoad.defi.nInitCoinbasePercent == profile.defi.nInitCoinbasePercent);
    BOOST_CHECK(profileLoad.defi.nPromotionRewardPercent == profile.defi.nPromotionRewardPercent);
    BOOST_CHECK(profileLoad.defi.nRewardCycle == profile.defi.nRewardCycle);
    BOOST_CHECK(profileLoad.defi.nSupplyCycle == profile.defi.nSupplyCycle);
    BOOST_CHECK(profileLoad.defi.nStakeMinToken == profile.defi.nStakeMinToken);
    BOOST_CHECK(profileLoad.defi.nStakeRewardPercent == profile.defi.nStakeRewardPercent);
    BOOST_CHECK(profileLoad.defi.mapPromotionTokenTimes.size() == profile.defi.mapPromotionTokenTimes.size());
    BOOST_CHECK(profileLoad.defi.mapCoinbasePercent.size() == profile.defi.mapCoinbasePercent.size());

    CForkContext forkContextWrite(uint256(), uint256(), uint256(), profile);
    CBufStream ss;
    ss << forkContextWrite;
    CForkContext forkContextRead;
    ss >> forkContextRead;

    BOOST_CHECK(forkContextRead.GetProfile().strName == profile.strName);
    BOOST_CHECK(forkContextRead.GetProfile().strSymbol == profile.strSymbol);
    BOOST_CHECK(forkContextRead.GetProfile().nVersion == profile.nVersion);
    BOOST_CHECK(forkContextRead.GetProfile().nMinTxFee == profile.nMinTxFee);
    BOOST_CHECK(forkContextRead.GetProfile().nMintReward == profile.nMintReward);
    BOOST_CHECK(forkContextRead.GetProfile().nAmount == profile.nAmount);

    BOOST_CHECK(forkContextRead.GetProfile().nForkType == profile.nForkType);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nMintHeight == profile.defi.nMintHeight);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nCoinbaseType == profile.defi.nCoinbaseType);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nDecayCycle == profile.defi.nDecayCycle);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nMaxSupply == profile.defi.nMaxSupply);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nInitCoinbasePercent == profile.defi.nInitCoinbasePercent);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nPromotionRewardPercent == profile.defi.nPromotionRewardPercent);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nRewardCycle == profile.defi.nRewardCycle);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nSupplyCycle == profile.defi.nSupplyCycle);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nStakeMinToken == profile.defi.nStakeMinToken);
    BOOST_CHECK(forkContextRead.GetProfile().defi.nStakeRewardPercent == profile.defi.nStakeRewardPercent);
    BOOST_CHECK(forkContextRead.GetProfile().defi.mapPromotionTokenTimes.size() == profile.defi.mapPromotionTokenTimes.size());
    BOOST_CHECK(forkContextRead.GetProfile().defi.mapCoinbasePercent.size() == profile.defi.mapCoinbasePercent.size());
}

static void RandGeneretor256(uint8_t* p)
{
    for (int i = 0; i < 32; i++)
    {
        *p++ = rand();
    }
}

BOOST_AUTO_TEST_CASE(defi_relation_graph)
{
    {
        CAddress A("1w8ehkb2jc0qcn7wze3tv8enzzwmytn9b7n7gghwfa219rv1vhhd82n6h");
        CAddress A1("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");

        std::map<CDestination, CAddrInfo> mapAddressTree{
            { A1, CAddrInfo(CDestination(), A) }
        };

        CDeFiRelationGraph defiGraph;
        for (auto& x : mapAddressTree)
        {
            BOOST_CHECK(defiGraph.Insert(x.first, x.second.destParent, x.second.destParent));
        }
    }

    {
        CAddress A("1w8ehkb2jc0qcn7wze3tv8enzzwmytn9b7n7gghwfa219rv1vhhd82n6h");
        CAddress A1("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");
        CAddress A2("1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q");
        CAddress A3("1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq");

        std::map<CDestination, CAddrInfo> mapAddressTree;
        mapAddressTree.insert(std::make_pair(A1, CAddrInfo(CDestination(), A)));
        mapAddressTree.insert(std::make_pair(A2, CAddrInfo(CDestination(), A)));
        mapAddressTree.insert(std::make_pair(A3, CAddrInfo(CDestination(), A)));

        CDeFiRelationGraph defiGraph;
        for (auto& x : mapAddressTree)
        {
            BOOST_CHECK(defiGraph.Insert(x.first, x.second.destParent, x.second.destParent));
        }
        BOOST_CHECK(defiGraph.mapRoot.size() == 1);
    }

    {
        CAddress A("1w8ehkb2jc0qcn7wze3tv8enzzwmytn9b7n7gghwfa219rv1vhhd82n6h");
        CAddress A1("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");
        CAddress A2("1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q");
        CAddress A3("1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq");
        CAddress AA11("1dq62d8y4fz20sfg63zzy4h4ayksswv1fgqjzvegde306bxxg5zygc27q");
        CAddress AA21("1awxt9zsbtjjxx4bk3q2j18s25kj00cajx3rj1bwg8beep7awmx1pkb8p");
        CAddress AA22("1t877w7b61wsx1rabkd69dbn2kgybpj4ayw2eycezg8qkyfekn97hrmgy");
        CAddress AAA111("18f2dv1vc6nv2xj7ak0e0yye4tx77205f5j73ep2a7a5w6szhjexkd5mj");
        CAddress AAA221("1yy76yav5mnah0jzew049a6gp5bs2ns67xzfvcengjkpqyymfb4n6vrda");
        CAddress AAA222("1g03a0775sbarkrazjrs7qymdepbkn3brn7375p7ysf0tnrcx408pj03n");

        std::map<CDestination, CAddrInfo> mapAddressTree;
        mapAddressTree.insert(std::make_pair(A1, CAddrInfo(CDestination(), A)));
        mapAddressTree.insert(std::make_pair(A2, CAddrInfo(CDestination(), A)));
        mapAddressTree.insert(std::make_pair(A3, CAddrInfo(CDestination(), A)));
        mapAddressTree.insert(std::make_pair(AA11, CAddrInfo(CDestination(), A1)));
        mapAddressTree.insert(std::make_pair(AAA111, CAddrInfo(CDestination(), AA11)));
        mapAddressTree.insert(std::make_pair(AA21, CAddrInfo(CDestination(), A2)));
        mapAddressTree.insert(std::make_pair(AA22, CAddrInfo(CDestination(), A2)));
        mapAddressTree.insert(std::make_pair(AAA221, CAddrInfo(CDestination(), AA22)));
        mapAddressTree.insert(std::make_pair(AAA222, CAddrInfo(CDestination(), AA22)));

        CDeFiRelationGraph defiGraph;
        for (auto& x : mapAddressTree)
        {
            BOOST_CHECK(defiGraph.Insert(x.first, x.second.destParent, x.second.destParent));
        }
        BOOST_CHECK(defiGraph.mapRoot.size() == 1);
        BOOST_CHECK(defiGraph.mapRoot.find(A) != defiGraph.mapRoot.end());
    }

    {
        CAddress A("1w8ehkb2jc0qcn7wze3tv8enzzwmytn9b7n7gghwfa219rv1vhhd82n6h");
        CAddress A1("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");
        CAddress A2("1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q");
        CAddress A3("1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq");
        CAddress AA11("1dq62d8y4fz20sfg63zzy4h4ayksswv1fgqjzvegde306bxxg5zygc27q");
        CAddress AA21("1awxt9zsbtjjxx4bk3q2j18s25kj00cajx3rj1bwg8beep7awmx1pkb8p");
        CAddress AA22("1t877w7b61wsx1rabkd69dbn2kgybpj4ayw2eycezg8qkyfekn97hrmgy");
        CAddress AAA111("18f2dv1vc6nv2xj7ak0e0yye4tx77205f5j73ep2a7a5w6szhjexkd5mj");
        CAddress AAA221("1yy76yav5mnah0jzew049a6gp5bs2ns67xzfvcengjkpqyymfb4n6vrda");
        CAddress AAA222("1g03a0775sbarkrazjrs7qymdepbkn3brn7375p7ysf0tnrcx408pj03n");

        CAddress B("1dt714q0p5143qhekgg0dx9qwnk6ww13f5v27xh6tpfmps25387ga2w5b");
        CAddress B1("1n1c0g6krvcvxhtgebptvz34rdq7qz2dcs6ngrphpnav465fdcpsmmbxj");
        CAddress B2("1m73jrn8np6ny50g3xr461yys6x3rme4yf1t7t9xa464v6n6p84ppzxa2");
        CAddress B3("1q284qfnpasxmkytpv5snda5ptqbpjxa9ryp2re0j1527qncmg38z7ar1");
        CAddress B4("134btp09511w3bnr1qq4de6btdkxkbp2y5x3zmr09g0m9hfn9frsa1k2f");

        std::map<CDestination, CAddrInfo> mapAddressTree;
        mapAddressTree.insert(std::make_pair(A1, CAddrInfo(CDestination(), A)));
        mapAddressTree.insert(std::make_pair(A2, CAddrInfo(CDestination(), A)));
        mapAddressTree.insert(std::make_pair(A3, CAddrInfo(CDestination(), A)));
        mapAddressTree.insert(std::make_pair(AA11, CAddrInfo(CDestination(), A1)));
        mapAddressTree.insert(std::make_pair(AAA111, CAddrInfo(CDestination(), AA11)));
        mapAddressTree.insert(std::make_pair(AA21, CAddrInfo(CDestination(), A2)));
        mapAddressTree.insert(std::make_pair(AA22, CAddrInfo(CDestination(), A2)));
        mapAddressTree.insert(std::make_pair(AAA221, CAddrInfo(CDestination(), AA22)));
        mapAddressTree.insert(std::make_pair(AAA222, CAddrInfo(CDestination(), AA22)));
        mapAddressTree.insert(std::make_pair(B1, CAddrInfo(CDestination(), B)));
        mapAddressTree.insert(std::make_pair(B2, CAddrInfo(CDestination(), B)));
        mapAddressTree.insert(std::make_pair(B3, CAddrInfo(CDestination(), B)));
        mapAddressTree.insert(std::make_pair(B4, CAddrInfo(CDestination(), B)));

        CDeFiRelationGraph defiGraph;
        for (auto& x : mapAddressTree)
        {
            BOOST_CHECK(defiGraph.Insert(x.first, x.second.destParent, x.second.destParent));
        }
        BOOST_CHECK(defiGraph.mapRoot.size() == 2);
        BOOST_CHECK(defiGraph.mapNode.size() == 15);
        BOOST_CHECK(defiGraph.mapRoot.find(A) != defiGraph.mapRoot.end());
        BOOST_CHECK(defiGraph.mapRoot.find(B) != defiGraph.mapRoot.end());
        BOOST_CHECK(defiGraph.mapNode.find(A) != defiGraph.mapNode.end());
        BOOST_CHECK(defiGraph.mapNode.find(B) != defiGraph.mapNode.end());
        BOOST_CHECK(defiGraph.mapRoot.find(B3) == defiGraph.mapRoot.end());
        BOOST_CHECK(defiGraph.mapRoot.find(AA22) == defiGraph.mapRoot.end());
    }
}

BOOST_AUTO_TEST_CASE(reward)
{
    // STD_DEBUG = true;
    // boost::filesystem::path logPath = boost::filesystem::absolute(boost::unit_test::framework::master_test_suite().argv[0]).parent_path();
    // InitLog(logPath, true, true, 1024, 1024);

    srand(time(0));
    CDeFiForkReward r;
    uint256 forkid1;
    RandGeneretor256(forkid1.begin());
    uint256 forkid2;
    RandGeneretor256(forkid2.begin());

    // test ExistFork and AddFork
    BOOST_CHECK(!r.ExistFork(forkid1));

    CProfile profile1;
    profile1.strName = "IBR Test1";
    profile1.strSymbol = "IBRA";
    profile1.nVersion = 1;
    profile1.nMinTxFee = MIN_TX_FEE;
    profile1.nMintReward = 0;
    profile1.nAmount = 21000000 * COIN;
    profile1.nJointHeight = 150;
    profile1.nForkType = FORK_TYPE_DEFI;
    profile1.defi.nMintHeight = 0;
    profile1.defi.nMaxSupply = 2100000000 * COIN;
    profile1.defi.nCoinbaseType = FIXED_DEFI_COINBASE_TYPE;
    profile1.defi.nDecayCycle = 1036800;
    profile1.defi.nCoinbaseDecayPercent = 50;
    profile1.defi.nInitCoinbasePercent = 10;
    profile1.defi.nPromotionRewardPercent = 50;
    profile1.defi.nRewardCycle = DAY_HEIGHT;
    profile1.defi.nSupplyCycle = 43200;
    profile1.defi.nStakeMinToken = 100 * COIN;
    profile1.defi.nStakeRewardPercent = 50;
    profile1.defi.mapPromotionTokenTimes.insert(std::make_pair(10000, 10));
    r.AddFork(forkid1, profile1);

    CProfile profile2 = profile1;
    profile2.strName = "IBR Test2";
    profile2.strSymbol = "IBRB";
    profile2.nVersion = 1;
    profile2.nMinTxFee = MIN_TX_FEE;
    profile2.nMintReward = 0;
    profile2.nAmount = 10000000 * COIN;
    profile2.nJointHeight = 150;
    profile2.nForkType = FORK_TYPE_DEFI;
    profile2.defi.nMintHeight = 1500;
    profile2.defi.nMaxSupply = 1000000000 * COIN;
    profile2.defi.nCoinbaseType = SPECIFIC_DEFI_COINBASE_TYPE;
    profile2.defi.mapCoinbasePercent = { { 259200, 10 }, { 777600, 8 }, { 1814400, 5 }, { 3369600, 3 }, { 5184000, 2 } };
    profile2.defi.nRewardCycle = DAY_HEIGHT;
    profile2.defi.nSupplyCycle = 43200;
    profile2.defi.nStakeMinToken = 100 * COIN;
    profile2.defi.nStakeRewardPercent = 50;
    profile2.defi.mapPromotionTokenTimes.insert(std::make_pair(10000, 10));
    r.AddFork(forkid2, profile2);

    BOOST_CHECK(r.ExistFork(forkid1));
    BOOST_CHECK(r.GetForkProfile(forkid1).strSymbol == "IBRA");

    // test common reward
    BOOST_CHECK(r.GetFixedDecayReward(profile1, 152) == 70000000000);
    BOOST_CHECK(r.GetSpecificDecayReward(profile2, 1500) == 33333333333);

    // test PrevRewardHeight
    BOOST_CHECK(r.PrevRewardHeight(forkid1, -10) == -1);
    BOOST_CHECK(r.PrevRewardHeight(forkid1, 0) == -1);
    BOOST_CHECK(r.PrevRewardHeight(forkid1, 151) == -1);
    BOOST_CHECK(r.PrevRewardHeight(forkid1, 152) == 151);
    BOOST_CHECK(r.PrevRewardHeight(forkid1, 1591) == 151);
    BOOST_CHECK(r.PrevRewardHeight(forkid1, 1592) == 1591);
    BOOST_CHECK(r.PrevRewardHeight(forkid1, 100000) == 99511);
    BOOST_CHECK(r.PrevRewardHeight(forkid1, 10000000) == 9999511);

    // test coinbase
    // fixed coinbase
    {
        // old type
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(0, uint224(0))) == 0);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(151, uint224(0))) == 0);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(1591, uint224(0))) == 70000000000);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(44791, uint224(0))) == 77000000000);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(1038391, uint224(0))) == 344740643653);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(4148791, uint224(0))) == 338687535219);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(7086391, uint224(0))) == 109335972442);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(7093591, uint224(0))) == 109335972442);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(7095031, uint224(0))) == 93313269565);
        BOOST_CHECK(r.GetSectionReward(forkid1, uint256(7096471, uint224(0))) == 0);

        // new type
        int64 nInitSupply = 21000000 * COIN;
        int64 nSupply = nInitSupply;
        int64 nInvalidSupply = rand() % nInitSupply;
        int64 nHeight = ((profile1.defi.nMintHeight == 0) ? (profile1.nJointHeight + 2) : profile1.defi.nMintHeight) - 1;
        double fRate = (double)profile1.defi.nInitCoinbasePercent / 100;
        bool fTerminate = false;
        for (int i = 0; i < 1000 && !fTerminate; i++)
        {
            for (int j = 0; j < profile1.defi.nDecayCycle / profile1.defi.nSupplyCycle && !fTerminate; j++)
            {
                for (int k = 0; k < profile1.defi.nSupplyCycle / profile1.defi.nRewardCycle && !fTerminate; k++)
                {
                    nHeight += profile1.defi.nRewardCycle;
                    int64 nReward = r.GetSectionReward(forkid1, uint256(nHeight, uint224(0)), nSupply, nInvalidSupply);
                    // cout << "... height: " << nHeight << ", reward: " << nReward << ", supply begin: " << nSupply << ", supply end: " << (nSupply + nReward) << ", rate: " << ((double)nReward / nSupply) << endl;
                    nSupply += nReward;
                    if (nReward == 0 || nSupply >= profile1.defi.nMaxSupply)
                    {
                        fTerminate = true;
                    }
                }
                if (!fTerminate)
                {
                    // cout << "rate: " << ((double)(nSupply - nInvalidSupply) / (nInitSupply - nInvalidSupply) - 1) << ", should be: " << fRate << endl;
                    BOOST_CHECK(fabs(((double)(nSupply - nInvalidSupply) / (nInitSupply - nInvalidSupply) - 1) - fRate) < numeric_limits<float>::epsilon());
                    nInitSupply = nSupply;
                }
            }
            fRate *= (double)profile1.defi.nCoinbaseDecayPercent / 100;
            // cout << "decay count: " << (i + 1) << ", rate: " << fRate << endl;
        }
        // cout << "supply: " << nSupply << endl;
    }

    // specific coinbase
    {
        // old type
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(0, uint224(0))) == 0);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(1499, uint224(0))) == 0);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(2939, uint224(0))) == 33333333333);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(46139, uint224(0))) == 36666666667);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(260699, uint224(0))) == 53683666667);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(262139, uint224(0))) == 47241626667);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(779099, uint224(0))) == 110150419020);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(780539, uint224(0))) == 74351532839);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(1815899, uint224(0))) == 228372499403);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(1817339, uint224(0))) == 143874674624);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(3371099, uint224(0))) == 404843545059);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(3372539, uint224(0))) == 277992567607);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(3371099, uint224(0))) == 404843545059);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(3372539, uint224(0))) == 277992567607);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(5185499, uint224(0))) == 626094987776);
        BOOST_CHECK(r.GetSectionReward(forkid2, uint256(5186939, uint224(0))) == 0);

        // new type
        int64 nInitSupply = 10000000 * COIN;
        int64 nSupply = nInitSupply;
        int64 nInvalidSupply = rand() % nInitSupply;
        int64 nHeight = ((profile2.defi.nMintHeight == 0) ? (profile2.nJointHeight + 2) : profile2.defi.nMintHeight) - 1;
        bool fTerminate = false;
        int32 nLastHeight = 0;
        for (auto& x : profile2.defi.mapCoinbasePercent)
        {
            double fRate = (double)x.second / 100;
            for (int i = 0; i < (x.first - nLastHeight) / profile2.defi.nSupplyCycle && !fTerminate; i++)
            {
                for (int k = 0; k < profile2.defi.nSupplyCycle / profile2.defi.nRewardCycle && !fTerminate; k++)
                {
                    nHeight += profile2.defi.nRewardCycle;
                    int64 nReward = r.GetSectionReward(forkid2, uint256(nHeight, uint224(0)), nSupply, nInvalidSupply);
                    // cout << "... height: " << nHeight << ", reward: " << nReward << ", supply begin: " << nSupply << ", supply end: " << (nSupply + nReward) << ", rate: " << ((double)nReward / nSupply) << endl;
                    nSupply += nReward;
                    if (nReward == 0 || nSupply >= profile2.defi.nMaxSupply)
                    {
                        fTerminate = true;
                    }
                }
                if (!fTerminate)
                {
                    // cout << "rate: " << ((double)(nSupply - nInvalidSupply) / (nInitSupply - nInvalidSupply) - 1) << ", should be: " << fRate << endl;
                    BOOST_CHECK(fabs(((double)(nSupply - nInvalidSupply) / (nInitSupply - nInvalidSupply) - 1) - fRate) < numeric_limits<float>::epsilon());
                    nInitSupply = nSupply;
                }
            }
            nLastHeight = x.first;
            // cout << "specific height: " << x.first << ", rate: " << fRate << endl;
        }
        // cout << "supply: " << nSupply << endl;
    }

    int64 nReward = 28762708333;
    CAddress A("1632srrskscs1d809y3x5ttf50f0gabf86xjz2s6aetc9h9ewwhm58dj3");
    CAddress a1("1f1nj5gjgrcz45g317s1y4tk18bbm89jdtzd41m9s0t14tp2ngkz4cg0x");
    CAddress a11("1pmj5p47zhqepwa9vfezkecxkerckhrks31pan5fh24vs78s6cbkrqnxp");
    CAddress a111("1bvaag2t23ybjmasvjyxnzetja0awe5d1pyw361ea3jmkfdqt5greqvfq");
    CAddress a2("1ab1sjh07cz7xpb0xdkpwykfm2v91cvf2j1fza0gj07d2jktdnrwtyd57");
    CAddress a21("1782a5jv06egd6pb2gjgp2p664tgej6n4gmj79e1hbvgfgy3t006wvwzt");
    CAddress a22("1c7s095dcvzdsedrkpj6y5kjysv5sz3083xkahvyk7ry3tag4ddyydbv4");
    CAddress a221("1ytysehvcj13ka9r1qhh9gken1evjdp9cn00cz0bhqjqgzwc6sdmse548");
    CAddress a222("16aaahq32cncamxbmvedjy5jqccc2hcpk7rc0h2zqcmmrnm1g2213t2k3");
    CAddress a3("1vz7k0748t840bg3wgem4mhf9pyvptf1z2zqht6bc2g9yn6cmke8h4dwe");
    CAddress B("1fpt2z9nyh0a5999zrmabg6ppsbx78wypqapm29fsasx993z11crp6zm7");
    CAddress b1("1rampdvtmzmxfzr3crbzyz265hbr9a8y4660zgpbw6r7qt9hdy535zed7");
    CAddress b2("1w0k188jwq5aenm6sfj6fdx9f3d96k20k71612ddz81qrx6syr4bnp5m9");
    CAddress b3("196wx05mee1zavws828vfcap72tebtskw094tp5sztymcy30y7n9varfa");
    CAddress b4("19k8zjwdntjp8avk41c8aek0jxrs1fgyad5q9f1gd1q2fdd0hafmm549d");
    CAddress C("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");

    // test stake reward
    // B = 1/1
    map<CDestination, int64> balance;
    balance = map<CDestination, int64>{
        { A, 0 },
        { B, 100 * COIN },
    };
    {
        CDeFiRewardSet reward = r.ComputeStakeReward(profile1.defi.nStakeMinToken, nReward, balance);
        BOOST_CHECK(reward.size() == 1);
        auto it = reward.begin();
        BOOST_CHECK(it->dest == B && it->nReward == nReward);
    }

    // a1 = 1/5, a11 = 3/5, a111 = 1/5
    balance = map<CDestination, int64>{
        { A, 0 },
        { a1, 100 * COIN },
        { a11, 1000 * COIN },
        { a111, 100 * COIN },
    };
    {
        CDeFiRewardSet reward = r.ComputeStakeReward(profile1.defi.nStakeMinToken, nReward, balance);
        BOOST_CHECK(reward.size() == 3);
        auto& destIdx = reward.get<0>();
        auto it = destIdx.find(a1);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 5752541666);
        it = destIdx.find(a11);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 17257624999);
        it = destIdx.find(a111);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 5752541666);
    }

    // test promotion reward
    balance = map<CDestination, int64>{
        { A, 10000 * COIN },
        { a1, 100000 * COIN },
        { a11, 100000 * COIN },
        { a111, 100000 * COIN },
        { a2, 1 * COIN },
        { a21, 1 * COIN },
        { a22, 12000 * COIN },
        { a221, 18000 * COIN },
        { a222, 5000 * COIN },
        { a3, 1000000 * COIN },
        { B, 10000 * COIN },
        { b1, 10000 * COIN },
        { b2, 11000 * COIN },
        { b3, 5000 * COIN },
        { b4, 50000 * COIN },
        { C, 19568998 * COIN },
    };

    map<CDestination, CAddrInfo> mapAddress;
    mapAddress = map<CDestination, CAddrInfo>{
        { a1, CAddrInfo(A, A) },
        { a11, CAddrInfo(A, a1) },
        { a111, CAddrInfo(A, a11) },
        { a2, CAddrInfo(A, A) },
        { a21, CAddrInfo(A, a2) },
        { a22, CAddrInfo(A, a2) },
        { a221, CAddrInfo(A, a22) },
        { a222, CAddrInfo(A, a22) },
        { a3, CAddrInfo(A, A) },
        { b1, CAddrInfo(B, B) },
        { b2, CAddrInfo(B, B) },
        { b3, CAddrInfo(B, B) },
        { b4, CAddrInfo(B, B) },
    };

    CDeFiRelationGraph relation;
    for (auto& x : mapAddress)
    {
        BOOST_CHECK(relation.Insert(x.first, x.second.destParent, x.second.destParent));
    }
    {
        CDeFiRewardSet reward = r.ComputePromotionReward(nReward, balance, profile1.defi.mapPromotionTokenTimes, relation, set<CDestination>());
        BOOST_CHECK(reward.size() == 6);

        auto& destIdx = reward.get<0>();
        auto it = destIdx.find(A);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 18149590582);
        it = destIdx.find(a1);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 2043626);
        it = destIdx.find(a11);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 1620807);
        it = destIdx.find(a2);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 1515102);
        it = destIdx.find(a22);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 1762663353);
        it = destIdx.find(B);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 8845274860);
    }

    // boost::filesystem::remove_all(logPath);
}

BOOST_AUTO_TEST_CASE(reward2)
{
    CDeFiForkReward r;
    uint256 forkid;
    RandGeneretor256(forkid.begin());

    // test ExistFork and AddFork
    BOOST_CHECK(!r.ExistFork(forkid));

    CProfile profile;
    profile.strName = "IBR Test2";
    profile.strSymbol = "IBRB";
    profile.nVersion = 1;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nMintReward = 0;
    profile.nAmount = 10000000 * COIN;
    profile.nJointHeight = 15;
    profile.nForkType = FORK_TYPE_DEFI;
    profile.defi.nMintHeight = 20;
    profile.defi.nMaxSupply = 1000000000 * COIN;
    profile.defi.nCoinbaseType = SPECIFIC_DEFI_COINBASE_TYPE;
    profile.defi.mapCoinbasePercent = { { 259200, 10 }, { 777600, 8 }, { 1814400, 5 }, { 3369600, 3 }, { 5184000, 2 } };
    profile.defi.nRewardCycle = 5;            // every 5 height once reward
    profile.defi.nSupplyCycle = 150;          // every 150  once supply
    profile.defi.nStakeMinToken = 100 * COIN; // min token required, >= 100, can be required to join this defi game
    profile.defi.nStakeRewardPercent = 50;    // 50% of supply amount per day
    profile.defi.mapPromotionTokenTimes.insert(std::make_pair(10000, 10));
    r.AddFork(forkid, profile);

    BOOST_CHECK(r.ExistFork(forkid));
    BOOST_CHECK(r.GetForkProfile(forkid).strSymbol == "IBRB");

    int64 nReward = r.GetSectionReward(forkid, uint256(24, uint224(0)));

    CAddress A("1632srrskscs1d809y3x5ttf50f0gabf86xjz2s6aetc9h9ewwhm58dj3");
    CAddress a1("1f1nj5gjgrcz45g317s1y4tk18bbm89jdtzd41m9s0t14tp2ngkz4cg0x");
    CAddress a11("1pmj5p47zhqepwa9vfezkecxkerckhrks31pan5fh24vs78s6cbkrqnxp");
    CAddress a111("1bvaag2t23ybjmasvjyxnzetja0awe5d1pyw361ea3jmkfdqt5greqvfq");
    CAddress a2("1ab1sjh07cz7xpb0xdkpwykfm2v91cvf2j1fza0gj07d2jktdnrwtyd57");
    CAddress a21("1782a5jv06egd6pb2gjgp2p664tgej6n4gmj79e1hbvgfgy3t006wvwzt");
    CAddress a22("1c7s095dcvzdsedrkpj6y5kjysv5sz3083xkahvyk7ry3tag4ddyydbv4");
    CAddress a221("1ytysehvcj13ka9r1qhh9gken1evjdp9cn00cz0bhqjqgzwc6sdmse548");
    CAddress a222("16aaahq32cncamxbmvedjy5jqccc2hcpk7rc0h2zqcmmrnm1g2213t2k3");
    CAddress a3("1vz7k0748t840bg3wgem4mhf9pyvptf1z2zqht6bc2g9yn6cmke8h4dwe");
    CAddress B("1fpt2z9nyh0a5999zrmabg6ppsbx78wypqapm29fsasx993z11crp6zm7");
    CAddress b1("1rampdvtmzmxfzr3crbzyz265hbr9a8y4660zgpbw6r7qt9hdy535zed7");
    CAddress b2("1w0k188jwq5aenm6sfj6fdx9f3d96k20k71612ddz81qrx6syr4bnp5m9");
    CAddress b3("196wx05mee1zavws828vfcap72tebtskw094tp5sztymcy30y7n9varfa");
    CAddress b4("19k8zjwdntjp8avk41c8aek0jxrs1fgyad5q9f1gd1q2fdd0hafmm549d");
    CAddress C("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");

    // test stake reward
    // B = 1/1
    map<CDestination, int64> balance;
    balance = map<CDestination, int64>{
        { A, 0 },
        { B, 100 * COIN },
    };
    // std::cout << "nReward " << nReward << std::endl;
    {
        CDeFiRewardSet reward = r.ComputeStakeReward(profile.defi.nStakeMinToken, (nReward), balance);
        BOOST_CHECK(reward.size() == 1);
        auto it = reward.begin();

        // nReward = 925925925
        // 1 / 1 * nReward * 50%
        BOOST_CHECK(it->dest == B && it->nReward == (nReward));
    }

    balance = map<CDestination, int64>{
        { A, 0 },
        { a1, 100 * COIN },   //  rank 1
        { a11, 1000 * COIN }, // rank 5
        { a111, 100 * COIN }, // rank 1
        { a221, 105 * COIN }, // rank 4
        { a222, 100 * COIN }, // rank 1
    };
    {
        CDeFiRewardSet reward = r.ComputeStakeReward(profile.defi.nStakeMinToken, nReward, balance);
        BOOST_CHECK(reward.size() == 5);
        auto& destIdx = reward.get<0>();
        auto it = destIdx.find(a1);
        // 1 / 12 * nReward
        BOOST_CHECK(it != destIdx.end() && it->nReward == (2777777777));
        // std::cout << "a1 " << it->nReward << std::endl;
        it = destIdx.find(a11);
        // 5 / 12 * nReward
        BOOST_CHECK(it != destIdx.end() && it->nReward == (13888888888));
        it = destIdx.find(a111);
        // 1 / 12 * nReward
        BOOST_CHECK(it != destIdx.end() && it->nReward == (2777777777));
        it = destIdx.find(a221);
        // 4 / 12 * nReward
        BOOST_CHECK(it != destIdx.end() && it->nReward == (11111111111));

        it = destIdx.find(a222);
        // 1 / 12 * nReward
        BOOST_CHECK(it != destIdx.end() && it->nReward == (2777777777));
    }

    // test promotion reward
    balance = map<CDestination, int64>{
        { A, 10000 * COIN },
        { a1, 100000 * COIN },
        { a11, 100000 * COIN },
        { a111, 100000 * COIN },
        { a2, 1 * COIN },
        { a21, 1 * COIN },
        { a22, 12000 * COIN },
        { a221, 18000 * COIN },
        { a222, 5000 * COIN },
        { a3, 1000000 * COIN },
        { B, 10000 * COIN },
        { b1, 10000 * COIN },
        { b2, 11000 * COIN },
        { b3, 5000 * COIN },
        { b4, 50000 * COIN },
        { C, 19568998 * COIN },
    };

    {
        CDeFiRewardSet reward = r.ComputeStakeReward(profile.defi.nStakeMinToken, (nReward), balance);
        BOOST_CHECK(reward.size() == 14);
        auto& destIdx = reward.get<0>();

        // 3 / 98 * nReward
        auto it = destIdx.find(A);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (1020408163));

        // 10 / 98 * nReward
        it = destIdx.find(a1);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (3401360544));

        // 10 / 98 * nReward
        it = destIdx.find(a11);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (3401360544));

        // 10 / 98 * nReward
        it = destIdx.find(a111);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (3401360544));

        // 7 / 98 * nReward
        it = destIdx.find(a22);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (2380952380));

        // 8 / 98 * nReward
        it = destIdx.find(a221);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (2721088435));

        // 1 / 98 * nReward
        it = destIdx.find(a222);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (340136054));

        // 13 / 98 * nReward
        it = destIdx.find(a3);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (4421768707));

        // 3 / 98 * nReward
        it = destIdx.find(B);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (1020408163));

        // 3 / 98 * nReward
        it = destIdx.find(b1);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (1020408163));

        // 6 / 98 * nReward
        it = destIdx.find(b2);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (2040816326));

        // 1 / 98 * nReward
        it = destIdx.find(b3);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (340136054));

        // 9 / 98 * nReward
        it = destIdx.find(b4);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (3061224489));

        // 14 / 98 * nReward
        it = destIdx.find(C);
        BOOST_CHECK(it != destIdx.end() && it->nReward == (4761904761));
    }

    map<CDestination, CAddrInfo> mapAddress;
    mapAddress = map<CDestination, CAddrInfo>{
        { a1, CAddrInfo(A, A) },
        { a11, CAddrInfo(A, a1) },
        { a111, CAddrInfo(A, a11) },
        { a2, CAddrInfo(A, A) },
        { a21, CAddrInfo(A, a2) },
        { a22, CAddrInfo(A, a2) },
        { a221, CAddrInfo(A, a22) },
        { a222, CAddrInfo(A, a22) },
        { a3, CAddrInfo(A, A) },
        { b1, CAddrInfo(B, B) },
        { b2, CAddrInfo(B, B) },
        { b3, CAddrInfo(B, B) },
        { b4, CAddrInfo(B, B) },
    };

    CDeFiRelationGraph relation;
    for (auto& x : mapAddress)
    {
        BOOST_CHECK(relation.Insert(x.first, x.second.destParent, x.second.destParent));
    }
    {
        CDeFiRewardSet reward = r.ComputePromotionReward(nReward, balance, profile.defi.mapPromotionTokenTimes, relation, set<CDestination>());
        BOOST_CHECK(reward.size() == 6);
        auto& destIdx = reward.get<0>();

        // 515102 / 816312 * nReward
        auto it = destIdx.find(A);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 21033706066);

        // 58 / 816312 * nReward
        it = destIdx.find(a1);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 2368375);

        // 46 / 816312 * nReward
        it = destIdx.find(a11);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 1878366);

        // 43 / 816312 * nReward
        it = destIdx.find(a2);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 1755864);

        // 50026 / 816312 * nReward
        it = destIdx.find(a22);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 2042764694);

        // 251037 / 816312 * nReward
        it = destIdx.find(B);
        BOOST_CHECK(it != destIdx.end() && it->nReward == 10250859965);
    }
}

// test second defi reward supply. because reward1, reward2 just change param of fork and test first defi reward
BOOST_AUTO_TEST_CASE(reward_fixed)
{
    CDeFiForkReward r;
    uint256 forkid;
    RandGeneretor256(forkid.begin());

    // test ExistFork and AddFork
    BOOST_CHECK(!r.ExistFork(forkid));

    CProfile profile;
    profile.strName = "IBR Test3";
    profile.strSymbol = "IBRC";
    profile.nVersion = 1;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nMintReward = 0;
    profile.nAmount = 20000000 * COIN;
    profile.nJointHeight = 0;
    profile.nForkType = FORK_TYPE_DEFI;
    profile.defi.nMintHeight = 10;
    profile.defi.nMaxSupply = 1000000000 * COIN;
    profile.defi.nCoinbaseType = FIXED_DEFI_COINBASE_TYPE;
    profile.defi.nRewardCycle = 5;   // 1440 = 60 * 24  every N height once reward
    profile.defi.nSupplyCycle = 150; // 43200 = 60 * 24 * 30 every N height once supply
    profile.defi.nDecayCycle = 3600;
    profile.defi.nCoinbaseDecayPercent = 50;
    profile.defi.nInitCoinbasePercent = 10;
    profile.defi.nStakeMinToken = 100 * COIN;  // min token required, >= 100, can be required to join this defi game
    profile.defi.nStakeRewardPercent = 50;     // 50% of supply amount per day
    profile.defi.nPromotionRewardPercent = 50; // 50% of supply amount per day
    profile.defi.mapPromotionTokenTimes.insert(std::make_pair(10000, 10));
    r.AddFork(forkid, profile);

    BOOST_CHECK(r.ExistFork(forkid));
    BOOST_CHECK(r.GetForkProfile(forkid).strSymbol == "IBRC");

    CAddress A("1ykdy1dtjw810t8g4ymjqfkkr8yj7m5mr6g20595yhssvr2gva56n2rdn");
    CAddress a1("1xnassszea7qevgn44hhg5fqswe78jn95gjd1mx184zr753yt0myfmfxf");
    CAddress a11("1nkz90d8jxzcw2rs21tmfxcvpewx7vrz2s0fr033wjm77fchsbzqy7ygb");
    CAddress a111("1g9pgwqpcbz3x9czsvtjhc3ed0b24vmyrj5k6zfbh1g82ma67qax55dqy");
    CAddress a2("1cem1035c3f7x58808yqge07z8fb72m3czfmzj23hmaxs477tt2ph9yrp");
    CAddress a21("16br4z57ys0dbgxzesvfhtxrpaq3w3j3bsqgdmkrvye06exg9e74ertx8");
    CAddress a22("123zbs56t8q5qn6sjchtvhcxc1ngq5v7jvst6hd2nq7hdtfmr691rd6mb");
    CAddress a221("1pqmm5nj6qpy436qcytshjra24bhvsysjv6k2xswfzma6w1scwyye25r9");
    CAddress a222("12z4t8dzgzmnh3zg1m0b4s2yean8w74cyv0fgrdm7dpq6hbnrq2w0gdwk");
    CAddress a3("1h1tn0dcf7cz44cfypmwqjkm7jvxznrdab2ycn0wcyc9q8983fq3akt40");
    CAddress B("1xftkpw9afcgrfac4v3xa9bgb1zsh03bd59npwtyjrqjbtqepr6anrany");
    CAddress b1("1thcbm7h2bnyb247wknsvnfsxxd7gw12m2bg5d5gdrmzqmxz93s7k6pvy");
    CAddress b2("15n2eszyzk3qm7y04es5fr5fxa3hpbdqqnp1470rmnbmkhpp66mq55qeb");
    CAddress b3("1espce7qwvy94qcanecfr7c65fgsjbr1g407ab00p44a65de0mm1mtrqn");
    CAddress b4("1mbv5et5sx6x1jejgfh214ze1vryg60n8j6tz30czgzmm3rr461jn0nwm");
    CAddress C("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");

    map<string, int64> mapReward = map<string, int64>{
        { A.ToString(), 0 },
        { a1.ToString(), 0 },
        { a11.ToString(), 0 },
        { a111.ToString(), 0 },
        { a2.ToString(), 0 },
        { a21.ToString(), 0 },
        { a22.ToString(), 0 },
        { a221.ToString(), 0 },
        { a222.ToString(), 0 },
        { a3.ToString(), 0 },
        { B.ToString(), 0 },
        { b1.ToString(), 0 },
        { b2.ToString(), 0 },
        { b3.ToString(), 0 },
        { b4.ToString(), 0 },
        { C.ToString(), 0 },
    };

    map<CDestination, int64> balance = map<CDestination, int64>{
        { A, 10000 * COIN },
        { a1, 100000 * COIN },
        { a11, 100000 * COIN },
        { a111, 100000 * COIN },
        { a2, 1 * COIN },
        { a21, 1 * COIN },
        { a22, 12000 * COIN },
        { a221, 18000 * COIN },
        { a222, 5000 * COIN },
        { a3, 1000000 * COIN },
        { B, 10000 * COIN },
        { b1, 10000 * COIN },
        { b2, 11000 * COIN },
        { b3, 5000 * COIN },
        { b4, 50000 * COIN },
        { C, 8568998 * COIN },
    };

    map<CDestination, CAddrInfo> mapAddress;
    mapAddress = map<CDestination, CAddrInfo>{
        { a1, CAddrInfo(A, A) },
        { a11, CAddrInfo(A, a1) },
        { a111, CAddrInfo(A, a11) },
        { a2, CAddrInfo(A, A) },
        { a21, CAddrInfo(A, a2) },
        { a22, CAddrInfo(A, a2) },
        { a221, CAddrInfo(A, a22) },
        { a222, CAddrInfo(A, a22) },
        { a3, CAddrInfo(A, A) },
        { b1, CAddrInfo(B, B) },
        { b2, CAddrInfo(B, B) },
        { b3, CAddrInfo(B, B) },
        { b4, CAddrInfo(B, B) },
    };

    CDeFiRelationGraph relation;
    for (auto& x : mapAddress)
    {
        BOOST_CHECK(relation.Insert(x.first, x.second.destParent, x.second.destParent));
    }

    for (int i = 0; i < 2000; i++)
    {
        // for (auto& x : mapReward)
        // {
        //     cout << "begin addr: " << x.first << ", stake: " << balance[CAddress(x.first)] << endl;
        // }

        int32 nHeight = profile.defi.nMintHeight + (i + 1) * profile.defi.nRewardCycle - 1;
        int64 nReward = r.GetSectionReward(forkid, uint256(nHeight, uint224(0)));
        // cout << "height: " << nHeight << ", reward: " << nReward << endl;
        int64 nStakeReward = nReward * profile.defi.nStakeRewardPercent / 100;
        CDeFiRewardSet stakeReward = r.ComputeStakeReward(profile.defi.nStakeMinToken, nStakeReward, balance);
        // cout << "stake reward: " << nStakeReward << ", size: " << stakeReward.size() << endl;
        for (auto& x : mapReward)
        {
            auto it = stakeReward.find(CAddress(x.first));
            if (it != stakeReward.end())
            {
                // cout << "stake reward addr: " << x.first << ", reward: " << it->nStakeReward << endl;
            }
        }

        int64 nPromotionReward = nReward * profile.defi.nPromotionRewardPercent / 100;
        CDeFiRewardSet promotionReward = r.ComputePromotionReward(nPromotionReward, balance, profile.defi.mapPromotionTokenTimes, relation, set<CDestination>());
        // cout << "promotion reward: " << nPromotionReward << ", size: " << promotionReward.size() << endl;
        for (auto& x : mapReward)
        {
            auto it = promotionReward.find(CAddress(x.first));
            if (it != promotionReward.end())
            {
                // cout << "promotion reward addr: " << x.first << ", reward: " << it->nPromotionReward << endl;
            }
        }

        CDeFiRewardSet s;
        CDeFiRewardSetByDest& destIdx = promotionReward.get<0>();
        for (auto& stake : stakeReward)
        {
            CDeFiReward reward = stake;
            auto it = destIdx.find(stake.dest);
            if (it != destIdx.end())
            {
                reward.nPower = it->nPower;
                reward.nPromotionReward = it->nPromotionReward;
                reward.nReward += it->nPromotionReward;
                destIdx.erase(it);
            }

            s.insert(move(reward));
        }

        for (auto& promotion : promotionReward)
        {
            CDeFiReward reward = promotion;
            s.insert(move(reward));
        }

        // cout << "height: " << (nHeight + 1) << endl;
        for (auto& x : mapReward)
        {
            auto it = s.find(CAddress(x.first));
            if (it != s.end())
            {
                x.second = it->nReward;
            }
            else
            {
                x.second = 0;
            }

            if (x.second > MIN_TX_FEE)
            {
                balance[CAddress(x.first)] += x.second - MIN_TX_FEE;
                // cout << "addr: " << x.first << ", reward: " << x.second << endl;
            }
        }

        if (s.size() == 0)
        {
            cout << "reward is 0, height: " << nHeight << endl;
            break;
        }
    }

    int64 nTotal = 0;
    for (auto& x : mapReward)
    {
        cout << "addr: " << x.first << ", reward: " << balance[CAddress(x.first)] << endl;
        nTotal += balance[CAddress(x.first)];
    }
    cout << "total: " << nTotal << endl;
}

// test second defi reward supply. because reward1, reward2 just change param of fork and test first defi reward
BOOST_AUTO_TEST_CASE(reward_specific)
{
    CDeFiForkReward r;
    uint256 forkid;
    RandGeneretor256(forkid.begin());

    // test ExistFork and AddFork
    BOOST_CHECK(!r.ExistFork(forkid));

    CProfile profile;
    profile.strName = "IBR Test3";
    profile.strSymbol = "IBRC";
    profile.nVersion = 1;
    profile.nMinTxFee = MIN_TX_FEE;
    profile.nMintReward = 0;
    profile.nAmount = 20000000 * COIN;
    profile.nJointHeight = 0;
    profile.nForkType = FORK_TYPE_DEFI;
    profile.defi.nMintHeight = 10;
    profile.defi.nMaxSupply = 1000000000 * COIN;
    profile.defi.nCoinbaseType = SPECIFIC_DEFI_COINBASE_TYPE;
    profile.defi.nRewardCycle = 5;   // 1440 = 60 * 24  every N height once reward
    profile.defi.nSupplyCycle = 150; // 43200 = 60 * 24 * 30 every N height once supply
    // profile.defi.nDecayCycle = 3600;
    // profile.defi.nCoinbaseDecayPercent = 50;
    // profile.defi.nInitCoinbasePercent = 10;
    profile.defi.mapCoinbasePercent = { { 900, 10 }, { 2700, 8 }, { 6300, 5 }, { 11700, 3 }, { 18000, 2 } };
    profile.defi.nStakeMinToken = 100 * COIN;  // min token required, >= 100, can be required to join this defi game
    profile.defi.nStakeRewardPercent = 50;     // 50% of supply amount per day
    profile.defi.nPromotionRewardPercent = 50; // 50% of supply amount per day
    profile.defi.mapPromotionTokenTimes.insert(std::make_pair(10000, 10));
    r.AddFork(forkid, profile);

    BOOST_CHECK(r.ExistFork(forkid));
    BOOST_CHECK(r.GetForkProfile(forkid).strSymbol == "IBRC");

    CAddress A("1ykdy1dtjw810t8g4ymjqfkkr8yj7m5mr6g20595yhssvr2gva56n2rdn");
    CAddress a1("1xnassszea7qevgn44hhg5fqswe78jn95gjd1mx184zr753yt0myfmfxf");
    CAddress a11("1nkz90d8jxzcw2rs21tmfxcvpewx7vrz2s0fr033wjm77fchsbzqy7ygb");
    CAddress a111("1g9pgwqpcbz3x9czsvtjhc3ed0b24vmyrj5k6zfbh1g82ma67qax55dqy");
    CAddress a2("1cem1035c3f7x58808yqge07z8fb72m3czfmzj23hmaxs477tt2ph9yrp");
    CAddress a21("16br4z57ys0dbgxzesvfhtxrpaq3w3j3bsqgdmkrvye06exg9e74ertx8");
    CAddress a22("123zbs56t8q5qn6sjchtvhcxc1ngq5v7jvst6hd2nq7hdtfmr691rd6mb");
    CAddress a221("1pqmm5nj6qpy436qcytshjra24bhvsysjv6k2xswfzma6w1scwyye25r9");
    CAddress a222("12z4t8dzgzmnh3zg1m0b4s2yean8w74cyv0fgrdm7dpq6hbnrq2w0gdwk");
    CAddress a3("1h1tn0dcf7cz44cfypmwqjkm7jvxznrdab2ycn0wcyc9q8983fq3akt40");
    CAddress B("1xftkpw9afcgrfac4v3xa9bgb1zsh03bd59npwtyjrqjbtqepr6anrany");
    CAddress b1("1thcbm7h2bnyb247wknsvnfsxxd7gw12m2bg5d5gdrmzqmxz93s7k6pvy");
    CAddress b2("15n2eszyzk3qm7y04es5fr5fxa3hpbdqqnp1470rmnbmkhpp66mq55qeb");
    CAddress b3("1espce7qwvy94qcanecfr7c65fgsjbr1g407ab00p44a65de0mm1mtrqn");
    CAddress b4("1mbv5et5sx6x1jejgfh214ze1vryg60n8j6tz30czgzmm3rr461jn0nwm");
    CAddress C("1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm");

    map<string, int64> mapReward = map<string, int64>{
        { A.ToString(), 0 },
        { a1.ToString(), 0 },
        { a11.ToString(), 0 },
        { a111.ToString(), 0 },
        { a2.ToString(), 0 },
        { a21.ToString(), 0 },
        { a22.ToString(), 0 },
        { a221.ToString(), 0 },
        { a222.ToString(), 0 },
        { a3.ToString(), 0 },
        { B.ToString(), 0 },
        { b1.ToString(), 0 },
        { b2.ToString(), 0 },
        { b3.ToString(), 0 },
        { b4.ToString(), 0 },
        { C.ToString(), 0 },
    };

    map<CDestination, int64> balance = map<CDestination, int64>{
        { A, 10000 * COIN },
        { a1, 100000 * COIN },
        { a11, 100000 * COIN },
        { a111, 100000 * COIN },
        { a2, 1 * COIN },
        { a21, 1 * COIN },
        { a22, 12000 * COIN },
        { a221, 18000 * COIN },
        { a222, 5000 * COIN },
        { a3, 1000000 * COIN },
        { B, 10000 * COIN },
        { b1, 10000 * COIN },
        { b2, 11000 * COIN },
        { b3, 5000 * COIN },
        { b4, 50000 * COIN },
        { C, 8568998 * COIN },
    };

    map<CDestination, CAddrInfo> mapAddress;
    mapAddress = map<CDestination, CAddrInfo>{
        { a1, CAddrInfo(A, A) },
        { a11, CAddrInfo(A, a1) },
        { a111, CAddrInfo(A, a11) },
        { a2, CAddrInfo(A, A) },
        { a21, CAddrInfo(A, a2) },
        { a22, CAddrInfo(A, a2) },
        { a221, CAddrInfo(A, a22) },
        { a222, CAddrInfo(A, a22) },
        { a3, CAddrInfo(A, A) },
        { b1, CAddrInfo(B, B) },
        { b2, CAddrInfo(B, B) },
        { b3, CAddrInfo(B, B) },
        { b4, CAddrInfo(B, B) },
    };

    CDeFiRelationGraph relation;
    for (auto& x : mapAddress)
    {
        BOOST_CHECK(relation.Insert(x.first, x.second.destParent, x.second.destParent));
    }

    int64 nTotalTxFee = 0;
    int64 nTotalSupply = 0;
    int64 nTotalReward = 0;
    for (int i = 0; i < 3000; i++)
    {
        // for (auto& x : mapReward)
        // {
        //     cout << "begin addr: " << x.first << ", stake: " << balance[CAddress(x.first)] << endl;
        // }

        int32 nHeight = profile.defi.nMintHeight + (i + 1) * profile.defi.nRewardCycle - 1;
        int64 nReward = r.GetSectionReward(forkid, uint256(nHeight, uint224(0)));
        // cout << "height: " << (nHeight + 1) << ", reward: " << nReward << endl;
        int64 nStakeReward = nReward * profile.defi.nStakeRewardPercent / 100;
        CDeFiRewardSet stakeReward = r.ComputeStakeReward(profile.defi.nStakeMinToken, nStakeReward, balance);
        // cout << "stake reward: " << nStakeReward << ", size: " << stakeReward.size() << endl;
        for (auto& x : mapReward)
        {
            auto it = stakeReward.find(CAddress(x.first));
            if (it != stakeReward.end())
            {
                // cout << "stake reward addr: " << x.first << ", reward: " << it->nStakeReward << endl;
            }
        }

        int64 nPromotionReward = nReward * profile.defi.nPromotionRewardPercent / 100;
        CDeFiRewardSet promotionReward = r.ComputePromotionReward(nPromotionReward, balance, profile.defi.mapPromotionTokenTimes, relation, set<CDestination>());
        // cout << "promotion reward: " << nPromotionReward << ", size: " << promotionReward.size() << endl;
        for (auto& x : mapReward)
        {
            auto it = promotionReward.find(CAddress(x.first));
            if (it != promotionReward.end())
            {
                // cout << "promotion reward addr: " << x.first << ", reward: " << it->nPromotionReward << endl;
            }
        }

        CDeFiRewardSet s;
        CDeFiRewardSetByDest& destIdx = promotionReward.get<0>();
        for (auto& stake : stakeReward)
        {
            CDeFiReward reward = stake;
            auto it = destIdx.find(stake.dest);
            if (it != destIdx.end())
            {
                reward.nPower = it->nPower;
                reward.nPromotionReward = it->nPromotionReward;
                reward.nReward += it->nPromotionReward;
                destIdx.erase(it);
            }

            s.insert(move(reward));
        }

        for (auto& promotion : promotionReward)
        {
            CDeFiReward reward = promotion;
            s.insert(move(reward));
        }

        // cout << "height: " << (nHeight + 1) << endl;
        for (auto& x : mapReward)
        {
            auto it = s.find(CAddress(x.first));
            if (it != s.end())
            {
                x.second = it->nReward;
            }
            else
            {
                x.second = 0;
            }

            nTotalReward += x.second;
            if (x.second > MIN_TX_FEE)
            {
                balance[CAddress(x.first)] += x.second - MIN_TX_FEE;
                nTotalTxFee += MIN_TX_FEE;
            }
            // cout << "addr: " << x.first << ", reward: " << x.second << endl;
        }

        if (s.size() == 0)
        {
            cout << "reward is 0, height: " << nHeight << endl;
            break;
        }
    }

    for (auto& x : mapReward)
    {
        cout << "addr: " << x.first << ", reward: " << balance[CAddress(x.first)] << endl;
        nTotalSupply += balance[CAddress(x.first)];
    }
    cout << "total supply: " << (nTotalSupply + nTotalTxFee) << ", total reward:" << nTotalReward << endl;
}

BOOST_AUTO_TEST_CASE(defi_relation_tx)
{
    CCryptoKey sub_key;
    CryptoMakeNewKey(sub_key);
    CAddress sub_address(CDestination(CPubKey(sub_key.pubkey)));
    cout << "sub_privkey: " << sub_key.secret.ToString() << endl;
    cout << "sub_pubkey: " << sub_key.pubkey.ToString() << endl;
    cout << "sub_address: " << sub_address.ToString() << endl;
    CCryptoKey parent_key;
    CryptoMakeNewKey(parent_key);
    CAddress parent_address(CDestination(CPubKey(parent_key.pubkey)));
    cout << "parent_privkey: " << parent_key.secret.ToString() << endl;
    cout << "parent_pubkey: " << parent_key.pubkey.ToString() << endl;
    cout << "parent_address: " << parent_address.ToString() << endl;
    CCryptoKey shared_key;
    CryptoMakeNewKey(shared_key);
    cout << "shared_privkey: " << shared_key.secret.ToString() << endl;
    cout << "shared_pubkey: " << shared_key.pubkey.ToString() << endl;

    uint256 forkid("00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70");
    cout << "forkid: " << forkid.ToString() << endl;

    string sub_sign_str = string("DeFiRelation") + forkid.ToString() + shared_key.pubkey.ToString();
    cout << "sub_sign_str: " << sub_sign_str << endl;
    uint256 sub_sign_hash_str = CryptoHash(sub_sign_str.data(), sub_sign_str.size());
    cout << "sub_sign_hash_str: " << ToHexString(sub_sign_hash_str.begin(), sub_sign_hash_str.size()) << endl;
    vector<uint8> sub_sign;
    CryptoSign(sub_key, sub_sign_hash_str.begin(), sub_sign_hash_str.size(), sub_sign);
    cout << "sub_sign: " << ToHexString(sub_sign) << endl;

    string parent_sign_str = string("DeFiRelation") + parent_key.pubkey.ToString();
    cout << "parent_sign_str: " << parent_sign_str << endl;
    uint256 parent_sign_hash_str = CryptoHash(parent_sign_str.data(), parent_sign_str.size());
    cout << "parent_sign_hash_str: " << ToHexString(parent_sign_hash_str.begin(), parent_sign_hash_str.size()) << endl;
    vector<uint8> parent_sign;
    CryptoSign(shared_key, parent_sign_hash_str.begin(), parent_sign_hash_str.size(), parent_sign);
    cout << "parent_sign: " << ToHexString(parent_sign) << endl;

    vector<uint8> vchData;
    vchData.insert(vchData.end(), shared_key.pubkey.begin(), shared_key.pubkey.end());
    cout << "vchData (shared_pubkey): " << ToHexString(vchData) << endl;
    vchData.insert(vchData.end(), sub_sign.begin(), sub_sign.end());
    cout << "vchData (shared_pubkey+sub_sign): " << ToHexString(vchData) << endl;
    vchData.insert(vchData.end(), parent_sign.begin(), parent_sign.end());
    cout << "vchData (shared_pubkey + sub_sign + parent_sign): " << ToHexString(vchData) << endl;

    BOOST_CHECK(vchData.size() == 160);
    uint256 verify_shared_pubKey(vector<uint8>(vchData.begin(), vchData.begin() + 32));
    BOOST_CHECK(verify_shared_pubKey == shared_key.pubkey);
    vector<uint8> verify_sub_sign(vchData.begin() + 32, vchData.begin() + 96);
    BOOST_CHECK(verify_sub_sign == sub_sign);
    vector<uint8> verify_parent_sign(vchData.begin() + 96, vchData.end());
    BOOST_CHECK(verify_parent_sign == parent_sign);

    BOOST_CHECK(CryptoVerify(sub_key.pubkey, sub_sign_hash_str.begin(), sub_sign_hash_str.size(), verify_sub_sign));
    BOOST_CHECK(CryptoVerify(verify_shared_pubKey, parent_sign_hash_str.begin(), parent_sign_hash_str.size(), verify_parent_sign));

    uint256 invalidKey("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    cout << crypto_core_ed25519_is_valid_point(invalidKey.begin()) << endl;
}

BOOST_AUTO_TEST_SUITE_END()
