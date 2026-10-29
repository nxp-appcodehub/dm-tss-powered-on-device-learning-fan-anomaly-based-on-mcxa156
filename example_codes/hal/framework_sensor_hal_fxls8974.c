/*
 * Copyright 2023-2026 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdint.h>
#include <stdbool.h>

#include "fxls8974_regdef.h"

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

#define DEFAULT_TEMP_FOR_ZERO                 (25)

#define ACCEL_AXIS_NUM                        (3)
#define TEMP_AXIS_NUM                         (1)
#define FIFO_SIZE                             (16)
#define AXIS_NUMBER                           (ACCEL_AXIS_NUM + TEMP_AXIS_NUM)
#define BYTES_PER_AXIS                        (2)
#define BYTES_PER_AXIS_TEMP                   (1)
#define ACCEL_DATA_BYTE                       (ACCEL_AXIS_NUM * BYTES_PER_AXIS)
#define TEMP_DATA_BYTE                        (TEMP_AXIS_NUM * BYTES_PER_AXIS_TEMP)
#define AXIS_DATA_BYTE                        (ACCEL_DATA_BYTE + TEMP_DATA_BYTE)
#define ACCEL_BUF_SIZE                        (ACCEL_DATA_BYTE * FIFO_SIZE)
#define ACCEL_TEMP_BUF_SIZE                   (AXIS_DATA_BYTE * FIFO_SIZE)
#define GET_ARRAY_LENGTH(array)               (*(&(array) + 1) - (array))
#define ODR_CAP_1                             3200
#define ODR_CAP_2                             1600
#define ODR_CAP_3                             800
#define ODR_CAP_4                             400
#define ODR_CAP_5                             200

typedef struct
{
    uint8_t addr;  /*!< Address where the value is writes to.*/
    uint8_t value; /*!< value. Note that value should be shifted based on the bit position.*/
    uint8_t mask;  /*!< mask of the field to be set with given value.*/
} register_write_list_t;

typedef enum
{
    ACCEL_ONLY,
    ACCEL_TEMP,
    OVER_SUPPORTED_NO
} sensor_index_t;

typedef struct
{
    sensor_index_t index;  /*!< index of the active sensor.*/
    uint32_t axis_num;  /*axis number of the active sensor.*/
} active_sensor_t;

static active_sensor_t cur_sensor;
static bool is_sensor_setup = IS_DEFAULT_SETUP;
static uint32_t buff_wr = 0;
static uint32_t buff_rd = 0;
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
const static uint32_t odr_range[] = {25, 50, 100, 200, 400, 800, 1600, 3200 };
const static uint32_t fsr_range[] = { 2, 4, 8, 16 };
const static uint32_t sc_range[] = {1, 32, 64, 128, 256, 512, 1024 };// 1 for continuous data log
const static char *sensor_name[] = {"Accelerometer", "Temperature+Accel"};
const static target_sensor_id_t sensor_id[] = {TARGET_ACCEL_1, TARGET_ACCEL_TEMP_1};
const static uint32_t sensor_axis_num[] = {3, 4};
static uint8_t buff[FIFO_SIZE * AXIS_NUMBER * BYTES_PER_AXIS];
static hal_gpio_pin_config_t irq_pin_config;
static GPIO_HANDLE_DEFINE(gpio_handle);

extern registered_sensor_t reg_sensor;
extern uint32_t SystemCoreClock;
extern const char* firmware_version;

extern int32_t board_i2c_read(uint8_t addr, uint8_t *buff, uint32_t size);
extern int32_t board_i2c_write(uint8_t addr, uint8_t *buff, uint32_t size);

static int32_t FXLS8974_notify(void *param);

