/*
 * Copyright 2023-2026 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"

#include "fsl_common.h"
#include "fsl_lpi2c.h"
#include "fsl_lpi2c_freertos.h"
#include "fsl_component_button.h"
#include "fsl_component_timer_manager.h"
#include "fsl_adapter_gpio.h"
#include "fsl_debug_console.h"

#include "framework_algorithm_hal.h"

#include "fxls8974_regdef.h"
#include "bmi323_regdef.h"

#define BOARD_CTIMER_CLK        kFRO_HF_to_CTIMER1
#define BOARD_CTIMER_SRCCLK_HZ  CLOCK_GetCTimerClkFreq(1U)
#define BORAD_CTIMER_DIV        kCLOCK_DivCTIMER1
#define BOARD_CTIMER_RST        kCTIMER1_RST_SHIFT_RSTn
#define BOARD_CTIMER_IRQ        CTIMER1_IRQn
#define BOARD_CTIMER_ISR_PRI    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1)
/* I2C for the sensor. Reserve LPI2C3 on Mikro BUS*/
#define BOARD_I2C_CLK               kFRO12M_to_LPI2C3
#define BOARD_I2C_DIV               kCLOCK_DivLPI2C3
#define BOARD_SENSOR_I2C            LPI2C3
#define	BOARD_SENSOR_I2C_SRCCLK_HZ	CLOCK_GetLpi2cClkFreq(3u)
#define LPI2C_IRQ_ID                LPI2C3_IRQn
#define LPCI2C_ISR_PRIORITY         (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1)

#define BOARD_SENSOR_FXLS8974_ADDR  FXLS8974_DEVICE_ADDRESS_SA0_0
#define BOARD_SENSOR_BMI323_ADDR    BMI323_DEVICE_ADDRESS_SDO_GND
#define BOARD_SENSOR_INTR_PORT      3U//set with Mikro BUS
#define BOARD_SENSOR_INTR_PIN       19U//set with Mikro BUS

/*
 * [Info] Supported framework HALs in the TimeSeries Studio.
 * ----------------------------------------------------------------------------------
 * |    name    |   Default   |          Description                                |
 *  --------------------------------------------------------------------------------
 * | shell_uart |   Enable    | Handle the user input from UART.                    |
 * | push_button|   Enable    | Handle the button_press actions.                    |
 * |  rgb_led   |   Enable    | Support the RGB LED indicator.                      |
 * |  FXLS8974  |   Enable    | Support the FXLS8974/BMI323 sensor on MIKROE board  |
 * |  BMI323                    via Mikro Bus.                                      |
 * |   dummy    |   Enable    | Handle the static payload(user data).               |
 * | time_series|   Enable    | Algorithm HAL to support the Time Series Libraries. |
 * ----------------------------------------------------------------------------------
 * ps: Uncomment the register function in register_hal_devices() to enable the specific HAL
 */
int32_t register_hal_devices()
{
    int err;

    extern int32_t register_input_hal_shell_uart();
    register_input_hal_shell_uart();

    extern int32_t register_input_hal_push_button();
    register_input_hal_push_button();

    extern int32_t register_output_hal_rgb_led();
    register_output_hal_rgb_led();

    extern int32_t register_sensor_hal_BMI323(uint32_t, uint32_t, uint32_t, hal_gpio_pin_config_t *);
    hal_gpio_pin_config_t bmi323_irq_pin_config =
    {
      .direction = kHAL_GpioDirectionIn,
      .level = 0,
      .port = BOARD_SENSOR_INTR_PORT,
      .pin = BOARD_SENSOR_INTR_PIN,
    };
    err = register_sensor_hal_BMI323(0, 0, 0, &bmi323_irq_pin_config);
    if (!err)
    {
        PRINTF("Sensor BMI323 is detected and registered !\r\n");
    }
    else
    {
        extern int32_t register_sensor_hal_FXLS8974(uint32_t, uint32_t, uint32_t, hal_gpio_pin_config_t *);
        hal_gpio_pin_config_t fxls8974_irq_pin_config =
        {
         .direction = kHAL_GpioDirectionIn,
         .level = 0,
         .port = BOARD_SENSOR_INTR_PORT,
         .pin = BOARD_SENSOR_INTR_PIN,
        };
        err = register_sensor_hal_FXLS8974(0, 0, 0, &fxls8974_irq_pin_config);
        if (!err)
        {
            PRINTF("Sensor FXLS8974 is detected and registered !\r\n");
        }
    }

    extern int32_t register_sensor_hal_dummy();
     register_sensor_hal_dummy();

    extern int32_t register_algorithm_hal_time_series();
    register_algorithm_hal_time_series();

    return 0;
}

static lpi2c_rtos_handle_t LPI2C_rtosHandle;

button_config_t push_button_configs[] =
{
    {	/* SW2 - WAKEUP */
        .gpio =
        {
            .direction = kHAL_GpioDirectionIn,
            .pinStateDefault = 1,
            .port = 1,
            .pin = BOARD_SW2_GPIO_PIN,
        },
    },
    {	/* SW3 - ISP */
        .gpio =
        {
            .direction = kHAL_GpioDirectionIn,
            .pinStateDefault = 1,
            .port = 0,
            .pin = BOARD_SW3_GPIO_PIN,
        },
    },
};

