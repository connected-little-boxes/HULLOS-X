#include "Motors.h"
#include "Utils.h"

#include <Arduino.h>
#include <math.h>

#include "settings.h"
#include "processes.h"
#include "errors.h"
#include "mqtt.h"
#include "Motors.h"

float turningCircle;
const int countsperrev = 512 * 8; // number of microsteps per full revolution

float leftStepsPerMM;
float rightStepsPerMM;
float leftWheelCircumference;
float rightWheelCircumference;

struct MotorSettings motorSettings;

struct SettingItem motorsOnOffSetting = {
    "Motors Active (yes or no)",
    "motors",
    &motorSettings.motorsActive,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

void setDefaultWheelDiameter(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 69;
}

struct SettingItem motorLeftWheelDiamSetting = {"Left wheel diameter",
                                                "motorleftwheeldiam",
                                                &motorSettings.leftWheelDiameter,
                                                NUMBER_INPUT_LENGTH,
                                                integerValue,
                                                setDefaultWheelDiameter,
                                                validateInt};

struct SettingItem motorRightWheelDiamSetting = {"Right wheel diameter",
                                                 "motorrightwheeldiam",
                                                 &motorSettings.rightWheelDiameter,
                                                 NUMBER_INPUT_LENGTH,
                                                 integerValue,
                                                 setDefaultWheelDiameter,
                                                 validateInt};

void setDefaultWheelSpacing(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 110;
}

struct SettingItem motorWheelSpacingSetting = {"Wheel spacing",
                                               "motorwheelspacing",
                                               &motorSettings.wheelSpacing,
                                               NUMBER_INPUT_LENGTH,
                                               integerValue,
                                               setDefaultWheelSpacing,
                                               validateInt};

void setDefaultLeftMotorPin1(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 15;
}

struct SettingItem motorLeftPin1 = {"Left motor pin 1",
                                    "leftmotorpin1",
                                    &motorSettings.lpin1,
                                    NUMBER_INPUT_LENGTH,
                                    integerValue,
                                    setDefaultLeftMotorPin1,
                                    validateInt};

void setDefaultLeftMotorPin2(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 14;
}

struct SettingItem motorLeftPin2 = {"Left motor pin 2",
                                    "leftmotorpin2",
                                    &motorSettings.lpin2,
                                    NUMBER_INPUT_LENGTH,
                                    integerValue,
                                    setDefaultLeftMotorPin2,
                                    validateInt};

void setDefaultLeftMotorPin3(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 13;
}

struct SettingItem motorLeftPin3 = {"Left motor pin 3",
                                    "leftmotorpin3",
                                    &motorSettings.lpin3,
                                    NUMBER_INPUT_LENGTH,
                                    integerValue,
                                    setDefaultLeftMotorPin3,
                                    validateInt};

void setDefaultLeftMotorPin4(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 12;
}

struct SettingItem motorLeftPin4 = {"Left motor pin 4",
                                    "leftmotorpin4",
                                    &motorSettings.lpin4,
                                    NUMBER_INPUT_LENGTH,
                                    integerValue,
                                    setDefaultLeftMotorPin4,
                                    validateInt};

void setDefaultRightMotorPin1(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 8;
}

struct SettingItem motorRightPin1 = {"Right motor pin 1",
                                     "rightmotorpin1",
                                     &motorSettings.rpin1,
                                     NUMBER_INPUT_LENGTH,
                                     integerValue,
                                     setDefaultRightMotorPin1,
                                     validateInt};

void setDefaultRightMotorPin2(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 9;
}

struct SettingItem motorRightPin2 = {"Right motor pin 2",
                                     "rightmotorpin2",
                                     &motorSettings.rpin2,
                                     NUMBER_INPUT_LENGTH,
                                     integerValue,
                                     setDefaultRightMotorPin2,
                                     validateInt};

void setDefaultRightMotorPin3(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 10;
}

struct SettingItem motorRightPin3 = {"Right motor pin 3",
                                     "rightmotorpin3",
                                     &motorSettings.rpin3,
                                     NUMBER_INPUT_LENGTH,
                                     integerValue,
                                     setDefaultRightMotorPin3,
                                     validateInt};

void setDefaultRightMotorPin4(void *dest)
{
  int *destInt = (int *)dest;
  *destInt = 11;
}

struct SettingItem motorRightPin4 = {"Right motor pin 4",
                                     "rightmotorpin4",
                                     &motorSettings.rpin4,
                                     NUMBER_INPUT_LENGTH,
                                     integerValue,
                                     setDefaultRightMotorPin4,
                                     validateInt};

struct SettingItem *motorSettingItemPointers[] =
    {
        &motorsOnOffSetting,
        &motorLeftWheelDiamSetting,
        &motorRightWheelDiamSetting,
        &motorWheelSpacingSetting,
        &motorLeftPin1,
        &motorLeftPin2,
        &motorLeftPin3,
        &motorLeftPin4,
        &motorRightPin1,
        &motorRightPin2,
        &motorRightPin3,
        &motorRightPin4};

struct SettingItemCollection motorSettingItems = {
    "motor",
    "Motor and wheel configuration",
    motorSettingItemPointers,
    sizeof(motorSettingItemPointers) / sizeof(struct SettingItem *)};

void setupWheelSettings()
{
  leftWheelCircumference = PI * motorSettings.leftWheelDiameter;
  rightWheelCircumference = PI * motorSettings.rightWheelDiameter;
  turningCircle = motorSettings.wheelSpacing * PI;

  leftStepsPerMM = countsperrev / leftWheelCircumference;
  rightStepsPerMM = countsperrev / rightWheelCircumference;
}

const char motorWaveformLookup[8] = {0b01000, 0b01100, 0b00100, 0b00110, 0b00010, 0b00011, 0b00001, 0b01001};

volatile int leftMotorWaveformPos = 0;
volatile int leftMotorWaveformDelta = 0;

volatile int rightMotorWaveformPos = 0;
volatile int rightMotorWaveformDelta = 0;

volatile unsigned long leftStepCounter = 0;
volatile unsigned long leftNumberOfStepsToMove = 0;

volatile unsigned long rightStepCounter = 0;
volatile unsigned long rightNumberOfStepsToMove = 0;

volatile unsigned long minInterruptIntervalInMicroSecs = 1200;

inline void setLeft(byte bits)
{
  digitalWrite(motorSettings.lpin1, bits & 1);
  digitalWrite(motorSettings.lpin2, bits & 2);
  digitalWrite(motorSettings.lpin3, bits & 4);
  digitalWrite(motorSettings.lpin4, bits & 8);
}

inline void setRight(byte bits)
{
  digitalWrite(motorSettings.rpin1, bits & 1);
  digitalWrite(motorSettings.rpin2, bits & 2);
  digitalWrite(motorSettings.rpin3, bits & 4);
  digitalWrite(motorSettings.rpin4, bits & 8);
}

#ifdef ARDUINO_ARCH_ESP32

hw_timer_t *leftTimer = NULL;
hw_timer_t *rightTimer = NULL;

void IRAM_ATTR onLeft()
{
  // If we are not moving, don't do anything
  if (leftMotorWaveformDelta != 0)
  {
    if (++leftStepCounter > leftNumberOfStepsToMove)
    {
      leftMotorWaveformDelta = 0;
      setLeft(0);
      timerAlarmDisable(leftTimer);
    }
    else
    {
      // Move the motor one step
      setLeft(motorWaveformLookup[leftMotorWaveformPos]);

      // Update and wrap the waveform position
      leftMotorWaveformPos += leftMotorWaveformDelta;
      if (leftMotorWaveformPos == 8)
        leftMotorWaveformPos = 0;
      if (leftMotorWaveformPos < 0)
        leftMotorWaveformPos = 7;
    }
  }
}

void ARDUINO_ISR_ATTR onRight()
{
  // If we are not moving, don't do anything
  if (rightMotorWaveformDelta != 0)
  {
    if (++rightStepCounter > rightNumberOfStepsToMove)
    {
      rightMotorWaveformDelta = 0;
      setRight(0);
      timerAlarmDisable(rightTimer);
    }
    else
    {
      // Move the motor one step

      setRight(motorWaveformLookup[rightMotorWaveformPos]);

      // Update and wrap the waveform position
      rightMotorWaveformPos += rightMotorWaveformDelta;
      if (rightMotorWaveformPos == 8)
        rightMotorWaveformPos = 0;
      if (rightMotorWaveformPos < 0)
        rightMotorWaveformPos = 7;
    }
  }
}

void initMotors()
{
  loadActiveWheelSettings();
  setupWheelSettings();

  pinMode(motorSettings.lpin1, OUTPUT);
  pinMode(motorSettings.lpin2, OUTPUT);
  pinMode(motorSettings.lpin3, OUTPUT);
  pinMode(motorSettings.lpin4, OUTPUT);

  pinMode(motorSettings.rpin1, OUTPUT);
  pinMode(motorSettings.rpin2, OUTPUT);
  pinMode(motorSettings.rpin3, OUTPUT);
  pinMode(motorSettings.rpin4, OUTPUT);

  leftTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(leftTimer, &onLeft, true);

  rightTimer = timerBegin(1, 80, true);
  timerAttachInterrupt(rightTimer, &onRight, true);
}

pinMode(motorSettings.lpin1, OUTPUT);
pinMode(motorSettings.lpin2, OUTPUT);
pinMode(motorSettings.lpin3, OUTPUT);
pinMode(motorSettings.lpin4, OUTPUT);

pinMode(motorSettings.rpin1, OUTPUT);
pinMode(motorSettings.rpin2, OUTPUT);
pinMode(motorSettings.rpin3, OUTPUT);
pinMode(motorSettings.rpin4, OUTPUT);

leftTimer = timerBegin(0, 80, true);
timerAttachInterrupt(leftTimer, &onLeft, true);

rightTimer = timerBegin(1, 80, true);
timerAttachInterrupt(rightTimer, &onRight, true);
}

