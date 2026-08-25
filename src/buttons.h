#pragma once

#include <Arduino.h>

// Set up button callbacks and press timings.
void setupButtons();

// Check each button for presses, releases, and long presses.
void updateButtons();