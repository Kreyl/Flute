/*
 * FluteSnd.h
 *
 *  Created on: 6 февр. 2021 г.
 *      Author: layst
 */

#pragma once

#include <inttypes.h>
#include "ch.h"

#define SONG_TIMEOUT    "timeout.wav"

class FluteSnd_t {
private:
    const char *FName;
    systime_t StartTime = 0;
public:
    uint32_t DelayBeforeRepeat_s;
    FluteSnd_t(const char *FName, uint32_t DelayBeforeRepeat_s) :
        FName(FName), DelayBeforeRepeat_s(DelayBeforeRepeat_s) {}
    void Play();
};
