/*
 * Copyright 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file sensor_task.c
 * @brief Accelerometer acquisition task feeding the Time Series Studio anomaly model.
 *
 * The task is sensor-agnostic: it only uses the ACCEL_SENSOR_xxx() API from
 * accel_sensor.h, so the physical sensor is selected at build time through
 * ::ACCEL_SENSOR_SELECT (FXLS8974CF or BMI323 / 6DOF IMU 20 Click).
 */

//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

//-----------------------------------------------------------------------
// Sensor abstraction
//-----------------------------------------------------------------------
#include "sensor_drv.h"
#include "accel_sensor.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include <stdio.h>
#include "TimeSeries.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief Model input window: 128 samples x 3 axes (X/Y/Z), as trained in Time Series
 *         Studio. The guard below stops a regenerated model from silently changing the
 *         window size out from under the acquisition code. */
#if (TSS_INPUT_DATA_LEN != 128) || (TSS_INPUT_DATA_DIM != 3)
#error "The anomaly model must use a 128-sample window with 3 channels (TSS_INPUT_DATA_LEN / TSS_INPUT_DATA_DIM)."
#endif

/*! @brief Number of floats in one model input window. */
#define BUF_CNT (TSS_INPUT_DATA_LEN * TSS_INPUT_DATA_DIM)

/*! @brief Event bit signalling that the sensor raised its watermark interrupt. */
#define EVENT_BIT_SENSOR_IRQ (1 << 0)

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Double buffer of model input windows. */
static float sensor_buf[2][BUF_CNT];

/*! @brief Scratch buffer for one FIFO burst. */
static accel_sensor_data_t sensor_samples[ACCEL_SENSOR_FIFO_DEPTH];

/*! @brief When non-zero, samples are printed to the console instead of being inferred. */
static uint8_t data_collect_flag = 0;

/*! @brief Signals sensor interrupts from the ISR to the task. */
static EventGroupHandle_t xEventGroup;

/*******************************************************************************
 * Code
 ******************************************************************************/
/*! -----------------------------------------------------------------------
 *  @brief       Sensor FIFO watermark ISR.
 *  @details     Wakes the acquisition task so it can drain the sensor FIFO.
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
void ACCEL_SENSOR_INT_ISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Clear external interrupt flag. */
    GPIO_GpioClearInterruptFlags(ACCEL_SENSOR_INT_GPIO, 1U << ACCEL_SENSOR_INT_PIN);

    xEventGroupSetBitsFromISR(xEventGroup, EVENT_BIT_SENSOR_IRQ, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    SDK_ISR_EXIT_BARRIER;
}

/*! -----------------------------------------------------------------------
 *  @brief       Bring up the accelerometer.
 *  @return      ::SENSOR_ERROR_NONE on success, an ::ESensorErrors code otherwise.
 *  -----------------------------------------------------------------------*/
static int32_t sensor_init(void)
{
    int32_t status;
    uint8_t whoami = 0;

    status = ACCEL_SENSOR_Init();
    if (SENSOR_ERROR_NONE != status)
    {
        return status;
    }

    if (SENSOR_ERROR_NONE == ACCEL_SENSOR_GetWhoAmI(&whoami))
    {
        PRINTF("%s detected, ID = 0x%02X\r\n", ACCEL_SENSOR_GetName(), whoami);
    }

    return SENSOR_ERROR_NONE;
}

/*! -----------------------------------------------------------------------
 *  @brief       Accelerometer acquisition task.
 *  @details     Waits for the sensor watermark interrupt, drains the FIFO and either
 *               assembles model input windows or dumps the raw samples to the console
 *               while a dataset collection is running.
 *  @param[in]   param  queue receiving the completed model input windows.
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
void sensor_task(void *param)
{
    QueueHandle_t qh_NewSample = (QueueHandle_t)param;
    uint32_t recv_cnt          = 0;
    uint8_t buf_idx            = 0;

    xEventGroup = xEventGroupCreate();
    if (xEventGroup == NULL)
    {
        PRINTF("Failed to create event group\r\n");
        return;
    }

    if (SENSOR_ERROR_NONE != sensor_init())
    {
        PRINTF("Sensor Init failed\r\n");
        return;
    }

    while (1)
    {
        EventBits_t xEventBits;
        uint32_t numSamples = 0;

        xEventBits = xEventGroupWaitBits(xEventGroup, EVENT_BIT_SENSOR_IRQ, pdTRUE, pdFALSE, portMAX_DELAY);
        if (0 == (xEventBits & EVENT_BIT_SENSOR_IRQ))
        {
            continue;
        }

        /*! Drain at most as many samples as still fit in the current window. */
        {
            uint32_t maxSamples = (BUF_CNT - recv_cnt) / TSS_INPUT_DATA_DIM;

            if (maxSamples > ACCEL_SENSOR_FIFO_DEPTH)
            {
                maxSamples = ACCEL_SENSOR_FIFO_DEPTH;
            }

            if (SENSOR_ERROR_NONE != ACCEL_SENSOR_ReadData(sensor_samples, maxSamples, &numSamples))
            {
                continue;
            }
        }

        if (data_collect_flag == 0)
        {
            for (uint32_t i = 0; i < numSamples; i++)
            {
                sensor_buf[buf_idx][recv_cnt++] = (float)sensor_samples[i].accel[0];
                sensor_buf[buf_idx][recv_cnt++] = (float)sensor_samples[i].accel[1];
                sensor_buf[buf_idx][recv_cnt++] = (float)sensor_samples[i].accel[2];
            }

            if (recv_cnt == BUF_CNT)
            {
                xQueueSend(qh_NewSample, (void *)sensor_buf[buf_idx], portMAX_DELAY);
                recv_cnt = 0;
                buf_idx ^= 1;
            }
        }
        else
        {
            for (uint32_t i = 0; i < numSamples; i++)
            {
                PRINTF("%d %d %d\r\n", sensor_samples[i].accel[0], sensor_samples[i].accel[1],
                       sensor_samples[i].accel[2]);
            }
        }
    }
}

/*! -----------------------------------------------------------------------
 *  @brief       Start or stop dumping raw samples to the console.
 *  @param[in]   start_flag  non-zero to dump samples, zero to run inference.
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
void sensor_task_start_data_collection(uint8_t start_flag)
{
    data_collect_flag = start_flag;
}
