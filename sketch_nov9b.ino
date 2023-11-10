#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// WiFi connection constants
const char* WIFI_SSID = "THMMPXF 1048";
const char* WIFI_PASSWORD = ">7r16F56";

// HTTP connection constants
const char* SERVER_URL = "http://10.161.3.199:3000/data";

// DHT sensor setup
const int DHT_SENSOR_PIN = D4;
DHT dhtSensor(DHT_SENSOR_PIN, DHT11);

// NTP setup
const long TIMEZONE_OFFSET = 25200; // 7 hours for Thailand
const int NTP_UPDATE_INTERVAL = 60000; // Update interval in milliseconds
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", TIMEZONE_OFFSET, NTP_UPDATE_INTERVAL);

// HTTP client
WiFiClient wifiClient;
HTTPClient httpClient;

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  dhtSensor.begin();
  timeClient.begin();
}

void sendSensorData(float humidity, float temperature) {
  // Prepare JSON document
  StaticJsonDocument<300> doc;
  char timeString[25];
  time_t currentTime = timeClient.getEpochTime();
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", gmtime(&currentTime));
  JsonObject dataObject = doc.createNestedObject(timeString);
  dataObject["humidity"] = humidity;
  dataObject["temperature"] = temperature;

  // Serialize JSON document
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  httpClient.begin(wifiClient, SERVER_URL);
  httpClient.addHeader("Content-Type", "application/json");
  int httpResponseCode = httpClient.PATCH(jsonPayload);

  // Handle HTTP response
  if (httpResponseCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    Serial.println("Returned payload:");
    Serial.println(httpClient.getString());
  } else {
    Serial.printf("HTTP PATCH request failed with error: %d\n", httpResponseCode);
  }

  httpClient.end();
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  const unsigned long UPDATE_DELAY = 15000; // Delay between updates in milliseconds

  if ((millis() - lastUpdateTime) > UPDATE_DELAY) {
    timeClient.update();
    float humidity = dhtSensor.readHumidity();
    float temperature = dhtSensor.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor");
      return;
    }

    Serial.printf("Humidity: %.2f%%\n", humidity);
    Serial.printf("Temperature: %.2f°C\n", temperature);
    sendSensorData(humidity, temperature);
    lastUpdateTime = millis();
  }
}
