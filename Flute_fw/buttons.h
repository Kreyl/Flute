/*
 * buttons.h
 *
 *  Created on: 07.02.2013
 *      Author: kreyl
 */

#pragma once

#include "hal.h"
#include "kl_lib.h"
#include "MsgQ.h"

#define BTN_RELEASE_MIN_DELAY_MS    720

#define BTN_IDLE_STATE              pssLo
#define BTN_HOLDDOWN_STATE          pssHi
#define BTN_PRESSING_STATE          pssRising
#define BTN_RELEASING_STATE         pssFalling

#define BTN_POLL_PERIOD_MS          72

#define BUTTONS_CNT     7

namespace Buttons {

void Init();
bool AreAllIdle();

}
