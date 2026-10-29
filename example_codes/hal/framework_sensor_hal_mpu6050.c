/*
 * Copyright 2023 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdint.h>
#include <stdbool.h>

#include "mpu6050_regdef.h"

#include "fsl_debug_console.h"

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "event_groups.h"

#include "framework_sensor_hal.h"
#include "framework_algorithm_hal.h"
#include "framework_sensor_manager.h"

#define EVENT_BIT_START_SENSOR_DATA_CAPTURE   (1 << 0)
#define EVENT_BIT_STOP_SENSOR_DATA_CAPTURE    (1 << 1)
#define EVENT_BIT_REQUEST_SENSOR_DATA         (1 << 2)
#define EVENT_BIT_SENSOR_FIFO_DATA_READY      (1 << 3)

#define DEFAULT_SAMPLE_COUNT                  (64)
#define DEFAULT_OUTPUT_DATA_RATE              (200)
#define DEFAULT_FULL_SCALE_RANGE              (2)

#define FIFO_SIZE                             (64)
#define AXIS_NUMBER                           (3)
#define BYTES_PER_AXIS                        (2)
#define GET_ARRAY_LENGTH(array)               (*(&(array) + 1) - (array))

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
const static uint32_t fsr_range[] = { 2, 4, 8, 16 };
const static uint32_t sc_range[] = { 64, 128, 256, 512, 1024, 2048 };
static uint8_t buff[FIFO_SIZE * AXIS_NUMBER * BYTES_PER_AXIS];
static TimerHandle_t timer_handle;

extern const char* firmware_version;
extern int32_t board_i2c_read(uint8_t addr, uint8_t *buff, uint32_t size);
extern int32_t board_i2c_write(uint8_t addr, uint8_t *buff, uint32_t size);

inline static float convert_number_to_mg(int16_t number)
{
    return number * sensitivity_per_lsb;
}

inline static uint8_t mpu_write_byte(uint8_t addr, uint8_t data)
{
    int32_t result = board_i2c_write(addr, &data, sizeof(data));
    if (result != 0)
    {
        PRINTF("%s: board_i2c_write failed to write 0x%X\r\n", __func__, addr);
    }
    return 0;
}

inline static uint8_t mpu_read_byte(uint8_t addr)
{
    uint8_t data = 0;
    int32_t result = board_i2c_read(addr, &data, sizeof(data));
    if (result != 0)
    {
        PRINTF("%s: board_i2c_write failed to read 0x%X\r\n", __func__, addr);
    }
    return data;
}

static void mpu_device_reset(bool sleep)
{
    uint8_t data;

    /* Reset all internal registers to their default values */
    mpu_write_byte(MPU_PWR_MGMT1_REG, PWR_MGMT1_DEVICE_RESET);

    vTaskDelay(pdMS_TO_TICKS(100));

    data = mpu_read_byte(MPU_PWR_MGMT1_REG);

    if (sleep)
    {
        data |= PWR_MGMT1_SLEEP_MODE;
    }
    else
    {
        data &= ~PWR_MGMT1_SLEEP_MODE;
    }

    mpu_write_byte(MPU_PWR_MGMT1_REG, data);
}

static uint16_t mpu_get_fifo_cnt()
{
    int32_t result;
    uint8_t data[2] = { 0, 0 };

    result = board_i2c_read(MPU_FIFO_CNTH_REG, data, sizeof(data));
    if (result != 0)
    {
        PRINTF("%s: board_i2c_read failed to read FIFO count\r\n", __func__);
        return 0;
    }

    return (data[0] << 8) | data[1];
}

static uint8_t mpu_set_lpf(uint16_t lpf)
{
    uint8_t data = 0;
    if (lpf >= 188)
    {
        data = 1;
    }
    else if (lpf >= 98)
    {
        data = 2;
    }
    else if (lpf >= 42)
    {
        data = 3;
    }
    else if (lpf >= 20)
    {
        data = 4;
    }
    else if(lpf >= 10)
    {
        data = 5;
    }
    else
    {
        data = 6;
    }
    return mpu_write_byte(MPU_CFG_REG, data);
}

static uint8_t mpu_set_rate(uint16_t rate)
{
    uint8_t data;
    if (rate > 1000)
    {
        rate = 1000;
    }
    if (rate < 4)
    {
        rate = 4;
    }
    data = 1000 / rate - 1;
    mpu_write_byte(MPU_SAMPLE_RATE_REG, data);
    mpu_set_lpf(rate / 2);
    return 0;
}

