#pragma once

#define RFID_NOT_FITTED -1
#define RFID_CONNECTED 1

#define SS_PIN D8
#define RST_PIN D2
#define INT_PIN D1

#define RFIDSENSOR_SEND_ON_CARD_SCANNED 1
#define RFID_MESSAGE_BUFFER_SIZE 80
#define RFID_MESSAGE_TOPIC "rfid"

struct RFIDSensorSettings
{
    bool RFIDFitted;
    int millisBetweenUpdates;
};

#define RFID_LENGTH 20

struct RFIDSensorReading {
    int counter;
    char idString[RFID_LENGTH];
};

void pollRFID();
void testRFID();

extern struct RFIDSensorSettings RFIDSensorSettings;

extern struct SettingItemCollection RFIDSettingItems;

extern struct sensor RFIDSensor;



