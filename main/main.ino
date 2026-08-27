#include "buttons.h"
#include "system.h"
#include "display.h"
#include "motor.h"
#include "battery.h"

void setup() {
  setupStatusLEDs();

  setupButtons();
  setupDisplay(); // CODE FOR 7SEG DISPLAY
  setUpMotor();
  setupBattery();

  delay(50);
  showBatteryPercentage(); // show battery % for two seconds when turned on

  digitalWrite(STATUS_LED_PIN, HIGH);
}

void loop() {
  updateDisplay(); // constant display update
  digitalWrite(DEBUG_LED_PIN, isMotorOn() ? HIGH : LOW); // when motor is running, onboard orange light on nano turns on
  updateButtons(); // constantly listening for button action
  refreshDisplayValue(); 
  finishStop(); 
}
