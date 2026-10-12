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
    void drawFrame();
    void advance();

    bool running = false;
    uint8_t index = 0;
    uint8_t frame = 0;
    int8_t direction = 1;
    uint8_t loopsDone = 0;
    uint8_t randomSteps = 0;
    uint32_t nextFrameAt = 0;
};

#endif
