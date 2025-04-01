#include "main.h"

// constexpr char WIFI_SSID[] = "RD-SEAI_2.4G";
// constexpr char WIFI_SSID[] = "ACLAB";
// constexpr char WIFI_PASSWORD[] = "ACLAB2023";
constexpr char WIFI_SSID[] = "GUEST";
constexpr char WIFI_PASSWORD[] = "tmagroup2025";
// constexpr char WLAN_SSID[] = "RNM esp sida";
// constexpr char WLAN_PASS[] = "whyarewestillhere";

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

constexpr char DHT20_TOKEN[] = "EbIRoRYlaYzCqTzd1djN"; //new device
// constexpr char DHT20_TOKEN[] = "E80kgYu3vQckw2E5YEd2"; //new device

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

DHT20 dht20;

time_t now;
struct tm timeinfo;

TaskHandle_t xdht20Handle = NULL;
TaskHandle_t xThingsHandle = NULL;

// const char* ntpServer = "pool.ntp.org";
// const long  gmtOffset_sec = 0;
// const int   daylightOffset_sec = 3600;

// DHT20 dht20;
float temp = 29.0;
float humid = 40.0;
char dht20State[256];

char turnOnSchedule[100] = ""; 
char turnOffSchedule[100] = "";
uint64_t turnOnEpoch = 0; 
uint64_t turnOffEpoch = 0;

bool subscribed = false;
bool firstRequestedShared = false;

constexpr char RPC_SENT_TELE_SUCCESS_METHOD[] = "getTeleResponse";
constexpr char RPC_SCHEDULER_METHOD[] = "setScheduler";
constexpr char RPC_TURNON_SCHE_KEY[] = "turnOnPeriod";
constexpr char RPC_TURNOFF_SCHE_KEY[] = "turnOffPeriod";
constexpr char RPC_TURNON_EPO_KEY[] = "turnOnEpoch";
constexpr char RPC_TURNOFF_EPO_KEY[] = "turnOffEpoch";

constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 5U;
constexpr uint8_t MAX_RPC_RESPONSE = 5U;
constexpr uint8_t MAX_ATTR_SUBSCRIPTIONS = 3U;
constexpr size_t MAX_ATTRIBUTES = 4U;
constexpr uint64_t REQUEST_TIMEOUT_MICROSECONDS = 1000U * 1000U;

constexpr std::array<const char*, MAX_ATTRIBUTES> REQUESTED_SHARED_ATTRIBUTES = {
  RPC_TURNON_SCHE_KEY, 
  RPC_TURNOFF_SCHE_KEY, 
  RPC_TURNON_EPO_KEY,
  RPC_TURNOFF_EPO_KEY
};

Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
Attribute_Request<MAX_ATTR_SUBSCRIPTIONS, MAX_ATTRIBUTES> attr_request;

const std::array<IAPI_Implementation*, 2U> apis = {
    &rpc,
    &attr_request
};

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, 4096, 4096, 4096, apis);

const Attribute_Request_Callback<MAX_ATTRIBUTES> sharedCallback(&processSharedAttributeRequest, REQUEST_TIMEOUT_MICROSECONDS, 
                                                                  &requestTimedOut, REQUESTED_SHARED_ATTRIBUTES);


// Callback function for RPC response
// void processDHT20State(const JsonVariantConst &data, JsonDocument &response) {
//   Serial.printf("receive state \n");
//   strcpy(dht20State, data[RPC_TEMPERATURE_KEY]);
  
//   Serial.printf("DHT20 state: %s\n", dht20State);
//   // Serial.println(dht20State);
// }
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


void processTeleSuccess(const JsonVariantConst &data, JsonDocument &response) {
  // Serial.printf("receive state \n");
  Serial.printf("Tele sent successfully (rpc)\n");
  // serializeJson(data[RPC_SENT_TELE_SUCCESS_METHOD], Serial);
  // Serial.println();
          
  // Serial.printf("temp: %.2f, humid: %.1f\n", ["temperature"], data[RPC_SENT_TELE_SUCCESS_METHOD]["humidity"]);
}

void requestTimedOut() {
  // Serial.printf("share attri time out\n");
}