MoveFailReason timedMoveSteps(long leftStepsToMove, long rightStepsToMove, float timeToMoveInSeconds)
{
#ifdef DEBUG_TIMED_MOVE
  Serial.println("timedMoveSteps");
  Serial.print("    Left steps to move: ");
  Serial.print(leftStepsToMove);
  Serial.print(" Right steps to move: ");
  Serial.print(rightStepsToMove);
  Serial.print(" Time to move in seconds: ");
  Serial.println(timeToMoveInSeconds);
#endif

  long leftInterruptIntervalInMicroSeconds;
  long absLeftStepsToMove = abs(leftStepsToMove);

  if (leftStepsToMove != 0)
  {
#ifdef DEBUG_TIMED_MOVE
    Serial.println("    Moving left steps..");
#endif
    leftInterruptIntervalInMicroSeconds = (long)((timeToMoveInSeconds / (double)absLeftStepsToMove) * 1000000L + 0.5);
    timerAlarmWrite(leftTimer, leftInterruptIntervalInMicroSeconds, true);
    leftNumberOfStepsToMove = absLeftStepsToMove;
    leftStepCounter = 0;
    if (leftStepsToMove > 0)
    {
      leftMotorWaveformDelta = 1;
    }
    else
    {
      leftMotorWaveformDelta = -1;
    }
    timerAlarmEnable(leftTimer);
  }

  long rightInterruptIntervalInMicroseconds;
  long absRightStepsToMove = abs(rightStepsToMove);

  if (rightStepsToMove != 0)
  {
#ifdef DEBUG_TIMED_MOVE
    Serial.println("    Moving right steps..");
#endif
    rightInterruptIntervalInMicroseconds = (long)((timeToMoveInSeconds / (double)absRightStepsToMove) * 1000000L + 0.5);
    timerAlarmWrite(rightTimer, rightInterruptIntervalInMicroseconds, true);
    rightNumberOfStepsToMove = absRightStepsToMove;
    rightStepCounter = 0;
    if (rightStepsToMove > 0)
    {
      rightMotorWaveformDelta = -1;
    }
    else
    {
      rightMotorWaveformDelta = 1;
    }
    timerAlarmEnable(rightTimer);
  }

#ifdef DEBUG_TIMED_MOVE
  Serial.print("    Left interval in microseconds: ");
  Serial.print(leftInterruptIntervalInMicroSeconds);
  Serial.print(" Right interval in microseconds: ");
  Serial.println(rightInterruptIntervalInMicroseconds);
#endif

  return Move_OK;
}

