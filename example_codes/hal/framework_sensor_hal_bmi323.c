/*
 * Copyright 2026 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bmi323_regdef.h"

#include "fsl_adapter_gpio.h"
#include "fsl_debug_console.h"

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "framework_sensor_hal.h"
#include "framework_algorithm_hal.h"
#include "framework_sensor_manager.h"

#define IS_DEFAULT_SETUP                      (false)

#define EVENT_BIT_START_SENSOR_DATA_CAPTURE   (1 << 0)
#define EVENT_BIT_STOP_SENSOR_DATA_CAPTURE    (1 << 1)
#define EVENT_BIT_REQUEST_SENSOR_DATA         (1 << 2)
#define EVENT_BIT_SENSOR_FIFO_DATA_READY      (1 << 3)

#define DEFAULT_SAMPLE_COUNT                  (128)
#define DEFAULT_OUTPUT_DATA_RATE              (400)
#define DEFAULT_FULL_SCALE_RANGE              (2)

#define ACCEL_AXIS_NUM                        (3)
#define FIFO_SIZE                             (16)
#define FIFO_FRAME_WORDS                      (ACCEL_AXIS_NUM)
#define FIFO_DATA_WORDS                       (FIFO_SIZE * FIFO_FRAME_WORDS)
#define BMI323_ROUTE_INTERRUPT_TO_INT2        (1)
#define GET_ARRAY_LENGTH(array)               (*(&(array) + 1) - (array))

#define ODR_CAP_1                             3200
#define ODR_CAP_2                             1600
#define ODR_CAP_3                             800
#define ODR_CAP_4                             400
#define ODR_CAP_5                             200

typedef struct
{
    uint8_t addr;   /*!< Address where the value is written to. */
    uint16_t value; /*!< Value shifted to the target bit position. */
    uint16_t mask;  /*!< Mask of the field to be updated. */
} register_write_list_t;

typedef enum
{
    ACCEL_ONLY,
    OVER_SUPPORTED_NO
} sensor_index_t;

static bool is_sensor_setup = IS_DEFAULT_SETUP;
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
const static uint32_t odr_range[] = { 12, 25, 50, 100, 200, 400, 800, 1600, 3200 };
const static uint32_t fsr_range[] = { 2, 4, 8, 16 };
const static uint32_t sc_range[] = { 1, 32, 64, 128, 256, 512, 1024 };
const static char *sensor_name[] = { "Accelerometer" };
const static target_sensor_id_t sensor_id[] = { TARGET_ACCEL_2 };
static uint16_t buff[FIFO_DATA_WORDS];
static hal_gpio_pin_config_t irq_pin_config;
static GPIO_HANDLE_DEFINE(gpio_handle);

extern registered_sensor_t reg_sensor;

extern int32_t board_i2c_read16(uint8_t addr, uint16_t *buff, uint32_t word_count);
extern int32_t board_i2c_write16(uint8_t addr, uint16_t value);

static int32_t BMI323_notify(void *param);

#if BMI323_ROUTE_INTERRUPT_TO_INT2
#define BMI323_ACC_DRDY_INT_MAP             BMI323_INT_MAP2_ACC_DRDY_INT2
#define BMI323_FIFO_WM_INT_MAP              BMI323_INT_MAP2_FIFO_WM_INT2
#define BMI323_DRDY_INT_STATUS_REG          BMI323_INT_STATUS_INT2
#else
#define BMI323_ACC_DRDY_INT_MAP             BMI323_INT_MAP2_ACC_DRDY_INT1
#define BMI323_FIFO_WM_INT_MAP              BMI323_INT_MAP2_FIFO_WM_INT1
#define BMI323_DRDY_INT_STATUS_REG          BMI323_INT_STATUS_INT1
#endif

static const register_write_list_t software_reset_configurations[] =
{
    { BMI323_CMD, BMI323_CMD_SOFT_RESET, 0xFFFFU },
    { .addr = 0xFF },
};

