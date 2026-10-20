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

// Words of Manfred's front (classic QlockTwo layout), which his time
// animations were drawn for: row, first and last column (0 = left).
struct Cells {
    uint8_t row, from, to;
};
const Cells SRC_ES_IST[] = {{0, 0, 1}, {0, 3, 5}};
const Cells SRC_UHR = {9, 8, 10};

// Reads the cells a word of the current layout lights (bit = column).
void layoutWord(FrontWord word, uint16_t *cells) {
    uint32_t saved[MAX_ROW_SIZE];
    memcpy(saved, frontMatrix, sizeof(saved));
    memset(frontMatrix, 0, sizeof(frontMatrix));
    usedClockType->show(word);
    for (uint8_t row = 0; row < animationData::ROWS; row++) {
        for (uint8_t col = 0; col < animationData::COLS; col++) {
            if (usedClockType->getFrontMatrixPixel(row, col)) {
                cells[row] |= 1U << col;
            }
        }
    }
    memcpy(frontMatrix, saved, sizeof(saved));
}

FrontWord hourWord(uint8_t hour) {
    static const FrontWord words[] = {
        FrontWord::hour_12, FrontWord::hour_1,  FrontWord::hour_2,
        FrontWord::hour_3,  FrontWord::hour_4,  FrontWord::hour_5,
        FrontWord::hour_6,  FrontWord::hour_7,  FrontWord::hour_8,
        FrontWord::hour_9,  FrontWord::hour_10, FrontWord::hour_11};
    return words[hour % 12];
}

bool isSet(const uint16_t *cells, uint8_t row, uint8_t col) {
    return (cells[row] >> col) & 1U;
}
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
    prepareTimeMorph();
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

void AnimationPlayer::setPixelRgb(uint8_t row, uint8_t col, uint8_t r,
                                  uint8_t g, uint8_t b, float brightness) {
    if (r == 0 && g == 0 && b == 0) {
        led.clearPixel(row, col);
        return;
    }
    HsbColor color{RgbColor(r, g, b)};
    color.B *= brightness;
    led.setPixel(row, col, color);
}

void AnimationPlayer::drawFrame() {
    const animationData::Animation &a = animationData::animations[index];
    const uint8_t *pixels = a.pixels + frame * PIXELS_PER_FRAME;

    // Use the brightness of the clock face (incl. automatic brightness).
    const float brightness =
        led.getColorbyPositionWithAppliedBrightness(Foreground).B;

    for (uint8_t row = 0; row < animationData::ROWS; row++) {
        for (uint8_t col = 0; col < animationData::COLS; col++) {
            if (morph && isSet(srcWordCells, row, col)) {
                led.clearPixel(row, col); // drawn by drawWords()
                continue;
            }
            const uint8_t p =
                pgm_read_byte(&pixels[row * animationData::COLS + col]);
            setPixelRgb(row, col, pgm_read_byte(&a.palette[p * 3]),
                        pgm_read_byte(&a.palette[p * 3 + 1]),
                        pgm_read_byte(&a.palette[p * 3 + 2]), brightness);
        }
    }
    if (morph) {
        drawWords(false);
    }
    led.show();
}

//------------------------------------------------------------------------------

void AnimationPlayer::drawWords(bool withHour) {
    const float brightness =
        led.getColorbyPositionWithAppliedBrightness(Foreground).B;
    for (uint8_t row = 0; row < animationData::ROWS; row++) {
        for (uint8_t col = 0; col < animationData::COLS; col++) {
            if (isSet(wordCells, row, col) ||
                (withHour && isSet(hourCells, row, col))) {
                setPixelRgb(row, col, wordR, wordG, wordB, brightness);
            }
        }
    }
}

//------------------------------------------------------------------------------

void AnimationPlayer::drawMorph() {
    const float brightness =
        led.getColorbyPositionWithAppliedBrightness(Foreground).B;
    led.clear();
    const bool done = morphStep > MORPH_STEPS;
    if (!done) {
        for (uint8_t i = 0; i < morphCount; i++) {
            const MorphPixel &m = morphPixels[i];
            const int row =
                m.row +
                ((m.toRow - m.row) * morphStep + MORPH_STEPS / 2) / MORPH_STEPS;
            const int col =
                m.col +
                ((m.toCol - m.col) * morphStep + MORPH_STEPS / 2) / MORPH_STEPS;
            setPixelRgb(row, col, m.r, m.g, m.b, brightness);
        }
    }
    drawWords(done);
    led.show();
}

//------------------------------------------------------------------------------

