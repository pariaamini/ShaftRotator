#include <Arduino.h>
#include "battery.h"
#include "display.h"
#include "constants.h"

// Original Battery Code --
struct BatPoint // Battery is nonlinear so a value table is used
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

// End of Original Battery Code

uint8_t getBatteryPercent() // battery % getter. Can be called anywhere
{
    uint32_t total_mV = 0;

    constexpr int sampleCount = 4; // takes 4 samples with a delay of 10ms

    for (int i = 0; i < sampleCount; i++)
    {
        delay(10);
        total_mV += readBatteryVoltage_mV();
    }

    uint32_t average_mV = total_mV / sampleCount; // finds avg of 4 samples to get current battery level

    return batteryPercent6S(average_mV); // returns current
}

void showBatteryPercentage() // can be run at any point to show the battery voltage on the display 
{
    uint8_t batLvl = getBatteryPercent();
    showTemporaryValue(batLvl, 2000); // will show for two seconds
}

void setupBattery() // battery init -> ran in setup() in main
{
    pinMode(BAT_PIN, INPUT);
    analogReference(VDD);
}