/*
 * Copyright 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fxls8974cf_freemaster_demo.c
 * @brief The fxls8974cf_freemaster_demo.c file implements FreeMASTER demo using the ISSDK
 *        FXLS8974CF sensor driver example demonstration with interrupt mode.
 */

//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include "fsl_debug_console.h"
#if defined(FSL_FEATURE_SOC_PORT_COUNT) && (FSL_FEATURE_SOC_PORT_COUNT)
#include "fsl_port.h"
#endif
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "math.h"
#ifndef CPU_LPC55S16JBD100
#include "fsl_lpuart.h"
#else
#include "fsl_usart.h"
#endif
#include "fsl_common.h"
//-----------------------------------------------------------------------
// CMSIS Includes
//-----------------------------------------------------------------------
#include "Driver_I2C.h"

//-----------------------------------------------------------------------
// ISSDK Includes
//-----------------------------------------------------------------------
#include "issdk_hal.h"
#include "gpio_driver.h"
#include "fxls8974_drv.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include <stdio.h>
#include "TimeSeries.h"

#define FXLS8974_NUM_REGISTERS (FXLS8974_SELF_TEST_CONFIG2 + 1)
#define FF_A_FFMT_THS          (0x08)       /* FreeFall Threshold Value. */
#define A_FFMT_COUNT           (0x18)       /* Freefall/motion debounce count value. */
#define PL_COUNT               (0x15)       /* Pulse debounce count value. */
#define ASLP_COUNTER           (0x07)       /* Auto Sleep after ~5s. */
#define ACCEL_2G_SENS          (0.000976)   /* Sensitivity factor for 2G FS */
#define ACCEL_4G_SENS          (0.001953)   /* Sensitivity factor for 4G FS */
#define ACCEL_8G_SENS          (0.003906)   /* Sensitivity factor for 8G FS */
#define ACCEL_16G_SENS         (0.007813)   /* Sensitivity factor for 16G FS */
#define N                      (100U)       /* Number of samples used to measure offset/noise */
#define RAW_ACCEL_DATA_SIZE    (6U)         /* Accel Data Size */
#define MAX8BITSTORAGE         (255U)
#define FXLS8974_DATA_SIZE 6

#define  FIFO_READ	1

#define FIFO_WMRK              20U

const registerwritelist_t cFxls8974ConfigNormal[] = {
    /*! Set Full-scale range as 4G. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_4G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    /*! Clear SENS_CONFIG2 */
    //{FXLS8974_SENS_CONFIG2, 0x00, 0x00},
    /*! Disable Self-Test. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_DISABLED, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_MASK},
    /*! Set Wake Mode ODR Rate as 12.5Hz. */
    {FXLS8974_SENS_CONFIG3, FXLS8974_SENS_CONFIG3_WAKE_ODR_400HZ, FXLS8974_SENS_CONFIG3_WAKE_ODR_MASK},
/*enable fifo mode*/
#if FIFO_READ
	{FXLS8974_BUF_CONFIG1, FXLS8974_BUF_CONFIG1_BUF_TYPE_FIFO | FXLS8974_BUF_CONFIG1_BUF_MODE_STOP_MODE},
	{FXLS8974_BUF_CONFIG2, FIFO_WMRK},

    /*! Enable Interrupts for Data Ready Events. */
    {FXLS8974_INT_EN, FXLS8974_INT_EN_DRDY_EN_EN|FXLS8974_INT_EN_BUF_EN_EN, FXLS8974_INT_EN_DRDY_EN_MASK},
#else
	{FXLS8974_INT_EN, FXLS8974_INT_EN_DRDY_EN_EN, FXLS8974_INT_EN_DRDY_EN_MASK},
#endif
    /*! Enable Temperature sensor. */
    {FXLS8974_SENS_CONFIG2,FXLS8974_SENS_CONFIG2_ANIC_TEMP_EN,FXLS8974_SENS_CONFIG2_ANIC_TEMP_MASK},
    /*! Set Self-Test ODR to 100 Hz. */
    {0x38,0x05,0x00},
    {0x2F,0x38,0x00},
    {0x30,0xD8,0x00},
    {0x33,0xC0,0x00},
    {0x34,0xFF,0x00},
    {0x35,0x40,0x00},
    __END_WRITE_DATA__};

