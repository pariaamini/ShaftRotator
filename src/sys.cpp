#include "sys.h"
#include "motor.h"

volatile SystemState currentState = STATE_IDLE;
volatile MotorMode motorMode = MODE_STOPPED;
int targetRotations = 0;

void handleEvent(Event event)
{
  switch (event)
  {
  case EVT_INC_1:
    targetRotations++;
    break;

  case EVT_DEC_1:
    if (targetRotations > 0)
    {
      targetRotations--;
    }
    break;

  case EVT_JOG_START:
    // Do not interrupt an active programmed movement.
    if (currentState == STATE_RUNNING)
    {
      break;
    }

    currentState = STATE_JOG;
    motorMode = MODE_JOG;

    startMotorJog();
    break;

  case EVT_JOG_STOP:
    stopMotor();

    currentState = STATE_IDLE;
    motorMode = MODE_STOPPED;
    break;

  case EVT_START_PAUSE:
    if (currentState == STATE_IDLE && targetRotations > 0)
    {
      currentState = STATE_RUNNING;
      motorMode = MODE_RUN;

      startMotorRun(targetRotations);
    }
    else if (currentState == STATE_RUNNING)
    {
      // Save the remaining rotations before stopping.
      targetRotations = getRemainingRotations();

      stopMotor();

      currentState = STATE_IDLE;
      motorMode = MODE_STOPPED;
    }

    break;

  case EVT_STOP:
    currentState = STATE_IDLE;
    targetRotations = 0;
    motorMode = MODE_STOPPED;
    break;
  }
}