timer_config_t button_timer_config =
{
    .instance = 1U,
    .srcClock_Hz = 0,
    .clockSrcSelect = 0U,
};

void button_timer_init()
{
    CLOCK_SetClockDiv(BORAD_CTIMER_DIV, 1u);
    CLOCK_AttachClk(BOARD_CTIMER_CLK);
    /* CTIMER1 peripheral is released from reset */
    RESET_ReleasePeripheralReset(BOARD_CTIMER_RST);
    NVIC_SetPriority(BOARD_CTIMER_IRQ, BOARD_CTIMER_ISR_PRI);

	button_timer_config.srcClock_Hz = BOARD_CTIMER_SRCCLK_HZ;
}

void board_i2c_init()
{
    CLOCK_SetClockDiv(BOARD_I2C_DIV, 1u);
    CLOCK_AttachClk(BOARD_I2C_CLK);

    lpi2c_master_config_t LPI2C_config;
    LPI2C_MasterGetDefaultConfig(&LPI2C_config);
    LPI2C_config.baudRate_Hz = 400000U;
    LPI2C_RTOS_Init(&LPI2C_rtosHandle, BOARD_SENSOR_I2C, &LPI2C_config, BOARD_SENSOR_I2C_SRCCLK_HZ);
    NVIC_SetPriority(LPI2C_IRQ_ID, LPCI2C_ISR_PRIORITY);
}

void hardware_init()
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    button_timer_init();

    board_i2c_init();
}

int32_t board_i2c_write(uint8_t addr, uint8_t *buff, uint32_t size)
{
    status_t status;
    lpi2c_master_transfer_t xfer = { 0 };

    xfer.slaveAddress   = BOARD_SENSOR_FXLS8974_ADDR;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = addr;
    xfer.subaddressSize = 1;
    xfer.data           = buff;
    xfer.dataSize       = size;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    status = LPI2C_RTOS_Transfer(&LPI2C_rtosHandle, &xfer);
    if (status != kStatus_Success)
    {
        PRINTF("%s: I2C_RTOS_Transfer failed (%d)\r\n", __func__, status);
    }

    return status;
}

int32_t board_i2c_read(uint8_t addr, uint8_t *buff, uint32_t size)
{
    status_t status;
    lpi2c_master_transfer_t xfer = { 0 };

    xfer.slaveAddress   = BOARD_SENSOR_FXLS8974_ADDR;
    xfer.direction      = kLPI2C_Read;
    xfer.subaddress     = addr;
    xfer.subaddressSize = 1;
    xfer.data           = buff;
    xfer.dataSize       = size;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    status = LPI2C_RTOS_Transfer(&LPI2C_rtosHandle, &xfer);
    if (status != kStatus_Success)
    {
        if (status != kStatus_LPI2C_Nak)
        {
            PRINTF("%s: I2C_RTOS_Transfer failed (%d)\r\n", __func__, status);
        }
    }

    return status;
}

int32_t board_i2c_write16(uint8_t addr, uint16_t value)
{
    uint8_t data[2];

    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8) & 0xFFU);

    status_t status;
    lpi2c_master_transfer_t xfer = { 0 };

    xfer.slaveAddress   = BOARD_SENSOR_BMI323_ADDR;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = addr;
    xfer.subaddressSize = 1;
    xfer.data           = data;
    xfer.dataSize       = 2;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    status = LPI2C_RTOS_Transfer(&LPI2C_rtosHandle, &xfer);
    if (status != kStatus_Success)
    {
        PRINTF("%s: I2C_RTOS_Transfer failed (%d)\r\n", __func__, status);
    }

    return status;
}

int32_t board_i2c_read16(uint8_t addr, uint16_t *buff, uint32_t word_count)
{
    status_t status;
    lpi2c_master_transfer_t xfer = { 0 };
    uint8_t data[2 + 64 * sizeof(uint16_t)];

    if (buff == NULL || word_count == 0 || word_count > 64)
    {
        return kStatus_InvalidArgument;
    }

    xfer.slaveAddress   = BOARD_SENSOR_BMI323_ADDR;
    xfer.direction      = kLPI2C_Read;
    xfer.subaddress     = addr;
    xfer.subaddressSize = 1;
    xfer.data           = data;
    xfer.dataSize       = 2 + word_count * sizeof(uint16_t);
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    status = LPI2C_RTOS_Transfer(&LPI2C_rtosHandle, &xfer);
    if (status != kStatus_Success)
    {
        if (status != kStatus_LPI2C_Nak)
        {
            PRINTF("%s: I2C_RTOS_Transfer failed (%d)\r\n", __func__, status);
        }
        return status;
    }

    for (uint32_t i = 0; i < word_count; i++)
    {
        uint32_t offset = 2 + i * sizeof(uint16_t);
        buff[i] = (uint16_t)data[offset] | ((uint16_t)data[offset + 1] << 8);
    }

    return status;
}