#endif

#ifdef PICOtimer

struct repeating_timer leftTimer;
struct repeating_timer rightTimer;

bool onLeft(struct repeating_timer *t)
{
  // If we are not moving, don't do anything
  if (leftMotorWaveformDelta != 0)
  {
    if (++leftStepCounter > leftNumberOfStepsToMove)
    {
      leftMotorWaveformDelta = 0;
      setLeft(0);
      cancel_repeating_timer(&leftTimer);
    }
    else
    {
      // Move the motor one step
      setLeft(motorWaveformLookup[leftMotorWaveformPos]);

      // Update and wrap the waveform position
      leftMotorWaveformPos += leftMotorWaveformDelta;
      if (leftMotorWaveformPos == 8)
        leftMotorWaveformPos = 0;
      if (leftMotorWaveformPos < 0)
        leftMotorWaveformPos = 7;
    }
  }
  return true;
}

bool onRight(struct repeating_timer *t)
{
  // If we are not moving, don't do anything
  if (rightMotorWaveformDelta != 0)
  {
    if (++rightStepCounter > rightNumberOfStepsToMove)
    {
      rightMotorWaveformDelta = 0;
      setRight(0);
      cancel_repeating_timer(&rightTimer);
    }
    else
    {
      // Move the motor one step
      setRight(motorWaveformLookup[rightMotorWaveformPos]);

      // Update and wrap the waveform position
      rightMotorWaveformPos += rightMotorWaveformDelta;
      if (rightMotorWaveformPos == 8)
        rightMotorWaveformPos = 0;
      if (rightMotorWaveformPos < 0)
        rightMotorWaveformPos = 7;
    }
  }
  return true;
}

