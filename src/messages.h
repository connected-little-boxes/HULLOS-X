#pragma once

#define MESSAGES_OK 400
#define MESSAGES_STOPPED 401

struct MessagesSettings {
	bool messagesEnabled;
};

void displayMessage(const char *format, ...);
void alwaysDisplayMessage(const char *format, ...);


enum ledFlashBehaviour { ledFlashOn, ledFlashNormalState, ledFlashStartingState,
	ledFlashConfigState, ledFlashAlertState};

void hardwareDisplayMessage(int messageNumber, ledFlashBehaviour flashBehaviour, char * messageText);

void ledFlashBehaviourToString(ledFlashBehaviour severity, char * dest, int length);

bool bindMessageHandler(void(*newHandler)(int messageNumber, ledFlashBehaviour severity, char* messageText));

void messagesOff();

void messagesOn();

void messagesStatusMessage(struct process * inputSwitchProcess, char * buffer, int bufferLength);

extern struct MessagesSettings messagesSettings;

extern struct SettingItemCollection messagesSettingItems;

extern struct process messagesProcess;