void processSharedAttributeRequest(const JsonObjectConst &data) {
  firstRequestedShared = true;
  Serial.println("Requested shared attributes...");
  // for (auto it = data.begin(); it != data.end(); ++it) {
  //   Serial.println(it->key().c_str());
  //   // Shared attributes have to be parsed by their type.
  //   Serial.println(it->value().as<const char*>());
    
  // }

  
  turnOnEpoch = data[RPC_TURNON_EPO_KEY].as<const uint64_t>();
  turnOffEpoch = data[RPC_TURNOFF_EPO_KEY].as<const uint64_t>();

  strcpy(turnOnSchedule, data[RPC_TURNON_SCHE_KEY].as<const char*>());
  strcpy(turnOffSchedule, data[RPC_TURNOFF_SCHE_KEY].as<const char*>());

  Serial.printf("on epo %lld, off epo %lld\n", turnOnEpoch, turnOffEpoch);
  Serial.printf("on %s, off %s\n", turnOnSchedule, turnOffSchedule);
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
    // Serial.println();
    
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
      if (!reconnect()) {
        Serial.println("Cant Reconect WIFI");
        return;
      }
      
      // if (!tb.connected()) {
      //   // Serial.printf("Scheduler Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, DHT20_TOKEN);
      //   if (!tb.connect(THINGSBOARD_SERVER, DHT20_TOKEN, THINGSBOARD_PORT)) {
      //     Serial.println("Failed to connect");
      //     return;
      //   }
      // }

      tb.loop();
      
      if(!subscribed){
        const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
        // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response
        RPC_Callback{ RPC_SENT_TELE_SUCCESS_METHOD, processTeleSuccess }
        };
        // Perform a subscription. All consequent data processing will happen in
        // processTemperatureChange() and processSwitchChange() functions,
        // as denoted by callbacks array.
        subscribed = rpc.RPC_Subscribe(callbacks.begin(), callbacks.end());
      }
      
    StaticJsonDocument<256> doc;
    doc["temperature"] = temp;
    doc["humidity"] = humid;
    
    // Serialize the JSON document
    char buffer[MAX_MESSAGE_SIZE];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    // Attempt to send telemetry data with retries
    const int maxRetries = 3;
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
      if (tb.sendTelemetryJson(doc, len)) {
          // serializeJson(doc, Serial);
          // Serial.println();
          // Serial.println("Telemery sent successfully");
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

    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);


    if (setenv("TZ", "CST-7", 1) != 0) {
      ESP_LOGE("setenv", "cant set time zone");
    }

    // Update the timezone settings
    tzset();

    time(&now);
    localtime_r(&now, &timeinfo);
    // // Is time set? If not, tm_year will be (1970 - 1900).
    // if (timeinfo.tm_year < (2025 - 1900)) {
    //     ESP_LOGI("time", "Time is not set yet. Connecting to WiFi and getting time over NTP.");
    //     // obtain_time();
    //     // update 'now' variable with current time
    //     time(&now);
    // }
    Serial.printf("Current local time in Vietnam: %s", asctime(&timeinfo));

    vTaskDelete(NULL);
}

void dht20Power(uint64_t turnOnEpoch, uint64_t turnOffEpoch, char turnOnPeriod[], char turnOffPeriod[]){
  time(&now);
  localtime_r(&now, &timeinfo);
  int turnOnSche = ((turnOnPeriod[0] - '0') * 10  + (turnOnPeriod[1] - '0')) * 60 + (turnOnPeriod[3] - '0') * 10 + (turnOnPeriod[0] - '4');
  int turnOffSche = ((turnOffPeriod[0] - '0') * 10  + (turnOffPeriod[1] - '0')) * 60 + (turnOffPeriod[3] - '0') * 10 + (turnOffPeriod[0] - '4');

  // Serial.printf("on Sche %d, off Sche %d\n", turnOnSche, turnOffSche);

  if(now >= turnOffEpoch || (timeinfo.tm_hour * 60 + timeinfo.tm_min) >=  turnOffSche){
    Serial.println("Turn off DHT20");
    vTaskSuspend(xdht20Handle);
  }

  if(now >= turnOnEpoch || (timeinfo.tm_hour * 60 + timeinfo.tm_min) >=  turnOnSche){
    Serial.println("Turn on DHT20");
    vTaskResume(xdht20Handle);
  }
}

