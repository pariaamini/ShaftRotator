#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "display.h"
#include "system.h"
#include "motor.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(
  U8G2_R0,
  SCL,
  SDA,
  U8X8_PIN_NONE);


int displayValue = 0;

// Temporary display state
bool temporaryDisplayActive = false;
unsigned long temporaryDisplayStartMs = 0;
unsigned long temporaryDisplayDurationMs = 0;
int temporaryDisplayValue = 0;
int valueBeforeTemporary = 0;
bool showingBattery = false;

void setupDisplay() {
  Wire.begin();
  oled.begin();

  updateDisplay();
}

void updateDisplay() {
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB08_tr);

  if (showingBattery) {
    oled.drawStr(38, 12, "BATTERY");
  } else {
    oled.drawStr(27, 12, "ROTATIONS");
  }

  char valueText[8];
  snprintf(valueText, sizeof(valueText), "%d", displayValue);

  oled.setFont(u8g2_font_logisoso32_tn);

  int textWidth = oled.getStrWidth(valueText);
  int x = (128 - textWidth) / 2;

  oled.drawStr(x, 54, valueText);

  oled.sendBuffer();
}

void setDisplayValue(int value) {  // display value setter
  if (displayValue == value)
    return;

  displayValue = value;
  updateDisplay();
}

void refreshDisplayValue() {  // runs periodically in main.
  if (temporaryDisplayActive) {
    if (millis() - temporaryDisplayStartMs >= temporaryDisplayDurationMs) {
      temporaryDisplayActive = false;
      showingBattery = false;

      displayValue = valueBeforeTemporary;
      updateDisplay();
    }

    return;
  }

  if (currentState == STATE_RUN) {
    setDisplayValue(getRotationsRemaining());
  }
}

void showTemporaryValue(int value, unsigned long durationMs) {
  valueBeforeTemporary = displayValue;

  temporaryDisplayValue = value;
  temporaryDisplayStartMs = millis();
  temporaryDisplayDurationMs = durationMs;
  temporaryDisplayActive = true;

  showingBattery = true;

  displayValue = temporaryDisplayValue;
  updateDisplay();
}