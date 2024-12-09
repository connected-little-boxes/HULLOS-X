#include <strings.h>
#include "utils.h"
#include "settings.h"
#include "sensors.h"
#include "processes.h"
#include "connectwifi.h"
#include "settingsWebServer.h"
#include "boot.h"
#include "pixels.h"
#include "controller.h"

#if defined(ARDUINO_ARCH_ESP32)

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#endif

boolean validateWifiSSID(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, WIFI_SSID_LENGTH));
}

boolean validateWifiPWD(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, WIFI_PASSWORD_LENGTH));
}

struct WifiConnectionSettings wifiConnectionSettings;

struct SettingItem wifiOnOff = {
	"Wifi on", 
	"wifiactive", 
	&wifiConnectionSettings.wiFiOn, 
	YESNO_INPUT_LENGTH, 
	yesNo, 
	setFalse, 
	validateYesNo};

void setDefaultWiFi1SSID(void *dest)
{
	strcpy((char *)dest, DEFAULT_WIFI1_SSID);
}

void setDefaultWiFi1Pwd(void *dest)
{
	strcpy((char *)dest, DEFAULT_WIFI1_PWD);
}

struct SettingItem wifi1SSIDSetting = {
	"WiFiSSID1", "wifissid1", wifiConnectionSettings.wifi1SSID, WIFI_SSID_LENGTH, text, setDefaultWiFi1SSID, validateWifiSSID};
struct SettingItem wifi1PWDSetting = {
	"WiFiPassword1", "wifipwd1", wifiConnectionSettings.wifi1PWD, WIFI_PASSWORD_LENGTH, password, setDefaultWiFi1Pwd, validateWifiPWD};

struct SettingItem wifi2SSIDSetting = {
	"WiFiSSID2", "wifissid2", wifiConnectionSettings.wifi2SSID, WIFI_SSID_LENGTH, text, setEmptyString, validateWifiSSID};
struct SettingItem wifi2PWDSetting = {
	"WiFiPassword2", "wifipwd2", wifiConnectionSettings.wifi2PWD, WIFI_PASSWORD_LENGTH, password, setEmptyString, validateWifiPWD};

struct SettingItem wifi3SSIDSetting = {
	"WiFiSSID3", "wifissid3", wifiConnectionSettings.wifi3SSID, WIFI_SSID_LENGTH, text, setEmptyString, validateWifiSSID};
struct SettingItem wifi3PWDSetting = {
	"WiFiPassword3", "wifipwd3", wifiConnectionSettings.wifi3PWD, WIFI_PASSWORD_LENGTH, password, setEmptyString, validateWifiPWD};

struct SettingItem wifi4SSIDSetting = {
	"WiFiSSID4", "wifissid4", wifiConnectionSettings.wifi4SSID, WIFI_SSID_LENGTH, text, setEmptyString, validateWifiSSID};
struct SettingItem wifi4PWDSetting = {
	"WiFiPassword4", "wifipwd4", wifiConnectionSettings.wifi4PWD, WIFI_PASSWORD_LENGTH, password, setEmptyString, validateWifiPWD};

struct SettingItem wifi5SSIDSetting = {
	"WiFiSSID5", "wifissid5", wifiConnectionSettings.wifi5SSID, WIFI_SSID_LENGTH, text, setEmptyString, validateWifiSSID};
struct SettingItem wifi5PWDSetting = {
	"WiFiPassword5", "wifipwd5", wifiConnectionSettings.wifi5PWD, WIFI_PASSWORD_LENGTH, password, setEmptyString, validateWifiPWD};

struct SettingItem *wifiConnectionSettingItemPointers[] =
	{
		&wifiOnOff,
		&wifi1SSIDSetting,
		&wifi1PWDSetting,

		&wifi2SSIDSetting,
		&wifi2PWDSetting,

		&wifi3SSIDSetting,
		&wifi3PWDSetting,

		&wifi4SSIDSetting,
		&wifi4PWDSetting,

		&wifi5SSIDSetting,
		&wifi5PWDSetting};

struct SettingItemCollection wifiConnectionSettingItems = {
	"WiFi",
	"WiFi ssids and passwords",
	wifiConnectionSettingItemPointers,
	sizeof(wifiConnectionSettingItemPointers) / sizeof(struct SettingItem *)};

// Note that if we add new WiFi passwords into the settings
// we will have to update this table.

