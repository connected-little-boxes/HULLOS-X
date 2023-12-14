#pragma once

extern struct process bootProcessDescriptor;

struct BootSettings 
{
    int accessPointTimeoutSecs;
};

extern struct BootSettings bootSettings;

void getBootReasonMessage(char *buffer, int bufferlength);

int getRestartCode();

int getInternalBootCode();
void setInternalBootCode(int value);
bool isSoftwareReset();

#define COLD_BOOT_MODE 1
#define WARM_BOOT_MODE 2
#define CONFIG_HOST_BOOT_NO_TIMEOUT_MODE 3
#define OTA_UPDATE_BOOT_MODE 4
#define CONFIG_HOST_TIMEOUT_BOOT_MODE 5

#define BOOT_REASON_MESSAGE_SIZE 120 

#define BOOT_CONFIG_AP_DURATION_MIN_SECS 10
#define BOOT_CONFIG_AP_DURATION_MAX_SECS 6000
#define BOOT_CONFIG_AP_DURATION_DEFAULT_SECS 600

extern char bootReasonMessage [BOOT_REASON_MESSAGE_SIZE];

extern unsigned char bootMode;

void getBootMode();

void internalReboot(unsigned char rebootCode);