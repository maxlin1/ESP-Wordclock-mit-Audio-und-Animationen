#include "Extras/AnimationPlayer.h"

#if EXTRAS_ENABLED

#include "WordClockState.h"
#include "WordClockTypes/ClockType.hpp"

#include "Extras/AnimationData.gen.h"
#include "Led.h"

extern Led led;
extern ClockType *usedClockType;

namespace {
// In random mode one "loop" consists of this many random frames, like in the
// original firmware.
constexpr uint8_t RANDOM_FRAMES_PER_LOOP = 20;
constexpr uint8_t PIXELS_PER_FRAME = animationData::ROWS * animationData::COLS;
} // namespace

//------------------------------------------------------------------------------

uint8_t AnimationPlayer::count() { return animationData::COUNT; }

//------------------------------------------------------------------------------

const char *AnimationPlayer::name(uint8_t i) {
    return i < animationData::COUNT ? animationData::animations[i].name : "";
}

//------------------------------------------------------------------------------

int16_t AnimationPlayer::find(const char *wanted) {
    if (wanted == nullptr || wanted[0] == '\0') {
        return -1;
    }
    for (uint8_t i = 0; i < animationData::COUNT; i++) {
        if (strcasecmp(animationData::animations[i].name, wanted) == 0) {
            return i;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------

const char *AnimationPlayer::currentName() const {
    return running ? name(index) : "";
}

//------------------------------------------------------------------------------

bool AnimationPlayer::start(uint8_t i) {
    if (i >= animationData::COUNT || usedClockType == nullptr) {
        return false;
    }
    // The animations are made for a 10 x 11 matrix.
    if (usedClockType->rowsWordMatrix() < animationData::ROWS ||
        usedClockType->colsWordMatrix() < animationData::COLS) {
        Serial.println("Animation: layout too small, skipped");
        return false;
    }

    index = i;
    frame = 0;
    direction = 1;
    loopsDone = 0;
    randomSteps = 0;
    running = true;
    nextFrameAt = millis();

    led.clear();
    Serial.printf("Animation: start %s\n", name(index));
    return true;
}

//------------------------------------------------------------------------------

void AnimationPlayer::stop() {
    if (!running) {
        return;
    }
    running = false;
    led.clear();
    led.show();
    // Let the clock redraw its face right away (not only at the next second).
    if (G.prog == COMMAND_IDLE) {
        G.prog = COMMAND_MODE_WORD_CLOCK;
    }
    parametersChanged = true;
}

//------------------------------------------------------------------------------

void AnimationPlayer::drawFrame() {
    const animationData::Animation &a = animationData::animations[index];
    const uint8_t *pixels = a.pixels + frame * PIXELS_PER_FRAME;

    // Use the brightness of the clock face (incl. automatic brightness).
    const float brightness =
        led.getColorbyPositionWithAppliedBrightness(Foreground).B;

    for (uint8_t row = 0; row < animationData::ROWS; row++) {
        for (uint8_t col = 0; col < animationData::COLS; col++) {
            const uint8_t p =
                pgm_read_byte(&pixels[row * animationData::COLS + col]);
            const uint8_t r = pgm_read_byte(&a.palette[p * 3]);
            const uint8_t g = pgm_read_byte(&a.palette[p * 3 + 1]);
            const uint8_t b = pgm_read_byte(&a.palette[p * 3 + 2]);
            if (r == 0 && g == 0 && b == 0) {
                led.clearPixel(row, col);
                continue;
            }
            HsbColor color{RgbColor(r, g, b)};
            color.B *= brightness;
            led.setPixel(row, col, color);
        }
    }
    led.show();
}

//------------------------------------------------------------------------------

void AnimationPlayer::advance() {
    const animationData::Animation &a = animationData::animations[index];
    const uint8_t frames = a.frameCount;

    switch (a.mode) {
    case 1: // forward, then backward
        if (frames < 2) {
            loopsDone++;
            break;
        }
        frame += direction;
        if (direction > 0 && frame >= frames) {
            direction = -1;
            frame = frames - 2;
            loopsDone++;
        } else if (direction < 0 && frame == 0) {
            direction = 1;
            loopsDone++;
        }
        break;

    case 2: // random frames
        frame = random(0, frames);
        if (++randomSteps >= RANDOM_FRAMES_PER_LOOP) {
            randomSteps = 0;
            loopsDone++;
        }
        break;

    default: // forward
        if (++frame >= frames) {
            frame = 0;
            loopsDone++;
        }
        break;
    }
}

//------------------------------------------------------------------------------

bool AnimationPlayer::loop() {
    if (!running) {
        return false;
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextFrameAt) < 0) {
        return true;
    }

    const animationData::Animation &a = animationData::animations[index];
    if (loopsDone >= a.loops) {
        Serial.printf("Animation: %s finished\n", name(index));
        stop();
        return false;
    }

    drawFrame();
    nextFrameAt = now + pgm_read_word(&a.delays[frame]);
    advance();
    return true;
}

#endif
