#if defined(SETTINGS_WEB_SERVER)

#pragma once

#include "settings.h"
#include "processes.h"
#include "connectwifi.h"

#define SETTINGS_WEBSITE_TIMEOUT_SECS 120
#define SETTINGS_ACCESS_POINT_SSID "CLB_SETUP"
#define SETTINGS_MDNS_NAME "clb"

void startHostingConfigWebsite(bool timeout);

#endif