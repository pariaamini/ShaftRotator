#pragma once

enum SystemState
{
  STATE_IDLE,
  STATE_RUNNING,
  STATE_PAUSED,
  STATE_JOGGING
};

enum Event
{
  EVT_INC_1,
  EVT_DEC_1,
  EVT_START_PAUSE,
  EVT_STOP_RESET,
  EVT_JOG_START,
  EVT_JOG_STOP
};

extern SystemState currentState;
extern int targetRotations;
extern bool displayNeedsUpdate;

void handleEvent(Event event);
const char *getStateText();