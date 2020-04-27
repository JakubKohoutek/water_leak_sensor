#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "config.h"

#define LED_PIN          2
#define SENSOR_PIN       12
#define PORT             443  // for https, http uses 80 by default
#define CRITICAL_VOLTAGE 5.0f
#define SLEEP_TIME       3*60*60e6 // the highest sleep period that works reliably

#define STANDARD_WAKEUP  1
#define ALARM_WAKEUP     2

void setup () {
  WiFi.mode(WIFI_STA);

  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println("");
  #endif

  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);

  int reasonCode = findOutResetReason();
  blinkCode(reasonCode);

  if (reasonCode == STANDARD_WAKEUP)
    handleStandardWakeup();
  else
    handleAlarmWakeup();

  ESP.deepSleep(SLEEP_TIME);
}

void connectToWiFi () {
  WiFi.begin(WIFI_NAME, WIFI_PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  #ifdef DEBUG
    Serial.println("Connected");
  #endif
}

void handleAlarmWakeup () {
  connectToWiFi();
  const char* message = "Water leak in your flat detected!";
  sendSMSNotification(message);
  sendEmailNotification(message, "Urgent - water is leaking");
}

void handleStandardWakeup () {
  float voltage = readVoltage();

  if (voltage < CRITICAL_VOLTAGE) {
    connectToWiFi();
    char message[60];
    sprintf(message, "Water leak sensor has a critical voltage of %.2fV!", voltage);
    sendSMSNotification(message);
    sendEmailNotification(message, "Warning - low battery");
  }
}

float readVoltage () {
  int sensorValue = analogRead(A0);
  float voltage = (float)map(sensorValue, 13, 796, 0, 840) / 100;

  return voltage;
}

void sendSMSNotification (const char* message) {
  String url = String(API_HOST) + "/sms";
  String payload = String("{") +
    "\"message\":\"" + message + "\"," +
    "\"token\":\"" + API_TOKEN + "\"," +
    "\"phoneNumber\":\"" + PHONE_NUM + "\"" +
  "}";

  sendApiRequest(url.c_str(), payload.c_str());
}

void sendEmailNotification (const char* message, const char* subject) {
  String url = String(API_HOST) + "/email";
  String payload = String("{") +
    "\"subject\":\"" + subject + "\"," +
    "\"body\":\"<p><b>" + message + "</b></p>\"," +
    "\"token\":\"" + API_TOKEN + "\"," +
    "\"email\":\"" + EMAIL + "\"" +
  "}";

  sendApiRequest(url.c_str(), payload.c_str());
}

void sendApiRequest (const char* url, const char* payload) {
  #ifdef DEBUG
    Serial.println(url);
    Serial.println(payload);
  #endif

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.begin(client, url);
  https.addHeader("Content-Type", "application/json");
  int responseCode = https.POST(payload);

  #ifdef DEBUG
    if (responseCode > 0) {
      Serial.println(responseCode);
    } else {
      Serial.println(https.errorToString(responseCode).c_str());
    }
  #endif
}

int findOutResetReason () {
  int sensorValue = digitalRead(SENSOR_PIN);

  return (sensorValue == HIGH) ? STANDARD_WAKEUP : ALARM_WAKEUP;
}

void blinkCode (int code) {
  for(int i = 0; i < code; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(500);
    digitalWrite(LED_PIN, HIGH);
    delay(500);
  }
}

void loop () {
  // Do nothing - code will never get here
}
