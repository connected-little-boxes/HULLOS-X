#if defined(SETTINGS_WEB_SERVER)

#include <Arduino.h>
#include <strings.h>
#include "utils.h"

#include "settingsWebServer.h"
#include "settings.h"
#include "sensors.h"
#include "processes.h"
#include "boot.h"
#include "otaupdate.h"
#include "pixels.h"
#include "statusled.h"
#include "console.h"

#if defined(ARDUINO_ARCH_ESP32)

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#ifndef FS_WEBSERVER_H
#define FS_WEBSERVER_H

#include <WebServer.h>
#include <ESPmDNS.h>

class fs_WebServer : public WebServer
{
public:
  fs_WebServer(int port = 80) : WebServer(port)
  {
  }
  using WebServer::send_P;
  void send_P(int code, PGM_P content_type, PGM_P content)
  {
    size_t contentLength = 0;

    if (content != NULL)
    {
      contentLength = strlen_P(content);
    }

    String header;
    char type[64];
    memccpy_P((void *)type, (PGM_VOID_P)content_type, 0, sizeof(type));
    _prepareHeader(header, code, (const char *)type, contentLength);
    _currentClientWrite(header.c_str(), header.length());
    if (contentLength)
    { // if rajouté par FS ...........................+++++
      sendContent_P(content);
    }
  }

  bool chunkedResponseModeStart_P(int code, PGM_P content_type)
  {
    if (_currentVersion == 0)
      // no chunk mode in HTTP/1.0
      return false;
    setContentLength(CONTENT_LENGTH_UNKNOWN);
    send_P(code, content_type, "");
    return true;
  }
  bool chunkedResponseModeStart(int code, const char *content_type)
  {
    return chunkedResponseModeStart_P(code, content_type);
  }
  bool chunkedResponseModeStart(int code, const String &content_type)
  {
    return chunkedResponseModeStart_P(code, content_type.c_str());
  }
  void chunkedResponseFinalize()
  {
    sendContent(emptyString);
  }
};
#endif // FS_WEBSERVER_H

fs_WebServer *server;

#endif

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266Webserver.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
ESP8266WebServer *server;
#endif

void sendMasterHeader()
{
  server->sendContent(F(R"HEADER(
<html>
<head>
<title>Connected Little Boxes</title>
<style>
	[type='radio'] {
		height:2vw; 
		width:2vw;
	}

	input {
		margin: 5px auto;
		font-size: 3vw;
		font-family: Arial, Helvetica, sans-serif;
	}

	p {
		font-size: 3vw;
		font-family: Arial, Helvetica, sans-serif;
	}

	a {
		font-size: 3vw;
		font-family: Arial, Helvetica, sans-serif;
	}

	label {
		font-size: 3vw;
		font-family: Arial, Helvetica, sans-serif;
	}

	input {
		font-size: 3vw;
		font-family: Arial, Helvetica, sans-serif;
	}

	h1 {
		font-size: 5vw;
		font-family: Arial, Helvetica, sans-serif;
	}

	h2 {
		font-size: 4vw;
		font-family: Arial, Helvetica, sans-serif;
	}
</style>
</header>
<body>

)HEADER"));
}

void sendMasterFooter()
{
  server->sendContent(F(R"FOOTER(
</body>
</html>
)FOOTER"));
}

const char *settingsPageHeader =
    "<h1>Settings</h1>"
    "<h2>%s</h2>"                                  // configuration description goes here
    "<form id='form' action='/%s' method='post'>"; // configuration short name goes here

const char *homePageHeader = R"HEADER(
<h1>Connected Little Boxes</h1>
<h2>%s</h2> <!--  configuration description goes here -->
<form id='form' action='%s' method='post'> <!-- configuration short name goes here -->
)HEADER";

const char *allSettingsPageHeader = R"HEADER(
<h1>Connected Little Boxes</h1>
<h2>All Settings</h2> 
<ul>
)HEADER";

const char *allSettingsPageFooter = R"FOOTER(
</ul>
<p>
Select the settings page you want to work with. 
</p>
<p><a href = "/">Return to the settings home screen </a></p>
)FOOTER";

