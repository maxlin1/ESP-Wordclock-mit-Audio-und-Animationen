#include "Extras/Extras.h"

#if EXTRAS_ENABLED

#include "Config.h" // WEB_PROTECTED, WEB_USER, WEB_PASSWORD
#include "WordClockState.h"
#include "WordClockTypes/ClockType.hpp"

#include "Extras/AnimationPlayer.h"
#include "Extras/Bme280.h"
#include "Extras/DfPlayer.h"
#include "Extras/ExtrasPage.h"
#include "Led.h"

#include <Preferences.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>
#include <vector>

extern Led led;
extern WebServer httpServer; // OTA web server on port 81 (WordClock.cpp)
extern PubSubClient mqttClient;

namespace extras {
namespace {

//------------------------------------------------------------------------------
// Sound file numbers (sound package of Manfred Hofmann's Wortuhr_mp3)
//------------------------------------------------------------------------------

struct SoundSet {
    const char *name;
    int16_t base;
};

constexpr int16_t BASE_VICKI = 1000;
constexpr int16_t BASE_HANS = 2000;
constexpr int16_t BASE_VICKI_HANS = -1;
constexpr int16_t BASE_RANDOM = -2;
constexpr uint8_t NUMBER_OF_GONG_SETS = 13; // sets 0-12 are gongs

// Same order (= same numbers) as in Manfred's firmware, so his settings for
// "sound per weekday" can be taken over 1:1.
const SoundSet SOUND_SETS[] = {
    {"Sound 1", 0},         {"Sound 2", 20},
    {"Sound 3", 40},        {"Sound 4", 60},
    {"Kuckuck", 80},        {"Bing", 120},
    {"Kirche", 140},        {"Westminster 1", 100},
    {"Westminster 2", 160}, {"Alte Uhr", 180},
    {"Ding", 200},          {"Hund und Katz", 220},
    {"Tod", 240},           {"Vicki", BASE_VICKI},
    {"Hans", BASE_HANS},    {"Vicki/Hans", BASE_VICKI_HANS},
    {"Zufall", BASE_RANDOM}};
constexpr uint8_t SOUND_SET_COUNT = sizeof(SOUND_SETS) / sizeof(SOUND_SETS[0]);

// Offsets inside a voice (Vicki = 1000, Hans = 2000)
constexpr uint16_t V_INDOOR_TEMPERATURE = 34;
constexpr uint16_t V_INDOOR_HUMIDITY = 35;
constexpr uint16_t V_PLUS = 53;
constexpr uint16_t V_MINUS = 54;
constexpr uint16_t V_COMMA = 55;
constexpr uint16_t V_PERCENT = 56;
constexpr uint16_t V_DEGREE = 58;
constexpr uint16_t V_YEAR_2000 = 60;
constexpr uint16_t V_TODAY_IS = 62;
constexpr uint16_t V_THE = 63;
constexpr uint16_t V_DAY = 100;     // + day of month ("erste" ...)
constexpr uint16_t V_WEEKDAY = 140; // + 1 (Sunday) ... 7 (Saturday)
constexpr uint16_t V_MONTH = 150;   // + 1 ... 12
constexpr uint16_t V_NUMBER = 200;  // + 0 ... 99
constexpr uint16_t V_HUNDRED = 301;

constexpr uint16_t SOUND_STARTUP = 703;
constexpr uint16_t SOUND_OK = 700;

// Volume in percent per hour of the day (from Manfred's configuration)
const uint8_t VOLUME_BY_HOUR[24] = {30,  20,  10,  10,  10,  20,  30,  50,
                                    80,  90,  100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 90,  80,  60};

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

struct Settings {
    bool sound = true;
    uint8_t volume = 70; // 0 - 100
    bool volumeByTime = true;
    bool weekendLater = true;
    bool quiet = true; // no sounds between quietFrom and quietTo
    uint8_t quietFrom = 22;
    uint8_t quietTo = 7;
    uint8_t voice = 1; // 0 = Vicki, 1 = Hans
    bool voice24h = true;
    bool chime = false;
    bool singleGong = false;
    uint8_t chimeSet[7] = {8, 0, 1, 2, 3, 4, 5}; // Sunday ... Saturday
    bool timeAnimations = true;
    bool events = true;
    bool startupSound = true;
    uint8_t rxPin = EXTRAS_DFPLAYER_RX_PIN;
    uint8_t txPin = EXTRAS_DFPLAYER_TX_PIN;
    uint8_t busyPin = EXTRAS_DFPLAYER_BUSY_PIN;
    int16_t altitude = 0; // m above sea level, for the relative pressure
    float temperatureOffset = 0.0f;
};

struct Event {
    uint8_t month = 1;
    uint8_t day = 1;
    uint16_t year = 0; // 0 = every year; otherwise "YY" in the text = age
    String text;
    uint16_t sound = 0;
    String pre;            // animation before the text
    String post;           // animation after the text
    uint8_t interval = 60; // minutes
};

constexpr uint8_t MAX_EVENTS = 20;
constexpr uint8_t MAX_EVENT_TEXT = 80;

Settings cfg;
std::vector<Event> events;
Preferences prefs;

DfPlayer audio;
AnimationPlayer player;
Bme280 sensor;

bool vickiHansToggle = false;
int8_t lastMinute = -1;
uint32_t lastSensorRead = 0;
uint32_t lastClimatePublish = 0;
bool statePublishPending = false;

//------------------------------------------------------------------------------
// Show = animation before + scrolling text + animation after (+ sound)
//------------------------------------------------------------------------------

enum class ShowStep : uint8_t { Idle, Pre, Text, Post };

struct Show {
    ShowStep step = ShowStep::Idle;
    int16_t pre = -1;
    int16_t post = -1;
    char text[MAX_EVENT_TEXT + 8] = {0};
    uint32_t textUntil = 0;
    bool textShown = false;
    uint16_t savedProg = COMMAND_IDLE;
};

Show show;

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

bool timeIsValid() { return time(nullptr) > 1600000000; }

bool localTime(struct tm &tm) {
    if (!timeIsValid()) {
        return false;
    }
    time_t now = time(nullptr);
    localtime_r(&now, &tm);
    return true;
}

bool isQuietHour(uint8_t hour) {
    if (!cfg.quiet || cfg.quietFrom == cfg.quietTo) {
        return false;
    }
    if (cfg.quietFrom < cfg.quietTo) {
        return hour >= cfg.quietFrom && hour < cfg.quietTo;
    }
    return hour >= cfg.quietFrom || hour < cfg.quietTo;
}

bool soundAllowedNow() {
    if (!cfg.sound || !audio.isStarted()) {
        return false;
    }
    struct tm tm;
    if (localTime(tm) && isQuietHour(tm.tm_hour)) {
        return false;
    }
    return true;
}

// Volume for the DFPlayer (0-30); volume100 = 0 means "use the setting".
uint8_t volume30(uint8_t volume100 = 0) {
    float volume = volume100 > 0 ? volume100 : cfg.volume;
    struct tm tm;
    if (cfg.volumeByTime && localTime(tm)) {
        int hour = tm.tm_hour;
        // Saturday and Sunday: louder two hours later in the morning
        if (cfg.weekendLater && (tm.tm_wday == 0 || tm.tm_wday == 6) &&
            hour > 4 && hour < 12) {
            hour -= 2;
        }
        volume = volume * VOLUME_BY_HOUR[hour] / 100.0f;
    }
    int v = static_cast<int>(volume * 0.3f + 0.5f);
    return static_cast<uint8_t>(constrain(v, 0, 30));
}

uint16_t voiceBase() { return cfg.voice == 0 ? BASE_VICKI : BASE_HANS; }

void playTrack(uint16_t track, bool interrupt = false, uint8_t volume100 = 0) {
    if (track == 0 || !audio.isStarted()) {
        return;
    }
    if (interrupt) {
        audio.playNow(track, volume30(volume100));
    } else {
        audio.enqueue(track, volume30(volume100));
    }
}

bool clockFaceVisible() {
    if (!led.getState() || G.conf != COMMAND_IDLE) {
        return false;
    }
    return G.prog == COMMAND_IDLE || G.prog == COMMAND_MODE_WORD_CLOCK ||
           G.prog == COMMAND_MODE_TRANSITION;
}

// Convert UTF-8 text to the code page of the clock font (CP437) so that
// German umlauts are shown correctly.
void toClockFont(const String &in, char *out, size_t outSize) {
    size_t o = 0;
    for (size_t i = 0; i < in.length() && o + 1 < outSize; i++) {
        uint8_t c = static_cast<uint8_t>(in[i]);
        if (c == 0xC3 && i + 1 < in.length()) {
            uint8_t n = static_cast<uint8_t>(in[++i]);
            switch (n) {
            case 0xA4:
                c = 0x84;
                break; // ä
            case 0xB6:
                c = 0x94;
                break; // ö
            case 0xBC:
                c = 0x81;
                break; // ü
            case 0x84:
                c = 0x8E;
                break; // Ä
            case 0x96:
                c = 0x99;
                break; // Ö
            case 0x9C:
                c = 0x9A;
                break; // Ü
            case 0x9F:
                c = 0xE1;
                break; // ß
            default:
                c = '?';
                break;
            }
        } else if (c >= 0x80) {
            // skip other multi-byte characters
            while (i + 1 < in.length() &&
                   (static_cast<uint8_t>(in[i + 1]) & 0xC0) == 0x80) {
                i++;
            }
            c = '?';
        }
        out[o++] = static_cast<char>(c);
    }
    out[o] = '\0';
}

//------------------------------------------------------------------------------
// Announcements
//------------------------------------------------------------------------------

void announceNumber(uint16_t base, int number) {
    if (number < 0) {
        playTrack(base + V_MINUS);
        number = -number;
    }
    if (number >= 100) {
        playTrack(base + V_HUNDRED + (number / 100) - 1);
        number %= 100;
        if (number == 0) {
            return;
        }
    }
    playTrack(base + V_NUMBER + number);
}

void announceTime() {
    struct tm tm;
    if (!localTime(tm)) {
        return;
    }
    const uint16_t base = voiceBase();
    playTrack(base + (tm.tm_hour == 0 ? 24 : tm.tm_hour));
    if (tm.tm_min > 0) {
        playTrack(base + V_NUMBER + tm.tm_min);
    }
}

void announceDate() {
    struct tm tm;
    if (!localTime(tm)) {
        return;
    }
    const uint16_t base = voiceBase();
    playTrack(base + V_TODAY_IS);
    playTrack(base + V_WEEKDAY + tm.tm_wday + 1);
    playTrack(base + V_THE);
    playTrack(base + V_DAY + tm.tm_mday);
    playTrack(base + V_MONTH + tm.tm_mon + 1);
    const int year = tm.tm_year + 1900;
    if (year >= 2000 && year < 2100) {
        playTrack(base + V_YEAR_2000);
        playTrack(base + V_NUMBER + (year - 2000));
    }
}

void announceClimate() {
    if (!sensor.isPresent() || isnan(sensor.temperature)) {
        return;
    }
    const uint16_t base = voiceBase();
    const float t = sensor.temperature + cfg.temperatureOffset;
    const int tenths = static_cast<int>(roundf(fabsf(t) * 10.0f));
    playTrack(base + V_INDOOR_TEMPERATURE);
    if (t < 0) {
        playTrack(base + V_MINUS);
    } else if (t > 0 && t < 5) {
        playTrack(base + V_PLUS);
    }
    playTrack(base + V_NUMBER + (tenths / 10) % 100);
    playTrack(base + V_COMMA);
    playTrack(base + V_NUMBER + tenths % 10);
    playTrack(base + V_DEGREE);

    if (sensor.hasHumidity() && !isnan(sensor.humidity)) {
        playTrack(base + V_INDOOR_HUMIDITY);
        const int h =
            constrain(static_cast<int>(sensor.humidity + 0.5f), 0, 100);
        announceNumber(base, h);
        playTrack(base + V_PERCENT);
    }
}

bool announce(const String &what) {
    if (!audio.isStarted()) {
        return false;
    }
    audio.stop();
    if (what == "time") {
        announceTime();
    } else if (what == "date") {
        announceDate();
    } else if (what == "climate") {
        announceClimate();
    } else {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
// Hourly chime
//------------------------------------------------------------------------------

void playChime(const struct tm &tm) {
    uint8_t set = cfg.chimeSet[tm.tm_wday % 7];
    if (set >= SOUND_SET_COUNT) {
        set = 0;
    }
    int16_t base = SOUND_SETS[set].base;
    if (base == BASE_RANDOM) {
        base = SOUND_SETS[random(0, NUMBER_OF_GONG_SETS)].base;
    } else if (base == BASE_VICKI_HANS) {
        base = vickiHansToggle ? BASE_HANS : BASE_VICKI;
        vickiHansToggle = !vickiHansToggle;
    }
    const bool voice = base == BASE_VICKI || base == BASE_HANS;
    uint8_t hour = tm.tm_hour;
    if (!(voice && cfg.voice24h) && hour > 12) {
        hour -= 12;
    }
    const uint16_t file = (voice || !cfg.singleGong) ? base + hour : base + 1;
    Serial.printf("Extras: chime %s, file %u\n", SOUND_SETS[set].name, file);
    playTrack(file);
}

//------------------------------------------------------------------------------
// Shows (animation / text / animation)
//------------------------------------------------------------------------------

uint32_t scrollDurationMs(const char *text) {
    // ClockWork scrolls one column every (11 - effectSpeed) * 30 ms, a
    // character is 6 columns wide and 2 characters are needed to scroll out.
    const uint32_t columns = (strlen(text) + 2) * 6;
    const uint32_t step = (11u - constrain(G.effectSpeed, 1, 10)) * 30u;
    return columns * step + 500;
}

void beginText() {
    show.savedProg = G.prog;
    show.textShown = true;
    show.textUntil = millis() + scrollDurationMs(show.text);
    G.prog = COMMAND_MODE_SCROLLINGTEXT;
    G.progInit = true;
}

void endText() {
    if (!show.textShown) {
        return;
    }
    show.textShown = false;
    // Only switch back if nobody else changed the mode meanwhile.
    if (G.prog == COMMAND_MODE_SCROLLINGTEXT) {
        const uint16_t prog = show.savedProg;
        G.prog = (prog == COMMAND_IDLE || prog == COMMAND_MODE_TRANSITION)
                     ? COMMAND_MODE_WORD_CLOCK
                     : prog;
        G.progInit = true;
    }
    parametersChanged = true;
}

void enterStep(ShowStep step) {
    show.step = step;
    switch (step) {
    case ShowStep::Pre:
        if (show.pre < 0 || !player.start(show.pre)) {
            enterStep(ShowStep::Text);
        }
        break;
    case ShowStep::Text:
        if (show.text[0] == '\0') {
            enterStep(ShowStep::Post);
        } else {
            beginText();
        }
        break;
    case ShowStep::Post:
        if (show.post < 0 || !player.start(show.post)) {
            show.step = ShowStep::Idle;
        }
        break;
    case ShowStep::Idle:
        break;
    }
}

void stopShow() {
    player.stop();
    endText();
    show.step = ShowStep::Idle;
}

void startShow(int16_t pre, const String &text, int16_t post, uint16_t sound) {
    stopShow();
    show.pre = pre;
    show.post = post;
    toClockFont(text, show.text, sizeof(show.text));
    if (sound > 0 && soundAllowedNow()) {
        playTrack(sound, true);
    }
    enterStep(ShowStep::Pre);
    statePublishPending = true;
}

void loopShow() {
    switch (show.step) {
    case ShowStep::Pre:
        if (!player.isRunning()) {
            enterStep(ShowStep::Text);
        }
        break;
    case ShowStep::Text:
        if (static_cast<int32_t>(millis() - show.textUntil) >= 0 ||
            G.prog != COMMAND_MODE_SCROLLINGTEXT) {
            endText();
            enterStep(ShowStep::Post);
        }
        break;
    case ShowStep::Post:
        if (!player.isRunning()) {
            show.step = ShowStep::Idle;
            statePublishPending = true;
        }
        break;
    case ShowStep::Idle:
        break;
    }
}

bool playAnimation(const String &name) {
    const int16_t index = AnimationPlayer::find(name.c_str());
    if (index < 0) {
        return false;
    }
    startShow(index, "", -1, 0);
    return true;
}

//------------------------------------------------------------------------------
// Events
//------------------------------------------------------------------------------

String eventText(const Event &e, const struct tm &tm) {
    String text = e.text;
    if (e.year > 1 && text.indexOf("YY") >= 0) {
        text.replace("YY", String(tm.tm_year + 1900 - e.year));
    }
    return text;
}

void runEvent(const Event &e, const struct tm &tm) {
    Serial.printf("Extras: event %02u-%02u %s\n", e.month, e.day,
                  e.text.c_str());
    startShow(AnimationPlayer::find(e.pre.c_str()), eventText(e, tm),
              AnimationPlayer::find(e.post.c_str()), e.sound);
}

const Event *dueEvent(const struct tm &tm) {
    if (!cfg.events) {
        return nullptr;
    }
    const uint16_t minuteOfDay = tm.tm_hour * 60 + tm.tm_min;
    for (const Event &e : events) {
        if (e.month != tm.tm_mon + 1 || e.day != tm.tm_mday) {
            continue;
        }
        const uint8_t interval = e.interval == 0 ? 60 : e.interval;
        if (minuteOfDay % interval == 0) {
            return &e;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
// Things that happen when the minute changes
//------------------------------------------------------------------------------

void onNewMinute(const struct tm &tm) {
    const bool quiet = isQuietHour(tm.tm_hour);

    if (tm.tm_min == 0 && cfg.chime && cfg.sound && !quiet) {
        playChime(tm);
    }

    if (!clockFaceVisible() || show.step != ShowStep::Idle) {
        return;
    }

    if (const Event *e = dueEvent(tm)) {
        runEvent(*e, tm);
        return;
    }

    if (cfg.timeAnimations) {
        char name[8];
        snprintf(name, sizeof(name), "Z%02d%02d", tm.tm_hour, tm.tm_min);
        playAnimation(name);
    }
}

//------------------------------------------------------------------------------
// Persistence
//------------------------------------------------------------------------------

void settingsToJson(JsonObject o) {
    o["sound"] = cfg.sound;
    o["volume"] = cfg.volume;
    o["volumeByTime"] = cfg.volumeByTime;
    o["weekendLater"] = cfg.weekendLater;
    o["quiet"] = cfg.quiet;
    o["quietFrom"] = cfg.quietFrom;
    o["quietTo"] = cfg.quietTo;
    o["voice"] = cfg.voice;
    o["voice24h"] = cfg.voice24h;
    o["chime"] = cfg.chime;
    o["singleGong"] = cfg.singleGong;
    JsonArray sets = o.createNestedArray("chimeSet");
    for (uint8_t s : cfg.chimeSet) {
        sets.add(s);
    }
    o["timeAnimations"] = cfg.timeAnimations;
    o["events"] = cfg.events;
    o["startupSound"] = cfg.startupSound;
    o["rxPin"] = cfg.rxPin;
    o["txPin"] = cfg.txPin;
    o["busyPin"] = cfg.busyPin;
    o["altitude"] = cfg.altitude;
    o["temperatureOffset"] = cfg.temperatureOffset;
}

uint8_t pinOrNone(JsonVariantConst v, uint8_t fallback) {
    if (v.isNull()) {
        return fallback;
    }
    const int pin = v.as<int>();
    return (pin >= 0 && pin <= 39) ? pin : DfPlayer::PIN_NONE;
}

void settingsFromJson(JsonObjectConst o) {
    cfg.sound = o["sound"] | cfg.sound;
    cfg.volume = constrain(o["volume"] | static_cast<int>(cfg.volume), 0, 100);
    cfg.volumeByTime = o["volumeByTime"] | cfg.volumeByTime;
    cfg.weekendLater = o["weekendLater"] | cfg.weekendLater;
    cfg.quiet = o["quiet"] | cfg.quiet;
    cfg.quietFrom =
        constrain(o["quietFrom"] | static_cast<int>(cfg.quietFrom), 0, 23);
    cfg.quietTo =
        constrain(o["quietTo"] | static_cast<int>(cfg.quietTo), 0, 23);
    cfg.voice = (o["voice"] | static_cast<int>(cfg.voice)) == 0 ? 0 : 1;
    cfg.voice24h = o["voice24h"] | cfg.voice24h;
    cfg.chime = o["chime"] | cfg.chime;
    cfg.singleGong = o["singleGong"] | cfg.singleGong;
    JsonArrayConst sets = o["chimeSet"];
    if (!sets.isNull()) {
        for (uint8_t i = 0; i < 7 && i < sets.size(); i++) {
            const int s = sets[i] | 0;
            cfg.chimeSet[i] = (s >= 0 && s < SOUND_SET_COUNT) ? s : 0;
        }
    }
    cfg.timeAnimations = o["timeAnimations"] | cfg.timeAnimations;
    cfg.events = o["events"] | cfg.events;
    cfg.startupSound = o["startupSound"] | cfg.startupSound;
    cfg.rxPin = pinOrNone(o["rxPin"], cfg.rxPin);
    cfg.txPin = pinOrNone(o["txPin"], cfg.txPin);
    cfg.busyPin = pinOrNone(o["busyPin"], cfg.busyPin);
    cfg.altitude =
        constrain(o["altitude"] | static_cast<int>(cfg.altitude), -500, 9000);
    cfg.temperatureOffset = constrain(
        o["temperatureOffset"] | cfg.temperatureOffset, -20.0f, 20.0f);
}

void eventsToJson(JsonArray a) {
    for (const Event &e : events) {
        JsonObject o = a.createNestedObject();
        o["month"] = e.month;
        o["day"] = e.day;
        o["year"] = e.year;
        o["text"] = e.text;
        o["sound"] = e.sound;
        o["pre"] = e.pre;
        o["post"] = e.post;
        o["interval"] = e.interval;
    }
}

// Accepts this firmware's format as well as the events.json of Manfred's
// firmware ({"events": [{"jahr", "monat", "tag", "text", "audionr", ...}]}).
bool eventsFromJson(JsonVariantConst root) {
    JsonArrayConst list = root.is<JsonArrayConst>()
                              ? root.as<JsonArrayConst>()
                              : root["events"].as<JsonArrayConst>();
    if (list.isNull()) {
        return false;
    }
    std::vector<Event> imported;
    for (JsonVariantConst item : list) {
        JsonObjectConst o = item.as<JsonObjectConst>();
        if (o.isNull()) {
            continue;
        }
        if (imported.size() >= MAX_EVENTS) {
            break;
        }
        Event e;
        const bool manfred = o.containsKey("monat");
        e.month = manfred ? (o["monat"] | 0) : (o["month"] | 0);
        e.day = manfred ? (o["tag"] | 0) : (o["day"] | 0);
        const int year = manfred ? (o["jahr"] | 0) : (o["year"] | 0);
        e.year = year > 1 ? year : 0;
        e.text = String(static_cast<const char *>(o["text"] | ""))
                     .substring(0, MAX_EVENT_TEXT);
        e.sound = manfred ? (o["audionr"] | 0) : (o["sound"] | 0);
        e.pre = manfred ? (o["preani"] | "") : (o["pre"] | "");
        e.post = manfred ? (o["postani"] | "") : (o["post"] | "");
        const int interval =
            manfred ? (o["intervall"] | 60) : (o["interval"] | 60);
        e.interval = constrain(interval, 1, 255);
        if (e.month < 1 || e.month > 12 || e.day < 1 || e.day > 31) {
            continue;
        }
        imported.push_back(e);
    }
    events = imported;
    return true;
}

void setDefaultEvents() {
    events.clear();
    auto add = [](uint8_t m, uint8_t d, const char *text, uint16_t sound,
                  const char *pre, const char *post, uint8_t interval) {
        Event e;
        e.month = m;
        e.day = d;
        e.text = text;
        e.sound = sound;
        e.pre = pre;
        e.post = post;
        e.interval = interval;
        events.push_back(e);
    };
    add(1, 1, "Frohes neues Jahr!", 733, "FEUERWERK", "FEUERWERK", 120);
    add(12, 24, "Frohe Weihnachten!", 719, "KERZE", "WINTER", 60);
    add(12, 25, "Frohes Fest!", 719, "BAUM", "KERZE", 60);
    add(12, 26, "Frohe Weihnachten!", 719, "WINTER", "KERZE", 120);
}

void saveSettings() {
    DynamicJsonDocument doc(1024);
    settingsToJson(doc.to<JsonObject>());
    String json;
    serializeJson(doc, json);
    prefs.putString("cfg", json);
}

void saveEvents() {
    DynamicJsonDocument doc(6144);
    eventsToJson(doc.to<JsonArray>());
    String json;
    serializeJson(doc, json);
    prefs.putString("events", json);
}

void loadSettings() {
    const String cfgJson = prefs.getString("cfg", "");
    if (cfgJson.length() > 0) {
        DynamicJsonDocument doc(1024);
        if (!deserializeJson(doc, cfgJson)) {
            settingsFromJson(doc.as<JsonObjectConst>());
        }
    }
    const String eventJson = prefs.getString("events", "");
    bool loaded = false;
    if (eventJson.length() > 0) {
        DynamicJsonDocument doc(6144);
        if (!deserializeJson(doc, eventJson)) {
            loaded = eventsFromJson(doc.as<JsonVariantConst>());
        }
    }
    if (!loaded) {
        setDefaultEvents();
    }
}

void startAudio() { audio.begin(cfg.rxPin, cfg.txPin, cfg.busyPin); }

//------------------------------------------------------------------------------
// Status JSON (web page and MQTT)
//------------------------------------------------------------------------------

float relativePressure() {
    if (isnan(sensor.pressure)) {
        return NAN;
    }
    return sensor.pressure / powf(1.0f - cfg.altitude / 44330.0f, 5.255f);
}

void climateToJson(JsonObject o) {
    o["sensor"] = sensor.chipName();
    if (!sensor.isPresent()) {
        return;
    }
    if (!isnan(sensor.temperature)) {
        o["temperature"] =
            roundf((sensor.temperature + cfg.temperatureOffset) * 10) / 10;
    }
    if (sensor.hasHumidity() && !isnan(sensor.humidity)) {
        o["humidity"] = roundf(sensor.humidity * 10) / 10;
    }
    const float p = relativePressure();
    if (!isnan(p)) {
        o["pressure"] = roundf(p * 10) / 10;
    }
}

//------------------------------------------------------------------------------
// Web interface (port 81)
//------------------------------------------------------------------------------

bool authorized() {
#if WEB_PROTECTED
    if (!httpServer.authenticate(WEB_USER, WEB_PASSWORD)) {
        httpServer.requestAuthentication();
        return false;
    }
#endif
    return true;
}

void sendJson(const JsonDocument &doc, int code = 200) {
    String out;
    serializeJson(doc, out);
    httpServer.send(code, "application/json", out);
}

void sendOk() { httpServer.send(200, "application/json", "{\"ok\":true}"); }

void sendError(const char *message) {
    String out = String("{\"ok\":false,\"error\":\"") + message + "\"}";
    httpServer.send(400, "application/json", out);
}

void handlePage() {
    if (!authorized()) {
        return;
    }
    httpServer.send_P(200, "text/html", EXTRAS_PAGE);
}

void handleGetAll() {
    if (!authorized()) {
        return;
    }
    DynamicJsonDocument doc(12288);
    settingsToJson(doc.createNestedObject("settings"));
    eventsToJson(doc.createNestedArray("events"));

    JsonObject status = doc.createNestedObject("status");
    status["dfplayer"] = audio.isStarted();
    status["playing"] = audio.isActive();
    status["animation"] = player.currentName();
    climateToJson(status.createNestedObject("climate"));
    struct tm tm;
    status["quietNow"] = localTime(tm) && isQuietHour(tm.tm_hour);
    status["volumeNow"] = volume30();

    JsonArray sets = doc.createNestedArray("soundSets");
    for (const SoundSet &s : SOUND_SETS) {
        sets.add(s.name);
    }
    JsonArray animations = doc.createNestedArray("animations");
    for (uint8_t i = 0; i < AnimationPlayer::count(); i++) {
        animations.add(AnimationPlayer::name(i));
    }
    sendJson(doc);
}

void handlePostSettings() {
    if (!authorized()) {
        return;
    }
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, httpServer.arg("plain"))) {
        sendError("invalid json");
        return;
    }
    const uint8_t rx = cfg.rxPin, tx = cfg.txPin, busy = cfg.busyPin;
    settingsFromJson(doc.as<JsonObjectConst>());
    saveSettings();
    if (rx != cfg.rxPin || tx != cfg.txPin || busy != cfg.busyPin) {
        startAudio();
    }
    statePublishPending = true;
    sendOk();
}

void handlePostEvents() {
    if (!authorized()) {
        return;
    }
    DynamicJsonDocument doc(12288);
    if (deserializeJson(doc, httpServer.arg("plain"))) {
        sendError("invalid json");
        return;
    }
    if (!eventsFromJson(doc.as<JsonVariantConst>())) {
        sendError("no events found");
        return;
    }
    saveEvents();
    sendOk();
}

void handlePlay() {
    if (!authorized()) {
        return;
    }
    const int track = httpServer.arg("track").toInt();
    if (track <= 0 || track > 2999 || !audio.isStarted()) {
        sendError("invalid track or no DFPlayer");
        return;
    }
    playTrack(track, true, httpServer.arg("volume").toInt());
    sendOk();
}

void handleStop() {
    if (!authorized()) {
        return;
    }
    audio.stop();
    stopShow();
    sendOk();
}

void handleAnimation() {
    if (!authorized()) {
        return;
    }
    if (!playAnimation(httpServer.arg("name"))) {
        sendError("unknown animation");
        return;
    }
    sendOk();
}

void handleAnnounce() {
    if (!authorized()) {
        return;
    }
    if (!announce(httpServer.arg("what"))) {
        sendError("unknown announcement or no DFPlayer");
        return;
    }
    sendOk();
}

void handleDfReset() {
    if (!authorized()) {
        return;
    }
    audio.reset();
    sendOk();
}

// Compatible with the /setEvent request of Manfred's firmware:
// /setEvent?text=Hallo&audio=701&preani=HERZ&postani=HERZ
void handleSetEvent() {
    if (!authorized()) {
        return;
    }
    startShow(AnimationPlayer::find(httpServer.arg("preani").c_str()),
              httpServer.arg("text"),
              AnimationPlayer::find(httpServer.arg("postani").c_str()),
              httpServer.arg("audio").toInt());
    sendOk();
}

void registerWebRoutes() {
    httpServer.on("/extras", HTTP_GET, handlePage);
    httpServer.on("/api/extras", HTTP_GET, handleGetAll);
    httpServer.on("/api/extras/settings", HTTP_POST, handlePostSettings);
    httpServer.on("/api/extras/events", HTTP_POST, handlePostEvents);
    httpServer.on("/api/extras/play", HTTP_GET, handlePlay);
    httpServer.on("/api/extras/stop", HTTP_GET, handleStop);
    httpServer.on("/api/extras/animation", HTTP_GET, handleAnimation);
    httpServer.on("/api/extras/announce", HTTP_GET, handleAnnounce);
    httpServer.on("/api/extras/dfreset", HTTP_GET, handleDfReset);
    httpServer.on("/setEvent", HTTP_GET, handleSetEvent);
}

//------------------------------------------------------------------------------
// MQTT helpers
//------------------------------------------------------------------------------

String topic(const char *suffix) {
    return String(G.mqtt.topic) + "/extras/" + suffix;
}

const char *onOff(bool value) { return value ? "ON" : "OFF"; }

bool parseOnOff(const char *payload, bool &out) {
    if (strcasecmp(payload, "ON") == 0 || strcmp(payload, "1") == 0 ||
        strcasecmp(payload, "true") == 0) {
        out = true;
        return true;
    }
    if (strcasecmp(payload, "OFF") == 0 || strcmp(payload, "0") == 0 ||
        strcasecmp(payload, "false") == 0) {
        out = false;
        return true;
    }
    return false;
}

const char *todaysChimeName() {
    struct tm tm;
    const uint8_t day = localTime(tm) ? tm.tm_wday : 0;
    const uint8_t set = cfg.chimeSet[day % 7];
    return set < SOUND_SET_COUNT ? SOUND_SETS[set].name : SOUND_SETS[0].name;
}

// The component id is passed as String: ArduinoJson copies String keys, while
// it only stores the pointer of a const char* key.
JsonObject addComponent(JsonObject components, const String &deviceId,
                        const String &id, const char *platform,
                        const char *name, const char *icon) {
    JsonObject c = components.createNestedObject(id);
    c["p"] = platform;
    c["uniq_id"] = deviceId + "_" + id;
    c["name"] = name;
    if (icon) {
        c["ic"] = icon;
    }
    return c;
}

void addSwitch(JsonObject cmps, const String &id, const String &base,
               const char *key, const char *name, const char *icon) {
    JsonObject c =
        addComponent(cmps, id, String("extras_") + key, "switch", name, icon);
    c["stat_t"] = base + "/extras/state";
    c["val_tpl"] = String("{{ value_json.") + key + " }}";
    c["cmd_t"] = base + "/extras/" + key + "/set";
    c["pl_on"] = "ON";
    c["pl_off"] = "OFF";
}

void addButton(JsonObject cmps, const String &id, const String &base,
               const char *componentId, const char *command,
               const char *payload, const char *name, const char *icon) {
    JsonObject c = addComponent(cmps, id, componentId, "button", name, icon);
    c["cmd_t"] = base + "/extras/" + command + "/set";
    c["pl_prs"] = payload;
}

void addClimateSensor(JsonObject cmps, const String &id, const String &base,
                      const char *key, const char *name,
                      const char *deviceClass, const char *unit) {
    JsonObject c = addComponent(cmps, id, String("extras_") + key, "sensor",
                                name, nullptr);
    c["stat_t"] = base + "/extras/climate";
    c["val_tpl"] = String("{{ value_json.") + key + " }}";
    c["dev_cla"] = deviceClass;
    c["unit_of_meas"] = unit;
    c["stat_cla"] = "measurement";
}

void publishClimate() {
    if (!sensor.isPresent() || !mqttClient.connected()) {
        return;
    }
    StaticJsonDocument<192> doc;
    climateToJson(doc.to<JsonObject>());
    char buffer[192];
    serializeJson(doc, buffer);
    mqttClient.publish(topic("climate").c_str(), buffer, true);
}

} // namespace

//------------------------------------------------------------------------------
// Public interface
//------------------------------------------------------------------------------

void setup() {
    prefs.begin("extras", false);
    loadSettings();

    startAudio();
    sensor.begin();
    registerWebRoutes();

    if (cfg.startupSound && soundAllowedNow()) {
        playTrack(SOUND_STARTUP);
    }
    Serial.printf("Extras: %u animations, %u events\n",
                  AnimationPlayer::count(),
                  static_cast<unsigned>(events.size()));
}

//------------------------------------------------------------------------------

void loop() {
    audio.loop();
    player.loop();
    loopShow();

    struct tm tm;
    if (localTime(tm)) {
        if (lastMinute < 0) {
            lastMinute = tm.tm_min; // do not trigger anything right after boot
        } else if (tm.tm_min != lastMinute) {
            lastMinute = tm.tm_min;
            onNewMinute(tm);
        }
    }

    const uint32_t now = millis();
    if (sensor.isPresent() && now - lastSensorRead >= 10000) {
        lastSensorRead = now;
        sensor.read();
    }
    if (now - lastClimatePublish >= 60000) {
        lastClimatePublish = now;
        publishClimate();
    }
    if (statePublishPending && G.mqtt.state && mqttClient.connected()) {
        statePublishPending = false;
        mqttPublishState();
    }
}

//------------------------------------------------------------------------------

bool ownsDisplay() { return player.isRunning(); }

const char *scrollingTextOverride() {
    return show.textShown ? show.text : nullptr;
}

//------------------------------------------------------------------------------
// MQTT / Home Assistant
//------------------------------------------------------------------------------

void mqttSubscribe() { mqttClient.subscribe(topic("+/set").c_str()); }

bool mqttHandle(const String &fullTopic, const char *payload) {
    const String prefix = String(G.mqtt.topic) + "/extras/";
    if (!fullTopic.startsWith(prefix) || !fullTopic.endsWith("/set")) {
        return false;
    }
    const String key =
        fullTopic.substring(prefix.length(), fullTopic.length() - 4);
    bool changed = false;
    bool value;

    if (key == "sound" && parseOnOff(payload, value)) {
        cfg.sound = value;
        if (!value) {
            audio.stop();
        }
        changed = true;
    } else if (key == "chime" && parseOnOff(payload, value)) {
        cfg.chime = value;
        changed = true;
    } else if (key == "time_animations" && parseOnOff(payload, value)) {
        cfg.timeAnimations = value;
        changed = true;
    } else if (key == "events" && parseOnOff(payload, value)) {
        cfg.events = value;
        changed = true;
    } else if (key == "quiet" && parseOnOff(payload, value)) {
        cfg.quiet = value;
        changed = true;
    } else if (key == "volume") {
        cfg.volume = constrain(atoi(payload), 0, 100);
        changed = true;
    } else if (key == "chime_sound") {
        for (uint8_t i = 0; i < SOUND_SET_COUNT; i++) {
            if (strcmp(SOUND_SETS[i].name, payload) == 0) {
                for (uint8_t &day : cfg.chimeSet) {
                    day = i;
                }
                changed = true;
                break;
            }
        }
    } else if (key == "voice") {
        cfg.voice = strcmp(payload, "Vicki") == 0 ? 0 : 1;
        changed = true;
    } else if (key == "announce") {
        announce(payload);
    } else if (key == "stop") {
        audio.stop();
        stopShow();
    } else if (key == "play") {
        const int track = atoi(payload);
        if (track > 0 && track <= 2999) {
            playTrack(track, true);
        }
    } else if (key == "animation") {
        playAnimation(payload);
    } else if (key == "event") {
        // {"text": "...", "sound": 726, "pre": "HERZ", "post": "HERZ"}
        // (also accepts Manfred's names audio / preani / postani)
        StaticJsonDocument<384> doc;
        if (!deserializeJson(doc, payload)) {
            const char *pre = doc["pre"] | (doc["preani"] | "");
            const char *post = doc["post"] | (doc["postani"] | "");
            const int sound = doc["sound"] | (doc["audio"] | 0);
            startShow(AnimationPlayer::find(pre), doc["text"] | "",
                      AnimationPlayer::find(post), sound);
        }
    }

    if (changed) {
        saveSettings();
    }
    mqttPublishState();
    return true;
}

void mqttAddDiscovery(JsonObject cmps, const String &id, const String &base) {
    addSwitch(cmps, id, base, "sound", "Sound", "mdi:volume-high");
    addSwitch(cmps, id, base, "chime", "Stundengong", "mdi:bell-ring");
    addSwitch(cmps, id, base, "time_animations", "Uhrzeit-Animationen",
              "mdi:animation-play");
    addSwitch(cmps, id, base, "events", "Ereignisse", "mdi:calendar-star");
    addSwitch(cmps, id, base, "quiet", "Nachtruhe", "mdi:sleep");

    {
        JsonObject c = addComponent(cmps, id, "extras_volume", "number",
                                    "Lautstärke", "mdi:volume-medium");
        c["stat_t"] = base + "/extras/state";
        c["val_tpl"] = "{{ value_json.volume }}";
        c["cmd_t"] = base + "/extras/volume/set";
        c["min"] = 0;
        c["max"] = 100;
        c["step"] = 5;
        c["unit_of_meas"] = "%";
    }
    {
        JsonObject c = addComponent(cmps, id, "extras_play", "number",
                                    "Sound abspielen (Nr.)", "mdi:music-note");
        c["cmd_t"] = base + "/extras/play/set";
        c["stat_t"] = base + "/extras/state";
        c["val_tpl"] = "{{ value_json.track }}";
        c["min"] = 0;
        c["max"] = 2999;
        c["mode"] = "box";
    }
    {
        JsonObject c = addComponent(cmps, id, "extras_chime_sound", "select",
                                    "Gong-Klang", "mdi:bell-cog");
        c["ent_cat"] = "config";
        c["stat_t"] = base + "/extras/state";
        c["val_tpl"] = "{{ value_json.chime_sound }}";
        c["cmd_t"] = base + "/extras/chime_sound/set";
        JsonArray options = c.createNestedArray("options");
        for (const SoundSet &s : SOUND_SETS) {
            options.add(s.name);
        }
    }
    {
        JsonObject c = addComponent(cmps, id, "extras_voice", "select",
                                    "Sprecher", "mdi:account-voice");
        c["ent_cat"] = "config";
        c["stat_t"] = base + "/extras/state";
        c["val_tpl"] = "{{ value_json.voice }}";
        c["cmd_t"] = base + "/extras/voice/set";
        JsonArray options = c.createNestedArray("options");
        options.add("Vicki");
        options.add("Hans");
    }
    {
        JsonObject c =
            addComponent(cmps, id, "extras_animation", "select",
                         "Animation abspielen", "mdi:movie-open-play");
        c["stat_t"] = base + "/extras/state";
        c["val_tpl"] = "{{ value_json.animation }}";
        c["cmd_t"] = base + "/extras/animation/set";
        JsonArray options = c.createNestedArray("options");
        options.add("-");
        for (uint8_t i = 0; i < AnimationPlayer::count(); i++) {
            options.add(AnimationPlayer::name(i));
        }
    }

    addButton(cmps, id, base, "extras_announce_time", "announce", "time",
              "Uhrzeit ansagen", "mdi:clock-time-four-outline");
    addButton(cmps, id, base, "extras_announce_date", "announce", "date",
              "Datum ansagen", "mdi:calendar-today");
    addButton(cmps, id, base, "extras_stop", "stop", "stop",
              "Sound/Animation stoppen", "mdi:stop");

    if (sensor.isPresent()) {
        addButton(cmps, id, base, "extras_announce_climate", "announce",
                  "climate", "Raumklima ansagen", "mdi:thermometer-lines");
        addClimateSensor(cmps, id, base, "temperature", "Temperatur",
                         "temperature", "°C");
        if (sensor.hasHumidity()) {
            addClimateSensor(cmps, id, base, "humidity", "Luftfeuchtigkeit",
                             "humidity", "%");
        }
        addClimateSensor(cmps, id, base, "pressure", "Luftdruck",
                         "atmospheric_pressure", "hPa");
    }
}

void mqttPublishState() {
    if (!mqttClient.connected()) {
        return;
    }
    StaticJsonDocument<384> doc;
    doc["sound"] = onOff(cfg.sound);
    doc["chime"] = onOff(cfg.chime);
    doc["time_animations"] = onOff(cfg.timeAnimations);
    doc["events"] = onOff(cfg.events);
    doc["quiet"] = onOff(cfg.quiet);
    doc["volume"] = cfg.volume;
    doc["chime_sound"] = todaysChimeName();
    doc["voice"] = cfg.voice == 0 ? "Vicki" : "Hans";
    doc["animation"] = player.isRunning() ? player.currentName() : "-";
    doc["track"] = audio.currentTrack();
    doc["dfplayer"] = audio.isStarted() ? "ON" : "OFF";
    char buffer[384];
    serializeJson(doc, buffer);
    mqttClient.publish(topic("state").c_str(), buffer, true);
    publishClimate();
}

} // namespace extras

#endif
