/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file bmi323_drv.c
 * @brief The bmi323_drv.c file implements the Bosch BMI323 sensor driver interfaces.
 *
 * Implementation notes
 * --------------------
 * The BMI323 differs from the ISSDK byte-register sensors in two ways that this driver
 * hides from its callers:
 *
 *  1. Every register is a 16-bit little-endian word. Reads return LSB then MSB and
 *     writes must supply both bytes, so the single-byte ::Register_I2C_Write path cannot
 *     be used; 16-bit writes go through ::Register_I2C_BlockWrite instead.
 *  2. A register read is preceded by dummy bytes on the wire (two over I2C, one over
 *     SPI). The read helpers request those extra bytes and strip them, so the buffer the
 *     caller gets back starts at the real payload.
 */

//-----------------------------------------------------------------------
// ISSDK Includes
//-----------------------------------------------------------------------
#include <string.h>
#include "gpio_driver.h"
#include "bmi323_drv.h"

extern void BOARD_DELAY_ms(int ms);

//-----------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------
uint8_t bmi323_spiRead_CmdBuffer[BMI323_SPI_MAX_MSG_SIZE]      = {0};
uint8_t bmi323_spiRead_DataBuffer[BMI323_SPI_MAX_MSG_SIZE]     = {0};
uint8_t bmi323_spiWrite_CmdDataBuffer[BMI323_SPI_MAX_MSG_SIZE] = {0};

//-----------------------------------------------------------------------
// Local Variables
//-----------------------------------------------------------------------
/*! @brief Staging buffer used to strip the leading dummy bytes off a read.
 *
 *  Sized for the largest supported burst: the dummy bytes plus one full FIFO drain. */
#define BMI323_RX_SCRATCH_SIZE (BMI323_I2C_DUMMY_BYTES + 208U)
static uint8_t s_rxScratch[BMI323_RX_SCRATCH_SIZE];

//-----------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------
void BMI323_SPI_ReadPreprocess(void *pCmdOut, uint32_t offset, uint32_t size)
{
    spiCmdParams_t *pSlaveCmd = pCmdOut;

    uint8_t *pWBuff = bmi323_spiRead_CmdBuffer;
    uint8_t *pRBuff = bmi323_spiRead_DataBuffer;

    /* Formatting for Read command of BMI323 SENSOR:
     * address byte with the read bit set, followed by the dummy byte the device
     * returns before the first payload byte. */
    memset(pWBuff, 0, BMI323_SPI_CMD_LEN);
    *(pWBuff) = (uint8_t)(offset | BMI323_SPI_READ_MASK);

    /* Create the slave read command. */
    pSlaveCmd->size         = size + BMI323_SPI_CMD_LEN;
    pSlaveCmd->pWriteBuffer = pWBuff;
    pSlaveCmd->pReadBuffer  = pRBuff;
}

void BMI323_SPI_WritePreprocess(void *pCmdOut, uint32_t offset, uint32_t size, void *pWritebuffer)
{
    spiCmdParams_t *pSlaveCmd = pCmdOut;

    uint8_t *pWBuff = bmi323_spiWrite_CmdDataBuffer;
    uint8_t *pRBuff = bmi323_spiWrite_CmdDataBuffer + size + 1;

    /* Formatting for Write command of BMI323 SENSOR. A write has no dummy byte: the
     * address byte is immediately followed by the payload. */
    *(pWBuff) = (uint8_t)(offset & 0x7F);

    /* Copy the slave write command */
    memcpy(pWBuff + 1, pWritebuffer, size);

    /* Create the slave command. */
    pSlaveCmd->size         = size + 1;
    pSlaveCmd->pWriteBuffer = pWBuff;
    pSlaveCmd->pReadBuffer  = pRBuff;
}

/*******************************************************************************
 * I2C
 ******************************************************************************/
