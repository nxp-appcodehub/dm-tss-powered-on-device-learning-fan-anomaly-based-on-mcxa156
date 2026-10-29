/*
 * Copyright 2023-2026 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "fsl_debug_console.h"

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "event_groups.h"

#include "framework_sensor_hal.h"
#include "framework_algorithm_hal.h"
#include "framework_sensor_manager.h"
#include <framework_sensor_dummy_data.h>

#define IS_DEFAULT_SETUP                      (true)

#define EVENT_BIT_START_SENSOR_DATA_CAPTURE   (1 << 0)
#define EVENT_BIT_STOP_SENSOR_DATA_CAPTURE    (1 << 1)
#define EVENT_BIT_REQUEST_SENSOR_DATA         (1 << 2)
#define EVENT_BIT_SENSOR_FIFO_DATA_READY      (1 << 3)

#define FIFO_SIZE                             (16)

typedef enum
{
    Dummy,
    OVER_SUPPORTED_NO
} sensor_index_t;

typedef struct
{
    sensor_index_t index;  /*!< index of the active sensor.*/
    uint32_t axis_num;  /*axis number of the active sensor.*/
} active_sensor_t;

registered_sensor_t reg_sensor = {.num = 0};

static uint32_t axis_num;
static uint32_t data_size;
static uint32_t fifo_size;
static uint32_t dummy_sample_size;
static active_sensor_t cur_sensor;
static bool is_sensor_setup = IS_DEFAULT_SETUP;
static uint32_t sensor_data_size;
static uint32_t sensor_buff_capacity;
static uint32_t write_index;
static uint32_t read_index;
static sensor_hal_callback_t callback;
static EventGroupHandle_t event_group = NULL;
static bool is_sensor_data_requested = false;
static bool is_sensor_activated = false;
static uint32_t data_len_index;
static uint32_t data_size_index;
static TimerHandle_t timer_handle;
extern const char* firmware_version;

static char *sensor_name[OVER_SUPPORTED_NO] = {"Dummy"};
const static target_sensor_id_t sensor_id[] = {TARGET_Dummy};

static int32_t dummy_notify(void *param);

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

static void reset_sensor_buff()
{
    sensor_buff_capacity = 0;
    write_index = 0;
    read_index = 0;
    is_sensor_data_requested = false;
}

static void handle_data_ready()
{
    sensor_payload_t *payload = pvPortMalloc(sizeof(sensor_payload_t));

    /* Take dummy sensor data as input */
    payload->sample_size = data_size;
    payload->axis_size = axis_num;
    payload->data = (float *)&dummy_sensor_raw_data[data_len_index][data_size_index];

    data_size_index += sensor_data_size;
    if (data_size_index > DUMMY_SENSOR_DATA_SIZE - 1)
    {
        data_size_index = 0;
        data_len_index++;

        if (data_len_index == DUMMY_SENSOR_DATA_LEN)
        {
            data_len_index = 0;
        }
    }

    reset_sensor_buff();

    callback(SENSOR_EVENT_ID_DATA_READY, payload);

}

static void dummy_timer_callback(TimerHandle_t timer)
{
    xEventGroupSetBits(event_group, EVENT_BIT_SENSOR_FIFO_DATA_READY);
}

static void dummy_task(void *param)
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
            xTimerStart(timer_handle, 0);
            is_sensor_activated = true;
            reset_sensor_buff();

            data_len_index = 0;
            data_size_index = 0;
        }

        if ((event_bits & EVENT_BIT_STOP_SENSOR_DATA_CAPTURE) == EVENT_BIT_STOP_SENSOR_DATA_CAPTURE)
        {
            xTimerStop(timer_handle, 0);
            is_sensor_activated = false;
            reset_sensor_buff();
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
                uint32_t fifo_data_size = sensor_data_size;
                sensor_buff_capacity -= fifo_data_size;
                read_index += fifo_data_size;
                if (read_index == sensor_data_size)
                {
                    read_index = 0;
                }
            }

            int32_t result = dummy_sample_size;
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

static int32_t dummy_init(sensor_hal_callback_t cb)
{
    const tss_algo_attribute_t *algo_attr = tss_get_task_ops()->algo_attribute();
    axis_num = algo_attr->data_dim;
    if (algo_attr->recursive_predicted && algo_attr->recursive_length)
    {
        data_size = 1;
        fifo_size = 1;
    }
    else
    {
        data_size = algo_attr->data_len;
        fifo_size = FIFO_SIZE;
    }
    dummy_sample_size = fifo_size * axis_num;
    sensor_data_size = data_size * axis_num;

    timer_handle = xTimerCreate("Dummy Timer", ( 5 / portTICK_PERIOD_MS), true, NULL, dummy_timer_callback);

    event_group = xEventGroupCreate();

    if (xTaskCreate(dummy_task, "Dummy Task", configMINIMAL_STACK_SIZE + 200, NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        PRINTF("%s: xTaskCreate failed!\r\n", __func__);
        return -1;
    }

    callback = cb;

    return 0;
}

const static sensor_hal_t sensor_hal_dummy =
{
    .name = "Dummy",
    .init = dummy_init,
    .start = NULL,
    .notify = dummy_notify,
};

static int32_t dummy_notify(void *param)
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
            if (is_sensor_activated && is_sensor_setup)
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
            break;
        }

        case SENSOR_EVENT_ID_SETUP_FULL_SCALE_RANGE:
        {
            break;
        }

        case SENSOR_EVENT_ID_SETUP_SAMPLE_COUNT:
        {
            break;
        }

        case SENSOR_EVENT_ID_GET_SYSTEM_INFO:
        {
            if (!is_sensor_activated)
            {
                if (is_sensor_setup)
                {
                    PRINTF("Your Target: %s, Sensor: dummy.\r\n", BOARD_NAME);
                }
            }
            break;
        }

        case SENSOR_EVENT_ID_GET_CONFIG_RANGE:
        {
            if (!is_sensor_activated && is_sensor_setup)
            {
                PRINTF("Dummy sensor has no data range.\r\n");
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
                        is_sensor_setup = true;
                        cur_sensor.index = index;
                        PRINTF("Sensor (%d: %s) setup successfully!\r\n", event->value, sensor_name[cur_sensor.index]);
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

int32_t register_sensor_hal_dummy()
{
    framework_sensor_manager_register_hal(&sensor_hal_dummy);

    register_sensor_id();

    return 0;
}
