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
#define RFIDSENSOR_SEND_ON_CARD1_SCANNED 2
#define RFIDSENSOR_SEND_ON_CARD2_SCANNED 4
#define RFIDSENSOR_SEND_ON_CARD3_SCANNED 8
#define RFIDSENSOR_SEND_ON_CARD4_SCANNED 16
#define RFIDSENSOR_SEND_ON_CARD5_SCANNED 32
#define RFIDSENSOR_SEND_ON_CARD6_SCANNED 64
#define RFIDSENSOR_SEND_ON_CARD7_SCANNED 128
#define RFIDSENSOR_SEND_ON_CARD8_SCANNED 256

#define RFID_MESSAGE_BUFFER_SIZE 80
#define RFID_MESSAGE_TOPIC "rfid"

#define RFID_KEY_LENGTH 15

struct RFIDSensorSettings
{
    bool RFIDFitted;
    bool DrinkMonitorActive;
    char DrinkResetKey[RFID_KEY_LENGTH];
    char Card1Key[RFID_KEY_LENGTH];
    char Card2Key[RFID_KEY_LENGTH];
    char Card3Key[RFID_KEY_LENGTH];
    char Card4Key[RFID_KEY_LENGTH];
    char Card5Key[RFID_KEY_LENGTH];
    char Card6Key[RFID_KEY_LENGTH];
    char Card7Key[RFID_KEY_LENGTH];
    char Card8Key[RFID_KEY_LENGTH];
    bool RFIDmqttAlertActive;
};

struct RFIDCardKeySetting
{
	char * id;
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



