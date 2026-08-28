#include <Arduino.h>

// #include <Wire.h> // for oled!!
// #include <U8g2lib.h> // for oled!!

#include "display.h"
#include "system.h"
#include "battery.h"
#include "motor.h"

bool temporaryDisplayActive = false;
unsigned long temporaryDisplayStartMs = 0;
unsigned long temporaryDisplayDurationMs = 0;
int temporaryDisplayValue = 0;
int valueBeforeTemporary = 0;

// Original 7seg Display Code

// Display Variables
uint32_t lastRefresh = 0;
const uint32_t refreshInterval = 3; // In ms
bool showLeft = true;
volatile int displayValue = 0;

// LED segment layout for 0-9
// Segment pins: A B C D E F G
const byte segPins[2][7] = {
    // Different segments for each the different digits
    // Arduino Pin, segment letter, led pin, wire colour
    {    // For digit 1
     9,  // A //Pin 2 //Black
     12, // B //Pin 6 //Blue
     15, // C //Pin 9 //Orange
     11, // D //Pin 5 //Purple
     14, // E //Pin 8 //Yellow
     10, // F //Pin 3 //White
     8}, // G //Pin 1 //Brown
    {    // For digit 2
     9,  // A //Pin 2 //Black
     14, // B //Pin 8 //Yellow
     12, // C //Pin 6 //Blue
     11, // D //Pin 5 //Purple
     10, // E //Pin 3 //White
     15, // F //Pin 9 //Orange
     8}  // G //Pin 1 //Brown}
};

// Digit anodes
const int DIG1 = 16; // Left digit //Pin7 green
const int DIG2 = 17; // Right digit //Pin4 grey
// Common-anode segment map (0 = ON, 1 = OFF)
const byte digits[10][7] = {
    {0, 0, 0, 0, 0, 0, 1}, // 0
    {1, 0, 0, 1, 1, 1, 1}, // 1
    {0, 0, 1, 0, 0, 1, 0}, // 2
    {0, 0, 0, 0, 1, 1, 0}, // 3
    {1, 0, 0, 1, 1, 0, 0}, // 4
    {0, 1, 0, 0, 1, 0, 0}, // 5
    {0, 1, 0, 0, 0, 0, 0}, // 6
    {0, 0, 0, 1, 1, 1, 1}, // 7
    {0, 0, 0, 0, 0, 0, 0}, // 8
    {0, 0, 0, 0, 1, 0, 0}  // 9
};
// FUNCTION DECLARATIONS
// Displays 1 digit per call from the global varible of the displayValue. That digit is left on between calls.
void updateDisplay()
{
  uint32_t now = millis();
  if (now - lastRefresh < refreshInterval)
    return;
  lastRefresh = now; // Set last refresh to current time

  // Turn digits off
  digitalWrite(DIG1, HIGH);
  digitalWrite(DIG2, HIGH);

  int activeDigit = showLeft ? 0 : 1;                                // Swaps since we want 0 for left and 1 for right
  int digitValue = showLeft ? displayValue / 10 : displayValue % 10; // Takes the required digit out of dispalyValue to write

  // For writing number to display
  for (int i = 0; i < 7; i++)
  {
    digitalWrite(segPins[activeDigit][i], digits[digitValue][i] ? HIGH : LOW);
  }
  digitalWrite(showLeft ? DIG1 : DIG2, LOW);
  showLeft = !showLeft;
}

void setupDisplay() // init display -> ran in setup() in main
{
  for (int i = 0; i < 7; i++)
  {
    pinMode(segPins[0][i], OUTPUT);
  }

  pinMode(DIG1, OUTPUT);
  pinMode(DIG2, OUTPUT); 

  digitalWrite(DIG1, HIGH);
  digitalWrite(DIG2, HIGH);
}

void setDisplayValue(int value) 
{
  displayValue = value;
}
void refreshDisplayValue() 
{
  if (temporaryDisplayActive)
  {
    if (millis() - temporaryDisplayStartMs < temporaryDisplayDurationMs)
    {
      setDisplayValue(temporaryDisplayValue);
      return;
    }

    // Temporary display finished
    temporaryDisplayActive = false;
    setDisplayValue(valueBeforeTemporary);
  }

  if (currentState == STATE_RUN)
  {
    setDisplayValue(getRotationsRemaining());
  }
}

void showTemporaryValue(int value, unsigned long durationMs) // primarily used to display battery %, can be used to show other things
{
  valueBeforeTemporary = displayValue;

  temporaryDisplayValue = value;
  temporaryDisplayStartMs = millis();
  temporaryDisplayDurationMs = durationMs;
  temporaryDisplayActive = true;

  setDisplayValue(value);
}