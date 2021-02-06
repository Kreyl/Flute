#if 1 // ============================ Includes =================================
#include "hal.h"
#include "MsgQ.h"
#include "shell.h"
#include "led.h"
#include "kl_sd.h"
#include "kl_fs_utils.h"
#include "acg_lsm6ds3.h"
#include "CS42L52.h"
#include "AuPlayer.h"
#include "usb_msd.h"
#include "buttons.h"
#include "main.h"
#include "Charger.h"
#include "FluteSnd.h"
#include "Sequences.h"
#endif
#if 1 // ======================== Variables & prototypes =======================
// Forever
bool OsIsInitialized = false;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
void OnCmd(Shell_t *PShell);
void ITask();

State_t State;
CS42L52_t Codec;
LedSmooth_t Led{LED_BTN};
PinInput_t PinUsbDetect(USB_DETECT_PIN, pudPullDown);
bool PinUsbIsHigh = false;

#define SONG_CNT    7
FluteSnd_t Songs[SONG_CNT] = {
        {"1.wav", 0},
        {"2.wav", 0},
        {"3.wav", 0},
        {"4.wav", 0},
        {"5.wav", 0},
        {"6.wav", 0},
        {"7.wav", 0},
};

//static enum WakeupSrc_t {wusPowerOn, wusWakeup, wusUsb} WakeupSrc;

//static void EnterSleep();
static TmrKL_t TmrOff {TIME_S2I(18), evtIdPwrOffTimeout, tktOneShot};
static TmrKL_t TmrOneSecond {TIME_MS2I(999), evtIdEverySecond, tktPeriodic}; // Measure battery periodically

static Charger_t Charger;
#endif

int main(void) {
#if 1 // ==== Iwdg, Clk, Os, EvtQ, Uart ====
    // Start Watchdog. Will reset in main thread by periodic 1 sec events.
    Iwdg::InitAndStart(4500);
    Iwdg::DisableInDebug();
    // Setup clock frequency
//    Clk.SetCoreClk(cclk24MHz);
    Clk.SetCoreClk(cclk48MHz);
    // 48MHz clock
    Clk.SetupSai1Qas48MhzSrc();
    Clk.UpdateFreqValues();
    // Init OS
    halInit();
    chSysInit();
    OsIsInitialized = true;

    // ==== Init hardware ====
    EvtQMain.Init();
    Uart.Init();
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();
#endif

    // Check if IWDG frozen in standby
    if(!Flash::IwdgIsFrozenInStandby()) {
        Printf("IWDG not frozen in Standby, frozing\r");
        chThdSleepMilliseconds(45);
        Flash::IwdgFrozeInStandby();
    }

    Led.Init();
    PinUsbDetect.Init();
    ButtonsInit();

//    Charger.Init();
//    bool BtnReleased = PinIsHi(BTN1_PIN);

    TmrOneSecond.StartOrRestart();

    // Sound
    Codec.Init();
    Codec.SetSpeakerVolume(-96);    // To remove speaker pop at power on
    Codec.DisableHeadphones();
    Codec.EnableSpeakerMono();
    Codec.SetupMonoStereo(Stereo);  // For wav player
    Codec.SetupSampleRate(22050); // Just default, will be replaced when changed
    Codec.SetMasterVolume(0);
    Codec.SetSpeakerVolume(0); // max
    AuPlayer.Init();

    // Init if SD ok
    SD.Init();
    if(SD.IsReady) {
        Led.StartOrRestart(lsqOk);
        UsbMsd.Init();

//        if(WakeupSrc == wusWakeup) {
//            Printf("Wakeup\r");
//        }
//        else {
//            // Otherwise, wakeup by USB connection occured
//            Printf("USB poweron\r");
//            EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbConnect));
//        }

//        AuPlayer.Play("alive.wav", spmSingle);
    } // if SD is ready
    else {
        Led.StartOrRestart(lsqFail);
        chThdSleepMilliseconds(3600);
//        EnterSleep();
    }

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdShellCmd:
                OnCmd((Shell_t*)Msg.Ptr);
                ((Shell_t*)Msg.Ptr)->SignalCmdProcessed();
                break;

            case evtIdPwrOffTimeout:
                Printf("TmrOff timeout\r");
                break;

            case evtIdButtons:
                Printf("Btn %u %u\r", Msg.BtnInfo.ID, Msg.BtnInfo.Evt);
                if(Msg.BtnInfo.Evt == beShortPress) Songs[Msg.BtnInfo.ID - 1].Play();
                else if(Msg.BtnInfo.Evt == beRelease) {
                    if(ButtonsAreAllIdle()) AuPlayer.FadeOut();
                }
                break;

            // ==== Sound ====
            case evtIdAudioPlayStop:
                Printf("Snd Done\r");
                break;

            case evtIdEverySecond:
//                Printf("Second\r");
                Iwdg::Reload();
                // Check SD
                if(!State.UsbConnected and !SD.IsReady) {
                    if(SD.Reconnect() == retvOk) Led.StartOrRestart(lsqOk);
                    else Led.StartOrContinue(lsqFail);
                }
                // Check USB
                if(PinUsbDetect.IsHi() and !PinUsbIsHigh) {
                    PinUsbIsHigh = true;
                    EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbConnect));
                }
                else if(!PinUsbDetect.IsHi() and PinUsbIsHigh) {
                    PinUsbIsHigh = false;
                    EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbDisconnect));
                }

