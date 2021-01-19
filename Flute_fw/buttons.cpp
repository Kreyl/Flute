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
    bool IsMidOrLongPressEvtSent = false;
    systime_t PressStartTime = 0;
    uint32_t Indx;
    void SendShortPressEvt() {
        EvtMsg_t Msg(evtIdButtons);
//        Msg.BtnEvtInfo = (Indx == 1)? eventShortPress : eventShortPress2;
        EvtQMain.SendNowOrExit(Msg);
    }
    void SendMidPressEvt() { // Second btn will never be here, he-he
        EvtMsg_t Msg(evtIdButtons);
//        Msg.BtnEvtInfo = eventMidPress;
        EvtQMain.SendNowOrExit(Msg);
    }
    void SendLongPressEvt() {
        EvtMsg_t Msg(evtIdButtons);
//        Msg.BtnEvtInfo = (Indx == 1)? eventLongPress : eventLongPress2;
        EvtQMain.SendNowOrExit(Msg);
    }
public:
    Btn_t(uint32_t Indx) : Indx(Indx) {}

    PinInputState_t State = pssNone;

    void Update(PinInputState_t AState) {
        State = AState;
        switch(AState) {
            case BTN_PRESSING_STATE:
                IsMidOrLongPressEvtSent = false;
                PressStartTime = chVTGetSystemTimeX(); // Start timer
                break;

            case BTN_RELEASING_STATE:
                if(!IsMidOrLongPressEvtSent) { // Do not send evt if LongPress was already sent
                    SendShortPressEvt();
                }
                break;

            case BTN_HOLDDOWN_STATE:
                // Check if longpress occured, if not yet
                if(!IsMidOrLongPressEvtSent and TIME_I2MS(chVTTimeElapsedSinceX(PressStartTime)) >= BTN_LONGPRESS_DELAY_MS) {
                    IsMidOrLongPressEvtSent = true;
                    SendLongPressEvt();
                }
                break;
            default: break;
        } // switch
    }

    void DisableLongpressAndRelease() {
        IsMidOrLongPressEvtSent = true; // Do not send SHort-, Mid- and LongPress Evt
    }
};

static Btn_t Btns[BUTTONS_CNT] = { 1, 2 };

PinInputState_t GetBtnState(uint8_t BtnID) {
    if(BtnID >= BUTTONS_CNT) return pssNone;
    else return Btns[BtnID].State;
}

static systime_t ComboStartTime;
static bool IsComboEvtSent = false;
static bool WasCombo = false;

void ProcessButtons(PinInputState_t State1, PinInputState_t State2) {
//    Printf("Btns: %u %u\r", State1, State2);
    // Check combo
    if((State1 == BTN_HOLDDOWN_STATE or State1 == BTN_PRESSING_STATE) and (State2 == BTN_HOLDDOWN_STATE or State2 == BTN_PRESSING_STATE)) {
        WasCombo = true;
        // If just occured
        if(State1 == BTN_PRESSING_STATE or State2 == BTN_PRESSING_STATE) {
            IsComboEvtSent = false;
            ComboStartTime = chVTGetSystemTimeX();
        }
        else if(State1 == BTN_HOLDDOWN_STATE or State2 == BTN_HOLDDOWN_STATE) {
            // Check if longpress occured, if not yet
            if(!IsComboEvtSent and TIME_I2MS(chVTTimeElapsedSinceX(ComboStartTime)) >= BTN_LONGPRESS_DELAY_MS) {
                IsComboEvtSent = true;
                EvtMsg_t Msg(evtIdButtons);
//                Msg.BtnEvtInfo = eventLongBoth;
                EvtQMain.SendNowOrExit(Msg);
            }
        }
    }
    // Not combo, but if it was before, wait until both buttons are released
    else if(WasCombo) {
        if((State1 == BTN_IDLE_STATE or State1 == pssNone) and (State2 == BTN_IDLE_STATE or State2 == pssNone)) WasCombo = false;
    }
    // No combo
    else {
        Btns[0].Update(State1);
        Btns[1].Update(State2);
    }
}

void DisableLongpressAndRelease(uint8_t BtnID) {
    Btns[BtnID].DisableLongpressAndRelease();
}
