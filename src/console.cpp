#include <strings.h>
#include "console.h"
#include "otaupdate.h"
#include "connectwifi.h"
#include "mqtt.h"
#include "errors.h"
#include "rotarySensor.h"
#include "buttonsensor.h"
#include "potSensor.h"
#include "pixels.h"
#include "connectwifi.h"
#include "settingsWebServer.h"
#include "HullOS.h"
#include "boot.h"
#include "robotProcess.h"
#include <LittleFS.h>

struct ConsoleSettings consoleSettings;

// set to true during startup to force a console to override any serial settings

bool forceConsole = false;

struct SettingItem echoSerialInput = {
	"Echo serial input",
	"echoserial",
	&consoleSettings.echoInput,
	ONOFF_INPUT_LENGTH,
	yesNo,
	setTrue,
	validateYesNo};

struct SettingItem autoSaveSettings = {
	"Auto save settings after change",
	"autosavesettings",
	&consoleSettings.autoSaveSettings,
	ONOFF_INPUT_LENGTH,
	yesNo,
	setTrue,
	validateYesNo};

struct SettingItem *consoleSettingItemPointers[] =
	{
		&echoSerialInput,
		&autoSaveSettings};

struct SettingItemCollection consoleSettingItems = {
	"Serial console",
	"Serial console configuration",
	consoleSettingItemPointers,
	sizeof(consoleSettingItemPointers) / sizeof(struct SettingItem *)};

void doHelp(char *commandLine);

char *skipCommand(char *commandLine)
{
	while ((*commandLine != ' ') && (*commandLine != 0))
	{
		commandLine++;
	}

	if (*commandLine == ' ')
	{
		commandLine++;
	}

	return commandLine;
}

void doShowSettings(char *commandLine)
{
	char *filterStart = skipCommand(commandLine);

	if (*filterStart != 0)
	{
		PrintSomeSettings(filterStart);
	}
	else
	{
		PrintAllSettings();
	}
	alwaysDisplayMessage("\nSettings complete\n");
}

void doDumpSettings(char *commandLine)
{

	char *filterStart = skipCommand(commandLine);

	if (*filterStart != 0)
	{
		DumpSomeSettings(filterStart);
	}
	else
	{
		DumpAllSettings();
	}
	alwaysDisplayMessage("\nDump complete\n");
}

#define CONSOLE_MESSAGE_SIZE 800

char consoleMessageBuffer[CONSOLE_MESSAGE_SIZE];

void doStartWebServer(char *commandLine)
{
	internalReboot(CONFIG_HOST_BOOT_NO_TIMEOUT_MODE);
}

void doDumpStatus(char *commandLine)
{
	dumpSensorStatus();
	dumpProcessStatus();
	alwaysDisplayMessage("Heap: ");

#if defined(ARDUINO_ARCH_ESP8266)

	alwaysDisplayMessage("%ul",ESP.getFreeHeap());

#endif

#if defined(ARDUINO_ARCH_ESP32)
	alwaysDisplayMessage("%ul",ESP.getFreeHeap());
#endif

}

void doRestart(char *commandLine)
{
	alwaysDisplayMessage("done");
	delay(2000);
	saveSettings();
	internalReboot(COLD_BOOT_MODE);
}

void doClear(char *commandLine)
{
	LittleFS.format();
	resetSettings();
	saveSettings();
	internalReboot(COLD_BOOT_MODE);
}

void doSaveSettings(char *commandline)
{
	saveSettings();
	alwaysDisplayMessage("\nSettings saved");
}

void doTestButtonSensor(char *commandline)
{
	buttonSensorTest();
}

void doTestPIRSensor(char *commandline)
{
	pirSensorTest();
}

void doTestRotarySensor(char *commandline)
{
	rotarySensorTest();
}

void doTestPotSensor(char *commandline)
{
	potSensorTest();
}

void doDumpListeners(char *commandline)
{
	alwaysDisplayMessage("\nSensor Listeners\n");
	printControllerListeners();
}

