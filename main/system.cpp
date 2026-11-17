#include "system.h"
#include "constants.h"
#include "display.h"
#include "battery.h"
#include "display.h"

volatile SystemState currentState = STATE_IDLE;

extern volatile bool motorOn;
extern int motorRotations;
extern bool resetAfterStop;

extern int getRotationsRemaining();

extern void startMotion(uint32_t outputRevs);
extern void stopMotion();
extern void startJog();
extern void stopJog();

void handleEvent(Event e) {
  switch (e) {

    case EVT_INC_1:
      motorRotations = min(motorRotations + 1, 99);
      setDisplayValue(motorRotations);
      break;

    case EVT_DEC_1:
      motorRotations = max(motorRotations - 1, 0);
      setDisplayValue(motorRotations);
      break;

    case EVT_START_PAUSE:
      if (motorRotations == 0 || currentState == STATE_JOG) {  // if 0 rotations or in jogging mode
        break;
      } else if (currentState == STATE_IDLE || currentState == STATE_PAUSE) {  // if idle or paused, start the motor
        motorOn = true;
        currentState = STATE_RUN;
        startMotion(motorRotations);
      } else if (currentState == STATE_RUN) {  // if running, pause the motor
        motorRotations = getRotationsRemaining();
        setDisplayValue(motorRotations);
        resetAfterStop = false;
        stopMotion();
        currentState = STATE_PAUSE;
      }
      break;

    case EVT_STOP_RESET:
      setDisplayValue(0);
      resetAfterStop = true;
      stopMotion();
      currentState = STATE_IDLE;
      break;

    case EVT_JOG_START:
      if (currentState != STATE_RUN) {
        currentState = STATE_JOG;
        motorOn = true;
        startJog();
      }
      break;

      // case EVT_JOG_STOP:
      // if (currentState == STATE_JOG) {
      //     stopJog();
      //     currentState = STATE_IDLE;
      // }
      // break;

    case EVT_JOG_STOP:
      if (currentState == STATE_JOG) {
        stopJog();  // Begin deceleration
        // Remain in STATE_JOG until deceleration finishes
      }
      break;

    case EVT_SHOW_BATTERY:
      {
        showTemporaryValue(getBatteryPercent(), 2000);
        break;
      }
  }
}

void setupStatusLEDs() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void updateStatusLED() {
  if (currentState == STATE_RUN) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
