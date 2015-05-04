#include <pebble.h>
#include "constants.h"
  
const uint64_t INITIAL_PRICE_REFINERY[MAX_UPGRADES] = {
  SEC_IN_MIN,
  SEC_IN_MIN*2,
  SEC_IN_MIN*10,
  SEC_IN_MIN*60,
  SEC_IN_MIN*100,
  SEC_IN_MIN*500,
  SEC_IN_MIN*1000,
  SEC_IN_MIN*5000,
  SEC_IN_MIN*10000,
  SEC_IN_MIN*50000,
  SEC_IN_MIN*100000,
  SEC_IN_MIN*500000,
  SEC_IN_MIN*1000000,
  SEC_IN_MIN*5000000,
  SEC_IN_MIN*10000000,
  SEC_IN_MIN*50000000};

const uint64_t REWARD_REFINERY[MAX_UPGRADES] = {
  1,
  5,
  40,
  100,
  400,
  1000,
  4000,
  50000,
  100000,
  1000000,
  10000000,
  1,
  1,
  1,
  1,
  1};

const char* const NAME_REFINERY[MAX_UPGRADES] = {
  "Time Tongs",
  "UPGRADE 2",
  "UPGRADE 3",
  "UPGRADE 4",
  "UPGRADE 5",
  "UPGRADE 6",
  "UPGRADE 7",
  "UPGRADE 8",
  "UPGRADE 9",
  "UPGRADE 10",
  "UPGRADE 11",
  "UPGRADE 12",
  "UPGRADE 13",
  "UPGRADE 14",
  "UPGRADE 15",
  "UPGRADE 16"};

const uint64_t INITIAL_PRICE_SIEVE[MAX_UPGRADES] = {
  SEC_IN_MIN,
  SEC_IN_MIN*2,
  SEC_IN_MIN*10,
  SEC_IN_MIN*60,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0};

const uint64_t REWARD_SIEVE[MAX_UPGRADES] = {
  SEC_IN_MIN,
  SEC_IN_MIN*2,
  SEC_IN_MIN*10,
  SEC_IN_MIN*60,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0};

const char* const NAME_SIEVE[MAX_UPGRADES] = {
  "Extra Eyes",
  "SUPGRADE 2",
  "SUPGRADE 3",
  "SUPGRADE 4",
  "SUPGRADE 5",
  "SUPGRADE 6",
  "SUPGRADE 7",
  "SUPGRADE 8",
  "SUPGRADE 9",
  "SUPGRADE 10",
  "SUPGRADE 11",
  "SUPGRADE 12",
  "SUPGRADE 13",
  "SUPGRADE 14",
  "SUPGRADE 15",
  "SUPGRADE 16"};

const uint64_t INITIAL_PRICE_TANK[MAX_UPGRADES] = {
  SEC_IN_MIN,
  SEC_IN_MIN*2,
  SEC_IN_MIN*10,
  SEC_IN_MIN*60,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0};

const uint64_t REWARD_TANK[MAX_UPGRADES] = {
  SEC_IN_MIN,
  SEC_IN_MIN*2,
  SEC_IN_MIN*10,
  SEC_IN_MIN*60,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0};

const char* const NAME_TANK[MAX_UPGRADES] = {
  "Minute Tank",
  "TUPGRADE 2",
  "TUPGRADE 3",
  "TUPGRADE 4",
  "TUPGRADE 5",
  "TUPGRADE 6",
  "TUPGRADE 7",
  "TUPGRADE 8",
  "TUPGRADE 9",
  "TUPGRADE 10",
  "TUPGRADE 11",
  "TUPGRADE 12",
  "TUPGRADE 13",
  "TUPGRADE 14",
  "TUPGRADE 15",
  "TUPGRADE 16"};

const uint64_t INITIAL_PRICE_WATCHER[MAX_UPGRADES] = {
  SEC_IN_MIN,
  SEC_IN_MIN*2,
  SEC_IN_MIN*10,
  SEC_IN_MIN*60,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0};

const uint64_t REWARD_WATCHER[MAX_UPGRADES] = {
  SEC_IN_MIN,
  SEC_IN_MIN*2,
  SEC_IN_MIN*10,
  SEC_IN_MIN*60,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0,
  (uint64_t)0};

const char* const NAME_WATCHER[MAX_UPGRADES] = {
  "Lacky",
  "WUPGRADE 2",
  "WUPGRADE 3",
  "WUPGRADE 4",
  "WUPGRADE 5",
  "WUPGRADE 6",
  "WUPGRADE 7",
  "WUPGRADE 8",
  "WUPGRADE 9",
  "WUPGRADE 10",
  "WUPGRADE 11",
  "WUPGRADE 12",
  "WUPGRADE 13",
  "WUPGRADE 14",
  "WUPGRADE 15",
  "WUPGRADE 16"};

const uint64_t SELL_PRICE_COMMON[MAX_TREASURES] = {
  SEC_IN_MIN*10,
  SEC_IN_MIN*30,
  SEC_IN_HOUR*2,
  SEC_IN_HOUR*6};

const uint64_t SELL_PRICE_MAGIC[MAX_TREASURES] = {
  SEC_IN_HOUR*18,
  SEC_IN_DAY*5,
  SEC_IN_DAY*20,
  SEC_IN_DAY*30*2};

const uint64_t SELL_PRICE_RARE[MAX_TREASURES] = {
  SEC_IN_DAY*31*7,
  SEC_IN_YEAR*3,
  SEC_IN_YEAR*15,
  SEC_IN_YEAR*75};

const uint64_t SELL_PRICE_EPIC[MAX_TREASURES] = {
  SEC_IN_YEAR*200,
  SEC_IN_YEAR*500,
  SEC_IN_MILLENIUM*1,
  SEC_IN_MILLENIUM*5};

const char* const NAME_COMMON[MAX_TREASURES] = {
  "COMMON 1",
  "COMMON 2",
  "COMMON 3",
  "COMMON 4"};

const char* const NAME_MAGIC[MAX_TREASURES] = {
  "COMMON 1",
  "COMMON 2",
  "COMMON 3",
  "COMMON 4"};

const char* const NAME_RARE[MAX_TREASURES] = {
  "COMMON 1",
  "COMMON 2",
  "COMMON 3",
  "COMMON 4"};

const char* const NAME_EPIC[MAX_TREASURES] = {
  "COMMON 1",
  "COMMON 2",
  "COMMON 3",
  "COMMON 4"};

const char* const NAME_LEGENDARY[MAX_UNIQUE] = {
  "LEG 1",
  "LEG 2",
  "LEG 3",
  "LEG 4",
  "LEG 5",
  "LEG 6",
  "LEG 7",
  "LEG 8",
  "LEG 9",
  "LEG 10",
  "LEG 11",
  "LEG 12",
  "LEG 13",
  "LEG 14",
  "LEG 15",
  "LEG 16"};

  