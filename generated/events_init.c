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
extern void algo_restore_model();
extern void app_gui_fan_icon_stop_ratate();
extern void algoo_stop_inference();
extern void algoo_start_inference();
extern void algo_change_train_from_beggining(int flag);
extern void algo_set_train_repeat_count(int count);
static void Main_Main_btnMainSwScr_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
    	app_gui_fan_icon_stop_ratate();
    	algoo_stop_inference();
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
    	algoo_stop_inference();
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
    	algoo_start_inference();
        ui_load_scr_animation(&guider_ui, &guider_ui.Main, guider_ui.Main_del, &guider_ui.Model_Training_del, setup_scr_Main, LV_SCR_LOAD_ANIM_NONE, 200, 200, false, true);
        break;
    }
    default:
        break;
    }
}

static void msgbox_event_cb (lv_event_t *e)
{

	lv_obj_t *mbox1 = (lv_obj_t *)e->user_data;
	lv_obj_t *btns = lv_msgbox_get_btns(mbox1);
	lv_event_code_t code = lv_event_get_code(e);

	switch (code) {
		case LV_EVENT_CLICKED:
		{
			uint16_t id = lv_btnmatrix_get_selected_btn(btns);

			if (id ==0)
				algo_restore_model();

			lv_msgbox_close(mbox1);
		}
		default:
			break;
	}


}

static void Model_btn_default_event_handler(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui *)e->user_data;
	switch (code) {
	case LV_EVENT_CLICKED:
	{

		static  char *btns_txt[] = {"Yes", "No",""};
		lv_obj_t *mbox1 = lv_msgbox_create(ui->Model_Training, "Warning", "Reset to default Model ?",btns_txt,1);

		lv_obj_align(mbox1,  LV_ALIGN_CENTER, 0, 0);

		lv_obj_t *btns = lv_msgbox_get_btns(mbox1);
		lv_obj_add_event_cb(btns, msgbox_event_cb,LV_EVENT_ALL,mbox1);


		break;
	}
	default:
		break;
	}
}

static void Model_Training_Model_Training_sw1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    switch (code) {
    	case LV_EVENT_CLICKED:
    	{
    		lv_obj_t * status_obj = lv_event_get_target(e);
    		int status = lv_obj_has_state(status_obj, LV_STATE_CHECKED) ? 1 : 0;
    		switch(status) {
    		case 0:
    		{
    			algo_change_train_from_beggining(0);
    			break;
    		}
    		case 1:
    		{
    			algo_change_train_from_beggining(1);
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

}


static void Model_Training_Model_Training_repeat_count_event_handler (lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_ui *ui = (lv_ui *)e->user_data;
	char string[10] = {0};
	int count;
	switch (code) {
	case LV_EVENT_VALUE_CHANGED:
	{
		lv_obj_t * status_obj = lv_event_get_target(e);
		count = lv_slider_get_value(status_obj);

		algo_set_train_repeat_count(count);
		sprintf(string, "%d",count);
		lv_label_set_text(ui->Model_Training_repeat_label, string);
		break;
	}
	default:
		break;
	}
}
void events_init_Model_Training (lv_ui *ui)
{
	lv_obj_add_event_cb(ui->Model_Training_repeat_count, Model_Training_Model_Training_repeat_count_event_handler, LV_EVENT_ALL, ui);
	lv_obj_add_event_cb(ui->Model_Training_sw_1, Model_Training_Model_Training_sw1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->Model_Training_Main_btnMainSwScr, Model_Training_Main_btnMainSwScr_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->Model_Training_btn_1, Model_Training_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->Model_Training_btn_default, Model_btn_default_event_handler, LV_EVENT_ALL, ui);
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
