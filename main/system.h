#pragma once

#include "constants.h"

enum SystemState
{
  STATE_IDLE,
  STATE_RUN,
  STATE_JOG,
  STATE_PAUSE

};
enum Event
{
  EVT_NONE,
  EVT_INC_1,
  EVT_DEC_1,
  EVT_START_PAUSE,
  EVT_STOP_RESET,
  EVT_JOG_START,
  EVT_JOG_STOP,
  EVT_SHOW_BATTERY,
};

extern volatile SystemState currentState;
extern int targetRotations;
extern bool displayNeedsUpdate;

void handleEvent(Event event);
const char *getStateText();

void setupStatusLEDs();