// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txpool.h"

#include <boost/test/unit_test.hpp>

#include "test_big.h"
#include "transaction.h"
#include "uint256.h"

using namespace std;
using namespace xengine;
using namespace ibrio;

BOOST_FIXTURE_TEST_SUITE(txpool_tests, BasicUtfSetup)

BOOST_AUTO_TEST_CASE(txcache_test)
{
    CTxCache cache(5);

    std::vector<CTransaction> vecTx;
    cache.AddNew(uint256(1, uint224()), vecTx);
    BOOST_CHECK(cache.Retrieve(uint256(1, uint224()), vecTx) == true);

    cache.AddNew(uint256(3, uint224()), vecTx);
    BOOST_CHECK(cache.Retrieve(uint256(1, uint224()), vecTx) == true);
    BOOST_CHECK(cache.Retrieve(uint256(3, uint224()), vecTx) == true);

    cache.AddNew(uint256(6, uint224()), vecTx);
    BOOST_CHECK(cache.Retrieve(uint256(1, uint224()), vecTx) == false);
    BOOST_CHECK(cache.Retrieve(uint256(3, uint224()), vecTx) == true);
    BOOST_CHECK(cache.Retrieve(uint256(6, uint224()), vecTx) == true);

    cache.AddNew(uint256(4, uint224()), vecTx);
    cache.AddNew(uint256(2, uint224()), vecTx);
    cache.AddNew(uint256(5, uint224()), vecTx);
    cache.AddNew(uint256(1, uint224()), vecTx);

    BOOST_CHECK(cache.Retrieve(uint256(1, uint224()), vecTx) == false);
    BOOST_CHECK(cache.Retrieve(uint256(2, uint224()), vecTx) == true);
    BOOST_CHECK(cache.Retrieve(uint256(6, uint224()), vecTx) == true);
    BOOST_CHECK(cache.Retrieve(uint256(4, uint224()), vecTx) == true);

    cache.Clear();
    vecTx.clear();

    cache.AddNew(uint256(125, uint224("1asfasf")), vecTx);
    cache.AddNew(uint256(125, uint224("awdaweawrawfasdadawd")), vecTx);
    cache.AddNew(uint256(126, uint224("126")), vecTx);
    BOOST_CHECK(cache.Retrieve(uint256(125, uint224("1asfasf")), vecTx) == true);
    BOOST_CHECK(cache.Retrieve(uint256(125, uint224("awdaweawrawfasdadawd")), vecTx) == true);

    cache.AddNew(uint256(130, uint224()), vecTx);
    cache.AddNew(uint256(120, uint224()), vecTx);
    cache.AddNew(uint256(110, uint224()), vecTx);
    cache.AddNew(uint256(110, uint224()), vecTx);
    cache.AddNew(uint256(115, uint224()), vecTx);

    BOOST_CHECK(cache.Retrieve(uint256(120, uint224()), vecTx) == false);
    BOOST_CHECK(cache.Retrieve(uint256(210, uint224()), vecTx) == false);
    BOOST_CHECK(cache.Retrieve(uint256(130, uint224()), vecTx) == true);
    BOOST_CHECK(cache.Retrieve(uint256(125, uint224()), vecTx) == false);
    BOOST_CHECK(cache.Retrieve(uint256(125, uint224("1asfasf")), vecTx) == false);
    BOOST_CHECK(cache.Retrieve(uint256(125, uint224("awdaweawrawfasdadawd")), vecTx) == false);
}

static uint64 GetSequenceNumber()
{
    static uint64 nLastSequenceNumber = 0;
    return ((++nLastSequenceNumber) << 24);
}

// tx1
//  |
// tx2
//  |
// tx3
BOOST_AUTO_TEST_CASE(seq_test)
{
    CTxPoolView view;

    CPooledTx tx1;
    tx1.nTimeStamp = 1;

    CPooledTx tx2;
    tx2.nTimeStamp = 2;
    tx2.vInput.push_back(CTxIn(CTxOutPoint(tx1.GetHash(), 0)));

    CPooledTx tx3;
    tx3.nTimeStamp = 3;
    tx3.vInput.push_back(CTxIn(CTxOutPoint(tx2.GetHash(), 0)));

    tx3.nSequenceNumber = GetSequenceNumber();
    tx1.nSequenceNumber = GetSequenceNumber();
    tx2.nSequenceNumber = GetSequenceNumber();

    BOOST_CHECK(view.AddNew(tx3.GetHash(), tx3));
    BOOST_CHECK(view.AddNew(tx1.GetHash(), tx1));
    BOOST_CHECK(view.AddNew(tx2.GetHash(), tx2));
}

