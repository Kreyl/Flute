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
#include "FluteSnd.h"
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

Color_t Clrs[7] = {
        clGreen,
        clBlack,
        clBlue,
        clCyan,
        clMagenta,
        clRed,
        clYellow
};

static void Standby();
static void Resume();

static void EnterSleepNow();
static void EnterSleep();
static TmrKL_t TmrOff {TIME_S2I(18), evtIdPwrOffTimeout, tktOneShot};
static TmrKL_t TmrOneSecond {TIME_MS2I(999), evtIdEverySecond, tktPeriodic}; // Measure battery periodically

static Charger_t Charger;
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

#if 1 // ==== Iwdg, Clk, Os, EvtQ, Uart ====
    // Setup clock frequency
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
    Buttons::Init();
    Charger.Init();
    AuPlayer.Init();

    TmrOneSecond.StartOrRestart();

    SD.Init();
    AU_i2c.Init();
    Resume();

    // Init if SD ok
    if(SD.IsReady) {
        Led.StartOrRestart(lsqOk);
        UsbMsd.Init();
        AuPlayer.Play("WakeUp.wav", spmSingle);
    } // if SD is ready
    else {
        Led.StartOrRestart(lsqFail);
        chThdSleepMilliseconds(3600);
        EnterSleep();
    }

    Radio.Init();

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
//                Printf("Btn %u %u\r", Msg.BtnInfo.ID, Msg.BtnInfo.Evt);
                if(Msg.BtnInfo.Evt == bePress) {
                    Resume();
                    IsPlayingIntro = false;
                    Songs[Msg.BtnInfo.ID - 1].Play();
                    Radio.ClrToTx = Clrs[Msg.BtnInfo.ID - 1];
                    Radio.MustTx = true;
                }
                else if(Msg.BtnInfo.Evt == beRelease) {
                    if(IsPlayingIntro) IsPlayingIntro = false;
                    else if(Buttons::AreAllIdle()) AuPlayer.FadeOut();
                    Radio.MustTx = false;
                }
                else if(Msg.BtnInfo.Evt == beCombo) {
                    Resume();
                    IsPlayingIntro = true;
                    MustSleep = true;
                    AuPlayer.Play("Sleep.wav", spmSingle);
                    Radio.MustTx = false;
                }
                break;

            // ==== Sound ====
            case evtIdAudioPlayStop:
//                Printf("Snd Done\r");
                IsPlayingIntro = false;
                if(MustSleep) EnterSleep();
                Standby();
                break;

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
//                        EnterSleep();
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
    // Clock
    Clk.SetCoreClk(cclk48MHz);
    Clk.SetupSai1Qas48MhzSrc();
    Clk.UpdateFreqValues();
    Clk.PrintFreqs();
    // Sound
    Codec.Init();
    Codec.SetSpeakerVolume(-96);    // To remove speaker pop at power on
    Codec.DisableHeadphones();
    Codec.EnableSpeakerMono();
    Codec.SetupMonoStereo(Stereo);  // For wav player
    Codec.SetupSampleRate(22050); // Just default, will be replaced when changed
    Codec.SetMasterVolume(5); // 12 is max
    Codec.SetSpeakerVolume(0); // 0 is max

    IsStandby = false;
}

void Standby() {
    Printf("Standby\r");
    // Sound
    Codec.Deinit();
    // Clock
    Clk.SwitchToMSI();
    Clk.DisablePLL();
    Clk.DisableSai1();

    Clk.UpdateFreqValues();
    Clk.PrintFreqs();
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