const char *homePageFooter PROGMEM =
    "<p>"
    "<input type='submit' value='Update'>"
    "</p>"
    "</form>"
    "<p>Enter your settings and select Update to write them into the device.</p>"
    "<p>Select the full settings page link below to view all the settings in the device.</p>"
    "<a href=full>full settings</a>"
    "<p>Select the reset link below to reset the device when you have finished.</p>"
    "<a href=reset> reset </a>";

const char settingsPageFooter[] =
    "<input type='submit' value='Update'>"
    "</form>";

const char sensorResetMessage[] =
    "<h1>Connected Little Boxes</h1>"
    "<h2>Reset</h2>"
    "<p>Your device will reset in a few seconds.</p>";

extern struct SettingItem wifi1SSIDSetting;
extern struct SettingItem wifi1PWDSetting;
extern struct SettingItem wifiOnOff;
extern struct SettingItem mqttServerSetting;
extern struct SettingItem mqttUserSetting;
extern struct SettingItem mqttPasswordSetting;
extern struct SettingItem mqttPublishTopicSetting;
extern struct SettingItem mqttSubscribeTopicSetting;
extern struct SettingItem pixelNoOfXPixelsSetting;
extern struct SettingItem pixelNoOfYPixelsSetting;

#define SETTING_BUFFER_SIZE 400

char settingBuffer[SETTING_BUFFER_SIZE];

const char replyPageHeader[] =
    "<html>"
    "<head>"
    "<style>input {margin: 5px auto; } </style>"
    "</head>"
    "<body>"
    "<h1>%s</h1>"
    "<h2>%s</h2>"; // configuration description goes here

const char replyPageFooter[] =
    "<p>Settings updated.</p>"
    "<p><a href = \"/ \">return to the settings home screen </a></p>";

void updateSettings(SettingItemCollection *settingCollection)
{
  char settingBuffer[SETTING_BUFFER_SIZE];

  server->chunkedResponseModeStart(200, F("text/html"));

  sendMasterHeader();

  snprintf(settingBuffer, SETTING_BUFFER_SIZE, replyPageHeader,
           settingCollection->collectionDescription,
           settingCollection->collectionName);

  server->sendContent(settingBuffer);

  for (int i = 0; i < settingCollection->noOfSettings; i++)
  {
    String fName = String(settingCollection->settings[i]->formName);
    String argValue = server->arg(fName);

    if (settingCollection->settings[i]->validateValue(
            settingCollection->settings[i]->value,
            argValue.c_str()))
    {
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<p>Set %s</p> ",
               settingCollection->settings[i]->prompt);
    }
    else
    {
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<p>Invalid value %s for %s</p> ",
               argValue.c_str(), settingCollection->settings[i]->prompt);
    }
    server->sendContent(settingBuffer);
  }

  saveSettings();

  server->sendContent(replyPageFooter);

  sendMasterFooter();

  server->chunkedResponseFinalize();
}

void sendPageText(const char *text)
{
  server->chunkedResponseModeStart(200, F("text/html"));

  sendMasterHeader();

  server->sendContent(text);

  sendMasterFooter();

  server->chunkedResponseFinalize();
}

