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
#include <stdio.h>
#include <math.h>
#include "TimeSeries.h"

#include "FreeRTOS.h"
#include "task.h"

#include "fsl_debug_console.h"

#include "framework_sensor_hal.h"
#include "framework_algorithm_hal.h"
#include "framework_algorithm_manager.h"


typedef enum
{
    TSS_STATE_IDLE,
    TSS_STATE_LOGGING,
    TSS_STATE_LEARN,
    TSS_STATE_PREDICT,
    TSS_STATE_MAX,
} tss_state_t;

#ifdef ENABLE_BINARY_ALGO
typedef struct tss_binary_header
{
    char magic[8]; /* "TSS1.5.0" */
    const tss_task_ops_t *(*get_task_ops)(void);
} tss_binary_header_t;
#endif
const static tss_task_ops_t *p_task_ops = NULL;
const  tss_algo_attribute_t *p_algo_attribute = NULL;

static algorithm_hal_callback_t callback;

static tss_state_t tss_state = TSS_STATE_IDLE;

static float prediction_threshold;
static float *p_prob = NULL;

static float *input_data;
static uint32_t data_index = 0;
static uint32_t recursive_cnt = 0;

static uint32_t learn_cnt_total;
static uint32_t signal_cnt_total;
static uint32_t learn_cnt_current;
static uint32_t signal_cnt_current;

#ifdef HS_USB_VCOM_DATA_LOG
static uint8_t *s_CDCACM_send_buf       = NULL;
static uint32_t s_CDCACM_send_buf_len   = 0;
extern void GetCDCACMSendBufInfo(uint8_t **buf, uint32_t *size);
extern int8_t CDCACMSend(uint8_t *data, uint32_t size);
#endif

#ifdef ENABLE_BINARY_ALGO
const tss_task_ops_t *tss_get_task_ops()
{
    return p_task_ops;
}
#endif

static int register_algo_ops()
{
#ifdef ENABLE_BINARY_ALGO
    tss_binary_header_t* (*entry)(void);
    // Convert integer address to function pointer
    extern uint32_t __base_BINARY_FLASH;
    entry = (tss_binary_header_t* (*)(void))(uintptr_t)((uint32_t)(&__base_BINARY_FLASH) + 1);
    tss_binary_header_t *p_algo_head = entry();
    PRINTF("MAGIC NUMBER:");
    int i = 0;
    for(i = 0; i < 8; i++)
    {
        PRINTF("%c", p_algo_head->magic[i]);
    }
    PRINTF("\r\n");
    // TODO Need to verify header content
    p_task_ops = p_algo_head->get_task_ops();
#else
    p_task_ops = tss_get_task_ops();
#endif
    if (p_task_ops == NULL)
    {
        PRINTF("Getting task operation failed!!");
        return -1;
    }

    if (p_task_ops->task > TASK_FC || p_task_ops->task < TASK_AD)
    {
        PRINTF("Task Type invalid !!");
        return -1;
    }
    return 0;
}

tss_status static inline _wrap_tss_init()
{
    tss_status  status = TSS_UNKNOWN_ERROR;
    switch (p_task_ops->task)
    {
    case TASK_AD:  // Anomaly Detection (AD) task specific initialization
        status = p_task_ops->ad_ops.init();
        break;
    case TASK_AD_ODL:
        status = p_task_ops->ad_odl_ops.init((const float *)p_algo_attribute->model_addr);
        break;
    case TASK_CLS:
        status = p_task_ops->cls_ops.init();
        break;
    case TASK_OC:
        status = p_task_ops->oc_ops.init();
        break;
    case TASK_REG:
        status = p_task_ops->reg_ops.init();
        break;
    case TASK_FC:
        status = p_task_ops->fc_ops.init();
        break;
    }
    return status;
}

static void sensor_control_callback(sensor_event_id_t id)
{
    sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
    event->id             = id;
    callback(ALGORITHM_EVENT_ID_SENSOR_CONTROL, event);
}

static void datalog_notification_callback(algorithm_state_t state, algorithm_result_id_t id, sensor_payload_t *payload)
{
    algorithm_result_t *result =
        pvPortMalloc(sizeof(algorithm_result_t) + payload->sample_size * payload->axis_size * sizeof(float));
    result->state = state;
    result->id    = id;
    result->size  = payload->sample_size * payload->axis_size * sizeof(float);
    memcpy(result->data, payload->data, result->size);
    callback(ALGORITHM_EVENT_ID_RESULT_NOTIFICATION, result);
}

