#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "melody.h"
#include "config.h"

#define LED_PIN          2
#define SENSOR_PIN       12
#define PORT             443  // for https, http uses 80 by default
#define CRITICAL_VOLTAGE 3.6f // we must leave some buffer as WiFi connection might drop
                              // the voltage even more and the whole module can freeze
#define SLEEP_TIME       3*60*60e6 // the highest sleep period that works reliably

#define STANDARD_WAKEUP  1
#define ALARM_WAKEUP     2

//#define DEBUG

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
  while(digitalRead(SENSOR_PIN) == LOW) {
    playMelody(melodyAlarm, melodyAlarmLength);
  }
}

void handleStandardWakeup () {
  float voltage = readVoltage();

  if (voltage < CRITICAL_VOLTAGE) {
    connectToWiFi();
    char message[60];
    sprintf(message, "Water leak sensor has a critical voltage of %.2fV!", voltage);
    sendEmailNotification(message, "Warning - low battery");
    playMelody(melodyBeep, melodyBeepLength);
  }
}

float readVoltage () {
  float samples = 0;
  int samplesLength = 100;
  for (int i = 0; i < samplesLength; i++) {
    samples += analogRead(A0);
    delay(2);
  }
  int sensorValue = samples / samplesLength;

  // Calculation of voltage is based on the values of voltage divider resistors
  int vccResistor = 1000;
  int gndResistor = 100;
  float callibrationCoefficient = 0.985; // differs device by device due to the cheap ADC used
  float resistorsRatio = (float)gndResistor / (vccResistor + gndResistor);
  float voltage = (callibrationCoefficient * (float)sensorValue / 1024.0) / resistorsRatio;
  
  #ifdef DEBUG
    Serial.println(String("Sensor value: ") + String(sensorValue) + String(", voltage: ") + String(voltage));
  #endif

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
    "\"subject\":\"" + subject + " (" + PLACE + ")" + "\"," +
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

void loop () {
  // Do nothing - code will never get here
}
