#include <Arduino.h>

#include "utils.h"
#include "settings.h"
#include "RFID.h"
#include "processes.h"
#include "mqtt.h"
#include "errors.h"
#include "clock.h"
#include "console.h"
#include "connectwifi.h"
#include "statusled.h"
#include "settings.h"
#include "console.h"
#include "controller.h"
#include <SPI.h>
#include "MFRC522.h"

#if defined(ARDUINO_ARCH_ESP8266)
#include "LittleFS.h"
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include "FS.h"
#endif

#include "RFID.h"

struct RFIDSensorSettings RFIDSensorSettings;

struct SettingItem RFIDFittedSetting = {
    "RFID fitted",
    "rfidfitted",
    &RFIDSensorSettings.RFIDFitted,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

struct SettingItem RFIDdrinkMonitorSetting = {
    "Drink monitoring active",
    "rfiddrinkmonitor",
    &RFIDSensorSettings.DrinkMonitorActive,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

struct SettingItem RFIDmqttActiveSetting = {
    "Send MQTT messages from RFID",
    "rfidmqttmessages",
    &RFIDSensorSettings.RFIDmqttAlertActive,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

boolean validateRFIDCardID(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, RFID_KEY_LENGTH));
}

void setDefaultRFIDKey(void *dest)
{
    snprintf((char *)dest, RFID_KEY_LENGTH, "set card key");
}

struct SettingItem RFIDdrinkResetKey = {
    "RFID drink reset key",
    "rfiddrinkresetkey",
    RFIDSensorSettings.DrinkResetKey,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};

struct SettingItem RFIDCard1Key = {
    "RFID card 1 key",
    "rfidcard1key",
    RFIDSensorSettings.Card1Key,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};

struct SettingItem RFIDCard2Key = {
    "RFID card 2 key",
    "rfidcard2key",
    RFIDSensorSettings.Card2Key,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};

struct SettingItem RFIDCard3Key = {
    "RFID card 3 key",
    "rfidcard3key",
    RFIDSensorSettings.Card3Key,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};

struct SettingItem RFIDCard4Key = {
    "RFID card 4 key",
    "rfidcard4key",
    RFIDSensorSettings.Card4Key,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};

struct SettingItem RFIDCard5Key = {
    "RFID card 5 key",
    "rfidcard5key",
    RFIDSensorSettings.Card5Key,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};

struct SettingItem RFIDCard6Key = {
    "RFID card 6 key",
    "rfidcard6key",
    RFIDSensorSettings.Card6Key,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};

struct SettingItem RFIDCard7Key = {
    "RFID card 7 key",
    "rfidcard7key",
    RFIDSensorSettings.Card7Key,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};

struct SettingItem RFIDCard8Key = {
    "RFID card 8 key",
    "rfidcard8key",
    RFIDSensorSettings.Card8Key,
    RFID_KEY_LENGTH,
    text,
    setDefaultRFIDKey,
    validateRFIDCardID};
 
struct SettingItem *RFIDSettingItemPointers[] =
    {
        &RFIDFittedSetting,
        &RFIDdrinkMonitorSetting,
        &RFIDdrinkResetKey,
        &RFIDmqttActiveSetting,
        &RFIDCard1Key,
        &RFIDCard2Key,
        &RFIDCard3Key,
        &RFIDCard4Key,
        &RFIDCard5Key,
        &RFIDCard6Key,
        &RFIDCard7Key,
        &RFIDCard8Key
        };

struct SettingItemCollection RFIDSensorSettingItems = {
    "RFIDSettings",
    "RFID setup",
    RFIDSettingItemPointers,
    sizeof(RFIDSettingItemPointers) / sizeof(struct SettingItem *)};

struct sensorEventBinder RFIDSensorListenerFunctions[] = {
    {"card", RFIDSENSOR_SEND_ON_CARD_SCANNED},
    {"card1", RFIDSENSOR_SEND_ON_CARD1_SCANNED},
    {"card2", RFIDSENSOR_SEND_ON_CARD2_SCANNED},
    {"card3", RFIDSENSOR_SEND_ON_CARD3_SCANNED},
    {"card4", RFIDSENSOR_SEND_ON_CARD4_SCANNED},
    {"card5", RFIDSENSOR_SEND_ON_CARD5_SCANNED},
    {"card6", RFIDSENSOR_SEND_ON_CARD6_SCANNED},
    {"card7", RFIDSENSOR_SEND_ON_CARD7_SCANNED},
    {"card8", RFIDSENSOR_SEND_ON_CARD8_SCANNED}
    };

MFRC522 *mfrc522 = NULL; // Create MFRC522 reference

void pollRFID()
{
    Serial.println("Polling RFID. Press ESC to exit");

    while (true)
    {

        if (Serial.available() != 0)
        {
            int ch = Serial.read();
            if (ch == ESC_KEY)
            {
                Serial.println("\nRFID test ended");
                break;
            }
        }

        delay(100);

        // Look for new cards
        if (!mfrc522->PICC_IsNewCardPresent())
        {
            continue;
        }

        // Select one of the cards
        if (!mfrc522->PICC_ReadCardSerial())
        {
            continue;
        }

        // Print card UID
        Serial.print("Card UID:");
        for (byte i = 0; i < mfrc522->uid.size; i++)
        {
            Serial.print(mfrc522->uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(mfrc522->uid.uidByte[i], HEX);
        }
        Serial.println();

        // Halt PICC
        mfrc522->PICC_HaltA();
    }
}

void testRFID()
{

    Serial.println("Testing RFID");

    SPI.begin();         // Init SPI bus
    mfrc522->PCD_Init(); // Init MFRC522

    pollRFID();
}

byte regVal = 0x7F;
volatile bool bNewInt = false;
volatile uint8_t reg;

// if the processor is not an ESP device the IRAM_ATTR symbol is defined as empty in utils.h

#if defined(PICO)
// maximum size is 10 according to the MFRC522 spec - so this should be fine

#define UID_BUFFER_LENGTH 20

byte uidLength;

byte uidReceivedBuffer[UID_BUFFER_LENGTH];

void IRAM_ATTR readCard()
{
    if (!bNewInt)
    {
        if (mfrc522->PICC_ReadCardSerial())
        {
            // ignore repeated interrupts if they have not been handled
            bNewInt = true;
            uidLength = mfrc522->uid.size;
            memcpy(uidReceivedBuffer, mfrc522->uid.uidByte, uidLength);
            mfrc522->PICC_HaltA();
        }
    }
    // enable interrupts
    mfrc522->PCD_WriteRegister(mfrc522->ComIrqReg, 0x7F);
}
#endif

#if defined(WEMOSD1MINI)

void IRAM_ATTR readCard()
{
    bNewInt = true;
    reg = mfrc522->PCD_ReadRegister(MFRC522::Status1Reg);
}
#endif

void activateRec()
{
    // Clear FIFO buffer
    mfrc522->PCD_WriteRegister(MFRC522::FIFOLevelReg, 0x80);

    // Load the REQA command into the FIFO buffer
    mfrc522->PCD_WriteRegister(MFRC522::FIFODataReg, MFRC522::PICC_CMD_REQA);

    // Set the MFRC522 to transceive mode to send the command and expect a response
    mfrc522->PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Transceive);

    // Start the transmission of data (REQA command) and set the bit framing
    mfrc522->PCD_WriteRegister(MFRC522::BitFramingReg, 0x87);
}

void oactivateRec()
{
    mfrc522->PCD_WriteRegister(mfrc522->FIFODataReg, mfrc522->PICC_CMD_REQA);
    mfrc522->PCD_WriteRegister(mfrc522->CommandReg, mfrc522->PCD_Transceive);
    mfrc522->PCD_WriteRegister(mfrc522->BitFramingReg, 0x87);
}

uint8_t oldError = 0;

void updateRec()
{
    activateRec();
}

void clearInt()
{
    mfrc522->PCD_WriteRegister(mfrc522->ComIrqReg, 0x7F);
}

void consumeRFIDJsonResult(char *resultText)
{
    // remove the comment to see what the result is - otherwise ignore
    // Serial.printf("     %s\n", resultText);
}

bool settingDrinksResetCard = false;

void doRFIDSetupDrinksResetCard(char * command)
{
    alwaysDisplayMessage("\nSetting up drinks clear card\n");
    if(!RFIDSensorSettings.DrinkMonitorActive){
        alwaysDisplayMessage("\nTurn on drink monitoring (rfiddrinkmonitor=yes) before using this command\n");
        return;
    }

    act_onJson_message("{\"process\":\"pixels\",\"command\":\"setnamedcolour\",\"colourname\":\"white\"}", consumeRFIDJsonResult);
    settingDrinksResetCard=true;
}

#define CARD_ID_LENGTH 9
#define NO_OF_CARDS 200

char seenCards[200][9];

#define RFID_LIGHT_TIMEOUT_MILLIS 1000

unsigned long rfidLightStart = 0;

void clearCards()
{
    Serial.println("Clearing cards\n");

    for (int cardNo = 0; cardNo < NO_OF_CARDS; cardNo++)
    {
        seenCards[cardNo][0] = 0;
    }
}

void storeID(char *id)
{
    Serial.printf("Storing id: %s\n", id);
    for (int cardNo = 0; cardNo < NO_OF_CARDS; cardNo++)
    {
        if (seenCards[cardNo][0] == 0)
        {
            strcpy(seenCards[cardNo], id);
            return;
        }
    }
    Serial.println("No room to store card\n");
}

bool seenCardBefore(char *id)
{
    Serial.printf("Checking id: %s\n", id);
    for (int cardNo = 0; cardNo < NO_OF_CARDS; cardNo++)
    {
        if (seenCards[cardNo][0] == 0)
        {
            break;
        }
        //        Serial.printf("    Testing: %s\n", seenCards[cardNo]);
        if (strcasecmp(id, seenCards[cardNo]) == 0)
        {
            return true;
        }
    }
    return false;
}

void checkRFIDCard(char *id)
{
    if (RFIDSensorSettings.RFIDmqttAlertActive)
    {
        if (MQTTProcessDescriptor.status == MQTT_OK)
        {
            char messageBuffer[RFID_MESSAGE_BUFFER_SIZE];
            char deviceNameBuffer[DEVICE_NAME_LENGTH];
            PrintSystemDetails(deviceNameBuffer, DEVICE_NAME_LENGTH);

            // send a message to indicate we have a card
            snprintf(messageBuffer, RFID_MESSAGE_BUFFER_SIZE,
                     "{\"device\":\"%s\",\"cardID\":\"%s\"}",
                     deviceNameBuffer,
                     id);

            Serial.printf("Sending rfid: %s\n", messageBuffer);

            publishBufferToMQTTTopic(messageBuffer, RFID_MESSAGE_TOPIC);

            rfidLightStart = millis();
        }
    }

    if (RFIDSensorSettings.DrinkMonitorActive)
    {
        // keep a counter array

        if(settingDrinksResetCard){
            strcpy(RFIDSensorSettings.DrinkResetKey,id);
            saveSettings();
            act_onJson_message("{\"process\":\"pixels\",\"command\":\"setnamedcolour\",\"colourname\":\"blue\"}", consumeRFIDJsonResult);
            alwaysDisplayMessage("\nDrink reset card set\n");
            settingDrinksResetCard=false;
            return;
        }

        rfidLightStart = millis();

        if (strcasecmp(RFIDSensorSettings.DrinkResetKey, id) == 0)
        {
            act_onJson_message("{\"process\":\"pixels\",\"command\":\"setnamedcolour\",\"colourname\":\"white\"}", consumeRFIDJsonResult);
            clearCards();
            return;
        }

        if (seenCardBefore(id))
        {
            act_onJson_message("{\"process\":\"pixels\",\"command\":\"setnamedcolour\",\"colourname\":\"red\"}", consumeRFIDJsonResult);
        }
        else
        {
            act_onJson_message("{\"process\":\"pixels\",\"command\":\"setnamedcolour\",\"colourname\":\"green\"}", consumeRFIDJsonResult);
            storeID(id);
        }
    }
}

void updateRFIDLight()
{
    if (RFIDSensorSettings.DrinkMonitorActive)
    {
        if (rfidLightStart != 0)
        {
            if (ulongDiff(millis(), rfidLightStart) > RFID_LIGHT_TIMEOUT_MILLIS)
            {
                act_onJson_message("{\"process\":\"pixels\",\"command\":\"pattern\",\"pattern\":\"walking\",\"colourmask\":\"RGBY\"}", consumeRFIDJsonResult);
                rfidLightStart = 0;
            }
        }
    }
}

void startRFIDSensor()
{
    settingDrinksResetCard=false;

    clearCards();

    if (RFIDSensor.activeReading == NULL)
    {
        RFIDSensor.activeReading = new RFIDSensorReading();
    }

    if (RFIDSensorSettings.RFIDFitted)
    {
#if defined(PICO)
        SPI.setMISO(MISO_PIN);
        SPI.setMOSI(MOSI_PIN);
        SPI.setSCK(SCK);
#endif
        SPI.begin(); // Init SPI bus

        if (mfrc522 == NULL)
        {
            mfrc522 = new MFRC522(SS_PIN, RST_PIN);
            mfrc522->PCD_Init(); // Init MFRC522

            /* setup the IRQ pin*/
            pinMode(INT_PIN, INPUT_PULLUP);
            /*
             * Allow the ... irq to be propagated to the IRQ pin
             * For test purposes propagate the IdleIrq and loAlert
             */
            regVal = 0xA0; // rx irq
            mfrc522->PCD_WriteRegister(mfrc522->ComIEnReg, regVal);

            bNewInt = false; // interrupt flag

            /*Activate the interrupt*/
            attachInterrupt(digitalPinToInterrupt(INT_PIN), readCard, FALLING);

            activateRec();
        }

        RFIDSensor.status = RFID_CONNECTED;
    }
    else
    {
        RFIDSensor.status = RFID_NOT_FITTED;
    }
}


void RFIDsendToListener(struct RFIDSensorReading *RFIDSensoractiveReading, sensorListener *pos){
    // if the command has a value element we now need to take the element value and put
    // it into the command data for the message that is about to be received.
    // The command data value is always the first item in the parameter block

    char *resultValue = RFIDSensoractiveReading->idString;

    char *messageBuffer = (char *)pos->config->optionBuffer + MESSAGE_START_POSITION;
    snprintf(messageBuffer, MAX_MESSAGE_LENGTH, "%s", resultValue);

    pos->receiveMessage(pos->config->destination, pos->config->optionBuffer);
    pos->lastReadingMillis = RFIDSensor.millisAtLastReading;
}

void sendRFIDtagToListeners()
{
    struct RFIDSensorReading *RFIDSensoractiveReading =
                (struct RFIDSensorReading *)RFIDSensor.activeReading;

    sensorListener *pos = RFIDSensor.listeners;

    while (pos != NULL)
    {
        switch(pos->config->sendOption){

        case RFIDSENSOR_SEND_ON_CARD_SCANNED:
            RFIDsendToListener(RFIDSensoractiveReading,pos);
            break;

        case RFIDSENSOR_SEND_ON_CARD1_SCANNED:
            Serial.printf("**** Comparing reading %s with stored %s\n",RFIDSensoractiveReading->idString,RFIDSensorSettings.Card1Key );
            if (strcasecmp(RFIDSensoractiveReading->idString, RFIDSensorSettings.Card1Key) == 0)
            {
                RFIDsendToListener(RFIDSensoractiveReading,pos);
            }
            break;

        case RFIDSENSOR_SEND_ON_CARD2_SCANNED:
            Serial.printf("**** Comparing reading %s with stored %s\n",RFIDSensoractiveReading->idString,RFIDSensorSettings.Card2Key );
            if (strcasecmp(RFIDSensoractiveReading->idString, RFIDSensorSettings.Card2Key) == 0)
            {
                RFIDsendToListener(RFIDSensoractiveReading,pos);
            }
            break;

        }
        // move on to the next one
        pos = pos->nextMessageListener;
    }
}

#if defined(PICO)

void updateRFIDSensorReading()
{
    if (RFIDSensor.status == RFID_CONNECTED)
    {
        if (bNewInt)
        {
            struct RFIDSensorReading *RFIDSensoractiveReading =
                (struct RFIDSensorReading *)RFIDSensor.activeReading;

            // attempt a read
            // read succeeded - display it

            // Convert UID to String

            String uidString = "";

            for (byte i = 0; i < uidLength; i++)
            {
                if (uidReceivedBuffer[i] < 0x10)
                {
                    uidString += "0";
                }
                uidString += String(mfrc522->uid.uidByte[i], HEX);
            }

            size_t length = uidString.length();

            char uidbuffer[length + 1]; // +1 for null terminator if needed

            for (size_t i = 0; i < length; ++i)
            {
                uidbuffer[i] = static_cast<unsigned char>(uidString[i]);
            }

            uidbuffer[length] = '\0';

            char *dest = RFIDSensoractiveReading->idString;

            snprintf(dest, RFID_LENGTH - 1, "%s", uidbuffer);

            RFIDSensoractiveReading->counter++;

            sensorListener *pos = RFIDSensor.listeners;

            Serial.printf("Got a card:%s\n", uidbuffer);

            checkRFIDCard(uidbuffer);

            RFIDSensor.millisAtLastReading = millis();

            sendRFIDtagToListeners();

            // clear the flag
            bNewInt = false;
        }

        updateRec();

        updateRFIDLight();
    }
}

#endif

#if defined(WEMOSD1MINI)

void updateRFIDSensorReading()
{
    if (RFIDSensor.status == RFID_CONNECTED)
    {
        if (bNewInt)
        {
            struct RFIDSensorReading *RFIDSensoractiveReading =
                (struct RFIDSensorReading *)RFIDSensor.activeReading;

            // Only trigger a read if we have a valid card
            if (1)
            {
                // attempt a read
                if (mfrc522->PICC_ReadCardSerial())
                {
                    // read succeeded - display it

                    // Convert UID to String

                    String uidString = "";

                    for (byte i = 0; i < mfrc522->uid.size; i++)
                    {
                        if (mfrc522->uid.uidByte[i] < 0x10)
                        {
                            uidString += "0";
                        }
                        uidString += String(mfrc522->uid.uidByte[i], HEX);
                    }

                    size_t length = uidString.length();

                    char uidbuffer[length + 1]; // +1 for null terminator if needed

                    for (size_t i = 0; i < length; ++i)
                    {
                        uidbuffer[i] = static_cast<unsigned char>(uidString[i]);
                    }

                    uidbuffer[length] = '\0';

                    char *dest = RFIDSensoractiveReading->idString;

                    snprintf(dest, RFID_LENGTH - 1, "%s", uidbuffer);

                    RFIDSensoractiveReading->counter++;

                    sendRFIDtagToListeners();

                    Serial.printf("Got a card:%s\n", uidbuffer);

                    checkRFIDCard(uidbuffer);

                    RFIDSensor.millisAtLastReading = millis();

                    sendRFIDtagToListeners();
                }
            }

            // clear the interrupt
            clearInt();

            // clear the flag
            bNewInt = false;

            // Halt PICC ready for the next card
            mfrc522->PICC_HaltA();
        }

        updateRec();

        updateRFIDLight();
    }
}

#endif

void stopRFIDSensor()
{
}

bool RFIDStatusOK()
{
    return RFIDSensor.status == RFID_CONNECTED;
}

void RFIDSensorStatusMessage(char *buffer, int bufferLength)
{
    uint8_t Status1Reg;
    uint8_t Status2Reg;

    switch (RFIDSensor.status)
    {
    case RFID_CONNECTED:
        Status1Reg = mfrc522->PCD_ReadRegister(MFRC522::Status1Reg);
        Status2Reg = mfrc522->PCD_ReadRegister(MFRC522::Status2Reg);
        snprintf(buffer, bufferLength, "RFID connected new value flag %d Status: 0x%X 0x%X", bNewInt, Status1Reg, Status2Reg);
        break;
    case RFID_NOT_FITTED:
        snprintf(buffer, bufferLength, "RFID not fitted");
        break;
    }
}

void startRFIDSensorReading()
{
}

void addRFIDSensorReading(char *jsonBuffer, int jsonBufferSize)
{
    struct RFIDSensorReading *RFIDSensoractiveReading =
        (struct RFIDSensorReading *)RFIDSensor.activeReading;

    if (RFIDSensor.status == SENSOR_OK)
    {
        appendFormattedString(jsonBuffer, jsonBufferSize, ",\"rfid\":\"%s\"",
                              RFIDSensoractiveReading->idString);
    }
}

struct sensor RFIDSensor = {
    "RFID",
    0, // millis at last reading
    0, // reading number
    0, // last transmitted reading number
    startRFIDSensor,
    stopRFIDSensor,
    updateRFIDSensorReading,
    startRFIDSensorReading,
    addRFIDSensorReading,
    RFIDSensorStatusMessage,
    -1,    // status
    false, // being updated
    NULL,  // active reading - set in setup
    0,     // active time
    (unsigned char *)&RFIDSensorSettings,
    sizeof(struct RFIDSensorSettings),
    &RFIDSensorSettingItems,
    NULL, // next active sensor
    NULL, // next all sensors
    NULL, // message listeners
    RFIDSensorListenerFunctions,
    sizeof(RFIDSensorListenerFunctions) / sizeof(struct sensorEventBinder)};