void AnimationPlayer::prepareTimeMorph() {
    morph = false;
    morphStep = 0;
    morphCount = 0;
    memset(wordCells, 0, sizeof(wordCells));
    memset(hourCells, 0, sizeof(hourCells));
    memset(srcWordCells, 0, sizeof(srcWordCells));

    const animationData::Animation &a = animationData::animations[index];
    // "Zhh00" with hh = 01..23 (Z0000 is a different kind of animation)
    const char *n = a.name;
    if (n[0] != 'Z' || strlen(n) != 5 || n[3] != '0' || n[4] != '0') {
        return;
    }
    const uint8_t hour = (n[1] - '0') * 10 + (n[2] - '0');
    if (hour == 0 || hour > 23 || a.frameCount < 2) {
        return;
    }

    for (const Cells &c : SRC_ES_IST) {
        for (uint8_t col = c.from; col <= c.to; col++) {
            srcWordCells[c.row] |= 1U << col;
        }
    }
    for (uint8_t col = SRC_UHR.from; col <= SRC_UHR.to; col++) {
        srcWordCells[SRC_UHR.row] |= 1U << col;
    }

    // The digits stand still in the first frames, then they start to move.
    const uint8_t *first = a.pixels;
    staticFrames = 1;
    while (staticFrames < a.frameCount &&
           memcmp_P(first, a.pixels + staticFrames * PIXELS_PER_FRAME,
                    PIXELS_PER_FRAME) == 0) {
        staticFrames++;
    }
    if (staticFrames >= a.frameCount) {
        return;
    }

    // Words of the selected layout
    layoutWord(FrontWord::es_ist, wordCells);
    layoutWord(FrontWord::uhr, wordCells);
    layoutWord(hourWord(hour), hourCells);
    uint8_t targets[2][MAX_MORPH_PIXELS];
    uint8_t targetCount = 0;
    for (uint8_t col = 0; col < animationData::COLS; col++) {
        for (uint8_t row = 0; row < animationData::ROWS; row++) {
            if (isSet(hourCells, row, col)) {
                targets[0][targetCount] = row;
                targets[1][targetCount] = col;
                targetCount++;
            }
        }
    }
    if (targetCount == 0) {
        return;
    }

    // Colour of the words = colour of "E" of ES in the animation
    const uint8_t pe = pgm_read_byte(&first[0]);
    if (pgm_read_byte(&a.palette[pe * 3]) |
        pgm_read_byte(&a.palette[pe * 3 + 1]) |
        pgm_read_byte(&a.palette[pe * 3 + 2])) {
        wordR = pgm_read_byte(&a.palette[pe * 3]);
        wordG = pgm_read_byte(&a.palette[pe * 3 + 1]);
        wordB = pgm_read_byte(&a.palette[pe * 3 + 2]);
    }

    // Digit pixels (column by column, left to right) -> letters of the hour
    uint8_t digits = 0;
    for (uint8_t col = 0; col < animationData::COLS; col++) {
        for (uint8_t row = 0; row < animationData::ROWS; row++) {
            const uint8_t p =
                pgm_read_byte(&first[row * animationData::COLS + col]);
            const uint8_t r = pgm_read_byte(&a.palette[p * 3]);
            const uint8_t g = pgm_read_byte(&a.palette[p * 3 + 1]);
            const uint8_t b = pgm_read_byte(&a.palette[p * 3 + 2]);
            if ((r | g | b) == 0 || isSet(srcWordCells, row, col)) {
                continue;
            }
            morphPixels[digits] = {row, col, 0, 0, r, g, b};
            digits++;
        }
    }
    if (digits == 0) {
        return;
    }
    for (uint8_t i = 0; i < digits; i++) {
        const uint8_t t = static_cast<uint16_t>(i) * targetCount / digits;
        morphPixels[i].toRow = targets[0][t];
        morphPixels[i].toCol = targets[1][t];
    }
    morphCount = digits;
    morph = true;
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

    if (morph && (morphStep > 0 || frame >= staticFrames)) {
        // own ending: digits run together into the hour word of the layout
        if (morphStep > MORPH_STEPS + 1) {
            Serial.printf("Animation: %s finished\n", name(index));
            stop();
            return false;
        }
        if (morphStep == 0) {
            morphStep = 1;
        }
        drawMorph();
        nextFrameAt = now + (morphStep > MORPH_STEPS ? 1500 : 60);
        morphStep++;
        return true;
    }

    drawFrame();
    nextFrameAt = now + pgm_read_word(&a.delays[frame]);
    advance();
    return true;
}

#endif