void TaskScheduler(void *pvParameters) {
  int count = 0, delay_period = 1000;
  // Increase buffer size if necessary
  tb.setBufferSize(MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE); // Adjust as needed
  
  while(1){
    if (!reconnect()) {
      Serial.println("Cant Reconect WIFI");
      return;
    }
    
    if (!tb.connected()) {
      // Serial.printf("Scheduler Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, DHT20_TOKEN);
      if (!tb.connect(THINGSBOARD_SERVER, DHT20_TOKEN, THINGSBOARD_PORT)) {
        Serial.println("Failed to connect");
        return;
      }
    }
    // vTaskResume(xThingsHandle);
    tb.loop();
    // wifiSemaphore = false;
    // Serial.println("Release");
    // xEventGroupSetBits(networkEventGroup, NETWORK_CONNECT);

    // Shared attributes we want to request from the server
    attr_request.Shared_Attributes_Request(sharedCallback);
    // ++count;
    
    if (firstRequestedShared) {
    // if (count > 0) {
      // Serial.printf("--------------------- %d\n", count);
      // vTaskSuspend(xThingsHandle);
      // tb.disconnect();
      // attr_request.Unsubscribe();
      // attr_request.Initialize();
      // attr_request.Resubscribe_Topic();
      delay_period = 10000;
      dht20Power(turnOnEpoch, turnOffEpoch, turnOnSchedule, turnOffSchedule);
      
    }
    
    vTaskDelay(pdMS_TO_TICKS(delay_period));
  }

  vTaskDelete(NULL);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_DEBUG_BAUD);
  delay(2000);
  if (!reconnect()) {
    Serial.println("Cant Reconect WIFI");
    return;
  }
  // InitWiFi();
  
  // WiFi.softAPConfig(local_ip, gateway, subnet);
  // WiFi.softAP(WLAN_SSID, WLAN_PASS);
  
  // xTaskCreate(WifiTask, "Wifi", 4096, NULL, 5, NULL);
  xTaskCreate(TaskLEDControl, "LED Control", 2048, NULL, 1, NULL);
  xTaskCreate(TaskTemperature_Humidity, "Temp & Humid", 2048, NULL, 4, &xdht20Handle);
  xTaskCreate(ThingsBoardTask, "Thingsboard", 4096, NULL, 3, &xThingsHandle);
  xTaskCreate(TaskScheduler, "Scheduler", 4096, NULL, 2, NULL);
  // xTaskCreate(onMessageReceived, "Temp & Humid", 2048, NULL, 3, NULL);
  // xTaskCreate(onSubscriptionSuccess, "Temp & Humid", 2048, NULL, 3, NULL);
}

void loop() {
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

  // tb.loop();

  

  // for(int count = 0; count < 4;){
    // if(!requestedShared){
      // int count = 0;

      // requestedShared = attr_request.Shared_Attributes_Request(sharedCallback);
      
      // if (!requestedShared) {
      //   // Serial.println("---------------------");
      //   // ++count;
      //   attr_request.Unsubscribe();
      // }
      // // if(count == 3){
      //   // count = -1;
        // Serial.printf("on epo %ld, off epo %ld\n", turnOnEpoch, turnOffEpoch);
        // Serial.printf("on %s, off %s\n", turnOnSchedule, turnOffSchedule);
      // }
      // count++;

      // requestedShared = attr_request.Shared_Attributes_Request(sharedCallback);
      // if (!requestedShared) {
      //   Serial.println("Failed to request shared attributes 3s");
        // ++count;
      // }
      // else {
      //   // Serial.println(count);
      //   // requestedShared = false;
      //   // break;
      // }
    // }
  //   delay(1000);
  // }

  
//     vTaskDelay(pdMS_TO_TICKS(1000));
//     Serial.println("1s");
}


// constexpr const char RPC_JSON_METHOD[] = "setState";
// constexpr const char RPC_TEMPERATURE_METHOD[] = "setState";
// constexpr const char RPC_SWITCH_METHOD[] = "setState";
// // constexpr const char RPC_TEMPERATURE_KEY[] = "temp";
// constexpr const char RPC_SWITCH_KEY[] = "dht20State";
// // constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
// // constexpr uint8_t MAX_RPC_RESPONSE = 5U;


