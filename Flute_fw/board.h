/*
 * board.h
 *
 *  Created on: 05 08 2018
 *      Author: Kreyl
 */

#ifndef BOARD_H__
#define BOARD_H__

// ==== General ====
#define APP_NAME            "Flute"

// Version of PCB
#define PCB_VER                 1

// MCU type as defined in the ST header.
#define STM32L476xx

// Freq of external crystal if any. Leave it here even if not used.
#define CRYSTAL_FREQ_HZ         12000000

// OS timer settings
#define STM32_ST_IRQ_PRIORITY   2
#define STM32_ST_USE_TIMER      5
#define SYS_TIM_CLK             (Clk.APB1FreqHz)    // Timer 5 is clocked by APB1

//  Periphery
#define I2C1_ENABLED            FALSE
#define I2C2_ENABLED            FALSE
#define I2C3_ENABLED            TRUE

#define ADC_REQUIRED            FALSE
#define STM32_DMA_REQUIRED      TRUE    // Leave this macro name for OS

#if 1 // ========================== GPIO =======================================
// EXTI
#define INDIVIDUAL_EXTI_IRQ_REQUIRED    FALSE

// Buttons
#define BTN1_PIN        GPIOA, 0, pudPullDown
#define BTN2_PIN        GPIOC, 13, pudPullDown
#define BTN3_PIN        GPIOA, 1, pudPullDown
#define BTN4_PIN        GPIOC, 14, pudPullDown
#define BTN5_PIN        GPIOB, 5, pudPullDown
#define BTN6_PIN        GPIOB, 6, pudPullDown
#define BTN7_PIN        GPIOB, 7, pudPullDown

// Charging
#define CHRG_EN_PIN     GPIOC, 15
#define IS_CHARGING_PIN GPIOC, 2

// UART
#define UART_GPIO       GPIOA
#define UART_TX_PIN     9
#define UART_RX_PIN     10

// LED
#define LED_BTN         { GPIOB, 0, TIM3, 3, invNotInverted, omPushPull, 255 }

// CS42L52
#define AU_RESET        GPIOB, 3, omPushPull
#define AU_MCLK         GPIOA, 8, omPushPull, pudNone, AF0  // MCO
#define AU_SDIN         GPIOC, 3, omPushPull, pudNone, AF13 // MOSI; SAI1_A
#define AU_LRCK         GPIOB, 9, omPushPull, pudNone, AF13
#define AU_SCLK         GPIOB, 10, omPushPull, pudNone, AF13
#define AU_i2c          i2c3
#define AU_SAI          SAI1
#define AU_SAI_A        SAI1_Block_A
#define AU_SAI_B        SAI1_Block_B
#define AU_SAI_RccEn()  RCC->APB2ENR |= RCC_APB2ENR_SAI1EN
#define AU_SAI_RccDis() RCC->APB2ENR &= ~RCC_APB2ENR_SAI1EN

// Acc
#define ACG_IRQ_PIN     GPIOB, 2
#define ACG_CS_PIN      GPIOB, 12
#define ACG_SCK_PIN     GPIOB, 13, omPushPull, pudNone, AF5
#define ACG_MISO_PIN    GPIOB, 14, omPushPull, pudNone, AF5
#define ACG_MOSI_PIN    GPIOB, 15, omPushPull, pudNone, AF5
#define ACG_PWR_PIN     GPIOC, 4
#define ACG_SPI         SPI2

// I2C
#define I2C1_GPIO       GPIOB
#define I2C1_SCL        6
#define I2C1_SDA        7
#define I2C2_GPIO       GPIOB
#define I2C2_SCL        10
#define I2C2_SDA        11
#define I2C3_GPIO       GPIOC
#define I2C3_SCL        0
#define I2C3_SDA        1
// I2C Alternate Function
#define I2C_AF          AF4

// USB
#define USB_DM          GPIOA, 11
#define USB_DP          GPIOA, 12
#define USB_AF          AF10
// USB detect
#define USB_DETECT_PIN  GPIOA, 2

