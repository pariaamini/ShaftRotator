#include "Motor.h"
#include <Arduino.h>
#include <EveryTimerB.h>
#include "constants.h"

#define pulseTimer TimerB2

volatile bool pulseLevel = LOW;

volatile unsigned long completedMotorPulses = 0;

void setupMotor()
{
    pinMode(MOTOR_PUL, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);
    pinMode(MOTOR_ENA, OUTPUT);

    // disable motor on setup
    // when motor disabled, pulse should remain high (battery handoff document)
    digitalWrite(MOTOR_ENA, LOW);
    pulseLevel = HIGH;
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
        completedMotorPulses++; // # of pulses completed
    }
}

void startMotor(unsigned long pulseTimerPeriodUs)
{
    // reset pulse counter
    noInterrupts();
    completedMotorPulses = 0;
    interrupts();

    // set motor to idle
    pulseLevel = HIGH;
    digitalWrite(MOTOR_PUL, pulseLevel);

    // enable motor controller
    digitalWrite(MOTOR_ENA, HIGH);

    // generate pulses at pulseTimerPeriodUs
    pulseTimer.setPeriod(pulseTimerPeriodUs);
}

void stopMotor()
{
    // stop generating pulses
    pulseTimer.stop();

    // disable motor controller
    digitalWrite(MOTOR_ENA, LOW);

    // set pulse signal to idle
    pulseLevel = HIGH;
    digitalWrite(MOTOR_PUL, pulseLevel);
}

void changeMotorSpeed(unsigned long pulseTimerPeriodUs)
{
    pulseTimer.setPeriod(pulseTimerPeriodUs);
}