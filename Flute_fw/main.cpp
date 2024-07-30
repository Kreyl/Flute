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
#include "Charger.h"
#include "Sequences.h"
#include "kl_i2c.h"
#include "radio_lvl1.h"
#endif
#if 1 // ======================== Variables & prototypes =======================
// Forever
bool OsIsInitialized = false;
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(115200, CMD_UART_PARAMS);
CmdUart_t Uart{&CmdUartParams};
void OnCmd(Shell_t *PShell);
void ITask();

CS42L52_t Codec;
LedSmooth_t Led{LED_BTN};
PinInput_t PinUsbDetect(USB_DETECT_PIN, pudPullDown);
// Flags
bool IsPlayingIntro = true, PinUsbIsHigh = false, UsbConnected = false, MustSleep = false, IsStandby = true;

#define BATTERY_LOW_mv  3200
#define BATTERY_DEAD_mv 3300

static void Standby();
static void Resume();

static void EnterSleepNow();
static void EnterSleep();
static TmrKL_t TmrOff {TIME_S2I(18), evtIdPwrOffTimeout, tktOneShot};
static TmrKL_t TmrOneSecond {TIME_MS2I(999), evtIdEverySecond, tktPeriodic}; // Measure battery periodically

static Charger_t Charger;

#define FNAME_CNT   20
const char *Filenames[FNAME_CNT] = {
        "1.wav", "2.wav", "3.wav", "4.wav", "5.wav",
        "6.wav", "7.wav", "8.wav", "9.wav", "10.wav",
        "11.wav", "12.wav", "13.wav", "14.wav", "15.wav",
        "16.wav", "17.wav", "18.wav", "19.wav", "20.wav"
};
#define BTN1                        (1UL<<0)
#define BTN2                        (1UL<<1)
#define BTN3                        (1UL<<2)
#define BTN4                        (1UL<<3)
#define BTN5                        (1UL<<4)
#define BTN6                        (1UL<<5)
#define BTN7                        (1UL<<6)
TmrKL_t TmrDoFade{TIME_MS2I(999), evtIdDoFade, tktOneShot};
int32_t PrevIndx = -1;
#endif

int main(void) {
#if 1 // ==== Get source of wakeup ====
    rccEnablePWRInterface(FALSE);
    if(Sleep::WasInStandby()) {
        // Is it button?
        PinSetupInput(GPIOA, 0, pudPullDown);
        if(PinIsHi(GPIOA, 0)) {
            // Check if pressed long enough
            for(uint32_t i=0; i<270000; i++) {
                // Go sleep if btn released too fast
                if(PinIsLo(GPIOA, 0)) EnterSleepNow();
            }
            // Btn was not released long enough, proceed with powerOn
        }
        else { // Check if USB is connected
            PinSetupInput(GPIOA, 2, pudPullDown);
            if(PinIsLo(GPIOA, 2)) EnterSleepNow(); // Something strange happened
        }
    }
#endif
    // Start Watchdog. Will reset in main thread by periodic 1 sec events.
    Iwdg::InitAndStart(4500);
    Iwdg::DisableInDebug();

#if 1 // ==== Clk, Os, EvtQ, Uart ====
    // Setup clock frequency
    Clk.SetCoreClk(cclk24MHz);
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
    Buttons::Init();
    Charger.Init();

    TmrOneSecond.StartOrRestart();

    AU_i2c.Init();

    Codec.Init();
    Codec.SetSpeakerVolume(-96);    // To remove speaker pop at power on
    Codec.DisableHeadphones();
    Codec.EnableSpeakerMono();
    Codec.SetupMonoStereo(Stereo);  // For wav player
    Codec.SetupSampleRate(22050); // Just default, will be replaced when changed
    Codec.SetMasterVolume(12); // 12 is max
    Codec.SetSpeakerVolume(0); // 0 is max
    AuPlayer.Init();

    SD.Init();

    // Init if SD ok
    if(SD.IsReady) {
        Led.StartOrRestart(lsqOk);
        UsbMsd.Init();
        AuPlayer.Play("WakeUp.wav", spmSingle);
        IsPlayingIntro = true;
        chThdSleepMilliseconds(99); // Allow it to start
    } // if SD is ready
    else {
        Led.StartOrRestart(lsqFail);
        chThdSleepMilliseconds(3600);
        EnterSleep();
    }

    Radio.Init(0);

    // Main cycle
    ITask();
}