void printCommandsJson(process *p)
{

	if (p->commands == NULL)
	{
		return;
	}

	struct CommandItemCollection *c = p->commands;

	if (c->noOfCommands > 0)
	{
		// have got some commands in the collection

		alwaysDisplayMessage("{\"name\":\"%s\",\"desc\":\"%s\",\n\"commands\":[", p->processName, c->description);

		for (int i = 0; i < c->noOfCommands; i++)
		{
			Command *com = c->commands[i];
			snprintf(consoleMessageBuffer, CONSOLE_MESSAGE_SIZE, "   ");
			appendCommandDescriptionToJson(com, consoleMessageBuffer, CONSOLE_MESSAGE_SIZE);
			alwaysDisplayMessage(consoleMessageBuffer);
			if(i < (c->noOfCommands-1)){
				alwaysDisplayMessage(",");
			}
		}

		alwaysDisplayMessage("]}");
	}
}

void doShowRemoteCommandsJson(char *commandLine)
{
	struct process *procPtr = getAllProcessList();

	alwaysDisplayMessage("\n { \n\"processes\": [\n");

	while (procPtr != NULL)
	{
		if (procPtr->commands == NULL)
		{
			procPtr = procPtr->nextAllProcesses;
		}
		else 
		{
			printCommandsJson(procPtr);
			procPtr = procPtr->nextAllProcesses;
			if (procPtr != NULL)
			{
				alwaysDisplayMessage(",");
			}
		}
	}

	alwaysDisplayMessage("]}");
}

void printCommandsText(process *p)
{

	if (p->commands == NULL)
	{
		return;
	}

	struct CommandItemCollection *c = p->commands;

	if (c->noOfCommands > 0)
	{
		// have got some commands in the collection

		alwaysDisplayMessage("Process:%s commands:%s\n", p->processName, c->description);

		for (int i = 0; i < c->noOfCommands; i++)
		{
			Command *com = c->commands[i];
			consoleMessageBuffer[0] = 0;
			appendCommandDescriptionToText(com, consoleMessageBuffer, CONSOLE_MESSAGE_SIZE);
			alwaysDisplayMessage(consoleMessageBuffer);
		}
	}
}

void doShowRemoteCommandsText(char *commandLine)
{
	alwaysDisplayMessage("\n");
	iterateThroughAllProcesses(printCommandsText);
}

void appendSensorDescriptionToJson(sensor *s, char *buffer, int bufferSize)
{
	snprintf(buffer, bufferSize, "%s{\"name\":\"%s\",\"version\":\"%s\",\"triggers\":[",
			 buffer, s->sensorName, Version);

	for (int i = 0; i < s->noOfSensorListenerFunctions; i++)
	{
		if (i > 0)
		{
			snprintf(buffer, bufferSize, "%s,", buffer);
		}

		sensorEventBinder *binder = &s->sensorListenerFunctions[i];
		snprintf(buffer, bufferSize, "%s{\"name\":\"%s\"}", buffer,
				 binder->listenerName);
	}

	snprintf(buffer, bufferSize, "%s]}", buffer);
}

void printSensorTriggersJson(sensor *s)
{
	if (s->noOfSensorListenerFunctions == 0)
		return;

	snprintf(consoleMessageBuffer, CONSOLE_MESSAGE_SIZE, "Sensor: %s\n     ", s->sensorName);
	appendSensorDescriptionToJson(s, consoleMessageBuffer, CONSOLE_MESSAGE_SIZE);
	alwaysDisplayMessage("%s\n", consoleMessageBuffer);
}

void doShowSensorsJson(char *commandLine)
{
	alwaysDisplayMessage("\n");
	iterateThroughSensors(printSensorTriggersJson);
}

void appendSensorDescriptionToText(sensor *s, char *buffer, int bufferSize)
{
	snprintf(buffer, bufferSize, "%sSensor name %s\n",
			 buffer, s->sensorName);

	for (int i = 0; i < s->noOfSensorListenerFunctions; i++)
	{
		sensorEventBinder *binder = &s->sensorListenerFunctions[i];
		snprintf(buffer, bufferSize, "%s   trigger:%s\n", buffer,
				 binder->listenerName);
	}
}

void printSensorTriggersText(sensor *s)
{
	if (s->noOfSensorListenerFunctions == 0)
		return;

	consoleMessageBuffer[0] = 0;

	appendSensorDescriptionToText(s, consoleMessageBuffer, CONSOLE_MESSAGE_SIZE);
	alwaysDisplayMessage("%s\n", consoleMessageBuffer);
}

void doShowSensorsText(char *commandLine)
{
;
	iterateThroughSensors(printSensorTriggersText);
}

