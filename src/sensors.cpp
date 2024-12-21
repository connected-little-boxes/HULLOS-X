#include <strings.h>

#include "debug.h"
#include "sensors.h"
#include "pixels.h"
#include "settings.h"
#include "controller.h"
#include "utils.h"
#include "messages.h"

struct sensor *activeSensorList = NULL;
struct sensor *allSensorList = NULL;
struct sensorListener * deletedSensorListeners = NULL;

void addSensorToAllSensorsList(struct sensor *newSensor)
{
	newSensor->nextAllSensors = NULL;

	if (allSensorList == NULL)
	{
		allSensorList = newSensor;
	}
	else
	{
		sensor *addPos = allSensorList;

		while (addPos->nextAllSensors != NULL)
		{
			addPos = addPos->nextAllSensors;
		}
		addPos->nextAllSensors = newSensor;
	}
}

void addListenerToDeletedListeners(struct sensorListener * listener)
{
	listener->nextMessageListener = deletedSensorListeners;
	deletedSensorListeners = listener;
}

void clearSensorListener(struct sensorListener * listener)
{
	listener->config=NULL;
	listener->lastReadingMillis = -1;
	listener->receiveMessage = NULL;
	listener->nextMessageListener = NULL;
}

struct sensorListener * getNewSensorListener()
{
	TRACELOGLN("Getting a sensor listener");

	sensorListener * result ;

	if(deletedSensorListeners == NULL)
	{
		TRACELOGLN("   creating a new listener");
		result = new sensorListener();
	}
	else {
		TRACELOGLN("   reusing a discarded listener");
		result = deletedSensorListeners;
		// remove this listener from the list
		deletedSensorListeners = deletedSensorListeners->nextMessageListener;
	}

	clearSensorListener(result);

	return result;
}


// Removes a listener from the sensor and adds the listner to the list of deleted listeners
// The listeners are recycled if used again

void removeMessageListenerFromSensor(struct sensor *sensor, struct sensorListener *listener)
{
	TRACELOGLN("Remove message listener from a sensor");

	sensorListener *nodeBeforeDel = sensor->listeners;

	while(nodeBeforeDel != NULL)
	{
		if(nodeBeforeDel->nextMessageListener == listener) 
		{
			break;
		}
		nodeBeforeDel = nodeBeforeDel->nextMessageListener;
	}

	if(nodeBeforeDel==NULL)
	{
		TRACELOGLN("Remove listener - listener not found");
		return;
	}

	// cut this listener out of the list
	nodeBeforeDel->nextMessageListener = listener->nextMessageListener;

	addListenerToDeletedListeners(listener);

	TRACELOG("   removing process:");
	TRACELOG(listener->config->commandProcess);
	TRACELOG("   removing command:");
	TRACELOGLN(listener->config->commandName);
}

void removeAllMessageListenersFromSensor(struct sensor *sensor)
{
	TRACELOG("Removing all message listeners from sensor:");
	TRACELOGLN(sensor->sensorName);

	sensorListener *node = sensor->listeners;
	// sping down the listeners and delete each one
	while(node != NULL)
	{
		TRACELOG("   removing process:");
		TRACELOG(node->config->commandProcess);
		TRACELOG("   removing command:");
		TRACELOGLN(node->config->commandName);
		// remember the node we are deleting
		sensorListener * nodeToDelete = node;
		// move on to the next node
		node = node->nextMessageListener;
		// delete the node we have just left
		addListenerToDeletedListeners(nodeToDelete);
	}

	// clear all the listeners from the sensor
	sensor->listeners = NULL;
}

void removeAllSensorMessageListeners()
{
	iterateThroughSensors(removeAllMessageListenersFromSensor);
}

void addMessageListenerToSensor(struct sensor *sensor, struct sensorListener *listener)
{
	listener->nextMessageListener = NULL;

	if (sensor->listeners == NULL)
	{
		sensor->listeners = listener;
	}
	else
	{
		sensorListener *addPos = sensor->listeners;

		while (addPos->nextMessageListener != NULL)
		{
			addPos = addPos->nextMessageListener;
		}
		addPos->nextMessageListener = listener;
	}
}

void addSensorToActiveSensorsList(struct sensor *newSensor)
{
	newSensor->nextActiveSensor = NULL;

	if (activeSensorList == NULL)
	{
		activeSensorList = newSensor;
	}
	else
	{
		sensor *addPos = activeSensorList;

		while (addPos->nextActiveSensor != NULL)
		{
			addPos = addPos->nextActiveSensor;
		}
		addPos->nextActiveSensor = newSensor;
	}
}

struct sensor *findSensorByName(const char *name)
{
	sensor *allSensorPtr = allSensorList;

	while (allSensorPtr != NULL)
	{
		if (strcasecmp(allSensorPtr->sensorName, name) == 0)
		{
			return allSensorPtr;
		}
		allSensorPtr = allSensorPtr->nextAllSensors;
	}
	return NULL;
}

