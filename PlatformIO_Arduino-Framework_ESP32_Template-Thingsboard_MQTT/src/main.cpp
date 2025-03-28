// // #ifdef ESP8266
// // #include <ESP8266WiFi.h>
// // #else
// // #ifdef ESP32
// // #include <WiFi.h>
// // #endif // ESP32
// // #endif // ESP8266

// // #include <Arduino_MQTT_Client.h>
// // #include <ThingsBoard.h>

// // #define ENCRYPTED false

// // constexpr char WIFI_SSID[] = "RD-SEAI_2.4G";
// // constexpr char WIFI_PASSWORD[] = "";
// // constexpr char TOKEN[] = "d3P1weVrXAvkGj2bqkDW";
// // constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";

// // #if ENCRYPTED
// // constexpr uint16_t THINGSBOARD_PORT = 8883U;
// // #else
// // constexpr uint16_t THINGSBOARD_PORT = 1883U;
// // #endif

// // constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 256U;
// // constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 256U;
// // constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

// // #if ENCRYPTED
// // WiFiClientSecure espClient;
// // #else
// // WiFiClient espClient;
// // #endif

// // Arduino_MQTT_Client mqttClient(espClient);
// // ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE);

// // void InitWiFi() {
// //   Serial.println("Connecting to AP ...");
// //   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
// //   while (WiFi.status() != WL_CONNECTED) {
// //     delay(500);
// //     Serial.print(".");
// //   }
// //   Serial.println("Connected to AP");
// // }

// // bool reconnect() {
// //   if (WiFi.status() == WL_CONNECTED) {
// //     return true;
// //   }
// //   InitWiFi();
// //   return true;
// // }

// // void setup() {
// //   Serial.begin(SERIAL_DEBUG_BAUD);
// //   delay(1000);
// //   InitWiFi();
// // }

// // void loop() {
// //   delay(1000);

// //   if (!reconnect()) {
// //     return;
// //   }

// //   if (!tb.connected()) {
// //     Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
// //     if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
// //       Serial.println("Failed to connect");
// //       return;
// //     }
// //   }

// //   // Create a JSON document
// //   StaticJsonDocument<256> doc;
// //   doc["temperature"] = random(20, 30);  // Example: Simulating a temperature reading

// //   // Serialize the JSON document
// //   char buffer[256];
// //   size_t len = serializeJson(doc, buffer, sizeof(buffer));

// //   // Send telemetry data
// //   if (!tb.sendTelemetryJson(doc, len)) {
// //     Serial.println("Failed to send telemetry");
// //   }

// //   tb.loop();
// // }


#include <Arduino.h>
#include <DHT20.h>

#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "Wire.h"
// #include <ArduinoOTA.h>

constexpr char WIFI_SSID[] = "ACLAB";
constexpr char WIFI_PASSWORD[] = "ACLAB2023";
// constexpr char WLAN_SSID[] = "RNM esp sida";
// constexpr char WLAN_PASS[] = "whyarewestillhere";

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

constexpr char DHT20_TOKEN[] = "s0uon5xr11cfwn6urwxt"; //new device
// constexpr char DHT20_TOKEN[] = "d3P1weVrXAvkGj2bqkDW"; //old device (telemetry don't exhibit)

constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_MODE_ATTR[] = "ledMode";
constexpr char LED_STATE_ATTR[] = "ledState";

constexpr char TEMP_INTERVAL_ATTR[] = "tempInterval";
constexpr char HUMID_INTERVAL_ATTR[] = "humidInterval";

volatile bool attributesChanged = false;
volatile int ledMode = 0;
volatile bool ledState = false;

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;

constexpr uint16_t TEMP_INTERVAL_MS_MIN = 10U;
constexpr uint16_t TEMP_INTERVAL_MS_MAX = 60000U;
volatile uint16_t tempInterval = 1000U;

constexpr uint16_t HUMID_INTERVAL_MS_MIN = 10U;
constexpr uint16_t HUMID_INTERVAL_MS_MAX = 60000U;
volatile uint16_t humidInterval = 1000U;