MoveFailReason timedMoveSteps(long leftStepsToMove, long rightStepsToMove, float timeToMoveInSeconds)
{
#ifdef DEBUG_TIMED_MOVE
  Serial.println("timedMoveSteps");
  Serial.print("    Left steps to move: ");
  Serial.print(leftStepsToMove);
  Serial.print(" Right steps to move: ");
  Serial.print(rightStepsToMove);
  Serial.print(" Time to move in seconds: ");
  Serial.println(timeToMoveInSeconds);
#endif

  long leftInterruptIntervalInMicroSeconds;
  long absLeftStepsToMove = abs(leftStepsToMove);

  if (leftStepsToMove != 0)
  {
    cancel_repeating_timer(&leftTimer);

#ifdef DEBUG_TIMED_MOVE
    Serial.println("    Moving left steps..");
#endif
    leftInterruptIntervalInMicroSeconds = (long)((timeToMoveInSeconds / (double)absLeftStepsToMove) * 1000000L + 0.5);
    leftNumberOfStepsToMove = absLeftStepsToMove;
    leftStepCounter = 0;
    if (leftStepsToMove > 0)
    {
      leftMotorWaveformDelta = 1;
    }
    else
    {
      leftMotorWaveformDelta = -1;
    }
    leftTimer.delay_us = leftInterruptIntervalInMicroSeconds;
  }

  long rightInterruptIntervalInMicroseconds;
  long absRightStepsToMove = abs(rightStepsToMove);

  if (rightStepsToMove != 0)
  {
#ifdef DEBUG_TIMED_MOVE
    Serial.println("    Moving right steps..");
#endif
    cancel_repeating_timer(&rightTimer);

    rightInterruptIntervalInMicroseconds = (long)((timeToMoveInSeconds / (double)absRightStepsToMove) * 1000000L + 0.5);
    rightNumberOfStepsToMove = absRightStepsToMove;
    rightStepCounter = 0;
    if (rightStepsToMove > 0)
    {
      rightMotorWaveformDelta = -1;
    }
    else
    {
      rightMotorWaveformDelta = 1;
    }
    add_repeating_timer_us(rightInterruptIntervalInMicroseconds, onRight, NULL, &rightTimer);
  }

#ifdef DEBUG_TIMED_MOVE
  Serial.print("    Left interval in microseconds: ");
  Serial.print(leftInterruptIntervalInMicroSeconds);
  Serial.print(" Right interval in microseconds: ");
  Serial.println(rightInterruptIntervalInMicroseconds);
#endif

  return Move_OK;
}

#endif

#ifdef PICO

unsigned long lastMicros;

volatile unsigned long leftIntervalBetweenSteps;
volatile unsigned long rightIntervalBetweenSteps;

volatile unsigned long leftTimeOfLastStep;
volatile unsigned long rightTimeOfLastStep;

volatile unsigned long leftTimeOfNextStep;
volatile unsigned long rightTimeOfNextStep;

volatile unsigned long currentMicros;
volatile unsigned long leftTimeSinceLastStep;
volatile unsigned long rightTimeSinceLastStep;

volatile bool motorsRunning = false;

inline void startMotor(unsigned long stepLimit, unsigned long microSecsPerPulse, bool forward,
                       volatile unsigned long *motorStepLimit, volatile unsigned long *motorPulseInterval,
                       volatile int *motorDelta, volatile int *motorPos)
{
  // If we are not moving - set the delta to zero and return

  if (stepLimit == 0)
  {
    motorDelta = 0;
    return;
  }

  *motorStepLimit = stepLimit;

  *motorPulseInterval = microSecsPerPulse;

  if (forward)
  {
    *motorDelta = 1;
  }
  else
  {
    *motorDelta = -1;
  }

  *motorPos = 0;
}

void startMotorsCore1(
    unsigned long leftSteps, unsigned long rightSteps,
    unsigned long leftMicroSecsPerPulse, unsigned long rightMicroSecsPerPulse,
    bool leftForward, bool rightForward)
{
  // Set up the counters for the left and right motor moves

  lastMicros = micros();

  leftStepCounter = 0;
  rightStepCounter = 0;

  startMotor(leftSteps, leftMicroSecsPerPulse, leftForward,
             &leftNumberOfStepsToMove, &leftIntervalBetweenSteps, &leftMotorWaveformDelta, &leftMotorWaveformPos);

  startMotor(rightSteps, rightMicroSecsPerPulse, rightForward,
             &rightNumberOfStepsToMove, &rightIntervalBetweenSteps, &rightMotorWaveformDelta, &rightMotorWaveformPos);

  return;
}

