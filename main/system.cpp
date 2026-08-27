#include "system.h"
#include "constants.h"
#include "display.h"
#include "battery.h"
#include "display.h"

volatile SystemState currentState = STATE_IDLE;

extern volatile bool motorOn;
extern int motorRotations;
extern volatile int displayValue;
extern volatile bool displayFlag;
extern bool resetAfterStop;

extern int getRotationsRemaining();

extern void startMotion(uint32_t outputRevs);
extern void stopMotion();
extern void startJog();
extern void stopJog();

void handleEvent(Event e)
{
  switch (e)
  {

  case EVT_INC_1:
    motorRotations = min(motorRotations + 1, 99);
    displayValue = motorRotations;
    displayFlag = false;
    break;

  case EVT_DEC_1:
    motorRotations = max(motorRotations - 1, 0);
    displayValue = motorRotations;
    displayFlag = false;
    break;

  case EVT_START_PAUSE:
    if (motorRotations == 0 || currentState == STATE_JOG)
    { // if 0 rotations or in jogging mode
      break;
    }
    else if (currentState == STATE_IDLE || currentState == STATE_PAUSE)
    { // if idle or paused, start the motor
      motorOn = true;
      currentState = STATE_RUN;
      startMotion(motorRotations);
    }
    else if (currentState == STATE_RUN)
    { // if running, pause the motor
      motorRotations = getRotationsRemaining();
      displayValue = motorRotations;
      resetAfterStop = false;
      stopMotion();
      currentState = STATE_PAUSE;
    }
    break;

  case EVT_STOP_RESET:
    displayValue = 0;
    displayFlag = false;

    resetAfterStop = true;
    stopMotion();
    break;

  case EVT_JOG_START:
    if (currentState != STATE_RUN)
    {
      currentState = STATE_JOG;
      motorOn = true;
      startJog();
    }
    break;

  case EVT_JOG_STOP:
    if (currentState == STATE_JOG)
    {
      stopJog(); // this should start deceleration
    }
    break;

  case EVT_SHOW_BATTERY:
  {
    showTemporaryValue(getBatteryPercent(), 2000);
    break;
  }
  }
}

void setupStatusLEDs()
{
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  pinMode(DEBUG_LED_PIN, OUTPUT);
}
