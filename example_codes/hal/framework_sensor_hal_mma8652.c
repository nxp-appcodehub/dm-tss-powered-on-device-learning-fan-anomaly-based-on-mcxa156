/*
 * Copyright 2023-2025 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdint.h>
#include <stdbool.h>

#include "mma8652_regdef.h"

#include "fsl_adapter_gpio.h"
#include "fsl_debug_console.h"

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "framework_sensor_hal.h"
#include "framework_algorithm_hal.h"
#include "framework_sensor_manager.h"

#define EVENT_BIT_START_SENSOR_DATA_CAPTURE   (1 << 0)
#define EVENT_BIT_STOP_SENSOR_DATA_CAPTURE    (1 << 1)
#define EVENT_BIT_REQUEST_SENSOR_DATA         (1 << 2)
#define EVENT_BIT_SENSOR_FIFO_DATA_READY      (1 << 3)

#define DEFAULT_SAMPLE_COUNT                  (128)
#define DEFAULT_OUTPUT_DATA_RATE              (400)
#define DEFAULT_FULL_SCALE_RANGE              (2)

#define FIFO_SIZE                             (16)
#define AXIS_NUMBER                           (3)
#define BYTES_PER_AXIS                        (2)
#define GET_ARRAY_LENGTH(array)               (*(&(array) + 1) - (array))

#define SENSITIVITY_MG_PER_LSB_2G			  (1000.0/1024)
#define SENSITIVITY_MG_PER_LSB_4G			  (1000.0/512)
#define SENSITIVITY_MG_PER_LSB_8G			  (1000.0/256)

typedef struct
{
    uint8_t addr;  /*!< Address where the value is writes to.*/
    uint8_t value; /*!< value. Note that value should be shifted based on the bit position.*/
    uint8_t mask;  /*!< mask of the field to be set with given value.*/
} register_write_list_t;

static uint32_t fifo_size;
static uint32_t sensor_data_size;
static uint32_t sensor_buff_capacity;
static uint32_t write_index;
static uint32_t read_index;
static float *sensor_data;
static float *sensor_buff;
static sensor_hal_callback_t callback;
static EventGroupHandle_t event_group = NULL;
static bool is_sensor_data_requested = false;
static bool is_sensor_activated = false;
static float sensitivity_per_lsb;
static uint32_t sample_count;
static uint32_t output_data_rate;
static uint32_t full_scale_range;
const static uint32_t odr_range[] = {6, 12, 50, 100, 200, 400, 800 };
const static uint32_t fsr_range[] = { 2, 4, 8 };
const static uint32_t sc_range[] = {1, 16, 32, 64, 128, 256, 512, 1024, 2048 };// 1 for continuous data log
static uint8_t buff[FIFO_SIZE * AXIS_NUMBER * BYTES_PER_AXIS];
static hal_gpio_pin_config_t irq_pin_config;
static GPIO_HANDLE_DEFINE(gpio_handle);

extern const char* firmware_version;

extern int32_t board_i2c_read(uint8_t addr, uint8_t *buff, uint32_t size);
extern int32_t board_i2c_write(uint8_t addr, uint8_t *buff, uint32_t size);

static const register_write_list_t interrupt_configurations[] =
{
    /*! Configure MMA8652 CTRL_REG3 Register "ipol" & "pp_od" bit-fields to set INT polarity to ActiveHigh and output mode selection as push-pull. */
    { MMA8652_CTRL_REG3, MMA8652_CTRL_REG3_IPOL_ACTIVE_HIGH | MMA8652_CTRL_REG3_PP_OD_PUSH_PULL, MMA8652_CTRL_REG3_IPOL_MASK | MMA8652_CTRL_REG3_PP_OD_MASK },
    /*! Configure MMA8652 CTRL_REG4 Register "int_en_fifo" bit-field to enable FIFO interrupt. */
    { MMA8652_CTRL_REG4, MMA8652_CTRL_REG4_INT_EN_FIFO_EN, MMA8652_CTRL_REG4_INT_EN_FIFO_MASK },
	/*! Configure MMA8652 CTRL_REG5 Register "int_cfg_fifo" bit-field to route FIFO interrupt to INT1. */
	{ MMA8652_CTRL_REG5, MMA8652_CTRL_REG5_INT_CFG_FIFO_INT1, MMA8652_CTRL_REG5_INT_CFG_FIFO_MASK },
	/* END OF WRITE LIST */
    { .addr = 0xFF }
};

