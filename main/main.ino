#include "buttons.h"
#include "system.h"
#include "display.h"
#include "motor.h"
#include "battery.h"

void setup() {
  Serial.begin(9600);

  setupStatusLEDs();

  setupDisplay();
  setupButtons();
  setupBattery();
  setUpMotor();


  showTemporaryValue(99, 2000);

  digitalWrite(STATUS_LED_PIN, HIGH);
}
void loop() {
  updateButtons();
  refreshDisplayValue();
  finishStop();

  digitalWrite(DEBUG_LED_PIN, isMotorOn() ? HIGH : LOW);
}