// /// @brief Initalizes WiFi connection,
// // will endlessly delay until a connection has been successfully established
// void InitWiFi() {
//   Serial.println("Connecting to AP ...");
//   // Attempting to establish a connection to the given WiFi network
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   while (WiFi.status() != WL_CONNECTED) {
//     // Delay 500ms until a connection has been successfully established
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("Connected to AP");
// #if ENCRYPTED
//   espClient.setCACert(ROOT_CERT);
// #endif
// }

// /// @brief Reconnects the WiFi uses InitWiFi if the connection has been removed
// /// @return Returns true as soon as a connection has been established again
// bool reconnect() {
//   // Check to ensure we aren't connected yet
//   const wl_status_t status = WiFi.status();
//   if (status == WL_CONNECTED) {
//     return true;
//   }

//   // If we aren't establish a new connection to the given WiFi network
//   InitWiFi();
//   return true;
// }

// /// @brief Processes function for RPC call "example_json"
// /// JsonVariantConst is a JSON variant, that can be queried using operator[]
// /// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
// /// @param data Data containing the rpc data that was called and its current value
// /// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
// void processGetJson(const JsonVariantConst &data, JsonDocument &response) {
//   Serial.println("Received the json RPC method");

//   // Size of the response document needs to be configured to the size of the innerDoc + 1.
//   StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
//   innerDoc["string"] = "exampleResponseString";
//   innerDoc["int"] = 5;
//   innerDoc["float"] = 5.0f;
//   innerDoc["bool"] = true;
//   response["json_data"] = innerDoc;
// }

// /// @brief Processes function for RPC call "example_set_temperature"
// /// JsonVariantConst is a JSON variant, that can be queried using operator[]
// /// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
// /// @param data Data containing the rpc data that was called and its current value
// /// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
// void processTemperatureChange(const JsonVariantConst &data, JsonDocument &response) {
//   Serial.println("Received the set temperature RPC method");

//   // Process data
//   const float example_temperature = data[RPC_TEMPERATURE_KEY];

//   Serial.print("Example temperature: ");
//   Serial.println(example_temperature);

//   // Ensure to only pass values do not store by copy, or if they do increase the MaxRPC template parameter accordingly to ensure that the value can be deserialized.RPC_Callback.
//   // See https://arduinojson.org/v6/api/jsondocument/add/ for more information on which variables cause a copy to be created
//   response["string"] = "exampleResponseString";
//   response["int"] = 5;
//   response["float"] = 5.0f;
//   response["double"] = 10.0;
//   response["bool"] = true;
// }

// /// @brief Processes function for RPC call "example_set_switch"
// /// JsonVariantConst is a JSON variant, that can be queried using operator[]
// /// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
// /// @param data Data containing the rpc data that was called and its current value
// /// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
// void processSwitchChange(const JsonVariantConst &data, JsonDocument &response) {
//   Serial.println("Received the set switch method");

//   // Process data
//   const bool switch_state = data[RPC_SWITCH_KEY];

//   Serial.print("Example switch state: ");
//   Serial.println(switch_state);

//   response.set(22.02);
// }

// void setup() {
//   // Initalize serial connection for debugging
//   Serial.begin(SERIAL_DEBUG_BAUD);
//   delay(1000);
//   InitWiFi();
// }

// void loop() {
//   delay(1000);

//   if (!reconnect()) {
//     return;
//   }

//   if (!tb.connected()) {
//     // Reconnect to the ThingsBoard server,
//     // if a connection was disrupted or has not yet been established
//     Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, DHT20_TOKEN);
//     if (!tb.connect(THINGSBOARD_SERVER, DHT20_TOKEN, THINGSBOARD_PORT)) {
//       Serial.println("Failed to connect");
//       return;
//     }
//   }

//   if (!subscribed) {
//     Serial.println("Subscribing for RPC...");
//     const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
//       // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response
//       RPC_Callback{ RPC_JSON_METHOD,           processGetJson },
//       // Requires additional memory in the JsonDocument for 5 key-value pairs that do not copy their value into the JsonDocument itself
//       RPC_Callback{ RPC_TEMPERATURE_METHOD,    processTemperatureChange },
//        // Internal size can be 0, because if we use the JsonDocument as a JsonVariant and then set the value we do not require additional memory
//       RPC_Callback{ RPC_SWITCH_METHOD,         processSwitchChange }
//     };
//     // Perform a subscription. All consequent data processing will happen in
//     // processTemperatureChange() and processSwitchChange() functions,
//     // as denoted by callbacks array.
//     if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
//       Serial.println("Failed to subscribe for RPC");
//       return;
//     }

//     Serial.println("Subscribe done");
//     subscribed = true;
//   }

//   tb.loop();
// }