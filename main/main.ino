#include <EveryTimerB.h>

#include "buttons.h"
#include "system.h"
#include "display.h"

#define RampTimer TimerB2

// Main global variables
const int STATUS_LED_PIN = 7;  // Pin used for Bat/Count LEDS
bool STATUS_LED_VAL = LOW;     // LOW for Battery HIGH for Count
// Battery Level
const int BAT_PIN = A7;  // Same as D21 (21)
uint8_t batLvl = 0;
uint32_t bat_mV = 0;
uint32_t lastBatRead = 0;  // Time of last battery read
// Voltage Divider Vout = Vin * (R2/(R2+R1)) this should give max 4.2v which is safely under 5v max (Vin max is 25.2V)
const float R1 = 100000.0;         // ohms R1 of voltage divider
const float R2 = 20000.0;          // ohms R2 of voltage divider
const uint32_t ADC_REF_mV = 5000;  // Nano Every reference
// uint32_t lowBat_mV = 6 * 3300;  //Do not let pack go under this voltage. Update this as needed. 3300mv minimum per cell, 6 cells.
// Battery is nonlinear so a value table is used
struct BatPoint {
  uint16_t mV;
  uint8_t percent;
};
const BatPoint batTable[] = {  // EYBMS fuel gauge and other online resources used to make this table. Testing of battery to verify accuracy would be recomended.
  { 24600, 100 },
  { 24000, 90 },
  { 23400, 80 },
  { 22800, 70 },
  { 21900, 60 },
  { 21000, 50 },
  { 20400, 40 },
  { 19800, 30 },
  { 19200, 20 },
  { 18600, 10 },
  { 18000, 0 }
};
const int BAT_TABLE_SIZE = sizeof(batTable) / sizeof(batTable[0]);

// NEW MOTOR VARIABLES
volatile bool motorOn = false;  // LED state will mimic motor
int motorRotations = 0;
const int MOTOR_GEAR_RATIO = 10;   // JKong gear reduction
const int SYSTEM_GEAR_RATIO = 63;  // External reduction only
int pulsesPerRev = 200;            // Dip switch setting off,off,off,off     Section 5.1 in motor manual
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
constexpr unsigned long RAMP_UPDATE_US = 10000;  // 10 ms

// How long acceleration/deceleration should take
constexpr float RUN_ACCEL_TIME_S = 2.0;
constexpr float RUN_DECEL_TIME_S = 1.0;
constexpr float JOG_ACCEL_TIME_S = 1.0;

// ================= RAMP STATE =================

volatile float currentFreqHz = START_FREQ_HZ;
volatile bool resetAfterStop = false;

constexpr float RAMP_UPDATE_S =
  RAMP_UPDATE_US / 1000000.0;

constexpr float RUN_ACCEL_STEP_HZ =
  (RUN_FREQ_HZ - START_FREQ_HZ) * RAMP_UPDATE_S / RUN_ACCEL_TIME_S;

constexpr float MOTOR_DECEL_STEP_HZ =
  (RUN_FREQ_HZ - START_FREQ_HZ) * RAMP_UPDATE_S / RUN_DECEL_TIME_S;

constexpr float JOG_ACCEL_STEP_HZ =
  (JOG_FREQ_HZ - START_FREQ_HZ) * RAMP_UPDATE_S / JOG_ACCEL_TIME_S;

// Motor pins
const int MOTOR_PUL = 18;  // Pin for motor PUL-     LOW for pulse return to HIGH for idle state
const int MOTOR_DIR = 19;  // Pin for motor DIR-
const int MOTOR_ENA = 20;  // Pin for motor ENA-     HIGH for motor on
const int MOTOR_ALM = 21;  // Pin for motor ALM-     INPUT PIN

int getRotationsRemaining() {  // Return how many more rotations are left. This is to be displayed and will be shown as a rounded integer
  // Pulse Counter Based
  noInterrupts();  // Makes sure values dont get changed as they are read. this should not take more than a couple micros so not a problem
  uint32_t sent = sentPulses;
  uint32_t target = targetPulses;
  interrupts();
  if (sent >= target)
    return 0;  // If at the end do not waste CPU time on math
  uint32_t remaining = target - sent;
  return ((remaining + pulsesPerRevSystemOut - 1) / (pulsesPerRevSystemOut));
}

// Motor running code
// MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE      MOTOR TIMER CODE
void setupPulseTimer(uint16_t ccmpInitial) {
  TCB0.CTRLB = TCB_CNTMODE_INT_gc;  // Periodic
  TCB0.CCMP = ccmpInitial;
  TCB0.INTCTRL = TCB_CAPT_bm;
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc;  // Not enabled yet
}

