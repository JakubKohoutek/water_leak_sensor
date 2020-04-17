#include <ESP8266WiFi.h>
#include <user_interface.h>
#include "config.h"

#define LED_PIN         2
#define SENSOR_PIN      12
#define SLEEP_TIME      12e6

#define POWER_UP        1
#define STANDARD_WAKEUP 2
#define ALARM_WAKEUP    3

// Set ADC to read VCC voltage
ADC_MODE(ADC_VCC);

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);

  int reasonCode = findOutResetReason();
  blinkCode(reasonCode);

  switch (reasonCode) {
    case ALARM_WAKEUP:
      // Send notification
      sleep();
      break;
    case STANDARD_WAKEUP:
      // Check battery
      // If it's low, send notification
      sleep();
      break;
    default:
      sleep();
  }
}

void sleep() {
  ESP.deepSleep(SLEEP_TIME);
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void sendNotification() {
  connectToWiFi();
  // TODO
  sleep();
}

uint16_t findOutResetReason() {
  rst_info *resetInfo = ESP.getResetInfoPtr();

  if(resetInfo->reason == 0)
    return POWER_UP;

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
