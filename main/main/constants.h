#pragma once

#include <Arduino.h>

// Jog button
constexpr uint8_t PIN_JOG = 4;

// Motor connections
constexpr uint8_t MOTOR_PUL = 9;
constexpr uint8_t MOTOR_DIR = 10;
constexpr uint8_t MOTOR_ENA = 11;

// Motor pulse rates, in pulses per second
constexpr uint32_t STARTING_PULSE_RATE = 333;
constexpr uint32_t JOG_PULSE_RATE = 7350;

// Jog acceleration settings
constexpr unsigned long JOG_ACCELERATION_TIME_MS = 1000;
constexpr unsigned long SPEED_UPDATE_INTERVAL_MS = 10;

// Button must be held this long before jogging begins
constexpr unsigned long JOG_LONG_PRESS_MS = 500;