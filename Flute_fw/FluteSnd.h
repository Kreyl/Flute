/*
 * FluteSnd.h
 *
 *  Created on: 6 февр. 2021 г.
 *      Author: layst
 */

#pragma once

#include <inttypes.h>

class FluteSnd_t {
private:
    const char *FName;
public:
    uint32_t DelayBeforeRepeat_ms;
    FluteSnd_t(const char *FName, uint32_t DelayBeforeRepeat_ms) :
        FName(FName), DelayBeforeRepeat_ms(DelayBeforeRepeat_ms) {}
    void Play();
};
