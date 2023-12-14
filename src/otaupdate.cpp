#include "utils.h"
#include "processes.h"
#include "settings.h"
#include "pixels.h"
#include "otaupdate.h"
#include "connectwifi.h"
#include "boot.h"
#include "statusled.h"

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266httpUpdate.h>
#endif

struct OtaUpdateSettings otaUpdateSettings;

struct SettingItem otaUpdateRequested = {
	"otaUpdateRequested (yes or no)",
	"otaupdaterequested",
	&otaUpdateSettings.otaUpdateRequested,
	ONOFF_INPUT_LENGTH,
	yesNo,
	setFalse, validateYesNo};

struct SettingItem *otaUpdateSettingItemPointers[] =
	{
		&otaUpdateRequested};

struct SettingItemCollection otaUpdateSettingItems = {
	"otaupdate",
	"Over The Air (OTA) update settings",
	otaUpdateSettingItemPointers,
	sizeof(otaUpdateSettingItemPointers) / sizeof(struct SettingItem *)};

void update_started()
{
	alwaysDisplayMessage("CALLBACK:  HTTP update process started");
}

void update_finished()
{
	alwaysDisplayMessage("CALLBACK:  HTTP update process finished");
	delay(1000);
	internalReboot(WARM_BOOT_MODE);
}

bool ota_update_init = true;

void update_progress(int cur, int total)
{
	statusLedToggle();
}

void update_error(int err)
{
	alwaysDisplayMessage("CALLBACK:  HTTP update fatal error code %d\n", err);
	delay(1000);
	internalReboot(COLD_BOOT_MODE);
}

void performOTAUpdate()
{

	beginStatusDisplay(VERY_DARK_RED_COLOUR);

	WiFiClient client;

	alwaysDisplayMessage("Boot OTA Update requested");

	if (!syncWiFiConnect())
	{
		internalReboot(COLD_BOOT_MODE);
	}

	alwaysDisplayMessage("WiFi Connected");

  initStatusLedHardware();
  statusLedOn();

#if defined(ARDUINO_ARCH_ESP8266)

	alwaysDisplayMessage("Performing OTA update from %s\n", ESP8266_OTA_UPDATE_URL);

	//	ESPhttpUpdate.onStart(update_started);
	ESPhttpUpdate.onEnd(update_finished);
	ESPhttpUpdate.onProgress(update_progress);
	ESPhttpUpdate.onError(update_error);

	t_httpUpdate_return ret = ESPhttpUpdate.update(client, ESP8266_OTA_UPDATE_URL, Version);

	alwaysDisplayMessage("Update running..........");

#endif

#if defined(ARDUINO_ARCH_ESP32)

	alwaysDisplayMessage("Performing OTA update from %s\n", ESP32_OTA_UPDATE_URL);

	httpUpdate.onEnd(update_finished);
	httpUpdate.onError(update_error);
	httpUpdate.onProgress(update_progress);
	t_httpUpdate_return ret = httpUpdate.update(client, ESP32_OTA_UPDATE_URL);

#endif
}

void requestOTAUpate()
{
	alwaysDisplayMessage("OTA Update");

	internalReboot(OTA_UPDATE_BOOT_MODE);
}