// Pulse counting ISR. Also handles stopping after target reached
ISR(TCB0_INT_vect) {  // This has to trigger twice to get 1 pulse

  if (!motorOn) {  // Leave if motor not on
    TCB0.INTFLAGS = TCB_CAPT_bm;
    return;
  }

  digitalWrite(MOTOR_PUL, pulseLevel);
  pulseLevel = !pulseLevel;

  if (pulseLevel && currentState != STATE_JOG) {  // count full pulses only and not when in jog mode
    sentPulses++;
    if (sentPulses >= decelStartPulse) {
      rampDecel = true;
    }

    if (sentPulses >= targetPulses) {
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

void setMotorFrequency(float freqHz) {
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

void updateMotorRamp() {
  if (!motorOn)
    return;
  // DECELERATION
  if (rampDecel) {
    currentFreqHz -= MOTOR_DECEL_STEP_HZ;

    if (currentFreqHz <= START_FREQ_HZ) {
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
  if (currentState == STATE_RUN) {
    if (rampDecel) {
      currentFreqHz -= MOTOR_DECEL_STEP_HZ;

      if (currentFreqHz < START_FREQ_HZ)
        currentFreqHz = START_FREQ_HZ;
    } else {
      currentFreqHz += RUN_ACCEL_STEP_HZ;

      if (currentFreqHz > RUN_FREQ_HZ)
        currentFreqHz = RUN_FREQ_HZ;
    }
  }

  // ================= JOG =================
  else if (currentState == STATE_JOG) {
    currentFreqHz += RUN_ACCEL_STEP_HZ;

    if (currentFreqHz > JOG_FREQ_HZ)
      currentFreqHz = JOG_FREQ_HZ;
  }

  setMotorFrequency(currentFreqHz);
}

// MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE       MOTOR START/STOP CODE
void startMotion(uint32_t outputRevs) {
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

void startJog() {
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

void stopMotion() {
  rampDecel = true;
}

void stopJog() {
  rampDecel = true;
}

// BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE
// BATTERY CODE  HAS DELAY SO DONT RUN DURING LOOP
uint32_t readBatteryVoltage_mV() {
  analogRead(BAT_PIN);  // Dummy read to allow ADC settling
  delayMicroseconds(50);
  uint32_t raw = analogRead(BAT_PIN);
  uint32_t v_adc_mV = raw * ADC_REF_mV / 1023UL;  // UL forces it to unsigned & long.get voltage at pin
  return v_adc_mV * (R1 + R2) / R2;               // Convert into battery voltage
}
uint8_t batteryPercent6S(uint32_t mV) {
  // Int mV = int(readBatteryVoltage_mV());
  for (int i = 0; i < BAT_TABLE_SIZE; i++) {
    if (mV >= batTable[i].mV)
      return batTable[i].percent;
  }
  return 0;
}

// SETUP
void setup() {
  // Serial
  pinMode(STATUS_LED_PIN, OUTPUT);               // Must be done at start to not leave as input()
  digitalWrite(STATUS_LED_PIN, STATUS_LED_VAL);  // Default display

  // Force values as opposed to assume default
  analogReference(VDD);

  setupButtons();
  pinMode(13, OUTPUT);  // Internal LED

  setupDisplay();
  // Setup motor pins
  pinMode(MOTOR_PUL, OUTPUT);
  pinMode(MOTOR_DIR, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);

  setupPulseTimer(65535);

  RampTimer.initialize();
  RampTimer.attachInterrupt(updateMotorRamp);
  RampTimer.setPeriod(RAMP_UPDATE_US);

  // Battery level
  pinMode(BAT_PIN, INPUT);  // I Belive this is not needed for analog pins but helps readability.
  delay(50);                // allow battery to stabalize
  uint32_t batAvg_mV = 0;
  uint32_t batCounter_mV = 0;
  uint32_t batAvgCount = 4;
  for (int i = 0; i < batAvgCount; i++) {  // average battery % over __ms
    delay(10);
    batCounter_mV += readBatteryVoltage_mV();
    // Serial.println(batCounter_mV);
    // batAvg = batAvg + 60;
  }

  batAvg_mV = (batCounter_mV / batAvgCount);
  batLvl = (uint8_t)batteryPercent6S(batAvg_mV);
  // Display battery level
  setDisplayValue(batLvl);
  uint32_t temp = millis();
  while (temp + 2000 > millis()) {  // Shows battery % for 2s
    updateDisplay();
  }
  setDisplayValue(0);
  setDisplayFlag(false);
  setDisplayFlag(true);
  STATUS_LED_VAL = HIGH;
  digitalWrite(STATUS_LED_PIN, STATUS_LED_VAL);
}

void finishStop() {
  if (motorOn || !rampDecel)
    return;

  digitalWrite(MOTOR_ENA, LOW);

  if (resetAfterStop) {
    motorRotations = 0;
    setDisplayValue(0);
    setDisplayFlag(false);
  }

  currentState = STATE_IDLE;
  currentFreqHz = START_FREQ_HZ;
  rampDecel = false;
  resetAfterStop = false;
}

void loop() {
  updateDisplay();                         // Always runs to dispaly something
  digitalWrite(13, motorOn ? HIGH : LOW);  // REMOVE AFTER just for debugging
  updateButtons();
  refreshDisplayValue();
  finishStop();
}
