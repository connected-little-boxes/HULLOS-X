#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "LittleFS.h"

#include "debug.h"
#include "utils.h"

#if defined(ARDUINO_ARCH_PICO)

#include "settings.h"
#include "pixels.h"
#include "processes.h"
#include "sensors.h"
#include "controller.h"
#include "statusled.h"
#include "messages.h"
#include "console.h"
#include "connectwifi.h"
#include "mqtt.h"
#include "clock.h"
#include "registration.h"
#include "boot.h" 
#include "robotProcess.h"

#else

#include "settings.h"
#include "pixels.h"
#include "processes.h"
#include "sensors.h"
#include "pirSensor.h"
#include "buttonsensor.h"
#include "inputswitch.h"
#include "controller.h"
#include "statusled.h"
#include "messages.h"
#include "console.h"
#include "connectwifi.h"
#include "mqtt.h"
#include "servoproc.h"
#include "clock.h"
#include "registration.h"
#include "rotarySensor.h"
#include "potSensor.h"
#include "settingsWebServer.h"
#include "MAX7219Messages.h"
#include "printer.h"
#include "BME280Sensor.h"
#include "HullOS.h"
#include "boot.h" 
#include "otaupdate.h"
#include "outpin.h"
#include "robotProcess.h"

#endif


// This function will be different for each build of the device.

void populateProcessList()
{
#if defined(ARDUINO_ARCH_PICO)
  addProcessToAllProcessList(&pixelProcess);
  addProcessToAllProcessList(&statusLedProcess);
  addProcessToAllProcessList(&messagesProcess);
  addProcessToAllProcessList(&consoleProcessDescriptor);
  addProcessToAllProcessList(&WiFiProcessDescriptor);
  addProcessToAllProcessList(&MQTTProcessDescriptor);
  addProcessToAllProcessList(&controllerProcess);
  addProcessToAllProcessList(&RegistrationProcess);
  addProcessToAllProcessList(&robotProcess);
#else
  addProcessToAllProcessList(&pixelProcess);
  addProcessToAllProcessList(&statusLedProcess);
  addProcessToAllProcessList(&inputSwitchProcess);
  addProcessToAllProcessList(&messagesProcess);
  addProcessToAllProcessList(&consoleProcessDescriptor);
  addProcessToAllProcessList(&WiFiProcessDescriptor);
  addProcessToAllProcessList(&MQTTProcessDescriptor);
  addProcessToAllProcessList(&controllerProcess);
  addProcessToAllProcessList(&ServoProcess);
  addProcessToAllProcessList(&RegistrationProcess);
  addProcessToAllProcessList(&max7219MessagesProcess);
  addProcessToAllProcessList(&printerProcess);
  addProcessToAllProcessList(&hullosProcess);
  addProcessToAllProcessList(&outPinProcess);
  addProcessToAllProcessList(&robotProcess);
#endif
}

void populateSensorList()
{
#if defined(ARDUINO_ARCH_PICO)
  addSensorToAllSensorsList(&clockSensor);
  addSensorToActiveSensorsList(&clockSensor);
#else
  addSensorToAllSensorsList(&pirSensor);
  addSensorToActiveSensorsList(&pirSensor);
  addSensorToAllSensorsList(&buttonSensor);
  addSensorToActiveSensorsList(&buttonSensor);
  addSensorToAllSensorsList(&clockSensor);
  addSensorToActiveSensorsList(&clockSensor);
  addSensorToAllSensorsList(&rotarySensor);
  addSensorToActiveSensorsList(&rotarySensor);
  addSensorToAllSensorsList(&potSensor);
  addSensorToActiveSensorsList(&potSensor);
  addSensorToAllSensorsList(&bme280Sensor);
  addSensorToActiveSensorsList(&bme280Sensor);
#endif
}

void displayControlMessage(int messageNumber, ledFlashBehaviour severity, char *messageText)
{
  char buffer[20];

  ledFlashBehaviourToString(severity, buffer, 20);

  displayMessage("%s: %d %s\n", buffer, messageNumber, messageText);
}

unsigned long heapPrintTime = 0;

