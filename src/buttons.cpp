#include "Buttons.h"
#include <OneButton.h>

// Button pin assignments
#define PIN_PLUS_1 2           // Pin D2 on Arduino, Btn3 on PCB
#define PIN_MINUS_1 3          // Pin D3 on Arduino, Btn2 on PCB
#define PIN_JOG 4              // Pin D4 on Arduino, Btn1 on PCB
#define PIN_START_PAUSE_STOP 5 // Pin D5 on Arduino, Btn4 on PCB

OneButton buttonUp(PIN_PLUS_1, true);            // buttonUp -> +1 Rot Button
OneButton buttonDown(PIN_MINUS_1, true);         // buttonDown -> -1 Rot Button
OneButton buttonJog(PIN_JOG, true);              // buttonJog -> Jog Button
OneButton buttonSPS(PIN_START_PAUSE_STOP, true); // buttonSPS -> Start/Pause/Stop Button

// Shared system state.
volatile SystemState currentState = STATE_IDLE;
volatile MotorMode motorMode = MODE_STOPPED;
int targetRotations = 0;

// Time when the target rotation count was last changed during a long press.
unsigned long lastTargetRotationChangeMs = 0;

// Pass the same information to up AND down buttons, int direction
// determines if the target rotation count increases or decreases
struct ButtonInfo
{
  OneButton *button;
  int direction; // +1 or -1
};

ButtonInfo upInfo = {&buttonUp, 1};
ButtonInfo downInfo = {&buttonDown, -1};

// Declare callbacks before they are assigned in setupButtons().
void rotDirection(bool increase);
void changeRot(void *context);

void startAccelRotChange(void *context);
void whileAccelRotChange(void *context);

void jogStart();
void jogStop();

void toggleStartPause();
void stopRotations();

void setupButtons()
{
  // Up Button Assignments
  buttonUp.attachClick(changeRot, &upInfo); // increases target rotations by 1
  buttonUp.attachLongPressStart(startAccelRotChange, &upInfo);
  buttonUp.attachDuringLongPress(whileAccelRotChange, &upInfo);

  // Down Button Assignments
  buttonDown.attachClick(changeRot, &downInfo); // decreases target rotations by 1
  buttonDown.attachLongPressStart(startAccelRotChange, &downInfo);
  buttonDown.attachDuringLongPress(whileAccelRotChange, &downInfo);

  // Up/Down Button Timing
  buttonUp.setPressMs(1000); // duration that button needs to be held to trigger long press (default 800ms)
  buttonDown.setPressMs(1000);
  buttonUp.setLongPressIntervalMs(10); // how often long press function is checked (default 0ms)
  buttonDown.setLongPressIntervalMs(10);

  // Jog Button Assignments
  buttonJog.attachLongPressStart(jogStart);
  buttonJog.attachLongPressStop(jogStop);
  buttonJog.setPressMs(500); // length of time button must be held to trigger a long press

  // Start/Pause/Stop Button Assignments
  buttonSPS.attachClick(toggleStartPause);       // toggle between start and pause system Sstates
  buttonSPS.setPressMs(3000);                    // length of time button must be held to trigger a long press
  buttonSPS.attachLongPressStart(stopRotations); // if long press, stop device and set target rotations to zero,
                                                 // set currentState to idle
  // buttonSPS.attachDoubleClick(); could be used
  // buttonSPS.setClickMs(); sets duration of time inbetween clicks to interpert double clicks
}

void updateButtons()
{
  buttonUp.tick();
  buttonDown.tick();
  buttonJog.tick();
  buttonSPS.tick();
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
  rotDirection(info->direction == 1); // feeds direction to rotDirection
}

// Up/Down Btn: Long Press
void startAccelRotChange(void *context)
{ // time when long press starts
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
// Jog Btn Methods:
void jogStart()
{
  handleEvent(EVT_JOG_START);
}

void jogStop()
{
  handleEvent(EVT_JOG_STOP);
}

// Start/Pause/Stop Btn Methods:
void toggleStartPause()
{
  if (targetRotations != 0 && currentState != STATE_JOG)
  {
    handleEvent(EVT_START_PAUSE);
  }
}

void stopRotations()
{
  handleEvent(EVT_STOP);
}

void handleEvent(Event event)
{
  switch (event)
  {
  case EVT_INC_1:
    targetRotations++;
    break;

  case EVT_DEC_1:
    if (targetRotations > 0)
    {
      targetRotations--;
    }
    break;

  case EVT_JOG_START:
    currentState = STATE_JOG;
    motorMode = MODE_JOG;
    break;

  case EVT_JOG_STOP:
    currentState = STATE_IDLE;
    motorMode = MODE_STOPPED;
    break;

  case EVT_START_PAUSE:
    if (currentState == STATE_IDLE)
    {
      currentState = STATE_RUNNING;
      motorMode = MODE_RUN;
    }
    else if (currentState == STATE_RUNNING)
    {
      currentState = STATE_IDLE;
      motorMode = MODE_STOPPED;
    }
    break;

  case EVT_STOP:
    currentState = STATE_IDLE;
    targetRotations = 0;
    motorMode = MODE_STOPPED;
    break;
  }
}