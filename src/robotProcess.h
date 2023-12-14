#pragma once

#define ROBOT_CONNECTED 2000
#define ROBOT_OFF 2001

#define ROBOT_MESSAGE_LENGTH 60
#define ROBOT_MESSAGE_COMMAND_LENGTH 10

#define ROBOT_TOPIC "robot"


struct robotSettings
{
    bool robotEnabled;
    int robotBaudRate;
    int dataPin;
    bool robotSwapSerial;
};

void sendMessageToRobot(char *messageText);

void robotOff();

void robotOn();

extern struct robotSettings robotSettings;

extern struct SettingItemCollection robotSettingItems;

extern struct process robotProcess;
