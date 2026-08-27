#include <OneButton.h>
#include <EveryTimerB.h>

#define RampTimer TimerB2

// Button pins
constexpr uint8_t PIN_UP = 2;
constexpr uint8_t PIN_DOWN = 3;
constexpr uint8_t PIN_JOG = 6;
constexpr uint8_t PIN_START_PAUSE_STOP = 4;  // aligns to +5 button

// Button timing
constexpr unsigned long ROTATION_LONG_PRESS_MS = 1000;
constexpr unsigned long BUTTON_LONG_PRESS_UPDATE_MS = 10;
constexpr unsigned long JOG_LONG_PRESS_MS = 500;
constexpr unsigned long STOP_LONG_PRESS_MS = 2000;

unsigned long lastTargetRotationChangeMs = 0;

OneButton buttonUp(PIN_UP, true);
OneButton buttonDown(PIN_DOWN, true);
OneButton buttonJog(PIN_JOG, true);
OneButton buttonStart(PIN_START_PAUSE_STOP, true);

struct ButtonInfo {
  OneButton *button;
  int direction;  // if the rotation is supposed to increase or decrease (by 1)
};

ButtonInfo upInfo = { &buttonUp, 1 };
ButtonInfo downInfo = { &buttonDown, -1 };

// LED segment layout for 0-9
// Segment pins: A B C D E F G
const byte segPins[2][7] = {
  // Different segments for each the different digits
  // Arduino Pin, segment letter, led pin, wire colour
  {       // For digit 1
    9,    // A //Pin 2 //Black
    12,   // B //Pin 6 //Blue
    15,   // C //Pin 9 //Orange
    11,   // D //Pin 5 //Purple
    14,   // E //Pin 8 //Yellow
    10,   // F //Pin 3 //White
    8 },  // G //Pin 1 //Brown
  {       // For digit 2
    9,    // A //Pin 2 //Black
    14,   // B //Pin 8 //Yellow
    12,   // C //Pin 6 //Blue
    11,   // D //Pin 5 //Purple
    10,   // E //Pin 3 //White
    15,   // F //Pin 9 //Orange
    8 }   // G //Pin 1 //Brown}
};
// Digit anodes
const int DIG1 = 16;  // Left digit //Pin7 green
const int DIG2 = 17;  // Right digit //Pin4 grey
// Common-anode segment map (0 = ON, 1 = OFF)
const byte digits[10][7] = {
  { 0, 0, 0, 0, 0, 0, 1 },  // 0
  { 1, 0, 0, 1, 1, 1, 1 },  // 1
  { 0, 0, 1, 0, 0, 1, 0 },  // 2
  { 0, 0, 0, 0, 1, 1, 0 },  // 3
  { 1, 0, 0, 1, 1, 0, 0 },  // 4
  { 0, 1, 0, 0, 1, 0, 0 },  // 5
  { 0, 1, 0, 0, 0, 0, 0 },  // 6
  { 0, 0, 0, 1, 1, 1, 1 },  // 7
  { 0, 0, 0, 0, 0, 0, 0 },  // 8
  { 0, 0, 0, 0, 1, 0, 0 }   // 9
};

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

// StateMachine
enum SystemState {  // For Readability
  STATE_IDLE,
  STATE_RUN,
  STATE_JOG,
  STATE_PAUSE

};
enum Event {
  EVT_NONE,
  EVT_INC_1,
  EVT_DEC_1,
  EVT_START_PAUSE,
  EVT_STOP_RESET,
  EVT_JOG_START,
  EVT_JOG_STOP
};

enum MotorMode {
  MODE_STOPPED,
  MODE_RUN,
  MODE_JOG
};
volatile SystemState currentState = STATE_IDLE;

// Display Variables
uint32_t lastRefresh = 0;
const uint32_t refreshInterval = 3;  // In ms
bool showLeft = true;
volatile int displayValue = 0;
volatile bool displayFlag = false;  // Flag to display just centre segments 'g'
uint32_t now = 0;

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