void act_onJson_message(const char *json, void (*deliverResult)(char *resultText));

void showRemoteCommandResult(char *resultText)
{
	alwaysDisplayMessage(resultText);
}

void performRemoteCommand(char *commandLine)
{
	act_onJson_message(commandLine, showRemoteCommandResult);
}

void doClearAllListeners(char *commandLine)
{
	alwaysDisplayMessage("\nClearing all the listeners\n");
	clearAllListeners();
}

void doClearSensorListeners(char *commandLine)
{
	char *sensorName = skipCommand(commandLine);

	if (clearSensorNameListeners(sensorName))
	{
		alwaysDisplayMessage("Sensor listeners cleared");
	}
	else
	{
		alwaysDisplayMessage("Sensor not found");
	}
}

void doDumpStorage(char *commandLine)
{
	PrintStorage();
}

void doOTAUpdate(char *commandLine)
{
	if (needWifiConfigBootMode())
	{
		alwaysDisplayMessage("Can't perform OTA update as no WiFi networks have been configured");
		return;
	}

	requestOTAUpate();
}

void doColourDisplay(char *commandLine)
{
	alwaysDisplayMessage("Press space to step through each colour");
	alwaysDisplayMessage("Press the ESC key to exit");

	for (int i = 0; i < noOfColours; i++)
	{
		while (Serial.available())
		{
			int ch = Serial.read();
			if (ch == ESC_KEY)
			{
				alwaysDisplayMessage("Colour display ended");
				return;
			}
		}
		alwaysDisplayMessage("Colour:%s\n", colourNames[i].name);
		frame->fadeToColour(colourNames[i].col, 5);
		do
		{
			pixelProcess.udpateProcess();
			delay(20);
		} while (Serial.available() == 0);
	}
	alwaysDisplayMessage("Colour display finished");
}

void doHullOSHelp(char *commandLine)
{
	alwaysDisplayMessage("Hullos help");
}

void doHullOSRun(char *commandLine)
{
	alwaysDisplayMessage("Hullos run");
}

struct consoleCommand HullOSCommands[] =
	{
		{"help", "show all the commands", doHullOSHelp},
		{"run", "run the HullOS program ", doHullOSRun}};

void doHullOS(char *commandLine)
{
	char *hullosCommand = skipCommand(commandLine);

	performCommand(hullosCommand, HullOSCommands, sizeof(HullOSCommands) / sizeof(struct consoleCommand));
}

void doDumpSprites(char *commandLine)
{
	frame->dump();
}

void dumpFilesInStores()
{
	File dir = LittleFS.open("/", "r");

	while (true)
	{

		File storeDir = dir.openNextFile();

		if (!storeDir)
		{
			// no more files in the folder
			break;
		}

		if (storeDir.isDirectory())
		{
			// Only dump the contents of directories
			alwaysDisplayMessage(" Store:%s\n", storeDir.name());
			while (true)
			{
				File storeFile = storeDir.openNextFile();

				if (!storeFile)
				{
					break;
				}

				alwaysDisplayMessage("   Command:%s\n", storeFile.name());

				String line = storeFile.readStringUntil('\n');
				const char *lineChar = line.c_str();
				alwaysDisplayMessage("      %s\n", lineChar);
				storeFile.close();
			}
		}
	}
	dir.close();
}

