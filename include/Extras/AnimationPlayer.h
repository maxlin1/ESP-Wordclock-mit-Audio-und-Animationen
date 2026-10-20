#pragma once

#include "../ExtrasConfig.h"
#include <Arduino.h>

#if EXTRAS_ENABLED

/*
 * Plays the animations of Manfred Hofmann's Wortuhr_mp3 project on the
 * 10 x 11 letter matrix. The animations are compiled into the firmware (see
 * tools/convert_animations.py).
 */
class AnimationPlayer {
public:
    static uint8_t count();
    static const char *name(uint8_t index);
    // Index of the animation with this name (case-insensitive) or -1.
    static int16_t find(const char *name);

    bool start(uint8_t index);
    void stop();
    bool isRunning() const { return running; }
    const char *currentName() const;

    // Draws the next frame when it is due. Returns false once the animation
    // has finished (or when none is running).
    bool loop();

private:
    // Manfred's time animations ("Z0800" ... "Z2200") show the time as
    // digits and then let the digits run together into "ES IST ... UHR" of
    // *his* front. The player keeps the digit part and calculates the end
    // itself for the words of the selected layout.
    struct MorphPixel {
        uint8_t row, col;     // start (digit pixel)
        uint8_t toRow, toCol; // target (letter of the hour word)
        uint8_t r, g, b;
    };
    static constexpr uint8_t MORPH_STEPS = 12;
    static constexpr uint8_t MAX_MORPH_PIXELS = 110;

    void prepareTimeMorph();
    void drawFrame();
    void drawMorph();
    void drawWords(bool withHour);
    void setPixelRgb(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b,
                     float brightness);
    void advance();

    bool morph = false;       // this is a time animation with own ending
    uint8_t staticFrames = 0; // frames before the digits start to move
    uint8_t morphStep = 0;    // 0 = not yet, 1..MORPH_STEPS+1
    uint8_t morphCount = 0;
    MorphPixel morphPixels[MAX_MORPH_PIXELS];
    uint16_t wordCells[10] = {0};    // ES IST + UHR of the layout (bit = col)
    uint16_t hourCells[10] = {0};    // hour word of the layout
    uint16_t srcWordCells[10] = {0}; // ES IST + UHR of Manfred's front
    uint8_t wordR = 255, wordG = 255, wordB = 255;

    bool running = false;
    uint8_t index = 0;
    uint8_t frame = 0;
    int8_t direction = 1;
    uint8_t loopsDone = 0;
    uint8_t randomSteps = 0;
    uint32_t nextFrameAt = 0;
};

#endif