//                Charger.OnSecond();
                // Check if need to sleep
//                if(State.Name == staSettings or State.Name == staWorking) {
//                    uint32_t Battery_mV = Codec.GetBatteryVmv();
////                    Printf("%u\r", Battery_mV);
//                    if(Battery_mV < BATTERY_DEAD_mv) {
//                        #if !CFG_RELEASE // power off in ProductionConfig or DebugConfig
//                        Printf("Discharged\r");
//                        EnterSleep();
//                        #endif
//                    }
//                    else if(Battery_mV < BATTERY_LOW_mv) {
//                        LedBtn.StartOrContinue(lsqSmoothDischarged);
//                        LedBtn2.StartOrContinue(lsqSmoothDischarged);
//                    }
//                }
                break;

#if 1 // ======= USB =======
            case evtIdUsbConnect:
                Printf("USB connect\r");
                State.UsbConnected = true;
                UsbMsd.Connect();
//                StateMachine(eventUsbConnect);
                break;
            case evtIdUsbDisconnect:
                Printf("USB disconnect\r");
                State.UsbConnected = false;
                State.UsbActive = false;
                UsbMsd.Disconnect();
//                StateMachine(eventDisconnect);
                break;
            case evtIdUsbReady:
                Printf("USB ready\r");
                State.UsbActive = true;
                break;
#endif
            default: break;
        } // switch
    } // while true
}

#if 0 // ===================== Small utils =====================================
void ProcessUsbDetect(PinInputState_t *PState, uint32_t Len) {
    if(*PState == pssRising) EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbConnect));
    else if(*PState == pssFalling) EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbDisconnect));
    else if(*PState == pssLo and State.Name == staUsbConnected) EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbDisconnect));
}

void ProcessCharging(PinInputState_t *PState, uint32_t Len) { Charger.OnChargeStatePin(*PState); }

void EnterSleep() {
    Printf("Entering sleep\r");
    chThdSleepMilliseconds(45);
    chSysLock();
    // Enable inner pull-ups
    PWR->PUCRC |= PWR_PUCRC_PC13;
    PWR->CR3 |= PWR_CR3_APC;
    // // Enable wake-up srcs
    Sleep::EnableWakeup1Pin(rfFalling); // Btn1
    Sleep::EnableWakeup2Pin(rfFalling); // Btn2
    Sleep::EnableWakeup4Pin(rfRising);  // USB
    Sleep::ClearWUFFlags();
    Sleep::EnterStandby();
    chSysUnlock();
}
#endif

#if 0 // ========================== State handlers =============================
void EnterOff() {
    LedBtn.Stop();
    // Wait btn release
    while(GetBtnState(0) != BTN_IDLE_STATE) {
        chThdSleepMilliseconds(18);
    }
    EnterSleep();
}

//void EnterWorking() {
//    TmrOff.Stop();   // Saber is active, stop timer
//    Sound.SetupVolume(Settings.GetVolume()); // to handle changes in settings
//    // Leds
//    LedBtn.StartOrRestart(lsqSmoothFadein);
//    LedBtn2.StartOrRestart(lsqSmoothFadein);
//    BladePowerOn();
//    AuxLeds.DoPowerOn();
//    // Sound
//    Sound.StartHumAndPlayPowerOn();
//    // Motion
//    SaberMotn.Reset();
//    SaberMotn.EnableAll();
//}

//
//void EnterPoweringOff(bool PlayPwrOffSnd) {
//    if(PlayPwrOffSnd) Sound.StopAllAndPlayPowerOff();
//    else Sound.StopAll();
//    // Leds
//    LedBtn.StartOrRestart(lsqSmoothFadeout);
//    LedBtn2.StartOrRestart(lsqSmoothFadeout);
//    BladePowerOff();
//    AuxLeds.DoPowerOff();
//}

void EnterUsbConnected() {
    SaberMotn.DisableAll();
    Sound.StopAll();
    Sound.PlayChargingAndWaitEnd();
    TmrOff.Stop();       // Saber is active, stop timer
    Charger.Enable();
    chThdSleepMilliseconds(108);
    UsbMsd.Connect();
}
#endif

#if 1 // ======================= Command processing ============================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    // Handle command
    if(PCmd->NameIs("Ping")) PShell->Ack(retvOk);
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    else if(PCmd->NameIs("mem")) PrintMemoryInfo();

    else if(PCmd->NameIs("play")) {
        int32_t v1, v2;
        if(PCmd->GetNext<int32_t>(&v1) != retvOk) return;
        if(PCmd->GetNext<int32_t>(&v2) != retvOk) return;
        Codec.SetSpeakerVolume(v1); // -96...0
        Codec.SetMasterVolume(v2); // -102...12
        AuPlayer.Play("1/1.wav", spmSingle);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