MoveFailReason timedMoveSteps(long leftStepsToMove, long rightStepsToMove, float timeToMoveInSeconds)
{
#ifdef DEBUG_TIMED_MOVE
  Serial.println("timedMoveSteps");
  Serial.print("    Left steps to move: ");
  Serial.print(leftStepsToMove);
  Serial.print(" Right steps to move: ");
  Serial.print(rightStepsToMove);
  Serial.print(" Time to move in seconds: ");
  Serial.println(timeToMoveInSeconds);
#endif

  long leftInterruptIntervalInMicroSeconds;

  if (leftStepsToMove != 0)
  {
    leftInterruptIntervalInMicroSeconds = (long)((timeToMoveInSeconds / (double)abs(leftStepsToMove)) * 1000000L + 0.5);
  }
  else
  {
    leftInterruptIntervalInMicroSeconds = minInterruptIntervalInMicroSecs;
  }

  long rightInterruptIntervalInMicroseconds;

  if (rightStepsToMove != 0)
  {
    rightInterruptIntervalInMicroseconds = (long)((timeToMoveInSeconds / (double)abs(rightStepsToMove)) * 1000000L + 0.5);
  }
  else
  {
    rightInterruptIntervalInMicroseconds = minInterruptIntervalInMicroSecs;
  }

#ifdef DEBUG_TIMED_MOVE
  Serial.print("    Left interval in microseconds: ");
  Serial.print(leftInterruptIntervalInMicroSeconds);
  Serial.print(" Right interval in microseconds: ");
  Serial.println(rightInterruptIntervalInMicroseconds);
#endif

  // There's a minium gap allowed between intervals. This is set by the top speed of the motors
  // Presently set at 1000 microseconds

  if (leftInterruptIntervalInMicroSeconds < minInterruptIntervalInMicroSecs & rightInterruptIntervalInMicroseconds < minInterruptIntervalInMicroSecs)
  {
    return Left_And_Right_Distance_Too_Large;
  }

  if (leftInterruptIntervalInMicroSeconds<minInterruptIntervalInMicroSecs & rightInterruptIntervalInMicroseconds> minInterruptIntervalInMicroSecs)
  {
    return Left_Distance_Too_Large;
  }

  if (rightInterruptIntervalInMicroseconds<minInterruptIntervalInMicroSecs & leftInterruptIntervalInMicroSeconds> minInterruptIntervalInMicroSecs)
  {
    return Right_Distance_Too_Large;
  }

  // If we get here we can move at this speed.

  startMotorsCore1(abs(leftStepsToMove), abs(rightStepsToMove),
              leftInterruptIntervalInMicroSeconds, rightInterruptIntervalInMicroseconds,
              leftStepsToMove > 0, rightStepsToMove > 0);

  return Move_OK;
}

void updateMotorsCore1()
{
  if (!motorsRunning)
  {
    return;
  }

  unsigned long currentMicros = micros();

  if (leftMotorWaveformDelta != 0)
  {
    // left is moving - see if it is time to do a step
    leftTimeSinceLastStep = ulongDiff(currentMicros, leftTimeOfLastStep);
    if (leftTimeSinceLastStep >= leftIntervalBetweenSteps)
    {
      if (++leftStepCounter > leftNumberOfStepsToMove)
      {
        leftMotorWaveformDelta = 0;
        setLeft(0);
      }
      else
      {
        // Move the motor one step
        setLeft(motorWaveformLookup[leftMotorWaveformPos]);

        // Update and wrap the waveform position
        leftMotorWaveformPos += leftMotorWaveformDelta;
        if (leftMotorWaveformPos == 8)
          leftMotorWaveformPos = 0;
        if (leftMotorWaveformPos < 0)
          leftMotorWaveformPos = 7;
      }
      leftTimeOfLastStep = currentMicros - (leftTimeSinceLastStep - leftIntervalBetweenSteps);
      leftTimeOfNextStep = currentMicros + leftIntervalBetweenSteps;
    }
  }
  if (rightMotorWaveformDelta != 0)
  {
    rightTimeSinceLastStep = ulongDiff(currentMicros, rightTimeOfLastStep);

    if (rightTimeSinceLastStep >= rightIntervalBetweenSteps)
    {
      if (++rightStepCounter > rightNumberOfStepsToMove)
      {
        rightMotorWaveformDelta = 0;
        setRight(0);
      }
      else
      {
        // Move the motor one step
        setRight(motorWaveformLookup[rightMotorWaveformPos]);

        // Update and wrap the waveform position
        rightMotorWaveformPos += rightMotorWaveformDelta;
        if (rightMotorWaveformPos == 8)
          rightMotorWaveformPos = 0;
        if (rightMotorWaveformPos < 0)
          rightMotorWaveformPos = 7;
      }
      rightTimeOfLastStep = currentMicros - (rightTimeSinceLastStep - rightIntervalBetweenSteps);
      rightTimeOfNextStep = currentMicros + rightIntervalBetweenSteps;
    }
  }
}

#endif