static void result_notification_callback(algorithm_state_t state, algorithm_result_id_t id)
{
    algorithm_result_t *result = pvPortMalloc(sizeof(algorithm_result_t));
    result->state              = state;
    result->id                 = id;
    result->size               = 0;
    callback(ALGORITHM_EVENT_ID_RESULT_NOTIFICATION, result);
}

static sensor_event_id_t forecast_recursive_auto_stop()
{
    tss_state = TSS_STATE_IDLE;
    recursive_cnt = 0;
    data_index = 0;

    return SENSOR_EVENT_ID_STOP;
}

static int32_t time_series_init(algorithm_hal_callback_t cb)
{
    tss_status status;

    callback = cb;

    p_algo_attribute = p_task_ops->algo_attribute();
    //TODO better to verify all attributes
    prediction_threshold = p_algo_attribute->recommend_threshold;

    if (p_algo_attribute->powerquad_enabled)
    {
        PRINTF("[INFO] PowerQuad accelerator is enabled in the library.\r\n");
    }
    if (p_algo_attribute->neutron_enabled)
    {
        PRINTF("[INFO] Neutron(v%s) is enabled in this library.\r\n", p_algo_attribute->neutron_version);
    }

    status = _wrap_tss_init();
    if (status != TSS_SUCCESS)
    {
        PRINTF("%s: Time Series init failed (%d)\r\n", __func__, status);
        return -1;
    }

    if (tss_state == TSS_STATE_LOGGING || tss_state == TSS_STATE_LEARN || tss_state == TSS_STATE_PREDICT)
    {
        sensor_control_callback(SENSOR_EVENT_ID_START);
        sensor_control_callback(SENSOR_EVENT_ID_DATA_REQUEST);
    }

    if (p_algo_attribute->target_num)
    {
        p_prob = pvPortMalloc(sizeof(float) * p_algo_attribute->target_num);
        if (p_prob == NULL) {
            PRINTF("Memory allocation for probabilities failed\r\n");
            return -1;
        }
        memset(p_prob, 0, sizeof(float) * p_algo_attribute->target_num);
    }

    input_data = pvPortMalloc(sizeof(float) * p_algo_attribute->data_len*p_algo_attribute->data_dim);
    if (input_data == NULL) {
        PRINTF("Memory allocation for forecast task failed\r\n");
        return -1;
    }

#ifdef HS_USB_VCOM_DATA_LOG
    GetCDCACMSendBufInfo(&s_CDCACM_send_buf, &s_CDCACM_send_buf_len);
    memset(s_CDCACM_send_buf, 0, s_CDCACM_send_buf_len);
#endif
    return 0;
}

