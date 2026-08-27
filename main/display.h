#pragma once

void setupDisplay();
void updateDisplay();

void setDisplayValue(int value);
void setDisplayFlag(bool flag);

void refreshDisplayValue();

void showTemporaryValue(int value, unsigned long durationMs);