struct WiFiSetting wifiSettings[] =
	{
		{wifiConnectionSettings.wifi1SSID, wifiConnectionSettings.wifi1PWD},
		{wifiConnectionSettings.wifi2SSID, wifiConnectionSettings.wifi2PWD},
		{wifiConnectionSettings.wifi3SSID, wifiConnectionSettings.wifi3PWD},
		{wifiConnectionSettings.wifi4SSID, wifiConnectionSettings.wifi4PWD},
		{wifiConnectionSettings.wifi5SSID, wifiConnectionSettings.wifi5PWD}};

bool needWifiConfigBootMode()
{
	if (!wifiConnectionSettings.wiFiOn)
	{
		return false;
	}

	for (unsigned int i = 0; i < sizeof(wifiSettings) / sizeof(struct WiFiSetting); i++)
	{
		if (wifiSettings[i].wifiSsid[0] != 0)
		{
			return false;
		}
	}
	return true;
}

#define WIFI_SETTING_NOT_FOUND -1

int findWifiSetting(String ssidName)
{
	char ssidBuffer[WIFI_SSID_LENGTH];

	ssidName.toCharArray(ssidBuffer, WIFI_SSID_LENGTH);

	displayMessage("*       Checking SSID:%s\n", ssidBuffer);

	for (unsigned int i = 0; i < sizeof(wifiSettings) / sizeof(struct WiFiSetting); i++)
	{
		if(wifiSettings[i].wifiSsid[0]==0){
			// no WiFi setting here - ignore it
			continue;
		}

		if (strcasecmp(wifiSettings[i].wifiSsid, ssidBuffer) == 0)
		{
			return i;
		}
	}
	return WIFI_SETTING_NOT_FOUND;
}

char wifiActiveAPName[WIFI_SSID_LENGTH];
int wifiError;

boolean firstRun = true;
unsigned long WiFiTimerStart;

int wifiConnectAttempts = 0;

void beginWiFiScanning()
{
	if (wifiConnectionSettings.wiFiOn == false)
	{
		WiFiProcessDescriptor.status = WIFI_TURNED_OFF;
		return;
	}

	WiFiTimerStart = millis();

	if (firstRun)
	{
#ifndef PICO
		WiFi.mode(WIFI_OFF);
		delay(100);
		WiFi.mode(WIFI_STA);
		delay(100);
#endif
		firstRun = false;
		wifiConnectAttempts = 0;
	}

	WiFi.scanNetworks(true);
	WiFiProcessDescriptor.status = WIFI_SCANNING;
}

void startReconnectTimer()
{
	WiFiTimerStart = millis();
	WiFiProcessDescriptor.status = WIFI_RECONNECT_TIMER;
}

void updateReconnectTimer()
{
	if (ulongDiff(millis(), WiFiTimerStart) > WIFI_CONNECT_RETRY_MILLS)
	{
		beginWiFiScanning();
	}
}

void handleConnectFailure()
{
	if (bootMode == WARM_BOOT_MODE)
	{
		// if we have already rebooted once because of a connection failure
		// we don't do it again.
		return;
	}

	wifiConnectAttempts++;

	if (wifiConnectAttempts >= WIFI_MAX_NO_OF_FAILED_SCANS)
	{
		TRACELOG("Reset due to WiFi lockup");
		internalReboot(CONFIG_HOST_TIMEOUT_BOOT_MODE);
	}
}

void handleFailedWiFiScan()
{
	displayMessage("No networks found that match stored network names\n");
	handleConnectFailure();
	hardwareDisplayMessage(WIFI_STATUS_NO_MATCHING_NETWORKS_MESSAGE_NUMBER, ledFlashAlertState, WIFI_STATUS_NO_MATCHING_NETWORKS_MESSAGE_TEXT);
	startReconnectTimer();
}

#ifdef PICO
#define WIFI_SCAN_RUNNING -1
#endif