void sendCollectionSettingsPage(SettingItemCollection *settingCollection, const char *header, const char *footer)
{
  int *intPointer;
  float *floatPointer;
  double *doublePointer;
  boolean *boolPointer;
  uint32_t *loraIDValuePointer;
  char loraKeyBuffer[LORA_KEY_LENGTH * 2 + 1];

  char *checked = "checked";
  char *empty = "";
  char *yesChecked;
  char *noChecked;

  char settingBuffer[SETTING_BUFFER_SIZE];

  server->chunkedResponseModeStart(200, F("text/html"));

  sendMasterHeader();

  snprintf(settingBuffer, SETTING_BUFFER_SIZE, header,
           settingCollection->collectionDescription,
           settingCollection->collectionName);

  server->sendContent(settingBuffer);

  // Start the search at setting collection 0 so that the quick settings are used to build the web page
  for (int i = 0; i < settingCollection->noOfSettings; i++)
  {
    char *formName = settingCollection->settings[i]->formName;
    char *prompt = settingCollection->settings[i]->prompt;
    void *valueRef = settingCollection->settings[i]->value;

    switch (settingCollection->settings[i]->settingType)
    {
    case text:
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%s'>%s</label> ",
               formName, prompt);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name='%s' id='%s' type ='text' value='%s' style='margin-left: 20px; line-height: 50%%'><br>",
               formName, formName, (char *)valueRef);
      server->sendContent(settingBuffer);
      break;
    case password:
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%s'>%s</label> ",
               formName, prompt);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name='%s' id='%s' type='password' value='%s' style='margin-left: 20px; line-height: 50%%'><br>",
               formName, formName, (char *)valueRef);
      server->sendContent(settingBuffer);
      break;
    case integerValue:
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%s'>%s</label> ",
               formName, prompt);
      server->sendContent(settingBuffer);
      intPointer = (int *)valueRef;
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name='%s' id='%s' type='text' value='%d' style='margin-left: 20px; line-height: 50%%''><br>",
               formName, formName, *intPointer);
      server->sendContent(settingBuffer);
      break;
    case floatValue:
      floatPointer = (float *)valueRef;
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%s'>%s</label> ",
               formName, prompt);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name='%s' id='%s' type='text' value='%f' style='margin-left: 20px; line-height: 50%%'><br>",
               formName, formName, *floatPointer);
      server->sendContent(settingBuffer);
      break;
    case doubleValue:
      doublePointer = (double *)valueRef;
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%s'>%s</label> ",
               formName, prompt);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name='%s' id='%s' type='text' value='%lf' style='margin-left: 20px; line-height: 50%%'><br>",
               formName, formName, *doublePointer);
      server->sendContent(settingBuffer);
      break;
    case yesNo:
      doublePointer = (double *)valueRef;
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%s'>%s</label> ",
               formName, prompt);
      server->sendContent(settingBuffer);
      boolPointer = (boolean *)valueRef;
      if (*boolPointer)
      {
        yesChecked = checked;
        noChecked = empty;
      }
      else
      {
        yesChecked = empty;
        noChecked = checked;
      }
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name = '%s' id = '%syes' type = 'radio' value='yes' style='margin-left:40px' %s>",
               formName, formName, yesChecked);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%syes'>Yes</label>",
               formName);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name = '%s' id = '%sno' type = 'radio' value='no' style='margin-left:40px' %s>",
               formName, formName, noChecked);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%sno'>No</label><br>",
               formName);
      server->sendContent(settingBuffer);
      break;

    case loraKey:
      dumpHexString(loraKeyBuffer, (uint8_t *)valueRef, LORA_KEY_LENGTH);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%s'>%s</label> ",
               formName, prompt);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name='%s' id='%s' type='text' value='%s'><br>",
               formName, formName, loraKeyBuffer);
      server->sendContent(settingBuffer);
      break;

    case loraID:
      loraIDValuePointer = (uint32_t *)valueRef;
      dumpUnsignedLong(loraKeyBuffer, *loraIDValuePointer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<label for='%s'>%s</label> ",
               formName, prompt);
      server->sendContent(settingBuffer);
      snprintf(settingBuffer, SETTING_BUFFER_SIZE, "<input name='%s' id='%s' type='text' value='%s'><br>",
               formName, formName, loraKeyBuffer);
      server->sendContent(settingBuffer);
      break;
    }
  }

  server->sendContent(footer);

  sendMasterFooter();

  server->chunkedResponseFinalize();
}

void addItem(SettingItemCollection *settings)
{
  // messageLogf("    %s %s\n", settings->collectionName, settings->collectionDescription);
  char settingBuffer[SETTING_BUFFER_SIZE];
  snprintf(settingBuffer, SETTING_BUFFER_SIZE,
           "<li><a href='%s'>%s</a></li>\n",
           settings->collectionName,
           settings->collectionDescription);
  server->sendContent(settingBuffer);
}

void sendFullSettingsHomePage()
{
  server->chunkedResponseModeStart(200, F("text/html"));
  sendMasterHeader();
  server->sendContent(allSettingsPageHeader);
  iterateThroughProcessSettingCollections(addItem);
  iterateThroughSensorSettingCollections(addItem);
  server->sendContent(allSettingsPageFooter);
  sendMasterFooter();
  server->chunkedResponseFinalize();
}

struct SettingItem *quickSettingPointers[] = {

    &wifi1SSIDSetting,
    &wifi1PWDSetting,
    &wifiOnOff,
    &pixelNoOfXPixelsSetting,
    &pixelNoOfYPixelsSetting};

