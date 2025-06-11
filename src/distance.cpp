#ifdef SENSOR_PIR

#include "distance.h"
#include "debug.h"
#include "sensors.h"
#include "settings.h"
#include "mqtt.h"
#include "controller.h"
#include "pixels.h"

struct DistanceSettings distanceSettings;

void setDefaultDistanceReadingIntervalMillis(void *dest)
{
	int *destInt = (int *)dest;
	*destInt = 100;
}

struct SettingItem DistanceReadingIntervalMillis = {
	"Distance reading interval (millis)",
	"distancereadinginterval",
	&distanceSettings.readingIntervalMillis,
	NUMBER_INPUT_LENGTH,
	integerValue,
	setDefaultDistanceReadingIntervalMillis,
	validateInt};


void setDefaultDistanceDeadZoneSize(void *dest)
{
	int *destInt = (int *)dest;
	*destInt = 10;
}

struct SettingItem DistanceDeadZoneSize = {
	"Distance dead zone (mm)",
	"distancedeadzonesize",
	&distanceSettings.deadZoneSize,
	NUMBER_INPUT_LENGTH,
	integerValue,
	setDefaultDistanceDeadZoneSize,
	validateInt};

void setDefaultDistanceTriggerPinNo(void *dest)
{
	int *destInt = (int *)dest;
	*destInt = 17;
}

struct SettingItem DistanceTriggerPinNoSetting = {
	"Distance Trigger Pin",
	"distancetriggerpin",
	&distanceSettings.TriggerPinNo,
	NUMBER_INPUT_LENGTH,
	integerValue,
	setDefaultDistanceTriggerPinNo,
	validateInt};

void setDefaultDistanceReplyPinNo(void *dest)
{
	int *destInt = (int *)dest;
	*destInt = 16;
}

struct SettingItem DistanceReplyPinNoSetting = {
	"Distance Reply Pin",
	"distancereplypin",
	&distanceSettings.ReplyPinNo,
	NUMBER_INPUT_LENGTH,
	integerValue,
	setDefaultDistanceReplyPinNo,
	validateInt};

struct SettingItem DistanceFittedSetting = {
	"Distance sensor fitted (yes or no)",
	"distancefitted",
	&distanceSettings.DistanceFitted,
	ONOFF_INPUT_LENGTH,
	yesNo,
	setFalse,
	validateYesNo};

struct SettingItem *DistanceSettingItemPointers[] =
	{
		&DistanceDeadZoneSize,
		&DistanceReadingIntervalMillis,
		&DistanceFittedSetting,
		&DistanceTriggerPinNoSetting,
		&DistanceReplyPinNoSetting};

struct SettingItemCollection DistanceSettingItems = {
	"Distance",
	"Distance hardware",
	DistanceSettingItemPointers,
	sizeof(DistanceSettingItemPointers) / sizeof(struct SettingItem *)};

struct sensorEventBinder DistanceListenerFunctions[] = {
	{"changed", DISTANCE_SEND_ON_CHANGE}
};

volatile long pulseStartTime;
volatile long pulseWidth;

volatile DistanceSensorState distanceSensorState = DISTANCE_SENSOR_OFF;

volatile int distanceSensorReadingIntervalInMillisecs;

volatile unsigned long timeOfLastDistanceReading;

void pulseEvent()
{
	if (digitalRead(distanceSettings.ReplyPinNo))
	{
		// pulse gone high - record start
		pulseStartTime = micros();
	}
	else
	{
		pulseWidth = micros() - pulseStartTime;
		distanceSensorState = DISTANCE_SENSOR_READING_READY;
	}
}

void startDistanceSensorReading()
{
	digitalWrite(distanceSettings.TriggerPinNo, LOW);
	delayMicroseconds(2);

	digitalWrite(distanceSettings.TriggerPinNo, HIGH);

	delayMicroseconds(10);

	distanceSensorState = DISTANCE_SENSOR_AWAITING_READING;

	digitalWrite(distanceSettings.TriggerPinNo, LOW);

	// let the signals settle (actually I've no idea why this is needed)

	delay(5);
}

void setupDistanceSensorHardware()
{
	if (distanceSensorState != DISTANCE_SENSOR_OFF)
		return;

	pulseWidth = 0;
	pinMode(distanceSettings.TriggerPinNo, OUTPUT);
	pinMode(distanceSettings.ReplyPinNo, INPUT);
	attachInterrupt(digitalPinToInterrupt(distanceSettings.ReplyPinNo), pulseEvent, CHANGE);

	startDistanceSensorReading();
}

inline void startWaitBetweenReadings()
{
	timeOfLastDistanceReading = millis();

	distanceSensorState = DISTANCE_SENSOR_BETWEEN_READINGS;
}

// checks to see if it is time to take another reading

