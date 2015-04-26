#pragma once
#include <pebble.h>
#include "constants.h"  
  
  
  
// User's current
uint64_t s_userLTime; 
uint64_t s_userLTimeCapacity;

void init_timeStore();
void destroy_timeStore();

void time_to_string(uint64_t time, char* buffer, size_t buffer_size, bool brief);

void percentage_to_string(uint64_t amount, uint64_t total, char* buffer, unsigned* value);

uint64_t getPriceOfUpgrade(const unsigned typeID, const unsigned resourceID, const unsigned alreadyOwned);