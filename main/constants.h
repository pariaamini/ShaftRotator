#pragma once

#include <Arduino.h>

// Button pins
constexpr uint8_t PIN_UP = 2;
constexpr uint8_t PIN_DOWN = 3;
constexpr uint8_t PIN_JOG = 6;
constexpr uint8_t PIN_START_PAUSE_STOP = 4; // aligns to +5 button

// Button timing
constexpr unsigned long ROTATION_LONG_PRESS_MS = 1000;
constexpr unsigned long BUTTON_LONG_PRESS_UPDATE_MS = 10;
constexpr unsigned long JOG_LONG_PRESS_MS = 500;
constexpr unsigned long STOP_LONG_PRESS_MS = 2000;

constexpr int STATUS_LED_PIN = 7;
constexpr int DEBUG_LED_PIN = 13;


// Battery Constants

const int BAT_PIN = A7; 
const float R1 = 100000.0;       
const float R2 = 20000.0;       
const uint32_t ADC_REF_mV = 5000; 
// uint32_t lowBat_mV = 6 * 3300;  //Do not let pack go under this voltage. Update this as needed. 3300mv minimum per cell, 6 cells.
// Battery is nonlinear so a value table is used