// FUNCTION DECLARATIONS
// Displays 1 digit per call from the global varible of the displayValue. That digit is left on between calls.
void updateDisplay() {
  uint32_t now = millis();
  if (now - lastRefresh < refreshInterval)
    return;
  lastRefresh = now;  // Set last refresh to current time

  // Turn digits off
  digitalWrite(DIG1, HIGH);
  digitalWrite(DIG2, HIGH);

  int activeDigit = showLeft ? 0 : 1;                                 // Swaps since we want 0 for left and 1 for right
  int digitValue = showLeft ? displayValue / 10 : displayValue % 10;  // Takes the required digit out of dispalyValue to write

  // Enter only if wanting to show centre lines only not a number
  if (displayFlag) {
    for (int i = 0; i < 7; i++) {  // Sets all segments OFF
      digitalWrite(segPins[activeDigit][i], HIGH);
    }
    digitalWrite(8, LOW);                       // Set 'G' to ON
    digitalWrite(showLeft ? DIG1 : DIG2, LOW);  // Turn ON digit
    showLeft = !showLeft;
    return;
  }
  // For writing number to display
  for (int i = 0; i < 7; i++) {
    digitalWrite(segPins[activeDigit][i], digits[digitValue][i] ? HIGH : LOW);
  }
  digitalWrite(showLeft ? DIG1 : DIG2, LOW);
  showLeft = !showLeft;
}

void jogButtonStart() {
  handleEvent(EVT_JOG_START);
}

void jogButtonStop() {
  handleEvent(EVT_JOG_STOP);
}

void toggleStartPause() {
  digitalWrite(13, HIGH);
  delay(200);
  digitalWrite(13, LOW);

  handleEvent(EVT_START_PAUSE);
}

void stopAndReset() {
  handleEvent(EVT_STOP_RESET);
}

// Up/Down Btn: Single Click
void rotDirection(bool polarity) {
  if (polarity) {  // if dir +ve, increase # of rotations by 1
    handleEvent(EVT_INC_1);
  } else {  // if dir -ve, decrease # of rotations by 1
    handleEvent(EVT_DEC_1);
  }
}

void changeRot(void *context) {  // changes target rotations by 1
  ButtonInfo *info = (ButtonInfo *)context;
  rotDirection(info->direction == 1);
}

void startAccelRotChange(void *context) {
  lastTargetRotationChangeMs = millis();
}

void whileAccelRotChange(void *context) {
  ButtonInfo *info = (ButtonInfo *)context;
  unsigned long buttonHeldMs = info->button->getPressedMs();  // how long the button has been held
  unsigned long now = millis();                               // current time
  unsigned long timeBetweenRotValueChangesMs;                 // how quick the amount of rotations is changing

  // Increase the repeat rate as the button is held
  if (buttonHeldMs < 2000) {  // if held for < 2s, change the target rotation # by 1 every 500ms
    timeBetweenRotValueChangesMs = 500;
  } else if (buttonHeldMs < 4000) {
    timeBetweenRotValueChangesMs = 250;  // if held for < 4s, change the target rotation # by 1 every 250ms
  } else {
    timeBetweenRotValueChangesMs = 100;  // if held for > 4s, change the target rotation # by 1 every 100ms
  }

  // Change target rotation # by 1 when timeBetweenRotValueChangesMs is surpassed
  if (now - lastTargetRotationChangeMs >= timeBetweenRotValueChangesMs) {
    rotDirection(info->direction == 1);
    lastTargetRotationChangeMs = now;
  }
}

void setupButtons() {
  // up/down button behaviour
  buttonUp.attachClick(changeRot, &upInfo);
  buttonUp.attachLongPressStart(startAccelRotChange, &upInfo);
  buttonUp.attachDuringLongPress(whileAccelRotChange, &upInfo);

  buttonDown.attachClick(changeRot, &downInfo);
  buttonDown.attachLongPressStart(startAccelRotChange, &downInfo);
  buttonDown.attachDuringLongPress(whileAccelRotChange, &downInfo);

  buttonUp.setPressMs(ROTATION_LONG_PRESS_MS);
  buttonDown.setPressMs(ROTATION_LONG_PRESS_MS);

  buttonUp.setLongPressIntervalMs(BUTTON_LONG_PRESS_UPDATE_MS);
  buttonDown.setLongPressIntervalMs(BUTTON_LONG_PRESS_UPDATE_MS);

  // jog button behaviour
  buttonJog.setPressMs(JOG_LONG_PRESS_MS);
  buttonJog.attachLongPressStart(jogButtonStart);
  buttonJog.attachLongPressStop(jogButtonStop);

  // start/pause and stop behaviour
  buttonStart.attachClick(toggleStartPause);
  buttonStart.setPressMs(STOP_LONG_PRESS_MS);
  buttonStart.attachLongPressStart(stopAndReset);
}

