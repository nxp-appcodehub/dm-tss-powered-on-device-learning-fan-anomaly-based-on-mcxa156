/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file bmi323_drv.h
 * @brief The bmi323_drv.h file describes the BMI323 driver interface and structures.
 *
 * The interface intentionally mirrors fxls8974_drv.h so both sensors can sit behind the
 * same accel_sensor.h abstraction. The one structural difference is that BMI323
 * registers are 16-bit, so the register list types used here carry 16-bit values
 * (::bmi323_regwritelist_t / ::bmi323_regreadlist_t) instead of the ISSDK byte-oriented
 * ::registerwritelist_t.
 */

#ifndef BMI323_DRV_H_
#define BMI323_DRV_H_

/* Standard C Includes */
#include <stdint.h>

/* ISSDK Includes */
#include "bmi323.h"
#include "sensor_io_i2c.h"
#include "sensor_io_spi.h"
#include "register_io_i2c.h"
#include "register_io_spi.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*!
 * @brief This defines the sensor specific information for SPI.
 */
typedef struct
{
    registerDeviceInfo_t deviceInfo;      /*!< SPI device context. */
    ARM_DRIVER_SPI *pCommDrv;             /*!< Pointer to the spi driver. */
    bool isInitialized;                   /*!< Whether sensor is intialized or not.*/
    spiSlaveSpecificParams_t slaveParams; /*!< Slave Specific Params.*/
} bmi323_spi_sensorhandle_t;

/*!
 * @brief This defines the sensor specific information for I2C.
 */
typedef struct
{
    registerDeviceInfo_t deviceInfo; /*!< I2C device context. */
    ARM_DRIVER_I2C *pCommDrv;        /*!< Pointer to the i2c driver. */
    bool isInitialized;              /*!< whether sensor is intialized or not.*/
    uint16_t slaveAddress;           /*!< slave address.*/
} bmi323_i2c_sensorhandle_t;

/*! @brief This structure defines the bmi323 raw data buffer.*/
typedef struct
{
    uint32_t timestamp; /*!< The time, this sample was recorded. */
    int16_t accel[3];   /*!< The accel data */
    int16_t gyro[3];    /*!< The gyro data */
} bmi323_sensordata_t;

/*!
 * @brief This structure defines a 16-bit register write list entry.
 *
 * Terminate a list with ::__END_BMI323_WRITE_DATA__.
 */
typedef struct
{
    uint16_t writeTo; /*!< Register address the value is written to. */
    uint16_t value;   /*!< Value, already shifted to its bit position. */
    uint16_t mask;    /*!< Mask of the field to update; 0 overwrites the whole register. */
} bmi323_regwritelist_t;

/*!
 * @brief This structure defines a 16-bit register read list entry.
 *
 * Terminate a list with ::__END_BMI323_READ_DATA__.
 */
typedef struct
{
    uint16_t readFrom; /*!< Register address the value is read from. */
    uint8_t numRegs;   /*!< Number of consecutive 16-bit registers to read. */
} bmi323_regreadlist_t;

/*! @brief Terminator for a ::bmi323_regwritelist_t array. */
#define __END_BMI323_WRITE_DATA__      \
    {                                  \
        .writeTo = 0xFFFF, .value = 0  \
    }

/*! @brief Terminator for a ::bmi323_regreadlist_t array. */
#define __END_BMI323_READ_DATA__       \
    {                                  \
        .readFrom = 0xFFFF, .numRegs = 0 \
    }

/*! @def    BMI323_SPI_MAX_MSG_SIZE
 *  @brief  The MAX size of SPI message.
 *
 *  Large enough to hold a full FIFO drain (::ACCEL_SENSOR_FIFO_DEPTH frames) plus the
 *  SPI command header. */
#define BMI323_SPI_MAX_MSG_SIZE (208)

/*! @def    BMI323_SPI_CMD_LEN
 *  @brief  The size of the Sensor specific SPI Header (address byte + dummy byte). */
#define BMI323_SPI_CMD_LEN (1 + BMI323_SPI_DUMMY_BYTES)

/*! @def    BMI323_SS_ACTIVE_VALUE
 *  @brief  Is the Slave Select Pin Active Low or High. */
#define BMI323_SS_ACTIVE_VALUE SPI_SS_ACTIVE_LOW

/*******************************************************************************
 * APIs
 ******************************************************************************/
/*! @brief       The interface function to initialize the sensor over I2C.
 *  @details     This function initializes the sensor and sensor handle and reads back
 *               the device CHIP_ID.
 *  @param[in]   pSensorHandle  handle to the sensor.
 *  @param[in]   pBus           pointer to the CMSIS API compatible I2C bus object.
 *  @param[in]   index          the I2C device number.
 *  @param[in]   sAddress       slave address of the device on the bus.
 *  @param[out]  pChipId        receives the CHIP_ID value read from the device.
 *  @constraints This should be the first API to be called.
 *               Application has to ensure that previous instances of these APIs have exited before invocation.
 *  @reeentrant  No
 *  @return      ::BMI323_I2C_Initialize() returns the status.
 */