// SD
#define SD_PWR_PIN      GPIOC, 7
#define SD_AF           AF12
#define SD_DAT0         GPIOC,  8, omPushPull, pudPullUp, SD_AF
#define SD_DAT1         GPIOC,  9, omPushPull, pudPullUp, SD_AF
#define SD_DAT2         GPIOC, 10, omPushPull, pudPullUp, SD_AF
#define SD_DAT3         GPIOC, 11, omPushPull, pudPullUp, SD_AF
#define SD_CLK          GPIOC, 12, omPushPull, pudNone,   SD_AF
#define SD_CMD          GPIOD,  2, omPushPull, pudPullUp, SD_AF

// Radio: SPI, PGpio, Sck, Miso, Mosi, Cs, Gdo0
#define CC_Setup0       SPI1, GPIOA, 5,6,7, GPIOA,4, GPIOA,3

#endif // GPIO

#if 1 // =========================== I2C =======================================
// i2cclkPCLK1, i2cclkSYSCLK, i2cclkHSI
#define I2C_CLK_SRC     i2cclkHSI
#define I2C_BAUDRATE_HZ 400000
#endif

#if ADC_REQUIRED // ======================= Inner ADC ==========================
// Clock divider: clock is generated from the APB2
#define ADC_CLK_DIVIDER		adcDiv2

// ADC channels
#define ADC_BATTERY_CHNL 	14
// ADC_VREFINT_CHNL
#define ADC_CHANNELS        { ADC_BATTERY_CHNL, ADC_VREFINT_CHNL }
#define ADC_CHANNEL_CNT     2   // Do not use countof(AdcChannels) as preprocessor does not know what is countof => cannot check
#define ADC_SAMPLE_TIME     ast24d5Cycles
#define ADC_OVERSAMPLING_RATIO  64   // 1 (no oversampling), 2, 4, 8, 16, 32, 64, 128, 256
#endif

#if 1 // =========================== DMA =======================================
// ==== Uart ====
// Remap is made automatically if required
#define UART_DMA_TX     STM32_DMA_STREAM_ID(2, 6)
#define UART_DMA_RX     STM32_DMA_STREAM_ID(2, 7)
#define UART_DMA_CHNL   2
#define UART_DMA_TX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_LOW | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_TCIE)
#define UART_DMA_RX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_MEDIUM | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC)

// ==== ACG ====
#define ACG_DMA_TX      STM32_DMA_STREAM_ID(1, 5)
#define ACG_DMA_RX      STM32_DMA_STREAM_ID(1, 4)
#define ACG_DMA_CHNL    1

// ==== I2C ====
#define I2C1_DMA_TX     STM32_DMA_STREAM_ID(1, 6)
#define I2C1_DMA_RX     STM32_DMA_STREAM_ID(1, 7)
#define I2C1_DMA_CHNL   3
#define I2C2_DMA_TX     STM32_DMA_STREAM_ID(1, 4)
#define I2C2_DMA_RX     STM32_DMA_STREAM_ID(1, 5)
#define I2C2_DMA_CHNL   3
#define I2C3_DMA_TX     STM32_DMA_STREAM_ID(1, 2)
#define I2C3_DMA_RX     STM32_DMA_STREAM_ID(1, 3)
#define I2C3_DMA_CHNL   3

// ==== SAI ====
#define SAI_DMA_A       STM32_DMA_STREAM_ID(2, 1)
#define SAI_DMA_CHNL    1

// ==== SDMMC ====
#define STM32_SDC_SDMMC1_DMA_STREAM   STM32_DMA_STREAM_ID(2, 5)

#if ADC_REQUIRED
#define ADC_DMA         STM32_DMA1_STREAM1
#define ADC_DMA_MODE    STM32_DMA_CR_CHSEL(0) |   /* DMA1 Stream1 Channel0 */ \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */
#endif // ADC

#endif // DMA

#if 1 // ========================== USART ======================================
#define PRINTF_FLOAT_EN TRUE
#define UART_TXBUF_SZ   4096
#define UART_RXBUF_SZ   99

#define UARTS_CNT       1

#define CMD_UART_PARAMS \
    USART1, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN, \
    UART_DMA_TX, UART_DMA_RX, UART_DMA_TX_MODE(UART_DMA_CHNL), UART_DMA_RX_MODE(UART_DMA_CHNL), \
    uartclkHSI // Use independent clock

#endif

#endif //BOARD_H__