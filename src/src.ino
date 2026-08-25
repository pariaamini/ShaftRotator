#include "motor.h"
#include "buttons.h"

void setup()
{
  setupButtons();
  setupMotor();
}

// Continuously runs and checks button outputs
void loop()
{
  updateButtons();
  updateMotor();
}