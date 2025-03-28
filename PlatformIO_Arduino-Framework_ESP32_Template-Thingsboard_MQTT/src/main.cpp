#include "main.h"

// constexpr char WIFI_SSID[] = "RD-SEAI_2.4G";
constexpr char WIFI_SSID[] = "ACLAB";
constexpr char WIFI_PASSWORD[] = "ACLAB2023";
// constexpr char WLAN_SSID[] = "RNM esp sida";
// constexpr char WLAN_PASS[] = "whyarewestillhere";

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

constexpr char DHT20_TOKEN[] = "d3P1weVrXAvkGj2bqkDW";

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

constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
constexpr uint8_t MAX_RPC_RESPONSE = 5U;

Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

DHT20 dht20;

time_t now;
struct tm timeinfo;

TaskHandle_t xdht20Handle = NULL;
TaskHandle_t xHandle = NULL;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7;
const int   daylightOffset_sec = 3600;

// DHT20 dht20;
float temp = 29.0;
float humid = 40.0;
bool dht20State = true;

uint32_t turnOnSchedule; 
uint32_t turnOffSchedule; 
bool subscribed = false;
constexpr char RPC_TEMPERATURE_KEY[] = "dht20State";
constexpr char RPC_TURNON_KEY[] = "turnOnPeriod";
constexpr char RPC_TURNOFF_KEY[] = "turnOffPeriod";

// Callback function for RPC response
void processDHT20State(const JsonVariantConst &data, JsonDocument &response) {
  dht20State = data[RPC_TEMPERATURE_KEY];
  Serial.print("DHT20 state: ");
  Serial.println(dht20State);
}

void processDHT20Scheduler(const JsonVariantConst &data, JsonDocument &response) {
  if(data[RPC_TURNON_KEY]){
    turnOnSchedule = data[RPC_TURNON_KEY];
  }

  if(data[RPC_TURNOFF_KEY]){
    turnOffSchedule = data[RPC_TURNOFF_KEY];
  }
}

void TaskLEDControl(void *pvParameters) {
  pinMode(LED, OUTPUT); // Initialize LED pin
  int ledState = 0;

  // uint8_t pin = 30;
  while(1) {
    // digitalWrite(LED, HIGH); // Turn ON LED
    // Serial.print(LED); Serial.print(" ");
    // Serial.print(digitalRead(pin));
    // Serial.println();
    Serial.println("Hello word");
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
    // Serial.println();
    
    // temp += 1;
    // humid += 1;
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void ThingsBoardTask(void *pvParameters) {
    Serial.print("Connecting to ThingsBoard...");
    if(!subscribed){
      const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
      // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response
      RPC_Callback{ RPC_TEMPERATURE_KEY, processDHT20State },
      RPC_Callback{ RPC_TURNON_KEY, processDHT20Scheduler },
      RPC_Callback{ RPC_TURNOFF_KEY, processDHT20Scheduler }
      };
      // Perform a subscription. All consequent data processing will happen in
      // processTemperatureChange() and processSwitchChange() functions,
      // as denoted by callbacks array.
      subscribed = rpc.RPC_Subscribe(callbacks.begin(), callbacks.end());
    }
    
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
      Serial.println("Connected to ThingsBoard");
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
          // if (tb.sendTelemetryJson(doc, len)) {
          if (tb.sendAttributeJson(doc, len)) {
              Serial.println("Attribute sent successfully");
              break; // Exit if successful
          } else {
              Serial.println("Failed to send attribute, retrying...");
              vTaskDelay(1000 / portTICK_PERIOD_MS);

              if(attempt == 2){

                // Serial.println((int)time(NULL));
                Serial.println("Failed to send attribute after multiple attempts");
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

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);


    // if (setenv("TZ", "CST-7", 1) != 0) {
    //   ESP_LOGE("setenv", "cant set time zone");
    // }

    // // Update the timezone settings
    // tzset();

    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2025 - 1900)) {
        ESP_LOGI("time", "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        // obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    Serial.printf("Current local time in Vietnam: %s", asctime(&timeinfo));

    vTaskDelete(NULL);
}

void TaskScheduler(void *pvParameters) {
  while(1){
    time(&now);

    if(now >= turnOffSchedule){
      vTaskSuspend(xdht20Handle);
    }

    if(now >= turnOnSchedule){
      vTaskResume(xdht20Handle);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

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

// void rpcCallback(const String &method, const String &params) {
  // Serial.println("RPC Method: " + method);
  // Serial.println("Params: " + params);
  // if (method == "setSchedule") {
  //   schedule = params; // Store the new schedule
  //   Serial.println("Schedule set: " + schedule);
  // }
//}


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
  delay(1000);
  // InitWiFi();
  
  // wifiClient.connect(THINGSBOARD_SERVER, THINGSBOARD_PORT);
  // wifiClient.connected();

  // WiFi.softAPConfig(local_ip, gateway, subnet);
  // WiFi.softAP(WLAN_SSID, WLAN_PASS);
  
  xTaskCreate(WifiTask, "Wifi", 4096, NULL, 4, NULL);
  xTaskCreate(TaskLEDControl, "LED Control", 2048, NULL, 2, &xHandle);
  xTaskCreate(TaskTemperature_Humidity, "Temp & Humid", 2048, NULL, 2, &xdht20Handle);
  xTaskCreate(ThingsBoardTask, "Thingsboard", 4096, NULL, 1, NULL);
  xTaskCreate(TaskScheduler, "Scheduler", 4096, NULL, 5, NULL);
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