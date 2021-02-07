/*
 * Charger.h
 *
 *  Created on: 10 янв. 2021 г.
 *      Author: layst
 */

#pragma once

#include "Sequences.h"

extern LedSmooth_t Led;

class Charger_t {
private:
    enum {chrgIdle, chrgCharging, chrgRestarting} IState = chrgIdle;
    int32_t SecCounter = 0;
    bool WasHi = true;
public:
    void Init()    {
        PinSetupOut(CHRG_EN_PIN, omPushPull);
        PinSetupInput(IS_CHARGING_PIN, pudPullUp);
    }
    void Enable()  { PinSetHi(CHRG_EN_PIN); }
    void Disable() { PinSetLo(CHRG_EN_PIN); }

    void OnSecond(bool IsUsbConnected) {
        if(!IsUsbConnected) {
            WasHi = true;
            IState = chrgIdle;
            return;
        }

        // Get state pin
        bool IsHiNow = PinIsHi(IS_CHARGING_PIN);
        PinInputState_t PinState = IsHiNow? pssHi : pssLo;
        if(IsHiNow and !WasHi) {
            WasHi = true;
            PinState = pssRising;
        }
        else if(!IsHiNow and WasHi) {
            WasHi = false;
            PinState = pssFalling;
        }
//        Printf("IsCh: %u\r", PinState);
        // Handle state pin. Lo is charging, hi is not charging
        if(PinState == pssFalling or PinState == pssLo) {
            if(IState == chrgIdle) { // Charging just have started
                Led.StartOrContinue(lsqCharging);
                IState = chrgCharging;
                SecCounter = 0; // Reset time counter
            }
        }
        else if(PinState == pssRising) { // Charge stopped
            if(IState == chrgCharging) {
                Led.StartOrContinue(lsqNotCharging);
                IState = chrgIdle;
            }
        }

        // Check charging timeout
        switch(IState) {
            case chrgIdle: break;
            case chrgCharging:
                SecCounter++;
                // Check if charging too long
                if(SecCounter >= (3600L * 7L)) { // Too much time passed, restart charging
                    Disable();
                    SecCounter = 0;
                    IState = chrgRestarting;
                }
                break;
            case chrgRestarting:
                SecCounter++;
                if(SecCounter >= 4) { // Restart after 4 seconds
                    Enable();
                    SecCounter = 0;
                    IState = chrgIdle;
                }
                break;
        } // switch
    }
};
