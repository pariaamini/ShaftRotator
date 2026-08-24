#pragma once

#include <Arduino.h>

// Current operating state of the shaft rotator
enum SystemState {
  STATE_IDLE,
  STATE_RUNNING,
  STATE_JOG
};

// Actions triggered by button presses.
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

// Shared system variables accessible from other files.
extern volatile SystemState currentState;
extern volatile MotorMode motorMode;
extern int targetRotations;

// Set up button callbacks and press timings.
void setupButtons();

// Check each button for presses, releases, and long presses.
void updateButtons();

// Process a button event and update the system state.
void handleEvent(Event event);