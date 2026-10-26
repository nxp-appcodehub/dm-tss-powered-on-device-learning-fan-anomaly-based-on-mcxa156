/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file accel_sensor_bmi323.c
 * @brief ::accel_sensor.h back-end for the Bosch BMI323 (6DOF IMU 20 Click).
 *
 * Wraps the bmi323 register driver into the sensor-agnostic API. Built only when
 * ACCEL_SENSOR_SELECT == ACCEL_SENSOR_BMI323.
 *
 * The accelerometer is configured exactly like the FXLS8974 back-end: 400 Hz ODR,
 * +/-4 g full scale, FIFO buffering and a watermark interrupt on the mikroBUS INT pin.
 * Only the accelerometer is stored in the FIFO; the gyroscope stays powered down since
 * the application feeds three axes to the model.
 */

#include "accel_sensor.h"

#if (ACCEL_SENSOR_SELECT == ACCEL_SENSOR_BMI323)

#include "fsl_gpio.h"
#include "issdk_hal.h"
#include "gpio_driver.h"
#include "bmi323_drv.h"

extern void BOARD_DELAY_ms(int ms);

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief FIFO watermark expressed in 16-bit words, as the device expects it.
 *
 *  With only the accelerometer stored, each FIFO frame is
 *  ::BMI323_FIFO_ACC_FRAME_WORDS words long. */
#define BMI323_FIFO_WMRK_WORDS (ACCEL_SENSOR_FIFO_WMRK * BMI323_FIFO_ACC_FRAME_WORDS)

/*! @brief Right shift that maps BMI323 4 g counts (8192 LSB/g) onto the FXLS8974
 *         4 g scale (512 LSB/g). The ratio is exactly 16, so a shift is lossless. */
#define BMI323_SCALE_SHIFT (4)

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Application configuration: 4 g full scale, 400 Hz ODR, FIFO watermark IRQ
 *         routed to INT2, which is the pin wired to mikroBUS INT on this click board. */
static const bmi323_regwritelist_t cBmi323ConfigNormal[] = {
    /*! Accelerometer: high performance mode, no averaging, ODR/2 bandwidth, 4 g, 400 Hz. */
    {BMI323_ACC_CONF,
     BMI323_ACC_CONF_MODE_HIGH_PERF | BMI323_ACC_CONF_AVG_1 | BMI323_ACC_CONF_BW_ODR_2 |
         BMI323_ACC_CONF_RANGE_4G | BMI323_ACC_CONF_ODR_400HZ,
     0x0000},
    /*! Gyroscope: not used by the application, keep it powered down. */
    {BMI323_GYR_CONF, BMI323_GYR_CONF_MODE_DISABLE, 0x0000},
    /*! FIFO holds accelerometer frames only and keeps running when it fills up. */
    {BMI323_FIFO_CONF, BMI323_FIFO_CONF_ACC_EN, 0x0000},
    /*! Raise the watermark interrupt once a full application batch is buffered. */
    {BMI323_FIFO_WATERMARK, BMI323_FIFO_WMRK_WORDS, 0x0000},
    /*! INT2 as a push-pull, active high output. */
    {BMI323_IO_INT_CTRL,
     BMI323_IO_INT_CTRL_INT2_OUTPUT_EN | BMI323_IO_INT_CTRL_INT2_PUSH_PULL | BMI323_IO_INT_CTRL_INT2_LVL_HIGH,
     0x0000},
    /*! Pulsed (non latched) interrupts, so the pin returns low on its own. */
    {BMI323_INT_CONF, BMI323_INT_CONF_LATCH_DIS, 0x0000},
    /*! Route the FIFO watermark interrupt to INT2. */
    {BMI323_INT_MAP2, BMI323_INT_MAP2_FWM_INT2, 0x0000},
    /*! Start from an empty FIFO. */
    {BMI323_FIFO_CTRL, BMI323_FIFO_CTRL_FLUSH, 0x0000},
    __END_BMI323_WRITE_DATA__};

/*! @brief Read list for the FIFO fill level. */
static const bmi323_regreadlist_t cBmi323FifoFillLevel[] = {{.readFrom = BMI323_FIFO_FILL_LEVEL, .numRegs = 1},
                                                            __END_BMI323_READ_DATA__};

