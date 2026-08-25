#include "Motor.h"
#include <Arduino.h>
#include <EveryTimerB.h>
#include "constants.h"

#define pulseTimer TimerB2

// variables shared iwth timer
volatile bool pulseLevel = HIGH;
volatile bool motorRunning = false;
volatile bool motorJogging = false;

volatile uint32_t completedMotorPulses = 0;
volatile uint32_t targetMotorPulses = 0;

// Variables used for acceleration and deceleration.
bool motorDecelerating = false;

uint32_t currentPulseRate = STARTING_PULSE_RATE;
uint32_t targetPulseRate = 0;

unsigned long rampStartTimeMs = 0;
unsigned long rampDurationMs = 0;
unsigned long lastSpeedUpdateMs = 0;

void generateMotorPulse();
void setMotorPulseRate(uint32_t pulsesPerSecond);
void startMotor(
    uint32_t pulseRate,
    unsigned long accelerationTimeMs,
    uint32_t targetPulses,
    bool jogging);

void setupMotor()
{
    pinMode(MOTOR_PUL, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);
    pinMode(MOTOR_ENA, OUTPUT);

    // disable motor on setup
    // when motor disabled, pulse should remain high (battery handoff document)
    pulseLevel = HIGH;
    digitalWrite(MOTOR_ENA, LOW);
    digitalWrite(MOTOR_DIR, LOW);
    digitalWrite(MOTOR_PUL, pulseLevel);

    // initialize timer and interrupt
    pulseTimer.initialize();
    pulseTimer.attachInterrupt(generateMotorPulse);

    // disable timer on setup
    pulseTimer.stop();
}

void generateMotorPulse()
{
    // toggle pulse from HIGH to LOW rapidly
    pulseLevel = !pulseLevel;
    digitalWrite(MOTOR_PUL, pulseLevel);

    // update when a full pulse happens [(HIGH->LOW->HIGH) = full pulse]
    if (pulseLevel == HIGH)
    {
        completedMotorPulses++; // # of completed pulses

        if (!motorJogging && completedMotorPulses >= targetMotorPulses)
        {
            pulseTimer.stop();
            motorRunning = false;
            digitalWrite(MOTOR_ENA, LOW);
        }
    }
}

void startMotorRun(uint32_t targetRotations)
{
    if (targetRotations == 0)
    {
        return;
    }
    startMotor(
        RUN_PULSE_RATE,
        RUN_ACCELERATION_TIME_MS,
        targetRotations * PULSES_PER_OUTPUT_ROTATION,
        false);
}

void startMotorJog()
{
    startMotor(
        JOG_PULSE_RATE,
        JOG_ACCELERATION_TIME_MS,
        0,
        true);
}

void startMotor(
    uint32_t pulseRate,
    unsigned long accelerationTimeMs,
    uint32_t targetPulses,
    bool jogging)
{
    stopMotor();

    completedMotorPulses = 0;
    targetMotorPulses = targetPulses;

    motorJogging = jogging;
    motorRunning = true;
    motorDecelerating = false;

    currentPulseRate = STARTING_PULSE_RATE;
    targetPulseRate = pulseRate;

    rampDurationMs = accelerationTimeMs;
    rampStartTimeMs = millis();
    lastSpeedUpdateMs = rampStartTimeMs;

    digitalWrite(MOTOR_ENA, HIGH);

    setMotorPulseRate(currentPulseRate);
}

void updateMotor()
{
    if (!motorRunning)
    {
        return;
    }

    unsigned long now = millis();

    if (now - lastSpeedUpdateMs < SPEED_UPDATE_INTERVAL_MS)
    {
        return;
    }

    lastSpeedUpdateMs = now;

    noInterrupts();

    uint32_t completedPulses = completedMotorPulses;
    uint32_t targetPulses = targetMotorPulses;

    interrupts();

    // Begin slowing down when approaching the target position.
    if (!motorJogging && !motorDecelerating)
    {
        uint32_t remainingPulses =
            completedPulses < targetPulses
                ? targetPulses - completedPulses
                : 0;

        if (remainingPulses <= RUN_DECELERATION_PULSES)
        {
            motorDecelerating = true;
            rampStartTimeMs = now;
            rampDurationMs = RUN_DECELERATION_TIME_MS;
            targetPulseRate = currentPulseRate;
        }
    }

    unsigned long elapsedRampTimeMs = now - rampStartTimeMs;

    if (elapsedRampTimeMs > rampDurationMs)
    {
        elapsedRampTimeMs = rampDurationMs;
    }

    uint32_t pulseRateChange =
        (targetPulseRate - STARTING_PULSE_RATE) * elapsedRampTimeMs / rampDurationMs;

    currentPulseRate = motorDecelerating
                           ? targetPulseRate - pulseRateChange
                           : STARTING_PULSE_RATE + pulseRateChange;

    setMotorPulseRate(currentPulseRate);
}

void stopMotor()
{
    pulseTimer.stop();

    motorRunning = false;
    motorJogging = false;
    motorDecelerating = false;

    pulseLevel = HIGH;
    digitalWrite(MOTOR_PUL, pulseLevel);
    digitalWrite(MOTOR_ENA, LOW);
}

void setMotorPulseRate(uint32_t pulseRate)
{
    if (pulseRate == 0)
    {
        return;
    }
    // Two timer interrupts create one complete motor pulse.
    uint32_t pulseTimerPeriodUs =
        (1000000UL + pulseRate) / (2UL * pulseRate);

    pulseTimer.setPeriod(pulseTimerPeriodUs);
}
bool isMotorRunning()
{
    return motorRunning;
}

uint32_t getRemainingRotations()
{
    noInterrupts();

    uint32_t completedPulses = completedMotorPulses;
    uint32_t targetPulses = targetMotorPulses;

    interrupts();

    if (completedPulses >= targetPulses)
    {
        return 0;
    }

    uint32_t remainingPulses = targetPulses - completedPulses;

    return (remainingPulses + PULSES_PER_OUTPUT_ROTATION - 1) / PULSES_PER_OUTPUT_ROTATION;
}