void updateButtons() {
  buttonUp.tick();
  buttonDown.tick();
  buttonStart.tick();
  buttonJog.tick();
}

// The state button handler
// STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE      STATE MACHINE CODE
void handleEvent(Event e) {
  switch (e) {

    case EVT_INC_1:
      motorRotations = min(motorRotations + 1, 99);
      displayValue = motorRotations;
      displayFlag = false;
      break;

    case EVT_DEC_1:
      motorRotations = max(motorRotations - 1, 0);
      displayValue = motorRotations;
      displayFlag = false;
      break;

    case EVT_START_PAUSE:
      if (motorRotations == 0 || currentState == STATE_JOG) {  // if 0 rotations or in jogging mode
        break;
      } else if (currentState == STATE_IDLE || currentState == STATE_PAUSE) {  // if idle or paused, start the motor
        motorOn = true;
        currentState = STATE_RUN;
        startMotion(motorRotations);
      } else if (currentState == STATE_RUN) {  // if running, pause the motor
        motorRotations = getRotationsRemaining();
        displayValue = motorRotations;
        resetAfterStop = false;
        stopMotion();
        currentState = STATE_PAUSE;
      }
      break;


    case EVT_JOG_START:
      if (currentState != STATE_RUN) {
        currentState = STATE_JOG;
        motorOn = true;
        startJog();
      }
      break;

    case EVT_JOG_STOP:
      if (currentState == STATE_JOG) {
        stopJog();  // this should start deceleration
      }
      break;
  }
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
      displayValue = 0;
      motorRotations = 0;
      displayFlag = true;  // Display just centre segments
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

// void stopMotion() {
//   noInterrupts();

//   motorOn = false;

//   TCB0.CTRLA &= ~TCB_ENABLE_bm;

//   interrupts();

//   digitalWrite(MOTOR_ENA, LOW);

//   currentFreqHz = START_FREQ_HZ;
// }

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
// void stopJog() {
//   noInterrupts();

//   motorOn = false;

//   TCB0.CTRLA &= ~TCB_ENABLE_bm;

//   interrupts();

//   digitalWrite(MOTOR_ENA, LOW);

//   currentFreqHz = START_FREQ_HZ;
// }

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

  // For 7 seg. Segments are LOW on. Digits are HIGH on.
  for (int i = 0; i < 7; i++)
    pinMode(segPins[0][i], OUTPUT);
  // Setup the common anodes and turn them off (swap to high if adding a PNP transistor)
  pinMode(DIG1, OUTPUT);
  pinMode(DIG2, OUTPUT);
  digitalWrite(DIG1, HIGH);
  digitalWrite(DIG2, HIGH);

  setupButtons();
  pinMode(13, OUTPUT);  // Internal LED

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
  displayValue = batLvl;
  uint32_t temp = millis();
  while (temp + 2000 > millis()) {  // Shows battery % for 2s
    updateDisplay();
    // DisplayValue = batteryPercent6S();
  }
  displayValue = motorRotations;  // Displays motor rotations (resets display to zero before proceeding)
  displayFlag = true;             // Displays centre segments only
  STATUS_LED_VAL = HIGH;
  digitalWrite(STATUS_LED_PIN, STATUS_LED_VAL);
}

void finishStop() {
  if (motorOn || !rampDecel)
    return;

  digitalWrite(MOTOR_ENA, LOW);

  if (resetAfterStop) {
    motorRotations = 0;
    displayValue = 0;
    displayFlag = false;
  }

  currentState = STATE_IDLE;

  currentFreqHz = START_FREQ_HZ;
  rampDecel = false;
  resetAfterStop = false;
}

void loop() {
  updateDisplay();                         // Always runs to dispaly something
  digitalWrite(13, motorOn ? HIGH : LOW);  // REMOVE AFTER just for debugging

  buttonUp.tick();
  buttonDown.tick();
  buttonStart.tick();
  buttonJog.tick();

  finishStop();
}
