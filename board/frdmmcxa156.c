/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file frdmmcxn947.c
 * @brief The frdmmcxn947.c file defines GPIO pins and I2C CMSIS utilities for frdmmcxn947 board.
 */

#include <frdmmcxa156.h>

// I2C2 Pin Handles
gpioHandleKSDK_t D14 = {.base = GPIO3, .pinNumber = 28, .mask = 1 << (28), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};
gpioHandleKSDK_t D15 = {.base = GPIO3, .pinNumber = 27, .mask = 1 << (27), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};

// SPI1 Handle
/* [PIO0_20] (coords: 74) configured as SPI master-out/slave-in (MOSI) data */
gpioHandleKSDK_t D11 = {.base = GPIO3, .pinNumber = 9, .mask = 1 << (9), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};
/* [PIO0_19] (coords: 90) is configured as SPI master-in/slave-out (MISO) data */
gpioHandleKSDK_t D12 = {.base = GPIO3, .pinNumber = 8, .mask = 1 << (8), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};
/* [PIO0_21] (coords: 76) configured as SPI SCK */
gpioHandleKSDK_t D13 = {.base = GPIO3, .pinNumber = 10, .mask = 1 << (10), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};
/* [PIO0_21] (coords: 76) configured as SPI SCK */
gpioHandleKSDK_t D10 = {.base = GPIO3, .pinNumber = 11, .mask = 1 << (11), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};

gpioHandleKSDK_t D2 = {.base = GPIO2, .pinNumber = 5, .mask = 1 << (5), .clockName = kCLOCK_GateGPIO2, .portNumber = 2};

// LPCXpresso55s69 Internal Peripheral Pin Definitions
gpioHandleKSDK_t RED_LED = {
    .base = GPIO3, .pinNumber = 12, .mask = 1 << (12), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};
gpioHandleKSDK_t GREEN_LED = {
    .base = GPIO3, .pinNumber = 13, .mask = 1 << (13), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};
gpioHandleKSDK_t BLUE_LED = {
    .base = GPIO3, .pinNumber = 0, .mask = 1 << (0), .clockName = kCLOCK_GateGPIO3, .portNumber = 3};

/*!
 * @brief Configures the system to WAIT power mode.
 *        API name used from Kinetis family to maintain compatibility.
 *
 * @param Power peripheral base address (dummy).
 * @return Configuration error code.
 */
status_t SMC_SetPowerModeWait(void *arg)
{
    // POWER_EnterSleep();

    return kStatus_Success;
}

/*!
 * @brief Configures the system to VLPR power mode.
 *        API name used from Kinetis family to maintain compatibility.
 *
 * @param Power peripheral base address (dummy).
 * @return Configuration error code.
 */
status_t SMC_SetPowerModeVlpr(void *arg)
{
    // POWER_EnterSleep();

    return kStatus_Success;
}

/*! @brief       Determines the Clock Frequency feature.
 *  @details     The Clock Frequecny computation API required by fsl_uart_cmsis.c.
 *  @param[in]   void
 *  @Constraints None
 *  @Reentrant   Yes
 *  @return      uint32_t Returns the clock frequency .
 */
uint32_t LPUART0_GetFreq(void)
{
    return CLOCK_GetLpuartClkFreq(0u);
}

/*! @brief       Determines the Clock Frequency feature.
 *  @details     The Clock Frequecny computation API required by fsl_i2c_cmsis.c.
 *  @param[in]   void
 *  @Constraints None
 *  @Reentrant   Yes
 *  @return      uint32_t Returns the clock frequency .
 */
uint32_t LPI2C0_GetFreq(void)
{
    /* Attach peripheral clock */
    CLOCK_SetupFRO12MClocking();
    CLOCK_SetClockDiv(kCLOCK_DivLPI2C0, 1u);
    CLOCK_AttachClk(kFRO12M_to_LPI2C0);

    return CLOCK_GetLpi2cClkFreq(0U);
}

uint32_t LPI2C3_GetFreq(void)
{
    /* Attach peripheral clock */
    CLOCK_SetupFRO12MClocking();
    CLOCK_SetClockDiv(kCLOCK_DivLPI2C3, 1u);
    CLOCK_AttachClk(kFRO12M_to_LPI2C3);

    return CLOCK_GetLpi2cClkFreq(3U);
}

/*! @brief       Determines the Clock Frequency feature.
 *  @details     The Clock Frequecny computation API required by fsl_spi_cmsis.c.
 *  @param[in]   void
 *  @Constraints None
 *  @Reentrant   Yes
 *  @return      uint32_t Returns the clock frequency .
 */
uint32_t LPSPI1_GetFreq(void)
{
    /* Attach peripheral clock */
    /* Attach peripheral clock */
    CLOCK_SetClockDiv(kCLOCK_DivLPSPI1, 1u);
    CLOCK_AttachClk(kFRO12M_to_LPSPI1);

    return CLOCK_GetLpspiClkFreq(1);
}
