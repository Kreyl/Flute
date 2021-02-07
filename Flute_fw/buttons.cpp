/*
 * keys.cpp
 *
 *  Created on: 07.02.2013
 *      Author: kreyl
 */

#include "buttons.h"
#include "ch.h"
#include "uart.h"
#include "MsgQ.h"

class Btn_t {
private:
    EvtMsg_t Msg{evtIdButtons};
    GPIO_TypeDef *PGpio;
    uint16_t Pin;
    PinPullUpDown_t PullUpDown;
    void SendShortPressEvt() {
        Msg.BtnInfo.Evt = bePress;
        EvtQMain.SendNowOrExit(Msg);
    }
    void SendReleaseEvt() {
        Msg.BtnInfo.Evt = beRelease;
        EvtQMain.SendNowOrExit(Msg);
    }
    systime_t ReleaseTime = 0;
    bool WasReleased = false;
public:
    Btn_t(uint32_t Indx, GPIO_TypeDef *PGpio, uint16_t Pin, PinPullUpDown_t PullUpDown) :
        PGpio(PGpio), Pin(Pin), PullUpDown(PullUpDown) { Msg.BtnInfo.ID = Indx; }

    PinInputState_t State = pssNone;

    void Init() const { PinSetupInput(PGpio, Pin, PullUpDown); }
    bool IsHi() const { return PinIsHi(PGpio, Pin); }

    void Update() {
        if(IsHi()) {
            if(State == pssLo or State == pssFalling) State = pssRising;
            else State = pssHi;
        }
        else { // is low
            if(State == pssHi or State == pssRising) State = pssFalling;
            else State = pssLo;
        }

        switch(State) {
            case BTN_PRESSING_STATE:
                // How long since release?
                if(chVTTimeElapsedSinceX(ReleaseTime) > TIME_MS2I(BTN_RELEASE_MIN_DELAY_MS)) { // Long ago
                    SendShortPressEvt();
                }
                WasReleased = false;
                break;

            case BTN_RELEASING_STATE:
                ReleaseTime = chVTGetSystemTimeX(); // Start timer
                WasReleased = true;
                break;

            case BTN_IDLE_STATE:
                // Check if release timeout occured
                if(WasReleased) {
                    if(chVTTimeElapsedSinceX(ReleaseTime) > TIME_MS2I(BTN_RELEASE_MIN_DELAY_MS)) { // Long ago
                        SendReleaseEvt();
                        WasReleased = false;
                    }
                }
                break;
            default: break;
        } // switch
    }
};

static Btn_t Btns[BUTTONS_CNT] = {
        {1, BTN1_PIN},
        {2, BTN2_PIN},
        {3, BTN3_PIN},
        {4, BTN4_PIN},
        {5, BTN5_PIN},
        {6, BTN6_PIN},
        {7, BTN7_PIN},
};

static bool WasCombo = false;

static THD_WORKING_AREA(waBtnsThread, 256);
__noreturn
static void BtnsThread(void *arg) {
    chRegSetThreadName("Btns");
    while(true) {
        // Check if combo
        if(Btns[6].IsHi() and Btns[5].IsHi()) {
            if(!WasCombo) {
                WasCombo = true;
                EvtMsg_t Msg{evtIdButtons};
                Msg.BtnInfo.Evt = beCombo;
                EvtQMain.SendNowOrExit(Msg);
            }
        }
        // Not combo
        else {
            WasCombo = false;
            for(auto &Btn : Btns) Btn.Update();
        }
        chThdSleepMilliseconds(BTN_POLL_PERIOD_MS);
    } // while true
}

namespace Buttons {

void Init() {
    // Init pins
    for(auto &Btn : Btns) {
        Btn.Init();
        Btn.State = pssNone;
    }
    Btns[0].State = Btns[0].IsHi()? pssHi : pssNone;
    // Create and start thread
    chThdCreateStatic(waBtnsThread, sizeof(waBtnsThread), NORMALPRIO, (tfunc_t)BtnsThread, NULL);
}

bool AreAllIdle() {
    bool Rslt = true;
    for(auto &Btn : Btns) {
        if(Btn.State != BTN_IDLE_STATE) {
            Rslt = false;
            break;
        }
    }
    return Rslt;
}

} // Namspace