static int32_t time_series_process(void *param)
{
    if (tss_state == TSS_STATE_LOGGING)
    {
        sensor_payload_t *payload = (sensor_payload_t *)(param);
        uint32_t total            = payload->sample_size * payload->axis_size;
        float *data               = (float *)(payload->data);

#ifdef HS_USB_VCOM_DATA_LOG
        if (payload->sample_size > 1)
        {
            for (uint32_t index = 0; index < total; index++)
            {
                PRINTF("%.3f", data[index]);
                PRINTF(index != (total - 1) ? " " : "\r\n");
            }
        }
        else
        {
            /* The following implementations depend on the value of the variable "total"
               far less than s_CDCACM_send_buf_len with the payload->sample_size = 1*/
            uint32_t size=0;
            for (uint32_t index = 0; index < total; index++)
            {
                int len = sprintf((char *)(s_CDCACM_send_buf+size), "%.3f ", data[index]);
                size += len;
            }
            s_CDCACM_send_buf[size-1] = '\r';
            s_CDCACM_send_buf[size] = '\n';
            s_CDCACM_send_buf[size+1] = 0;

            CDCACMSend(s_CDCACM_send_buf, size+1);
        }
#else
        for (uint32_t index = 0; index < total; index++)
        {
            PRINTF("%.3f", data[index]);
            PRINTF(index != (total - 1) ? " " : "\r\n");
        }
#endif
        datalog_notification_callback(ALGORITHM_STATE_LOGGING, ALGORITHM_RESULT_ID_SUCCESS, payload);
        sensor_control_callback(SENSOR_EVENT_ID_DATA_REQUEST);
    }
    else if (tss_state == TSS_STATE_LEARN)
    {
        if (p_task_ops->task != TASK_AD_ODL)
        {
            PRINTF("The learn function is only available in the Anomaly Detection task.\r\n");
            result_notification_callback(ALGORITHM_STATE_IDLE, ALGORITHM_RESULT_ID_PREDICT_ONLY);
            tss_state = TSS_STATE_IDLE;
            return 0;
        }
        if (p_task_ops->ad_odl_ops.learn == NULL)
        {
            PRINTF("The learn API is not supported.\r\n");
            result_notification_callback(ALGORITHM_STATE_IDLE, ALGORITHM_RESULT_ID_PREDICT_ONLY);
            tss_state = TSS_STATE_IDLE;
        }

        sensor_payload_t *payload = (sensor_payload_t *)(param);
        float *data               = (float *)(payload->data);

        uint64_t time_us = framework_get_time_us();
        tss_status status = p_task_ops->ad_odl_ops.learn(data);
        time_us = framework_get_time_us() - time_us;
        signal_cnt_current++;
        signal_cnt_total++;

        if (status == TSS_LEARNING_NOT_ENOUGH || status == TSS_RECOMMEND_LEARNING_DONE)
        {
            if (status == TSS_RECOMMEND_LEARNING_DONE)
            {
                learn_cnt_current = signal_cnt_current;
                learn_cnt_total = signal_cnt_total;
            }

            PRINTF("The learning result is %d, time: %d us, samples count [ Current: learn %d of signal %d ] [ Total: learn %d of signal %d ]\r\n",
                status, (uint32_t)time_us, learn_cnt_current, signal_cnt_current, learn_cnt_total, signal_cnt_total);

            if (learn_cnt_current >= p_algo_attribute->recommend_learning_num)
            {
                PRINTF("[INFO] The learning sample number has reached the recommend one(%d), you can start predicting or keep learning.\r\n", p_algo_attribute->recommend_learning_num);
                result_notification_callback(ALGORITHM_STATE_LEARNING, ALGORITHM_RESULT_ID_SUCCESS);
            }
            else
            {
                result_notification_callback(ALGORITHM_STATE_LEARNING, ALGORITHM_RESULT_ID_NOT_ENOUGH);
            }
            sensor_control_callback(SENSOR_EVENT_ID_DATA_REQUEST);
        }
        else
        {
            result_notification_callback(ALGORITHM_STATE_IDLE, ALGORITHM_RESULT_ID_UNKNOWN_ERROR);
            sensor_control_callback(SENSOR_EVENT_ID_STOP);
            tss_state = TSS_STATE_IDLE;
        }
    }
    else if (tss_state == TSS_STATE_PREDICT)
    {
        sensor_event_id_t event_id = SENSOR_EVENT_ID_DATA_REQUEST;
        algorithm_result_id_t result_id = ALGORITHM_RESULT_ID_PREDICT_ONLY;
        sensor_payload_t *payload = (sensor_payload_t *)(param);
        float *data               = (float *)(payload->data);

        uint32_t payload_data_len = payload->axis_size * payload->sample_size;
        uint32_t data_len = data_index + payload_data_len;
        uint32_t algo_data_len = p_algo_attribute->data_len*p_algo_attribute->data_dim;
        if (data_len < algo_data_len)
        {
            memcpy(&input_data[data_index], data, sizeof(float)*payload_data_len);
            data_index = data_len;
        }
        else
        {
            memcpy(&input_data[data_index], data, sizeof(float)*(algo_data_len - data_index));
            data_index = 0;

            switch (p_task_ops->task)
            {
                case TASK_AD:
                {
                      float prob = 0.0f;
                      uint64_t time_us = framework_get_time_us();
                      tss_status status = p_task_ops->ad_ops.predict(input_data, &prob);
                      time_us = framework_get_time_us() - time_us;
                      uint8_t label_index;
                      static const char *label_ad[] = { "normal", "anomaly" };

                      if (prob >= prediction_threshold)
                      {
                          label_index = 0;
                      }
                      else
                      {
                          label_index = 1;
                      }

                      PRINTF("PREDICT tss_status: %d, INFERENCE time: %d us, RESULT [%.6f], label: %s\r\n",
                              status, (uint32_t)time_us, prob, label_ad[label_index]);

                      if (status != TSS_SUCCESS)
                      {
                          result_id = ALGORITHM_RESULT_ID_UNKNOWN_ERROR;
                      }
                      else if (prob < prediction_threshold)
                      {
                          result_id = ALGORITHM_RESULT_ID_ANOMALY_DETECTED;
                      }
                      else
                      {
                          result_id = ALGORITHM_RESULT_ID_SUCCESS;
                      }
                  }
                    break;
                case TASK_AD_ODL:
                {
                    float prob = 0.0f;
                    uint64_t time_us = framework_get_time_us();
                    tss_status status = p_task_ops->ad_odl_ops.predict(input_data, &prob);
                    time_us = framework_get_time_us() - time_us;
                    uint8_t label_index;
                    static const char *label_ad[] = { "normal", "anomaly" };

                    if (prob >= prediction_threshold)
                    {
                        label_index = 0;
                    }
                    else
                    {
                        label_index = 1;
                    }

                    PRINTF("PREDICT tss_status: %d, INFERENCE time: %d us, RESULT [%.6f], label: %s\r\n",
                            status, (uint32_t)time_us, prob, label_ad[label_index]);

                    if (status != TSS_SUCCESS)
                    {
                        result_id = ALGORITHM_RESULT_ID_UNKNOWN_ERROR;
                    }
                    else if (prob < prediction_threshold)
                    {
                        result_id = ALGORITHM_RESULT_ID_ANOMALY_DETECTED;
                    }
                    else
                    {
                        result_id = ALGORITHM_RESULT_ID_SUCCESS;
                    }
                }

                break;
                case TASK_CLS:
                {
                    int class = 0;

                    uint64_t time_us = framework_get_time_us();
                    tss_status status = p_task_ops->cls_ops.predict(input_data, p_prob, &class);
                    time_us = framework_get_time_us() - time_us;

                    PRINTF("PREDICT tss_status: %d, INFERENCE time: %d us, RESULT [", status, (uint32_t)time_us);

                    for (int i = 0; i < p_algo_attribute->target_num; i++)
                    {
                        PRINTF(" %.6f", p_prob[i]);
                    }

                    PRINTF(" ]  CLASS: %d\r\n", class);

                    if (status != TSS_SUCCESS)
                    {
                        result_id = ALGORITHM_RESULT_ID_UNKNOWN_ERROR;
                    }
                    else
                    {
                        result_id = ALGORITHM_RESULT_ID_SUCCESS;
                    }
                }
                break;
                case TASK_OC:
                {
                    float score = 0.0f;
                    int is_outlier = 0;

                    uint64_t time_us = framework_get_time_us();
                    tss_status status = p_task_ops->oc_ops.predict(input_data, &score, &is_outlier);
                    time_us = framework_get_time_us() - time_us;

                    static const char *label_oc[] = { "normal", "anomaly" };

                    PRINTF("PREDICT tss_status: %d, INFERENCE time: %d us, RESULT [%.6f], label: %s\r\n",
                            status, (uint32_t)time_us, score, label_oc[is_outlier]);

                    if (status != TSS_SUCCESS)
                    {
                        result_id = ALGORITHM_RESULT_ID_UNKNOWN_ERROR;
                    }
                    else
                    {
                        result_id = ALGORITHM_RESULT_ID_SUCCESS;
                    }

                }
                break;
                case TASK_REG:
                {
                    uint64_t time_us = framework_get_time_us();
                    tss_status status = p_task_ops->reg_ops.predict(input_data, p_prob);
                    time_us = framework_get_time_us() - time_us;

                    PRINTF("PREDICT tss_status: %d, INFERENCE time: %d us, RESULT [", status, (uint32_t)time_us);

                    for (int i = 0; i < p_algo_attribute->target_num; i++)
                    {
                        PRINTF(" %.6f", p_prob[i]);
                    }

                    PRINTF(" ]\r\n");

                    if (status != TSS_SUCCESS)
                    {
                        result_id = ALGORITHM_RESULT_ID_UNKNOWN_ERROR;
                    }
                    else
                    {
                        result_id = ALGORITHM_RESULT_ID_SUCCESS;
                    }
                }
                break;
                case TASK_FC:
                {
                    if (recursive_cnt)
                    {
                        for (int i = 0; i < p_algo_attribute->target_num; i++)
                        {
                            input_data[algo_data_len - payload->axis_size + p_algo_attribute->forecast_channels[i]-1] = p_prob[i];
                        }
                    }

                    uint64_t time_us = framework_get_time_us();
                    tss_status status = p_task_ops->fc_ops.predict(input_data, p_prob);
                    time_us = framework_get_time_us() - time_us;

                    PRINTF("PREDICT tss_status: %d, INFERENCE time: %d us, RESULT [", status, (uint32_t)time_us);

                    for (int i = 0; i < p_algo_attribute->target_num; i++)
                    {
                        PRINTF(" %.6f", p_prob[i]);
                    }

                    if (p_algo_attribute->recursive_predicted == false)
                    {
                        PRINTF(" ]\r\n");
                    }
                    else
                    {
                        recursive_cnt++;
                        data_index = algo_data_len - payload->axis_size;
                        memmove(input_data, input_data + payload->axis_size, sizeof(float)*(algo_data_len - payload->axis_size));
                        PRINTF("] Recursive count: %d\r\n", recursive_cnt);
                    }
                    if (status != TSS_SUCCESS)
                    {
                        result_id = ALGORITHM_RESULT_ID_UNKNOWN_ERROR;
                        if (p_algo_attribute->recursive_predicted == true)
                        {
                            event_id = forecast_recursive_auto_stop();
                            PRINTF(" Recursive prediction error! Stopped! \r\n");
                        }
                    }
                    else
                    {
                        result_id = ALGORITHM_RESULT_ID_SUCCESS;
                        if (recursive_cnt == p_algo_attribute->recursive_length)
                        {
                            recursive_cnt = 0;
                            data_index = 0;
                        }
                    }
                }
                break;
                default:
                    PRINTF("Cannot execute Prediction Unknown task type: %d.\r\n", p_task_ops->task);
                    result_notification_callback(ALGORITHM_STATE_IDLE, ALGORITHM_RESULT_ID_PREDICT_ONLY);
                    tss_state = TSS_STATE_IDLE;
                    return 0;
            }
        }
        result_notification_callback(ALGORITHM_STATE_PREDICTING, result_id);
        sensor_control_callback(event_id);
    }

    return 0;
}

