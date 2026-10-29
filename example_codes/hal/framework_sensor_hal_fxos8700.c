/*
 * Copyright 2023, 2025-2026 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdint.h>
#include <stdbool.h>

#include "fxos8700_regdef.h"

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

#define FIFO_SIZE                             (16)
#define AXIS_NUMBER                           (6)
#define BYTES_PER_AXIS                        (2)
#define GET_ARRAY_LENGTH(array)               (*(&(array) + 1) - (array))
#define ODR_CAP_1                             800
#define ODR_CAP_2                             400
#define ODR_CAP_3                             200


typedef struct
{
    uint8_t addr;  /*!< Address where the value is writes to.*/
    uint8_t value; /*!< value. Note that value should be shifted based on the bit position.*/
    uint8_t mask;  /*!< mask of the field to be set with given value.*/
} register_write_list_t;

typedef enum
{
    ACCEL_ONLY,
    MAGNET_ONLY,
    ACCEL_MAGNET,
    OVER_SUPPORTED_NO
} sensor_index_t;

typedef struct
{
    sensor_index_t index;  /*!< index of the active sensor.*/
    uint32_t axis_num;  /*axis number of the active sensor.*/
} active_sensor_t;

static active_sensor_t cur_sensor;
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
const static uint32_t odr_range[] = { 50, 100, 200, 400, 800 };
const static uint32_t fsr_range[] = { 2, 4, 8 };
const static uint32_t sc_range[] = {1, 16, 32, 64, 128, 256, 512, 1024 };// 1 for continuous data log
const static char *sensor_name[] = {"Accelerometer", "Magnetometer", "Accel+Magnet"};
const static target_sensor_id_t sensor_id[] = {TARGET_ACCEL_1, TARGET_MAGNET_1, TARGET_ACCEL_MAGNET_1};
const static uint32_t sensor_axis_num[] = { 3, 3, 6};
static uint8_t buff[FIFO_SIZE * AXIS_NUMBER * BYTES_PER_AXIS];
static hal_gpio_pin_config_t irq_pin_config;
static GPIO_HANDLE_DEFINE(gpio_handle);

extern registered_sensor_t reg_sensor;
extern const char* firmware_version;

extern int32_t board_i2c_read(uint8_t addr, uint8_t *buff, uint32_t size);
extern int32_t board_i2c_write(uint8_t addr, uint8_t *buff, uint32_t size);

static int32_t fxos8700_notify(void *param);

