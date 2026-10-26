/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file accel_sensor_fxls8974.c
 * @brief ::accel_sensor.h back-end for the NXP FXLS8974CF (ACCEL-4-CLICK).
 *
 * Wraps the ISSDK fxls8974 register driver into the sensor-agnostic API. Built only
 * when ACCEL_SENSOR_SELECT == ACCEL_SENSOR_FXLS8974.
 */

#include "accel_sensor.h"

#if (ACCEL_SENSOR_SELECT == ACCEL_SENSOR_FXLS8974)

#include "fsl_gpio.h"
#include "issdk_hal.h"
#include "gpio_driver.h"
#include "fxls8974_drv.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief Bytes per accelerometer sample in the FIFO (X/Y/Z, 16-bit little endian). */
#define FXLS8974_SAMPLE_SIZE (6U)

/* INTF_SEL / BT_MODE pins that select the FXLS8974 I2C interface on FRDM-MCXA156. */
#define FXLS8974_INTF_SEL_PORT GPIO1
#define FXLS8974_INTF_SEL_PIN  21U
#define FXLS8974_BT_MODE_PORT  GPIO1
#define FXLS8974_BT_MODE_PIN   2U

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Application configuration: 4 g full scale, 400 Hz ODR, FIFO watermark IRQ. */
static const registerwritelist_t cFxls8974ConfigNormal[] = {
    /*! Set Full-scale range as 4G. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_4G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    /*! Disable Self-Test. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_DISABLED, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_MASK},
    /*! Set Wake Mode ODR Rate as 400Hz. */
    {FXLS8974_SENS_CONFIG3, FXLS8974_SENS_CONFIG3_WAKE_ODR_400HZ, FXLS8974_SENS_CONFIG3_WAKE_ODR_MASK},
    /*! Enable the FIFO in stop-on-full mode with the application watermark. */
    {FXLS8974_BUF_CONFIG1, FXLS8974_BUF_CONFIG1_BUF_TYPE_FIFO | FXLS8974_BUF_CONFIG1_BUF_MODE_STOP_MODE, 0x00},
    {FXLS8974_BUF_CONFIG2, ACCEL_SENSOR_FIFO_WMRK, 0x00},
    /*! Enable Interrupts for Data Ready / Buffer Events. */
    {FXLS8974_INT_EN, FXLS8974_INT_EN_DRDY_EN_EN | FXLS8974_INT_EN_BUF_EN_EN,
     FXLS8974_INT_EN_DRDY_EN_MASK | FXLS8974_INT_EN_BUF_EN_MASK},
    /*! Enable Temperature sensor. */
    {FXLS8974_SENS_CONFIG2, FXLS8974_SENS_CONFIG2_ANIC_TEMP_EN, FXLS8974_SENS_CONFIG2_ANIC_TEMP_MASK},
    /*! Set Self-Test ODR to 100 Hz. */
    {0x38, 0x05, 0x00},
    {0x2F, 0x38, 0x00},
    {0x30, 0xD8, 0x00},
    {0x33, 0xC0, 0x00},
    {0x34, 0xFF, 0x00},
    {0x35, 0x40, 0x00},
    __END_WRITE_DATA__};

/*! @brief Read list for the FIFO fill level. */
static const registerreadlist_t cFxls8974BufStatus[] = {{.readFrom = FXLS8974_BUF_STATUS, .numBytes = 1},
                                                        __END_READ_DATA__};

/*! @brief Read list for the FIFO payload. @c numBytes is patched per read. */
static registerreadlist_t cFxls8974BufRead[] = {{.readFrom = FXLS8974_BUF_X_LSB, .numBytes = 0}, __END_READ_DATA__};

/*! @brief Read list for the temperature register. */
static const registerreadlist_t cFxls8974Temp[] = {{.readFrom = FXLS8974_TEMP_OUT, .numBytes = 1}, __END_READ_DATA__};

/*! @brief Sensor handle. */
static fxls8974_i2c_sensorhandle_t g_fxls8974Driver;

/*! @brief Scratch buffer for one FIFO burst. */
static uint8_t g_fifoBuffer[ACCEL_SENSOR_FIFO_DEPTH * FXLS8974_SAMPLE_SIZE];

/*******************************************************************************
 * Local functions
 ******************************************************************************/
/*! @brief Drive INTF_SEL and BT_MODE low to select the I2C interface. */
static void FXLS8974_InitInterfacePins(void)
{
    gpio_pin_config_t modeConfig = {
        kGPIO_DigitalOutput,
        0,
    };

    GPIO_PinInit(FXLS8974_INTF_SEL_PORT, FXLS8974_INTF_SEL_PIN, &modeConfig);
    GPIO_PinInit(FXLS8974_BT_MODE_PORT, FXLS8974_BT_MODE_PIN, &modeConfig);

    /* Set INTF_SEL pin to Low (I2C). */
    GPIO_PinWrite(FXLS8974_INTF_SEL_PORT, FXLS8974_INTF_SEL_PIN, 0);
    /* Set BT_MODE pin to Low. */
    GPIO_PinWrite(FXLS8974_BT_MODE_PORT, FXLS8974_BT_MODE_PIN, 0);
}

