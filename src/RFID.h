#pragma once

#define RFID_CONNECTED 3003
#define RFID_OFF 3001
#define RFID_ON 3002

#define RFID_MESSAGE_LENGTH 60
#define RFID_MESSAGE_COMMAND_LENGTH 10

#define RFID_TOPIC "RFID"

#define SS_PIN D8
#define RST_PIN D2
#define INT_PIN D0

struct RFIDSettings
{
    bool RFIDEnabled;
    int millisBetweenUpdates;
};

void sendMessageToRFID(char *messageText);

void RFIDOff();

void RFIDOn();

extern struct RFIDSettings RFIDSettings;

extern struct SettingItemCollection RFIDSettingItems;

extern struct process RFIDProcess;

void pollRFID();
void testRFID();


