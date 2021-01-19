/*
 * buttons.h
 *
 *  Created on: 07.02.2013
 *      Author: kreyl
 */

#pragma once

#include "hal.h"
#include "kl_lib.h"

// Shortpress is everything shorter than MidPress.
#define BTN_MIDPRESS_DELAY_MS       450
#define BTN_LONGPRESS_DELAY_MS      1260

#define BTN_IDLE_STATE              pssHi
#define BTN_HOLDDOWN_STATE          pssLo
#define BTN_PRESSING_STATE          pssFalling
#define BTN_RELEASING_STATE         pssRising

#define BUTTONS_CNT     2

PinInputState_t GetBtnState(uint8_t BtnID);
void DisableLongpressAndRelease(uint8_t BtnID);