void checkWiFiScanResult()
{
	int noOfNetworks = WiFi.scanComplete();

	if (noOfNetworks == WIFI_SCAN_RUNNING)
	{
		if (ulongDiff(millis(), WiFiTimerStart) > WIFI_SCAN_TIMEOUT_MILLIS)
		{
			WiFiProcessDescriptor.status = WIFI_ERROR_SCAN_TIMEOUT;
			displayMessage("WiFi scan timeout\n");
		}
		return;
	}

	int settingNumber;

	displayMessage("*       WiFi scan complete %d networks found.\n", noOfNetworks);
	// if we get here we have some networks
	for (int i = 0; i < noOfNetworks; ++i)
	{
		settingNumber = findWifiSetting(WiFi.SSID(i));

		if (settingNumber != WIFI_SETTING_NOT_FOUND)
		{
			snprintf(wifiActiveAPName, WIFI_SSID_LENGTH, "%s", wifiSettings[settingNumber].wifiSsid);
			displayMessage("*       Connecting to %s\n", wifiActiveAPName);
			WiFi.begin(wifiSettings[settingNumber].wifiSsid,
					   wifiSettings[settingNumber].wifiPassword);
			WiFiTimerStart = millis();
			WiFiProcessDescriptor.status = WIFI_CONNECTING;
			return;
		}
	}

	// if we get here we didn't find a matching network

	handleFailedWiFiScan();
}

bool syncConnectToAP(char *wifiSsid, char *wifiPassword)
{
	int timeoutCount = 0;
	WiFi.begin(wifiSsid, wifiPassword);

	displayMessage("Connecting to WiFi %s\n", wifiSsid);

	while ((WiFi.status() != WL_CONNECTED) && (timeoutCount++ < 20))
	{
		delay(500);
		Serial.print(".");
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		displayMessage("\nConnected OK\n");
		return true;
	}
	else
	{
		displayMessage("\nConnect failed\n");
		return false;
	}
}

bool syncWiFiConnect()
{
	displayMessage("Performing Synchronous WiFi connect");

	#if defined(ARDUINO_ARCH_ESP32)

	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector 

	#endif

	WiFi.mode(WIFI_OFF);
	delay(200);
	WiFi.mode(WIFI_STA);
	delay(200);
	displayMessage("Starting scan");

	int networkCount = WiFi.scanNetworks();

	if (networkCount == 0)
	{
		displayMessage("No WiFi networks found.");
	}
	else
	{
		displayMessage("*       WiFi scan complete %d networks found.\n", networkCount);
		int settingNumber;

		// Print SSID and signal strength for each network
		for (int i = 0; i < networkCount; ++i)
		{
			settingNumber = findWifiSetting(WiFi.SSID(i));

			if (settingNumber != WIFI_SETTING_NOT_FOUND)
			{
				return syncConnectToAP(wifiSettings[settingNumber].wifiSsid,wifiSettings[settingNumber].wifiPassword );
			}
		}
	}
	return false;
}

void handleConnectTimeout()
{
	TRACELOGLN("WiFi connect timeout");
	WiFi.scanDelete();
	WiFiProcessDescriptor.status = WIFI_ERROR_CONNECT_TIMEOUT;
	hardwareDisplayMessage(WIFI_STATUS_CONNECT_FAILED_MESSAGE_NUMBER, ledFlashAlertState, WIFI_STATUS_CONNECT_FAILED_MESSAGE_TEXT);
}

void checkWiFiConnectResult()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		if (ulongDiff(millis(), WiFiTimerStart) > WIFI_CONNECT_TIMEOUT_MILLIS)
		{
			handleConnectTimeout();
		}
		return;
	}

	// if we get here we are connected
	TRACELOGLN("Deleting scan");
	WiFi.scanDelete();

	wifiError = WiFi.status();

	if (wifiError == WL_CONNECTED)
	{
		TRACELOGLN("Wifi OK");
		WiFiProcessDescriptor.status = WIFI_OK;
		char messageBuffer[WIFI_MESSAGE_BUFFER_SIZE];
		snprintf(messageBuffer, WIFI_MESSAGE_BUFFER_SIZE, "%s %s", WIFI_STATUS_OK_MESSAGE_TEXT, WiFi.localIP().toString().c_str());
		hardwareDisplayMessage(WIFI_STATUS_OK_MESSAGE_NUMBER, ledFlashNormalState, messageBuffer);
		wifiConnectAttempts = 0;
		performCommandsInStore(WIFI_CONNECT_COMMAND_STORE);
		return;
	}

	TRACELOG("Fail status:");
	TRACE_HEXLN(wifiError);
	hardwareDisplayMessage(WIFI_STATUS_CONNECT_FAILED_MESSAGE_NUMBER, ledFlashAlertState, WIFI_STATUS_CONNECT_FAILED_MESSAGE_TEXT);
	startReconnectTimer();
}

