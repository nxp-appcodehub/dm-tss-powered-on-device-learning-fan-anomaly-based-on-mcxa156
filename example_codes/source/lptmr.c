/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017,2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "clock_config.h"
#include "fsl_lptmr.h"
#include "fsl_clock.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LPTMR_BASE          LPTMR0
#define LPTMR_PERIOD_USEC   1000000000
#define LPTMR_IRQn          LPTMR0_IRQn
#define LPTMR_SOURCE_CLOCK  CLK_FRO_12MHZ
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile static uint32_t lptmrSeconds = 0U;
/*******************************************************************************
 * Code
 ******************************************************************************/
void LPTMR0_IRQHandler(void)
{
    LPTMR_ClearStatusFlags(LPTMR_BASE, kLPTMR_TimerCompareFlag);
    lptmrSeconds++;
    /*
     * Workaround for TWR-KV58: because write buffer is enabled, adding
     * memory barrier instructions to make sure clearing interrupt flag completed
     * before go out ISR
     */
    __DSB();
    __ISB();
}
void LPTMR_init()
{
    lptmr_config_t lptmrConfig;

    CLOCK_SetClockDiv(kCLOCK_DivLPTMR0, 1u);
    CLOCK_AttachClk(kFRO12M_to_LPTMR0);
    /* Configure LPTMR */
    /*
     * lptmrConfig.timerMode = kLPTMR_TimerModeTimeCounter;
     * lptmrConfig.pinSelect = kLPTMR_PinSelectInput_0;
     * lptmrConfig.pinPolarity = kLPTMR_PinPolarityActiveHigh;
     * lptmrConfig.enableFreeRunning = false;
     * lptmrConfig.bypassPrescaler = true;
     * lptmrConfig.prescalerClockSource = kLPTMR_PrescalerClock_1;
     * lptmrConfig.value = kLPTMR_Prescale_Glitch_0;
     */
    LPTMR_GetDefaultConfig(&lptmrConfig);
    lptmrConfig.prescalerClockSource = kLPTMR_PrescalerClock_3;
    lptmrConfig.bypassPrescaler      = false;
    lptmrConfig.value                = kLPTMR_Prescale_Glitch_1;
    /* Initialize the LPTMR */
    LPTMR_Init(LPTMR_BASE, &lptmrConfig);
    /*
     * Set timer period.
     * Note : the parameter "ticks" of LPTMR_SetTimerPeriod should be equal or greater than 1.
     */
    LPTMR_SetTimerPeriod(LPTMR_BASE, USEC_TO_COUNT(LPTMR_PERIOD_USEC, LPTMR_SOURCE_CLOCK >> (lptmrConfig.value + 1)));
    /* Enable timer interrupt */
    LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
    /* Enable at the NVIC */
    EnableIRQWithPriority(LPTMR_IRQn, 7);
    /* Start counting */
    LPTMR_StartTimer(LPTMR_BASE);
}
uint64_t LPTMR_time_get_us()
{
    return (uint64_t)lptmrSeconds * LPTMR_PERIOD_USEC +
           COUNT_TO_USEC(LPTMR_GetCurrentTimerCount(LPTMR0), LPTMR_SOURCE_CLOCK >> (kLPTMR_Prescale_Glitch_1 + 1));
}
uint64_t framework_get_time_us()
{
    return LPTMR_time_get_us();
}