// tx1         tx4
//  |           |
// tx2    tx5  tx6   tx7
//  |      |    |     |
// tx3     --- tx8-----    tx9
//  |           |          |
//  |---------tx10---------|
BOOST_AUTO_TEST_CASE(txpoolview_test)
{
    CTxPoolView view;

    CPooledTx tx1;
    tx1.nTimeStamp = 1001;
    tx1.nSequenceNumber = GetSequenceNumber();

    CPooledTx tx2;
    tx2.nTimeStamp = 1002;
    tx2.nSequenceNumber = GetSequenceNumber();
    tx2.vInput.push_back(CTxIn(CTxOutPoint(tx1.GetHash(), 0)));

    CPooledTx tx3;
    tx3.nTimeStamp = 1003;
    tx3.nSequenceNumber = GetSequenceNumber();
    tx3.vInput.push_back(CTxIn(CTxOutPoint(tx2.GetHash(), 0)));

    CPooledTx tx4;
    tx4.nTimeStamp = 1004;
    tx4.nSequenceNumber = GetSequenceNumber();

    CPooledTx tx5;
    tx5.nTimeStamp = 1005;
    tx5.nSequenceNumber = GetSequenceNumber();

    CPooledTx tx6;
    tx6.nTimeStamp = 1006;
    tx6.nSequenceNumber = GetSequenceNumber();
    tx6.vInput.push_back(CTxIn(CTxOutPoint(tx4.GetHash(), 0)));

    CPooledTx tx7;
    tx7.nTimeStamp = 1007;
    tx7.nSequenceNumber = GetSequenceNumber();

    CPooledTx tx8;
    tx8.nTimeStamp = 1008;
    tx8.nSequenceNumber = GetSequenceNumber();
    tx8.vInput.push_back(CTxIn(CTxOutPoint(tx5.GetHash(), 0)));
    tx8.vInput.push_back(CTxIn(CTxOutPoint(tx6.GetHash(), 0)));
    tx8.vInput.push_back(CTxIn(CTxOutPoint(tx7.GetHash(), 0)));

    CPooledTx tx9;
    tx9.nTimeStamp = 1009;
    tx9.nSequenceNumber = GetSequenceNumber();

    CPooledTx tx10;
    tx10.nTimeStamp = 1010;
    tx10.nSequenceNumber = GetSequenceNumber();
    tx10.vInput.push_back(CTxIn(CTxOutPoint(tx3.GetHash(), 0)));
    tx10.vInput.push_back(CTxIn(CTxOutPoint(tx8.GetHash(), 0)));
    tx10.vInput.push_back(CTxIn(CTxOutPoint(tx9.GetHash(), 0)));

    BOOST_CHECK(view.AddNew(tx1.GetHash(), tx1));
    BOOST_CHECK(view.AddNew(tx2.GetHash(), tx2));
    BOOST_CHECK(view.AddNew(tx3.GetHash(), tx3));
    BOOST_CHECK(view.AddNew(tx4.GetHash(), tx4));
    BOOST_CHECK(view.AddNew(tx5.GetHash(), tx5));
    BOOST_CHECK(view.AddNew(tx6.GetHash(), tx6));
    BOOST_CHECK(view.AddNew(tx7.GetHash(), tx7));
    BOOST_CHECK(view.AddNew(tx8.GetHash(), tx8));
    BOOST_CHECK(view.AddNew(tx9.GetHash(), tx9));
    BOOST_CHECK(view.AddNew(tx10.GetHash(), tx10));

    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx1.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx2.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx3.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx4.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx5.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx6.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx7.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx8.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx9.GetHash(), 0)));
    BOOST_CHECK(!view.IsSpent(CTxOutPoint(tx10.GetHash(), 0)));

    CTxPoolView involvedTxPoolView;
    view.InvalidateSpent(CTxOutPoint(tx1.GetHash(), 0), involvedTxPoolView);

    BOOST_CHECK(!view.IsSpent(CTxOutPoint(tx1.GetHash(), 0)));
    BOOST_CHECK(!view.IsSpent(CTxOutPoint(tx2.GetHash(), 0)));
    BOOST_CHECK(!view.IsSpent(CTxOutPoint(tx3.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx4.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx5.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx6.GetHash(), 0)));
    BOOST_CHECK(view.IsSpent(CTxOutPoint(tx7.GetHash(), 0)));
    BOOST_CHECK(!view.IsSpent(CTxOutPoint(tx8.GetHash(), 0)));
    BOOST_CHECK(!view.IsSpent(CTxOutPoint(tx9.GetHash(), 0)));
    BOOST_CHECK(!view.IsSpent(CTxOutPoint(tx10.GetHash(), 0)));

    BOOST_CHECK(view.Exists(tx1.GetHash()));
    BOOST_CHECK(!view.Exists(tx2.GetHash()));
    BOOST_CHECK(!view.Exists(tx3.GetHash()));
    BOOST_CHECK(view.Exists(tx4.GetHash()));
    BOOST_CHECK(view.Exists(tx5.GetHash()));
    BOOST_CHECK(view.Exists(tx6.GetHash()));
    BOOST_CHECK(view.Exists(tx7.GetHash()));
    BOOST_CHECK(view.Exists(tx8.GetHash()));
    BOOST_CHECK(view.Exists(tx9.GetHash()));
    BOOST_CHECK(!view.Exists(tx10.GetHash()));

    BOOST_CHECK(!involvedTxPoolView.Exists(tx1.GetHash()));
    BOOST_CHECK(involvedTxPoolView.Exists(tx2.GetHash()));
    BOOST_CHECK(involvedTxPoolView.Exists(tx3.GetHash()));
    BOOST_CHECK(!involvedTxPoolView.Exists(tx4.GetHash()));
    BOOST_CHECK(!involvedTxPoolView.Exists(tx5.GetHash()));
    BOOST_CHECK(!involvedTxPoolView.Exists(tx6.GetHash()));
    BOOST_CHECK(!involvedTxPoolView.Exists(tx7.GetHash()));
    BOOST_CHECK(!involvedTxPoolView.Exists(tx8.GetHash()));
    BOOST_CHECK(!involvedTxPoolView.Exists(tx9.GetHash()));
    BOOST_CHECK(involvedTxPoolView.Exists(tx10.GetHash()));

    BOOST_CHECK(involvedTxPoolView.IsSpent(CTxOutPoint(tx2.GetHash(), 0)));
    BOOST_CHECK(involvedTxPoolView.IsSpent(CTxOutPoint(tx3.GetHash(), 0)));
    BOOST_CHECK(!involvedTxPoolView.IsSpent(CTxOutPoint(tx10.GetHash(), 0)));
}
BOOST_AUTO_TEST_SUITE_END()