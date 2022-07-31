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

static uint8_t BtnMskPrev = 0;
static EvtMsg_t Msg{evtIdButtons};

class Btn_t {
private:
    GPIO_TypeDef *PGpio;
    uint16_t Pin;
    PinPullUpDown_t PullUpDown;
public:
    Btn_t(GPIO_TypeDef *PGpio, uint16_t Pin, PinPullUpDown_t PullUpDown) :
        PGpio(PGpio), Pin(Pin), PullUpDown(PullUpDown) { }

    PinInputState_t State = pssNone;

    void Init() const { PinSetupInput(PGpio, Pin, PullUpDown); }
    bool IsHi() const { return PinIsHi(PGpio, Pin); }
};

static Btn_t Btns[BUTTONS_CNT] = {
        {BTN1_PIN},
        {BTN2_PIN},
        {BTN3_PIN},
        {BTN4_PIN},
        {BTN5_PIN},
        {BTN6_PIN},
        {BTN7_PIN},
};

static THD_WORKING_AREA(waBtnsThread, 256);
__noreturn
static void BtnsThread(void *arg) {
    chRegSetThreadName("Btns");
    while(true) {
        chThdSleepMilliseconds(BTN_POLL_PERIOD_MS);
        uint8_t BtnMskCurr = 0;
        for(uint32_t i=0; i<BUTTONS_CNT; i++) {
            if(Btns[i].IsHi()) BtnMskCurr |= (1 << i);
        }
        if(BtnMskCurr != BtnMskPrev) {
            Msg.BtnPressedMask = BtnMskCurr;
            EvtQMain.SendNowOrExit(Msg);
            BtnMskPrev = BtnMskCurr;
        }
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

} // Namspace