int32_t BMI323_I2C_Initialize(
    bmi323_i2c_sensorhandle_t *pSensorHandle, ARM_DRIVER_I2C *pBus, uint8_t index, uint16_t sAddress, uint8_t *pChipId);

/*! @brief      :  The interface function to set the I2C Idle Task.
 *  @param[in]  :  bmi323_i2c_sensorhandle_t *pSensorHandle, handle to the sensor handle.
 *  @param[in]  :  registeridlefunction_t idleTask, function pointer to the function to execute on I2C Idle Time.
 *  @param[in]  :  void *userParam, the pointer to the user idle task parameters.
 *  @return        void.
 *  @constraints   This can be called any number of times only after BMI323_I2C_Initialize().
 *                 Application has to ensure that previous instances of these APIs have exited before invocation.
 *  @reeentrant    No
 */
void BMI323_I2C_SetIdleTask(bmi323_i2c_sensorhandle_t *pSensorHandle,
                            registeridlefunction_t idleTask,
                            void *userParam);

/*! @brief       The interface function to configure the sensor.
 *  @details     This function applies every entry of the given 16-bit register write list.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   pRegWriteList pointer to the register list.
 *  @constraints This can be called any number of times only after BMI323_I2C_Initialize().
 *               Application has to ensure that previous instances of these APIs have exited before invocation.
 *  @reeentrant  No
 *  @return      ::BMI323_I2C_Configure() returns the status.
 */
int32_t BMI323_I2C_Configure(bmi323_i2c_sensorhandle_t *pSensorHandle, const bmi323_regwritelist_t *pRegWriteList);

/*! @brief       The interface function to read the sensor data.
 *  @details     This function reads the registers named by the read list and returns the
 *               payload with the device's leading dummy bytes already stripped. Each
 *               register contributes two bytes, LSB first.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   pReadList     pointer to the list of device registers to read.
 *  @param[out]  pBuffer       buffer which holds raw sensor data; must be large enough for
 *                             2 bytes per register across the whole list.
 *  @constraints This can be called any number of times only after BMI323_I2C_Initialize().
 *               Application has to ensure that previous instances of these APIs have exited before invocation.
 *  @reeentrant  No
 *  @return      ::BMI323_I2C_ReadData() returns the status.
 */
int32_t BMI323_I2C_ReadData(bmi323_i2c_sensorhandle_t *pSensorHandle,
                            const bmi323_regreadlist_t *pReadList,
                            uint8_t *pBuffer);

/*! @brief       The interface function to read a single 16-bit register.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   offset        the register address to read.
 *  @param[out]  pValue        receives the register value.
 *  @constraints This can be called any number of times only after BMI323_I2C_Initialize().
 *  @reeentrant  No
 *  @return      ::BMI323_I2C_ReadReg() returns the status.
 */
int32_t BMI323_I2C_ReadReg(bmi323_i2c_sensorhandle_t *pSensorHandle, uint16_t offset, uint16_t *pValue);

/*! @brief       The interface function to write a single 16-bit register.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   offset        the register address to write.
 *  @param[in]   value         the value to write, already shifted to its bit position.
 *  @param[in]   mask          mask of the field to update; 0 overwrites the whole register.
 *  @constraints This can be called any number of times only after BMI323_I2C_Initialize().
 *  @reeentrant  No
 *  @return      ::BMI323_I2C_WriteReg() returns the status.
 */
int32_t BMI323_I2C_WriteReg(bmi323_i2c_sensorhandle_t *pSensorHandle, uint16_t offset, uint16_t value, uint16_t mask);

/*! @brief       The interface function to De Initialize sensor.
 *  @details     This function triggers a soft reset and de-initializes the handle.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @constraints This can be called only after BMI323_I2C_Initialize().
 *               Application has to ensure that previous instances of these APIs have exited before invocation.
 *  @reeentrant  No
 *  @return      ::BMI323_I2C_DeInit() returns the status.
 */
int32_t BMI323_I2C_DeInit(bmi323_i2c_sensorhandle_t *pSensorHandle);

/*! @brief       The interface function to initialize the sensor over SPI.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   pBus          pointer to the CMSIS API compatible SPI bus object.
 *  @param[in]   index         the SPI device number.
 *  @param[in]   pSlaveSelect  slave select handle of the device on the bus.
 *  @param[out]  pChipId       receives the CHIP_ID value read from the device.
 *  @constraints This should be the first API to be called.
 *               Application has to ensure that previous instances of these APIs have exited before invocation.
 *  @reeentrant  No
 *  @return      ::BMI323_SPI_Initialize() returns the status.
 */
