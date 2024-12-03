#pragma once

#define RFID_NOT_FITTED -1
#define RFID_CONNECTED 1

#ifdef WEMOSD1MINI
#define SS_PIN D8
#define RST_PIN D2
#define INT_PIN D1
#endif

#ifdef PICO
#define SS_PIN 5
#define RST_PIN 20
#define INT_PIN 21
#define MOSI_PIN 3
#define MISO_PIN 4
#define SCK 2
#endif



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



