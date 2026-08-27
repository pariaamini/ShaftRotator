#include <Arduino.h>
#include <EveryTimerB.h>

#include "motor.h"
#include "system.h"
#include "display.h"

#define RampTimer TimerB2

// NEW MOTOR VARIABLES
volatile bool motorOn = false; // LED state will mimic motor
int motorRotations = 0;
const int MOTOR_GEAR_RATIO = 10;  // JKong gear reduction
const int SYSTEM_GEAR_RATIO = 63; // External reduction only
int pulsesPerRev = 200;           // Dip switch setting off,off,off,off     Section 5.1 in motor manual
uint32_t pulsesPerRevMotorOut = MOTOR_GEAR_RATIO * pulsesPerRev;
const uint32_t pulsesPerRevSystemOut = pulsesPerRevMotorOut * SYSTEM_GEAR_RATIO;
volatile bool pulseLevel = LOW;
volatile uint32_t sentPulses = 0;
volatile uint32_t targetPulses = 0;

volatile uint32_t decelStartPulse = 0;

volatile bool rampDecel = false;

// ================= MOTOR SPEED SETTINGS =================

// Motor pulse frequency at startup
constexpr float START_FREQ_HZ = 300.0;

// Normal run speed
constexpr float RUN_FREQ_HZ = 9000.0;

// Jog speed
constexpr float JOG_FREQ_HZ = 7000.0;

// Ramp update interval
constexpr unsigned long RAMP_UPDATE_US = 10000; // 10 ms

// How long acceleration/deceleration should take
constexpr float ACCEL_TIME_S = 2.0;
constexpr float DECEL_TIME_S = 1.0;

// ================= RAMP STATE =================

volatile float currentFreqHz = START_FREQ_HZ;
volatile bool resetAfterStop = false;

constexpr float RAMP_UPDATE_S =
    RAMP_UPDATE_US / 1000000.0;

constexpr float ACCEL_STEP_HZ =
    (RUN_FREQ_HZ - START_FREQ_HZ) * RAMP_UPDATE_S / ACCEL_TIME_S;

constexpr float DECEL_STEP_HZ =
    (RUN_FREQ_HZ - START_FREQ_HZ) * RAMP_UPDATE_S / DECEL_TIME_S;

// Motor pins
const int MOTOR_PUL = 18; // Pin for motor PUL-     LOW for pulse return to HIGH for idle state
const int MOTOR_DIR = 19; // Pin for motor DIR-
const int MOTOR_ENA = 20; // Pin for motor ENA-     HIGH for motor on
const int MOTOR_ALM = 21; // Pin for motor ALM-     INPUT PIN

int getRotationsRemaining()
{ // Return how many more rotations are left. This is to be displayed and will be shown as a rounded integer
    // Pulse Counter Based
    noInterrupts(); // Makes sure values dont get changed as they are read. this should not take more than a couple micros so not a problem
    uint32_t sent = sentPulses;
    uint32_t target = targetPulses;
    interrupts();
    if (sent >= target)
        return 0; // If at the end do not waste CPU time on math
    uint32_t remaining = target - sent;
    return ((remaining + pulsesPerRevSystemOut - 1) / (pulsesPerRevSystemOut));
}

// Motor running code
// MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE
void setupPulseTimer(uint16_t ccmpInitial)
{
    TCB0.CTRLB = TCB_CNTMODE_INT_gc; // Periodic
    TCB0.CCMP = ccmpInitial;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc; // Not enabled yet
}

// Pulse counting ISR. Also handles stopping after target reached
ISR(TCB0_INT_vect)
{ // This has to trigger twice to get 1 pulse

    if (!motorOn)
    { // Leave if motor not on
        TCB0.INTFLAGS = TCB_CAPT_bm;
        return;
    }

    digitalWrite(MOTOR_PUL, pulseLevel);
    pulseLevel = !pulseLevel;

    if (pulseLevel && currentState != STATE_JOG)
    { // count full pulses only and not when in jog mode
        sentPulses++;
        if (sentPulses >= decelStartPulse)
        {
            rampDecel = true;
        }

        if (sentPulses >= targetPulses)
        {
            motorOn = false;
            currentState = STATE_IDLE;
            setDisplayValue(0);
            motorRotations = 0;
            setDisplayFlag(true);
            TCB0.CTRLA &= ~TCB_ENABLE_bm;
            digitalWrite(MOTOR_ENA, LOW);
        }
    }

    TCB0.INTFLAGS = TCB_CAPT_bm;
}