static int32_t time_series_notify(void *param)
{
    algorithm_event_t *algorithm_event = (algorithm_event_t *)param;
    algorithm_event_id_t id            = algorithm_event->id;

    switch (tss_state)
    {
        case TSS_STATE_IDLE:
        {
            if (id == ALGORITHM_EVENT_ID_TOGGLE_TSS_LEARNING)
            {
                id = ALGORITHM_EVENT_ID_START_TSS_LEARNING;
            }
            if (id == ALGORITHM_EVENT_ID_TOGGLE_TSS_PREDICTION)
            {
                id = ALGORITHM_EVENT_ID_START_TSS_PREDICTION;
            }
            break;
        }

        case TSS_STATE_LOGGING:
        {
            if (id == ALGORITHM_EVENT_ID_TOGGLE_TSS_LEARNING || id == ALGORITHM_EVENT_ID_TOGGLE_TSS_PREDICTION)
            {
                id = ALGORITHM_EVENT_ID_STOP;
            }
            break;
        }

        case TSS_STATE_LEARN:
        {
            if (id == ALGORITHM_EVENT_ID_TOGGLE_TSS_LEARNING || id == ALGORITHM_EVENT_ID_TOGGLE_TSS_PREDICTION)
            {
                id = ALGORITHM_EVENT_ID_STOP;
            }
            break;
        }
        case TSS_STATE_PREDICT:
        {
            if (id == ALGORITHM_EVENT_ID_TOGGLE_TSS_LEARNING || id == ALGORITHM_EVENT_ID_TOGGLE_TSS_PREDICTION)
            {
                id = ALGORITHM_EVENT_ID_STOP;
            }
            break;
        }

        default:
            break;
    }

    switch (id)
    {
        case ALGORITHM_EVENT_ID_SETUP_PREDICTION_THRESHOLD:
        {
            if (p_task_ops->task == TASK_AD || p_task_ops->task == TASK_AD_ODL)
            {
                prediction_threshold = algorithm_event->value;
                PRINTF("Prediction threshold setup successfully.\r\n");
            }
            else
            {
                PRINTF("Setup the prediction threshold is available in the Anomaly Detection task only.\r\n");
            }
            break;
        }

        case ALGORITHM_EVENT_ID_START_DATA_LOGGER:
        {
            sensor_control_callback(SENSOR_EVENT_ID_START);
            sensor_control_callback(SENSOR_EVENT_ID_DATA_REQUEST);
            tss_state = TSS_STATE_LOGGING;
            break;
        }

        case ALGORITHM_EVENT_ID_START_TSS_LEARNING:
        {
            sensor_control_callback(SENSOR_EVENT_ID_START);
            sensor_control_callback(SENSOR_EVENT_ID_DATA_REQUEST);
            tss_state = TSS_STATE_LEARN;
            break;
        }

        case ALGORITHM_EVENT_ID_RESET_TSS_MODEL:
        {
            if (p_task_ops->task == TASK_AD_ODL)
            {
                tss_status status = p_task_ops->ad_odl_ops.init(NULL);
                if (status == TSS_SUCCESS)
                {
                    sensor_control_callback(SENSOR_EVENT_ID_STOP);
                    tss_state = TSS_STATE_IDLE;
                    PRINTF("The model has been reset successfully, enter 'start learn' to continue.\r\n");
                    signal_cnt_total = 0;
                    signal_cnt_current = 0;
                    learn_cnt_total = 0;
                    learn_cnt_current = 0;
                }
            }
            else
            {
                PRINTF("The learn API is not supported.\r\n");
            }
            break;
        }

        case ALGORITHM_EVENT_ID_START_TSS_PREDICTION:
        {
            sensor_control_callback(SENSOR_EVENT_ID_START);
            sensor_control_callback(SENSOR_EVENT_ID_DATA_REQUEST);
            tss_state = TSS_STATE_PREDICT;
            break;
        }

        case ALGORITHM_EVENT_ID_STOP:
        {
            if (p_task_ops->task == TASK_AD_ODL)
            {
                learn_cnt_current = 0;
                signal_cnt_current = 0;
            }
            if (p_task_ops->task == TASK_FC)
            {
                recursive_cnt = 0;
            }
            data_index = 0;
            result_notification_callback(ALGORITHM_STATE_IDLE, ALGORITHM_RESULT_ID_SUCCESS);
            sensor_control_callback(SENSOR_EVENT_ID_STOP);
            tss_state = TSS_STATE_IDLE;
            break;
        }
        case ALGORITHM_EVENT_ID_GET_TSS_STATE:
        {
            const char *TSS_STATE_LIST[] = {
                "TSS_STATE_IDLE", "TSS_STATE_LOGGING", "TSS_STATE_LEARN", "TSS_STATE_PREDICT", "TSS_STATE_MAX",
            };
            PRINTF("Current State is %s\r\n", TSS_STATE_LIST[tss_state]);
            break;
        }
//        case ALGORITHM_EVENT_ID_DYNAMIC_LOAD:
//        {
//            if (current_task_ops != &tss_default_ops)
//            {
//                current_task_ops->deinit();
//            }
//
//            if (algorithm_event->lib_entry == 0)
//            {
//                current_task_ops = &tss_default_ops;
//            }
//            else
//            {
//                typedef const tss_task_ops_t *(*tss_entry_t)(void);
//                tss_entry_t tss_entry = (tss_entry_t)(algorithm_event->lib_entry | 0x1);
//                current_task_ops = tss_entry();
//
//                tss_status status = current_task_ops->init();
//                if (status != TSS_SUCCESS)
//                {
//                    PRINTF("%s: lib:%p init failed (%d)\r\n", __func__, current_task_ops, status);
//                    return -1;
//                }
//            }
//
//            PRINTF("%s: switch to algo lib at %p\r\n", __func__, current_task_ops);
//            break;
//        }

        default:
            break;
    }

    return 0;
}

const static algorithm_hal_t algorithm_hal_time_series = {
    .name    = "Time Series",
    .init    = time_series_init,
    .process = time_series_process,
    .notify  = time_series_notify,
};

int32_t register_algorithm_hal_time_series(algorithm_state_t state)
{
    framework_algorithm_manager_register_hal(&algorithm_hal_time_series);

    register_algo_ops();

    switch (state)
    {
        case ALGORITHM_STATE_IDLE:
        {
            tss_state = TSS_STATE_IDLE;
            break;
        }

        case ALGORITHM_STATE_LOGGING:
        {
            tss_state = TSS_STATE_LOGGING;
            break;
        }

        case ALGORITHM_STATE_LEARNING:
        {
            tss_state = TSS_STATE_LEARN;
            break;
        }

        case ALGORITHM_STATE_PREDICTING:
        {
            tss_state = TSS_STATE_PREDICT;
            break;
        }

        default:
        {
            tss_state = TSS_STATE_IDLE;
            break;
        }
    }

    return 0;
}
