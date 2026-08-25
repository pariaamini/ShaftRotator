#include "system.h"
#include "constants.h"

SystemState currentState = STATE_IDLE;
bool displayNeedsUpdate = true;

int targetRotations = 0; // initial value

void handleEvent(Event event)
{
  switch (event)
  {
  case EVT_INC_1:
    if (currentState != STATE_JOGGING &&
        targetRotations < MAX_ROTATIONS)
    {
      targetRotations++;
    }
    break;

  case EVT_DEC_1:
    if (currentState != STATE_JOGGING &&
        targetRotations > MIN_ROTATIONS)
    {
      targetRotations--;
    }
    break;

  case EVT_START_PAUSE:
    if (currentState == STATE_JOGGING)
    {
      break;
    }

    if (currentState == STATE_IDLE ||
        currentState == STATE_PAUSED)
    {
      if (targetRotations > 0)
      {
        currentState = STATE_RUNNING;
      }
    }
    else if (currentState == STATE_RUNNING)
    {
      currentState = STATE_PAUSED;
    }
    break;

  case EVT_STOP_RESET:
    currentState = STATE_IDLE;
    targetRotations = 0;
    break;

  case EVT_JOG_START:
    if (currentState != STATE_RUNNING)
    {
      currentState = STATE_JOGGING;
    }
    break;

  case EVT_JOG_STOP:
    if (currentState == STATE_JOGGING)
    {
      currentState = STATE_IDLE;
    }
    break;
  }
  displayNeedsUpdate = true;
}

const char *getStateText()
{
  switch (currentState)
  {
  case STATE_RUNNING:
    return "RUNNING";

  case STATE_PAUSED:
    return "PAUSED";

  case STATE_JOGGING:
    return "JOGGING";

  case STATE_IDLE:
  default:
    return "IDLE";
  }
}