#include "registration.h"
#include "mqtt.h"
#include "errors.h"
#include "clock.h"
#include "boot.h"
#include "otaupdate.h"

#if defined(ARDUINO_ARCH_PICO)
#include "WiFi.h"
#endif

struct RegistrationSettings RegistrationSettings;

struct SettingItem friendlyNameSetting = {
	"Friendly name",
	"friendlyName",
	RegistrationSettings.friendlyName,
	FRIENDLY_NAME_LENGTH,
	text,
	setEmptyString,
	validateMQTTtopic};

struct SettingItem *RegistrationSettingItemPointers[] =
	{
		&friendlyNameSetting
};

struct SettingItemCollection registrationSettingItems = {
	"Device Registration",
	"register a device with the portal",
	RegistrationSettingItemPointers,
	sizeof(RegistrationSettingItemPointers) / sizeof(struct SettingItem *)};

struct RegistrationCommand
{
	char *name;
	char *commandDescription;
	void (*actOnCommand)(char *commandLine);
};

void initRegistration()
{
	RegistrationProcess.status = REGISTRATION_OFF;
}

void startRegistration()
{
	RegistrationProcess.status = REGISTRATION_WAITING_FOR_MQTT;
}

void stopRegistration()
{
	RegistrationProcess.status = REGISTRATION_OFF;
}

#define CONNECTION_MESSAGE_BUFFER_SIZE 500

extern struct process *allProcessList;
extern struct sensor *allSensorList;

void buildConfigJson(char *destination, int bufferSize)
{
	struct process *procPtr = allProcessList;

	snprintf(destination, CONNECTION_MESSAGE_BUFFER_SIZE,
			 "%s\"processes\":[", destination);

	bool firstItem = true;

	while (procPtr != NULL)
	{
		if (procPtr->commands != NULL)
		{
			if (procPtr->statusOK())
			{
				if (firstItem)
				{
					snprintf(destination, CONNECTION_MESSAGE_BUFFER_SIZE, "%s \"%s\"", destination, procPtr->processName);
					firstItem = false;
				}
				else
				{
					snprintf(destination, CONNECTION_MESSAGE_BUFFER_SIZE, "%s,\"%s\"", destination, procPtr->processName);
				}
			}
		}
		procPtr = procPtr->nextAllProcesses;
	}

	snprintf(destination, CONNECTION_MESSAGE_BUFFER_SIZE, "%s],\"sensors\":[", destination);

	sensor *allSensorPtr = allSensorList;
	firstItem = true;
	while (allSensorPtr != NULL)
	{
		if (allSensorPtr->status == SENSOR_OK)
		{
			if (firstItem)
			{
				snprintf(destination, CONNECTION_MESSAGE_BUFFER_SIZE, "%s \"%s\"", destination, allSensorPtr->sensorName);
				firstItem = false;
			}
			else
			{
				snprintf(destination, CONNECTION_MESSAGE_BUFFER_SIZE, "%s, \"%s\"", destination, allSensorPtr->sensorName);
			}
		}
		allSensorPtr = allSensorPtr->nextAllSensors;
	}

	snprintf(destination, CONNECTION_MESSAGE_BUFFER_SIZE, "%s]", destination);
}

void sendRegistrationMessage()
{
	char messageBuffer[CONNECTION_MESSAGE_BUFFER_SIZE];
	char deviceNameBuffer [DEVICE_NAME_LENGTH];
	PrintSystemDetails(deviceNameBuffer,DEVICE_NAME_LENGTH);

	unsigned char mac[6];
	WiFi.macAddress(mac);

	snprintf(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE,
			 "{\"name\":\"%s\",\"processor\":\"%s\",\"friendlyName\":\"%s\",\"version\":\"%s\",\"macAddress\":\"%02x:%02x:%02x:%02x:%02x:%02x\",",
			 deviceNameBuffer,
			 PROC_NAME,
			 RegistrationSettings.friendlyName,
			 Version,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	buildConfigJson(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE);

	snprintf(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE, "%s}", messageBuffer);

	publishBufferToMQTTTopic(messageBuffer, MQTT_REGISTERED_TOPIC);
}

void sendConnectionMessage()
{
	char messageBuffer[CONNECTION_MESSAGE_BUFFER_SIZE];
	char deviceNameBuffer [DEVICE_NAME_LENGTH];
	PrintSystemDetails(deviceNameBuffer,DEVICE_NAME_LENGTH);


	snprintf(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE,
				"{\"name\":\"%s\",\"reset\":\"%s\","
				"\"cpu\":\"%s\",\"resetcode\":%d}",
				deviceNameBuffer, bootReasonMessage,
				PROC_NAME, getRestartCode());

	publishBufferToMQTTTopic(messageBuffer, MQTT_CONNECTED_TOPIC);
}

void updateRegistration()
{
	switch (RegistrationProcess.status)
	{
	case REGISTRATION_OK:
		break;

	case REGISTRATION_OFF:
		break;

	case REGISTRATION_WAITING_FOR_MQTT:
		if (MQTTProcessDescriptor.status != MQTT_OK)
		{
			RegistrationProcess.status = REGISTRATION_WAITING_FOR_MQTT;
		}
		else
		{
			if (clockSensor.status == SENSOR_OK)
			{
				// register the settings for this device
				sendRegistrationMessage();

				// say we have connected
				sendConnectionMessage();

				RegistrationProcess.status = REGISTRATION_OK;
			}
		}

		break;

	case WAITING_FOR_REGISTRATION_REPLY:
		if (MQTTProcessDescriptor.status != MQTT_OK)
		{
			RegistrationProcess.status = REGISTRATION_WAITING_FOR_MQTT;
		}

		break;
	}
}

bool registrationStatusOK()
{
	return RegistrationProcess.status == REGISTRATION_OK;
}

void RegistrationStatusMessage(char *buffer, int bufferLength)
{
	switch (RegistrationProcess.status)
	{
	case REGISTRATION_OK:
		snprintf(buffer, bufferLength, "Registration OK");
		break;
	case REGISTRATION_OFF:
		snprintf(buffer, bufferLength, "Registration OFF");
		break;
	case REGISTRATION_WAITING_FOR_MQTT:
		snprintf(buffer, bufferLength, "Waiting for MQTT");
		break;
	case WAITING_FOR_REGISTRATION_REPLY:
		snprintf(buffer, bufferLength, "Waiting for registration reply");
		break;
	default:
		snprintf(buffer, bufferLength, "Registration status invalid");
		break;
	}
}

boolean validateRegistrationCommandString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, REGISTRATION_COMMAND_SIZE));
}

