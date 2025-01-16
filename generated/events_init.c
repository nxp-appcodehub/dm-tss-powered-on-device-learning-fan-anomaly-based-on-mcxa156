/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif


extern void sensor_task_start_data_collection(uint8_t start_flag);
extern void algo_start_train();
extern void algo_stop_train();
extern void app_gui_fan_icon_stop_ratate();
static void Main_Main_btnMainSwScr_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
    	app_gui_fan_icon_stop_ratate();
        ui_load_scr_animation(&guider_ui, &guider_ui.Model_Training, guider_ui.Model_Training_del, &guider_ui.Main_del, setup_scr_Model_Training, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, true);
        break;
    }
    default:
        break;
    }
}

static void Main_btn_Data_collect_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
    	app_gui_fan_icon_stop_ratate();
        ui_load_scr_animation(&guider_ui, &guider_ui.DS_Collection, guider_ui.DS_Collection_del, &guider_ui.Main_del, setup_scr_DS_Collection, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, true);

        break;
    }
    default:
        break;
    }
}

void events_init_Main (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->Main_Main_btnMainSwScr, Main_Main_btnMainSwScr_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->Main_btn_Data_collect, Main_btn_Data_collect_event_handler, LV_EVENT_ALL, ui);
}

static void Model_Training_Main_btnMainSwScr_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
    	algo_start_train();
        break;
    }
    default:
        break;
    }
}

static void Model_Training_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
    	algo_stop_train();
        ui_load_scr_animation(&guider_ui, &guider_ui.Main, guider_ui.Main_del, &guider_ui.Model_Training_del, setup_scr_Main, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, true);
        break;
    }
    default:
        break;
    }
}

void events_init_Model_Training (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->Model_Training_Main_btnMainSwScr, Model_Training_Main_btnMainSwScr_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->Model_Training_btn_1, Model_Training_btn_1_event_handler, LV_EVENT_ALL, ui);
}


static void DS_Collection_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        ui_load_scr_animation(&guider_ui, &guider_ui.Main, guider_ui.Main_del, &guider_ui.DS_Collection_del, setup_scr_Main, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, true);
        sensor_task_start_data_collection(0);
        break;
    }
    default:
        break;
    }
}

void events_init_DS_Collection (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->DS_Collection_btn_1, DS_Collection_btn_1_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}