/*! @brief Read list for the FIFO payload. @c numRegs is patched per read. */
static bmi323_regreadlist_t cBmi323FifoRead[] = {{.readFrom = BMI323_FIFO_DATA, .numRegs = 0},
                                                 __END_BMI323_READ_DATA__};

/*! @brief Read list for the temperature register. */
static const bmi323_regreadlist_t cBmi323Temp[] = {{.readFrom = BMI323_TEMP_DATA, .numRegs = 1},
                                                   __END_BMI323_READ_DATA__};

/*! @brief Read list for the chip identification register. */
static const bmi323_regreadlist_t cBmi323ChipId[] = {{.readFrom = BMI323_CHIP_ID, .numRegs = 1},
                                                     __END_BMI323_READ_DATA__};

/*! @brief Sensor handle. */
static bmi323_i2c_sensorhandle_t g_bmi323Driver;

/*! @brief Scratch buffer for one FIFO burst. */
static uint8_t g_fifoBuffer[ACCEL_SENSOR_FIFO_DEPTH * BMI323_FIFO_ACC_FRAME_BYTES];

/*******************************************************************************
 * Local functions
 ******************************************************************************/
/*! @brief Configure the mikroBUS INT pin as a rising-edge interrupt source. */
static void BMI323_InitWakeupInt(void)
{
    gpio_pin_config_t intConfig = {
        kGPIO_DigitalInput,
        0,
    };

    GPIO_SetPinInterruptConfig(ACCEL_SENSOR_INT_GPIO, ACCEL_SENSOR_INT_PIN, kGPIO_InterruptRisingEdge);

    IRQ_SetPriority(ACCEL_SENSOR_INT_IRQ, 2);
    EnableIRQ(ACCEL_SENSOR_INT_IRQ);
    GPIO_PinInit(ACCEL_SENSOR_INT_GPIO, ACCEL_SENSOR_INT_PIN, &intConfig);
}

/*! @brief Assemble a signed 16-bit count out of a little-endian byte pair. */
static inline int16_t BMI323_Word(const uint8_t *pRaw)
{
    return (int16_t)(((uint16_t)pRaw[1] << 8) | pRaw[0]);
}

/*! @brief Bring the raw count onto the scale the application expects. */
static inline int16_t BMI323_Rescale(int16_t raw)
{
#if (ACCEL_SENSOR_NORMALIZE_SCALE)
    /*! Arithmetic shift: 8192 LSB/g (BMI323 @ 4 g) -> 512 LSB/g (FXLS8974 @ 4 g). */
    return (int16_t)(raw >> BMI323_SCALE_SHIFT);
#else
    return raw;
#endif
}

/*******************************************************************************
 * Code
 ******************************************************************************/
int32_t ACCEL_SENSOR_Init(void)
{
    int32_t status;
    uint8_t chipId          = 0;
    uint16_t dummy          = 0;
    ARM_DRIVER_I2C *pI2Cdrv = &I2C_S_DRIVER;

    /*! Arm the watermark interrupt line before the sensor starts driving it. */
    BMI323_InitWakeupInt();

    /*! Initialize the I2C driver. */
    status = pI2Cdrv->Initialize(I2C_S_SIGNAL_EVENT);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Set the I2C Power mode. */
    status = pI2Cdrv->PowerControl(ARM_POWER_FULL);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Set the I2C bus speed. */
    status = pI2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Initialize the BMI323 sensor driver (also verifies the CHIP_ID). */
    status = BMI323_I2C_Initialize(&g_bmi323Driver, &I2C_S_DRIVER, I2C_S_DEVICE_INDEX, BMI323_I2C_ADDR, &chipId);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /*! Set the task to be executed while waiting for I2C transactions to complete. */
    BMI323_I2C_SetIdleTask(&g_bmi323Driver, (registeridlefunction_t)SMC_SetPowerModeVlpr, SMC);

    /*! Start from a known state: soft reset, then let the device boot. */
    status = BMI323_I2C_WriteReg(&g_bmi323Driver, BMI323_CMD, BMI323_CMD_SOFT_RESET, 0x0000);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }
    BOARD_DELAY_ms(BMI323_TBOOT_MAX);

    /*! A dummy read re-selects the I2C interface after the reset. */
    (void)BMI323_I2C_ReadReg(&g_bmi323Driver, BMI323_CHIP_ID, &dummy);

    /*! Apply the application configuration. */
    return BMI323_I2C_Configure(&g_bmi323Driver, cBmi323ConfigNormal);
}

