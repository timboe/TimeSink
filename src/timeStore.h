#pragma once
#include <pebble.h>
#include "constants.h"  
  
void init_timeStore(); 
void destroy_timeStore();

void timeToString(uint64_t time, char* buffer, size_t buffer_size, bool brief);
void percentageToString(uint64_t amount, uint64_t total, char* buffer, const size_t buffer_size, unsigned* value);

uint64_t getPriceOfUpgrade(const unsigned typeID, const unsigned resourceID);
uint64_t getPriceOfNext(uint64_t priceOfCurrent);

bool doPurchase(const unsigned typeID, const unsigned resourceID);

uint64_t getTimePerMin();
void updateTimePerMin();

uint64_t getTankCapacity();
void updateTankCapacity();

void modulateSellPrices();
uint64_t getCurrentSellPrice();
void currentSellPricePercentage(char* buffer, const size_t buffer_size,  unsigned* value, const unsigned treasureID, const unsigned itemID);
uint64_t currentCategorySellPrice(const unsigned treasureID);

void addTime(uint64_t toAdd);
void removeTime(uint64_t toSubtract);

uint64_t getDisplayTime();
void updateDisplayTime(uint64_t t);
