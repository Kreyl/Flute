/*
 * mem_msd_glue.h
 *
 *  Created on: 30 ���. 2016 �.
 *      Author: Kreyl
 */

#ifndef MEM_MSD_GLUE_H__
#define MEM_MSD_GLUE_H__

#include "kl_sd.h"

extern SDCDriver SDCD1;

#define MSD_BLOCK_CNT   SDCD1.capacity
#define MSD_BLOCK_SZ    512

uint8_t MSDRead(uint32_t BlockAddress, uint8_t *Ptr, uint32_t BlocksCnt);
uint8_t MSDWrite(uint32_t BlockAddress, uint8_t *Ptr, uint32_t BlocksCnt);

#endif //MEM_MSD_GLUE_H__