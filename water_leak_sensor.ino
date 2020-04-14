#include <ESP8266WiFi.h>
#include <user_interface.h>
#include "config.h"

#define LED          2
#define POWER_UP     0
#define CODE_POWER   1
#define CODE_RESET   2

// Set ADC to read VCC voltage
ADC_MODE(ADC_VCC);

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  uint16_t reasonCode = findOutResetReason();

  switch (reasonCode) {
    case POWER_UP:
      blinkCode(CODE_POWER);
      sleep();
      break;
    default:
      blinkCode(CODE_RESET);
      sendNotification();
  }
}

void sleep() {
  ESP.deepSleep(0);
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
  uint16_t reasonCode = resetInfo->reason;

  return reasonCode;
}

void blinkCode (int code) {
  for(int i = 0; i < code; i++) {
    digitalWrite(LED, LOW);
    delay(500);
    digitalWrite(LED, HIGH);
    delay(500);
  }
}

void loop() {
  // Do nothing - code will never get here
}
