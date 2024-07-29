/*
 * acg_lsm6ds3_defins.h
 *
 *  Created on: 7 рту. 2017 у.
 *      Author: Kreyl
 */

#pragma once

#define ACG_DMA_TX_MODE \
    STM32_DMA_CR_CHSEL(ACG_DMA_CHNL) |                    \
    DMA_PRIORITY_MEDIUM |                                \
    STM32_DMA_CR_MSIZE_BYTE | /* Size byte */            \
    STM32_DMA_CR_PSIZE_BYTE |                            \
    STM32_DMA_CR_DIR_M2P       /* Mem to peripheral */  \

#define ACG_DMA_RX_MODE  \
    STM32_DMA_CR_CHSEL(ACG_DMA_CHNL) |                  \
    DMA_PRIORITY_MEDIUM |                              \
    STM32_DMA_CR_MSIZE_BYTE | /* Memory Size - Byte */ \
    STM32_DMA_CR_PSIZE_BYTE | /* Periph Size - Byte */ \
    STM32_DMA_CR_MINC |    /* Mem pointer increase */  \
    STM32_DMA_CR_DIR_P2M | /* Periph to Mem */         \
    STM32_DMA_CR_TCIE      /* Enable TxCompleted Int  */

/*******************************************************************************
* Register      : CTRL1_XL
* Address       : 0X10
* Bit Group Name: BW_XL
* Permission    : RW
*******************************************************************************/
typedef enum {
    LSM6DS3_ACC_GYRO_BW_XL_400Hz         = 0x00,
    LSM6DS3_ACC_GYRO_BW_XL_200Hz         = 0x01,
    LSM6DS3_ACC_GYRO_BW_XL_100Hz         = 0x02,
    LSM6DS3_ACC_GYRO_BW_XL_50Hz          = 0x03,
} LSM6DS3_ACC_GYRO_BW_XL_t;

/*******************************************************************************
* Register      : CTRL1_XL
* Address       : 0X10
* Bit Group Name: FS_XL
* Permission    : RW
*******************************************************************************/
typedef enum {
    LSM6DS3_ACC_GYRO_FS_XL_2g        = 0x00,
    LSM6DS3_ACC_GYRO_FS_XL_16g       = 0x04,
    LSM6DS3_ACC_GYRO_FS_XL_4g        = 0x08,
    LSM6DS3_ACC_GYRO_FS_XL_8g        = 0x0C,
} LSM6DS3_ACC_GYRO_FS_XL_t;

/*******************************************************************************
* Register      : CTRL1_XL
* Address       : 0X10
* Bit Group Name: ODR_XL
* Permission    : RW
*******************************************************************************/
typedef enum {
    LSM6DS3_ACC_GYRO_ODR_XL_POWER_DOWN   = 0x00,
    LSM6DS3_ACC_GYRO_ODR_XL_13Hz         = 0x10,
    LSM6DS3_ACC_GYRO_ODR_XL_26Hz         = 0x20,
    LSM6DS3_ACC_GYRO_ODR_XL_52Hz         = 0x30,
    LSM6DS3_ACC_GYRO_ODR_XL_104Hz        = 0x40,
    LSM6DS3_ACC_GYRO_ODR_XL_208Hz        = 0x50,
    LSM6DS3_ACC_GYRO_ODR_XL_416Hz        = 0x60,
    LSM6DS3_ACC_GYRO_ODR_XL_833Hz        = 0x70,
    LSM6DS3_ACC_GYRO_ODR_XL_1660Hz       = 0x80,
    LSM6DS3_ACC_GYRO_ODR_XL_3330Hz       = 0x90,
    LSM6DS3_ACC_GYRO_ODR_XL_6660Hz       = 0xA0,
    LSM6DS3_ACC_GYRO_ODR_XL_13330Hz      = 0xB0,
} LSM6DS3_ACC_GYRO_ODR_XL_t;

/*******************************************************************************
* Register      : CTRL2_G
* Address       : 0X11
* Bit Group Name: FS_G
* Permission    : RW
*******************************************************************************/
typedef enum {
    LSM6DS3_ACC_GYRO_FS_G_245dps         = 0x00,
    LSM6DS3_ACC_GYRO_FS_G_500dps         = 0x04,
    LSM6DS3_ACC_GYRO_FS_G_1000dps        = 0x08,
    LSM6DS3_ACC_GYRO_FS_G_2000dps        = 0x0C,
} LSM6DS3_ACC_GYRO_FS_G_t;

/*******************************************************************************
* Register      : CTRL2_G
* Address       : 0X11
* Bit Group Name: ODR_G
* Permission    : RW
*******************************************************************************/
typedef enum {
    LSM6DS3_ACC_GYRO_ODR_G_POWER_DOWN        = 0x00,
    LSM6DS3_ACC_GYRO_ODR_G_13Hz          = 0x10,
    LSM6DS3_ACC_GYRO_ODR_G_26Hz          = 0x20,
    LSM6DS3_ACC_GYRO_ODR_G_52Hz          = 0x30,
    LSM6DS3_ACC_GYRO_ODR_G_104Hz         = 0x40,
    LSM6DS3_ACC_GYRO_ODR_G_208Hz         = 0x50,
    LSM6DS3_ACC_GYRO_ODR_G_416Hz         = 0x60,
    LSM6DS3_ACC_GYRO_ODR_G_833Hz         = 0x70,
    LSM6DS3_ACC_GYRO_ODR_G_1660Hz        = 0x80,
} LSM6DS3_ACC_GYRO_ODR_G_t;

