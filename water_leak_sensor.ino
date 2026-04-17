#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <credentials.h>

#include "melody.h"
#include "mqtt.h"

#define LED_PIN          2
#define SENSOR_PIN       12
#define PORT             443  // for https, http uses 80 by default
#define CRITICAL_VOLTAGE 3.6f // we must leave some buffer as WiFi connection might drop
                              // the voltage even more and the whole module can freeze
#define SLEEP_TIME       3*60*60e6 // the highest sleep period that works reliably
#define PLACE     "bathroom" // place of the sensor, used in email subject + MQTT topic

#define STANDARD_WAKEUP  1
#define ALARM_WAKEUP     2

#define DEBUG

void setup () {
  // Keep the radio off at boot so readVoltage() samples the ADC with a quiet
  // RF environment. connectToWiFi() switches to WIFI_STA when we actually
  // need the network.
  WiFi.mode(WIFI_OFF);

  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println("");
  #endif

  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  digitalWrite(LED_PIN, HIGH);

  initiateMqtt(PLACE);

  int reasonCode = findOutResetReason();

  if (reasonCode == STANDARD_WAKEUP)
    handleStandardWakeup();
  else
    handleAlarmWakeup();

  ESP.deepSleep(SLEEP_TIME);
}

void connectToWiFi () {
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  #ifdef DEBUG
    Serial.println("Connected");
  #endif
}

void publishMqttStatus (float voltage, bool wet) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!connectMqtt()) {
    #ifdef DEBUG
      Serial.println("MQTT connect failed");
    #endif
    return;
  }

  publishDiscovery();
  publishBattery(voltage);
  publishLeak(wet);
  disconnectMqtt();
}

void handleAlarmWakeup () {
  // Sample voltage before turning on WiFi so RF noise doesn't corrupt the ADC.
  float voltage = readVoltage();
  connectToWiFi();
  const char* message = "Water leak in your flat detected!";
  sendSMSNotification(message);
  sendEmailNotification(message, "Urgent - water is leaking");
  publishMqttStatus(voltage, true);
  while(digitalRead(SENSOR_PIN) == LOW) {
    playMelody(melodyAlarm, melodyAlarmLength);
  }
}

void handleStandardWakeup () {
  float voltage = readVoltage();

  // Always connect WiFi on standard wake so we can publish battery to MQTT
  // and clear any stale leak=ON retained state. The extra WiFi session costs
  // ~2s every 3h, which is acceptable for continuous HA visibility.
  connectToWiFi();

  if (voltage < CRITICAL_VOLTAGE) {
    char message[60];
    sprintf(message, "Water leak sensor has a critical voltage of %.2fV!", voltage);
    sendEmailNotification(message, "Warning - low battery");
    playMelody(melodyBeep, melodyBeepLength);
  }

  publishMqttStatus(voltage, false);
}

float readVoltage () {
  // Discard the first reads — the ADC is noisy right after wake-up and
  // the first sample after a long idle period is frequently an outlier.
  const int warmupSamples = 16;
  for (int i = 0; i < warmupSamples; i++) {
    analogRead(A0);
    delay(5);
  }

  // Collect samples and reject the highest and lowest few as outliers
  // (trimmed mean) — cheaper than a full median sort but far more robust
  // to occasional ADC glitches than a plain average.
  const int samplesLength = 100;
  const int trim = 10; // drop 10 lowest + 10 highest
  int samples[samplesLength];
  for (int i = 0; i < samplesLength; i++) {
    samples[i] = analogRead(A0);
    delay(5);
  }

  // Simple insertion sort — samplesLength is small enough.
  for (int i = 1; i < samplesLength; i++) {
    int v = samples[i];
    int j = i - 1;
    while (j >= 0 && samples[j] > v) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = v;
  }

  float sum = 0;
  for (int i = trim; i < samplesLength - trim; i++) {
    sum += samples[i];
  }
  float sensorValue = sum / (float)(samplesLength - 2 * trim);

  // Calculation of voltage is based on the values of voltage divider resistors
  const int vccResistor = 1000;
  const int gndResistor = 100;
  const float callibrationCoefficient = 0.985f; // differs device by device due to the cheap ADC used
  const float resistorsRatio = (float)gndResistor / (float)(vccResistor + gndResistor);
  const float voltage = (callibrationCoefficient * sensorValue / 1024.0f) / resistorsRatio;

  #ifdef DEBUG
    Serial.println(String("ADC trimmed mean: ") + String(sensorValue, 2) +
                   String(", min: ") + String(samples[0]) +
                   String(", max: ") + String(samples[samplesLength - 1]) +
                   String(", voltage: ") + String(voltage, 3));
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
