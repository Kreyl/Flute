/*
 * main.h
 *
 *  Created on: 31 ���. 2018 �.
 *      Author: Kreyl
 */

#pragma once

enum StateName_t {
    staOff = 0,
    staUsbConnected = 1,
    staWorking = 2
};

#define BATTERY_LOW_mv  3200
#define BATTERY_DEAD_mv 2800

struct State_t {
    StateName_t Name = staOff;
    bool UsbConnected = false;
    bool UsbActive = false;
};

extern State_t State;
