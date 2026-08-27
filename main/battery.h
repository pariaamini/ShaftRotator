#pragma once
#include <Arduino.h>

void setupBattery();
uint32_t readBatteryVoltage_mV();
uint8_t getBatteryPercent();

void showBatteryPercentage();