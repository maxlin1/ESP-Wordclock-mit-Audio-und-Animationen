#include "Extras/Extras.h"

#if EXTRAS_ENABLED

#include "Config.h" // WEB_PROTECTED, WEB_USER, WEB_PASSWORD
#include "WordClockState.h"
#include "WordClockTypes/ClockType.hpp"

#include "Extras/AnimationPlayer.h"
#include "Extras/Bme280.h"
#include "Extras/DfPlayer.h"
#include "Led.h"

#include <Preferences.h>
#include <PubSubClient.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>
#include <vector>

#ifndef EXTRAS_BUILD
#define EXTRAS_BUILD "dev"
#endif

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
    // Animation at the full hour: "" = off, "Z" = time animations "Zhhmm"
    // (also at other minutes), "*" = random animation, otherwise a name.
    String hourAnimation = "Z";
    bool events = true;
    bool startupSound = true;
    uint8_t rxPin = EXTRAS_DFPLAYER_RX_PIN;
    uint8_t txPin = EXTRAS_DFPLAYER_TX_PIN;
    uint8_t busyPin = EXTRAS_DFPLAYER_BUSY_PIN;
    int16_t altitude = 0; // m above sea level, for the relative pressure
    float temperatureOffset = 0.0f;
    // automatic color change of the clock face
    uint8_t colorAuto = 0;   // see COLOR_AUTO_*
    uint16_t colorFade = 60; // seconds to fade to the next color
    uint8_t colorCycle = 24; // hours for one round through all colors
};

enum ColorAuto : uint8_t {
    COLOR_AUTO_OFF = 0,
    COLOR_AUTO_5_MINUTES = 1,
    COLOR_AUTO_HOURLY = 2,
    COLOR_AUTO_DAILY = 3,
    COLOR_AUTO_CONTINUOUS = 4,
};