inline void updateSensorBetweenReadings()
{
	unsigned long now = millis();
	unsigned long timeSinceLastReading = ulongDiff(now, timeOfLastDistanceReading);

	if (timeSinceLastReading >= distanceSettings.readingIntervalMillis)
	{
		startDistanceSensorReading();
	}
}

void updateDistanceSensor()
{
	switch (distanceSensorState)
	{
	case DISTANCE_SENSOR_OFF:
		// if the sensor has not been turned on - do nothing
		break;

	case DISTANCE_SENSOR_ON:
		// if the sensor is on, start a reading
		startDistanceSensorReading();
		break;

	case DISTANCE_SENSOR_AWAITING_READING:
		// if the sensor is awaiting a reading - do nothing
		break;

	case DISTANCE_SENSOR_BETWEEN_READINGS:
		// if the sensor is between readings - check the timer
		updateSensorBetweenReadings();
		break;

	case DISTANCE_SENSOR_READING_READY:
		startWaitBetweenReadings();
		break;
	}
}

int getDistanceValueInt()
{
	return (int)(pulseWidth / 5.8) + distanceSensorState;
}

float getDistanceValueFloat()
{
	return (float)pulseWidth / 5.80;
}

void readDistance(struct DistanceReading *DistanceactiveReading)
{
	DistanceactiveReading->distance = getDistanceValueInt();
}

void updateDistance()
{
	struct DistanceReading *distanceactiveReading =
		(struct DistanceReading *)Distance.activeReading;

	updateDistanceSensor();

	int newDistanceReading = getDistanceValueInt();

	int previousDistanceReading = distanceactiveReading->distance;

	if (abs(previousDistanceReading - previousDistanceReading) < distanceSettings.deadZoneSize)
	{
		// we have read the distance but the change is less than the dead zone
		// just return
		return;
	}

	Distance.millisAtLastReading = millis();

	sensorListener *pos = Distance.listeners;

	while (pos != NULL)
	{
		struct sensorListenerConfiguration *config = pos->config;

		if (config->sendOption & DISTANCE_SEND_ON_CHANGE)
		{
			unsigned char *optionBuffer = pos->config->optionBuffer;
			putUnalignedFloat(distanceactiveReading->distance, (unsigned char *)optionBuffer);
			char *messageBuffer = (char *)optionBuffer + MESSAGE_START_POSITION;
			snprintf(messageBuffer, MAX_MESSAGE_LENGTH, "changed");
		}

		pos->receiveMessage(config->destination, config->optionBuffer);
		pos->lastReadingMillis = Distance.millisAtLastReading;
		pos = pos->nextMessageListener;
	}
}


void startDistance()
{
	if (Distance.activeReading == NULL)
	{
		Distance.activeReading = new DistanceReading();
	}

	if (!DistanceSettings.DistanceFitted)
	{
		Distance.status = DISTANCE_NOT_FITTED;
	}
	else
	{
		setupDistanceSensorHardware();
		Distance.status = SENSOR_OK;
	}
}

void stopDistance()
{
}

void updateDistanceReading()
{
	switch (Distance.status)
	{
	case SENSOR_OK:
		updateDistance();
		break;

	case DISTANCE_NOT_FITTED:
		break;
	}
}

void addDistanceReading(char *jsonBuffer, int jsonBufferSize)
{
	struct DistanceReading *DistanceactiveReading =
		(struct DistanceReading *)Distance.activeReading;

	if (Distance.status == SENSOR_OK)
	{
		appendFormattedString(jsonBuffer, jsonBufferSize, ",\"pir\":\"%d\"",
							  DistanceactiveReading->distance);
	}
}

void DistanceStatusMessage(char *buffer, int bufferLength)
{
	struct DistanceReading *DistanceactiveReading =
		(struct DistanceReading *)Distance.activeReading;

	switch (Distance.status)
	{
	case SENSOR_OK:
		snprintf(buffer, bufferLength, "Distance %d",DistanceactiveReading->distance);
		break;

	case SENSOR_OFF:
		snprintf(buffer, bufferLength, "Distance off");
		break;

	case DISTANCE_NOT_FITTED:
		snprintf(buffer, bufferLength, "Distance not fitted");
		break;

	default:
		snprintf(buffer, bufferLength, "Distance status invalid");
		break;
	}
}

struct sensor Distance = {
	"PIR",
	0, // millis at last reading
	0, // reading number
	0, // last transmitted reading number
	startDistance,
	stopDistance,
	updateDistanceReading,
	startDistanceReading,
	addDistanceReading,
	DistanceStatusMessage,
	-1,	   // status
	false, // being updated
	NULL,  // active reading - set in setup
	0,	   // active time
	(unsigned char *)&DistanceSettings,
	sizeof(struct DistanceSettings),
	&DistanceSettingItems,
	NULL, // next active sensor
	NULL, // next all sensors
	NULL, // message listeners
	DistanceListenerFunctions,
	sizeof(DistanceListenerFunctions) / sizeof(struct sensorEventBinder)};

#endif