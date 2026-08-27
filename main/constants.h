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

// unsigned long lastTargetRotationChangeMs = 0;