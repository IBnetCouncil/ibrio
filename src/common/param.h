// Copyright (c) 2019-2021 The Ibrio developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IBRIO_PARAM_H
#define IBRIO_PARAM_H

#include "uint256.h"

///////////////////////////////////
extern bool TESTNET_FLAG;
#define GET_PARAM(MAINNET_PARAM, TESTNET_PARAM) (TESTNET_FLAG ? TESTNET_PARAM : MAINNET_PARAM)

///////////////////////////////////
static const int64 COIN = 1000000;
static const int64 MIN_TX_FEE = 10000;
static const int64 MAX_MONEY = 1000000000000 * COIN;
inline bool MoneyRange(int64 nValue)
{
    return (nValue >= 0 && nValue <= MAX_MONEY);
}
static const int64 MAX_REWARD_MONEY = 1000 * 10000 * COIN;
inline bool RewardRange(int64 nValue)
{
    return (nValue >= 0 && nValue <= MAX_REWARD_MONEY);
}
inline double ValueFromToken(uint64 amount)
{
    return ((double)amount / (double)COIN);
}

static const unsigned int MAX_BLOCK_SIZE = 2000000;
static const unsigned int MAX_TX_SIZE = (MAX_BLOCK_SIZE / 20);
static const unsigned int MAX_SIGNATURE_SIZE = 2048;
static const unsigned int MAX_TX_INPUT_COUNT = (MAX_TX_SIZE - MAX_SIGNATURE_SIZE - 4) / 33;

static const unsigned int BLOCK_TARGET_SPACING = 10;  //60; // 1-minute block spacing
static const unsigned int EXTENDED_BLOCK_SPACING = 1; //2;
static const unsigned int PROOF_OF_WORK_BLOCK_SPACING = BLOCK_TARGET_SPACING;

static const unsigned int DAY_HEIGHT = 24 * 60 * 60 / BLOCK_TARGET_SPACING;
static const unsigned int MONTH_HEIGHT = 30 * DAY_HEIGHT;
static const unsigned int YEAR_HEIGHT = 12 * MONTH_HEIGHT;

///////////////////////////////////
// DELEGATE
#define CONSENSUS_DISTRIBUTE_INTERVAL GET_PARAM(5, 3) //15
#define CONSENSUS_ENROLL_INTERVAL GET_PARAM(10, 6)    //30
#define CONSENSUS_INTERVAL (CONSENSUS_DISTRIBUTE_INTERVAL + CONSENSUS_ENROLL_INTERVAL + 1)

#define MAX_DELEGATE_THRESH (25)

///////////////////////////////////
// FORK
const int64 MORTGAGE_BASE = 10000 * COIN;          // initial mortgage. Change from 100000 * COIN to 10000 * COIN on about 565620 height
#define MORTGAGE_DECAY_CYCLE GET_PARAM(129600, 20) //60 * 24 * 90 = 129600; // decay cycle
const double MORTGAGE_DECAY_QUANTITY = 0.5;        // decay quantity

#define MIN_CREATE_FORK_INTERVAL_HEIGHT GET_PARAM(30, 0)
#define MAX_JOINT_FORK_INTERVAL_HEIGHT GET_PARAM(DAY_HEIGHT, 0x7FFFFFFF)

#define DISABLE_FORK_CREATETION //Disable fork creation

///////////////////////////////////
// CORE
static const int64 MAX_CLOCK_DRIFT = 80;

static const int64 NPSP_TOKEN_INIT = 20000 * 10000 * COIN;

static const int64 DELEGATE_PROOF_OF_STAKE_ENROLL_MINIMUM_AMOUNT = 1000 * 10000 * COIN;
#define DELEGATE_PROOF_OF_STAKE_ENROLL_MAXIMUM_AMOUNT GET_PARAM((10000 * 10000 * COIN), NPSP_TOKEN_INIT)
static const int64 DELEGATE_PROOF_OF_STAKE_UNIT_AMOUNT = 1000 * COIN;
static const int64 DELEGATE_PROOF_OF_STAKE_MAXIMUM_TIMES = 100 * 10000 * COIN;

///////////////////////////////////
enum ConsensusMethod
{
    CM_MPVSS = 0,
    CM_CRYPTONIGHT = 1,
    CM_MAX
};

///////////////////////////////////
// FUNCTOIN
static const int64 INVEST_UNIT_AMOUNT = 100 * COIN;
static const int64 MIN_ACTIVATE_AMOUNT = 20 * COIN;
static const int64 MINT_REWARD_PER = 100;
static const int64 MINT_REWARD_RATIO = 5;
static const int64 USER_REWARD_RATIO = (MINT_REWARD_PER - MINT_REWARD_RATIO);

#define ACTIVATE_CONFIRM_HEIGHT GET_PARAM(6, 1)    //6
#define INVEST_DAY_HEIGHT GET_PARAM(DAY_HEIGHT, 2) //DAY_HEIGHT;

//7=1%, 14=2.2%, 30=4.9%, 90=16%
static const int64 nInterestPer = 10000;
static const std::map<uint32, int64> mapInterestRule = {
    { 7, 100 },
    { 14, 220 },
    { 30, 490 },
    { 90, 1600 },
};

inline int64 CalcHeightInvestReward(const uint32 nCycle)
{
    auto it = mapInterestRule.find(nCycle);
    if (it == mapInterestRule.end())
    {
        return 0;
    }
    return ((INVEST_UNIT_AMOUNT * it->second / nInterestPer) / (nCycle * INVEST_DAY_HEIGHT));
}

inline int64 CalcInvestReward(const uint32 nCycle, const int64 nInvestAmount)
{
    auto it = mapInterestRule.find(nCycle);
    if (it == mapInterestRule.end())
    {
        return 0;
    }
    return (nInvestAmount * it->second / nInterestPer);
}

static const std::map<uint256, std::map<int, uint256>> mapCheckPointsList_Mainnet = {
    { uint256("00000000312731792b03593ba4a4fa476f8db31b8a02ee45a2c5a7acf9112bbf"), //Genesis
      { { 0, uint256("00000000312731792b03593ba4a4fa476f8db31b8a02ee45a2c5a7acf9112bbf") } } }
};
static const std::map<uint256, std::map<int, uint256>> mapCheckPointsList_Testnet = {
    { uint256("00000000a8b00ba26899af9bab3c44898976102504972b71bd7298d99420719b"), //Genesis
      { { 0, uint256("00000000a8b00ba26899af9bab3c44898976102504972b71bd7298d99420719b") } } }
};
#define mapCheckPointsList GET_PARAM(mapCheckPointsList_Mainnet, mapCheckPointsList_Testnet)

#endif //IBRIO_PARAM_H
