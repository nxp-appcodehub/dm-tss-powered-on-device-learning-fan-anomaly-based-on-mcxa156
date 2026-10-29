/*
 * Copyright 2023 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "fsl_debug_console.h"

#include "framework_algorithm_hal.h"
#include "framework_output_hal.h"
#include "framework_output_manager.h"

typedef enum { RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PRUPLE, WHITE, OFF, COLOR_MAX } rgb_led_color_t;

static rgb_led_color_t prev_color = OFF;

const static bool color_control_matrix[COLOR_MAX][3] =
{
    { true, false, false },
    { true, true, false },
    { true, true, false },
    { false, true, false },
    { false, true, true },
    { false, false, true },
    { true, false, true },
    { true, true, true },
    { false, false, false },
};

static void rgb_led_set_color(rgb_led_color_t color)
{
    if (prev_color == color)
    {
        prev_color = OFF;
        LED_RED_OFF();
        LED_GREEN_OFF();
        LED_BLUE_OFF();
    }
    else
    {
        prev_color = color;
        (color_control_matrix[color][0]) ? LED_RED_ON() : LED_RED_OFF();
        (color_control_matrix[color][1]) ? LED_GREEN_ON() : LED_GREEN_OFF();
        (color_control_matrix[color][2]) ? LED_BLUE_ON() : LED_BLUE_OFF();
    }
}

static int32_t rgb_led_init()
{
    LED_RED_INIT(1);
    LED_GREEN_INIT(1);
    LED_BLUE_INIT(1);

    return 0;
}

static int32_t rgb_led_notify(void *param)
{
    algorithm_result_t *result = (algorithm_result_t *)param;

    switch (result->state)
    {
        case ALGORITHM_STATE_LOGGING:
        {
            switch (result->id)
            {
                case ALGORITHM_RESULT_ID_SUCCESS:
                {
                    rgb_led_set_color(BLUE);
                    break;
                }
                default:
                {
                    rgb_led_set_color(WHITE);
                    break;
                }
            }
            break;
        }

        case ALGORITHM_STATE_LEARNING:
        {
            switch (result->id)
            {
                case ALGORITHM_RESULT_ID_SUCCESS:
                {
                    rgb_led_set_color(GREEN);
                    break;
                }
                case ALGORITHM_RESULT_ID_NOT_ENOUGH:
                {
                    rgb_led_set_color(BLUE);
                    break;
                }
                default:
                {
                    rgb_led_set_color(WHITE);
                    break;
                }
            }
            break;
        }

        case ALGORITHM_STATE_PREDICTING:
        {
            switch (result->id)
            {
                case ALGORITHM_RESULT_ID_SUCCESS:
                {
                    rgb_led_set_color(GREEN);
                    break;
                }
                case ALGORITHM_RESULT_ID_ANOMALY_DETECTED:
                {
                    rgb_led_set_color(RED);
                    break;
                }
                default:
                {
                    rgb_led_set_color(WHITE);
                    break;
                }
            }
            break;
        }

        case ALGORITHM_STATE_IDLE:
        {
            switch (result->id)
            {
                case ALGORITHM_RESULT_ID_UNKNOWN_ERROR:
                {
                    rgb_led_set_color(RED);
                    break;
                }

                case ALGORITHM_RESULT_ID_PREDICT_ONLY:
                {
                    rgb_led_set_color(YELLOW);
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }

        default:
        {
            rgb_led_set_color(OFF);
            break;
        }
    }

    return 0;
}

const static output_hal_t output_hal_rgb_led =
{
    .name = "RGB LED",
    .init = rgb_led_init,
    .start = NULL,
    .notify = rgb_led_notify,
};

int32_t register_output_hal_rgb_led()
{
    framework_output_manager_register_hal(&output_hal_rgb_led);

    return 0;
}
