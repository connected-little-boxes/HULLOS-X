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
#include <SPI.h>
#include <MFRC522.h>
#include "RFID.h"

#define RFID_MESSAGE_BUFFER_SIZE 256

struct RFIDSettings RFIDSettings;

struct SettingItem RFIDEnabledSetting = {
    "RFID enabled",
    "rfidon",
    &RFIDSettings.RFIDEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setFalse,
    validateYesNo};

void setDefaultRFIDmillisBetweenUpdates(void *dest)
{
    int *destInt = (int *)dest;
    *destInt = 100;
}

struct SettingItem RFIDmillisBetweenUpdates = {
    "RFID milliseconds between updates",
    "rfidmillisupdate",
    &RFIDSettings.millisBetweenUpdates,
    NUMBER_INPUT_LENGTH,
    integerValue,
    setDefaultRFIDmillisBetweenUpdates,
    validateInt};

struct SettingItem *RFIDSettingItemPointers[] =
    {
        &RFIDEnabledSetting,
        &RFIDmillisBetweenUpdates
    };

struct SettingItemCollection RFIDMessagesSettingItems = {
    "RFIDSettings",
    "RFID setup",
    RFIDSettingItemPointers,
    sizeof(RFIDSettingItemPointers) / sizeof(struct SettingItem *)};

void RFIDMessagesOff()
{
    RFIDSettings.RFIDEnabled = false;
    saveSettings();
}

void RFIDMessagesOn()
{
    RFIDSettings.RFIDEnabled = true;
    saveSettings();
}

void sendRFIDMessageToServer(char *messageText)
{
    char messageBuffer[RFID_MESSAGE_BUFFER_SIZE];
    char deviceNameBuffer[DEVICE_NAME_LENGTH];

    PrintSystemDetails(deviceNameBuffer, DEVICE_NAME_LENGTH);

    snprintf(messageBuffer, RFID_MESSAGE_BUFFER_SIZE,
             "{\"name\":\"%s\",\"from\":\"RFID\",\"message\":\"%s\"}",
             deviceNameBuffer,
             messageText);

    publishBufferToMQTTTopic(messageBuffer, RFID_TOPIC);
}

void sendMessageToRFID(char *messageText)
{
    statusLedToggle();
    Serial.print((char)0x0d);
    Serial.print(messageText);
    Serial.print((char)0x0d);
    Serial.print((char)0x0a);
}

#define RFID_FLOAT_VALUE_OFFSET 0
#define RFID_MESSAGE_OFFSET (RFID_FLOAT_VALUE_OFFSET + sizeof(float))

boolean validateRFIDOptionString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, RFID_MESSAGE_COMMAND_LENGTH));
}

boolean validateRFIDMessageString(void *dest, const char *newValueStr)
{
    return (validateString((char *)dest, newValueStr, RFID_MESSAGE_LENGTH));
}

struct CommandItem RFIDMessageText = {
    "text",
    "RFID message text",
    RFID_MESSAGE_OFFSET,
    textCommand,
    validateRFIDMessageString,
    noDefaultAvailable};

struct CommandItem *RFIDCommandItems[] =
    {
        &RFIDMessageText};

int doSendRFIDMessage(char *destination, unsigned char *settingBase);

struct Command RFIDMessageCommand
{
    "Send",
        "Sends a message to the RFID",
        RFIDCommandItems,
        sizeof(RFIDCommandItems) / sizeof(struct CommandItem *),
        doSendRFIDMessage
};

