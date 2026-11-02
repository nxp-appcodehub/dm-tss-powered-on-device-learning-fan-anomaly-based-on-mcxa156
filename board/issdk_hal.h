/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file issdk_hal.h
 * @brief Wrapper for Hardware Abstraction Layer (HAL)

    This file simply provides one level of indirection for the developer
    to select the particular Hardware Abstraction Layer they would like to use.
*/

#ifndef __ISSDK_HAL_H__
#define __ISSDK_HAL_H__

#include "fsl_lpi2c_cmsis.h"
#include "fsl_lpspi_cmsis.h"
#include "fsl_lpuart_cmsis.h"


/* Include the sensor shield board header file matching the selected sensor.
 * ACCEL_SENSOR_SELECT lives in sensors/accel_sensor.h and defaults there, so the two
 * shield headers (which both define SHIELD_NAME) are never pulled in together. */
#include "accel_sensor.h"

#if (ACCEL_SENSOR_SELECT == ACCEL_SENSOR_BMI323)
#include "c6dofimu20_click.h" //6DOF IMU 20 Click (BMI323) shield board header file
#else
#include "a8974_click.h"      //ACCEL 4 Click (FXLS8974CF) shield board header file
#endif

#include "frdmmcxa156.h"
// Pin mapping and driver information for default I2C brought to shield
// By default, we use I2C_S1 defined in the lpc55s16.h file.
// Other options: I2C_S2.
// S1 is on D15:14.  S2 is on A5:4.
#define I2C_S_SCL_PIN      I2C_S1_SCL_PIN
#define I2C_S_SDA_PIN      I2C_S1_SDA_PIN
#define I2C_S_DRIVER       I2C_S1_DRIVER
#define I2C_S_SIGNAL_EVENT I2C_S1_SIGNAL_EVENT
#define I2C_S_DEVICE_INDEX I2C_S1_DEVICE_INDEX

#endif // __ISSDK_HAL_H__
