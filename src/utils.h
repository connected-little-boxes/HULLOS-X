#pragma once

#pragma GCC diagnostic ignored "-Wwrite-strings"

int localRand();
int localRand(int limit);
int localRand(int low, int high);
void localSrand(int seed);

int srtcasecmp(const char *string1, const char *string2);

unsigned long ulongDiff(unsigned long end, unsigned long start);

bool strContains(char* searchMe,char* findMe);

int getUnalignedInt(unsigned char * source);

float getUnalignedFloat(unsigned char * source);

void putUnalignedFloat(float fval, unsigned char * dest);

float getUnalignedDouble(unsigned char *source);
void putUnalignedDouble(double dval, unsigned char *dest);
void start_memory_monitor();
void display_memory_monitor( char * item);
void appendFormattedString(char * dest, int limit, const char *format, ...);

#define ESC_KEY 0x1b

#if defined(ARDUINO_ARCH_ESP32)

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
//#include <DNSServer.h>

#define PROC_NAME "ESP32"

#endif

#if defined(ARDUINO_ARCH_ESP8266)

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#define PROC_NAME "ESP8266"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#endif

#if defined(PICO)

#include <Arduino.h>
#include "WiFi.h"
#define PROC_NAME "PICO"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#endif

void getProcID (char * dest, int length);

unsigned long getProcIDSalt ();

#if !defined(WEMOSD1MINI) && !defined(ESP32DOIT)
#define IRAM_ATTR 
#endif

extern const char * version;

bool endsWith(const char *str, const char *suffix);

void strip_end(char *str, int n);