void deleteFileInStore(char *deleteName)
{
	alwaysDisplayMessage("Deleting file:%s\n", deleteName);

	File dir = LittleFS.open("/", "r");

	char fullDeleteFileName[STORE_FILENAME_LENGTH];

	// set the delete filename to empty
	fullDeleteFileName[0] = 0;

	// spin until we have a file to delete
	while (fullDeleteFileName[0] == 0)
	{
		File storeDir = dir.openNextFile();

		if (!storeDir)
		{
			// no more files in the folder
			break;
		}

		if (storeDir.isDirectory())
		{
			const char *storeName = storeDir.name();

			alwaysDisplayMessage("   Searching store:%s\n", storeName);

			while (fullDeleteFileName[0] == 0)
			{
				File storeFile = storeDir.openNextFile();

				if (!storeFile)
				{
					break;
				}

				const char *filename = (const char *)storeFile.name();
				char compareFileName[STORE_FILENAME_LENGTH];

				// on the ESP8266 LittleFS the dir name function just delivers the name of the file in the folder(test)
				// on the ESP32 LittleFS it delivers the file path to the file (\start\test)

#if defined(ARDUINO_ARCH_ESP32)
				buildStoreFilename(compareFileName, STORE_FILENAME_LENGTH, storeName, deleteName);
#endif

#if defined(ARDUINO_ARCH_ESP8266)
				strcpy(compareFileName, deleteName);
#endif

				alwaysDisplayMessage("       Checking:%s for %s\n", filename, compareFileName);

				if (strcasecmp(compareFileName, filename) == 0)
				{
#if defined(ARDUINO_ARCH_ESP32)
					strcpy(fullDeleteFileName, compareFileName);
#endif

#if defined(ARDUINO_ARCH_ESP8266)
					buildStoreFilename(fullDeleteFileName, STORE_FILENAME_LENGTH, storeName, filename);
#endif
				}

				storeFile.close();
			}
		}
	}

	dir.close();

	if (fullDeleteFileName[0] != 0)
	{
		alwaysDisplayMessage("\nRemoving:%s", fullDeleteFileName);
		if (LittleFS.remove(fullDeleteFileName))
		{
			alwaysDisplayMessage("\n   done\n");
		}
		else
		{
			alwaysDisplayMessage("\n   failed\n");
		};
	}
	else
	{
		alwaysDisplayMessage("\nFile:%s not found", deleteName);
	}
}

void doDumpStores(char *commandLine)
{
	alwaysDisplayMessage("\nStored commands\n");
	dumpFilesInStores();
	alwaysDisplayMessage("\nend of stored commands\n");
}

void doDeleteCommand(char *commandLine)
{
	char *filename = skipCommand(commandLine);
	deleteFileInStore(filename);
}

struct consoleCommand userCommands[] =
	{
		{"buttontest", "test the button sensor", doTestButtonSensor},
		{"clearalllisteners", "clear all the command listeners", doClearAllListeners},
		{"clear", "clear all settings and restart the device", doClear},
		{"clearsensorlisteners", "clear the command listeners for a sensor", doClearSensorListeners},
		{"colours", "step through all the colours", doColourDisplay},
		{"commands", "show all the remote commands", doShowRemoteCommandsText},
		{"commandsjson", "show all the remote commands in json", doShowRemoteCommandsJson},
		{"deletecommand", "delete the named command", doDeleteCommand},
		{"dump", "dump all the setting values", doDumpSettings},
		{"help", "show all the commands", doHelp},
		{"host", "start the configuration web host", doStartWebServer},
		{"hullos", "HullOS commands", doHullOS},
		{"listeners", "list the command listeners", doDumpListeners},
		{"help", "show all the commands", doHelp},
		{"otaupdate", "start an over-the-air firmware update", doOTAUpdate},
		{"pirtest", "test the PIR sensor", doTestPIRSensor},
		{"pottest", "test the pot sensor", doTestPotSensor},
		{"rotarytest", "test the rotary sensor", doTestRotarySensor},
		{"restart", "restart the device", doRestart},
		{"save", "save all the setting values", doSaveSettings},
		{"sensors", "list all the sensor triggers", doShowSensorsText},
		{"sensorsjson", "list all the sensor triggers in json", doShowSensorsJson},
		{"settings", "show all the setting values", doShowSettings},
		{"sprites", "dump sprite data", doDumpSprites},
		{"status", "show the sensor status", doDumpStatus},
		{"stores", "dump all the command stores", doDumpStores},
		{"storage", "show the storage use of sensors and processes", doDumpStorage},
};

void doHelp(char *commandLine)
{
	alwaysDisplayMessage("\n\nConnected Little Boxes\n Device Version %s\n\nThese are all the available commands.\n\n",
				  Version);

	int noOfCommands = sizeof(userCommands) / sizeof(struct consoleCommand);

	for (int i = 0; i < noOfCommands; i++)
	{
		alwaysDisplayMessage("    %s - %s\n", userCommands[i].name, userCommands[i].commandDescription);
	}

	alwaysDisplayMessage("\nYou can view the value of any setting just by typing the setting name, for example:\n\n"
				  "    mqttdevicename\n\n"
				  "- would show you the MQTT device name.\n"
				  "You can assign a new value to a setting, for example:\n\n"
				  "     mqttdevicename=Rob\n\n"
				  "- would set the name of the mqttdevicename to Rob.\n\n"
				  "To see a list of all the setting names use the command settings.\n"
				  "This displays all the settings, their values and names.\n"
				  "To see a dump of settings (which can be restored to the device later) use dump.\n"
				  "The dump and settings can be followed by a filter string to match setting names\n\n"
				  "   dump pix\n\n"
				  "- would dump all the settings that contain the string pix\n\n"
				  "If you enter a JSON string this will be interpreted as a remote command.\n"
				  "See the remote command documentation for more details of this.\n");
}

