#include <EveryTimerB.h>

#include "buttons.h"
#include "system.h"
#include "display.h"
#include "motor.h"
#include "battery.h"

// Main global variables
const int STATUS_LED_PIN = 7; // Pin used for Bat/Count LEDS
bool STATUS_LED_VAL = LOW;    // LOW for Battery HIGH for Count

// SETUP
void setup()
{
  // Serial
  pinMode(STATUS_LED_PIN, OUTPUT);              // Must be done at start to not leave as input()
  digitalWrite(STATUS_LED_PIN, STATUS_LED_VAL); // Default display

  setupButtons();
  pinMode(13, OUTPUT); // Internal LED

  setupDisplay();

  // Setup motor pins
  setUpMotor();
  setupBattery();

  delay(50);                
  uint8_t batLvl = getBatteryPercent();
  // Display battery level
  setDisplayValue(batLvl);
  uint32_t temp = millis();
  while (temp + 2000 > millis())
  { // Shows battery % for 2s
    updateDisplay();
  }
  setDisplayValue(0);
  setDisplayFlag(false);
  setDisplayFlag(true);
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
