/*
*                Copyright 2014, Marvell International Ltd.
* This code contains confidential information of Marvell Semiconductor, Inc.
* No rights are granted herein under any patent, mask work right or copyright
* of Marvell or any third party.
* Marvell reserves the right at its sole discretion to request that this code
* be immediately returned to Marvell. This code is provided "as is".
* Marvell makes no warranties, express, implied or otherwise, regarding its
* accuracy, completeness or performance.
*/

/*!
 * \file    poolconfig.h
 * \brief  This header file defines block pool configuration for stack. 
*/
//#ifndef _HOGP_DEV_POOLCONFIG_H_
//#define _HOGP_DEV_POOLCONFIG_H_

#include "buffermgmtlib.h"


/******************************** Pool ID 0 ********************************/
// Pool 0 buffer pool sizes and buffer counts
// Buffer 0
#define POOL_ID0_BUF0_SZ        32
#define POOL_ID0_BUF0_CNT       7

// Calculate the real size of the buffer by adding buffer descriptor size,
#define POOL_ID0_BUF0_REAL_SZ   (BML_ADD_DESC(POOL_ID0_BUF0_SZ))

// Pool 0 total buffer0 size calculation
#define POOL_ID0_BUF0_TOT_SZ    (POOL_ID0_BUF0_REAL_SZ * POOL_ID0_BUF0_CNT)

// Buffer 1
#define POOL_ID0_BUF1_SZ        64 
#define POOL_ID0_BUF1_CNT      15

// Calculate the real size of the buffer by adding buffer descriptor size,
#define POOL_ID0_BUF1_REAL_SZ   (BML_ADD_DESC(POOL_ID0_BUF1_SZ))

// Pool 0 total buffer1 size calculation
#define POOL_ID0_BUF1_TOT_SZ    (POOL_ID0_BUF1_REAL_SZ * POOL_ID0_BUF1_CNT)

// Buffer 2
#define POOL_ID0_BUF2_SZ        128
#define POOL_ID0_BUF2_CNT       8

// Calculate the real size of the buffer by adding buffer descriptor size,
#define POOL_ID0_BUF2_REAL_SZ   (BML_ADD_DESC(POOL_ID0_BUF2_SZ))

// Pool 0 total buffer2 size calculation
#define POOL_ID0_BUF2_TOT_SZ    (POOL_ID0_BUF2_REAL_SZ * POOL_ID0_BUF2_CNT)

// Buffer 3
#define POOL_ID0_BUF3_SZ        356//290
#define POOL_ID0_BUF3_CNT       3

// Calculate the real size of the buffer by adding buffer descriptor size,
#define POOL_ID0_BUF3_REAL_SZ   (BML_ADD_DESC(POOL_ID0_BUF3_SZ))

// Pool 0 total buffer3 size calculation
#define POOL_ID0_BUF3_TOT_SZ    (POOL_ID0_BUF3_REAL_SZ * POOL_ID0_BUF3_CNT)

/**********************************************/
/******* Pool ID 0 total               ********/
/**********************************************/
// Pool 0 total includes 1 block buffer pools
#define POOL_ID0_SZ             (POOL_ID0_BUF0_TOT_SZ +  POOL_ID0_BUF1_TOT_SZ + POOL_ID0_BUF2_TOT_SZ + POOL_ID0_BUF3_TOT_SZ)

// Pool 0 block buffer pool pointer overhead calculation
#define POOL_ID0_OVERHEAD       ((POOL_ID0_BUF0_CNT + POOL_ID0_BUF1_CNT + POOL_ID0_BUF2_CNT + POOL_ID0_BUF3_CNT) * BML_STACK_OVERHEAD)

//#endif /* _POOLCONFIG_H_ */

