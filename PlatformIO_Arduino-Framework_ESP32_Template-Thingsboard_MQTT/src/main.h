#include <Arduino.h>
#include <DHT20.h>
#include <Server_Side_RPC.h>

#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "Wire.h"
#include <ArduinoOTA.h>

#define LED GPIO_NUM_48
#define SDA GPIO_NUM_11
#define SCL GPIO_NUM_12

void processDHT20State(const JsonVariantConst &data, JsonDocument &response);
void processDHT20Scheduler(const JsonVariantConst &data, JsonDocument &response);
void TaskLEDControl(void *pvParameters);
void TaskTemperature_Humidity(void *pvParameters);
void ThingsBoardTask(void *pvParameters);
void WifiTask(void *pvParameters);
void TaskScheduler(void *pvParameters);