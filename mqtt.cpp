#include "Arduino.h"
#include <credentials.h>
#include "mqtt.h"

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

static String placeStr;
static String baseTopic;
static String batteryTopic;
static String leakTopic;
static String availableTopic;
static String deviceId;

void initiateMqtt (const char* place) {
  placeStr       = String(place);
  deviceId       = String("water_leak_") + String(ESP.getChipId(), HEX);
  baseTopic      = String("water_leak_sensor/") + placeStr;
  batteryTopic   = baseTopic + "/battery";
  leakTopic      = baseTopic + "/leak";
  availableTopic = baseTopic + "/available";

  // PubSubClient default buffer is 256 bytes — HA discovery payloads can
  // exceed that, so bump it before publishing discovery messages.
  mqttClient.setBufferSize(512);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

bool connectMqtt () {
  String clientId = deviceId + "_" + String(millis());

  bool connected;
  #ifdef MQTT_USER
    connected = mqttClient.connect(
      clientId.c_str(), MQTT_USER, MQTT_PASS,
      availableTopic.c_str(), 0, true, "offline"
    );
  #else
    connected = mqttClient.connect(
      clientId.c_str(),
      availableTopic.c_str(), 0, true, "offline"
    );
  #endif

  if (connected) {
    mqttClient.publish(availableTopic.c_str(), "online", true);
  }
  return connected;
}

void disconnectMqtt () {
  // Drain outbound queue briefly so retained messages actually reach the broker.
  unsigned long start = millis();
  while (millis() - start < 100) {
    mqttClient.loop();
    delay(10);
  }
  mqttClient.disconnect();
}

static String titleCase (const String& s) {
  String r = s;
  if (r.length() > 0) r.setCharAt(0, toupper(r.charAt(0)));
  return r;
}

static void publishEntity (
  const char* component,   // "sensor" or "binary_sensor"
  const char* objectId,    // "battery", "leak"
  const char* name,        // HA friendly name
  const char* stateTopic,
  const char* deviceClass,
  const char* unit,        // NULL if not applicable
  const char* stateClass,  // NULL for binary_sensor
  int displayPrecision     // -1 = let HA decide
) {
  char topic[128];
  snprintf(topic, sizeof(topic),
    "homeassistant/%s/%s/%s/config",
    component, deviceId.c_str(), objectId);

  char payload[512];
  int len = 0;
  len += snprintf(payload + len, sizeof(payload) - len,
    "{\"name\":\"%s\",\"stat_t\":\"%s\",\"uniq_id\":\"%s_%s\","
    "\"avty_t\":\"%s\","
    "\"dev\":{\"ids\":[\"%s\"],\"name\":\"Water Leak Sensor (%s)\","
    "\"mf\":\"DIY\",\"mdl\":\"ESP-12E\"}",
    name, stateTopic, deviceId.c_str(), objectId,
    availableTopic.c_str(),
    deviceId.c_str(), titleCase(placeStr).c_str());

  if (deviceClass) {
    len += snprintf(payload + len, sizeof(payload) - len,
      ",\"dev_cla\":\"%s\"", deviceClass);
  }
  if (unit) {
    len += snprintf(payload + len, sizeof(payload) - len,
      ",\"unit_of_meas\":\"%s\"", unit);
  }
  if (stateClass) {
    len += snprintf(payload + len, sizeof(payload) - len,
      ",\"stat_cla\":\"%s\"", stateClass);
  }
  if (displayPrecision >= 0) {
    // Use the full HA field name — some HA versions only honor the
    // abbreviation "sug_dsp_prc" inconsistently for this relatively new
    // option (added in HA 2023.2).
    len += snprintf(payload + len, sizeof(payload) - len,
      ",\"suggested_display_precision\":%d", displayPrecision);
  }
  if (strcmp(component, "binary_sensor") == 0) {
    len += snprintf(payload + len, sizeof(payload) - len,
      ",\"pl_on\":\"ON\",\"pl_off\":\"OFF\"");
  }
  snprintf(payload + len, sizeof(payload) - len, "}");

  mqttClient.publish(topic, payload, true);
}

void publishDiscovery () {
  if (!mqttClient.connected()) return;

  publishEntity(
    "sensor", "battery", "Battery",
    batteryTopic.c_str(), "voltage", "V", "measurement", 2
  );
  publishEntity(
    "binary_sensor", "leak", "Leak",
    leakTopic.c_str(), "moisture", NULL, NULL, -1
  );
}

void publishBattery (float voltage) {
  char buf[16];
  dtostrf(voltage, 4, 2, buf);
  mqttClient.publish(batteryTopic.c_str(), buf, true);
}

void publishLeak (bool wet) {
  mqttClient.publish(leakTopic.c_str(), wet ? "ON" : "OFF", true);
}