void displayWiFiStatus(int status)
{
	switch (status)
	{
	case WL_IDLE_STATUS:
		TRACELOG("Idle");
		break;
	case WL_NO_SSID_AVAIL:
		TRACELOG("No SSID");
		break;
	case WL_SCAN_COMPLETED:
		TRACELOG("Scan completed");
		break;
	case WL_CONNECTED:
		TRACELOG("Connected");
		break;
	case WL_CONNECT_FAILED:
		TRACELOG("Connect failed");
		break;
	case WL_CONNECTION_LOST:
		TRACELOG("Connection lost");
		break;
	case WL_DISCONNECTED:
		TRACELOG("Disconnected");
		break;
	default:
		TRACELOG("WiFi status value: ");
		TRACELOG(status);
		break;
	}
}

void checkWiFIOK()
{
	int wifiStatusValue = WiFi.status();

	if (wifiStatusValue != WL_CONNECTED)
	{
		performCommandsInStore(WIFI_DISCONNECT_COMMAND_STORE);
		startReconnectTimer();
	}
}

void stopWiFi()
{
	TRACELOGLN("WiFi turned off");

	WiFiProcessDescriptor.status = WIFI_TURNED_OFF;
	WiFi.mode(WIFI_OFF);
	delay(500);
}

void initWifi()
{
	WiFiProcessDescriptor.status = WIFI_TURNED_OFF;
}

void startWifi()
{
	if (wifiConnectionSettings.wiFiOn)
	{
		beginWiFiScanning();
	}
	else
	{
		WiFiProcessDescriptor.status = WIFI_TURNED_OFF;
		WiFi.mode(WIFI_OFF);
	}
}

void checkWiFiTurnedOff()
{
	if (wifiConnectionSettings.wiFiOn)
	{
		beginWiFiScanning();
	}
}

void updateNoMatchingNetworks()
{
}

void wifiStatusMessage(char *buffer, int bufferLength)
{
	switch (WiFiProcessDescriptor.status)
	{
	case WIFI_OK:
		snprintf(buffer, bufferLength, "%s: %s", wifiActiveAPName, WiFi.localIP().toString().c_str());
		break;
	case WIFI_TURNED_OFF:
		snprintf(buffer, bufferLength, "Wifi OFF");
		break;
	case WIFI_SCANNING:
		snprintf(buffer, bufferLength, "Scanning for networks");
		break;
	case WIFI_ERROR_CONNECT_TIMEOUT:
		snprintf(buffer, bufferLength, "Connect timeout");
		break;
	case WIFI_CONNECTING:
		snprintf(buffer, bufferLength, "Connecting to %s", wifiActiveAPName);
		break;
	case WIFI_ERROR_SCAN_TIMEOUT:
		snprintf(buffer, bufferLength, "WiFi scan timeout");
		break;
	case WIFI_RECONNECT_TIMER:
		snprintf(buffer, bufferLength, "WiFi waiting to reconnect");
		break;
	default:
		snprintf(buffer, bufferLength, "WiFi status invalid %d", WiFiProcessDescriptor.status);
		break;
	}
}

void updateWifi()
{
	switch (WiFiProcessDescriptor.status)
	{
	case WIFI_OK:
		checkWiFIOK();
		break;

	case WIFI_TURNED_OFF:
		checkWiFiTurnedOff();
		break;

	case WIFI_SCANNING:
		checkWiFiScanResult();
		break;

	case WIFI_CONNECTING:
		checkWiFiConnectResult();
		break;

	case WIFI_ERROR_CONNECT_TIMEOUT:
		startReconnectTimer();
		break;

	case WIFI_RECONNECT_TIMER:
		updateReconnectTimer();
		break;

	case WIFI_ERROR_SCAN_TIMEOUT:
		startReconnectTimer();
		break;
	}
}

bool connectWiFiStatusOK()
{
	return WiFiProcessDescriptor.status == WIFI_OK;
}

struct process WiFiProcessDescriptor = {
	"wifi",
	initWifi,
	startWifi,
	updateWifi,
	stopWiFi,
	connectWiFiStatusOK,
	wifiStatusMessage,
	false,
	0,
	0,
	0,
	NULL,
	(unsigned char *)&wifiConnectionSettings, sizeof(WifiConnectionSettings), &wifiConnectionSettingItems,
	NULL,
	BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS,
	NULL,
	NULL,
	NULL,
	NULL, // no command options
	0	  // no command options
};