struct SettingItemCollection QuickSettingItems = {
    "Quick Settings",
    "Quick Settings",
    quickSettingPointers,
    sizeof(quickSettingPointers) / sizeof(struct SettingItem *)};

String bigChunk;

bool timeoutActive;

void handleRoot()
{
  timeoutActive = false;
  switch (server->method())
  {
  case HTTP_GET:
    sendCollectionSettingsPage(&QuickSettingItems, homePageHeader, homePageFooter);
    break;
  case HTTP_POST:
    break;
  }
}

void handleNotFound()
{
  timeoutActive = false;
  char settingBuffer[SETTING_BUFFER_SIZE];

  String uriString = server->uri();
  uriString.remove(0, 1);
  uriString.replace("%20", " "); // put the spaces back
  uriString.toCharArray(settingBuffer, SETTING_BUFFER_SIZE);

  if (strcasecmp(settingBuffer, "reset") == 0)
  {
    displayMessage("Resetting device");
    sendPageText(sensorResetMessage);
    // reset the device
    delay(5000);
    internalReboot(WARM_BOOT_MODE);
  }

  if (strcasecmp(settingBuffer, "full") == 0)
  {
    sendFullSettingsHomePage();
    return;
  }

  // the url might contain the name of a setting collection which is to
  // be displayed. Get the name out of the url and check it.

  SettingItemCollection *items = NULL;

  // The quick setting collection needs to be handled specially

  if (strcasecmp("Quick Settings", settingBuffer) == 0)
  {
    // messageLogf("  Got the Quick Settings");
    items = &QuickSettingItems;
  }
  else
  {
    // Search setting collections for sensors and processes
    items = findSettingItemCollectionByName(settingBuffer);
  }

  if (items != NULL)
  {
    // The url contains the name of a setting collection
    if (server->method() == HTTP_GET)
    {
      // respond with a settings form
      sendCollectionSettingsPage(items, settingsPageHeader, settingsPageFooter);
    }
    else
    {
      // update the settings from the POST
      updateSettings(items);
    }
  }
  else
  {
    // not setting any items - send diagnostic page
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server->uri();
    message += "\nMethod: ";
    message += (server->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server->args();
    message += "\n";
    for (uint8_t i = 0; i < server->args(); i++)
    {
      message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
    }
    server->send(404, "text/plain", message);
  }
}

void startHostingConfigWebsite(bool timeout)
{

  unsigned long startTime = millis();
  unsigned long endTime = startTime + 1000 * SETTINGS_WEBSITE_TIMEOUT_SECS;

  timeoutActive = timeout;

  displayMessage("Hosting the configuration website\nPress any key to reboot\n");

  beginStatusDisplay(VERY_DARK_GREY_COLOUR);

	#if defined(ARDUINO_ARCH_ESP32)

	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector 

	#endif

  WiFi.mode(WIFI_AP);

#if defined(ARDUINO_ARCH_ESP32)
  server = new fs_WebServer(80);
  if (MDNS.begin(SETTINGS_MDNS_NAME))
  {
    displayMessage("MDNS responder started");
  }
#endif

#if defined(ARDUINO_ARCH_ESP8266)
  server = new ESP8266WebServer(80);
  if (MDNS.begin(SETTINGS_MDNS_NAME))
  {
    displayMessage("MDNS responder started\n");
  }
#endif

  delay(100);

  WiFi.softAP(SETTINGS_ACCESS_POINT_SSID);

  delay(500);

  server->on("/", handleRoot);

  server->on("/inline", []()
             { server->send(200, "text/plain", "this works as well"); });

  server->onNotFound(handleNotFound);

  server->begin();
  displayMessage("HTTP server started");

  initStatusLedHardware();
  statusLedOn();
  consoleProcessDescriptor.initProcess();
  consoleProcessDescriptor.startProcess();

  while (true)
  {
    unsigned long time = millis();
    if (timeoutActive && time > endTime)
    {
      displayMessage("Rebooting after timeout ");
      internalReboot(WARM_BOOT_MODE);
    }

    server->handleClient();
    
    consoleProcessDescriptor.udpateProcess();

#if defined(ARDUINO_ARCH_ESP8266)
    MDNS.update();
#endif
  }
}

#endif