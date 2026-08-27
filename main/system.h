#pragma once

// StateMachine
enum SystemState {  // For Readability
  STATE_IDLE,
  STATE_RUN,
  STATE_JOG,
  STATE_PAUSE

};
enum Event {
  EVT_NONE,
  EVT_INC_1,
  EVT_DEC_1,
  EVT_START_PAUSE,
  EVT_STOP_RESET,
  EVT_JOG_START,
  EVT_JOG_STOP
};

extern volatile SystemState currentState;
extern int targetRotations;
extern bool displayNeedsUpdate;

void handleEvent(Event event);
const char *getStateText();