#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "fileLogger.h"



void setupOTA(const char* hostname) {

  

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    logToFile("Start updating..." + type);
  })
  .onEnd([]() {
    logToFile("...upload suceed");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) logToFile("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) logToFile("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) logToFile("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) logToFile("Receive Failed");
    else if (error == OTA_END_ERROR) logToFile("End Failed");
  });

  ArduinoOTA.begin();

  logToFile("OTA Initialized");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