boolean findCommandName(consoleCommand *com, char *name)
{
	int commandNameLength = strlen(com->name);

	for (int i = 0; i < commandNameLength; i++)
	{
		if (tolower(name[i]) != tolower(com->name[i]))
			return false;
	}

	// reached the end of the name, character that follows should be zero (end of the string)
	// or a space delimiter to the next part of the command

	if (name[commandNameLength] == 0)
		return true;

	if (name[commandNameLength] == ' ')
		return true;

	return false;
}

struct consoleCommand *findCommand(char *commandLine, consoleCommand *commands, int noOfCommands)
{
	for (int i = 0; i < noOfCommands; i++)
	{
		if (findCommandName(&commands[i], commandLine))
		{
			return &commands[i];
		}
	}
	return NULL;
}

boolean performCommand(char *commandLine, consoleCommand *commands, int noOfCommands)
{
	alwaysDisplayMessage("Got command: %s\n", commandLine);

	if (commandLine[0] == '{')
	{
		// treat the command as JSON
		performRemoteCommand(commandLine);
		return true;
	}

	// Look for a command with that name

	consoleCommand *comm = findCommand(commandLine, commands, noOfCommands);

	if (comm != NULL)
	{
		comm->actOnCommand(commandLine);
		return true;
	}

	// Look for a setting with that name

	processSettingCommandResult result;

	result = processSettingCommand(commandLine);

	switch (result)
	{
	case displayedOK:
		alwaysDisplayMessage("setting displayed OK");
		return true;
	case setOK:
		alwaysDisplayMessage("setting set OK");
		if (consoleSettings.autoSaveSettings)
		{
			saveSettings();
		}
		return true;
	case settingNotFound:
		alwaysDisplayMessage("setting not found");
		return false;
	case settingValueInvalid:
		alwaysDisplayMessage("setting value invalid");
		return false;
	}

	return false;
}

void showHelp()
{
	doHelp("help");
}

#define SERIAL_BUFFER_LIMIT SERIAL_BUFFER_SIZE - 1

char serialReceiveBuffer[SERIAL_BUFFER_SIZE];

int serialReceiveBufferPos = 0;

void reset_serial_buffer()
{
	serialReceiveBufferPos = 0;
}

void actOnSerialCommand(char * buffer)
{
	performCommand(buffer, userCommands, sizeof(userCommands) / sizeof(struct consoleCommand));
}

#define BACKSPACE_CHAR 0x08

void bufferSerialChar(char ch)
{
	if (consoleSettings.echoInput)
	{
		Serial.print(ch);
		if (ch == BACKSPACE_CHAR)
		{
			if (serialReceiveBufferPos > 0)
			{
				serialReceiveBufferPos--;
				Serial.print(' ');
				Serial.print(ch);
			}
			return;
		}
	}

	if (ch == '\n' || ch == '\r' || ch == 0)
	{
		if (serialReceiveBufferPos > 0)
		{
			serialReceiveBuffer[serialReceiveBufferPos] = 0;
			alwaysDisplayMessage("\n\r");
			actOnSerialCommand(serialReceiveBuffer);
			reset_serial_buffer();
		}
		return;
	}

	if (serialReceiveBufferPos < SERIAL_BUFFER_SIZE)
	{
		serialReceiveBuffer[serialReceiveBufferPos] = ch;
		serialReceiveBufferPos++;
	}
}

void checkSerialBuffer()
{
	// console is disabled when the robot is connected

	if(robotProcess.status == ROBOT_CONNECTED){
		return;
	}

	while (Serial.available())
	{
		bufferSerialChar(Serial.read());
	}
}

void initConsole()
{
	consoleProcessDescriptor.status = CONSOLE_OFF;
}

void startConsole()
{
	consoleProcessDescriptor.status = CONSOLE_OK;
}