struct sensor *findSensorSettingCollectionByName(const char *name)
{
	sensor *allSensorPtr = allSensorList;

	while (allSensorPtr != NULL)
	{
		if (allSensorPtr->settingItems == NULL)
			continue;

		if (strcasecmp(allSensorPtr->settingItems->collectionName, name) == 0)
		{
			return allSensorPtr;
		}
		allSensorPtr = allSensorPtr->nextAllSensors;
	}
	return NULL;
}

#define SENSOR_STATUS_BUFFER_SIZE 300

char sensorStatusBuffer[SENSOR_STATUS_BUFFER_SIZE];

#define SENSOR_VALUE_BUFFER_SIZE 300

char sensorValueBuffer[SENSOR_VALUE_BUFFER_SIZE];

void startSensors()
{
	alwaysDisplayMessage("Starting sensors\n");
	// start all the sensor managers

	sensor *activeSensorPtr = activeSensorList;

	while (activeSensorPtr != NULL)
	{
		alwaysDisplayMessage("   %s: ", activeSensorPtr->sensorName);
		activeSensorPtr->startSensor();
		activeSensorPtr->getStatusMessage(sensorStatusBuffer, SENSOR_STATUS_BUFFER_SIZE);
		alwaysDisplayMessage("%s\n", sensorStatusBuffer);
		activeSensorPtr->beingUpdated = true;
		activeSensorPtr = activeSensorPtr->nextAllSensors;
	}
}

void dumpSensorStatus()
{
	alwaysDisplayMessage("Sensors\n");
	unsigned long currentMillis = millis();

	sensor *activeSensorPtr = activeSensorList;

	while (activeSensorPtr != NULL)
	{
		activeSensorPtr->getStatusMessage(sensorStatusBuffer, SENSOR_STATUS_BUFFER_SIZE);
		sensorValueBuffer[0] = 0; // empty the buffer string
		activeSensorPtr->addReading(sensorValueBuffer, SENSOR_VALUE_BUFFER_SIZE);
		alwaysDisplayMessage("    %s:%s %s Active time(microsecs): ",
					  activeSensorPtr->sensorName, sensorStatusBuffer, sensorValueBuffer);
		alwaysDisplayMessage("%d",activeSensorPtr->activeTime);
		alwaysDisplayMessage("  Millis since last reading: ");
		alwaysDisplayMessage("%lu\n",ulongDiff(currentMillis, activeSensorPtr->millisAtLastReading));

		activeSensorPtr = activeSensorPtr->nextActiveSensor;
	}
}

void startSensorsReading()
{
	sensor *activeSensorPtr = activeSensorList;

	while (activeSensorPtr != NULL)
	{
		if (activeSensorPtr->beingUpdated)
		{
			activeSensorPtr->startReading();
		}
		activeSensorPtr = activeSensorPtr->nextActiveSensor;
	}
}

void updateSensors()
{
	sensor *activeSensorPtr = activeSensorList;

	while (activeSensorPtr != NULL)
	{
		if (activeSensorPtr->beingUpdated)
		{
			unsigned long startMicros = micros();
			activeSensorPtr->updateSensor();
			DISPLAY_MEMORY_MONITOR(activeSensorPtr->sensorName);
			activeSensorPtr->activeTime = ulongDiff(micros(), startMicros);
		}
		activeSensorPtr = activeSensorPtr->nextActiveSensor;
	}
}

void createSensorJson(char *name, char *buffer, int bufferLength)
{
	snprintf(buffer, bufferLength, "{ \"dev\":\"%s\"", name);

	sensor *activeSensorPtr = activeSensorList;

	while (activeSensorPtr != NULL)
	{
		if (activeSensorPtr->beingUpdated)
		{
			activeSensorPtr->addReading(buffer, bufferLength);
		}
		activeSensorPtr = activeSensorPtr->nextActiveSensor;
	}

	appendFormattedString(buffer, bufferLength, "}");
}

void displaySensorStatus()
{
	sensor *activeSensorPtr = activeSensorList;

	while (activeSensorPtr != NULL)
	{
		PixelStatusLevels status;
		if(activeSensorPtr->status == SENSOR_OK){
			status = PIXEL_STATUS_OK;
		}
		else {
			status = PIXEL_STATUS_ERROR;
		}
		addStatusItem(status);
		activeSensorPtr = activeSensorPtr->nextActiveSensor;
	}
}

void stopSensors()
{
	alwaysDisplayMessage("Stopping sensors\n");

	sensor *activeSensorPtr = activeSensorList;

	while (activeSensorPtr != NULL)
	{
		alwaysDisplayMessage("   %s\n", activeSensorPtr->sensorName);
		activeSensorPtr->stopSensor();
		activeSensorPtr = activeSensorPtr->nextActiveSensor;
	}
}

