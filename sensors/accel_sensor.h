/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file accel_sensor.h
 * @brief Sensor-agnostic accelerometer interface used by the fan anomaly application.
 *
 * The application only ever talks to the ACCEL_SENSOR_xxx() API declared here, so the
 * physical sensor can be swapped by changing a single macro (::ACCEL_SENSOR_SELECT).
 *
 * Two sensors are supported:
 *  - ::ACCEL_SENSOR_FXLS8974 : NXP FXLS8974CF on the ACCEL-4-CLICK board (I2C, 0x18)
 *  - ::ACCEL_SENSOR_BMI323   : Bosch BMI323 on the 6DOF-IMU-20-CLICK board (I2C, 0x68)
 *
 * Both back-ends are configured for the same behaviour:
 *  - 400 Hz accelerometer ODR, +/-2 g full scale
 *  - FIFO buffering with a watermark interrupt on the mikroBUS INT pin
 *  - samples reported as int16 counts in the *FXLS8974 scale* (~1024 LSB/g @ 2 g) so that
 *    a model trained with one sensor stays usable with the other
 *    (see ::ACCEL_SENSOR_NORMALIZE_SCALE).
 *
 * The host side (source/sensor_task.c) groups the samples into the model input window:
 * 128 samples x 3 axes (::APP_TSS_WINDOW_LEN / ::APP_TSS_WINDOW_DIM in source/tss_app.h).
 */

#ifndef ACCEL_SENSOR_H_
#define ACCEL_SENSOR_H_

#include <stdint.h>

/*******************************************************************************
 * Sensor selection
 ******************************************************************************/
/*! @brief Selector value for the NXP FXLS8974CF (ACCEL-4-CLICK). */
#define ACCEL_SENSOR_FXLS8974 (1)
/*! @brief Selector value for the Bosch BMI323 (6DOF-IMU-20-CLICK). */
#define ACCEL_SENSOR_BMI323 (2)

/*! @brief The sensor the firmware is built for.
 *
 *  Override from the build system (-DACCEL_SENSOR_SELECT=ACCEL_SENSOR_FXLS8974) or
 *  by editing the default below.
 */
#ifndef ACCEL_SENSOR_SELECT
#define ACCEL_SENSOR_SELECT ACCEL_SENSOR_BMI323
#endif

#if (ACCEL_SENSOR_SELECT != ACCEL_SENSOR_FXLS8974) && (ACCEL_SENSOR_SELECT != ACCEL_SENSOR_BMI323)
#error "ACCEL_SENSOR_SELECT must be ACCEL_SENSOR_FXLS8974 or ACCEL_SENSOR_BMI323"
#endif

/*! @brief When 1, raw counts are rescaled to the FXLS8974 2 g scale (~1024 LSB/g).
 *
 *  This keeps the numeric range fed to the Time Series Studio model identical for both
 *  sensors, so a model trained on an FXLS8974 capture can be reused as-is. Set to 0 to
 *  feed the sensor's native counts instead (requires re-training the model).
 */
#ifndef ACCEL_SENSOR_NORMALIZE_SCALE
#define ACCEL_SENSOR_NORMALIZE_SCALE (1)
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief FIFO watermark, in samples, that triggers the data ready interrupt. */
#define ACCEL_SENSOR_FIFO_WMRK (20U)

/*! @brief Maximum number of samples a single ::ACCEL_SENSOR_ReadData() call can return. */
#define ACCEL_SENSOR_FIFO_DEPTH (32U)

/*! @brief Interrupt pin wiring of the mikroBUS INT line on FRDM-MCXA156.
 *
 *  Both click boards raise their watermark interrupt on the mikroBUS INT pin, so this
 *  mapping is shared: the FXLS8974 drives it from its INT1 output and the BMI323 from
 *  its INT2 output (mikroBUS INT is wired to BMI323 INT2 on the 6DOF IMU 20 Click).
 */
#define ACCEL_SENSOR_INT_GPIO GPIO3
#define ACCEL_SENSOR_INT_PIN  19U
#define ACCEL_SENSOR_INT_IRQ  GPIO3_IRQn
#define ACCEL_SENSOR_INT_ISR  GPIO3_IRQHandler

/*! @brief One accelerometer sample. */
typedef struct
{
    int16_t accel[3]; /*!< X, Y, Z acceleration counts. */
} accel_sensor_data_t;

/*******************************************************************************
 * API
 ******************************************************************************/
/*! @brief       Bring up the communication bus and the accelerometer.
 *  @details     Initializes the sensor board control pins, the I2C bus, probes the
 *               device ID and applies the application configuration (400 Hz, 2 g, FIFO
 *               watermark interrupt on the mikroBUS INT pin). After this call the sensor
 *               is active and starts filling its FIFO.
 *  @constraints Must be the first ACCEL_SENSOR API called.
 *  @reentrant   No
 *  @return      ::SENSOR_ERROR_NONE on success, an ::ESensorErrors code otherwise.
 */
int32_t ACCEL_SENSOR_Init(void);

/*! @brief       Reset the sensor and release its handle.
 *  @constraints Only valid after a successful ::ACCEL_SENSOR_Init().
 *  @reentrant   No
 *  @return      ::SENSOR_ERROR_NONE on success, an ::ESensorErrors code otherwise.
 */
int32_t ACCEL_SENSOR_Deinit(void);

/*! @brief       Read the device identification register.
 *  @param[out]  pWhoAmI  receives the WHO_AM_I / CHIP_ID value.
 *  @constraints Only valid after a successful ::ACCEL_SENSOR_Init().
 *  @reentrant   No
 *  @return      ::SENSOR_ERROR_NONE on success, an ::ESensorErrors code otherwise.
 */
int32_t ACCEL_SENSOR_GetWhoAmI(uint8_t *pWhoAmI);

/*! @brief       Drain the pending accelerometer samples from the sensor FIFO.
 *  @details     Reads however many complete samples the FIFO currently holds, up to
 *               @p maxSamples. Returns ::SENSOR_ERROR_NONE with @p *pNumSamples == 0 when
 *               the FIFO is empty. Samples are scaled per ::ACCEL_SENSOR_NORMALIZE_SCALE.
 *  @param[out]  pSamples     buffer receiving the samples, at least @p maxSamples entries.
 *  @param[in]   maxSamples   capacity of @p pSamples, in samples.
 *  @param[out]  pNumSamples  receives the number of samples actually written.
 *  @constraints Only valid after a successful ::ACCEL_SENSOR_Init().
 *  @reentrant   No
 *  @return      ::SENSOR_ERROR_NONE on success, an ::ESensorErrors code otherwise.
 */
int32_t ACCEL_SENSOR_ReadData(accel_sensor_data_t *pSamples, uint32_t maxSamples, uint32_t *pNumSamples);

/*! @brief       Read the on-die temperature.
 *  @param[out]  pTemperature  receives the temperature in degrees Celsius.
 *  @constraints Only valid after a successful ::ACCEL_SENSOR_Init().
 *  @reentrant   No
 *  @return      ::SENSOR_ERROR_NONE on success, an ::ESensorErrors code otherwise.
 */
int32_t ACCEL_SENSOR_GetTemperature(int8_t *pTemperature);

/*! @brief       Name of the sensor this build talks to, for logging.
 *  @return      A static, NUL-terminated string, e.g. "BMI323".
 */
const char *ACCEL_SENSOR_GetName(void);

#endif /* ACCEL_SENSOR_H_ */
