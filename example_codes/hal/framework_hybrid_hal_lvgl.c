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
#include "framework_input_hal.h"
#include "framework_input_manager.h"
#include "framework_output_hal.h"
#include "framework_output_manager.h"

#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"
#include "lv_demo_widgets.h"

typedef struct
{
    uint32_t width;
    uint32_t height;
    void* frame_buff;
    uint32_t frame_buff_size;
    void (*fill_pic)(const lv_area_t *area, lv_color_t *color_p);
    void (*read_touch)(lv_indev_drv_t *drv, lv_indev_data_t *data);
} lvgl_configuration_t;

static lvgl_configuration_t configuration;

static input_hal_callback_t callback;

static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

extern void lvgl_display_notify(algorithm_state_t state, algorithm_result_id_t result, float *features, float score);

static int32_t touch_init(input_hal_callback_t cb)
{
    callback = cb;
    return 0;
}

static void lvgl_rounder(lv_disp_drv_t * disp_drv, lv_area_t * area)
{
   area->x1 &= ~0x1f;
   area->x2 |= 0x1f;
}

static void lvgl_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    configuration.fill_pic(area, color_p);
    lv_disp_flush_ready(disp_drv);
}

static void lvgl_task(void *parameters)
{
    lv_init();

    lv_disp_draw_buf_init(&disp_buf, configuration.frame_buff, NULL, configuration.frame_buff_size);

    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = configuration.width;
    disp_drv.ver_res = configuration.height;
    disp_drv.flush_cb = lvgl_flush;
    disp_drv.rounder_cb = lvgl_rounder;
    disp_drv.draw_buf = &disp_buf;

    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = configuration.read_touch;

    lv_indev_drv_register(&indev_drv);

    lv_demo_widgets();

    for (;;)
    {
        TickType_t tick_start;
        TickType_t tick_end;
        TickType_t delay_target;

        tick_start = xTaskGetTickCount();

        lv_timer_handler();

        tick_end = xTaskGetTickCount();

        if (tick_end - tick_start >= pdMS_TO_TICKS(LV_DISP_DEF_REFR_PERIOD))
        {
            delay_target = 1;
        }
        else
        {
            delay_target = pdMS_TO_TICKS(LV_DISP_DEF_REFR_PERIOD) - (tick_end - tick_start);
        }

        vTaskDelay(delay_target);
    }
}

static int32_t display_init()
{
    if (xTaskCreate(lvgl_task, "LVGL Task", configMINIMAL_STACK_SIZE + 1000, NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        PRINTF("%s: xTaskCreate failed!\r\n", __func__);
        return -1;
    }

    return 0;
}

static int32_t display_notify(void *param)
{
    algorithm_result_t *result = (algorithm_result_t *)param;
    lvgl_display_notify(result->state, result->id, result->features, result->score);
    return 0;
}

const static input_hal_t input_hal_lvgl_touch =
{
    .name = "LVGL Touch",
    .init = touch_init,
    .start = NULL,
};

const static output_hal_t output_hal_lvgl_display =
{
    .name = "LVGL Display",
    .init = display_init,
    .start = NULL,
    .notify = display_notify,
};

void lvgl_touch_notify(algorithm_event_id_t id)
{
    algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
    event->id = id;
    callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
}

int32_t register_hybrid_hal_lvgl(uint32_t width, uint32_t height, void * buff, uint32_t buff_size,
                                 void (*fill_pic)(const lv_area_t *area, lv_color_t *color_p),
                                 void (*read_touch)(lv_indev_drv_t *drv, lv_indev_data_t *data))
{
    configuration.width = width;
    configuration.height = height;
    configuration.frame_buff = buff;
    configuration.frame_buff_size = buff_size;
    configuration.fill_pic = fill_pic;
    configuration.read_touch = read_touch;

    framework_input_manager_register_hal(&input_hal_lvgl_touch);

    framework_output_manager_register_hal(&output_hal_lvgl_display);

    return 0;
}