void setMotorFrequency(float freqHz)
{
    if (freqHz < 1.0)
        freqHz = 1.0;

    /*
     * TCB0 interrupt toggles MOTOR_PUL every interrupt.
     *
     * Therefore:
     *
     * 2 interrupts = 1 complete pulse
     *
     * timerInterruptFrequency = motorPulseFrequency * 2
     */

    float interruptFreqHz = freqHz * 2.0;

    uint32_t ccmp =
        (uint32_t)(F_CPU / interruptFreqHz);

    if (ccmp > 65535)
        ccmp = 65535;

    if (ccmp < 1)
        ccmp = 1;

    TCB0.CCMP = (uint16_t)ccmp;
}

void updateMotorRamp()
{
    if (!motorOn)
    {
        return;
    }

    // DECELERATION
    if (rampDecel)
    {
        currentFreqHz -= DECEL_STEP_HZ;

        if (currentFreqHz <= START_FREQ_HZ)
        {
            currentFreqHz = START_FREQ_HZ;

            motorOn = false;
            TCB0.CTRLA &= ~TCB_ENABLE_bm;

            return;
        }

        setMotorFrequency(currentFreqHz);
        return;
    }

    // NORMAL ACCELERATION
    // ================= RUN =================
    if (currentState == STATE_RUN)
    {

        currentFreqHz += ACCEL_STEP_HZ;

        if (currentFreqHz > RUN_FREQ_HZ)
            currentFreqHz = RUN_FREQ_HZ;
    }

    // ================= JOG =================
    else if (currentState == STATE_JOG)
    {
        currentFreqHz += ACCEL_STEP_HZ;

        if (currentFreqHz > JOG_FREQ_HZ)
            currentFreqHz = JOG_FREQ_HZ;
    }

    setMotorFrequency(currentFreqHz);
}

// MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE
void startMotion(uint32_t outputRevs)
{
    noInterrupts();

    sentPulses = 0;

    targetPulses =
        (uint32_t)outputRevs * pulsesPerRevSystemOut;

    rampDecel = false;
    currentFreqHz = START_FREQ_HZ;

    /*
     * Temporary simple decel trigger:
     * begin decelerating near the end.
     *
     * We can make this mathematically precise later.
     */
    uint32_t decelPulseCount =
        pulsesPerRevSystemOut / 2;

    if (targetPulses > decelPulseCount)
        decelStartPulse =
            targetPulses - decelPulseCount;
    else
        decelStartPulse = 0;

    setMotorFrequency(currentFreqHz);

    motorOn = true;

    digitalWrite(MOTOR_PUL, LOW);
    pulseLevel = LOW;

    digitalWrite(MOTOR_ENA, HIGH);

    TCB0.CTRLA |= TCB_ENABLE_bm;

    interrupts();
}

void startJog()
{
    noInterrupts();

    sentPulses = 0;
    targetPulses = 0;

    rampDecel = false;
    currentFreqHz = START_FREQ_HZ;

    setMotorFrequency(currentFreqHz);

    motorOn = true;

    digitalWrite(MOTOR_PUL, LOW);
    pulseLevel = LOW;

    digitalWrite(MOTOR_ENA, HIGH);

    TCB0.CTRLA |= TCB_ENABLE_bm;

    interrupts();
}

void stopMotion()
{
    rampDecel = true;
}

void stopJog()
{
    rampDecel = true;
}
bool isMotorOn()
{
    return motorOn;
}

void setUpMotor()
{
    pinMode(MOTOR_PUL, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);
    pinMode(MOTOR_ENA, OUTPUT);
    pinMode(MOTOR_ALM, INPUT);

    setupPulseTimer(65535);

    RampTimer.initialize();
    RampTimer.attachInterrupt(updateMotorRamp);
    RampTimer.setPeriod(RAMP_UPDATE_US);
}

void finishStop()
{
    if (motorOn || !rampDecel)
        return;

    digitalWrite(MOTOR_ENA, LOW);

    if (resetAfterStop)
    {
        motorRotations = 0;
        setDisplayValue(0);
        setDisplayFlag(false);
    }

    currentState = STATE_IDLE;
    currentFreqHz = START_FREQ_HZ;
    rampDecel = false;
    resetAfterStop = false;
}