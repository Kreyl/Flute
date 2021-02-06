/*
 * FluteSnd.cpp
 *
 *  Created on: 6 февр. 2021 г.
 *      Author: layst
 */

#include "FluteSnd.h"
#include "AuPlayer.h"

void FluteSnd_t::Play() {
    uint32_t Elapsed = TIME_I2S(chVTTimeElapsedSinceX(StartTime));
//    Printf("%S elapsed: %u\r", FName, Elapsed);
    if(Elapsed > DelayBeforeRepeat_s) {
        StartTime = chVTGetSystemTimeX();
        Printf("%S\r", FName);
        AuPlayer.Play(FName, spmSingle);
    }
    else {
        Printf("%S: not enough time elapsed, %u\r", FName, Elapsed);
        AuPlayer.Play(SONG_TIMEOUT, spmSingle);
    }
}

