#include "Extras/DfPlayer.h"

#if EXTRAS_ENABLED

namespace {
// DFPlayer serial commands (see DFPlayer Mini data sheet)
constexpr uint8_t CMD_SET_VOLUME = 0x06;
constexpr uint8_t CMD_SET_EQ = 0x07;
constexpr uint8_t CMD_RESET = 0x0C;
constexpr uint8_t CMD_PLAY_MP3_FOLDER = 0x12;
constexpr uint8_t CMD_STOP = 0x16;

HardwareSerial dfSerial(1);
} // namespace

//------------------------------------------------------------------------------

void DfPlayer::begin(uint8_t rxPin, uint8_t txPin, uint8_t busy) {
    end();
    if (rxPin == PIN_NONE || txPin == PIN_NONE || rxPin == txPin) {
        Serial.println("DFPlayer: disabled (no pins configured)");
        return;
    }

    busyPin = busy;
    if (busyPin != PIN_NONE) {
        pinMode(busyPin, INPUT_PULLUP);
    }

    dfSerial.begin(9600, SERIAL_8N1, rxPin, txPin);
    serial = &dfSerial;
    started = true;
    lastCommand = millis();

    // The module needs a moment after power-up before it accepts commands.
    delay(300);
    sendCommand(CMD_SET_EQ, 0);
    delay(MIN_COMMAND_GAP_MS);
    Serial.printf("DFPlayer: started (RX %u, TX %u, BUSY %u)\n", rxPin, txPin,
                  busyPin);
}

//------------------------------------------------------------------------------

void DfPlayer::end() {
    if (serial != nullptr) {
        serial->end();
        serial = nullptr;
    }
    started = false;
    queueRead = queueWrite = 0;
    state = State::Idle;
    lastVolume = 255;
}

//------------------------------------------------------------------------------

void DfPlayer::sendCommand(uint8_t command, uint16_t parameter) {
    if (serial == nullptr) {
        return;
    }
    uint8_t frame[10] = {0x7E,
                         0xFF,
                         0x06,
                         command,
                         0x00,
                         static_cast<uint8_t>(parameter >> 8),
                         static_cast<uint8_t>(parameter & 0xFF),
                         0x00,
                         0x00,
                         0xEF};
    uint16_t sum = 0;
    for (uint8_t i = 1; i < 7; i++) {
        sum += frame[i];
    }
    const uint16_t checksum = static_cast<uint16_t>(0 - sum);
    frame[7] = checksum >> 8;
    frame[8] = checksum & 0xFF;
    serial->write(frame, sizeof(frame));
    lastCommand = millis();
}

//------------------------------------------------------------------------------

bool DfPlayer::busyPinSignalsPlaying() const {
    return busyPin != PIN_NONE && digitalRead(busyPin) == LOW;
}

//------------------------------------------------------------------------------

bool DfPlayer::enqueue(uint16_t track, uint8_t volume30) {
    if (!started) {
        return false;
    }
    const uint8_t next = (queueWrite + 1) % QUEUE_SIZE;
    if (next == queueRead) {
        return false; // full
    }
    queue[queueWrite].track = track;
    queue[queueWrite].volume = volume30 > 30 ? 30 : volume30;
    queueWrite = next;
    return true;
}

//------------------------------------------------------------------------------

void DfPlayer::playNow(uint16_t track, uint8_t volume30) {
    queueRead = queueWrite = 0;
    if (state != State::Idle) {
        sendCommand(CMD_STOP, 0);
        state = State::Idle;
    }
    enqueue(track, volume30);
}

//------------------------------------------------------------------------------

void DfPlayer::stop() {
    queueRead = queueWrite = 0;
    if (started) {
        sendCommand(CMD_STOP, 0);
    }
    state = State::Idle;
    playingTrack = 0;
}

//------------------------------------------------------------------------------

void DfPlayer::reset() {
    queueRead = queueWrite = 0;
    state = State::Idle;
    lastVolume = 255;
    if (started) {
        sendCommand(CMD_RESET, 0);
        lastCommand = millis() + 2000; // module reboots, keep quiet
    }
}

//------------------------------------------------------------------------------

bool DfPlayer::isActive() const {
    return state != State::Idle || queueRead != queueWrite;
}

//------------------------------------------------------------------------------

void DfPlayer::loop() {
    if (!started) {
        return;
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - lastCommand) < MIN_COMMAND_GAP_MS) {
        return;
    }

    switch (state) {
    case State::Idle: {
        if (queueRead == queueWrite) {
            return;
        }
        const Entry &entry = queue[queueRead];
        if (entry.volume != lastVolume && !volumeSent) {
            sendCommand(CMD_SET_VOLUME, entry.volume);
            lastVolume = entry.volume;
            volumeSent = true;
            return; // play in the next round, the module needs a short gap
        }
        volumeSent = false;
        playingTrack = entry.track;
        queueRead = (queueRead + 1) % QUEUE_SIZE;
        sendCommand(CMD_PLAY_MP3_FOLDER, playingTrack);
        state = State::WaitForStart;
        stateSince = now;
        idleReads = 0;
        break;
    }

    case State::WaitForStart: {
        if (busyPin == PIN_NONE) {
            if (now - stateSince >= NO_BUSY_PIN_TRACK_MS) {
                state = State::Idle;
                playingTrack = 0;
            }
        } else if (busyPinSignalsPlaying()) {
            state = State::Playing;
            stateSince = now;
        } else if (now - stateSince >= START_TIMEOUT_MS) {
            // Missing file or no BUSY signal: continue with the next track.
            Serial.printf("DFPlayer: track %u did not start\n", playingTrack);
            state = State::Idle;
            playingTrack = 0;
        }
        break;
    }

    case State::Playing: {
        // BUSY goes HIGH when the track has ended; read it twice to ignore
        // short glitches.
        if (!busyPinSignalsPlaying()) {
            if (++idleReads >= 2) {
                state = State::Idle;
                playingTrack = 0;
            }
        } else {
            idleReads = 0;
        }
        if (now - stateSince > MAX_TRACK_MS) {
            stop();
        }
        lastCommand = now; // poll the BUSY pin every MIN_COMMAND_GAP_MS
        break;
    }
    }
}

#endif