/*! @brief Register settings for Self-Test in X Axis (Positive polarity). */
const registerwritelist_t cFxls8974STXP[] = {
    /* Set Self Test Axis. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_16G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_EN_X, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_POL_POSITIVE,  FXLS8974_SENS_CONFIG1_ST_POL_MASK},
    __END_WRITE_DATA__};

/*! @brief Register settings for Self-Test in X Axis (Negative polarity). */
const registerwritelist_t cFxls8974STXN[] = {
    /* Set Self Test Axis. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_16G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_EN_X, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_POL_NEGATIVE, FXLS8974_SENS_CONFIG1_ST_POL_MASK},
    __END_WRITE_DATA__};

/*! @brief Register settings for Self-Test in Y Axis (Positive polarity). */
const registerwritelist_t cFxls8974STYP[] = {
    /* Set Self Test Axis. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_16G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_EN_Y, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_POL_POSITIVE,FXLS8974_SENS_CONFIG1_ST_POL_MASK},
    __END_WRITE_DATA__};

/*! @brief Register settings for Self-Test in Y Axis (Negative polarity). */
const registerwritelist_t cFxls8974STYN[] = {
    /* Set Self Test Axis. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_16G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_EN_Y, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_POL_NEGATIVE, FXLS8974_SENS_CONFIG1_ST_POL_MASK},
    __END_WRITE_DATA__};

/*! @brief Register settings for Self-Test in Z Axis (Positive polarity). */
const registerwritelist_t cFxls8974STZP[] = {
    /* Set Self Test Axis. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_16G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_EN_Z, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_POL_POSITIVE, FXLS8974_SENS_CONFIG1_ST_POL_MASK},
    __END_WRITE_DATA__};

/*! @brief Register settings for Self-Test in Z Axis (Negative polarity). */
const registerwritelist_t cFxls8974STZN[] = {
    /* Set Self Test Axis. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_16G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_EN_Z, FXLS8974_SENS_CONFIG1_ST_AXIS_SEL_MASK},
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ST_POL_NEGATIVE,  FXLS8974_SENS_CONFIG1_ST_POL_MASK},
    __END_WRITE_DATA__};

/*! @brief Address of Raw Accel Data in Normal Mode. */
const registerreadlist_t cFxls8974OutputNormal[] = {{.readFrom = FXLS8974_OUT_X_LSB, .numBytes = FXLS8974_DATA_SIZE},
    __END_READ_DATA__};

const registerreadlist_t cFXLS8974_whoami[] = {
        {.readFrom = FXLS8974_WHO_AM_I, .numBytes = 1}, __END_READ_DATA__};

/*! @brief Prepare the register read for INT Status Register. */
const registerreadlist_t cFXLS8974_int_src[] = {
        {.readFrom = FXLS8974_INT_STATUS, .numBytes = 1}, __END_READ_DATA__};

/*! @brief Prepare the register read for FullScale range Register. */
const registerreadlist_t cFXLS8974_fs_src[] = {
        {.readFrom = FXLS8974_SENS_CONFIG1, .numBytes = 1}, __END_READ_DATA__};

/*! @brief FXLS8974 register list to read all registers */
const registerreadlist_t FXLS8974_ALL_REG_READ[] = {{.readFrom = FXLS8974_INT_STATUS, .numBytes = FXLS8974_NUM_REGISTERS},
                                                      __END_READ_DATA__};

registerreadlist_t FXLS8974_READ[] = {{.readFrom = FXLS8974_BUF_X_LSB, .numBytes = FXLS8974_NUM_REGISTERS},
                                                      __END_READ_DATA__};

const registerreadlist_t FXLS8974_BUF_ST[] = {{.readFrom = FXLS8974_BUF_STATUS, .numBytes = 1},
                                                      __END_READ_DATA__};

/* INT GPIO Pin on Mikrobus Header */
#define FXLS8974_INT1_GPIO     GPIO3
#define FXLS8974_INT1_PIN      19U
#define FXLS8974_INT1_IRQ      GPIO3_IRQn
#define FXLS8974_INT1_ISR      GPIO3_IRQHandler

#define FXLS8974_INTF_SEL_PORT GPIO1
#define FXLS8974_INTF_SEL_PIN  21U
#define FXLS8974_BT_MODE_PORT  GPIO1
#define FXLS8974_BT_MODE_PIN   2U

//-----------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------
/*! @brief This structure defines the fxls8974 all registers metadata.*/
typedef struct
{
    uint8_t offset;
    uint8_t value;
    uint8_t trigger;
    uint8_t read_offset;
    uint8_t read_value;
    uint8_t read_trigger;
    uint8_t readall_value[FXLS8974_NUM_REGISTERS];
    uint8_t readall_size;
    uint8_t readall_trigger;
    uint8_t toggle;
    uint8_t trigger_accel_offnoise;
    uint8_t trigger_selftest;
    uint8_t fs_value;
    uint8_t mods_value;
    uint8_t odr_value;
    uint8_t reg_addr[FXLS8974_NUM_REGISTERS];
    uint8_t dataready_cntr;
    float accel[3];
    int16_t accelraw[3];
    uint8_t sdcd;
    int8_t temp;
    int16_t selftest[3];
    uint32_t timestamp;
} fxls8974_allregs_t;
//-----------------------------------------------------------------------
/*! @brief This structure defines the fxls8974 offset and noise calculation parameters. */
typedef struct
{
  	float offx;
  	float offy;
  	float offz;
  	float rmsx;
  	float rmsy;
  	float rmsz;
  	uint8_t complete_accel_offnoise;
} fxls8974_offset_noise_t;

/*! @brief This structure defines variables to compute self-test output change (STOC) and self-test offset (STOF). */
typedef struct
{
	int16_t x_stoc;
	int16_t y_stoc;
	int16_t z_stoc;
	int16_t x_stof;
	int16_t y_stof;
	int16_t z_stof;
  	uint8_t complete_selftest;
} fxls8974_selftest_t;

/*! @brief This structure defines the fxls8974 raw data buffer.*/
typedef struct
{
    int16_t xdata; /*!< The x accel data */
    int16_t ydata; /*!< The y accel data */
    int16_t zdata; /*!< The z accel data */
} sensor_data;


typedef union rawdata
{
	uint8_t byte_data[sizeof(sensor_data)];
	sensor_data dat;
}RAW_DATA;

/*! @brief This structure defines the fxls8974 host operation type.*/
typedef enum fxls8974_operation_type
{
	FXLS8974_REG_WRITE   = 1U,
	FXLS8974_REG_READ    = 2U,
    FXLS8974_ALLREG_READ = 3U,
	FXLS8974_ACCEL_CONFIG_END

} fxls8974_operation_type_t;

/*******************************************************************************
 * Globals
 ******************************************************************************/

fxls8974_acceldata_t rawData;
fxls8974_allregs_t      registers;
fxls8974_offset_noise_t offnoise_data;
fxls8974_selftest_t selftest;
uint8_t prev_toggle = 1;
volatile bool bFxls8974IntFlag = false;


uint8_t axis=0;
/* variables to store self-test values (Positive(P) + / Negative(N) -) */
int16_t XSTP[2]={0,0},YSTP[2]={0,0},ZSTP[2]={0,0},XSTN[2]={0,0},YSTN[2]={0,0},ZSTN[2]={0,0};

/* Define the init structure for the interrupt pin */
gpio_pin_config_t int1_config = {
    kGPIO_DigitalInput,
    0,
};

#define BUF_CNT (TSS_INPUT_DATA_LEN*TSS_INPUT_DATA_DIM)
SemaphoreHandle_t xSemaphore;
float sensor_buf[2][BUF_CNT];
uint8_t fifo_buffer[32*6];
uint8_t data_collect_flag = 0;
fxls8974_i2c_sensorhandle_t fxls8974Driver;

EventGroupHandle_t xEventGroup;
// 定义事件位
#define EVENT_BIT_1 (1 << 0)
/*******************************************************************************
 * Local functions
 ******************************************************************************/
 /*! @brief           Function to initialize target communication to FreeMASTER host.
 *  @details         This function initializes FreeMASTER UART communication.
 *  @param[in]       void.
 *  @return          void.
 */
static void init_freemaster_lpuart(void);
/*! @brief           ISR for FXLS8974 interrupt source event.
 *  @details         This function implements ISR for FXLS8974 INT source.
 *  @param[in]       void *.
 *  @return          void.
 */
void fxls8974_isr_callback(void *pUserData);
/*! @brief           Function to apply FXLS8974 register write operation.
 *  @details         This function apply FXLS8974 register write based on write trigger from host.
 *  @param[in]       fxls8974_i2c_sensorhandle_t fxls8974Driver, FXLS8974 sensor I2C handle.
 *  @param[in]       uint8_t offset, the address of the register to start writing from.
 *  @param[in]       uint8_t value, value to write on register offset.
 *  @return          returns the status of the operation.
 */
int32_t apply_register_write(fxls8974_i2c_sensorhandle_t fxls8974Driver, uint8_t offset, uint8_t value);
/*! @brief           Function to apply FXLS8974 register read operation.
 *  @details         This function apply FXLS8974 register read based on read trigger from host.
 *  @param[in]       fxls8974_i2c_sensorhandle_t fxls8974Driver, FXLS8974 sensor I2C handle.
 *  @param[in]       uint8_t offset, the address of the register to read from.
 *  @param[in/out]   uint8_t *value, pointer to output buffer.
 *  @return          returns the status of the operation.
 */
int32_t apply_register_read(fxls8974_i2c_sensorhandle_t fxls8974Driver, uint8_t offset, uint8_t *value);
/*! @brief           Function to apply FXLS8974 register read-all operation.
 *  @details         This function apply FXLS8974 all-registers read based on read-all trigger from host.
 *  @param[in]       fxls8974_i2c_sensorhandle_t fxls8974Driver, FXLS8974 sensor I2C handle.
 *  @return          returns the status of the operation.
 */
int32_t apply_register_readall(fxls8974_i2c_sensorhandle_t fxls8974Driver);
/*! @brief           Function to update dropdown selection.
 *  @details         This function updates the dropdown selection values in real-time based on read/write/read-all triggers.
 *  @param[in/out]   fxls8974_allregs_t *registers, pointer to FXLS8974 all-registers metadata.
 *  @param[in]       uint8_t caller, called from which operation type.
 *  @return          returns the status of the operation.
 */
int32_t update_dropdown_selection(fxls8974_allregs_t *registers, uint8_t caller);
/*! @brief           Function to initialize offset noise measurement.
 *  @details         This function initializes offset noise measurement metadata.
 *  @param[in/out]   fxls8974_offset_noise_t *offnoiseptr, pointer to FXLS8974 offset noise metadata.
 *  @return          void.
 */
void offset_noise_init(fxls8974_offset_noise_t *offnoiseptr);
/*! @brief           Function to measure accelerometer offset noise.
 *  @details         This function measures accelerometer offset noise.
 *  @param[in]       fxls8974_acceldata_t *rawData, pointer to FXLS8974 rawdata metadata.
 *  @param[in/out]   fxls8974_offset_noise_t *offnoiseptr, pointer to FXLS8974 offset noise metadata.
 *  @param[in]       float sens, FXLS8974 sensitivity based on FS configuration.
 *  @return          void.
 */
void accel_off_noise(fxls8974_acceldata_t* rawData, fxls8974_offset_noise_t *offnoiseptr, float sens);
/*! @brief           Function to initialize FXLS8974 self test metadata.
 *  @details         This function initializes FXLS8974 self test metadata.
 *  @param[in/out]   fxls8974_selftest_t *selftest, pointer to FXLS8974 selftest metadata.
 *  @return          void.
 */
void selftest_init(fxls8974_selftest_t *selftest);
/*! @brief           Function to perform FXLS8974 self test.
 *  @details         This function performs FXLS8974 self test.
 *  @param[in]       fxls8974_i2c_sensorhandle_t fxls8974Driver, FXLS8974 sensor I2C handle.
 *  @param[in/out]   fxls8974_selftest_t *selftest, pointer to FXLS8974 selftest metadata.
 *  @return          returns the status of the operation..
 */
int32_t perform_selftest(fxls8974_i2c_sensorhandle_t fxls8974Driver, fxls8974_selftest_t *selftest);


/*******************************************************************************
 * Code
 ******************************************************************************/
/*! -----------------------------------------------------------------------
 *  @brief       This is the Sensor Data Ready ISR implementation.
 *  @details     This function sets the flag which indicates SDCD motion wake up event occurred.
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
void FXLS8974_INT1_ISR(void)
{
    /* Clear external interrupt flag. */
    GPIO_GpioClearInterruptFlags(FXLS8974_INT1_GPIO, 1U << FXLS8974_INT1_PIN);
    /* Change state of button. */

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// 设置事件组的相应位
	xEventGroupSetBitsFromISR(xEventGroup, EVENT_BIT_1, &xHigherPriorityTaskWoken);

	// 如果有更高优先级任务被唤醒，进行任务切换
	if (xHigherPriorityTaskWoken == pdTRUE)
	{
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

    SDK_ISR_EXIT_BARRIER;
}

/*! -----------------------------------------------------------------------
 *  @brief       Initialize FXLS8974CF Interrupt Pin and Enable IRQ
 *  @details     This function initializes FXLS8974CF interrupt pin to wake up sensor
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
void init_fxls8974_wakeup_int(void)
{

    GPIO_SetPinInterruptConfig(FXLS8974_INT1_GPIO, FXLS8974_INT1_PIN, kGPIO_InterruptRisingEdge);

    IRQ_SetPriority(FXLS8974_INT1_IRQ, 2);
    EnableIRQ(FXLS8974_INT1_IRQ);
    GPIO_PinInit(FXLS8974_INT1_GPIO, FXLS8974_INT1_PIN, &int1_config);
}

/*! -----------------------------------------------------------------------
 *  @brief       Initialize FXLS8974CF INTF_SEL and BT_MODE Pins
 *  @details     This function initializes FXLS8974CF INTF_SEL and BT_MODE pins
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
void init_fxls8974cf_mode(void)
{
    /* Define the init structure for the PRT Stimulus pins */
    gpio_pin_config_t fxls8974cf_mode_config = {
        kGPIO_DigitalOutput,
        0,
    };



    GPIO_PinInit(FXLS8974_INTF_SEL_PORT, FXLS8974_INTF_SEL_PIN, &fxls8974cf_mode_config);
    GPIO_PinInit(FXLS8974_BT_MODE_PORT, FXLS8974_BT_MODE_PIN, &fxls8974cf_mode_config);

    /* Set INTF_SEL pin to Low */
    GPIO_PinWrite(FXLS8974_INTF_SEL_PORT, FXLS8974_INTF_SEL_PIN, 0);
    /* Set VBT_MODE pin to Low */
    GPIO_PinWrite(FXLS8974_BT_MODE_PORT, FXLS8974_BT_MODE_PIN, 0);
}
/* Create TSA table and add output variables. */
/*!
 * @brief Target Side Addressable (TSA) table created for this application.
 */

/*!
 * @brief Main function
 */

int sensor_init()
{
	int32_t status;
	uint8_t whoami = 0;
	uint8_t regdata;
	float sensitivity = ACCEL_4G_SENS;

	ARM_DRIVER_I2C *I2Cdrv = &I2C_S_DRIVER; // Now using the shield.h value!!!
	GENERIC_DRIVER_GPIO *pGpioDriver = &Driver_GPIO_KSDK;


	/*! Initialize FXLS8974CF INTF_SEL and BT_MODE pins */
	init_fxls8974cf_mode();
	/*! Initialize FXLS8974 wakeup pin used by MCX board */
	init_fxls8974_wakeup_int();

	/*! Initialize the I2C driver. */
	status = I2Cdrv->Initialize(I2C_S_SIGNAL_EVENT);
	if (ARM_DRIVER_OK != status)
	{
		return -1;
	}

	/*! Set the I2C Power mode. */
	status = I2Cdrv->PowerControl(ARM_POWER_FULL);
	if (ARM_DRIVER_OK != status)
	{
		return -1;
	}

	/*! Set the I2C bus speed. */
	status = I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
	if (ARM_DRIVER_OK != status)
	{
		return -1;
	}

	/*! Initialize FXLS8974 sensor driver. */
	status = FXLS8974_I2C_Initialize(&fxls8974Driver, &I2C_S_DRIVER, I2C_S_DEVICE_INDEX, FXLS8974_I2C_ADDR,
									 &whoami);
	if (SENSOR_ERROR_NONE != status)
	{
		return status;
	}

	/*!  Set the task to be executed while waiting for I2C transactions to complete. */
	FXLS8974_I2C_SetIdleTask(&fxls8974Driver, (registeridlefunction_t)SMC_SetPowerModeVlpr, SMC);

	/*! Configure the FXLS8974 sensor. */
	status = FXLS8974_I2C_Configure(&fxls8974Driver, cFxls8974ConfigNormal);
	if (SENSOR_ERROR_NONE != status)
	{
		return status;
	}

	/*! Initialize trigger flags */
	registers.toggle = 1;
	registers.trigger = 0;
	registers.read_trigger = 0;
	registers.read_value = 0;
	registers.readall_trigger = 0;
	registers.trigger_accel_offnoise=0;
	registers.trigger_selftest=0;
	registers.dataready_cntr = 0;
	registers.selftest[0]=0;
	registers.selftest[1]=0;
	registers.selftest[2]=0;
	registers.timestamp = 0;

	for(int i = 0; i < FXLS8974_NUM_REGISTERS; i++)
	{
		registers.readall_value[i] = 0;
	}

	return SENSOR_ERROR_NONE;
}

void sensor_task(void *param)
{
	int st;

	xEventGroup = xEventGroupCreate();

	if (xEventGroup == NULL) {
		PRINTF("Failed to create semaphore\n");
		return;
	}
	st = sensor_init();
	if (st != SENSOR_ERROR_NONE)
	{
		PRINTF("Sensor Init failed");
		return;
	}

	QueueHandle_t qh_NewSample = (QueueHandle_t)param;
	int  recv_cnt = 0;
	uint8_t buf_idx = 0;
	int status;
	int fifo_len = 0;
	EventBits_t xEventBits;

	while(1)
	{
		xEventBits = xEventGroupWaitBits(xEventGroup, EVENT_BIT_1,
		                                        pdTRUE, pdFALSE, portMAX_DELAY);

		if (xEventBits & (EVENT_BIT_1))
		{
#if FIFO_READ
			uint8_t buf_status;
			status = FXLS8974_I2C_ReadData(&fxls8974Driver, FXLS8974_BUF_ST, &buf_status);
			if (ARM_DRIVER_OK != status)
			{
				continue;
			}
			fifo_len = FXLS8974_BUF_STATUS_BUF_CNT_MASK&buf_status;

			if ((BUF_CNT - recv_cnt)/3 < fifo_len)
				fifo_len = (BUF_CNT - recv_cnt)/3;

			FXLS8974_READ->numBytes =  fifo_len * 6;
			status = FXLS8974_I2C_ReadData(&fxls8974Driver, FXLS8974_READ, fifo_buffer);
			if (ARM_DRIVER_OK != status)
			{
				continue;
			}
#else
			status = FXLS8974_I2C_ReadData(&fxls8974Driver, FXLS8974_ALL_REG_READ, registers.reg_addr);
			if (ARM_DRIVER_OK != status)
			{
				return -1;
			}

#endif
			registers.temp = registers.reg_addr[1]+25;
			registers.sdcd = (registers.reg_addr[0] & 0x10)>>4;

			int16_t x, y ,z;

			if (data_collect_flag == 0)
			{
	#if FIFO_READ
				for (int i=0;i<fifo_len;i++)
				{
					x = (((int16_t)fifo_buffer[i * FXLS8974_DATA_SIZE + 1] << 8) | fifo_buffer[i * FXLS8974_DATA_SIZE + 0]);
					y = (((int16_t)fifo_buffer[i * FXLS8974_DATA_SIZE + 3] << 8) | fifo_buffer[i * FXLS8974_DATA_SIZE + 2]);
					z = (((int16_t)fifo_buffer[i * FXLS8974_DATA_SIZE + 5] << 8) | fifo_buffer[i * FXLS8974_DATA_SIZE + 4]);
					sensor_buf[buf_idx][recv_cnt++] = x;
					sensor_buf[buf_idx][recv_cnt++] = y;
					sensor_buf[buf_idx][recv_cnt++] = z;

				}
	#else
					sensor_buf[buf_idx][recv_cnt++]=(float)(((int16_t)(((int16_t)registers.reg_addr[3] << 8) | registers.reg_addr[2])));
					sensor_buf[buf_idx][recv_cnt++]=(float)(((int16_t)(((int16_t)registers.reg_addr[5] << 8) | registers.reg_addr[4])));
					sensor_buf[buf_idx][recv_cnt++]=(float)(((int16_t)(((int16_t)registers.reg_addr[7] << 8) | registers.reg_addr[6])));
	#endif

				if (recv_cnt == BUF_CNT)
				{
					xQueueSend(qh_NewSample, (void*)sensor_buf[buf_idx], portMAX_DELAY);
					recv_cnt = 0;
					buf_idx ^= 1;
				}
			}
			else
			{

#if FIFO_READ
				for (int i=0;i<fifo_len;i++)
				{
					x = (((int16_t)fifo_buffer[i * FXLS8974_DATA_SIZE + 1] << 8) | fifo_buffer[i * FXLS8974_DATA_SIZE + 0]);
					y = (((int16_t)fifo_buffer[i * FXLS8974_DATA_SIZE + 3] << 8) | fifo_buffer[i * FXLS8974_DATA_SIZE + 2]);
					z = (((int16_t)fifo_buffer[i * FXLS8974_DATA_SIZE + 5] << 8) | fifo_buffer[i * FXLS8974_DATA_SIZE + 4]);

					PRINTF("%d %d %d\r\n",(int16_t)x,(int16_t)y,(int16_t)z);
				}
#else
				PRINTF("%d %d %d\r\n",(((int16_t)(((int16_t)registers.reg_addr[3] << 8) | registers.reg_addr[2]))), (((int16_t)(((int16_t)registers.reg_addr[5] << 8) | registers.reg_addr[4])))
						(((int16_t)(((int16_t)registers.reg_addr[7] << 8) | registers.reg_addr[6]))));
#endif


			}

		}
	}

}

void sensor_task_start_data_collection(uint8_t start_flag)
{
	data_collect_flag = start_flag;
}
