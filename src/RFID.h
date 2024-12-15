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

#define DRINK_RESET_KEY_LENGTH 15

struct RFIDSensorSettings
{
    bool RFIDFitted;
    bool DrinkMonitorActive;
    char DrinkResetKey[DRINK_RESET_KEY_LENGTH];
    bool RFIDmqttAlertActive;
};

#define RFID_LENGTH 20

struct RFIDSensorReading {
    int counter;
    char idString[RFID_LENGTH];
};

void pollRFID();
void testRFID();

void doRFIDSetupDrinksResetCard(char * command);

extern struct RFIDSensorSettings RFIDSensorSettings;

extern struct SettingItemCollection RFIDSettingItems;

extern struct sensor RFIDSensor;



