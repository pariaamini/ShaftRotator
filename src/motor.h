#pragma once

// Configure motor pins and initialize the pulse timer.
void setupMotor();

// Start the motor using the specified timer period.
void startMotor(unsigned long pulseTimerPeriodUs);

// Stop the motor and disable the motor controller.
void stopMotor();

// Change the motor pulse timing while running.
void changeMotorSpeed(unsigned long pulseTimerPeriodUs);

// Timer interrupt callback that generates motor pulses.
void generateMotorPulse();