/*
 * Charger.h
 *
 *  Created on: 10 янв. 2021 г.
 *      Author: layst
 */

#pragma once

extern LedSmooth_t LedBtn, LedBtn2;

class Charger_t {
private:
    enum {chrgIdle, chrgCharging, chrgRestarting} IState = chrgIdle;
    int32_t SecCounter = 0;
public:
    void Init()    { PinSetupOut(CHRG_EN_PIN, omPushPull); }
    void Enable()  { PinSetHi(CHRG_EN_PIN); }
    void Disable() { PinSetLo(CHRG_EN_PIN); }

    void OnSecond() {
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

    // Lo is charging, hi is not charging
    void OnChargeStatePin(PinInputState_t PinState) {
        if(PinState == pssFalling or PinState == pssLo) {
            if(IState == chrgIdle) { // Charging just have started
//                LedBtn.StartOrContinue(lsq5VCharging);
//                LedBtn2.StartOrContinue(lsq5VCharging);
                IState = chrgCharging;
                SecCounter = 0; // Reset time counter
            }
        }
        else if(PinState == pssRising) { // Charge stopped
            if(IState == chrgCharging) {
//                LedBtn.StartOrContinue(lsq5VNotCharging);
//                LedBtn2.StartOrContinue(lsq5VNotCharging);
                IState = chrgIdle;
            }
        }
    }
};