float fastMoveSteps(long leftStepsToMove, long rightStepsToMove)
{

#ifdef DEBUG_FAST_MOVE_STEPS
  Serial.println("fastMoveSteps");
  Serial.print("    Left steps to move: ");
  Serial.print(leftStepsToMove);
  Serial.print(" Right steps to move: ");
  Serial.println(rightStepsToMove);
#endif

  // work out how long it will take to move in seconds

  float timeForLeftMoveInSeconds;

  if (leftStepsToMove != 0)
  {
    timeForLeftMoveInSeconds = ((float)abs(leftStepsToMove) * (float)minInterruptIntervalInMicroSecs) / 1000000.0;
  }
  else
  {
    timeForLeftMoveInSeconds = 0;
  }

  float timeForRightMoveInSeconds;

  if (rightStepsToMove != 0)
  {
    timeForRightMoveInSeconds = ((float)abs(rightStepsToMove) * (float)minInterruptIntervalInMicroSecs) / 1000000.0;
  }
  else
  {
    timeForRightMoveInSeconds = 0;
  }

#ifdef DEBUG_FAST_MOVE_STEPS
  Serial.print("    Left time to move: ");
  Serial.print(timeForLeftMoveInSeconds);
  Serial.print(" Right time to move: ");
  Serial.println(timeForRightMoveInSeconds);
#endif

  // Allow time for the slowest mover
  if (timeForLeftMoveInSeconds > timeForRightMoveInSeconds)
  {
    timedMoveSteps(leftStepsToMove, rightStepsToMove, timeForLeftMoveInSeconds);
    return timeForLeftMoveInSeconds;
  }
  else
  {
    timedMoveSteps(leftStepsToMove, rightStepsToMove, timeForRightMoveInSeconds);
    return timeForRightMoveInSeconds;
  }
}

int timedMoveDistanceInMM(float leftMMs, float rightMMs, float timeToMoveInSeconds)
{

#ifdef TIMED_MOVE_MM_DEBUG
  Serial.println("timedMoveDistanceInMM");
  Serial.print("    Left mms to move: ");
  Serial.print(leftMMs);
  Serial.print(" Right mms to move: ");
  Serial.print(rightMMs);
  Serial.print(" Time to move in seconds: ");
  Serial.println(timeToMoveInSeconds);
#endif

  // Do this because rounding errors can lead to different numbers for
  // positive and negative moves of the same length

  long leftSteps = (long)((abs(leftMMs) * leftStepsPerMM) + 0.5);
  long rightSteps = (long)((abs(rightMMs) * rightStepsPerMM) + 0.5);
  if (leftMMs < 0)
  {
    leftSteps *= -1;
  }

  if (rightMMs < 0)
  {
    rightSteps *= -1;
  }

#ifdef TIMED_MOVE_MM_DEBUG
  Serial.print("    Left steps to move: ");
  Serial.print(leftSteps);
  Serial.print(" Right steps to move: ");
  Serial.println(rightSteps);
#endif

  return timedMoveSteps(leftSteps, rightSteps, timeToMoveInSeconds);
}

int fastMoveDistanceInMM(float leftMMs, float rightMMs)
{

#ifdef FAST_MOVE_MM_DEBUG
  Serial.println("fastMoveDistanceInMM");
  Serial.print("    Left mms to move: ");
  Serial.print(leftMMs);
  Serial.print(" Right mms to move: ");
  Serial.println(rightMMs);
  Serial.print("    Left mms per step: ");
  Serial.print(leftStepsPerMM);
  Serial.print(" Right mms per step: ");
  Serial.println(rightStepsPerMM);

#endif

  long leftSteps = (long)((abs(leftMMs) * leftStepsPerMM) + 0.5);
  long rightSteps = (long)((abs(rightMMs) * rightStepsPerMM) + 0.5);

  if (leftMMs < 0)
  {
    leftSteps *= -1;
  }

  if (rightMMs < 0)
  {
    rightSteps *= -1;
  }

#ifdef FAST_MOVE_MM_DEBUG
  Serial.print("    Left steps to move: ");
  Serial.print(leftSteps);
  Serial.print(" Right steps to move: ");
  Serial.println(rightSteps);
#endif

  int result = fastMoveSteps(leftSteps, rightSteps);

  return result;
}

void rightStop()
{
  setRight(0);
  rightMotorWaveformDelta = 0;
  rightStepCounter = 0;
}

void leftStop()
{
  setLeft(0);
  leftMotorWaveformDelta = 0;
  leftStepCounter = 0;
}

void motorStop()
{
  leftStop();
  rightStop();
}

bool motorsMoving()
{
  if (rightMotorWaveformDelta != 0)
    return true;
  if (leftMotorWaveformDelta != 0)
    return true;
  return false;
}

