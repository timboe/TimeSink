#pragma once
#include <pebble.h>

uint8_t getItemRarity(TimeUnits units_changed);

void checkForItem(TimeUnits units_changed);

bool getItemAutoCollect();