void updateConsole()
{
	if (consoleProcessDescriptor.status == CONSOLE_OK)
	{
		checkSerialBuffer();
	}
}

void stopConsole()
{
	consoleProcessDescriptor.status = CONSOLE_OFF;
}

bool consoleStatusOK()
{
	return consoleProcessDescriptor.status == CONSOLE_OK;
}

void consoleStatusMessage(char *buffer, int bufferLength)
{
	switch (consoleProcessDescriptor.status)
	{
	case CONSOLE_OK:
		snprintf(buffer, bufferLength, "Console OK");
		break;
	case CONSOLE_OFF:
		snprintf(buffer, bufferLength, "Console OFF");
		break;
	default:
		snprintf(buffer, bufferLength, "Console status invalid");
		break;
	}
}

boolean validateConsoleCommandString(void *dest, const char *newValueStr)
{
	alwaysDisplayMessage("  Validate console command %s\n", newValueStr);
	return (validateString((char *)dest, newValueStr, CONSOLE_COMMAND_SIZE));
}

/************************************************************************
 *
 * Command offsets */

#define CONSOLE_FLOAT_VALUE_OFFSET 0
#define CONSOLE_REPORT_TEXT_OFFSET (CONSOLE_FLOAT_VALUE_OFFSET + sizeof(float))
#define CONSOLE_JSON_ATTR_OFFSET (CONSOLE_REPORT_TEXT_OFFSET + CONSOLE_MAX_MESSAGE_LENGTH)
#define CONSOLE_PRE_TEXT_OFFSET (CONSOLE_JSON_ATTR_OFFSET + CONSOLE_MAX_OPTION_LENGTH)
#define CONSOLE_POST_TEXT_OFFSET (CONSOLE_PRE_TEXT_OFFSET + CONSOLE_PRE_MESSAGE_LENGTH)
#define CONSOLE_COMMAND_OFFSET (CONSOLE_REPORT_TEXT_OFFSET)

/************************************************************************
 *
 * Command option validation */

boolean validateConsoleOptionString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, CONSOLE_MAX_OPTION_LENGTH));
}

boolean validateConsoleReportString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, CONSOLE_MAX_MESSAGE_LENGTH));
}

boolean validateConsolePreString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, CONSOLE_PRE_MESSAGE_LENGTH));
}

boolean validateConsolePostString(void *dest, const char *newValueStr)
{
	return (validateString((char *)dest, newValueStr, CONSOLE_POST_MESSAGE_LENGTH));
}

/************************************************************************
 *
 * Command items */

struct CommandItem ConsolefloatValueItem = {
	"value",
	"value",
	CONSOLE_FLOAT_VALUE_OFFSET,
	floatCommand,
	validateFloat,
	noDefaultAvailable};

struct CommandItem ConsoleReportText = {
	"text",
	"console message text",
	CONSOLE_REPORT_TEXT_OFFSET,
	textCommand,
	validateConsoleReportString,
	noDefaultAvailable};

struct CommandItem JSONAttribute = {
	"attr",
	"json attribute for value",
	CONSOLE_JSON_ATTR_OFFSET,
	textCommand,
	validateConsoleOptionString,
	noDefaultAvailable};

struct CommandItem ConsolePreText = {
	"pre",
	"console pre text",
	CONSOLE_PRE_TEXT_OFFSET,
	textCommand,
	validateConsolePreString,
	setDefaultEmptyString};

struct CommandItem ConsolePostText = {
	"post",
	"console post text",
	CONSOLE_POST_TEXT_OFFSET,
	textCommand,
	validateConsolePostString,
	setDefaultEmptyString};

/************************************************************************
 *
 * Display message command */

struct CommandItem *ConsolealwaysDisplayMessageCommandItems[] =
	{
		&ConsoleReportText,
		&ConsolePreText,
		&ConsolePostText};

int doSetConsoleReport(char *destination, unsigned char *settingBase);

struct Command consoleReportText
{
	"reporttext",
		"Reports a text message",
		ConsolealwaysDisplayMessageCommandItems,
		sizeof(ConsolealwaysDisplayMessageCommandItems) / sizeof(struct CommandItem *),
		doSetConsoleReport
};

