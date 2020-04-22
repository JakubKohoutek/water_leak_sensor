#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "config.h"

#define LED_PIN          2
#define SENSOR_PIN       12
#define PORT             443  // for https, http uses 80 by default
#define CRITICAL_VOLTAGE 4.5f

#define STANDARD_WAKEUP  1
#define ALARM_WAKEUP     2

void setup () {
  Serial.begin(115200);
  Serial.println("");
  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);

  int reasonCode = findOutResetReason();
  blinkCode(reasonCode);

  if (reasonCode == STANDARD_WAKEUP)
    handleStandardWakeup();
  else
    handleAlarmWakeup();

  ESP.deepSleep(ESP.deepSleepMax());
}

void connectToWiFi () {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected");
}

void disconnectFromWiFi () {
  WiFi.disconnect();
}

void handleAlarmWakeup () {
  connectToWiFi();
  const char* message = "Water leak in your flat detected!";
  sendSMSNotification(message);
  sendEmailNotification(message, "Urgent - water is leaking");
  disconnectFromWiFi();
}

void handleStandardWakeup () {
  float voltage = readVoltage();

  if (voltage < CRITICAL_VOLTAGE) {
    connectToWiFi();
    char message[60];
    sprintf(message, "Water leak sensor has a critical voltage of %.2fV!", voltage);
    sendSMSNotification(message);
    sendEmailNotification(message, "Warning - low battery");
    disconnectFromWiFi();
  }
}

float readVoltage () {
  int sensorValue = analogRead(A0);
  float voltage = (float)map(sensorValue, 13, 796, 0, 840) / 100;

  return voltage;
}

void sendSMSNotification (const char* message) {
  WiFiClientSecure client;

  client.setInsecure();
  
  HTTPClient https;
  String url = String(API_HOST) + "/sms";
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

void sendEmailNotification (const char* message, const char* subject) {
  WiFiClientSecure client;

  client.setInsecure();
  
  HTTPClient https;
  String url = String(API_HOST) + "/email";
  String payload = String("{") +
    "\"subject\":\"" + subject + "\"," +
    "\"body\":\"<p><b>" + message + "</b></p>\"," +
    "\"token\":\"" + API_TOKEN + "\"," +
    "\"email\":\"" + EMAIL + "\"" +
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

int findOutResetReason () {
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

void loop () {
  // Do nothing - code will never get here
}