int32_t BMI323_SPI_Initialize(
    bmi323_spi_sensorhandle_t *pSensorHandle, ARM_DRIVER_SPI *pBus, uint8_t index, void *pSlaveSelect, uint8_t *pChipId);

/*! @brief      :  The interface function to set the SPI Idle Task.
 *  @param[in]  :  bmi323_spi_sensorhandle_t *pSensorHandle, handle to the sensor handle.
 *  @param[in]  :  registeridlefunction_t idleTask, function pointer to the function to execute on SPI Idle Time.
 *  @param[in]  :  void *userParam, the pointer to the user idle task parameters.
 *  @return        void.
 *  @constraints   This can be called any number of times only after BMI323_SPI_Initialize().
 *  @reeentrant    No
 */
void BMI323_SPI_SetIdleTask(bmi323_spi_sensorhandle_t *pSensorHandle,
                            registeridlefunction_t idleTask,
                            void *userParam);

/*! @brief       The interface function to configure the sensor over SPI.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   pRegWriteList pointer to the register list.
 *  @constraints This can be called any number of times only after BMI323_SPI_Initialize().
 *  @reeentrant  No
 *  @return      ::BMI323_SPI_Configure() returns the status.
 */
int32_t BMI323_SPI_Configure(bmi323_spi_sensorhandle_t *pSensorHandle, const bmi323_regwritelist_t *pRegWriteList);

/*! @brief       The interface function to read the sensor data over SPI.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   pReadList     pointer to the list of device registers to read.
 *  @param[out]  pBuffer       buffer which holds raw sensor data, dummy bytes stripped.
 *  @constraints This can be called any number of times only after BMI323_SPI_Initialize().
 *  @reeentrant  No
 *  @return      ::BMI323_SPI_ReadData() returns the status.
 */
int32_t BMI323_SPI_ReadData(bmi323_spi_sensorhandle_t *pSensorHandle,
                            const bmi323_regreadlist_t *pReadList,
                            uint8_t *pBuffer);

/*! @brief       The interface function to read a single 16-bit register over SPI.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   offset        the register address to read.
 *  @param[out]  pValue        receives the register value.
 *  @constraints This can be called any number of times only after BMI323_SPI_Initialize().
 *  @reeentrant  No
 *  @return      ::BMI323_SPI_ReadReg() returns the status.
 */
int32_t BMI323_SPI_ReadReg(bmi323_spi_sensorhandle_t *pSensorHandle, uint16_t offset, uint16_t *pValue);

/*! @brief       The interface function to write a single 16-bit register over SPI.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @param[in]   offset        the register address to write.
 *  @param[in]   value         the value to write, already shifted to its bit position.
 *  @param[in]   mask          mask of the field to update; 0 overwrites the whole register.
 *  @constraints This can be called any number of times only after BMI323_SPI_Initialize().
 *  @reeentrant  No
 *  @return      ::BMI323_SPI_WriteReg() returns the status.
 */
int32_t BMI323_SPI_WriteReg(bmi323_spi_sensorhandle_t *pSensorHandle, uint16_t offset, uint16_t value, uint16_t mask);

/*! @brief       The interface function to De Initialize sensor over SPI.
 *  @param[in]   pSensorHandle handle to the sensor.
 *  @constraints This can be called only after BMI323_SPI_Initialize().
 *  @reeentrant  No
 *  @return      ::BMI323_SPI_Deinit() returns the status.
 */
int32_t BMI323_SPI_Deinit(bmi323_spi_sensorhandle_t *pSensorHandle);

/*! @brief       The SPI Read Pre-Process function to generate Sensor specific SPI Message Header.
 *  @details     This function prepares the SPI Read Command Header with the register
 *               address and the R/W bit encoded as the sensor expects.
 *  @param[out]  pCmdOut  handle to the output buffer.
 *  @param[in]   offset   the address of the register to start reading from.
 *  @param[in]   size     number of bytes to read.
 *  @constraints None
 *  @reeentrant  No
 *  @return      :: None.
 */
void BMI323_SPI_ReadPreprocess(void *pCmdOut, uint32_t offset, uint32_t size);

/*! @brief       The SPI Write Pre-Process function to generate Sensor specific SPI Message Header.
 *  @details     This function prepares the SPI Write Command Header with the register
 *               address and the R/W bit encoded as the sensor expects.
 *  @param[out]  pCmdOut       handle to the output buffer.
 *  @param[in]   offset        the address of the register to start writing from.
 *  @param[in]   size          number of bytes to write.
 *  @param[in]   pWritebuffer  the bytes to write.
 *  @constraints None
 *  @reeentrant  No
 *  @return      :: None.
 */
void BMI323_SPI_WritePreprocess(void *pCmdOut, uint32_t offset, uint32_t size, void *pWritebuffer);

#endif // BMI323_DRV_H_
