#include <Arduino.h>

#include "utils.h"
#include "settings.h"
#include "messages.h"
#include "processes.h"
#include "robotProcess.h"

struct MessagesSettings messagesSettings;

struct SettingItem messagesEnabled = {
    "Messages enabled",
    "messagesactive",
    &messagesSettings.messagesEnabled,
    ONOFF_INPUT_LENGTH,
    yesNo,
    setTrue,
    validateYesNo};

struct SettingItem *messagesSettingItemPointers[] =
    {
        &messagesEnabled};

struct SettingItemCollection messagesSettingItems = {
    "messages",
    "Enable/disable for message output",
    messagesSettingItemPointers,
    sizeof(messagesSettingItemPointers) / sizeof(struct SettingItem *)};

void messagesOff()
{
    messagesSettings.messagesEnabled = false;
    saveSettings();
}

void messagesOn()
{
    messagesSettings.messagesEnabled = true;
    saveSettings();
}

void displayMessage(const char *format, ...)
{

    // don't send messages when turned off or the robot not enabled
    
    if (!messagesSettings.messagesEnabled | robotSettings.robotEnabled)
    {
        return;
    }

    // Use va_list to handle variable number of arguments
    va_list args;
    va_start(args, format);

    // Use vsnprintf to format the string
    // Assumes a maximum buffer size of 256 characters
    char buffer[512];
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    // Check if vsnprintf was successful
    if (len >= 0 && len < sizeof(buffer))
    {
        // Print the formatted string
        Serial.print(buffer);
    }

    va_end(args);
}

void alwaysDisplayMessage(const char *format, ...)
{
    // Use va_list to handle variable number of arguments
    va_list args;
    va_start(args, format);

    // Use vsnprintf to format the string
    // Assumes a maximum buffer size of 256 characters
    char buffer[512];
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    // Check if vsnprintf was successful
    if (len >= 0 && len < sizeof(buffer))
    {
        // Print the formatted string
        Serial.print(buffer);
    }

    va_end(args);
}



// enought room for four message handlers

void (*messageHandlerList[])(int messageNumber, ledFlashBehaviour severity, char *messageText) = {NULL, NULL, NULL, NULL};

int noOfMessageHandlers = sizeof(messageHandlerList) / sizeof(int (*)(int, char *));

bool bindMessageHandler(void (*newHandler)(int messageNumber, ledFlashBehaviour severity, char *messageText))
{
    for (int i = 0; i < noOfMessageHandlers; i++)
        if (messageHandlerList[i] == NULL)
        {
            messageHandlerList[i] = newHandler;
            return true;
        }
    return false;
}

void ledFlashBehaviourToString(ledFlashBehaviour severity, char *dest, int length)
{
    switch (severity)
    {
    case ledFlashOn:
        snprintf(dest, length, "Starting");
        break;

    case ledFlashNormalState:
        snprintf(dest, length, "OK");
        break;

    case ledFlashConfigState:
        snprintf(dest, length, "Config");
        break;

    case ledFlashAlertState:
        snprintf(dest, length, "Alert");
        break;

    default:
        snprintf(dest, length, "Unknown");
        break;
    }
}

void hardwareDisplayMessage(int messageNumber, ledFlashBehaviour severity, char *messageText)
{
    for (int i = 0; i < noOfMessageHandlers; i++)
    {
        if (messageHandlerList[i] != NULL)
        {
            messageHandlerList[i](messageNumber, severity, messageText);
        }
    }
}

void initMessages()
{
    messagesProcess.status = MESSAGES_STOPPED;
}

void startMessages()
{
    if (messagesSettings.messagesEnabled)
    {
        messagesProcess.status = MESSAGES_OK;
    }
}

void updateMessages()
{
}

void stopmessages()
{
    messagesProcess.status = MESSAGES_STOPPED;
}

bool messagesStatusOK()
{
    return messagesProcess.status == MESSAGES_OK;
}

void messagesStatusMessage(char *buffer, int bufferLength)
{
    if (messagesProcess.status == MESSAGES_STOPPED)
    {
        snprintf(buffer, bufferLength, "Messages stopped");
    }
    else
    {
        snprintf(buffer, bufferLength, "Messages enabled");
    }
}

struct process messagesProcess = {
    "messages",
    initMessages,
    startMessages,
    updateMessages,
    stopmessages,
    messagesStatusOK,
    messagesStatusMessage,
    false,
    0,
    0,
    0,
    NULL,
    (unsigned char *)&messagesSettings, sizeof(MessagesSettings), &messagesSettingItems,
    NULL,
    BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
    NULL,
    NULL,
    NULL,
    NULL, // no command options
    0     // no command options
};
