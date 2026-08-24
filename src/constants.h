#pragma once

#include <Arduino.h>

// Button pin assignments
constexpr uint8_t PIN_PLUS_1 = 2;           // Pin D2 on Arduino, Btn3 on PCB
constexpr uint8_t PIN_MINUS_1 = 3;          // Pin D3 on Arduino, Btn2 on PCB
constexpr uint8_t PIN_JOG = 4;              // Pin D4 on Arduino, Btn1 on PCB
constexpr uint8_t PIN_START_PAUSE_STOP = 5; // Pin D5 on Arduino, Btn4 on PCB

// Motor pin assignments
constexpr uint8_t MOTOR_PUL = 9;  // Pin D9 on Arduino
constexpr uint8_t MOTOR_DIR = 10; // Pin D10 on Arduino
constexpr uint8_t MOTOR_ENA = 11; // Pin D11 on Arduino

// Motor and gearbox config
constexpr uint32_t MOTOR_PULSES_PER_REVOLUTION = 200;
constexpr uint32_t MOTOR_GEAR_RATIO = 10;
constexpr uint32_t SYSTEM_GEAR_RATIO = 63;

constexpr uint32_t PULSES_PER_OUTPUT_ROTATION =
    MOTOR_PULSES_PER_REVOLUTION * MOTOR_GEAR_RATIO * SYSTEM_GEAR_RATIO;

// Motor pulse rates 
constexpr uint32_t RUN_PULSE_RATE = 9450;
constexpr uint32_t JOG_PULSE_RATE = 7350;
constexpr uint32_t STARTING_PULSE_RATE = 333;

// Motor acceleration and deceleration durations
constexpr unsigned long RUN_ACCELERATION_TIME_MS = 4000;
constexpr unsigned long RUN_DECELERATION_TIME_MS = 4000;
constexpr unsigned long JOG_ACCELERATION_TIME_MS = 1000; // should be changed?? idk why its slower

// Motor ramp configuration
constexpr uint32_t RUN_DECELERATION_PULSES = 19600;
constexpr unsigned long SPEED_UPDATE_INTERVAL_MS = 10;

// Button timing
constexpr unsigned long ROTATION_LONG_PRESS_MS = 1000;
constexpr unsigned long BUTTON_LONG_PRESS_UPDATE_MS = 10;
constexpr unsigned long JOG_LONG_PRESS_MS = 500;
constexpr unsigned long STOP_LONG_PRESS_MS = 3000;

// Battery voltage sensing
constexpr uint8_t BATTERY_SENSE_PIN = A0;
constexpr uint32_t BATTERY_DIVIDER_R1_OHMS = 100000;
constexpr uint32_t BATTERY_DIVIDER_R2_OHMS = 20000;