void OnNewBtnMsk(uint32_t Msk) {
    Printf("Msk 0x%02X\r", Msk);
    if(MustSleep) return;
    Resume();
    // Check if all buttons released
    if(Msk == 0) {
        if(!IsPlayingIntro) { // Ignore btn release after pwron
            TmrDoFade.StartOrRestart(); // Fade after some time
        }
        IsPlayingIntro = false;
    }
    // Something pressed
    else {
        if(IsPlayingIntro) return;
        TmrDoFade.Stop(); // Do not fade, as btn is pressed
        int32_t Indx = -1;
        // No combo for Btn0
        if     (Msk == BTN1) Indx = 0;
        // Btn1: Is there a Combo?
        else if(Msk == (BTN2|BTN3)) Indx = 7; // Combo 1 + 2
        else if(Msk == (BTN2|BTN4)) Indx = 8; // Combo 1 + 3
        else if(Msk == (BTN2|BTN5)) Indx = 9; // Combo 1 + 4
        else if(Msk == (BTN2|BTN6)) Indx = 10; // Combo 1 + 5
        else if(Msk == (BTN2|BTN7)) Indx = 11; // Combo 1 + 6
        else if(Msk == BTN2) Indx = 1; // No Combo
        // Btn2: is there a Combo?
        else if(Msk == (BTN3|BTN4)) Indx = 12; // Combo 2 + 3
        else if(Msk == (BTN3|BTN5)) Indx = 13; // Combo 2 + 4
        else if(Msk == (BTN3|BTN6)) Indx = 14; // Combo 2 + 5
        else if(Msk == (BTN3|BTN7)) Indx = 15; // Combo 2 + 6
        else if(Msk == BTN3) Indx = 2; // No Combo
        // Btn3: is there a Combo?
        else if(Msk == (BTN4|BTN5)) Indx = 15; // Combo 3 + 4
        else if(Msk == (BTN4|BTN6)) Indx = 16; // Combo 3 + 5
        else if(Msk == (BTN4|BTN7)) Indx = 17; // Combo 3 + 6
        else if(Msk == BTN4) Indx = 3; // No Combo
        // Btn4: is there a Combo?
        else if(Msk == (BTN5|BTN6)) Indx = 18; // Combo 4 + 5
        else if(Msk == (BTN5|BTN7)) Indx = 19; // Combo 4 + 6
        else if(Msk == BTN5) Indx = 4; // No Combo
        // Check if need to sleep: 5 & 6 pressed
        else if(Msk == (BTN6 | BTN7)) {
            MustSleep = true;
            AuPlayer.Play("Sleep.wav", spmSingle);
            return;
        }
        // Btn5 & Btn6: no Combo
        else if(Msk == BTN6) Indx = 5;
        else if(Msk == BTN7) Indx = 6;
        if(Indx < 0 or Indx >= FNAME_CNT) return; // Something strange there, do nothing
        // Correct btn is pressed, process it depending on state
        if(!AuPlayer.IsPlayingNow() or (Indx != PrevIndx)) { // not playing or other btn is pressed
            AuPlayer.Play(Filenames[Indx], spmSingle);
        }
            PrevIndx = Indx;
    }
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

            case evtIdButtons: OnNewBtnMsk(Msg.BtnPressedMask); break;

            case evtIdAudioPlayStop:
                Printf("Snd Done\r");
                Standby();
                IsPlayingIntro = false;
                if(MustSleep) EnterSleep();
                break;

            case evtIdDoFade: AuPlayer.FadeOut(); break;

            case evtIdEverySecond:
//                Printf("Second\r");
                Iwdg::Reload();
                // Check SD
                if(!UsbConnected and !SD.IsReady) {
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
                    Led.StartOrContinue(lsqOk);
                }
                // Charger
                Charger.OnSecond(PinUsbIsHigh);

                // Check battery
                if(!IsStandby) {
                    uint32_t Battery_mV = Codec.GetBatteryVmv();
//                    Printf("%u\r", Battery_mV);
                    if(Battery_mV < BATTERY_DEAD_mv) {
                        Printf("Discharged: %u\r", Battery_mV);
                        EnterSleep();
                    }
                }
                break;

#if 1 // ======= USB =======
            case evtIdUsbConnect:
                Printf("USB connect\r");
                Resume();
                UsbConnected = true;
                UsbMsd.Connect();
                Charger.Enable();
                break;
            case evtIdUsbDisconnect:
                Standby();
                Printf("USB disconnect\r");
                UsbConnected = false;
                UsbMsd.Disconnect();
                break;
            case evtIdUsbReady:
                Printf("USB ready\r");
                break;
#endif
            default: break;
        } // switch
    } // while true
}

void Resume() {
    if(!IsStandby) return;
    Printf("Resume\r");
    Codec.Resume();

    IsStandby = false;
}

void Standby() {
    Printf("Standby\r");
    Codec.Standby();
    IsStandby = true;
}

void EnterSleepNow() {
    // Enable inner pull-ups
//    PWR->PUCRC |= PWR_PUCRC_PC13;
    // Enable inner pull-downs
    PWR->PDCRA |= PWR_PDCRA_PA0 | PWR_PDCRA_PA2;
//    PWR->PDCRC |= PWR_PDCRC_PC13;
    // Apply PullUps and PullDowns
    PWR->CR3 |= PWR_CR3_APC;
    // Enable wake-up srcs
    Sleep::EnableWakeup1Pin(rfRising); // Btn1
//    Sleep::EnableWakeup2Pin(rfRising); // Btn2
    Sleep::EnableWakeup4Pin(rfRising); // USB
    Sleep::ClearWUFFlags();
    Sleep::EnterStandby();
}

void EnterSleep() {
    Printf("Entering sleep\r");
    chThdSleepMilliseconds(45);
    chSysLock();
    EnterSleepNow();
    chSysUnlock();
}

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
        AuPlayer.Play("1.wav", spmSingle);
    }

    else PShell->Ack(retvCmdUnknown);
}
#endif
