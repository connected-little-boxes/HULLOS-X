#include <Arduino.h>

#include "utils.h"
#include "settings.h"
#include "RFID.h"
#include "processes.h"
#include "mqtt.h"
#include "errors.h"
#include "clock.h"
#include "console.h"
#include "statusled.h"
#include "console.h"
#include "controller.h"
#include <SPI.h>
#include <MFRC522.h>

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

struct SettingItem *RFIDSettingItemPointers[] =
    {
        &RFIDFittedSetting};

struct SettingItemCollection RFIDSensorSettingItems = {
    "RFIDSettings",
    "RFID setup",
    RFIDSettingItemPointers,
    sizeof(RFIDSettingItemPointers) / sizeof(struct SettingItem *)};

struct sensorEventBinder RFIDSensorListenerFunctions[] = {
    {"card", RFIDSENSOR_SEND_ON_CARD_SCANNED}};

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

void IRAM_ATTR readCard()
{
    bNewInt = true;
    reg = mfrc522->PCD_ReadRegister(MFRC522::Status1Reg);    
    Serial.printf("card ahoy 0x%X\n", reg);

}

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

void consumeRFIDJsonResult (char *resultText)
{
    Serial.printf("     %s\n", resultText);
}

#define CARD_ID_LENGTH 9
#define NO_OF_CARDS 200

char seenCards [200] [9];

#define RFID_LIGHT_TIMEOUT_MILLIS 3000

unsigned long rfidLightStart = 0;

void clearCards()
{
    Serial.println("Clearing cards\n");

    for(int cardNo=0;cardNo<NO_OF_CARDS;cardNo++){
        seenCards[cardNo][0]=0;
    }

}

void storeID(char * id)
{
    Serial.printf("Storing id: %s\n", id);
        for(int cardNo=0;cardNo<NO_OF_CARDS;cardNo++){
            if(seenCards[cardNo][0]==0){
                strcpy(seenCards[cardNo],id);
                return;
            }
    }
    Serial.println("No room to store card\n");
}


bool seenCardBefore(char * id){
    Serial.printf("Checking id: %s\n", id);
    for(int cardNo=0;cardNo<NO_OF_CARDS;cardNo++){
        if(seenCards[cardNo][0]==0){
            break;
        }
        Serial.printf("    Testing: %s\n", seenCards[cardNo]);
        if(strcasecmp(id, seenCards[cardNo])==0){
            return true;
        }
    }
    return false;
}

void checkRFIDCard(char * id)
{
    rfidLightStart = millis();

    if(strcasecmp("734addf5",id)==0){
        act_onJson_message("{\"process\":\"pixels\",\"command\":\"setnamedcolour\",\"colourname\":\"white\"}", consumeRFIDJsonResult);
        clearCards();
        return;
    }

    if (seenCardBefore(id)){
        act_onJson_message("{\"process\":\"pixels\",\"command\":\"setnamedcolour\",\"colourname\":\"red\"}", consumeRFIDJsonResult);
    }
    else{
        act_onJson_message("{\"process\":\"pixels\",\"command\":\"setnamedcolour\",\"colourname\":\"green\"}", consumeRFIDJsonResult);
        storeID(id);
    }
}

void updateRFIDLight(){
    if(rfidLightStart!= 0){
        if (ulongDiff(millis(), rfidLightStart) > RFID_LIGHT_TIMEOUT_MILLIS)
        {
            act_onJson_message("{\"process\":\"pixels\",\"command\":\"pattern\",\"pattern\":\"walking\",\"colourmask\":\"RGBY\"}", consumeRFIDJsonResult);
            rfidLightStart = 0;
        }
    }
}

void startRFIDSensor()
{
    clearCards();
    if (RFIDSensor.activeReading == NULL)
    {
        RFIDSensor.activeReading = new RFIDSensorReading();
    }

    if (RFIDSensorSettings.RFIDFitted)
    {
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

                    sensorListener *pos = RFIDSensor.listeners;

                    Serial.printf("Got a card:%s\n", uidbuffer);

                    checkRFIDCard(uidbuffer);

                    RFIDSensor.millisAtLastReading = millis();

                    while (pos != NULL)
                    {
                        if (pos->config->sendOptionMask & RFIDSENSOR_SEND_ON_CARD_SCANNED)
                        {
                            // if the command has a value element we now need to take the element value and put
                            // it into the command data for the message that is about to be received.
                            // The command data value is always the first item in the parameter block

                            char *resultValue = RFIDSensoractiveReading->idString;

                            char *messageBuffer = (char *)pos->config->optionBuffer + MESSAGE_START_POSITION;
                            snprintf(messageBuffer, MAX_MESSAGE_LENGTH, "%s", resultValue);

                            pos->receiveMessage(pos->config->destination, pos->config->optionBuffer);
                            pos->lastReadingMillis = RFIDSensor.millisAtLastReading;
                            // move on to the next one
                            pos = pos->nextMessageListener;
                            continue;
                        }

                        // move on to the next one
                        pos = pos->nextMessageListener;
                    }
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
        Status1Reg  = mfrc522->PCD_ReadRegister(MFRC522::Status1Reg);    
        Status2Reg  = mfrc522->PCD_ReadRegister(MFRC522::Status2Reg);   
        snprintf(buffer, bufferLength, "RFID connected new value flag %d Status: 0x%X 0x%X", bNewInt,Status1Reg,Status2Reg);
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