struct Event {
    uint8_t month = 1;
    uint8_t day = 1;
    uint16_t year = 0; // 0 = every year; otherwise "YY" in the text = age
    String text;
    uint16_t sound = 0;
    String pre;       // animation before the text
    String post;      // animation after the text
    uint8_t hour = 8; // time of the first show on that day
    uint8_t minute = 0;
    uint8_t endHour = 23; // no repetitions after this time
    uint8_t endMinute = 59;
    uint8_t interval = 0; // repeat every n minutes, 0 = only once
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

// Automatic sounds (chime, events, startup) only play while the clock is
// switched on (also via MQTT / Home Assistant) and outside the quiet hours.
bool soundAllowedNow() {
    if (!cfg.sound || !audio.isStarted() || !led.getState()) {
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

//------------------------------------------------------------------------------
// Random sound from a category: numbers from 3001 on stand for "a random
// track of this category" (e.g. 3001 = a random birthday song).
//------------------------------------------------------------------------------

struct TrackRange {
    uint16_t code;
    uint16_t first;
    uint16_t last;
};

constexpr uint16_t RANDOM_SOUND_BASE = 3000;

const TrackRange RANDOM_SOUND_RANGES[] = {
    {3001, 720, 726}, // Geburtstag
    {3002, 710, 719}, // Weihnachten
    {3003, 730, 733}, // Silvester
    {3004, 761, 769}, // Natur
    {3005, 811, 839}, // Tiere
    {3006, 841, 869}, // Musik (no 870 in the sound package)
    {3006, 871, 897}, // Musik
};

bool isRandomSound(uint16_t code) {
    for (const TrackRange &r : RANDOM_SOUND_RANGES) {
        if (r.code == code) {
            return true;
        }
    }
    return false;
}

bool isValidSound(int track) {
    return (track > 0 && track < RANDOM_SOUND_BASE) || isRandomSound(track);
}

// Returns the track to play: a random track of the category for the codes
// above, otherwise the number itself.
uint16_t resolveTrack(uint16_t track) {
    uint16_t count = 0;
    for (const TrackRange &r : RANDOM_SOUND_RANGES) {
        if (r.code == track) {
            count += r.last - r.first + 1;
        }
    }
    if (count == 0) {
        return track;
    }
    uint16_t pick = random(0, count);
    for (const TrackRange &r : RANDOM_SOUND_RANGES) {
        if (r.code != track) {
            continue;
        }
        const uint16_t size = r.last - r.first + 1;
        if (pick < size) {
            return r.first + pick;
        }
        pick -= size;
    }
    return track;
}

void playTrack(uint16_t track, bool interrupt = false, uint8_t volume100 = 0) {
    if (track == 0 || !audio.isStarted()) {
        return;
    }
    track = resolveTrack(track);
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

void playChime(const struct tm &tm);

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
    } else if (what == "chime") {
        struct tm tm;
        if (!localTime(tm)) {
            return false;
        }
        playChime(tm);
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
        const uint16_t start = e.hour * 60 + e.minute;
        const uint16_t end = e.endHour * 60 + e.endMinute;
        if (minuteOfDay < start || minuteOfDay > end) {
            continue;
        }
        if (e.interval == 0 ? minuteOfDay == start
                            : (minuteOfDay - start) % e.interval == 0) {
            return &e;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
// Automatic color change of the clock face (like "Farbwechsel" of the old
// clock): a new color every 5 minutes / hour / day, faded in slowly, or a
// continuous, very slow walk through all colors.
//------------------------------------------------------------------------------

struct ColorFade {
    bool active = false;
    float from = 0;
    float delta = 0;
    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t lastStep = 0;
};

ColorFade colorFade;
uint32_t lastContinuousStep = 0;

bool colorAutoAllowed() {
    return led.getState() && clockFaceVisible() && !player.isRunning();
}

void setForegroundHue(float hue) {
    HsbColor color = G.color[Foreground];
    color.H = hue - floorf(hue);
    if (color.S < 0.3f) {
        color.S = 1.0f; // white would not show any change
    }
    G.color[Foreground] = color;
    parametersChanged = true;
}

void startColorChange() {
    const float from = G.color[Foreground].H;
    // the next color is at least 60 degrees away on the color wheel
    const float to = from + 0.17f + random(0, 67) / 100.0f;
    if (cfg.colorFade == 0) {
        setForegroundHue(to);
        return;
    }
    colorFade.active = true;
    colorFade.from = from;
    colorFade.delta = to - from;
    colorFade.start = millis();
    colorFade.duration = cfg.colorFade * 1000UL;
    colorFade.lastStep = 0;
}

void colorChangeOnNewMinute(const struct tm &tm) {
    if (!colorAutoAllowed()) {
        return;
    }
    switch (cfg.colorAuto) {
    case COLOR_AUTO_5_MINUTES:
        if (tm.tm_min % 5 == 0) {
            startColorChange();
        }
        break;
    case COLOR_AUTO_HOURLY:
        if (tm.tm_min == 0) {
            startColorChange();
        }
        break;
    case COLOR_AUTO_DAILY:
        if (tm.tm_hour == 0 && tm.tm_min == 0) {
            startColorChange();
        }
        break;
    default:
        break;
    }
}

void loopColor() {
    const uint32_t now = millis();

    if (cfg.colorAuto == COLOR_AUTO_CONTINUOUS) {
        colorFade.active = false;
        if (now - lastContinuousStep >= 2000 && colorAutoAllowed() &&
            timeIsValid()) {
            lastContinuousStep = now;
            const double cycle = cfg.colorCycle * 3600.0;
            const float hue =
                fmod(static_cast<double>(time(nullptr)), cycle) / cycle;
            if (fabsf(hue - G.color[Foreground].H) > 0.001f) {
                setForegroundHue(hue);
            }
        }
        return;
    }

    if (!colorFade.active || now - colorFade.lastStep < 1000) {
        return;
    }
    colorFade.lastStep = now;
    float progress = static_cast<float>(now - colorFade.start) /
                     static_cast<float>(colorFade.duration);
    if (progress >= 1.0f || cfg.colorAuto == COLOR_AUTO_OFF) {
        progress = 1.0f;
        colorFade.active = false;
    }
    if (colorAutoAllowed()) {
        setForegroundHue(colorFade.from + colorFade.delta * progress);
    }
}

//------------------------------------------------------------------------------
// Things that happen when the minute changes
//------------------------------------------------------------------------------

void onNewMinute(const struct tm &tm) {
    colorChangeOnNewMinute(tm);

    if (tm.tm_min == 0 && cfg.chime && soundAllowedNow()) {
        playChime(tm);
    }

    if (!clockFaceVisible() || show.step != ShowStep::Idle) {
        return;
    }

    if (const Event *e = dueEvent(tm)) {
        runEvent(*e, tm);
        return;
    }

    if (cfg.hourAnimation == "Z") {
        // Manfred's time animations, e.g. "Z0800" at 08:00
        char name[8];
        snprintf(name, sizeof(name), "Z%02d%02d", tm.tm_hour, tm.tm_min);
        playAnimation(name);
    } else if (tm.tm_min == 0 && cfg.hourAnimation == "*") {
        // random animation, but not the time animations "Zhhmm"
        for (uint8_t tries = 0; tries < 20; tries++) {
            const uint8_t i = random(0, AnimationPlayer::count());
            if (AnimationPlayer::name(i)[0] != 'Z') {
                startShow(i, "", -1, 0);
                break;
            }
        }
    } else if (tm.tm_min == 0 && cfg.hourAnimation.length() > 0) {
        playAnimation(cfg.hourAnimation);
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
    o["hourAnimation"] = cfg.hourAnimation;
    o["events"] = cfg.events;
    o["startupSound"] = cfg.startupSound;
    o["rxPin"] = cfg.rxPin;
    o["txPin"] = cfg.txPin;
    o["busyPin"] = cfg.busyPin;
    o["altitude"] = cfg.altitude;
    o["temperatureOffset"] = cfg.temperatureOffset;
    o["colorAuto"] = cfg.colorAuto;
    o["colorFade"] = cfg.colorFade;
    o["colorCycle"] = cfg.colorCycle;
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
    if (o.containsKey("hourAnimation")) {
        cfg.hourAnimation =
            String(static_cast<const char *>(o["hourAnimation"] | ""))
                .substring(0, 20);
    } else if (o.containsKey("timeAnimations")) { // older settings
        cfg.hourAnimation = (o["timeAnimations"] | false) ? "Z" : "";
    }
    cfg.events = o["events"] | cfg.events;
    cfg.startupSound = o["startupSound"] | cfg.startupSound;
    cfg.rxPin = pinOrNone(o["rxPin"], cfg.rxPin);
    cfg.txPin = pinOrNone(o["txPin"], cfg.txPin);
    cfg.busyPin = pinOrNone(o["busyPin"], cfg.busyPin);
    cfg.altitude =
        constrain(o["altitude"] | static_cast<int>(cfg.altitude), -500, 9000);
    cfg.temperatureOffset = constrain(
        o["temperatureOffset"] | cfg.temperatureOffset, -20.0f, 20.0f);
    cfg.colorAuto =
        constrain(o["colorAuto"] | static_cast<int>(cfg.colorAuto), 0, 4);
    cfg.colorFade =
        constrain(o["colorFade"] | static_cast<int>(cfg.colorFade), 0, 3600);
    cfg.colorCycle =
        constrain(o["colorCycle"] | static_cast<int>(cfg.colorCycle), 1, 168);
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
        o["hour"] = e.hour;
        o["minute"] = e.minute;
        o["endHour"] = e.endHour;
        o["endMinute"] = e.endMinute;
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
        if (manfred) {
            // The old clock showed events all day long in its interval.
            e.hour = (e.month == 1 && e.day == 1) ? 0 : 8;
            e.minute = 0;
            e.interval = constrain(o["intervall"] | 60, 1, 255);
        } else {
            e.hour = constrain(o["hour"] | 0, 0, 23);
            e.minute = constrain(o["minute"] | 0, 0, 59);
            e.endHour = constrain(o["endHour"] | 23, 0, 23);
            e.endMinute = constrain(o["endMinute"] | 59, 0, 59);
            e.interval = constrain(o["interval"] | 0, 0, 255);
        }
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
        e.hour = (m == 1 && d == 1) ? 0 : 8;
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
// Web API on port 81. The settings are shown on the clock's main page
// (port 80), so the responses allow cross-origin requests.
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

void allowCrossOrigin() {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendJson(const JsonDocument &doc, int code = 200) {
    String out;
    serializeJson(doc, out);
    allowCrossOrigin();
    httpServer.send(code, "application/json", out);
}

void sendOk() {
    allowCrossOrigin();
    httpServer.send(200, "application/json", "{\"ok\":true}");
}

void sendError(const char *message) {
    String out = String("{\"ok\":false,\"error\":\"") + message + "\"}";
    allowCrossOrigin();
    httpServer.send(400, "application/json", out);
}

void handleOptions() {
    allowCrossOrigin();
    httpServer.send(204);
}

//------------------------------------------------------------------------------
// Firmware update from the clock's main page (Settings > Firmware update).
// Unlike the built-in page on /update it answers with JSON and allows
// cross-origin requests, so the page can show the progress.
//------------------------------------------------------------------------------

String updateError;

void handleUpdateUpload() {
    HTTPUpload &upload = httpServer.upload();
    switch (upload.status) {
    case UPLOAD_FILE_START:
        updateError = "";
#if WEB_PROTECTED
        if (!httpServer.authenticate(WEB_USER, WEB_PASSWORD)) {
            updateError = "not authorized";
            break;
        }
#endif
        Serial.printf("Firmware update: %s\n", upload.filename.c_str());
        audio.stop();
        stopShow();
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
            updateError = Update.errorString();
        }
        break;
    case UPLOAD_FILE_WRITE:
        if (updateError.isEmpty() &&
            Update.write(upload.buf, upload.currentSize) !=
                upload.currentSize) {
            updateError = Update.errorString();
        }
        break;
    case UPLOAD_FILE_END:
        if (updateError.isEmpty() && !Update.end(true)) {
            updateError = Update.errorString();
        }
        break;
    case UPLOAD_FILE_ABORTED:
        Update.abort();
        updateError = "upload aborted";
        break;
    default:
        break;
    }
}

void handleUpdateDone() {
    if (!updateError.isEmpty()) {
        Serial.printf("Firmware update failed: %s\n", updateError.c_str());
        sendError(updateError.c_str());
        return;
    }
    Serial.println("Firmware update done, restarting");
    sendOk();
    delay(500);
    ESP.restart();
}

// The settings used to be on their own page; send old bookmarks to the clock.
void handleOldPage() {
    httpServer.sendHeader("Location",
                          "http://" + WiFi.localIP().toString() + "/", true);
    httpServer.send(302, "text/plain", "");
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
    status["firmware"] = String(VERSION) + " + Extras " + EXTRAS_BUILD;

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
    if (!isValidSound(track) || !audio.isStarted()) {
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

// Shows an event right now (button "Testen" on the web page).
void handleEventPreview() {
    if (!authorized()) {
        return;
    }
    const int index = httpServer.arg("index").toInt();
    struct tm tm;
    if (index < 0 || index >= static_cast<int>(events.size()) ||
        !localTime(tm)) {
        sendError("unknown event");
        return;
    }
    runEvent(events[index], tm);
    sendOk();
}

void registerWebRoutes() {
    httpServer.on("/extras", HTTP_GET, handleOldPage);
    httpServer.on("/api/extras/settings", HTTP_OPTIONS, handleOptions);
    httpServer.on("/api/extras/events", HTTP_OPTIONS, handleOptions);
    httpServer.on("/api/extras/update", HTTP_OPTIONS, handleOptions);
    httpServer.on("/api/extras/update", HTTP_POST, handleUpdateDone,
                  handleUpdateUpload);
    httpServer.on("/api/extras/event", HTTP_GET, handleEventPreview);
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

const char *const COLOR_AUTO_LABELS[] = {"Aus", "Alle 5 Minuten", "Stündlich",
                                         "Täglich", "Fließend"};
constexpr uint8_t COLOR_AUTO_LABEL_COUNT =
    sizeof(COLOR_AUTO_LABELS) / sizeof(COLOR_AUTO_LABELS[0]);

constexpr const char *HOUR_ANIMATION_OFF = "Aus";
constexpr const char *HOUR_ANIMATION_TIME = "Uhrzeit-Animationen";
constexpr const char *HOUR_ANIMATION_RANDOM = "Zufall";

const char *hourAnimationLabel() {
    if (cfg.hourAnimation.length() == 0) {
        return HOUR_ANIMATION_OFF;
    }
    if (cfg.hourAnimation == "Z") {
        return HOUR_ANIMATION_TIME;
    }
    if (cfg.hourAnimation == "*") {
        return HOUR_ANIMATION_RANDOM;
    }
    const int16_t i = AnimationPlayer::find(cfg.hourAnimation.c_str());
    return i >= 0 ? AnimationPlayer::name(i) : HOUR_ANIMATION_OFF;
}

String hourAnimationFromLabel(const char *label) {
    if (strcmp(label, HOUR_ANIMATION_TIME) == 0) {
        return "Z";
    }
    if (strcmp(label, HOUR_ANIMATION_RANDOM) == 0) {
        return "*";
    }
    const int16_t i = AnimationPlayer::find(label);
    return i >= 0 ? String(AnimationPlayer::name(i)) : String();
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
    loopColor();

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
    } else if (key == "hour_animation") {
        cfg.hourAnimation = hourAnimationFromLabel(payload);
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
    } else if (key == "color_auto") {
        for (uint8_t i = 0; i < COLOR_AUTO_LABEL_COUNT; i++) {
            if (strcmp(COLOR_AUTO_LABELS[i], payload) == 0) {
                cfg.colorAuto = i;
                changed = true;
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
        if (isValidSound(track)) {
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
        c["max"] = 3006; // 3001-3006: random sound of a category
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
        JsonObject c = addComponent(cmps, id, "extras_color_auto", "select",
                                    "Automatischer Farbwechsel", "mdi:palette");
        c["stat_t"] = base + "/extras/state";
        c["val_tpl"] = "{{ value_json.color_auto }}";
        c["cmd_t"] = base + "/extras/color_auto/set";
        JsonArray options = c.createNestedArray("options");
        for (const char *label : COLOR_AUTO_LABELS) {
            options.add(label);
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
            addComponent(cmps, id, "extras_hour_animation", "select",
                         "Animation zur vollen Stunde", "mdi:animation-play");
        c["ent_cat"] = "config";
        c["stat_t"] = base + "/extras/state";
        c["val_tpl"] = "{{ value_json.hour_animation }}";
        c["cmd_t"] = base + "/extras/hour_animation/set";
        JsonArray options = c.createNestedArray("options");
        options.add(HOUR_ANIMATION_OFF);
        options.add(HOUR_ANIMATION_TIME);
        options.add(HOUR_ANIMATION_RANDOM);
        for (uint8_t i = 0; i < AnimationPlayer::count(); i++) {
            if (AnimationPlayer::name(i)[0] != 'Z') {
                options.add(AnimationPlayer::name(i));
            }
        }
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
    StaticJsonDocument<512> doc;
    doc["sound"] = onOff(cfg.sound);
    doc["chime"] = onOff(cfg.chime);
    doc["hour_animation"] = hourAnimationLabel();
    doc["events"] = onOff(cfg.events);
    doc["quiet"] = onOff(cfg.quiet);
    doc["volume"] = cfg.volume;
    doc["chime_sound"] = todaysChimeName();
    doc["voice"] = cfg.voice == 0 ? "Vicki" : "Hans";
    doc["color_auto"] =
        COLOR_AUTO_LABELS[cfg.colorAuto % COLOR_AUTO_LABEL_COUNT];
    doc["animation"] = player.isRunning() ? player.currentName() : "-";
    doc["track"] = audio.currentTrack();
    doc["dfplayer"] = audio.isStarted() ? "ON" : "OFF";
    char buffer[512];
    serializeJson(doc, buffer);
    mqttClient.publish(topic("state").c_str(), buffer, true);
    publishClimate();
}

} // namespace extras

#endif
