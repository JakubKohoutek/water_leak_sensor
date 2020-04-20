#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "config.h"

#define LED_PIN         2
#define SENSOR_PIN      12
#define SLEEP_TIME      12e6
#define PORT            443  // for https, http uses 80 by default

#define STANDARD_WAKEUP 1
#define ALARM_WAKEUP    2

void setup() {
  Serial.begin(115200);
  Serial.println("");
  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);

  int reasonCode = findOutResetReason();
  blinkCode(reasonCode);

  switch (reasonCode) {
    case ALARM_WAKEUP:
      handleAlarmWakeup();
      break;
    case STANDARD_WAKEUP:
      handleStandardWakeup();
      break;
    default:
      break;
  }

  ESP.deepSleep(SLEEP_TIME);
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected");
}

void handleAlarmWakeup() {
  // Send alarm notification
}

void handleStandardWakeup() {
  // Check battery
  // If it's low, send battery notification
  float voltage = readVoltage();
  char message[15];
  sprintf(message, "Voltage: %.2f", voltage);
  sendNotification(message);
}

float readVoltage() {
  int sensorValue = analogRead(A0);
  float voltage = (float)map(sensorValue, 13, 796, 0, 840) / 100;

  return voltage;
}

void sendNotification(const char* message) {
  connectToWiFi();
  WiFiClientSecure client;

  // TODO: set proper SSL communication
  client.setInsecure();
  
  HTTPClient https;
  String url = String(HOST) + "/sms";
  String payload = String("{") +
    "\"message\":\"" + message + "\"," +
    "\"token\":\"" + API_TOKEN + "\"," +
    "\"phoneNumber\":\"" + PHONE_NUM + "\"" +
  "}";

  Serial.println(url);
  Serial.println(payload);

  https.begin(client, url.c_str());
  https.addHeader("Content-Type", "application/json");
  int responseCode = https.POST(payload);

  if (responseCode > 0) {
    Serial.println(responseCode);
  } else {
    Serial.println(https.errorToString(responseCode).c_str());
  }
}

int findOutResetReason() {
  int sensorValue = digitalRead(SENSOR_PIN);
  if(sensorValue == HIGH)
    return STANDARD_WAKEUP;

  return ALARM_WAKEUP;
}

void blinkCode (int code) {
  for(int i = 0; i < code; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(500);
    digitalWrite(LED_PIN, HIGH);
    delay(500);
  }
}

void loop() {
  // Do nothing - code will never get here
}
