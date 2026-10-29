/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file tss_app.c
 * @brief Validates the linked Time Series Studio model and caches its attributes.
 */

#include "tss_app.h"

#include "fsl_debug_console.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Task-ops table exported by libtss.a, resolved once by APP_TSS_Init(). */
static const tss_task_ops_t *s_ops;

/*! @brief Attributes of the linked model, non-NULL once APP_TSS_Init() succeeded. */
static const tss_algo_attribute_t *s_attr;

/*******************************************************************************
 * Code
 ******************************************************************************/
const tss_algo_attribute_t *APP_TSS_Init(void)
{
    const tss_task_ops_t *ops;
    const tss_algo_attribute_t *attr;

    /*! The table lives in flash and never changes, so resolving it once is enough. */
    if (NULL != s_attr)
    {
        return s_attr;
    }

    ops = tss_get_task_ops();
    if (NULL == ops)
    {
        PRINTF("TSS: the library exports no task ops\r\n");
        return NULL;
    }

    /*! This firmware only implements the anomaly detection + on-device learning flow. */
    if (TASK_AD_ODL != ops->task)
    {
        PRINTF("TSS: task %d is not anomaly detection with ODL\r\n", ops->task);
        return NULL;
    }

    if (NULL == ops->algo_attribute)
    {
        PRINTF("TSS: the task ops carry no attributes\r\n");
        return NULL;
    }

    attr = ops->algo_attribute();
    if (NULL == attr)
    {
        PRINTF("TSS: the attributes are unavailable\r\n");
        return NULL;
    }

    /*! The acquisition task, the queues and the GUI are all sized for this window. */
    if ((APP_TSS_WINDOW_LEN != attr->data_len) || (APP_TSS_WINDOW_DIM != attr->data_dim))
    {
        PRINTF("TSS: model window is %ux%u, application expects %ux%u\r\n", attr->data_len, attr->data_dim,
               APP_TSS_WINDOW_LEN, APP_TSS_WINDOW_DIM);
        return NULL;
    }

    if (NULL == attr->model_addr)
    {
        PRINTF("TSS: the model carries no default weights\r\n");
        return NULL;
    }


    if (0U == attr->odl_supported)
    {
        PRINTF("TSS: the model does not support on-device learning\r\n");
        return NULL;
    }

    if (0U == attr->recommend_learning_num)
    {
        PRINTF("TSS: the model recommends no learning samples\r\n");
        return NULL;
    }

    s_ops  = ops;
    s_attr = attr;

    PRINTF("TSS: %s, %u weights, window %ux%u, threshold %.2f, %u learning windows\r\n",
           (NULL != attr->lib_id) ? attr->lib_id : "unknown", attr->model_size, attr->data_len, attr->data_dim,
           (double)attr->recommend_threshold, attr->recommend_learning_num);

    return s_attr;
}

const tss_algo_attribute_t *APP_TSS_Attribute(void)
{
    return s_attr;
}

const tss_ad_odl_task_ops_t *APP_TSS_Ops(void)
{
    return (NULL == s_ops) ? NULL : &s_ops->ad_odl_ops;
}
