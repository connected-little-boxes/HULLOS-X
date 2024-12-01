#pragma once

#include "version.h"
#include "processes.h"
#include "connectwifi.h"

#define OTAUPDATE_OK 1200
#define OTAUPDATE_OFF 1201

void performOTAUpdate();
void checkBootOtaUpdate();
void requestOTAUpate();

extern struct process otaUpdateProcessDescriptor;

struct OtaUpdateSettings 
{
    boolean otaUpdateRequested;
};

extern struct OtaUpdateSettings otaUpdateSettings;

#define ESP8266_OTA_UPDATE_URL "http://clbportal.com/firmware/ota/esp8266/firmware.bin"
//#define ESP8266_OTA_UPDATE_URL "http://192.168.4.43:3000/firmware/ota/esp8266/firmware.bin"

#define ESP32_OTA_UPDATE_URL "http://clbportal.com/firmware/ota/esp32/firmware.bin"
//#define ESP32_OTA_UPDATE_URL "http://192.168.4.43:3000/firmware/ota/esp32/firmware.bin"