#define REGISTRATION_COMMAND_VALUE_OFFSET 0

struct CommandItem RegistrationCommandName = {
	"name",
	"name of item",
	REGISTRATION_COMMAND_VALUE_OFFSET,
	textCommand,
	validateRegistrationCommandString,
	noDefaultAvailable};

struct CommandItem *RegistrationCommandItems[] =
	{};

int doRemoteRegistrationCommand(char *destination, unsigned char *settingBase);

struct Command performRegistrationCommnad
{
	"register",
		"Perform a remote Registration command",
		RegistrationCommandItems,
		sizeof(RegistrationCommandItems) / sizeof(struct CommandItem *),
		doRemoteRegistrationCommand
};

int doRemoteRegistrationCommand(char *destination, unsigned char *settingBase)
{
	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("registration", &performRegistrationCommnad, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	displayMessage("\nPerforming remote registration\n");

	saveSettings();
	return WORKED_OK;
}

struct CommandItem *RegistrationGetSetupItems[] =
	{};

int doRegistrationGetSetupCommand(char *destination, unsigned char *settingBase);

struct Command RegistrationGetSetupCommand
{
	"getsetup",
		"Get the device setup",
		RegistrationGetSetupItems,
		sizeof(RegistrationGetSetupItems) / sizeof(struct CommandItem *),
		doRegistrationGetSetupCommand
};

int doRegistrationGetSetupCommand(char *destination, unsigned char *settingBase)
{
	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("registration", &RegistrationGetSetupCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	char messageBuffer[CONNECTION_MESSAGE_BUFFER_SIZE];
	char deviceNameBuffer [DEVICE_NAME_LENGTH];
	PrintSystemDetails(deviceNameBuffer,DEVICE_NAME_LENGTH);

	snprintf(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE,
				"{\"name\":\"%s\",",
				deviceNameBuffer);

	buildConfigJson(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE);

	snprintf(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE, "%s}", messageBuffer);

	displayMessage(messageBuffer);

	return WORKED_OK;
}

struct CommandItem *RegistrationGetSettingsItems[] =
	{
		&RegistrationCommandName};

int doRegistrationGetSettingsCommand(char *destination, unsigned char *settingBase);

struct Command RegistrationGetProcessSettingsCommand
{
	"getsettings",
		"Get the settings for a process or sensor",
		RegistrationGetSettingsItems,
		sizeof(RegistrationGetSettingsItems) / sizeof(struct CommandItem *),
		doRegistrationGetSettingsCommand
};

int doRegistrationGetSettingsCommand(char *destination, unsigned char *settingBase)
{
	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("registration", &RegistrationGetProcessSettingsCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	char *name = (char *)(settingBase + REGISTRATION_COMMAND_VALUE_OFFSET);

	SettingItemCollection *settingCollection = NULL;

	struct process *proc = findProcessByName(name);

	if (proc != NULL)
	{
		settingCollection = proc->settingItems;
	}
	else
	{
		struct sensor *sensor = findSensorByName(name);
		if (sensor != NULL)
		{
			settingCollection = sensor->settingItems;
		}
	}

	if (settingCollection == NULL)
	{
		return JSON_MESSAGE_SETTINGS_NOT_FOUND;
	}

	char messageBuffer[CONNECTION_MESSAGE_BUFFER_SIZE];
	char deviceNameBuffer [DEVICE_NAME_LENGTH];
	PrintSystemDetails(deviceNameBuffer,DEVICE_NAME_LENGTH);

	snprintf(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE,
			 "{\"name\":\"%s\",\"name\":\"%s\",\"settings\":",
			 deviceNameBuffer, name);

	appendSettingCollectionJson(settingCollection, messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE);

	snprintf(messageBuffer, CONNECTION_MESSAGE_BUFFER_SIZE, "%s}", messageBuffer);

	displayMessage(messageBuffer);

	return WORKED_OK;
}

struct Command *RegistrationCommandList[] = {
	&performRegistrationCommnad,
	&RegistrationGetSetupCommand,
	&RegistrationGetProcessSettingsCommand};

struct CommandItemCollection RegistrationCommands =
	{
		"Perform Registration commands on a remote device",
		RegistrationCommandList,
		sizeof(RegistrationCommandList) / sizeof(struct Command *)};

struct process RegistrationProcess = {
	"registration",
	initRegistration,
	startRegistration,
	updateRegistration,
	stopRegistration,
	registrationStatusOK,
	RegistrationStatusMessage,
	false,
	0,
	0,
	0,
	NULL,
	(unsigned char *)&RegistrationSettings, sizeof(RegistrationSettings), &registrationSettingItems,
	&RegistrationCommands,
	BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
	NULL,
	NULL,
	NULL};