/*! @brief Configure the mikroBUS INT pin as a rising-edge interrupt source. */
static void FXLS8974_InitWakeupInt(void)
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

/*******************************************************************************
 * Code
 ******************************************************************************/
int32_t ACCEL_SENSOR_Init(void)
{
    int32_t status;
    uint8_t whoami = 0;
    ARM_DRIVER_I2C *pI2Cdrv = &I2C_S_DRIVER;

    /*! Select the sensor's I2C interface and arm the watermark interrupt. */
    FXLS8974_InitInterfacePins();
    FXLS8974_InitWakeupInt();

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

    /*! Initialize the FXLS8974 sensor driver. */
    status = FXLS8974_I2C_Initialize(&g_fxls8974Driver, &I2C_S_DRIVER, I2C_S_DEVICE_INDEX, FXLS8974_I2C_ADDR, &whoami);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /*! Set the task to be executed while waiting for I2C transactions to complete. */
    FXLS8974_I2C_SetIdleTask(&g_fxls8974Driver, (registeridlefunction_t)SMC_SetPowerModeVlpr, SMC);

    /*! Apply the application configuration. */
    return FXLS8974_I2C_Configure(&g_fxls8974Driver, cFxls8974ConfigNormal);
}

int32_t ACCEL_SENSOR_Deinit(void)
{
    return FXLS8974_I2C_DeInit(&g_fxls8974Driver);
}

int32_t ACCEL_SENSOR_GetWhoAmI(uint8_t *pWhoAmI)
{
    const registerreadlist_t whoAmIRead[] = {{.readFrom = FXLS8974_WHO_AM_I, .numBytes = 1}, __END_READ_DATA__};

    if (NULL == pWhoAmI)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    return FXLS8974_I2C_ReadData(&g_fxls8974Driver, whoAmIRead, pWhoAmI);
}

int32_t ACCEL_SENSOR_ReadData(accel_sensor_data_t *pSamples, uint32_t maxSamples, uint32_t *pNumSamples)
{
    int32_t status;
    uint8_t bufStatus = 0;
    uint32_t sampleCount;

    if ((NULL == pSamples) || (NULL == pNumSamples))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    *pNumSamples = 0;

    /*! Find out how many samples the FIFO holds. */
    status = FXLS8974_I2C_ReadData(&g_fxls8974Driver, cFxls8974BufStatus, &bufStatus);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    sampleCount = (uint32_t)(bufStatus & FXLS8974_BUF_STATUS_BUF_CNT_MASK);
    if (sampleCount > maxSamples)
    {
        sampleCount = maxSamples;
    }
    if (sampleCount > ACCEL_SENSOR_FIFO_DEPTH)
    {
        sampleCount = ACCEL_SENSOR_FIFO_DEPTH;
    }
    if (0U == sampleCount)
    {
        return SENSOR_ERROR_NONE;
    }

    /*! Burst-read the samples out of the FIFO. */
    cFxls8974BufRead[0].numBytes = (uint8_t)(sampleCount * FXLS8974_SAMPLE_SIZE);
    status                       = FXLS8974_I2C_ReadData(&g_fxls8974Driver, cFxls8974BufRead, g_fifoBuffer);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /*! Assemble the little-endian 16-bit counts.
     *  The FXLS8974 already reports ~512 LSB/g at 4 g, which is the reference scale,
     *  so no rescaling is needed here regardless of ACCEL_SENSOR_NORMALIZE_SCALE. */
    for (uint32_t i = 0; i < sampleCount; i++)
    {
        const uint8_t *pRaw = &g_fifoBuffer[i * FXLS8974_SAMPLE_SIZE];

        pSamples[i].accel[0] = (int16_t)(((uint16_t)pRaw[1] << 8) | pRaw[0]);
        pSamples[i].accel[1] = (int16_t)(((uint16_t)pRaw[3] << 8) | pRaw[2]);
        pSamples[i].accel[2] = (int16_t)(((uint16_t)pRaw[5] << 8) | pRaw[4]);
    }

    *pNumSamples = sampleCount;

    return SENSOR_ERROR_NONE;
}

int32_t ACCEL_SENSOR_GetTemperature(int8_t *pTemperature)
{
    int32_t status;
    uint8_t rawTemp = 0;

    if (NULL == pTemperature)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    status = FXLS8974_I2C_ReadData(&g_fxls8974Driver, cFxls8974Temp, &rawTemp);
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /*! TEMP_OUT is a signed offset from 25 degC. */
    *pTemperature = (int8_t)((int8_t)rawTemp + 25);

    return SENSOR_ERROR_NONE;
}

const char *ACCEL_SENSOR_GetName(void)
{
    return "FXLS8974";
}

#endif /* ACCEL_SENSOR_SELECT == ACCEL_SENSOR_FXLS8974 */
