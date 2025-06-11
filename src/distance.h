#pragma once

#include <Arduino.h>
#include "settings.h"

#define PIR_READING_LIFETIME_MSECS 5000

#define DISTANCE_NOT_FITTED -1
#define DISTANCE_NOT_CONNECTED -2

#define DISTANCE_SEND_ON_CHANGE 1

struct DistanceReading {
	int distance;
};

struct DistanceSettings {
	int readingIntervalMillis;
	int deadZoneSize;
	int TriggerPinNo;
	int ReplyPinNo;
	bool DistanceFitted;
	bool DistanceInputPinActiveHigh;
};


enum DistanceSensorState
{
  DISTANCE_SENSOR_OFF,
  DISTANCE_SENSOR_ON,
  DISTANCE_SENSOR_BETWEEN_READINGS,
  DISTANCE_SENSOR_AWAITING_READING,
  DISTANCE_SENSOR_READING_READY
};


inline void startWaitBetweenReadings();
inline void updateSensorBetweenReadings();

void updateDistanceSensor();

int getDistanceValueInt();

float getDistanceValueFloat();

void directDistanceReadTest();

void testDistanceSensor();

extern struct DistanceSettings DistanceSettings;

extern struct SettingItemCollection DistanceSettingItems;

int startDistance(struct sensor * DistanceSensor);
int stopDistance(struct sensor* DistanceSensor);
int updateDistanceReading(struct sensor * DistanceSensor);
void startDistanceReading();
int addDistanceReading(struct sensor * DistanceSensor, char * jsonBuffer, int jsonBufferSize);
void DistanceStatusMessage(struct sensor * Distancesensor, char * buffer, int bufferLength);
void DistanceTest();

extern struct sensor Distance;
