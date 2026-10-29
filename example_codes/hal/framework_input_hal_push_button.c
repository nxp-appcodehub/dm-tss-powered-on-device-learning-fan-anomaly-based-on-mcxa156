/*
 * Copyright 2023 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdlib.h>
#include <string.h>

#include "fsl_debug_console.h"
#include "fsl_component_button.h"
#include "fsl_component_timer_manager.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "framework_input_hal.h"
#include "framework_algorithm_hal.h"
#include "framework_input_manager.h"

typedef enum
{
    BUTTON_ID_PREDICTION,
    BUTTON_ID_LEARNING,
    BUTTON_ID_MAX,
} button_id_t;

static input_hal_callback_t callback;

static BUTTON_HANDLE_ARRAY_DEFINE(button_handles, BUTTON_ID_MAX);

static uint8_t button_ids[BUTTON_ID_MAX] = { BUTTON_ID_PREDICTION, BUTTON_ID_LEARNING };

extern button_config_t push_button_configs[];

extern timer_config_t button_timer_config;

static button_status_t button_callback(void *handle, button_callback_message_t *message, void *param)
{
    button_id_t id = *(button_id_t *)param;

    switch (id)
    {
        case BUTTON_ID_PREDICTION:
        {
            switch (message->event)
            {
                case kBUTTON_EventOneClick:
                case kBUTTON_EventDoubleClick:
                case kBUTTON_EventShortPress:
                case kBUTTON_EventLongPress:
                {
                    algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
                    event->id = ALGORITHM_EVENT_ID_TOGGLE_TSS_PREDICTION;
                    callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case BUTTON_ID_LEARNING:
        {
            switch (message->event)
            {
                case kBUTTON_EventOneClick:
                case kBUTTON_EventDoubleClick:
                case kBUTTON_EventShortPress:
                case kBUTTON_EventLongPress:
                {
                    algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
                    event->id = ALGORITHM_EVENT_ID_TOGGLE_TSS_LEARNING;
                    callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return kStatus_BUTTON_Success;
}

static int32_t push_button_init(input_hal_callback_t cb)
{
    callback = cb;

    TM_Init(&button_timer_config);

    for (uint8_t index = 0; index < BUTTON_ID_MAX; index++)
    {
        BUTTON_Init((button_handle_t)button_handles[index], &push_button_configs[index]);
        BUTTON_InstallCallback((button_handle_t)button_handles[index], button_callback, &button_ids[index]);
    }

    return 0;
}

const static input_hal_t input_hal_push_button =
{
    .name = "Push Button",
    .init = push_button_init,
    .start = NULL,
};

int32_t register_input_hal_push_button()
{
    framework_input_manager_register_hal(&input_hal_push_button);

    return 0;
}
