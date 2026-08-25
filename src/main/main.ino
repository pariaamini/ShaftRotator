#include <Arduino.h>
#include <OneButton.h>
#include <EveryTimerB.h>

#include "constants.h"

#define pulseTimer TimerB2

OneButton buttonJog(PIN_JOG, true);

volatile bool pulseLevel = HIGH;
bool motorJogging = false;

uint32_t currentPulseRate = STARTING_PULSE_RATE;
unsigned long jogStartTimeMs = 0;
unsigned long lastSpeedUpdateMs = 0;

// Function declarations.
void startJog();
void stopJog();
void updateJogRamp();
void generateMotorPulse();
void setMotorPulseRate(uint32_t pulseRate);

void setup() {
  // motor control pins
  pinMode(MOTOR_PUL, OUTPUT);
  pinMode(MOTOR_DIR, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);

  // motor starts disabled, with the pulse signal HIGH
  digitalWrite(MOTOR_PUL, HIGH);
  digitalWrite(MOTOR_DIR, LOW);
  digitalWrite(MOTOR_ENA, LOW);

  // config hardware timer, but do not start pulsing yet
  pulseTimer.initialize();
  pulseTimer.attachInterrupt(generateMotorPulse);
  pulseTimer.stop();

  // config jog button
  buttonJog.setPressMs(JOG_LONG_PRESS_MS);
  buttonJog.attachLongPressStart(startJog);
  buttonJog.attachLongPressStop(stopJog);
}

void loop() {
  // check whether the jog button has been pressed or released
  buttonJog.tick();

  // gradually increase motor speed while jogging
  updateJogRamp();
}

void startJog() {
  if (motorJogging) {
    return;
  }

  // ensure each jog begins from a clean stopped state
  pulseTimer.stop();
  pulseLevel = HIGH;
  setMotorPulse(pulseLevel);
  motorJogging = true;
  currentPulseRate = STARTING_PULSE_RATE;

  jogStartTimeMs = millis();
  lastSpeedUpdateMs = jogStartTimeMs;

  // Enable the motor driver.
  digitalWrite(MOTOR_ENA, HIGH);

  // Begin generating motor pulses at the starting speed.
  setMotorPulseRate(currentPulseRate);
}

void stopJog() {
  // Stop generating pulses immediately.
  pulseTimer.stop();

  motorJogging = false;

  // Return the pulse signal to its idle state.
  pulseLevel = HIGH;
  setMotorPulse(pulseLevel);

  // Disable the motor driver immediately.
  digitalWrite(MOTOR_ENA, LOW);
}

void updateJogRamp() {
  if (!motorJogging) {
    return;
  }

  unsigned long now = millis();

  // update speed only once every SPEED_UPDATE_INTERVAL_MS
  if (now - lastSpeedUpdateMs < SPEED_UPDATE_INTERVAL_MS) {
    return;
  }

  lastSpeedUpdateMs = now;

  unsigned long elapsedMs = now - jogStartTimeMs;

  // stop increasing speed once the accel period finishes
  if (elapsedMs > JOG_ACCELERATION_TIME_MS) {
    elapsedMs = JOG_ACCELERATION_TIME_MS;
  }

  // increase speed linearly from STARTING_PULSE_RATE to JOG_PULSE_RATE
  uint32_t speedIncrease =
    (JOG_PULSE_RATE - STARTING_PULSE_RATE) * elapsedMs / JOG_ACCELERATION_TIME_MS;

  currentPulseRate = STARTING_PULSE_RATE + speedIncrease;

  setMotorPulseRate(currentPulseRate);
}

void generateMotorPulse() {
  pulseLevel = !pulseLevel;
  setMotorPulse(pulseLevel);
}

void setMotorPulseRate(uint32_t pulseRate) {
  if (pulseRate == 0) {
    return;
  }

  // Two timer interrupts create one full motor pulse:
  // HIGH -> LOW -> HIGH.
  uint32_t pulseTimerPeriodUs =
    (1000000UL + pulseRate) / (2UL * pulseRate);

  // setPeriod() also starts the timer.
  pulseTimer.setPeriod(pulseTimerPeriodUs);
}

void setMotorPulse(bool pulseLevel) {
  digitalWrite(MOTOR_PUL, pulseLevel);
}