void waitForMotorsStop()
{
  while (motorsMoving())
    delay(1);
}

// #define DEBUG_FAST_ROTATE

void fastRotateRobot(float angle)
{
  float noOfTurns = angle / 360.0;
  float distanceToRotate = noOfTurns * turningCircle;

  fastMoveDistanceInMM(distanceToRotate, -distanceToRotate);

#ifdef DEBUG_FAST_ROTATE
  Serial.print(". angle: ");
  Serial.print(angle);
  Serial.print(" noOfTurns: ");
  Serial.print(noOfTurns);
  Serial.print(" distanceToRotate: ");
  Serial.println(distanceToRotate);
#endif
}

int timedRotateRobot(float angle, float timeToMoveInSeconds)
{
  float noOfTurns = angle / 360.0;
  float distanceToRotate = noOfTurns * turningCircle;

#ifdef DEBUG_TIMED_ROTATE
  Serial.print(". angle: ");
  Serial.print(angle);
  Serial.print(" noOfTurns: ");
  Serial.print(noOfTurns);
  Serial.print(" time: ");
  Serial.print(timeToMoveInSeconds);
  Serial.print(" distanceToRotate: ");
  Serial.println(distanceToRotate);
#endif

  return timedMoveDistanceInMM(distanceToRotate, -distanceToRotate, timeToMoveInSeconds);
}

void fastMoveArcRobot(float radius, float angle)
{
  float noOfTurns = angle / 360.0;
  float absRadius = abs(radius);

  float leftDistanceToMove = noOfTurns * ((absRadius + (motorSettings.wheelSpacing / 2.0)) * 2.0 * PI);
  float rightDistanceToMove = noOfTurns * ((absRadius - (motorSettings.wheelSpacing / 2.0)) * 2.0 * PI);

#ifdef DEBUG_FAST_ARC
  Serial.println("fastMoveArcRobot");
  Serial.print(" radius: ");
  Serial.print(radius);
  Serial.print(" angle: ");
  Serial.print(angle);
  Serial.print(" leftDistanceToMove: ");
  Serial.print(leftDistanceToMove);
  Serial.print(" rightDistanceToMove: ");
  Serial.println(rightDistanceToMove);
#endif

  if (radius >= 0)
  {
    fastMoveDistanceInMM(leftDistanceToMove, rightDistanceToMove);
  }
  else
  {
    fastMoveDistanceInMM(rightDistanceToMove, leftDistanceToMove);
  }
}

int timedMoveArcRobot(float radius, float angle, float timeToMoveInSeconds)
{
  float noOfTurns = angle / 360.0;
  float absRadius = abs(radius);

  float leftDistanceToMove = noOfTurns * ((absRadius + (motorSettings.wheelSpacing / 2.0)) * 2.0 * PI);
  float rightDistanceToMove = noOfTurns * ((absRadius - (motorSettings.wheelSpacing / 2.0)) * 2.0 * PI);

#ifdef DEBUG_TIMED_ARC
  Serial.println("timedMoveArcRobot");
  Serial.print(" radius: ");
  Serial.print(radius);
  Serial.print(" angle: ");
  Serial.print(angle);
  Serial.print(" time: ");
  Serial.print(timeToMoveInSeconds);
  Serial.print(" leftDistanceToMove: ");
  Serial.print(leftDistanceToMove);
  Serial.print(" rightDistanceToMove: ");
  Serial.println(rightDistanceToMove);
#endif

  if (radius >= 0)
  {
    return timedMoveDistanceInMM(leftDistanceToMove, rightDistanceToMove, timeToMoveInSeconds);
  }
  else
  {
    return timedMoveDistanceInMM(rightDistanceToMove, leftDistanceToMove, timeToMoveInSeconds);
  }
}

void initMotors()
{
  if (motorSettings.motorsActive)
  {
    setupWheelSettings();

    pinMode(motorSettings.lpin1, OUTPUT);
    pinMode(motorSettings.lpin2, OUTPUT);
    pinMode(motorSettings.lpin3, OUTPUT);
    pinMode(motorSettings.lpin4, OUTPUT);

    pinMode(motorSettings.rpin1, OUTPUT);
    pinMode(motorSettings.rpin2, OUTPUT);
    pinMode(motorSettings.rpin3, OUTPUT);
    pinMode(motorSettings.rpin4, OUTPUT);
}
  else
  {
    motorProcessDescriptor.status = MOTORS_OFF;
  }
}

void startMotors()
{
  if (motorSettings.motorsActive)
  {
//    add_repeating_timer_us(50000, onLeft, NULL, &leftTimer);
//    add_repeating_timer_us(50000, onRight, NULL, &rightTimer);
    motorProcessDescriptor.status = MOTORS_OK;
  motorsRunning = true;
  }
}

void updateMotors()
{
//  updateMotorsCore1();
}

