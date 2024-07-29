/*
 * acg_lsm6ds3.h
 *
 *  Created on: 2 рту. 2017 у.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"
#include "shell.h"

struct AccSpd_t {
    uint8_t __dummy;    // DMA will write here when transmitting addr
    int16_t g[3];
    int16_t a[3];
    void Print() { Printf("%d %d %d; %d %d %d\r", a[0],a[1],a[2], g[0],g[1],g[2]); }
} __packed;

void AcgInit();

extern AccSpd_t AccSpd;
