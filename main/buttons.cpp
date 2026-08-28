#include <OneButton.h>

#include "buttons.h"
#include "constants.h"
#include "system.h"

unsigned long lastTargetRotationChangeMs = 0;

OneButton buttonUp(PIN_UP, true);
OneButton buttonDown(PIN_DOWN, true);
OneButton buttonJog(PIN_JOG, true);
OneButton buttonStart(PIN_START_PAUSE_STOP, true);

struct ButtonInfo {
  OneButton *button;
  int direction;   // +1 to increase target rotations, -1 to decrease
};

ButtonInfo upInfo = { &buttonUp, 1 };
ButtonInfo downInfo = { &buttonDown, -1 };

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
void showBattery() {
  handleEvent(EVT_SHOW_BATTERY);
}

// Up/Down Btns: Single Click
void rotDirection(bool polarity) {
  if (polarity) {  // positive direction: increase target rotations by 1
    handleEvent(EVT_INC_1);
  } else {  // negative direction: decrease target rotations by 1
    handleEvent(EVT_DEC_1);
  }
}
void changeRot(void *context)
{
    ButtonInfo *info = (ButtonInfo *)context;

    rotDirection(info->direction == 1);
}



void startAccelRotChange(void *context) {
  lastTargetRotationChangeMs = millis();
}

void whileAccelRotChange(void *context) {
  ButtonInfo *info = (ButtonInfo *)context;
  unsigned long buttonHeldMs = info->button->getPressedMs();  //  time button has been held
  unsigned long now = millis();                           
  unsigned long timeBetweenRotValueChangesMs;                 // Current repeat interval

  // Speed up rotation changes the longer the button is held
  if (buttonHeldMs < ACCEL_STAGE_1_END_MS) {
    timeBetweenRotValueChangesMs = ROT_CHANGE_SLOW_MS;
  } else if (buttonHeldMs < ACCEL_STAGE_2_END_MS) {
    timeBetweenRotValueChangesMs = ROT_CHANGE_MEDIUM_MS;
  } else {
    timeBetweenRotValueChangesMs = ROT_CHANGE_FAST_MS;
  }

  // Change the target rotation once the current repeat interval has passed
  if (now - lastTargetRotationChangeMs >= timeBetweenRotValueChangesMs) {
    rotDirection(info->direction == 1);
    lastTargetRotationChangeMs = now;
  }
}



void setupButtons() {
  // up/down button behaviour.
  buttonUp.attachClick(changeRot, &upInfo);
  buttonUp.attachLongPressStart(startAccelRotChange, &upInfo);
  buttonUp.attachDuringLongPress(whileAccelRotChange, &upInfo);

  buttonDown.attachClick(changeRot, &downInfo);
  buttonDown.attachLongPressStart(startAccelRotChange, &downInfo);
  buttonDown.attachDuringLongPress(whileAccelRotChange, &downInfo);

  buttonUp.setPressMs(ROTATION_LONG_PRESS_MS);  // sets how long the button has to be help before it is considered a "long press"
  buttonDown.setPressMs(ROTATION_LONG_PRESS_MS);

  buttonUp.setLongPressIntervalMs(BUTTON_LONG_PRESS_UPDATE_MS);  // sets how often the DuringLongPress procedure is called while button is held
  buttonDown.setLongPressIntervalMs(BUTTON_LONG_PRESS_UPDATE_MS);

  // jog button behaviour
  buttonJog.attachLongPressStart(jogButtonStart);
  buttonJog.attachLongPressStop(jogButtonStop);
  buttonJog.setPressMs(JOG_LONG_PRESS_MS);

  buttonJog.attachDoubleClick(showBattery);
  buttonJog.setClickMs(JOG_DOUBLE_CLICK_DELAY_MS);

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
