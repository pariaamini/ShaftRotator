
#include <Arduino.h>
#include <OneButton.h>

#include "buttons.h"
#include "constants.h"
#include "system.h"

OneButton buttonUp(PIN_UP, true);
OneButton buttonDown(PIN_DOWN, true);
OneButton buttonJog(PIN_JOG, true);
OneButton buttonStart(PIN_START_PAUSE_STOP, true);

struct ButtonInfo
{
    OneButton *button;
    int direction; // if the rotation is supposed to increase or decrease (by 1)
};

ButtonInfo upInfo = {&buttonUp, 1};
ButtonInfo downInfo = {&buttonDown, -1};

void jogButtonStart()
{
    handleEvent(EVT_JOG_START);
}

void jogButtonStop()
{
    handleEvent(EVT_JOG_STOP);
}

void toggleStartPause()
{
    digitalWrite(13, HIGH);
    delay(200);
    digitalWrite(13, LOW);

    handleEvent(EVT_START_PAUSE);
}

void stopAndReset()
{
    handleEvent(EVT_STOP_RESET);
}

// Up/Down Btn: Single Click
void rotDirection(bool polarity)
{
    if (polarity)
    { // if dir +ve, increase # of rotations by 1
        handleEvent(EVT_INC_1);
    }
    else
    { // if dir -ve, decrease # of rotations by 1
        handleEvent(EVT_DEC_1);
    }
}

void changeRot(void *context)
{ // changes target rotations by 1
    ButtonInfo *info = (ButtonInfo *)context;
    rotDirection(info->direction == 1);
}

void startAccelRotChange(void *context)
{
    lastTargetRotationChangeMs = millis();
}

void whileAccelRotChange(void *context)
{
    ButtonInfo *info = (ButtonInfo *)context;
    unsigned long buttonHeldMs = info->button->getPressedMs(); // how long the button has been held
    unsigned long now = millis();                              // current time
    unsigned long timeBetweenRotValueChangesMs;                // how quick the amount of rotations is changing

    // Increase the repeat rate as the button is held
    if (buttonHeldMs < 2000)
    { // if held for < 2s, change the target rotation # by 1 every 500ms
        timeBetweenRotValueChangesMs = 500;
    }
    else if (buttonHeldMs < 4000)
    {
        timeBetweenRotValueChangesMs = 250; // if held for < 4s, change the target rotation # by 1 every 250ms
    }
    else
    {
        timeBetweenRotValueChangesMs = 100; // if held for > 4s, change the target rotation # by 1 every 100ms
    }

    // Change target rotation # by 1 when timeBetweenRotValueChangesMs is surpassed
    if (now - lastTargetRotationChangeMs >= timeBetweenRotValueChangesMs)
    {
        rotDirection(info->direction == 1);
        lastTargetRotationChangeMs = now;
    }
}

void setupButtons()
{
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

void updateButtons()
{
    buttonUp.tick();
    buttonDown.tick();
    buttonStart.tick();
    buttonJog.tick();
}