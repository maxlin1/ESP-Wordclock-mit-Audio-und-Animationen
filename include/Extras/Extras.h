#pragma once

/*
 * "Extras" of this fork: sound with a DFPlayer Mini (hourly chime, time
 * announcement, event sounds), the animations of Manfred Hofmann's
 * Wortuhr_mp3 project, events (birthdays, Christmas, New Year ...) and a
 * BME280/BMP280 sensor. Everything can be controlled on the web page
 * http://<clock-ip>:81/extras and via MQTT / Home Assistant.
 *
 * On boards without extras (ESP8266) all functions are empty.
 */

#include "../ExtrasConfig.h"
#include <Arduino.h>
#include <ArduinoJson.h>

namespace extras {

#if EXTRAS_ENABLED

// Call once at the end of setup() (needs I2C, WiFi and the OTA web server).
void setup();
// Call in every loop().
void loop();

// True while an animation is shown; the clock must not draw then.
bool ownsDisplay();
// Text that the scrolling text mode should show instead of G.scrollingText
// (used for events), or nullptr.
const char *scrollingTextOverride();

// MQTT / Home Assistant integration (called from Mqtt.cpp)
void mqttSubscribe();
bool mqttHandle(const String &topic, const char *payload);
void mqttAddDiscovery(JsonObject components, const String &deviceId,
                      const String &baseTopic);
void mqttPublishState();

#else

inline void setup() {}
inline void loop() {}
inline bool ownsDisplay() { return false; }
inline const char *scrollingTextOverride() { return nullptr; }
inline void mqttSubscribe() {}
inline bool mqttHandle(const String &, const char *) { return false; }
inline void mqttAddDiscovery(JsonObject, const String &, const String &) {}
inline void mqttPublishState() {}

#endif

} // namespace extras
