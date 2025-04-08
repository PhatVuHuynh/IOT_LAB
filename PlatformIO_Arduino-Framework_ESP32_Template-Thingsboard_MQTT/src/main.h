#ifndef __MAIN_H
#define __MAIN_H

#include <Arduino.h>
#include <DHT20.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>

#include <Arduino_MQTT_Client.h>
#include "DHT20.h"
#include "Wire.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif // ESP32
#endif // ESP8266

#include <Arduino_MQTT_Client.h>
#include <OTA_Firmware_Update.h>
#include "HttpsOTAUpdate.h"
#include <ThingsBoard.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#ifdef ESP8266
#include <Arduino_ESP8266_Updater.h>
#else
#ifdef ESP32
#include <Espressif_Updater.h>
#endif // ESP32
#endif // ESP8266

#define LED GPIO_NUM_48
#define SDA GPIO_NUM_11
#define SCL GPIO_NUM_12

// #define NETWORK_CONNECT     BIT0

// EventGroupHandle_t networkEventGroup;

// bool wifiSemaphore = false;

void requestTimedOut();
void processSharedAttributeRequest(const JsonObjectConst &data) ;
void processTeleSuccess(const JsonVariantConst &data, JsonDocument &response);

bool reconnect();
void InitWiFi();
void dht20Power(uint64_t turnOnEpoch, uint64_t turnOffEpoch, char turnOnPeriod[], char turnOffPeriod[]);

void TaskLEDControl(void *pvParameters);
void TaskTemperature_Humidity(void *pvParameters);
void ThingsBoardTask(void *pvParameters);
void WifiTask(void *pvParameters);
void TaskScheduler(void *pvParameters);

#endif // __MAIN_H