static const register_write_list_t active_mode_configurations[] =
{
    /*! Set FXOS8700 CTRL_REG1 Register "active" bit-field to put sensor in Active mode. */
    { FXOS8700_CTRL_REG1, FXOS8700_CTRL_REG1_ACTIVE_ACTIVE_MODE, FXOS8700_CTRL_REG1_ACTIVE_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

static const register_write_list_t standby_mode_configurations[] =
{
    /*! Reset FXOS8700 CTRL_REG1 Register "active" bit-field to put sensor in StandBy mode. */
    { FXOS8700_CTRL_REG1, FXOS8700_CTRL_REG1_ACTIVE_STANDBY_MODE, FXOS8700_CTRL_REG1_ACTIVE_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

static const register_write_list_t software_reset_configurations[] =
{
    /*! Set FXOS8700 CTRL_REG2 Register "rst" bit-field to software reset sensor. */
    { FXOS8700_CTRL_REG2, FXOS8700_CTRL_REG2_RST_EN, FXOS8700_CTRL_REG2_RST_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF },
};

static const register_write_list_t hyb_autoinc_mode_configurations[] =
{
    /*! Configure FXOS8700 M_CTRL_REG2 Register to set auto-increasement address for data read in hybrid mode. */
    { FXOS8700_M_CTRL_REG2, FXOS8700_M_CTRL_REG2_M_AUTOINC_HYBRID_MODE, FXOS8700_M_CTRL_REG2_M_AUTOINC_MASK },
    /* END OF WRITE LIST */
    { .addr = 0xFF }
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
    uint8_t int_register_value;
    uint8_t fifo_en_register_value;
    uint8_t fifo_wmrk_register_value;
    uint8_t sensor_register_value;


    if (odr >= 800)
    {
        odr_register_value = FXOS8700_CTRL_REG1_DR_SINGLE_800_HZ;
    }
    else if (odr >= 400)
    {
        odr_register_value = FXOS8700_CTRL_REG1_DR_SINGLE_400_HZ;
    }
    else if (odr >= 200)
    {
        odr_register_value = FXOS8700_CTRL_REG1_DR_SINGLE_200_HZ;
    }
    else if (odr >= 100)
    {
        odr_register_value = FXOS8700_CTRL_REG1_DR_SINGLE_100_HZ;
    }
    else
    {
        odr_register_value = FXOS8700_CTRL_REG1_DR_SINGLE_50_HZ;
    }

    if (fsr >= 8)
    {
        fsr_register_value = FXOS8700_XYZ_DATA_CFG_FS_8G_0P976;
        sensitivity_per_lsb = 0.976;
    }
    else if (fsr >= 4)
    {
        fsr_register_value = FXOS8700_XYZ_DATA_CFG_FS_4G_0P488;
        sensitivity_per_lsb = 0.488;
    }
    else
    {
        fsr_register_value = FXOS8700_XYZ_DATA_CFG_FS_2G_0P244;
        sensitivity_per_lsb = 0.244;
    }

    if (cur_sensor.index != ACCEL_ONLY)
    {
        fifo_size = 1;
        int_register_value = FXOS8700_CTRL_REG4_INT_EN_DRDY_EN;
        fifo_en_register_value = FXOS8700_F_SETUP_F_MODE_FIFO_DISABLE;
        fifo_wmrk_register_value = 0;
        if (cur_sensor.index == MAGNET_ONLY)
        {
            sensor_register_value = FXOS8700_M_CTRL_REG1_M_HMS_MAG_ONLY;
        }
        else
        {
            sensor_register_value = FXOS8700_M_CTRL_REG1_M_HMS_HYBRID_MODE;
        }
    }
    else
    {
        if (sample_count == 1)
        {
            fifo_size = 1;
        }
        else
        {
            fifo_size = FIFO_SIZE;
        }
        int_register_value = FXOS8700_CTRL_REG4_INT_EN_FIFO_EN;
        fifo_en_register_value = FXOS8700_F_SETUP_F_MODE_FIFO_STOP_OVF;
        fifo_wmrk_register_value = fifo_size;
        sensor_register_value = FXOS8700_M_CTRL_REG1_M_HMS_ACCEL_ONLY;
    }

    register_write_list_t interrupt_configurations[] =
    {
        /*! Configure FXOS8700 CTRL_REG3 Register "ipol" & "pp_od" bit-fields to set INT polarity to ActiveHigh and output mode selection as push-pull. */
        { FXOS8700_CTRL_REG3, FXOS8700_CTRL_REG3_IPOL_ACTIVE_HIGH | FXOS8700_CTRL_REG3_PP_OD_PUSH_PULL, FXOS8700_CTRL_REG3_IPOL_MASK | FXOS8700_CTRL_REG3_PP_OD_MASK },
        /*! Configure FXOS8700 CTRL_REG4 Register "int_en_fifo" bit-field to enable FIFO interrupt. */
        { FXOS8700_CTRL_REG4, int_register_value, FXOS8700_CTRL_REG4_INT_EN_DRDY_MASK | FXOS8700_CTRL_REG4_INT_EN_FIFO_MASK },
        /* END OF WRITE LIST */
        { .addr = 0xFF }
    };

    register_write_list_t odr_fsr_configurations[] =
    {
        /*! Configure FXOS8700 CTRL_REG1 Register "dr[2:0]" bit-fields to set ODR value. */
        { FXOS8700_CTRL_REG1, odr_register_value, FXOS8700_CTRL_REG1_DR_MASK },
        /*! Configure FXOS8700 XYZ_DATA_CFG Register "fs[1:0]" bit-fileds to set FSR value . */
        { FXOS8700_XYZ_DATA_CFG, fsr_register_value, FXOS8700_XYZ_DATA_CFG_FS_MASK },
        /* END OF WRITE LIST */
        { .addr = 0xFF }
    };

    register_write_list_t fifo_configurations[] =
    {
        /*! Configure FXOS8700 F_SETUP Register "f_mode" and "f_wmrk" bit-fields to enable FIFO mode and set watermark. */
        { FXOS8700_F_SETUP,  fifo_en_register_value | fifo_wmrk_register_value, FXOS8700_F_SETUP_F_MODE_MASK | FXOS8700_F_SETUP_F_WMRK_MASK },
        { .addr = 0xFF }
    };

    register_write_list_t sensor_mosr_configurations[] =
    {
        /*! Configure FXOS8700 M_CTRL_REG1 Register to set sensor and OSR for magnet. */
        { FXOS8700_M_CTRL_REG1, FXOS8700_M_CTRL_REG1_M_RST_EN | FXOS8700_M_CTRL_REG1_M_OS_OSR7
                                | sensor_register_value,
                                FXOS8700_M_CTRL_REG1_M_RST_MASK| FXOS8700_M_CTRL_REG1_M_OS_MASK
                                | FXOS8700_M_CTRL_REG1_M_HMS_MASK },
        /* END OF WRITE LIST */
        { .addr = 0xFF }
    };

    sensor_burst_write(software_reset_configurations);
    vTaskDelay(pdMS_TO_TICKS(100));
    sensor_burst_write(fifo_configurations);
    sensor_burst_write(interrupt_configurations);
    sensor_burst_write(odr_fsr_configurations);
    sensor_burst_write(sensor_mosr_configurations);
    sensor_burst_write(hyb_autoinc_mode_configurations);
    sensor_burst_write(active_mode_configurations);
}

static void stop_sample()
{
    sensor_burst_write(standby_mode_configurations);
}

static int32_t fetch_fifo_data(float *data)
{
    uint8_t value;
    uint32_t rd_byte, data_byte;

    data_byte = fifo_size * cur_sensor.axis_num;
    rd_byte = data_byte * BYTES_PER_AXIS;

    switch (cur_sensor.index)
    {
        case ACCEL_ONLY:
        {
            board_i2c_read(FXOS8700_STATUS, &value, sizeof(value));
            if ((value & FXOS8700_F_STATUS_F_WMRK_FLAG_EVTDET) == 0)
            {
                /* It is reasonable that interrupt asserted again if the new data sample enters the FIFO before data read out */
                return -1;
            }

            board_i2c_read(FXOS8700_OUT_X_MSB, buff, rd_byte);

            for (uint32_t index = 0, position = 0; index < data_byte; index += cur_sensor.axis_num, position += 6)
            {
                data[index + 0] = convert_number_to_mg((int16_t)(buff[position + 0] << 8 | buff[position + 1]) >> 2);
                data[index + 1] = convert_number_to_mg((int16_t)(buff[position + 2] << 8 | buff[position + 3]) >> 2);
                data[index + 2] = convert_number_to_mg((int16_t)(buff[position + 4] << 8 | buff[position + 5]) >> 2);
            }

            break;
        }
        case MAGNET_ONLY:
        {
            board_i2c_read(FXOS8700_M_DR_STATUS, &value, sizeof(value));
            if ((value & FXOS8700_M_DR_STATUS_ZYXDR_MASK) == 0)
            {
                return -1;
            }

            board_i2c_read(FXOS8700_M_OUT_X_MSB, buff, rd_byte);

            data[0] = (int16_t)(buff[0] << 8 | buff[1]);
            data[1] = (int16_t)(buff[2] << 8 | buff[3]);
            data[2] = (int16_t)(buff[4] << 8 | buff[5]);

            break;
        }
        case ACCEL_MAGNET:
        {
            board_i2c_read(FXOS8700_M_DR_STATUS, &value, sizeof(value));
            if ((value & FXOS8700_M_DR_STATUS_ZYXDR_MASK) == 0)
            {
                return -1;
            }

            board_i2c_read(FXOS8700_OUT_X_MSB, buff, rd_byte);

            data[0] = convert_number_to_mg((int16_t)(buff[0] << 8 | buff[1]) >> 2);
            data[1] = convert_number_to_mg((int16_t)(buff[2] << 8 | buff[3]) >> 2);
            data[2] = convert_number_to_mg((int16_t)(buff[4] << 8 | buff[5]) >> 2);
            data[3] = (int16_t)(buff[6] << 8 | buff[7]);
            data[4] = (int16_t)(buff[8] << 8 | buff[9]);
            data[5] = (int16_t)(buff[10] << 8 | buff[11]);

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

static void fxos8700_task(void *param)
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
            if (sample_count > 1)
            {/* activate for segment data transmission with data request*/
                sensor_burst_write(active_mode_configurations);
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

static void fxos8700_interrupt_callback(void *param)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xEventGroupSetBitsFromISR(event_group, EVENT_BIT_SENSOR_FIFO_DATA_READY, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static int32_t fxos8700_init(sensor_hal_callback_t cb)
{
    HAL_GpioInit(&gpio_handle, &irq_pin_config);
    HAL_GpioSetTriggerMode(&gpio_handle, kHAL_GpioInterruptRisingEdge);
    HAL_GpioInstallCallback(&gpio_handle, fxos8700_interrupt_callback, NULL);
    event_group = xEventGroupCreate();

    if (xTaskCreate(fxos8700_task, "FXOS8700 Task", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        PRINTF("%s: xTaskCreate failed!\r\n", __func__);
        return -1;
    }

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
    uint32_t single_cont_odr_cap, single_seg_odr_cap, hybrid_cont_odr_cap, hybrid_seg_odr_cap;

#ifdef HS_USB_VCOM_DATA_LOG
    single_cont_odr_cap = ODR_CAP_1;
    single_seg_odr_cap = ODR_CAP_1;
    hybrid_cont_odr_cap = ODR_CAP_2;
    hybrid_seg_odr_cap = ODR_CAP_2;
#else
    single_cont_odr_cap = ODR_CAP_3;
    single_seg_odr_cap = ODR_CAP_1;
    hybrid_cont_odr_cap = ODR_CAP_2;
    hybrid_seg_odr_cap = ODR_CAP_1;
#endif
    if (sensor_index == ACCEL_ONLY || sensor_index == MAGNET_ONLY)
    {
        if (sc == 1)
        {
            if (odr > single_cont_odr_cap)
            {
                return false;
            }
        }
        else
        {
            if (odr > single_seg_odr_cap)
            {
                return false;
            }
        }
    }
    else
    {
        if (sc == 1)
        {
            if (odr > hybrid_cont_odr_cap)
            {
                return false;
            }
        }
        else
        {
            if (odr > hybrid_seg_odr_cap)
            {
                return false;
            }
        }
    }

    return true;
}

const static sensor_hal_t sensor_hal_fxos8700 =
{
    .name = "FXOS8700",
    .init = fxos8700_init,
    .start = NULL,
    .notify = fxos8700_notify,
};

static int32_t fxos8700_notify(void *param)
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
                int len = GET_ARRAY_LENGTH(odr_range);
                if (!is_setup_param_valid(cur_sensor.index, event->value, sample_count))
                {
                    PRINTF("Invalid Output Data Rate (%d) for sample count = %d !\r\n", event->value, sample_count);
                    break;
                }

                for (int i = 0; i < len; i++)
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
                if (cur_sensor.index != MAGNET_ONLY)
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
                else
                {
                    PRINTF("%s: Invalid Parameter for Magetometer!\r\n", __func__);
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_SETUP_SAMPLE_COUNT:
        {
            if (!is_sensor_activated && is_sensor_setup)
            {
                if (!is_setup_param_valid(cur_sensor.index, output_data_rate, event->value))
                {
                    PRINTF("Invalid sample count (%d) for %d of Output Data Rate!\r\n", event->value, output_data_rate);
                    break;
                }

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

        case SENSOR_EVENT_ID_SETUP_MULTI_SENSOR:
        {
            if (!is_sensor_activated)
            {
                if(is_registered_sensor(event->value))
                {
                    sensor_index_t index = get_sensor_index(event->value);
                    if (index < OVER_SUPPORTED_NO)
                    {
                        if (!is_setup_param_valid(index, output_data_rate, sample_count))
                        {
                            PRINTF("Invalid sample count (%d) and Output Data Rate (%d) for %s!\r\n", sample_count, output_data_rate, sensor_name[index]);
                            break;
                        }
                        is_sensor_setup = true;
                        cur_sensor.index = index;
                        cur_sensor.axis_num = sensor_axis_num[cur_sensor.index];

                        PRINTF("Sensor (%d: %s) setup successfully on %s!\r\n", event->value,
                                sensor_name[cur_sensor.index], sensor_hal_fxos8700.name);
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
                        PRINTF("Unsupported sensor on %s! List the supported sensors by 'get sensors' command\r\n", BOARD_NAME);
                    }
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_GET_SYSTEM_INFO:
        {
            if (!is_sensor_activated)
            {
                if (is_sensor_setup)
                {
                    if (cur_sensor.index == ACCEL_ONLY || cur_sensor.index == ACCEL_MAGNET)
                    {
                        PRINTF("Your Target: %s, Sensor: %s-%s, "
                                "odr: %u, fsr: %u, sc: %u\r\n", BOARD_NAME, sensor_name[cur_sensor.index],
                                sensor_hal_fxos8700.name, output_data_rate, full_scale_range,
                                sample_count);
                    }
                    else if (cur_sensor.index == MAGNET_ONLY)
                    {
                        PRINTF("Your Target: %s, Sensor: %s-%s, "
                                "odr: %u, sc: %u\r\n", BOARD_NAME, sensor_name[cur_sensor.index],
                                sensor_hal_fxos8700.name, output_data_rate, sample_count);
                    }
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_GET_CONFIG_RANGE:
        {
            int i;
            if (!is_sensor_activated && is_sensor_setup)
            {
                PRINTF("odr: ");
                int len = GET_ARRAY_LENGTH(odr_range);
                if (cur_sensor.index == ACCEL_MAGNET)
                {
                    len -= 1;
                }
                for (i = 0; i < len; i++)
                {
                    PRINTF("%u ", odr_range[i]);
                }
                if (cur_sensor.index != MAGNET_ONLY)
                {
                    PRINTF("\r\nfsr: ");
                    for (i = 0; i < GET_ARRAY_LENGTH(fsr_range); i++)
                    {
                        PRINTF("%u ", fsr_range[i]);
                    }
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

int32_t register_sensor_hal_fxos8700(uint32_t sc, uint32_t odr, uint32_t fsr, hal_gpio_pin_config_t *cfg)
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

    framework_sensor_manager_register_hal(&sensor_hal_fxos8700);

    register_sensor_id();

    return 0;
}
