#include <EveryTimerB.h>

#include "buttons.h"
#include "system.h"
#include "display.h"
#include "motor.h"

// Main global variables
const int STATUS_LED_PIN = 7; // Pin used for Bat/Count LEDS
bool STATUS_LED_VAL = LOW;    // LOW for Battery HIGH for Count
// Battery Level
const int BAT_PIN = A7; // Same as D21 (21)
uint8_t batLvl = 0;
uint32_t bat_mV = 0;
uint32_t lastBatRead = 0; // Time of last battery read
// Voltage Divider Vout = Vin * (R2/(R2+R1)) this should give max 4.2v which is safely under 5v max (Vin max is 25.2V)
const float R1 = 100000.0;        // ohms R1 of voltage divider
const float R2 = 20000.0;         // ohms R2 of voltage divider
const uint32_t ADC_REF_mV = 5000; // Nano Every reference
// uint32_t lowBat_mV = 6 * 3300;  //Do not let pack go under this voltage. Update this as needed. 3300mv minimum per cell, 6 cells.
// Battery is nonlinear so a value table is used
struct BatPoint
{
  uint16_t mV;
  uint8_t percent;
};
const BatPoint batTable[] = { // EYBMS fuel gauge and other online resources used to make this table. Testing of battery to verify accuracy would be recomended.
    {24600, 100},
    {24000, 90},
    {23400, 80},
    {22800, 70},
    {21900, 60},
    {21000, 50},
    {20400, 40},
    {19800, 30},
    {19200, 20},
    {18600, 10},
    {18000, 0}};
const int BAT_TABLE_SIZE = sizeof(batTable) / sizeof(batTable[0]);

// BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE      BATTERY CODE
// BATTERY CODE  HAS DELAY SO DONT RUN DURING LOOP
uint32_t readBatteryVoltage_mV()
{
  analogRead(BAT_PIN); // Dummy read to allow ADC settling
  delayMicroseconds(50);
  uint32_t raw = analogRead(BAT_PIN);
  uint32_t v_adc_mV = raw * ADC_REF_mV / 1023UL; // UL forces it to unsigned & long.get voltage at pin
  return v_adc_mV * (R1 + R2) / R2;              // Convert into battery voltage
}
uint8_t batteryPercent6S(uint32_t mV)
{
  // Int mV = int(readBatteryVoltage_mV());
  for (int i = 0; i < BAT_TABLE_SIZE; i++)
  {
    if (mV >= batTable[i].mV)
      return batTable[i].percent;
  }
  return 0;
}

// SETUP
void setup()
{
  // Serial
  pinMode(STATUS_LED_PIN, OUTPUT);              // Must be done at start to not leave as input()
  digitalWrite(STATUS_LED_PIN, STATUS_LED_VAL); // Default display

  // Force values as opposed to assume default
  analogReference(VDD);

  setupButtons();
  pinMode(13, OUTPUT); // Internal LED

  setupDisplay();

  // Setup motor pins
  setUpMotor();

  // Battery level
  pinMode(BAT_PIN, INPUT); // I Belive this is not needed for analog pins but helps readability.
  delay(50);               // allow battery to stabalize
  uint32_t batAvg_mV = 0;
  uint32_t batCounter_mV = 0;
  uint32_t batAvgCount = 4;
  for (int i = 0; i < batAvgCount; i++)
  { // average battery % over __ms
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
  while (temp + 2000 > millis())
  { // Shows battery % for 2s
    updateDisplay();
  }
  setDisplayValue(0);
  setDisplayFlag(false);
  setDisplayFlag(true);
  STATUS_LED_VAL = HIGH;
  digitalWrite(STATUS_LED_PIN, STATUS_LED_VAL);
}

void loop()
{
  updateDisplay();                        // Always runs to dispaly something
  digitalWrite(13, isMotorOn() ? HIGH : LOW); // REMOVE AFTER just for debugging
  updateButtons();
  refreshDisplayValue();
  finishStop();
}
