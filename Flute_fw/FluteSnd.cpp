/*
 * FluteSnd.cpp
 *
 *  Created on: 6 февр. 2021 г.
 *      Author: layst
 */

#include "FluteSnd.h"
#include "AuPlayer.h"

void FluteSnd_t::Play() {
    AuPlayer.Play(FName, spmSingle);
}

