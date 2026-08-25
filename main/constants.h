#pragma once

#include <Arduino.h>

// Button pins
constexpr uint8_t PIN_UP = 2;
constexpr uint8_t PIN_DOWN = 3;
constexpr uint8_t PIN_JOG = 4;
constexpr uint8_t PIN_START_PAUSE_STOP = 5;

// Allowed rotation range
constexpr int MIN_ROTATIONS = 0;
constexpr int MAX_ROTATIONS = 99; // not essential

// Button timing
constexpr unsigned long ROTATION_LONG_PRESS_MS = 1000;
constexpr unsigned long BUTTON_LONG_PRESS_UPDATE_MS = 10;
constexpr unsigned long JOG_LONG_PRESS_MS = 500;
constexpr unsigned long STOP_LONG_PRESS_MS = 3000;