static const register_write_list_t stop_configurations[] =
{
    { BMI323_INT_MAP2, 0, BMI323_INT_MAP2_ACC_DRDY_MASK | BMI323_INT_MAP2_FIFO_WM_MASK },
    { BMI323_ACC_CONF, BMI323_ACC_CONF_MODE_DISABLE, BMI323_ACC_CONF_MODE_MASK },
    { BMI323_FIFO_CONF, 0, 0xFFFFU },
    { BMI323_FIFO_CTRL, BMI323_FIFO_CTRL_FLUSH, 0xFFFFU },
    { .addr = 0xFF },
};

inline static float convert_number_to_mg(int16_t number)
{
    return (number * sensitivity_per_lsb);
}

static int32_t sensor_burst_write(const register_write_list_t *list)
{
    uint16_t temp = 0;

    for (const register_write_list_t *item = list; item->addr != 0xFF; item++)
    {
        if (item->mask != 0xFFFFU)
        {
            if (board_i2c_read16(item->addr, &temp, 1) != 0)
            {
                return -1;
            }
        }

        temp = (temp & ~item->mask) | item->value;

        if (board_i2c_write16(item->addr, temp) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static void register_sensor_id()
{
    for (int i = 0; i < OVER_SUPPORTED_NO; i++)
    {
        reg_sensor.id[reg_sensor.num++] = sensor_id[i];
    }

    return;
}

static bool is_registered_sensor(target_sensor_id_t id)
{
    for (uint32_t i = 0; i < reg_sensor.num; i++)
    {
        if (reg_sensor.id[i] == id)
        {
            return true;
        }
    }
    return false;
}

static uint16_t get_odr_register_value(uint32_t odr)
{
    if (odr >= 6400)
    {
        return BMI323_ACC_CONF_ODR_6400HZ;
    }
    else if (odr >= 3200)
    {
        return BMI323_ACC_CONF_ODR_3200HZ;
    }
    else if (odr >= 1600)
    {
        return BMI323_ACC_CONF_ODR_1600HZ;
    }
    else if (odr >= 800)
    {
        return BMI323_ACC_CONF_ODR_800HZ;
    }
    else if (odr >= 400)
    {
        return BMI323_ACC_CONF_ODR_400HZ;
    }
    else if (odr >= 200)
    {
        return BMI323_ACC_CONF_ODR_200HZ;
    }
    else if (odr >= 100)
    {
        return BMI323_ACC_CONF_ODR_100HZ;
    }
    else if (odr >= 50)
    {
        return BMI323_ACC_CONF_ODR_50HZ;
    }
    else if (odr >= 25)
    {
        return BMI323_ACC_CONF_ODR_25HZ;
    }
    else
    {
        return BMI323_ACC_CONF_ODR_12P5HZ;
    }
}

static uint16_t get_fsr_register_value(uint32_t fsr)
{
    if (fsr >= 16)
    {
        sensitivity_per_lsb = BMI323_SENSITIVITY_MG_PER_LSB_16G;
        return BMI323_ACC_CONF_RANGE_16G;
    }
    else if (fsr >= 8)
    {
        sensitivity_per_lsb = BMI323_SENSITIVITY_MG_PER_LSB_8G;
        return BMI323_ACC_CONF_RANGE_8G;
    }
    else if (fsr >= 4)
    {
        sensitivity_per_lsb = BMI323_SENSITIVITY_MG_PER_LSB_4G;
        return BMI323_ACC_CONF_RANGE_4G;
    }
    else
    {
        sensitivity_per_lsb = BMI323_SENSITIVITY_MG_PER_LSB_2G;
        return BMI323_ACC_CONF_RANGE_2G;
    }
}

static void start_sample(uint32_t odr, uint32_t fsr)
{
    uint16_t int_map;
    uint16_t fifo_conf;
    uint16_t fifo_watermark;
    uint16_t acc_conf = get_odr_register_value(odr) |
                        get_fsr_register_value(fsr) |
                        BMI323_ACC_CONF_BW_ODR_BY_4 |
                        BMI323_ACC_CONF_AVG_4 |
                        BMI323_ACC_CONF_MODE_NORMAL;

    uint16_t io_int_ctrl = BMI323_IO_INT_CTRL_INT1_LVL_LOW |
                           BMI323_IO_INT_CTRL_INT1_OD_OPEN_DRAIN |
                           BMI323_IO_INT_CTRL_INT1_OUTPUT_EN |
                           BMI323_IO_INT_CTRL_INT2_LVL_LOW |
                           BMI323_IO_INT_CTRL_INT2_OD_OPEN_DRAIN |
                           BMI323_IO_INT_CTRL_INT2_OUTPUT_EN;

    fifo_size = (sample_count == 1) ? 1 : FIFO_SIZE;

    if (fifo_size == 1)
    {
        int_map = BMI323_ACC_DRDY_INT_MAP;
        fifo_conf = 0;
        fifo_watermark = 0;
    }
    else
    {
        int_map = BMI323_FIFO_WM_INT_MAP;
        fifo_conf = BMI323_FIFO_CONF_STOP_ON_FULL | BMI323_FIFO_CONF_ACC_EN;
        fifo_watermark = fifo_size * ACCEL_AXIS_NUM;
    }

    register_write_list_t sample_configurations[] =
    {
        { BMI323_ACC_CONF, BMI323_ACC_CONF_MODE_DISABLE, BMI323_ACC_CONF_MODE_MASK },
        { BMI323_IO_INT_CTRL, io_int_ctrl, 0xFFFFU },
        { BMI323_INT_CONF, BMI323_INT_CONF_LATCHED, 0xFFFFU },
        { BMI323_INT_MAP2, 0, BMI323_INT_MAP2_ACC_DRDY_MASK | BMI323_INT_MAP2_FIFO_WM_MASK },
        { BMI323_FIFO_CONF, fifo_conf, 0xFFFFU },
        { BMI323_FIFO_CTRL, BMI323_FIFO_CTRL_FLUSH, 0xFFFFU },
        { BMI323_FIFO_WATERMARK, fifo_watermark, 0xFFFFU },
        { BMI323_INT_MAP2, int_map, BMI323_INT_MAP2_ACC_DRDY_MASK | BMI323_INT_MAP2_FIFO_WM_MASK },
        { BMI323_ACC_CONF, acc_conf, 0xFFFFU },
        { .addr = 0xFF },
    };

    sensor_burst_write(sample_configurations);
}

static void stop_sample()
{
    sensor_burst_write(stop_configurations);
}

static int32_t fetch_fifo_data(float *data)
{
    uint16_t int_status;
    uint16_t fifo_level;
    uint32_t data_size = fifo_size * ACCEL_AXIS_NUM;

    if (fifo_size == 1)
    {
        if (board_i2c_read16(BMI323_ACC_DATA_X, buff, ACCEL_AXIS_NUM) != 0)
        {
            return -1;
        }

        if (board_i2c_read16(BMI323_DRDY_INT_STATUS_REG, &int_status, 1) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (board_i2c_read16(BMI323_FIFO_FILL_LEVEL, &fifo_level, 1) != 0)
        {
            return -1;
        }

        if ((fifo_level & 0x07FFU) < data_size)
        {
            return -1;
        }

        if (board_i2c_read16(BMI323_FIFO_DATA, buff, data_size) != 0)
        {
            return -1;
        }

        if (board_i2c_read16(BMI323_DRDY_INT_STATUS_REG, &int_status, 1) != 0)
        {
            return -1;
        }
    }

    for (uint32_t index = 0; index < data_size; index++)
    {
        data[index] = convert_number_to_mg((int16_t)buff[index]);
    }

    return data_size;
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
    if (sample_count > 1)
    {
        is_sensor_activated = false;
        stop_sample();
    }

    memcpy(&sensor_data[0], &sensor_buff[read_index], (sensor_data_size - read_index) * sizeof(float));
    if (read_index != 0)
    {
        memcpy(&sensor_data[sensor_data_size - read_index], &sensor_buff[0], read_index * sizeof(float));
    }

    reset_sensor_buff();
    sensor_payload_t *payload = pvPortMalloc(sizeof(sensor_payload_t));
    payload->sample_size = sample_count;
    payload->axis_size = ACCEL_AXIS_NUM;
    payload->data = sensor_data;
    callback(SENSOR_EVENT_ID_DATA_READY, payload);
}

static void BMI323_task(void *param)
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
            sensor_data_size = sample_count * ACCEL_AXIS_NUM;
            sensor_data = pvPortMalloc(sensor_data_size * sizeof(float));
            sensor_buff = pvPortMalloc(sensor_data_size * sizeof(float));
            if (sensor_data == NULL || sensor_buff == NULL)
            {
                PRINTF("%s: Memory allocation failed!\r\n", __func__);
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
            if (fifo_size == 1)
            {
                uint16_t status;
                if (board_i2c_read16(BMI323_STATUS, &status, 1) == 0 &&
                    (status & BMI323_STATUS_DRDY_ACC_MASK) != 0)
                {
                    xEventGroupSetBits(event_group, EVENT_BIT_SENSOR_FIFO_DATA_READY);
                }
            }
            else if (!is_sensor_activated)
            {
                start_sample(output_data_rate, full_scale_range);
                is_sensor_activated = true;
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
                uint32_t fifo_data_size = fifo_size * ACCEL_AXIS_NUM;
                sensor_buff_capacity -= fifo_data_size;
                read_index += fifo_data_size;
                if (read_index >= sensor_data_size)
                {
                    read_index = 0;
                }
            }

            int32_t result = fetch_fifo_data(&sensor_buff[write_index]);
            if (result >= 0)
            {
                write_index += result;
                if (write_index >= sensor_data_size)
                {
                    result -= (write_index - sensor_data_size);
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

static void BMI323_interrupt_callback(void *param)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xEventGroupSetBitsFromISR(event_group, EVENT_BIT_SENSOR_FIFO_DATA_READY, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static int32_t BMI323_init(sensor_hal_callback_t cb)
{
    uint16_t status = 0;

    HAL_GpioInit(&gpio_handle, &irq_pin_config);
    HAL_GpioSetTriggerMode(&gpio_handle, kHAL_GpioInterruptFallingEdge);
    HAL_GpioInstallCallback(&gpio_handle, BMI323_interrupt_callback, NULL);
    event_group = xEventGroupCreate();

    sensor_burst_write(software_reset_configurations);
    vTaskDelay(pdMS_TO_TICKS(5));
    board_i2c_read16(BMI323_STATUS, &status, 1);

    if (xTaskCreate(BMI323_task, "BMI323 Task", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        PRINTF("%s: xTaskCreate failed!\r\n", __func__);
        return -1;
    }

    callback = cb;

    if (!is_registered_sensor(TARGET_Dummy))
    {
        is_sensor_setup = true;
    }

    return 0;
}

static bool is_setup_param_valid(uint32_t odr, uint32_t sc)
{
    uint32_t accel_cont_odr_cap, accel_seg_odr_cap;

#ifdef HS_USB_VCOM_DATA_LOG
    accel_cont_odr_cap = ODR_CAP_2;
    accel_seg_odr_cap = ODR_CAP_1;
#else
    accel_cont_odr_cap = ODR_CAP_4;
    accel_seg_odr_cap = ODR_CAP_1;
#endif
    if (sc == 1)
    {
        if (odr > accel_cont_odr_cap)
        {
            return false;
        }
    }
    else
    {
        if (odr > accel_seg_odr_cap)
        {
            return false;
        }
    }

    return true;
}

const static sensor_hal_t sensor_hal_BMI323 =
{
    .name = "BMI323",
    .init = BMI323_init,
    .start = NULL,
    .notify = BMI323_notify,
};

static int32_t BMI323_notify(void *param)
{
    sensor_event_t *event = (sensor_event_t *)param;

    switch (event->id)
    {
        case SENSOR_EVENT_ID_START:
        {
            if (!is_sensor_activated && is_sensor_setup)
            {
                xEventGroupSetBits(event_group, EVENT_BIT_START_SENSOR_DATA_CAPTURE);
            }
            break;
        }

        case SENSOR_EVENT_ID_STOP:
        {
            if (is_sensor_setup)
            {
                xEventGroupSetBits(event_group, EVENT_BIT_STOP_SENSOR_DATA_CAPTURE);
            }
            break;
        }

        case SENSOR_EVENT_ID_DATA_REQUEST:
        {
            if (is_sensor_setup)
            {
                xEventGroupSetBits(event_group, EVENT_BIT_REQUEST_SENSOR_DATA);
            }
            break;
        }

        case SENSOR_EVENT_ID_SETUP_OUTPUT_DATA_RATE:
        {
            if (!is_sensor_activated && is_sensor_setup)
            {
                if (!is_setup_param_valid(event->value, sample_count))
                {
                    PRINTF("Invalid Output Data Rate (%d) for sample count = %d !\r\n", event->value, sample_count);
                    break;
                }

                int len = GET_ARRAY_LENGTH(odr_range);
                int i;
                for (i = 0; i < len; i++)
                {
                    if (odr_range[i] == event->value)
                    {
                        output_data_rate = event->value;
                        PRINTF("Output Data Rate setup successfully (%d)!\r\n", event->value);
                        break;
                    }
                    else if (i == len - 1)
                    {
                        PRINTF("%s: Invalid Output Data Rate (%d)!\r\n", __func__, event->value);
                    }
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_SETUP_FULL_SCALE_RANGE:
        {
            if (!is_sensor_activated && is_sensor_setup)
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
            if (!is_sensor_activated && is_sensor_setup)
            {
                if(!is_setup_param_valid(output_data_rate, event->value))
                {
                    PRINTF("Invalid sample count (%d) for %d of Output Data Rate!\r\n", event->value, output_data_rate);
                    break;
                }

                int i=0;
                int len = GET_ARRAY_LENGTH(sc_range);
                while (i < len)
                {
                    if (sc_range[i] == event->value)
                    {
                        sample_count = event->value;
                        PRINTF("Sample count setup successfully (%d)!\r\n", event->value);
                        break;
                    }
                    else if (i == len - 1)
                    {
                        PRINTF("%s: Invalid sample count (%d)!\r\n", __func__, event->value);
                    }
                    i++;
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_SETUP_MULTI_SENSOR:
        {
            if (!is_sensor_activated)
            {
                if(is_registered_sensor(event->value))
                {
                    if(event->value == sensor_id[0])
                    {
                        is_sensor_setup = true;

                        PRINTF("Sensor (%d: %s) setup successfully on %s!\r\n", event->value,
                                sensor_name[0], sensor_hal_BMI323.name);
                    }
                    else
                    {
                        is_sensor_setup = false;
                    }
                }
                else
                {
                    if (reg_sensor.id[0] == sensor_id[0])
                    {
                        /* Print the log by the hal sensor registered firstly*/
                        PRINTF("Invalid sensor on %s! List the supported sensors by 'get sensors' command\r\n", BOARD_NAME);
                    }
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_GET_SYSTEM_INFO:
        {
            if (!is_sensor_activated && is_sensor_setup)
            {
                PRINTF("Your Target: %s, Sensor: %s-%s, odr: %u, fsr: %u, sc: %u\r\n",
                       BOARD_NAME, sensor_name[0], sensor_hal_BMI323.name,
                       output_data_rate, full_scale_range, sample_count);
            }
            break;
        }

        case SENSOR_EVENT_ID_GET_CONFIG_RANGE:
        {
            int i;
            if (!is_sensor_activated && is_sensor_setup)
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

        case SENSOR_EVENT_ID_GET_MULTI_SENSOR:
        {
            if (!is_sensor_activated)
            {
                PRINTF("%d:%s ", sensor_id[0], sensor_name[0]);
                if (reg_sensor.num > 0 && reg_sensor.id[reg_sensor.num - 1] == sensor_id[0])
                {
                    PRINTF("\r\n");
                }
            }
            break;
        }

        default:
            break;
    }

    return 0;
}

int32_t register_sensor_hal_BMI323(uint32_t sc, uint32_t odr, uint32_t fsr, hal_gpio_pin_config_t *cfg)
{
    irq_pin_config = *cfg;
    uint16_t chip_id = 0;

    if (board_i2c_read16(BMI323_CHIP_ID, &chip_id, 1) != 0 ||
        (chip_id & 0x00FFU) != BMI323_CHIP_ID_VALUE)
    {
        PRINTF("Warning: No sensor-BMI323 found!\r\n");
        return -1;
    }

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

    framework_sensor_manager_register_hal(&sensor_hal_BMI323);
    register_sensor_id();

    return 0;
}
