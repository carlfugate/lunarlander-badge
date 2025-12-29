#ifndef HARDWARE_BADGE_REGISTRATION_H
#define HARDWARE_BADGE_REGISTRATION_H

#include <Arduino.h>


#define NVS_KEY_REGISTERED "badge_registered"
#define NVS_KEY_TIME "reg_time"
#define NVS_KEY_LAST_CHECKIN "last_checkin"
#define NVS_KEY_CODE_VERSION "code_version"
#define NVS_KEY_WIFI_SSID "wifi_ssid"


void handleBadgeRegistration();
int handleBadgeRegistrationWithResult();
bool isBadgeRegistered();
String getRegistrationTime();
String getLastCheckinTime();
String getCodeVersion();
String getWiFiSSID();
int getTimesSeen();
void incrementTimesSeen();

#endif // HARDWARE_BADGE_REGISTRATION_H