void stopMotors()
{
  if (motorProcessDescriptor.status == MOTORS_OK){
    motorStop();
  }

  motorsRunning=false;
//  cancel_repeating_timer(&leftTimer);
//  cancel_repeating_timer(&rightTimer);
  motorProcessDescriptor.status = MOTORS_OFF;
}

bool motorStatusOK()
{
  return motorProcessDescriptor.status == MOTORS_OK;
}

void motorStatusMessage(char *buffer, int bufferLength)
{
  switch (motorProcessDescriptor.status)
  {
  case MOTORS_OFF:
    snprintf(buffer, bufferLength, "Motors off");
    break;
  case MOTORS_OK:
    snprintf(buffer, bufferLength, "Motors OK Moving: %d", motorsMoving());
    break;
  default:
    snprintf(buffer, bufferLength, "Motor status invalid");
    break;
  }
}

boolean validateMoveDistance(void *dest, const char *newValueStr)
{
	float value;

	if (!validateFloat(&value, newValueStr))
	{
		return false;
	}

	if (value < -10000 || value > 10000)
	{
		return false;
	}

	*(float *)dest = value;
	return true;
}

boolean validateMoveTime(void *dest, const char *newValueStr)
{
	float value;

	if (!validateFloat(&value, newValueStr))
	{
		return false;
	}

	if (value < 0 || value > 10000)
	{
		return false;
	}

	*(float *)dest = value;
	return true;
}

#define MOTOR_FLOAT_VALUE_OFFSET 0
#define MOTOR_LEFT_DISTANCE_OFFSET (MOTOR_FLOAT_VALUE_OFFSET + sizeof(float))
#define MOTOR_RIGHT_DISTANCE_OFFSET (MOTOR_LEFT_DISTANCE_OFFSET + sizeof(float))
#define MOTOR_MOVE_TIME_OFFSET (MOTOR_RIGHT_DISTANCE_OFFSET + sizeof(float))

struct CommandItem motorLeftCommandItem = {
	"left",
	"left motor move distance mm",
	MOTOR_LEFT_DISTANCE_OFFSET,
	floatCommand,
	validateMoveDistance,
	noDefaultAvailable};

struct CommandItem motorRightCommandItem = {
	"right",
	"right motor move distance mm",
	MOTOR_RIGHT_DISTANCE_OFFSET,
	floatCommand,
	validateMoveDistance,
	noDefaultAvailable};

struct CommandItem motorMoveTimeCommandItem = {
	"time",
	"time for the move to take (secs)",
	MOTOR_MOVE_TIME_OFFSET,
	floatCommand,
	validateMoveTime,
	setDefaultFloatZero};

struct CommandItem *motorMoveItems[] =
	{
		&motorLeftCommandItem,
		&motorRightCommandItem,
		&motorMoveTimeCommandItem
  };

int doMotorLeftRightIntime(char *destination, unsigned char *settingBase);

struct Command motorLeftRightIntimeCommand
{
	"move",
		"Moves the left and right sides a distance in a given time",
		motorMoveItems,
		sizeof(motorMoveItems) / sizeof(struct CommandItem *),
		doMotorLeftRightIntime
};

int doMotorLeftRightIntime(char *destination, unsigned char *settingBase)
{
	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("motors", &motorLeftRightIntimeCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	if(motorProcessDescriptor.status != MOTORS_OK)
	{
		return JSON_MESSAGE_MOTORS_NOT_ENABLED;
	}
	
	float left = (float)getUnalignedFloat(settingBase + MOTOR_LEFT_DISTANCE_OFFSET);
	float right = (float)getUnalignedFloat(settingBase + MOTOR_RIGHT_DISTANCE_OFFSET);
	float time = (float)getUnalignedFloat(settingBase + MOTOR_MOVE_TIME_OFFSET);

  if(time>0){
    if (timedMoveDistanceInMM(left,right,time)==Move_OK){
      return WORKED_OK;
    }
    else{
      return JSON_MESSAGE_INVALID_MOVE_VALUES;
    }
  }
  else{
    if (fastMoveDistanceInMM(left,right)==Move_OK){
      return WORKED_OK;
    }
    else{
      return JSON_MESSAGE_INVALID_MOVE_VALUES;
    }
  }
}

struct Command *motorCommandList[] = {
  &motorLeftRightIntimeCommand
};

struct CommandItemCollection motorCommands =
    {
        "Control the pixels on the device",
        motorCommandList,
        sizeof(motorCommandList) / sizeof(struct Command *)};

struct process motorProcessDescriptor = {
    "motors",
    initMotors,
    startMotors,
    updateMotors,
    stopMotors,
    motorStatusOK,
    motorStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&motorSettings, sizeof(motorSettings), &motorSettingItems,
    &motorCommands,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL};
