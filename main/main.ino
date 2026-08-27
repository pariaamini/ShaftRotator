#include <EveryTimerB.h>

#include "buttons.h"
#include "system.h"
#include "display.h"
#include "motor.h"
#include "battery.h"

const int STATUS_LED_PIN = 7;
bool STATUS_LED_VAL = LOW;

void setup()
{
  // Serial
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, STATUS_LED_VAL);
  pinMode(13, OUTPUT); // Internal LED

  setupButtons();
  setupDisplay();
  setUpMotor();
  setupBattery();

  delay(50);
  showBatteryPercentage();

  STATUS_LED_VAL = HIGH;
  digitalWrite(STATUS_LED_PIN, STATUS_LED_VAL);
}

void loop()
{
  updateDisplay();                            // Always runs to dispaly something
  digitalWrite(13, isMotorOn() ? HIGH : LOW); // REMOVE AFTER just for debugging
  updateButtons();
  refreshDisplayValue();
  finishStop();
}