uint32_t previousStateChange;

constexpr int16_t telemetrySendInterval = 10000U;
uint32_t previousDataSend;

constexpr std::array<const char *, 4U> SHARED_ATTRIBUTES_LIST = {
  LED_STATE_ATTR,
  BLINKING_INTERVAL_ATTR,
  TEMP_INTERVAL_ATTR,
  HUMID_INTERVAL_ATTR,
};

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

DHT20 dht20;



#define LED GPIO_NUM_48
#define SDA GPIO_NUM_11
#define SCL GPIO_NUM_12

// DHT20 dht20;
float temp = 29.0;
float humid = 40.0;
// int light = 50;
// float longi = 106.80633605864662;
// float lat = 10.880018410410052;

void TaskLEDControl(void *pvParameters) {
  pinMode(LED, OUTPUT); // Initialize LED pin
  int ledState = 0;

  // uint8_t pin = 30;
  while(1) {
    // digitalWrite(LED, HIGH); // Turn ON LED
    // Serial.print(LED); Serial.print(" ");
    // Serial.print(digitalRead(pin));
    // Serial.println();
    // Serial.println("Hello word");
    if (ledState == 0) {
      digitalWrite(LED, HIGH); // Turn ON LED
    } else {
      digitalWrite(LED, LOW); // Turn OFF LED
    }
    ledState = 1 - ledState;
    // pin += 1;
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void TaskTemperature_Humidity(void *pvParameters){
  DHT20 dht20;
  Wire.begin(SDA, SCL);
  dht20.begin();
  while(1){
    dht20.read();
    // double temperature = dht20.getTemperature();
    // double humidity = dht20.getHumidity();

    // Serial.print("Temp: "); Serial.print(temperature); Serial.print(" *C ");
    // Serial.print(" Humidity: "); Serial.print(humidity); Serial.print(" %");
    // Serial.println();
    temp = dht20.getTemperature();
    humid = dht20.getHumidity();

    // Serial.print("Temp: "); Serial.print(temp); Serial.print(" *C ");
    // Serial.print(" Humidity: "); Serial.print(humid); Serial.print(" %");
    Serial.println();
    
    // temp += 1;
    // humid += 1;
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void ThingsBoardTask(void *pvParameters) {
    Serial.print("Connecting to ThingsBoard...");
    
    while (1)
    {
      while (WiFi.status() != WL_CONNECTED) {
        continue;
        // vTaskDelay(500 / portTICK_PERIOD_MS);
      }

      while (!tb.connect(THINGSBOARD_SERVER, DHT20_TOKEN, THINGSBOARD_PORT)) {
          Serial.print("-");
          vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      // Serial.println("Connected to ThingsBoard");
      tb.loop();
      // String collectData = String("{\"temperature\":") + temp +
      //                    ",\"humidity\":" + humid +
      //                    ",\"light\":" + light +
      //                    ",\"long\":" + longi +
      //                    ",\"lat\":" + lat + "}";
      StaticJsonDocument<256> doc;
      doc["temperature"] = temp;
      doc["humidity"] = humid;
      // doc["light"] = light;
      // doc["long"] = longi;
      // doc["lat"] = lat;

      // Serialize the JSON document
      char buffer[256];
      size_t len = serializeJson(doc, buffer, sizeof(buffer));

      // serializeJson(doc, Serial);
      // Serial.println();
      // Serial.print("len ");
      // Serial.println(len);

      // Increase buffer size if necessary
      tb.setBufferSize(128, 128); // Adjust as needed

      // // Send telemetry data
      // if (!tb.sendTelemetryJson(doc, len)) {
      //   Serial.println("Failed to send telemetry");
      // }

      // DynamicJsonDocument doc(MAX_MESSAGE_SIZE);
      // doc["temperature"] = temp;
      // doc["humidity"] = humid;
      // doc["light"] = light;
      // doc["long"] = longi;
      // doc["lat"] = lat;
      

      // // Publish telemetry data
      // // Serial.println(doc.size());
      // char buffer[256];
      // size_t len = serializeJson(doc, buffer, sizeof(buffer));
      // if (!tb.sendTelemetryJson(doc, len)) {
      //   Serial.println("Failed to send telemetry");
      // }
      // tb.sendTelemetryData("temperature", temp);
      // tb.sendTelemetryData("humidity", humid);

      // Serial.println("Sended");

      // serializeJson(doc, Serial);
      // Serial.println();

      // Attempt to send telemetry data with retries
      const int maxRetries = 3;
      for (int attempt = 0; attempt < maxRetries; ++attempt) {
          if (tb.sendTelemetryJson(doc, len)) {
              Serial.println("Telemetry sent successfully");
              break; // Exit if successful
          } else {
              Serial.println("Failed to send telemetry, retrying...");
              vTaskDelay(1000 / portTICK_PERIOD_MS);

              if(attempt == 2){

                // Serial.println((int)time(NULL));
                Serial.println("Failed to send telemetry after multiple attempts");
              }
              
          }
      }
      

      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void WifiTask(void *pvParameters) {
    Serial.print("Connecting to Wifi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    Serial.println("Wifi connected");

    vTaskDelete(NULL);
}

void onMessageReceived(const String &topic, const String &payload) {
    Serial.print("Received message on topic: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(payload);

    // Parse JSON
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
        if (doc["method"] == "setValue") {
            bool value = doc["params"];
            // Handle the value as needed
            Serial.printf("Value set to: %s\n", value ? "true" : "false");
            // Publish back to ThingsBoard if necessary
            tb.sendTelemetryJson(doc, doc.size());
        }
    } else {
        Serial.println("Failed to parse JSON");
    }

    vTaskDelete(NULL);
}

void onSubscriptionSuccess(void *pvParameters) {
    Serial.println("Successfully subscribed to topic");
    // tb.onMessage(onMessageReceived); // Register message callback

    vTaskDelete(NULL);
}

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

bool reconnect() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  InitWiFi();
  return true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_DEBUG_BAUD);
  // delay(1000);
  InitWiFi();
  
  // wifiClient.connect(THINGSBOARD_SERVER, THINGSBOARD_PORT);
  // wifiClient.connected();

  // WiFi.softAPConfig(local_ip, gateway, subnet);
  // WiFi.softAP(WLAN_SSID, WLAN_PASS);

  // xTaskCreate(WifiTask, "Wifi", 4096, NULL, 4, NULL);
  xTaskCreate(TaskLEDControl, "LED Control", 2048, NULL, 2, NULL);
  xTaskCreate(TaskTemperature_Humidity, "Temp & Humid", 2048, NULL, 2, NULL);
  xTaskCreate(ThingsBoardTask, "Thingsboard", 4096, NULL, 1, NULL);
  // xTaskCreate(onMessageReceived, "Temp & Humid", 2048, NULL, 3, NULL);
  // xTaskCreate(onSubscriptionSuccess, "Temp & Humid", 2048, NULL, 3, NULL);
}

void loop() {
  // delay(1000);

  // if (!reconnect()) {
  //   return;
  // }

  // if (!tb.connected()) {
  //   Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, DHT20_TOKEN);
  //   if (!tb.connect(THINGSBOARD_SERVER, DHT20_TOKEN, THINGSBOARD_PORT)) {
  //     Serial.println("Failed to connect");
  //     return;
  //   }
  // }

  // // Create a JSON document
  // StaticJsonDocument<256> doc;
  // doc["temperature"] = random(20, 30);  // Example: Simulating a temperature reading

  // // Serialize the JSON document
  // char buffer[256];
  // size_t len = serializeJson(doc, buffer, sizeof(buffer));

  // // Send telemetry data
  // if (!tb.sendTelemetryJson(doc, len)) {
  //   Serial.println("Failed to send telemetry");
  // }

  // tb.loop();
}