int32_t ACCEL_SENSOR_Deinit(void)
{
    return BMI323_I2C_DeInit(&g_bmi323Driver);
}

int32_t ACCEL_SENSOR_GetWhoAmI(uint8_t *pWhoAmI)
{
    int32_t status;
    uint8_t raw[BMI323_REG_SIZE] = {0};

    if (NULL == pWhoAmI)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    status = BMI323_I2C_ReadData(&g_bmi323Driver, cBmi323ChipId, raw);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /*! The chip ID lives in the low byte of the register. */
    *pWhoAmI = raw[0];

    return SENSOR_ERROR_NONE;
}

int32_t ACCEL_SENSOR_ReadData(accel_sensor_data_t *pSamples, uint32_t maxSamples, uint32_t *pNumSamples)
{
    int32_t status;
    uint8_t raw[BMI323_REG_SIZE] = {0};
    uint32_t fillLevel;
    uint32_t frameCount;
    uint32_t outCount = 0;

    if ((NULL == pSamples) || (NULL == pNumSamples))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    *pNumSamples = 0;

    /*! Find out how many 16-bit words the FIFO holds. */
    status = BMI323_I2C_ReadData(&g_bmi323Driver, cBmi323FifoFillLevel, raw);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    fillLevel = (uint32_t)((((uint16_t)raw[1] << 8) | raw[0]) & BMI323_FIFO_FILL_LEVEL_MASK);

    /*! Only complete frames can be decoded; a partial frame stays in the FIFO. */
    frameCount = fillLevel / BMI323_FIFO_ACC_FRAME_WORDS;
    if (frameCount > maxSamples)
    {
        frameCount = maxSamples;
    }
    if (frameCount > ACCEL_SENSOR_FIFO_DEPTH)
    {
        frameCount = ACCEL_SENSOR_FIFO_DEPTH;
    }
    if (0U == frameCount)
    {
        return SENSOR_ERROR_NONE;
    }

    /*! Burst-read the frames out of the FIFO. */
    cBmi323FifoRead[0].numRegs = (uint8_t)(frameCount * BMI323_FIFO_ACC_FRAME_WORDS);
    status                     = BMI323_I2C_ReadData(&g_bmi323Driver, cBmi323FifoRead, g_fifoBuffer);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /*! Decode the frames, dropping the placeholder frames the FIFO emits when a sample
     *  was not available. */
    for (uint32_t i = 0; i < frameCount; i++)
    {
        const uint8_t *pRaw = &g_fifoBuffer[i * BMI323_FIFO_ACC_FRAME_BYTES];
        int16_t x           = BMI323_Word(&pRaw[0]);
        int16_t y           = BMI323_Word(&pRaw[2]);
        int16_t z           = BMI323_Word(&pRaw[4]);

        if ((BMI323_FIFO_INVALID_SAMPLE == x) && (BMI323_FIFO_INVALID_SAMPLE == y) &&
            (BMI323_FIFO_INVALID_SAMPLE == z))
        {
            continue;
        }

        pSamples[outCount].accel[0] = BMI323_Rescale(x);
        pSamples[outCount].accel[1] = BMI323_Rescale(y);
        pSamples[outCount].accel[2] = BMI323_Rescale(z);
        outCount++;
    }

    *pNumSamples = outCount;

    return SENSOR_ERROR_NONE;
}

int32_t ACCEL_SENSOR_GetTemperature(int8_t *pTemperature)
{
    int32_t status;
    uint8_t raw[BMI323_REG_SIZE] = {0};
    int16_t rawTemp;

    if (NULL == pTemperature)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    status = BMI323_I2C_ReadData(&g_bmi323Driver, cBmi323Temp, raw);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /*! TEMP_DATA is a signed count: degC = raw / 512 + 23. */
    rawTemp       = BMI323_Word(raw);
    *pTemperature = (int8_t)((rawTemp / BMI323_TEMP_DIVIDER) + BMI323_TEMP_OFFSET);

    return SENSOR_ERROR_NONE;
}

const char *ACCEL_SENSOR_GetName(void)
{
    return "BMI323";
}

#endif /* ACCEL_SENSOR_SELECT == ACCEL_SENSOR_BMI323 */