void startDevice()
{
  Serial.begin(115200);

  delay(100);

  alwaysDisplayMessage("\n\n\n\n");

	char deviceNameBuffer[DEVICE_NAME_LENGTH];
	PrintSystemDetails(deviceNameBuffer, DEVICE_NAME_LENGTH);
	alwaysDisplayMessage("%s\n",deviceNameBuffer);
  alwaysDisplayMessage("Connected Little Boxes Device\n");
  alwaysDisplayMessage("Powered by HULLOS-X\n");
  alwaysDisplayMessage("www.connectedlittleboxes.com\n");
  alwaysDisplayMessage("Version %s build date: %s %s\n", Version, __DATE__, __TIME__);

  #if defined(ARDUINO_ARCH_ESP8266)
  // If we are using an ESP8266 to connect to a robot (or other serial device) we will
  // use the alternative serial port connection for this. This means when the robot is active
  // it will be impossible to interact with the serial terminal
  // this lets you interrupt the start process and retail the serial connection if you are using a terminal

  alwaysDisplayMessage("Press any key to force console mode.\n");

  delay(1000);

  if(Serial.available()){
    forceConsole = true;
    alwaysDisplayMessage("  forcing console mode\n");
  }
  else {
    forceConsole = false;
    alwaysDisplayMessage("  console mode not forced\n");

  }

#endif

#ifdef DEBUG
  messageLogf("**** Debug output enabled");
#endif

  START_MEMORY_MONITOR();

  populateProcessList();

  DISPLAY_MEMORY_MONITOR("Populate process list");

  populateSensorList();

  DISPLAY_MEMORY_MONITOR("Populate sensor list");

  PixelStatusLevels settingsStatus;

  SettingsSetupStatus status = setupSettings();

  switch (status)
  {
  case SETTINGS_SETUP_OK:
    settingsStatus = PIXEL_STATUS_OK;
    displayMessage("Settings loaded OK\n");
    break;
  case SETTINGS_RESET_TO_DEFAULTS:
    settingsStatus = PIXEL_STATUS_NOTIFICATION;
    displayMessage("Settings reset to defaults\n");
    break;
  case SETTINGS_FILE_SYSTEM_FAIL:
    settingsStatus = PIXEL_STATUS_WARNING;
    displayMessage("Settings file system fail\n");
    break;
  default:
    displayMessage("Invalid setupSettings return\n");
    settingsStatus = PIXEL_STATUS_ERROR;
  }

  // get the boot mode
  getBootMode();

  if (bootMode == COLD_BOOT_MODE)
  {
    // first power up
    if (needWifiConfigBootMode())
    {
      // start hosting the config website and request a timeout
      startHostingConfigWebsite(true);
    }
  }
  else
  {

    if (bootMode == CONFIG_HOST_BOOT_NO_TIMEOUT_MODE)
    {
      startHostingConfigWebsite(false);
    }

    if (bootMode == CONFIG_HOST_TIMEOUT_BOOT_MODE)
    {
      startHostingConfigWebsite(true);
    }

    if (bootMode == OTA_UPDATE_BOOT_MODE)
    {
      performOTAUpdate();
    }
  }

  startstatusLedFlash(1000);

  initialiseAllProcesses();

  DISPLAY_MEMORY_MONITOR("Initialise all processes");

  beginStatusDisplay(VERY_DARK_RED_COLOUR);

  addStatusItem(settingsStatus);
  renderStatusDisplay();

  buildActiveProcessListFromMask(BOOT_PROCESS);

  startProcesses();

  DISPLAY_MEMORY_MONITOR("Initialise all processes");

  bindMessageHandler(displayControlMessage);

  startSensors();

  DISPLAY_MEMORY_MONITOR("Start all sensors");

  addStatusItem(PIXEL_STATUS_OK);
  renderStatusDisplay();

  delay(1000); // show the status for a while
  if (messagesSettings.messagesEnabled)
  {
    displayMessage("Start complete\n\nType help and press enter for help\n\n");
  }
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
uint32_t oldHeap = 0;

#define HEAP_PRINT_INTERVAL 500

void heapMonitor()
{
  unsigned long m = millis();
  if ((m - heapPrintTime) > HEAP_PRINT_INTERVAL)
  {
    uint32_t newHeap = ESP.getFreeHeap();
    if (newHeap != oldHeap)
    {
      Serial.print("Heap: ");
      displayMessage("%lu",newHeap);
      oldHeap = newHeap;
    }
    heapPrintTime = millis();
  }
}
#endif


void setup()
{
  startDevice();
}

void loop()
{
  updateSensors();
  updateProcesses();
  delay(5);
  DISPLAY_MEMORY_MONITOR("System");
}