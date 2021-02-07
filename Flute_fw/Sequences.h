/*
 * Sequences.h
 *
 *  Created on: 09 ���. 2015 �.
 *      Author: Kreyl
 */

#pragma once

#include "ChunkTypes.h"

#define LED_TOP_BRIGHTNESS  255

const LedSmoothChunk_t lsqOk[] = {
        {csSetup, 207, LED_TOP_BRIGHTNESS},
        {csSetup, 207, 4},
        {csEnd}
};

const LedSmoothChunk_t lsqFail[] = {
        {csSetup, 0, LED_TOP_BRIGHTNESS},
        {csWait, 45},
        {csSetup, 0, 0},
        {csWait, 45},
        {csGoto, 0}
};

const LedSmoothChunk_t lsqCharging[] = {
        {csSetup, 360, LED_TOP_BRIGHTNESS},
        {csSetup, 360, 4},
        {csGoto, 0}
};

const LedSmoothChunk_t lsqNotCharging[] = {
        {csSetup, 360, LED_TOP_BRIGHTNESS},
        {csEnd}
};

const LedSmoothChunk_t lsqOff[] = {
        {csSetup, 207, LED_TOP_BRIGHTNESS},
        {csSetup, 0, 0},
        {csEnd}
};