int doSendRFIDMessage(char *destination, unsigned char *settingBase)
{
    if (*destination != 0)
    {
        // we have a destination for the command. Build the string
        char buffer[JSON_BUFFER_SIZE];
        createJSONfromSettings("RFID", &RFIDMessageCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
        return publishCommandToRemoteDevice(buffer, destination);
    }

    if (RFIDProcess.status != RFID_CONNECTED)
    {
        return JSON_MESSAGE_RFID_NOT_ENABLED;
    }

    char *message = (char *)(settingBase + RFID_MESSAGE_OFFSET);

    sendMessageToRFID(message);

    TRACELOG("Sending: ");
    TRACELOGLN(buffer);

    return WORKED_OK;
}

struct Command *RFIDCommandList[] = {
    &RFIDMessageCommand};

struct CommandItemCollection RFIDCommands =
    {
        "Control RFID",
        RFIDCommandList,
        sizeof(RFIDCommandList) / sizeof(struct Command *)};

unsigned long RFIDmillisAtLastScroll;

void initRFIDProcess()
{
    // all the setup is performed in start
}

MFRC522 * mfrc522 = NULL; // Create MFRC522 reference

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

    SPI.begin();        // Init SPI bus
    mfrc522->PCD_Init(); // Init MFRC522

    pollRFID();
}

void startRFIDProcess()
{
    if (RFIDSettings.RFIDEnabled)
    {
        SPI.begin();        // Init SPI bus

        if (mfrc522 == NULL){
            mfrc522 = new MFRC522(SS_PIN, RST_PIN); 
            mfrc522->PCD_Init(); // Init MFRC522
        }

        RFIDProcess.status = RFID_CONNECTED;
    }
    else
    {
        RFIDProcess.status = RFID_OFF;
    }
}

void processMessageFromRFID()
{
    if (RFIDReceiveBuffer[0] == '#')
    {
        actOnSerialCommand(RFIDReceiveBuffer + 1);
    }
    else
    {
        sendRFIDMessageToServer(RFIDReceiveBuffer);
    }
}

void bufferRFIDSerialChar(char ch)
{
    if (ch == '\n' || ch == '\r' || ch == 0)
    {
        if (RFIDReceiveBufferPos > 0)
        {
            RFIDReceiveBuffer[RFIDReceiveBufferPos] = 0;
            processMessageFromRFID();
            reset_RFID_buffer();
        }
        return;
    }

    if (RFIDReceiveBufferPos < RFID_BUFFER_SIZE)
    {
        RFIDReceiveBuffer[RFIDReceiveBufferPos] = ch;
        RFIDReceiveBufferPos++;
    }
}

void checkRFIDBuffer()
{
    while (Serial.available())
    {
        bufferRFIDSerialChar(Serial.read());
    }
}

void updateRFIDProcess()
{
    if (RFIDProcess.status == RFID_CONNECTED)
    {
        // Look for new cards
        if (!mfrc522->PICC_IsNewCardPresent())
        {
            return;
        }

        // Select one of the cards
        if (!mfrc522->PICC_ReadCardSerial())
        {
            return;
        }

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

        unsigned char *uidbuffer = new unsigned char[length + 1]; // +1 for null terminator if needed

        for (size_t i = 0; i < length; ++i)
        {
            uidbuffer[i] = static_cast<unsigned char>(uidString[i]);
        }

        uidbuffer[length] = '\0';

        Serial.printf("UID: %s\n", uidbuffer);

        // Halt PICC
        mfrc522->PICC_HaltA();
    }
}

void stopRFIDProcess()
{
    RFIDProcess.status = RFID_OFF;
}

bool RFIDStatusOK()
{
    return RFIDProcess.status == RFID_CONNECTED;
}

void RFIDStatusMessage(char *buffer, int bufferLength)
{
    switch(RFIDProcess.status){
        case RFID_CONNECTED:
        snprintf(buffer, bufferLength, "RFID connected");
        break;
        case RFID_OFF:
        snprintf(buffer, bufferLength, "RFID off");
        break;
        case RFID_ON:
        snprintf(buffer, bufferLength, "RFID on");
        break;
    }
}

struct process RFIDProcess = {
    "RFID",
    initRFIDProcess,
    startRFIDProcess,
    updateRFIDProcess,
    stopRFIDProcess,
    RFIDStatusOK,
    RFIDStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&RFIDSettings, sizeof(RFIDSettings), &RFIDMessagesSettingItems,
    &RFIDCommands,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL,
    NULL, // no command options
    0     // no command options
};