static const register_write_list_t active_mode_configurations[] =
{
    /*! Set FXLS8974 to put sensor in Active mode. */
    { FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ACTIVE_ACTIVE, FXLS8974_SENS_CONFIG1_ACTIVE_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

static const register_write_list_t standby_mode_configurations[] =
{
    /*! Reset FXLS8974 CTRL_REG1 Register "active" bit-field to put sensor in StandBy mode. */
    { FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_ACTIVE_STANDBY, FXLS8974_SENS_CONFIG1_ACTIVE_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

static const register_write_list_t endian_output_configurations[] =
{
    /*! Set FXLS8974 to big-endian output mode. */
    { FXLS8974_SENS_CONFIG2, FXLS8974_SENS_CONFIG2_LE_BE_BE, FXLS8974_SENS_CONFIG2_LE_BE_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

static const register_write_list_t software_reset_configurations[] =
{
    /*! Set FXLS8974 to software reset sensor. */
    { FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_RST_RST, FXLS8974_SENS_CONFIG1_RST_MASK },
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
    for (int i = 0; i < reg_sensor.num; i++)
    {
        if (reg_sensor.id[i] == id)
        {
            return true;
        }
    }
    return false;
}

static sensor_index_t get_sensor_index(target_sensor_id_t cur_id)
{
    sensor_index_t index;

    for (index = 0; index < OVER_SUPPORTED_NO; index++)
    {
        if (sensor_id[index] == cur_id)
        {
            break;
        }
    }
    return index;
}

static void start_sample(uint32_t odr, uint32_t fsr)
{
    uint8_t odr_register_value = 0;
    uint8_t fsr_register_value = 0;
    uint8_t int_pin_register_value;
    uint8_t int_en_register_value;
    uint8_t fifo_en_register_value;
    uint8_t fifo_wmrk_register_value;


    if (odr >= 3200)
    {
        odr_register_value = FXLS8974_SENS_CONFIG3_WAKE_ODR_3200HZ;
    }
    else if (odr >= 1600)
    {
        odr_register_value = FXLS8974_SENS_CONFIG3_WAKE_ODR_1600HZ;
    }
    else if (odr >= 800)
    {
        odr_register_value = FXLS8974_SENS_CONFIG3_WAKE_ODR_800HZ;
    }
    else if (odr >= 400)
    {
        odr_register_value = FXLS8974_SENS_CONFIG3_WAKE_ODR_400HZ;
    }
    else if (odr >= 200)
    {
        odr_register_value = FXLS8974_SENS_CONFIG3_WAKE_ODR_200HZ;
    }
    else if (odr >= 100)
    {
        odr_register_value = FXLS8974_SENS_CONFIG3_WAKE_ODR_100HZ;
    }
    else if (odr >= 50)
    {
        odr_register_value = FXLS8974_SENS_CONFIG3_WAKE_ODR_50HZ;
    }
    else
    {
        odr_register_value = FXLS8974_SENS_CONFIG3_WAKE_ODR_25HZ;
    }

    if (fsr >= 16)
    {
        fsr_register_value = FXLS8974_SENS_CONFIG1_FSR_16G;
        sensitivity_per_lsb = FXLS8974_SENSITIVITY_MG_PER_LSB_16G;
    }
    else if (fsr >= 8)
    {
        fsr_register_value = FXLS8974_SENS_CONFIG1_FSR_8G;
        sensitivity_per_lsb = FXLS8974_SENSITIVITY_MG_PER_LSB_8G;
    }
    else if (fsr >= 4)
    {
        fsr_register_value = FXLS8974_SENS_CONFIG1_FSR_4G;
        sensitivity_per_lsb = FXLS8974_SENSITIVITY_MG_PER_LSB_4G;
    }
    else
    {
        fsr_register_value = FXLS8974_SENS_CONFIG1_FSR_2G;
        sensitivity_per_lsb = FXLS8974_SENSITIVITY_MG_PER_LSB_2G;
    }

    if (cur_sensor.index == ACCEL_ONLY)
    {
        if (sample_count == 1)
        {
            fifo_size = 1;
        }
        else
        {
            fifo_size = FIFO_SIZE;
        }
    }
    else if (cur_sensor.index == ACCEL_TEMP)
    {
        fifo_size = 1;
    }

    if (fifo_size == 1)
    {
        buff_wr = 0;
        buff_rd = 0;

        int_pin_register_value = FXLS8974_INT_PIN_SEL_DRDY_INT2_DIS;
        int_en_register_value = FXLS8974_INT_EN_DRDY_EN_EN;
        fifo_en_register_value = FXLS8974_BUF_CONFIG1_BUF_MODE_DIS;
        fifo_wmrk_register_value = 0;
    }
    else
    {
        int_pin_register_value = FXLS8974_INT_PIN_SEL_BUF_INT2_DIS;
        int_en_register_value = FXLS8974_INT_EN_BUF_EN_EN;
        fifo_en_register_value = FXLS8974_BUF_CONFIG1_BUF_MODE_STOP_MODE;
        fifo_wmrk_register_value = fifo_size;
    }

    register_write_list_t odr_fsr_configurations[] =
    {
        /*! Configure ODR value. */
        { FXLS8974_SENS_CONFIG3, odr_register_value, FXLS8974_SENS_CONFIG3_WAKE_ODR_MASK },
        /*! Configure FSR value . */
        { FXLS8974_SENS_CONFIG1, fsr_register_value, FXLS8974_SENS_CONFIG1_FSR_MASK },
        /* END OF WRITE LIST */
        { .addr = 0xFF }
    };

    register_write_list_t fifo_configurations[] =
    {
        /*! Configure FXLS8974 FIFO size to set watermark. */
        { FXLS8974_BUF_CONFIG2, fifo_wmrk_register_value, FXLS8974_BUF_CONFIG2_BUF_WMRK_MASK },
        /*! Configure FXLS8974 to enable buffer stop mode. */
        { FXLS8974_BUF_CONFIG1, fifo_en_register_value, FXLS8974_BUF_CONFIG1_BUF_MODE_MASK },
        /* END OF WRITE LIST */
        { .addr = 0xFF }
    };

    register_write_list_t interrupt_configurations[] =
    {
        /*! Configure FXLS8974 to set INT polarity to Active Low and output mode selection as push-pull. */
        { FXLS8974_SENS_CONFIG4, FXLS8974_SENS_CONFIG4_INT_POL_ACT_LOW | FXLS8974_SENS_CONFIG4_INT_PP_OD_OPEN_DRAIN, FXLS8974_SENS_CONFIG4_INT_POL_MASK | FXLS8974_SENS_CONFIG4_INT_PP_OD_MASK },
        /*! Configure FXLS8974 to route buffer interrupt to INT1 PIN. */
        { FXLS8974_INT_PIN_SEL, int_pin_register_value, FXLS8974_INT_PIN_SEL_BUF_INT2_MASK | FXLS8974_INT_PIN_SEL_DRDY_INT2_MASK },
        /*! Configure FXLS8974 to enable buffer interrupt output. */
        { FXLS8974_INT_EN, int_en_register_value, FXLS8974_INT_EN_BUF_EN_MASK | FXLS8974_INT_EN_DRDY_EN_MASK },
        /* END OF WRITE LIST */
        { .addr = 0xFF }
    };

    sensor_burst_write(odr_fsr_configurations);
    sensor_burst_write(endian_output_configurations);
    sensor_burst_write(fifo_configurations);
    sensor_burst_write(interrupt_configurations);
    sensor_burst_write(active_mode_configurations);
}

static void stop_sample()
{
    sensor_burst_write(standby_mode_configurations);
}

static int32_t fetch_fifo_data(float *data)
{
    uint32_t data_byte;

    data_byte = fifo_size * cur_sensor.axis_num;

    switch (cur_sensor.index)
    {
        case ACCEL_ONLY:
        {
            if (fifo_size == 1)
            {
                if (buff_wr == ACCEL_BUF_SIZE)
                {
                    buff_wr = 0;
                }
                if (buff_rd == ACCEL_BUF_SIZE)
                {
                    buff_rd = 0;
                }

                board_i2c_read(FXLS8974_OUT_X_LSB, &buff[buff_wr], ACCEL_DATA_BYTE);

                if (is_sensor_data_requested)
                {
                    data[0] = convert_number_to_mg((int16_t)(buff[buff_rd + 0] << 8 | buff[buff_rd + 1]) >> 4);
                    data[1] = convert_number_to_mg((int16_t)(buff[buff_rd + 2] << 8 | buff[buff_rd + 3]) >> 4);
                    data[2] = convert_number_to_mg((int16_t)(buff[buff_rd + 4] << 8 | buff[buff_rd + 5]) >> 4);
                    buff_rd += ACCEL_DATA_BYTE;
                }
                else
                {
                    data_byte = 0;
                }
                buff_wr += ACCEL_DATA_BYTE;
            }
            else
            {
                board_i2c_read(FXLS8974_BUF_X_LSB, buff, ACCEL_BUF_SIZE);//read FIFO_SIZE of data

                for (uint32_t index = 0, position = 0; index < data_byte; index += ACCEL_AXIS_NUM, position += ACCEL_DATA_BYTE)
                {
                    data[index + 0] = convert_number_to_mg((int16_t)(buff[position + 0] << 8 | buff[position + 1]) >> 4);
                    data[index + 1] = convert_number_to_mg((int16_t)(buff[position + 2] << 8 | buff[position + 3]) >> 4);
                    data[index + 2] = convert_number_to_mg((int16_t)(buff[position + 4] << 8 | buff[position + 5]) >> 4);
                }
            }
            break;
        }
        case ACCEL_TEMP:
        {
            if (buff_wr == ACCEL_TEMP_BUF_SIZE)
            {
                buff_wr = 0;
            }
            if (buff_rd == ACCEL_TEMP_BUF_SIZE)
            {
                buff_rd = 0;
            }

            board_i2c_read(FXLS8974_TEMP_OUT, &buff[buff_wr], AXIS_DATA_BYTE);

            if (is_sensor_data_requested)
            {
                data[0] = (int8_t)(buff[buff_rd + 0]) + DEFAULT_TEMP_FOR_ZERO;
                data[1] = convert_number_to_mg((int16_t)(buff[buff_rd + 1] << 8 | buff[buff_rd + 2]) >> 4);
                data[2] = convert_number_to_mg((int16_t)(buff[buff_rd + 3] << 8 | buff[buff_rd + 4]) >> 4);
                data[3] = convert_number_to_mg((int16_t)(buff[buff_rd + 5] << 8 | buff[buff_rd + 6]) >> 4);
                buff_rd += AXIS_DATA_BYTE;
            }
            else
            {
                data_byte = 0;
            }
            buff_wr += AXIS_DATA_BYTE;

            break;
        }
        default:
            break;
    }

    return data_byte;
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
    { /* enter standby for segment data transmission*/
        is_sensor_activated = false;
        sensor_burst_write(standby_mode_configurations);
    }
    memcpy(&sensor_data[0], &sensor_buff[read_index], (sensor_data_size - read_index) * sizeof(float));
    if (read_index != 0)
    {
        memcpy(&sensor_data[sensor_data_size - read_index], &sensor_buff[0], read_index * sizeof(float));
    }

    reset_sensor_buff();
    sensor_payload_t *payload = pvPortMalloc(sizeof(sensor_payload_t));
    payload->sample_size = sample_count;
    payload->axis_size = cur_sensor.axis_num;
    payload->data = sensor_data;
    callback(SENSOR_EVENT_ID_DATA_READY, payload);
}

static void FXLS8974_task(void *param)
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
            sensor_data_size = sample_count * cur_sensor.axis_num;
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
            if (sample_count == 1)
            {/* keep reading for continuous data*/
                uint8_t val;
                board_i2c_read(FXLS8974_INT_STATUS, &val, 1);
                if (val & FXLS8974_INT_STATUS_SRC_DRDY_MASK)
                {
                    xEventGroupSetBits(event_group, EVENT_BIT_SENSOR_FIFO_DATA_READY);
                }
            }
            else
            {/* activate for segment data transmission with data request*/
                is_sensor_activated = true;
                sensor_burst_write(active_mode_configurations);
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
                uint32_t fifo_data_size = fifo_size * cur_sensor.axis_num;
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

static void FXLS8974_interrupt_callback(void *param)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xEventGroupSetBitsFromISR(event_group, EVENT_BIT_SENSOR_FIFO_DATA_READY, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static int32_t FXLS8974_init(sensor_hal_callback_t cb)
{
    HAL_GpioInit(&gpio_handle, &irq_pin_config);
    HAL_GpioSetTriggerMode(&gpio_handle, kHAL_GpioInterruptFallingEdge);
    HAL_GpioInstallCallback(&gpio_handle, FXLS8974_interrupt_callback, NULL);
    event_group = xEventGroupCreate();

    if (xTaskCreate(FXLS8974_task, "FXLS8974 Task", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        PRINTF("%s: xTaskCreate failed!\r\n", __func__);
        return -1;
    }

    sensor_burst_write(software_reset_configurations);

    callback = cb;

    /* Set the sensor-accellerometer active in default if Dummy hal is unregistered */
    if (!is_registered_sensor(TARGET_Dummy))
    {
        is_sensor_setup = true;
        cur_sensor.index = ACCEL_ONLY;
        cur_sensor.axis_num = sensor_axis_num[cur_sensor.index];
    }

    return 0;
}

static bool is_setup_param_valid(sensor_index_t sensor_index, uint32_t odr, uint32_t sc)
{
    uint32_t accel_cont_odr_cap, accel_seg_odr_cap, accel_temp_cont_odr_cap, accel_temp_seg_odr_cap;

#ifdef HS_USB_VCOM_DATA_LOG
    accel_cont_odr_cap = ODR_CAP_2;
    accel_seg_odr_cap = ODR_CAP_1;
    accel_temp_cont_odr_cap = ODR_CAP_2;
    accel_temp_seg_odr_cap = ODR_CAP_1;
#else
    accel_cont_odr_cap = ODR_CAP_4;
    accel_seg_odr_cap = ODR_CAP_1;
    accel_temp_cont_odr_cap = ODR_CAP_5;
    accel_temp_seg_odr_cap = ODR_CAP_2;
#endif
    if (sensor_index == ACCEL_ONLY)
    {
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
    }
    else
    {
        if (sc == 1)
        {
            if (odr > accel_temp_cont_odr_cap)
            {
                return false;
            }
        }
        else
        {
            if (odr > accel_temp_seg_odr_cap)
            {
                return false;
            }
        }
    }

    return true;
}

const static sensor_hal_t sensor_hal_FXLS8974 =
{
    .name = "FXLS8974",
    .init = FXLS8974_init,
    .start = NULL,
    .notify = FXLS8974_notify,
};

static int32_t FXLS8974_notify(void *param)
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
                if(!is_setup_param_valid(cur_sensor.index, event->value, sample_count))
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
                if(!is_setup_param_valid(cur_sensor.index, output_data_rate, event->value))
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
                    sensor_index_t index = get_sensor_index(event->value);
                    if (index < OVER_SUPPORTED_NO)
                    {
                        if(!is_setup_param_valid(index, output_data_rate, sample_count))
                        {
                            PRINTF("Invalid sample count (%d) and Output Data Rate (%d) for %s!\r\n", sample_count, output_data_rate, sensor_name[index]);
                            break;
                        }

                        is_sensor_setup = true;
                        cur_sensor.index = index;
                        cur_sensor.axis_num = sensor_axis_num[cur_sensor.index];

                        PRINTF("Sensor (%d: %s) setup successfully on %s!\r\n", event->value,
                                sensor_name[cur_sensor.index], sensor_hal_FXLS8974.name);
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
                PRINTF("Your Target: %s, Sensor: %s-%s, "
                        "odr: %u, fsr: %u, sc: %u\r\n", BOARD_NAME, sensor_name[cur_sensor.index],
                        sensor_hal_FXLS8974.name, output_data_rate, full_scale_range, sample_count);
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
                sensor_index_t i;
                for (i = 0; i < OVER_SUPPORTED_NO; i++)
                {
                    PRINTF("%d:%s ", sensor_id[i], sensor_name[i]);
                }
                int last_no = reg_sensor.num-1;
                if (get_sensor_index(reg_sensor.id[last_no]) < OVER_SUPPORTED_NO)
                {
                    /* Print the end flag by the last registered hal sensor */
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

int32_t register_sensor_hal_FXLS8974(uint32_t sc, uint32_t odr, uint32_t fsr, hal_gpio_pin_config_t *cfg)
{
    irq_pin_config = *cfg;
    uint8_t who_am_I_value=0;

    board_i2c_read(FXLS8974_WHO_AM_I, &who_am_I_value, sizeof(who_am_I_value));
    if(who_am_I_value != FXLS8974_WHOAMI_VALUE)
    {
        PRINTF("Warning: No sensor-FXLS8974 found!\r\n");
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

    framework_sensor_manager_register_hal(&sensor_hal_FXLS8974);

    register_sensor_id();

    return 0;
}
