#pragma once
#include <Arduino.h>

void setUpMotor();

void startMotion(uint32_t outputRevs);
void stopMotion();

void startJog();
void stopJog();

void finishStop();

int getRotationsRemaining();
void updateMotorRamp();

bool isMotorOn();
void resumeMotion();
void printMotionPosition(const char *label);

int getRotationTenthsRemaining();