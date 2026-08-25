#include "buttons.h"
#include "display.h"
#include "system.h"

void setup()
{
  setupButtons();
  setupDisplay();
}

void loop()
{
  updateButtons();
  updateDisplay();
}