void iterateThroughSensors(void (*func)(sensor *s))
{
	sensor *allSensorPtr = allSensorList;

	while (allSensorPtr != NULL)
	{
		func(allSensorPtr);
		allSensorPtr = allSensorPtr->nextAllSensors;
	}
}

void iterateThroughSensorSettingCollections(void (*func)(SettingItemCollection *s))
{
	sensor *allSensorPtr = allSensorList;

	while (allSensorPtr != NULL)
	{
		if (allSensorPtr->settingItems != NULL)
		{
			func(allSensorPtr->settingItems);
		}
		allSensorPtr = allSensorPtr->nextAllSensors;
	}
}

void iterateThroughSensorSettings(void (*func)(unsigned char *settings, int size))
{
	sensor *allSensorPtr = allSensorList;

	while (allSensorPtr != NULL)
	{
		//messageLogf("  Settings %s\n", allSensorPtr->sensorName);
		func(allSensorPtr->settingsStoreBase,
			 allSensorPtr->settingsStoreLength);
		allSensorPtr = allSensorPtr->nextAllSensors;
	}
}

void iterateThroughSensorSettings(void (*func)(SettingItem *s))
{
	sensor *allSensorPtr = allSensorList;

	while (allSensorPtr != NULL)
	{
		if (allSensorPtr->settingItems != NULL)
		{
			for (int j = 0; j < allSensorPtr->settingItems->noOfSettings; j++)
			{
				func(allSensorPtr->settingItems->settings[j]);
			}
		}
		allSensorPtr = allSensorPtr->nextAllSensors;
	}
}

void resetSensorsToDefaultSettings()
{
	sensor *allSensorPtr = allSensorList;

	while (allSensorPtr != NULL)
	{
		if (allSensorPtr->settingItems != NULL)
		{
			for (int j = 0; j < allSensorPtr->settingItems->noOfSettings; j++)
			{
				void *dest = allSensorPtr->settingItems->settings[j]->value;
				allSensorPtr->settingItems->settings[j]->setDefault(dest);
			}
		}
		allSensorPtr = allSensorPtr->nextAllSensors;
	}
}

SettingItem *FindSensorSettingByFormName(const char *settingName)
{
	sensor *allSensorPtr = allSensorList;

	while (allSensorPtr != NULL)
	{
		if (allSensorPtr->settingItems != NULL)
		{
			SettingItemCollection *testItems = allSensorPtr->settingItems;

			for (int j = 0; j < allSensorPtr->settingItems->noOfSettings; j++)
			{
				SettingItem *testSetting = testItems->settings[j];
				if (matchSettingName(testSetting, settingName))
				{
					return testSetting;
				}
			}
			allSensorPtr = allSensorPtr->nextAllSensors;
		}
	}
	return NULL;
}

struct sensorEventBinder *findSensorListenerByName(struct sensor *s, const char *name)
{
	if (s->sensorListenerFunctions == NULL)
	{
		return NULL;
	}

	if (s->noOfSensorListenerFunctions == 0)
	{ 
		return NULL;
	}

	for (int i = 0; i < s->noOfSensorListenerFunctions; i++)
	{
		sensorEventBinder *binder = &s->sensorListenerFunctions[i];
		if (strcasecmp(binder->listenerName, name) == 0)
		{
			return binder;
		}
	}
	return NULL;
}

void iterateThroughSensorListeners(struct sensor *sensor, void (*func)(struct sensorListener *listener))
{
	struct sensorListener *pos = sensor->listeners;

	while (pos != NULL)
	{
		func((sensorListener *)pos);
		pos = pos->nextMessageListener;
	}
}
 
void fireSensorListenersOnTrigger(struct sensor *sensor, int trigger)
{
	struct sensorListener *pos = sensor->listeners;

	//messageLogf("      Sensor:%s mask:%d\n", sensor->sensorName, mask);

	while (pos != NULL)
	{
		//messageLogf("        Listener:%s sendoption:%d\n", pos->config->listenerName, pos->config->sendOption);
		if (pos->config->sendOption == trigger)
		{
			//messageLogf("Got a match");
			// dumpCommand(pos->config->commandProcess, pos->config->commandName, pos->config->optionBuffer);

			pos->receiveMessage(pos->config->destination, pos->config->optionBuffer);
			//messageLogf("command performed");
		}
		pos = pos->nextMessageListener;
	}
}

struct sensorEventBinder * findSensorEventBinderByTrigger(struct sensor * s, int trigger)
{
	for(int i=0; i<s->noOfSensorListenerFunctions;i++)
	{
		struct sensorEventBinder * pos = &s->sensorListenerFunctions[i];
		if(pos->trigger==trigger)
		{
			return pos;
		}
	}
	return NULL;
}