static void start_sample(uint32_t odr, uint32_t fsr)
{
    uint8_t data;
    uint8_t accel_config_value = 0;

    mpu_device_reset(false);

    if (fsr >= 16)
    {
        accel_config_value = 3 << ACCL_CONFIG_FSR_SHIFT;
        sensitivity_per_lsb = 1000.0 / 2048;
    }
    else if (fsr >= 8)
    {
        accel_config_value = 2 << ACCL_CONFIG_FSR_SHIFT;
        sensitivity_per_lsb = 1000.0 / 4096;
    }
    else if (fsr >= 4)
    {
        accel_config_value = 1 << ACCL_CONFIG_FSR_SHIFT;
        sensitivity_per_lsb = 1000.0 / 8192;
    }
    else
    {
        accel_config_value = 0 << ACCL_CONFIG_FSR_SHIFT;
        sensitivity_per_lsb = 1000.0 / 16384;
    }

    mpu_write_byte(MPU_ACCEL_CFG_REG, accel_config_value);

    mpu_set_rate(output_data_rate);

    /* Configure clock source to PLL with X axis gyro reference */
    data = mpu_read_byte(MPU_PWR_MGMT1_REG);
    data |= 1 << PWR_MGMT1_CLK_SEL_SHIFT;
    mpu_write_byte(MPU_PWR_MGMT1_REG, data);

    /* Configure FIFO related registers */
    mpu_write_byte(MPU_FIFO_EN_REG, ACCEL_FIFO_EN);
    mpu_write_byte(MPU_USER_CTRL_REG, USR_CTRL_FIFO_ENABLE);

    xTimerStart(timer_handle, 0);
}

static void stop_sample()
{
    mpu_device_reset(true);
    xTimerStop(timer_handle, 0);
}

static int32_t fetch_fifo_data(float *data)
{
    uint16_t cnt = mpu_get_fifo_cnt();
    if (cnt < sizeof(buff))
    {
        return -1;
    }

    int32_t result = board_i2c_read(MPU_FIFO_RW_REG, buff, sizeof(buff));
    if (result != 0)
    {
        PRINTF("%s: board_i2c_read failed to read FIFO data\r\n", __func__);
        return -1;
    }

    for (uint32_t index = 0, position = 0; index < FIFO_SIZE * AXIS_NUMBER; index += 3, position += 6)
    {
        data[index + 0] = convert_number_to_mg((int16_t)(buff[position + 0] << 8 | buff[position + 1]));
        data[index + 1] = convert_number_to_mg((int16_t)(buff[position + 2] << 8 | buff[position + 3]));
        data[index + 2] = convert_number_to_mg((int16_t)(buff[position + 4] << 8 | buff[position + 5]));
    }

    return FIFO_SIZE * AXIS_NUMBER;
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

static void mpu6050_timer_callback(TimerHandle_t timer)
{
    xEventGroupSetBits(event_group, EVENT_BIT_SENSOR_FIFO_DATA_READY);
}

static void mpu6050_task(void *param)
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
                uint32_t fifo_data_size = FIFO_SIZE * AXIS_NUMBER;
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

static int32_t mpu6050_init(sensor_hal_callback_t cb)
{
    timer_handle = xTimerCreate("MPU6050 Timer", (50 / portTICK_PERIOD_MS), true, NULL, mpu6050_timer_callback);

    event_group = xEventGroupCreate();

    if (xTaskCreate(mpu6050_task, "MPU6050 Task", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        PRINTF("%s: xTaskCreate failed!\r\n", __func__);
        return -1;
    }

    callback = cb;

    return 0;
}

static int32_t mpu6050_notify(void *param)
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
                PRINTF("Your Target: %s, Sensor: MPU6050, "
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

const static sensor_hal_t sensor_hal_mpu6050 =
{
    .name = "MPU6050",
    .init = mpu6050_init,
    .start = NULL,
    .notify = mpu6050_notify,
};

int32_t register_sensor_hal_mpu6050(uint32_t sc, uint32_t odr, uint32_t fsr)
{
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

    framework_sensor_manager_register_hal(&sensor_hal_mpu6050);

    return 0;
}