int32_t BMI323_I2C_Initialize(
    bmi323_i2c_sensorhandle_t *pSensorHandle, ARM_DRIVER_I2C *pBus, uint8_t index, uint16_t sAddress, uint8_t *pChipId)
{
    int32_t status;
    uint16_t chipId = 0;

    /*! Check the input parameters. */
    if ((pSensorHandle == NULL) || (pBus == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    pSensorHandle->deviceInfo.deviceInstance = index;
    pSensorHandle->deviceInfo.functionParam  = NULL;
    pSensorHandle->deviceInfo.idleFunction   = NULL;
    pSensorHandle->pCommDrv                  = pBus;
    pSensorHandle->slaveAddress              = sAddress;

    /*! Mark the handle usable so the register helpers below accept it; it is cleared
     *  again if the device does not answer. */
    pSensorHandle->isInitialized = true;

    /*! A dummy read puts the device in I2C mode after power-up. */
    (void)BMI323_I2C_ReadReg(pSensorHandle, BMI323_CHIP_ID, &chipId);

    /*! Read and store the device's CHIP_ID. */
    status = BMI323_I2C_ReadReg(pSensorHandle, BMI323_CHIP_ID, &chipId);
    if (NULL != pChipId)
    {
        *pChipId = (uint8_t)(chipId & BMI323_CHIP_ID_MASK);
    }
    if (SENSOR_ERROR_NONE != status)
    {
        pSensorHandle->isInitialized = false;
        return SENSOR_ERROR_INIT;
    }
    if (BMI323_CHIP_ID_VALUE != (chipId & BMI323_CHIP_ID_MASK))
    {
        pSensorHandle->isInitialized = false;
        return SENSOR_ERROR_BAD_ADDRESS;
    }

    return SENSOR_ERROR_NONE;
}

void BMI323_I2C_SetIdleTask(bmi323_i2c_sensorhandle_t *pSensorHandle,
                            registeridlefunction_t idleTask,
                            void *userParam)
{
    pSensorHandle->deviceInfo.functionParam = userParam;
    pSensorHandle->deviceInfo.idleFunction  = idleTask;
}

int32_t BMI323_I2C_ReadReg(bmi323_i2c_sensorhandle_t *pSensorHandle, uint16_t offset, uint16_t *pValue)
{
    int32_t status;
    uint8_t buffer[BMI323_I2C_DUMMY_BYTES + BMI323_REG_SIZE] = {0};

    /*! Validate for the correct handle. */
    if ((pSensorHandle == NULL) || (pValue == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Request the dummy bytes along with the register word, then skip them. */
    status = Register_I2C_Read(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                               (uint8_t)offset, sizeof(buffer), buffer);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_READ;
    }

    *pValue = (uint16_t)(((uint16_t)buffer[BMI323_I2C_DUMMY_BYTES + 1] << 8) | buffer[BMI323_I2C_DUMMY_BYTES]);

    return SENSOR_ERROR_NONE;
}

int32_t BMI323_I2C_WriteReg(bmi323_i2c_sensorhandle_t *pSensorHandle, uint16_t offset, uint16_t value, uint16_t mask)
{
    int32_t status;
    uint16_t regValue;
    uint8_t payload[BMI323_REG_SIZE];

    /*! Validate for the correct handle. */
    if (pSensorHandle == NULL)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    if (mask)
    {
        /*! Read-modify-write: only the masked field is updated. */
        status = BMI323_I2C_ReadReg(pSensorHandle, offset, &regValue);
        if (SENSOR_ERROR_NONE != status)
        {
            return status;
        }
        regValue = (uint16_t)((regValue & (uint16_t)~mask) | (value & mask));
    }
    else
    {
        /*! Overwrite the whole register. */
        regValue = value;
    }

    /*! Registers are little endian: LSB goes out first. */
    payload[0] = (uint8_t)(regValue & 0xFF);
    payload[1] = (uint8_t)((regValue >> 8) & 0xFF);

    status = Register_I2C_BlockWrite(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                                     (uint8_t)offset, payload, sizeof(payload));
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }

    return SENSOR_ERROR_NONE;
}

int32_t BMI323_I2C_Configure(bmi323_i2c_sensorhandle_t *pSensorHandle, const bmi323_regwritelist_t *pRegWriteList)
{
    int32_t status;
    const bmi323_regwritelist_t *pCmd = pRegWriteList;

    /*! Validate for the correct handle and register write list. */
    if ((pSensorHandle == NULL) || (pRegWriteList == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    /*! Check whether sensor handle is initialized before applying configuration. */
    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Apply the Sensor Configuration based on the Register Write List. */
    while (pCmd->writeTo != 0xFFFF)
    {
        status = BMI323_I2C_WriteReg(pSensorHandle, pCmd->writeTo, pCmd->value, pCmd->mask);
        if (SENSOR_ERROR_NONE != status)
        {
            return status;
        }
        pCmd++;
    }

    return SENSOR_ERROR_NONE;
}

int32_t BMI323_I2C_ReadData(bmi323_i2c_sensorhandle_t *pSensorHandle,
                            const bmi323_regreadlist_t *pReadList,
                            uint8_t *pBuffer)
{
    int32_t status;
    const bmi323_regreadlist_t *pCmd = pReadList;
    uint8_t *pOut                    = pBuffer;

    /*! Validate for the correct handle and register read list. */
    if ((pSensorHandle == NULL) || (pReadList == NULL) || (pBuffer == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    /*! Check whether sensor handle is initialized before reading sensor data. */
    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Parse through the read list and read the registers one burst at a time. */
    for (; pCmd->numRegs != 0; pCmd++)
    {
        uint32_t payloadBytes = (uint32_t)pCmd->numRegs * BMI323_REG_SIZE;
        uint32_t totalBytes   = payloadBytes + BMI323_I2C_DUMMY_BYTES;

        /*! Reject bursts that would overrun the staging buffer or the 8-bit length the
         *  CMSIS register helper takes. */
        if ((totalBytes > BMI323_RX_SCRATCH_SIZE) || (totalBytes > 255U))
        {
            return SENSOR_ERROR_INVALID_PARAM;
        }

        status = Register_I2C_Read(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, pSensorHandle->slaveAddress,
                                   (uint8_t)pCmd->readFrom, (uint8_t)totalBytes, s_rxScratch);
        if (ARM_DRIVER_OK != status)
        {
            return SENSOR_ERROR_READ;
        }

        /*! Hand back only the payload, dummy bytes stripped. */
        memcpy(pOut, &s_rxScratch[BMI323_I2C_DUMMY_BYTES], payloadBytes);
        pOut += payloadBytes;
    }

    return SENSOR_ERROR_NONE;
}

int32_t BMI323_I2C_DeInit(bmi323_i2c_sensorhandle_t *pSensorHandle)
{
    int32_t status;

    if (pSensorHandle == NULL)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    /*! Check whether sensor handle is initialized before triggering sensor reset. */
    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Trigger sensor device reset. */
    status = BMI323_I2C_WriteReg(pSensorHandle, BMI323_CMD, BMI323_CMD_SOFT_RESET, 0);

    /*! De-initialize sensor handle regardless of whether the reset was acknowledged. */
    pSensorHandle->isInitialized = false;

    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /* Wait for the device to finish its internal boot sequence after the soft reset. */
    BOARD_DELAY_ms(BMI323_TBOOT_MAX);

    return SENSOR_ERROR_NONE;
}

/*******************************************************************************
 * SPI
 ******************************************************************************/
int32_t BMI323_SPI_Initialize(
    bmi323_spi_sensorhandle_t *pSensorHandle, ARM_DRIVER_SPI *pBus, uint8_t index, void *pSlaveSelect, uint8_t *pChipId)
{
    int32_t status;
    uint16_t chipId                   = 0;
    GENERIC_DRIVER_GPIO *pGPIODriver = &Driver_GPIO_KSDK;

    /*! Check the input parameters. */
    if ((pSensorHandle == NULL) || (pBus == NULL) || (pSlaveSelect == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    /*! Initialize the sensor handle. */
    pSensorHandle->pCommDrv                             = pBus;
    pSensorHandle->slaveParams.pReadPreprocessFN        = BMI323_SPI_ReadPreprocess;
    pSensorHandle->slaveParams.pWritePreprocessFN       = BMI323_SPI_WritePreprocess;
    pSensorHandle->slaveParams.pTargetSlavePinID        = pSlaveSelect;
    pSensorHandle->slaveParams.spiCmdLen                = BMI323_SPI_CMD_LEN;
    pSensorHandle->slaveParams.ssActiveValue            = BMI323_SS_ACTIVE_VALUE;

    pSensorHandle->deviceInfo.deviceInstance = index;
    pSensorHandle->deviceInfo.functionParam  = NULL;
    pSensorHandle->deviceInfo.idleFunction   = NULL;

    /* Initialize the Slave Select Pin. */
    pGPIODriver->pin_init(pSlaveSelect, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
    if (pSensorHandle->slaveParams.ssActiveValue == SPI_SS_ACTIVE_LOW)
    {
        pGPIODriver->set_pin(pSlaveSelect);
    }
    else
    {
        pGPIODriver->clr_pin(pSlaveSelect);
    }

    pSensorHandle->isInitialized = true;

    /*! Read and store the device's CHIP_ID. */
    status = BMI323_SPI_ReadReg(pSensorHandle, BMI323_CHIP_ID, &chipId);
    if (NULL != pChipId)
    {
        *pChipId = (uint8_t)(chipId & BMI323_CHIP_ID_MASK);
    }
    if (SENSOR_ERROR_NONE != status)
    {
        pSensorHandle->isInitialized = false;
        return SENSOR_ERROR_INIT;
    }
    if (BMI323_CHIP_ID_VALUE != (chipId & BMI323_CHIP_ID_MASK))
    {
        pSensorHandle->isInitialized = false;
        return SENSOR_ERROR_BAD_ADDRESS;
    }

    return SENSOR_ERROR_NONE;
}

void BMI323_SPI_SetIdleTask(bmi323_spi_sensorhandle_t *pSensorHandle,
                            registeridlefunction_t idleTask,
                            void *userParam)
{
    pSensorHandle->deviceInfo.functionParam = userParam;
    pSensorHandle->deviceInfo.idleFunction  = idleTask;
}

int32_t BMI323_SPI_ReadReg(bmi323_spi_sensorhandle_t *pSensorHandle, uint16_t offset, uint16_t *pValue)
{
    int32_t status;
    uint8_t buffer[BMI323_REG_SIZE] = {0};

    if ((pSensorHandle == NULL) || (pValue == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Register_SPI_Read() already skips spiCmdLen bytes, which covers the SPI dummy
     *  byte, so the returned buffer starts at the register payload. */
    status = Register_SPI_Read(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, &pSensorHandle->slaveParams,
                               (uint8_t)offset, sizeof(buffer), buffer);
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_READ;
    }

    *pValue = (uint16_t)(((uint16_t)buffer[1] << 8) | buffer[0]);

    return SENSOR_ERROR_NONE;
}

int32_t BMI323_SPI_WriteReg(bmi323_spi_sensorhandle_t *pSensorHandle, uint16_t offset, uint16_t value, uint16_t mask)
{
    int32_t status;
    uint16_t regValue;
    uint8_t payload[BMI323_REG_SIZE];

    if (pSensorHandle == NULL)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    if (mask)
    {
        /*! Read-modify-write: only the masked field is updated. */
        status = BMI323_SPI_ReadReg(pSensorHandle, offset, &regValue);
        if (SENSOR_ERROR_NONE != status)
        {
            return status;
        }
        regValue = (uint16_t)((regValue & (uint16_t)~mask) | (value & mask));
    }
    else
    {
        regValue = value;
    }

    /*! Registers are little endian: LSB goes out first. */
    payload[0] = (uint8_t)(regValue & 0xFF);
    payload[1] = (uint8_t)((regValue >> 8) & 0xFF);

    status = Register_SPI_BlockWrite(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, &pSensorHandle->slaveParams,
                                     (uint8_t)offset, payload, sizeof(payload));
    if (ARM_DRIVER_OK != status)
    {
        return SENSOR_ERROR_WRITE;
    }

    return SENSOR_ERROR_NONE;
}

int32_t BMI323_SPI_Configure(bmi323_spi_sensorhandle_t *pSensorHandle, const bmi323_regwritelist_t *pRegWriteList)
{
    int32_t status;
    const bmi323_regwritelist_t *pCmd = pRegWriteList;

    if ((pSensorHandle == NULL) || (pRegWriteList == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    while (pCmd->writeTo != 0xFFFF)
    {
        status = BMI323_SPI_WriteReg(pSensorHandle, pCmd->writeTo, pCmd->value, pCmd->mask);
        if (SENSOR_ERROR_NONE != status)
        {
            return status;
        }
        pCmd++;
    }

    return SENSOR_ERROR_NONE;
}

int32_t BMI323_SPI_ReadData(bmi323_spi_sensorhandle_t *pSensorHandle,
                            const bmi323_regreadlist_t *pReadList,
                            uint8_t *pBuffer)
{
    int32_t status;
    const bmi323_regreadlist_t *pCmd = pReadList;
    uint8_t *pOut                    = pBuffer;

    if ((pSensorHandle == NULL) || (pReadList == NULL) || (pBuffer == NULL))
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    for (; pCmd->numRegs != 0; pCmd++)
    {
        uint32_t payloadBytes = (uint32_t)pCmd->numRegs * BMI323_REG_SIZE;

        /*! The SPI read staging buffer also has to hold the command header. */
        if (((payloadBytes + BMI323_SPI_CMD_LEN) > BMI323_SPI_MAX_MSG_SIZE) || (payloadBytes > 255U))
        {
            return SENSOR_ERROR_INVALID_PARAM;
        }

        status = Register_SPI_Read(pSensorHandle->pCommDrv, &pSensorHandle->deviceInfo, &pSensorHandle->slaveParams,
                                   (uint8_t)pCmd->readFrom, (uint8_t)payloadBytes, pOut);
        if (ARM_DRIVER_OK != status)
        {
            return SENSOR_ERROR_READ;
        }
        pOut += payloadBytes;
    }

    return SENSOR_ERROR_NONE;
}

int32_t BMI323_SPI_Deinit(bmi323_spi_sensorhandle_t *pSensorHandle)
{
    int32_t status;

    if (pSensorHandle == NULL)
    {
        return SENSOR_ERROR_INVALID_PARAM;
    }

    if (pSensorHandle->isInitialized != true)
    {
        return SENSOR_ERROR_INIT;
    }

    /*! Trigger sensor device reset. */
    status = BMI323_SPI_WriteReg(pSensorHandle, BMI323_CMD, BMI323_CMD_SOFT_RESET, 0);

    pSensorHandle->isInitialized = false;

    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    /* Wait for the device to finish its internal boot sequence after the soft reset. */
    BOARD_DELAY_ms(BMI323_TBOOT_MAX);

    return SENSOR_ERROR_NONE;
}
