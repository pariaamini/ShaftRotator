#pragma once

#include <Arduino.h>

// Current operating state of the shaft rotator.
enum SystemState {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_JOG
};

// Actions triggered by buttons.
enum Event {
    EVT_INC_1,
    EVT_DEC_1,
    EVT_START_PAUSE,
    EVT_STOP,
    EVT_JOG_START,
    EVT_JOG_STOP
};

// Current motor operating mode.
enum MotorMode {
    MODE_STOPPED,
    MODE_RUN,
    MODE_JOG
};

// Shared system state, defined in sys.cpp.
extern volatile SystemState currentState;
extern volatile MotorMode motorMode;
extern int targetRotations;

// Process events and update the system accordingly.
void handleEvent(Event event);