static const register_write_list_t active_mode_configurations[] =
{
    /*! Set MMA8652 CTRL_REG1 Register "active" bit-field to put sensor in Active mode. */
    { MMA8652_CTRL_REG1, MMA8652_CTRL_REG1_ACTIVE_ACTIVE_MODE, MMA8652_CTRL_REG1_ACTIVE_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

static const register_write_list_t standby_mode_configurations[] =
{
    /*! Reset MMA8652 CTRL_REG1 Register "active" bit-field to put sensor in StandBy mode. */
    { MMA8652_CTRL_REG1, MMA8652_CTRL_REG1_ACTIVE_STANDBY_MODE, MMA8652_CTRL_REG1_ACTIVE_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

static const register_write_list_t software_reset_configurations[] =
{
    /*! Set MMA8652 CTRL_REG2 Register "rst" bit-field to software reset sensor. */
    { MMA8652_CTRL_REG2, MMA8652_CTRL_REG2_RST_EN, MMA8652_CTRL_REG2_RST_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

inline static float convert_number_to_mg(int16_t number)
{
    return number * sensitivity_per_lsb;
}

static int32_t sensor_burst_write(const register_write_list_t *list)
{
    uint8_t temp;

    for (const register_write_list_t *item = list; item->addr != 0xFF; item++)
    {
        if (item->mask != 0xFF)
        {
            board_i2c_read(item->addr, &temp, sizeof(temp));
        }
        temp = (temp & ~item->mask) | item->value;
        board_i2c_write(item->addr, &temp, sizeof(temp));
    }

    return 0;
}

static void start_sample(uint32_t odr, uint32_t fsr)
{
    uint8_t odr_register_value = 0;
    uint8_t fsr_register_value = 0;

    if (odr >= 800)
    {
        odr_register_value = MMA8652_CTRL_REG1_DR_SINGLE_800_HZ;
    }
    else if (odr >= 400)
    {
        odr_register_value = MMA8652_CTRL_REG1_DR_SINGLE_400_HZ;
    }
    else if (odr >= 200)
    {
        odr_register_value = MMA8652_CTRL_REG1_DR_SINGLE_200_HZ;
    }
    else if (odr >= 100)
    {
        odr_register_value = MMA8652_CTRL_REG1_DR_SINGLE_100_HZ;
    }
    else if (odr >= 50)
    {
        odr_register_value = MMA8652_CTRL_REG1_DR_SINGLE_50_HZ;
    }
    else if (odr > 12)
    {
    	odr_register_value = MMA8652_CTRL_REG1_DR_SINGLE_12P5_HZ;
    }
    else if (odr > 6)
    {
    	odr_register_value = MMA8652_CTRL_REG1_DR_SINGLE_6P25_HZ;
    }
    else
    {
    	odr_register_value = MMA8652_CTRL_REG1_DR_SINGLE_1P56_HZ;
    }

    if (fsr >= 8)
    {
        fsr_register_value = MMA8652_XYZ_DATA_CFG_FS_8G;
        sensitivity_per_lsb = MMA8652_SENSITIVITY_MG_PER_LSB_8G;
    }
    else if (fsr >= 4)
    {
        fsr_register_value = MMA8652_XYZ_DATA_CFG_FS_4G;
        sensitivity_per_lsb = MMA8652_SENSITIVITY_MG_PER_LSB_4G;
    }
    else
    {
        fsr_register_value = MMA8652_XYZ_DATA_CFG_FS_2G;
        sensitivity_per_lsb = MMA8652_SENSITIVITY_MG_PER_LSB_2G;
    }

    if (sample_count < FIFO_SIZE)
    {
        fifo_size = sample_count;
    }
    else
    {
        fifo_size = FIFO_SIZE;
    }

    register_write_list_t odr_fsr_configurations[] =
    {
        /*! Configure MMA8652 CTRL_REG1 Register "dr[2:0]" bit-fields to set ODR value. */
        { MMA8652_CTRL_REG1, odr_register_value, MMA8652_CTRL_REG1_DR_MASK },
        /*! Configure MMA8652 XYZ_DATA_CFG Register "fs[1:0]" bit-fileds to set FSR value . */
        { MMA8652_XYZ_DATA_CFG, fsr_register_value, MMA8652_XYZ_DATA_CFG_FS_MASK },
        /* END OF WRITE LIST */
        { .addr = 0xFF }
    };

    register_write_list_t fifo_configurations[] =
    {
        /*! Configure MMA8652 F_SETUP Register "f_mode" and "f_wmrk" bit-fields to enable FIFO mode and set watermark. */
        { MMA8652_F_SETUP, MMA8652_F_SETUP_F_MODE_FIFO_STOP_OVF | fifo_size, MMA8652_F_SETUP_F_MODE_MASK | MMA8652_F_SETUP_F_WMRK_MASK },
        { .addr = 0xFF }
    };

    sensor_burst_write(software_reset_configurations);
    vTaskDelay(pdMS_TO_TICKS(100));
    sensor_burst_write(fifo_configurations);
    sensor_burst_write(interrupt_configurations);
    sensor_burst_write(odr_fsr_configurations);
    sensor_burst_write(active_mode_configurations);
}

static void stop_sample()
{
    sensor_burst_write(standby_mode_configurations);
}

static int32_t fetch_fifo_data(float *data)
{
    uint8_t value;

    board_i2c_read(MMA8652_STATUS, &value, sizeof(value));
    if ((value & MMA8652_F_STATUS_F_WMRK_FLAG_EVTDET) == 0)
    {
        /* It is reasonable that interrupt asserted again if the new data sample enters the FIFO before data read out */
        return -1;
    }

    board_i2c_read(MMA8652_OUT_X_MSB, buff, sizeof(buff));

    for (uint32_t index = 0, position = 0; index < fifo_size * AXIS_NUMBER; index += 3, position += 6)
    {
        data[index + 0] = convert_number_to_mg((int16_t)(buff[position + 0] << 8 | buff[position + 1]) >> 4);
        data[index + 1] = convert_number_to_mg((int16_t)(buff[position + 2] << 8 | buff[position + 3]) >> 4);
        data[index + 2] = convert_number_to_mg((int16_t)(buff[position + 4] << 8 | buff[position + 5]) >> 4);
    }

    return fifo_size * AXIS_NUMBER;
}

static void reset_sensor_buff()
{
    sensor_buff_capacity = 0;
    write_index = 0;
    read_index = 0;
    is_sensor_data_requested = false;
}

static void handle_data_ready()
{
    memcpy(&sensor_data[0], &sensor_buff[read_index], (sensor_data_size - read_index) * sizeof(float));
    if (read_index != 0)
    {
        memcpy(&sensor_data[sensor_data_size - read_index], &sensor_buff[0], read_index * sizeof(float));
    }
    reset_sensor_buff();
    sensor_payload_t *payload = pvPortMalloc(sizeof(sensor_payload_t));
    payload->sample_size = sample_count;
    payload->axis_size = AXIS_NUMBER;
    payload->data = sensor_data;
    callback(SENSOR_EVENT_ID_DATA_READY, payload);
}

static void mma8652_task(void *param)
{
    const EventBits_t event_bits_to_wait = EVENT_BIT_START_SENSOR_DATA_CAPTURE |
                                           EVENT_BIT_STOP_SENSOR_DATA_CAPTURE |
                                           EVENT_BIT_REQUEST_SENSOR_DATA |
                                           EVENT_BIT_SENSOR_FIFO_DATA_READY;

    while (true)
    {
        EventBits_t event_bits = xEventGroupWaitBits(event_group, event_bits_to_wait, pdTRUE, pdFALSE, portMAX_DELAY);

        if ((event_bits & EVENT_BIT_START_SENSOR_DATA_CAPTURE) == EVENT_BIT_START_SENSOR_DATA_CAPTURE)
        {
            sensor_data_size = sample_count * AXIS_NUMBER;
            sensor_data = pvPortMalloc(sensor_data_size * sizeof(float));
            sensor_buff = pvPortMalloc(sensor_data_size * sizeof(float));
            if (sensor_data == NULL || sensor_buff == NULL)
            {
                PRINTF("%s: Memory allocation failed!", __func__);
                vPortFree(sensor_data);
                vPortFree(sensor_buff);
                sensor_data = NULL;
                sensor_buff = NULL;
                continue;
            }

            start_sample(output_data_rate, full_scale_range);
            is_sensor_activated = true;
            reset_sensor_buff();
        }

        if ((event_bits & EVENT_BIT_STOP_SENSOR_DATA_CAPTURE) == EVENT_BIT_STOP_SENSOR_DATA_CAPTURE)
        {
            stop_sample();
            is_sensor_activated = false;
            reset_sensor_buff();
            vPortFree(sensor_data);
            vPortFree(sensor_buff);
            sensor_data = NULL;
            sensor_buff = NULL;
        }

        if ((event_bits & EVENT_BIT_REQUEST_SENSOR_DATA) == EVENT_BIT_REQUEST_SENSOR_DATA)
        {
            is_sensor_data_requested = true;
            if (sensor_buff_capacity == sensor_data_size)
            {
                handle_data_ready();
            }
        }

        if ((event_bits & EVENT_BIT_SENSOR_FIFO_DATA_READY) == EVENT_BIT_SENSOR_FIFO_DATA_READY)
        {
            if (!is_sensor_activated)
            {
                continue;
            }

            if (sensor_buff_capacity == sensor_data_size)
            {
                uint32_t fifo_data_size = fifo_size * AXIS_NUMBER;
                sensor_buff_capacity -= fifo_data_size;
                read_index += fifo_data_size;
                if (read_index == sensor_data_size)
                {
                    read_index = 0;
                }
            }

            int32_t result = fetch_fifo_data(&sensor_buff[write_index]);
            if (result >= 0)
            {
                write_index += result;
                if (write_index == sensor_data_size)
                {
                    write_index = 0;
                }

                sensor_buff_capacity += result;
                if (sensor_buff_capacity == sensor_data_size && is_sensor_data_requested)
                {
                    handle_data_ready();
                }
            }
        }
    }
}

static void mma8652_interrupt_callback(void *param)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xEventGroupSetBitsFromISR(event_group, EVENT_BIT_SENSOR_FIFO_DATA_READY, NULL);
    }
}

static int32_t mma8652_init(sensor_hal_callback_t cb)
{
	uint8_t who_am_I_value=0;

    HAL_GpioInit(&gpio_handle, &irq_pin_config);
    HAL_GpioSetTriggerMode(&gpio_handle, kHAL_GpioInterruptRisingEdge);
    HAL_GpioInstallCallback(&gpio_handle, mma8652_interrupt_callback, NULL);
    event_group = xEventGroupCreate();

    board_i2c_read(MMA8652_WHO_AM_I, &who_am_I_value, sizeof(who_am_I_value));
    if(who_am_I_value != MMA8652_WHO_AM_I_PROD_VALUE)
    {
    	PRINTF("%s: i2c failed to read sensor ID!\r\n", __func__);
    	return -1;
    }

    if (xTaskCreate(mma8652_task, "mma8652 Task", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        PRINTF("%s: xTaskCreate failed!\r\n", __func__);
        return -1;
    }

    callback = cb;

    return 0;
}

static int32_t mma8652_notify(void *param)
{
    sensor_event_t *event = (sensor_event_t *)param;
    switch (event->id)
    {
        case SENSOR_EVENT_ID_START:
        {
            if (!is_sensor_activated)
            {
                xEventGroupSetBits(event_group, EVENT_BIT_START_SENSOR_DATA_CAPTURE);
            }
            break;
        }

        case SENSOR_EVENT_ID_STOP:
        {
            if (is_sensor_activated)
            {
                xEventGroupSetBits(event_group, EVENT_BIT_STOP_SENSOR_DATA_CAPTURE);
            }
            break;
        }

        case SENSOR_EVENT_ID_DATA_REQUEST:
        {
            xEventGroupSetBits(event_group, EVENT_BIT_REQUEST_SENSOR_DATA);
            break;
        }

        case SENSOR_EVENT_ID_SETUP_OUTPUT_DATA_RATE:
        {
            if (!is_sensor_activated)
            {
                for (int i = 0; i < GET_ARRAY_LENGTH(odr_range); i++)
                {
                    if (odr_range[i] == event->value)
                    {
                        output_data_rate = event->value;
                        PRINTF("Output Data Rate setup successfully (%d)!\r\n", event->value);
                        break;
                    }
                    else if (i == GET_ARRAY_LENGTH(odr_range) - 1)
                    {
                        PRINTF("%s: Invalid Output Data Rate (%d)!\r\n", __func__, event->value);
                    }
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_SETUP_FULL_SCALE_RANGE:
        {
            if (!is_sensor_activated)
            {
                for (int i = 0; i < GET_ARRAY_LENGTH(fsr_range); i++)
                {
                    if (fsr_range[i] == event->value)
                    {
                        full_scale_range = event->value;
                        PRINTF("Full scale range setup successfully (%d)!\r\n", event->value);
                        break;
                    }
                    else if (i == GET_ARRAY_LENGTH(fsr_range) - 1)
                    {
                        PRINTF("%s: Invalid Full scale range (%d)!\r\n", __func__, event->value);
                    }
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_SETUP_SAMPLE_COUNT:
        {
            if (!is_sensor_activated)
            {
                for (int i = 0; i < GET_ARRAY_LENGTH(sc_range); i++)
                {
                    if (sc_range[i] == event->value)
                    {
                        sample_count = event->value;
                        PRINTF("Sample count setup successfully (%d)!\r\n", event->value);
                        break;
                    }
                    else if (i == GET_ARRAY_LENGTH(sc_range) - 1)
                    {
                        PRINTF("%s: Invalid sample count (%d)!\r\n", __func__, event->value);
                    }
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_GET_SYSTEM_INFO:
        {
            if (!is_sensor_activated)
            {
                PRINTF("Your Target: %s, Sensor: Accelerometer-MMA8652, "
                        "odr: %u, fsr: %u, sc: %u\r\n", BOARD_NAME, output_data_rate, full_scale_range, sample_count);
            }
            break;
        }

        case SENSOR_EVENT_ID_GET_CONFIG_RANGE:
        {
            int i;
            if (!is_sensor_activated)
            {
                PRINTF("odr: ");
                for (i = 0; i < GET_ARRAY_LENGTH(odr_range); i++)
                {
                    PRINTF("%u ", odr_range[i]);
                }
                PRINTF("\r\nfsr: ");
                for (i = 0; i < GET_ARRAY_LENGTH(fsr_range); i++)
                {
                    PRINTF("%u ", fsr_range[i]);
                }
                PRINTF("\r\nsc: ");
                for (i = 0; i < GET_ARRAY_LENGTH(sc_range); i++)
                {
                    PRINTF("%u ", sc_range[i]);
                }
                PRINTF("\r\n");
            }
            break;
        }

        default:
            break;
    }

    return 0;
}

const static sensor_hal_t sensor_hal_mma8652 =
{
    .name = "mma8652",
    .init = mma8652_init,
    .start = NULL,
    .notify = mma8652_notify,
};

int32_t register_sensor_hal_mma8652(uint32_t sc, uint32_t odr, uint32_t fsr, hal_gpio_pin_config_t *cfg)
{
    irq_pin_config = *cfg;

    if (sc != 0)
    {
        sample_count = sc;
    }
    else
    {
        sample_count = DEFAULT_SAMPLE_COUNT;
    }

    if (odr != 0)
    {
        output_data_rate = odr;
    }
    else
    {
        output_data_rate = DEFAULT_OUTPUT_DATA_RATE;
    }

    if (fsr != 0)
    {
        full_scale_range = fsr;
    }
    else
    {
        full_scale_range = DEFAULT_FULL_SCALE_RANGE;
    }

    framework_sensor_manager_register_hal(&sensor_hal_mma8652);

    return 0;
}
