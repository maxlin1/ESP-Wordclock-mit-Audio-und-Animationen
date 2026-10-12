#pragma once

#include "../ExtrasConfig.h"
#include <Arduino.h>

#if EXTRAS_ENABLED

/*
 * Minimal, non-blocking driver for the DFPlayer Mini mp3 module.
 *
 * Tracks are played from the folder "mp3" on the SD card, named
 * "0001_xyz.mp3" ... "2999_xyz.mp3" (the layout used by Manfred Hofmann's
 * Wortuhr_mp3 sound package). Several tracks can be queued, e.g. for a time
 * announcement made of "Es ist" + "acht Uhr" + "fünfzehn". The BUSY pin is used
 * to find out when a track has finished; without it a fixed pause is used.
 */
class DfPlayer {
public:
    static constexpr uint8_t PIN_NONE = 255;

    void begin(uint8_t rxPin, uint8_t txPin, uint8_t busyPin);
    void end();
    void loop();

    // Queue a track. volume30 is the DFPlayer volume 0-30.
    bool enqueue(uint16_t track, uint8_t volume30);
    // Stop what is playing, clear the queue and play this track.
    void playNow(uint16_t track, uint8_t volume30);
    void stop();
    void reset();

    bool isActive() const; // something is playing or queued
    bool isStarted() const { return started; }
    uint16_t currentTrack() const { return playingTrack; }

private:
    enum class State : uint8_t { Idle, WaitForStart, Playing };

    struct Entry {
        uint16_t track;
        uint8_t volume;
    };

    static constexpr uint8_t QUEUE_SIZE = 16;
    static constexpr uint16_t MIN_COMMAND_GAP_MS = 80;
    static constexpr uint16_t START_TIMEOUT_MS = 1500;
    static constexpr uint16_t NO_BUSY_PIN_TRACK_MS = 1300;
    static constexpr uint32_t MAX_TRACK_MS = 15UL * 60UL * 1000UL;

    void sendCommand(uint8_t command, uint16_t parameter);
    bool busyPinSignalsPlaying() const;

    HardwareSerial *serial = nullptr;
    uint8_t busyPin = PIN_NONE;
    bool started = false;

    Entry queue[QUEUE_SIZE];
    uint8_t queueRead = 0;
    uint8_t queueWrite = 0;

    State state = State::Idle;
    uint32_t stateSince = 0;
    uint32_t lastCommand = 0;
    uint8_t lastVolume = 255;
    uint16_t playingTrack = 0;
    uint8_t idleReads = 0;
    bool volumeSent = false;
};

#endif
