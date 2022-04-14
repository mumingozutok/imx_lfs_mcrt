/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "mcrt_os_adaptor.h"
#include "mcrt_adaptor.h"
#include "mcrt_fs_adaptor.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

/*!
 * @brief Application entry point.
 */
int main(void)
{
    /* Init board hardware. */
     BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    /*Clock setting for flexspi1*/
    /*CLOCK_SetRootClockDiv(kCLOCK_Root_Flexspi1, 2);
    CLOCK_SetRootClockMux(kCLOCK_Root_Flexspi1, 0);*/

#ifdef MCRT_FS_ADAPTOR
    init_fs();
#endif

    initiate_ECU();

#ifdef MCRT_OS_ADAPTOR

    init_mcrt_os();

    vTaskStartScheduler();
    for (;;){
    }

#else
    while (1)
    {
    	mcCore_SM();
    }
#endif
}
