#include <Arduino.h>

#include "utils.h"
#include "settings.h"
#include "robotProcess.h"
#include "processes.h"
#include "mqtt.h"
#include "errors.h"
#include "clock.h"
#include "console.h"
#include "statusled.h"
#include "console.h"

#define ROBOT_MESSAGE_BUFFER_SIZE 256

struct robotSettings robotSettings;

void setDefaultrobotDataPinNo(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 16;
}

struct SettingItem robotDataPinNo = {
    "robot Data Pin",
    "robotdatapin",
    &robotSettings.dataPin,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultrobotDataPinNo,
    validateInt};

void setDefaultrobotBaudRate(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 1200;
}

struct SettingItem robotBaudRate = {
    "Robot baud rate",
    "robotbaud",
    &robotSettings.robotBaudRate,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultrobotBaudRate,
    validateInt};

struct SettingItem robotEnabledSetting = {
    "Robot enabled",
    "roboton",
    &robotSettings.robotEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

struct SettingItem robotSwapSerialSetting = {
    "Robot swap serial",
    "robotswapserial",
    &robotSettings.robotSwapSerial,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

struct SettingItem *robotSettingItemPointers[] =
    {
        &robotEnabledSetting,
        &robotBaudRate,
        &robotDataPinNo,
        &robotSwapSerialSetting};

struct SettingItemCollection robotMessagesSettingItems = {
    "robotSettings",
    "robot setup",
    robotSettingItemPointers,
    sizeof(robotSettingItemPointers) / sizeof(struct SettingItem *)};

void robotMessagesOff()
{
    robotSettings.robotEnabled = false;
    saveSettings();
}

void robotMessagesOn()
{
    robotSettings.robotEnabled = true;
    saveSettings();
}

void sendRobotMessageToServer(char *messageText)
{
    char messageBuffer[ROBOT_MESSAGE_BUFFER_SIZE];
    char deviceNameBuffer[DEVICE_NAME_LENGTH];

    PrintSystemDetails(deviceNameBuffer, DEVICE_NAME_LENGTH);

    snprintf(messageBuffer, ROBOT_MESSAGE_BUFFER_SIZE,
             "{\"name\":\"%s\",\"from\":\"robot\",\"message\":\"%s\"}",
             deviceNameBuffer,
             messageText);

    publishBufferToMQTTTopic(messageBuffer, ROBOT_TOPIC);
}

void sendMessageToRobot(char *messageText)
{
    statusLedToggle();
    Serial.print((char)0x0d);
    Serial.print(messageText);
    Serial.print((char)0x0d);
    Serial.print((char)0x0a);
}

#define robot_FLOAT_VALUE_OFFSET 0
#define robot_MESSAGE_OFFSET (robot_FLOAT_VALUE_OFFSET + sizeof(float))

boolean validaterobotOptionString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, ROBOT_MESSAGE_COMMAND_LENGTH));
}

boolean validaterobotMessageString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, ROBOT_MESSAGE_LENGTH));
}

struct CommandItem robotMessageText = {
    "text",
    "robot message text",
    robot_MESSAGE_OFFSET,
    textCommand,
    validaterobotMessageString,
    noDefaultAvailable};

struct CommandItem *RobotCommandItems[] =
    {
        &robotMessageText};

int doSendRobotMessage(char *destination, unsigned char *settingBase);

struct Command robotMessageCommand
{
    "Send",
        "Sends a message to the robot",
        RobotCommandItems,
        sizeof(RobotCommandItems) / sizeof(struct CommandItem *),
        doSendRobotMessage
};

int doSendRobotMessage(char *destination, unsigned char *settingBase)
{
    if (*destination != 0)
    {
        // we have a destination for the command. Build the string
        char buffer[JSON_BUFFER_SIZE];
        createJSONfromSettings("robot", &robotMessageCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
        return publishCommandToRemoteDevice(buffer, destination);
    }

    if (robotProcess.status != ROBOT_CONNECTED)
    {
        return JSON_MESSAGE_ROBOT_NOT_ENABLED;
    }

    char *message = (char *)(settingBase + robot_MESSAGE_OFFSET);

    sendMessageToRobot(message);

    TRACELOG("Sending: ");
    TRACELOGLN(buffer);

    return WORKED_OK;
}

struct Command *robotCommandList[] = {
    &robotMessageCommand};

struct CommandItemCollection robotCommands =
    {
        "Control robot",
        robotCommandList,
        sizeof(robotCommandList) / sizeof(struct Command *)};

unsigned long robotmillisAtLastScroll;

void initRobotProcess()
{
    // all the setup is performed in start
}

#define ROBOT_BUFFER_SIZE 129
#define ROBOT_BUFFER_LIMIT ROBOT_BUFFER_SIZE - 1

char robotReceiveBuffer[ROBOT_BUFFER_SIZE];

int robotReceiveBufferPos = 0;

void reset_robot_buffer()
{
    robotReceiveBufferPos = 0;
}

void startRobotProcess()
{
    reset_robot_buffer();

    if (robotSettings.robotEnabled)
    {
        if (forceConsole)
        {
            alwaysDisplayMessage("Robot not enabled as console has been forced");
            robotProcess.status = ROBOT_OFF;
        }
        else
        {
            robotProcess.status = ROBOT_CONNECTED;
            alwaysDisplayMessage("Enabling robot connection");
            delay(200);
            Serial.end();
            Serial.begin(robotSettings.robotBaudRate);

#if defined(ARDUINO_ARCH_ESP8266)

            if (robotSettings.robotSwapSerial)
            {
                Serial.swap();
            }
#endif
        }
    }
    else
    {
        robotProcess.status = ROBOT_OFF;
    }
}

void processMessageFromRobot()
{
    if (robotReceiveBuffer[0] == '#')
    {
        actOnSerialCommand(robotReceiveBuffer + 1);
    }
    else
    {
        sendRobotMessageToServer(robotReceiveBuffer);
    }
}

void bufferRobotSerialChar(char ch)
{
    if (ch == '\n' || ch == '\r' || ch == 0)
    {
        if (robotReceiveBufferPos > 0)
        {
            robotReceiveBuffer[robotReceiveBufferPos] = 0;
            processMessageFromRobot();
            reset_robot_buffer();
        }
        return;
    }

    if (robotReceiveBufferPos < ROBOT_BUFFER_SIZE)
    {
        robotReceiveBuffer[robotReceiveBufferPos] = ch;
        robotReceiveBufferPos++;
    }
}

void checkRobotBuffer()
{
    while (Serial.available())
    {
        bufferRobotSerialChar(Serial.read());
    }
}

void updateRobotProcess()
{
    if (robotProcess.status != ROBOT_CONNECTED)
    {
        return;
    }
    checkRobotBuffer();
}

void stopRobotProcess()
{
    robotProcess.status = ROBOT_OFF;
}

bool robotStatusOK()
{
    return robotProcess.status == ROBOT_CONNECTED;
}

void robotStatusMessage(char *buffer, int bufferLength)
{
    if (robotProcess.status == ROBOT_OFF)
    {
        snprintf(buffer, bufferLength, "robot off");
    }
    else
    {
        snprintf(buffer, bufferLength, "robot on");
    }
}

struct process robotProcess = {
    "robot",
    initRobotProcess,
    startRobotProcess,
    updateRobotProcess,
    stopRobotProcess,
    robotStatusOK,
    robotStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&robotSettings, sizeof(robotSettings), &robotMessagesSettingItems,
    &robotCommands,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL,
    NULL, // no command options
    0     // no command options
};
