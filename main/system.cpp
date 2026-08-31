#include "system.h"
#include "constants.h"
#include "display.h"
#include "battery.h"
#include "display.h"
#include "motor.h"

volatile SystemState currentState = STATE_IDLE;

extern volatile bool motorOn;
extern int motorRotations;
extern volatile int displayValue;
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
    break;

  case EVT_DEC_1:
    motorRotations = max(motorRotations - 1, 0);
    displayValue = motorRotations;
    break;

  case EVT_START_PAUSE:

    if (motorRotations == 0 || currentState == STATE_JOG)
    {
        break;
    }

    if (currentState == STATE_IDLE)
    {
        currentState = STATE_RUN;
        startMotion(motorRotations);
    }
    else if (currentState == STATE_PAUSE)
    {
        currentState = STATE_RUN;
        resumeMotion();
    }
    else if (currentState == STATE_RUN)
    {
        resetAfterStop = false;
        currentState = STATE_PAUSE;
        stopMotion();
    }

    break;

  case EVT_STOP_RESET:
    displayValue = 0;

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
