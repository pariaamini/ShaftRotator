#include <Arduino.h>
// #include <Wire.h>
// #include <U8g2lib.h>

#include "display.h"
#include "system.h"

// Display Variables
uint32_t lastRefresh = 0;
const uint32_t refreshInterval = 3;  // In ms
bool showLeft = true;
volatile int displayValue = 0;
volatile bool displayFlag = false;  // Flag to display just centre segments 'g'
uint32_t now = 0;