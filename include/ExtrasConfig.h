#pragma once

/*
 * Settings for the "Extras" of this fork: DFPlayer Mini sound, the animations
 * from Manfred Hofmann's Wortuhr_mp3 project and a BME280/BMP280 sensor.
 *
 * The extras are only built for ESP32 boards. On the ESP8266 the firmware is
 * identical to the upstream ESPWortuhr project.
 *
 * Default pins match an "ESP32 D1 mini" plugged into a board that was built
 * for a WeMos D1 mini running Manfred Hofmann's firmware:
 *
 *   Function          WeMos D1 mini   ESP32 D1 mini
 *   ---------------   -------------   -------------
 *   LED data          D8              GPIO 5   (LED_PIN in Config.h)
 *   DFPlayer TX->ESP  D3              GPIO 17
 *   ESP->DFPlayer RX  D5              GPIO 18
 *   DFPlayer BUSY     D0              GPIO 26
 *   I2C SDA           D2              GPIO 21  (SDA_PIN_ESP32 in Config.h)
 *   I2C SCL           D1              GPIO 22  (SCL_PIN_ESP32 in Config.h)
 *
 * All extras pins can also be changed later on the web page
 * http://<clock-ip>:81/extras without recompiling.
 */

#include <Arduino.h> // defines ESP32 / CONFIG_IDF_TARGET_*

// Classic ESP32 only (the default pins do not exist on the ESP32-C3).
#if defined(ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3)
#define EXTRAS_ENABLED 1
#else
#define EXTRAS_ENABLED 0
#endif

//--------------------------------------------------------------------------
// DFPlayer Mini
//--------------------------------------------------------------------------
// Pin on which the ESP receives data from the DFPlayer (DFPlayer TX).
#define EXTRAS_DFPLAYER_RX_PIN 17
// Pin on which the ESP sends data to the DFPlayer (DFPlayer RX).
#define EXTRAS_DFPLAYER_TX_PIN 18
// DFPlayer BUSY output (LOW while a track is playing). 255 = not connected.
#define EXTRAS_DFPLAYER_BUSY_PIN 26

//--------------------------------------------------------------------------
// LED chain
//--------------------------------------------------------------------------
/*
 * Some clocks have the four minute LEDs at the *start* of the LED chain
 * (LED 0-3) and the letter matrix afterwards. Upstream ESPWortuhr expects the
 * minute LEDs at the end of the chain. Set to true for the first variant.
 *
 * Valid values [true, false]
 */
#define MINUTE_LEDS_FIRST true
#define MINUTE_LEDS_FIRST_COUNT 4
