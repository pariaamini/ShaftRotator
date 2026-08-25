#pragma once

#include <Arduino.h>

void setupMotor();
void updateMotor();

void startMotorRun(uint32_t rotations);
void startMotorJog();
void stopMotor();

bool isMotorRunning();
uint32_t getRemainingRotations();