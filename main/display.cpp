#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "display.h"
#include "system.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE);

void drawDisplay()
{
  display.clearBuffer();

  // Heading
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 11, "SHAFT ROTATOR TEST");
  display.drawHLine(0, 15, 128);

  // Rotation number
  display.setFont(u8g2_font_logisoso24_tn);

  char rotationText[8];

  snprintf(
      rotationText,
      sizeof(rotationText),
      "%d",
      targetRotations);

  display.drawStr(0, 46, rotationText);

  // Labels and system state
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(54, 30, "ROTATIONS");
  display.drawStr(54, 46, getStateText());

  display.sendBuffer();
}

void setupDisplay()
{
  display.begin();

  displayNeedsUpdate = true;
  updateDisplay();
}

void updateDisplay()
{
  if (!displayNeedsUpdate)
  {
    return;
  }

  drawDisplay();
  displayNeedsUpdate = false;
}