int doSetConsoleReport(char *destination, unsigned char *settingBase)
{
	TRACELOGLN("Doing console text report");

	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("console", &consoleReportText, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	char *message = (char *)(settingBase + CONSOLE_REPORT_TEXT_OFFSET);
	char *post = (char *)(settingBase + CONSOLE_POST_TEXT_OFFSET);
	char *pre = (char *)(settingBase + CONSOLE_PRE_TEXT_OFFSET);

	char buffer[MAX_MESSAGE_LENGTH];

	snprintf(buffer, MAX_MESSAGE_LENGTH, "%s%s%s", pre, message, post);

	alwaysDisplayMessage(buffer);

	TRACELOGLN("Done console display");
	return WORKED_OK;
}

/************************************************************************
 *
 * Display JSON value command */

struct CommandItem *ConsoleJSONsendValueCommandItems[] =
	{
		&ConsoleReportText,
		&JSONAttribute};

int doShowJSONvalue(char *destination, unsigned char *settingBase);

struct Command consoleReportJSONvalue
{
	"reportjson",
		"report a message as a JSON object",
		ConsoleJSONsendValueCommandItems,
		sizeof(ConsoleJSONsendValueCommandItems) / sizeof(struct CommandItem *),
		doShowJSONvalue
};

int doShowJSONvalue(char *destination, unsigned char *settingBase)
{
	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("console", &consoleReportJSONvalue, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	char *message = (char *)(settingBase + CONSOLE_REPORT_TEXT_OFFSET);
	char *attributeName = (char *)(settingBase + CONSOLE_JSON_ATTR_OFFSET);

	char buffer[MAX_MESSAGE_LENGTH];

	if (message[0] == '{')
	{
		// the message is already formatted as JSON, just send it
		snprintf(buffer, MAX_MESSAGE_LENGTH, "{\"%s\":%s}", attributeName, message);
	}
	else
	{
		// single value - wrap in quotes
		snprintf(buffer, MAX_MESSAGE_LENGTH, "{\"%s\":\"%s\"}", attributeName, message);
	}

	alwaysDisplayMessage(buffer);

	return WORKED_OK;
}

struct CommandItem consoleCommandName = {
	"cmd",
	"console command text",
	CONSOLE_COMMAND_OFFSET,
	textCommand,
	validateConsoleCommandString,
	noDefaultAvailable};

struct CommandItem *consoleCommandItems[] =
	{
		&consoleCommandName};

int doRemoteConsoleCommand(char *destination, unsigned char *settingBase);

struct Command performConsoleCommand
{
	"remote",
		"Perform a remote console command",
		consoleCommandItems,
		sizeof(consoleCommandItems) / sizeof(struct CommandItem *),
		doRemoteConsoleCommand
};

int doRemoteConsoleCommand(char *destination, unsigned char *settingBase)
{
	if (*destination != 0)
	{
		// we have a destination for the command. Build the string
		char buffer[JSON_BUFFER_SIZE];
		createJSONfromSettings("console", &performConsoleCommand, destination, settingBase, buffer, JSON_BUFFER_SIZE);
		return publishCommandToRemoteDevice(buffer, destination);
	}

	char *command = (char *)(settingBase + CONSOLE_COMMAND_OFFSET);

	alwaysDisplayMessage("Performing remote command: %s\n", command);

	if (performCommand(command, userCommands, sizeof(userCommands) / sizeof(struct consoleCommand)))
	{
		return WORKED_OK;
	}

	return JSON_MESSAGE_INVALID_CONSOLE_COMMAND;
}

struct Command *consoleCommandList[] = {
	&consoleReportText,
	&consoleReportJSONvalue,
	&performConsoleCommand};

struct CommandItemCollection consoleCommands =
	{
		"Perform console commands on a remote device",
		consoleCommandList,
		sizeof(consoleCommandList) / sizeof(struct Command *)};

struct process consoleProcessDescriptor = {
	"console",
	initConsole,
	startConsole,
	updateConsole,
	stopConsole,
	consoleStatusOK,
	consoleStatusMessage,
	false,
	0,
	0,
	0,
	NULL,
	(unsigned char *)&consoleSettings, sizeof(consoleSettings), &consoleSettingItems,
	&consoleCommands,
	BOOT_PROCESS + ACTIVE_PROCESS + CONFIG_PROCESS + WIFI_CONFIG_PROCESS,
	NULL,
	NULL,
	NULL};
