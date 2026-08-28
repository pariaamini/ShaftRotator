#pragma once

#include <Arduino.h>

// Button Pins (comment/uncomment corresponding section depending on what version is used)

// ORIGNAL SHAFT ROTATOR BOARD PINS (buttons cannot be assigned to pin 5, wiring/connection issue present)
constexpr uint8_t PIN_UP = 2;
constexpr uint8_t PIN_DOWN = 3;
constexpr uint8_t PIN_JOG = 6;
constexpr uint8_t PIN_START_PAUSE_STOP = 4; // aligns to button labeled +5  on rotater

// // NEW SHAFT ROTATOR BOARD PINS
// constexpr uint8_t PIN_UP = 2;               // SW3 on PCB
// constexpr uint8_t PIN_DOWN = 5;             // SW2 on PCB
// constexpr uint8_t PIN_JOG = 4;              // SW1 on PCB
// constexpr uint8_t PIN_START_PAUSE_STOP = 3; // SW4 on PCB


// Button timing
constexpr unsigned long ROTATION_LONG_PRESS_MS = 500;    // btn hold time before +/- long press procedure starts
constexpr unsigned long BUTTON_LONG_PRESS_UPDATE_MS = 10; // how often the long press procedure runs when btn is held
constexpr unsigned long JOG_LONG_PRESS_MS = 500;          // btn hold time before jog procedure starts
constexpr unsigned long JOG_DOUBLE_CLICK_DELAY_MS = 200;  // max time between clicks to register as a double-click
constexpr unsigned long STOP_LONG_PRESS_MS = 2000;        // btn hold time before stop procedure runs

// Single-click runs only after the double-click waiting interval passes with no second click.
// ***This can cause a delay before the single-click procedure runs.


// Accelerating rotation-change timing
// As +/- is held, target rotation changes faster
// ACCEL_STAGE constants define when the speed changes
// ROT_CHANGE constants define how often the target rotation changes by 1

constexpr unsigned long ACCEL_STAGE_1_END_MS = 1000; // slow speed ends after 1s
constexpr unsigned long ACCEL_STAGE_2_END_MS = 2000; // medium speed ends after 2s

constexpr unsigned long ROT_CHANGE_SLOW_MS = 350;   // change by 1 every 350ms
constexpr unsigned long ROT_CHANGE_MEDIUM_MS = 150; //  change by 1 every 150ms
constexpr unsigned long ROT_CHANGE_FAST_MS = 80;   // change by 1 every 80ms



constexpr int STATUS_LED_PIN = 7;
constexpr int DEBUG_LED_PIN = 13;

// Battery Constants
const int BAT_PIN = A7;
const float R1 = 100000.0;
const float R2 = 20000.0;
const uint32_t ADC_REF_mV = 5000;
// uint32_t lowBat_mV = 6 * 3300;  //Do not let pack go under this voltage. Update this as needed. 3300mv minimum per cell, 6 cells.
