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

#define FNAME_CNT   15
const char *Filenames[FNAME_CNT] = {
        "1.wav", "2.wav", "3.wav", "4.wav", "5.wav",
        "6.wav", "7.wav", "8.wav", "9.wav", "10.wav",
        "11.wav", "12.wav", "13.wav", "14.wav", "15.wav"
};
const Color_t Colors[FNAME_CNT] = {
        clGreen, clBlack, clBlue, clCyan, clMagenta,
        clRed, clYellow, clGreen, clCyan, clBlue,
        clGreen, clBlack, clBlue, clCyan, clMagenta,
};
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

    uint32_t RPwrId = 5;

    // Init if SD ok
    if(SD.IsReady) {
        if(ini::Read<uint32_t>("Settings.ini", "Radio", "Power", &RPwrId) != retvOk) RPwrId = 5;
        if(RPwrId > 11) RPwrId = 11;

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

    Radio.Init(RPwrId);

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
                Printf("Btn 0x%02X\r", Msg.BtnPressedMask);
                Resume();
                // Check if all buttons released
                if(Msg.BtnPressedMask == 0) {
                    if(IsPlayingIntro) IsPlayingIntro = false; // Ignore btn release after pwron
                    else AuPlayer.FadeOut();
                    Radio.MustTx = false;
                }
                // Something pressed
                else if(!(IsPlayingIntro and AuPlayer.IsPlayingNow())) { // Ignore btn if playing intro after pwron
                    IsPlayingIntro = false;
                    uint8_t Msk = Msg.BtnPressedMask; // To make things shorter
                    // Check if need to sleep: 6 & 7 pressed
                    if(Msk == ((1<<5) | (1<<6))) {
                        MustSleep = true;
                        AuPlayer.Play("Sleep.wav", spmSingle);
                        Radio.MustTx = false;
                    }
                    // Check if need to send "Stop" to speaking stone
                    else if(Msk == ((1<<1) | (1<<0))) {
                        Radio.ClrToTx = clBlack;
                        Radio.BtnIndx = 0xFF; // Means Stop
                        Radio.MustTx = true;
                    }
                    // Something else pressed, play what needed
                    else {
                        int8_t Indx = -1;
                        // Get lesser button
                        if     (Msk & (1<<0)) Indx = 0;
                        else if(Msk & (1<<1)) Indx = 1;
                        else if(Msk & (1<<2)) Indx = 2;
                        else if(Msk & (1<<3)) Indx = 3;
                        else if(Msk & (1<<4)) Indx = 4;
                        if(Indx >= 0) { // Do nothing if no lesser button is pressed
                            // Get combo
                            if(Msk & (1<<5)) Indx += 5; // Combo with 6
                            else if(Msk & (1<<6)) Indx += 10; // Combo with 7
                           // Play
                            AuPlayer.Play(Filenames[Indx], spmSingle);
                            // Radio
                            Radio.ClrToTx = Colors[Indx];
                            Radio.BtnIndx = Indx;
                            Radio.MustTx = true;
                        }
                    }
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
