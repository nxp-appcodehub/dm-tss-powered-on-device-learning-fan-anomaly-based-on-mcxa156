/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_Model_Training(lv_ui *ui)
{
    //Write codes Model_Training
    ui->Model_Training = lv_obj_create(NULL);
    lv_obj_set_size(ui->Model_Training, 480, 320);
    lv_obj_set_scrollbar_mode(ui->Model_Training, LV_SCROLLBAR_MODE_OFF);

    //Write style for Model_Training, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->Model_Training, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Model_Training, lv_color_hex(0x599cc7), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->Model_Training, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->Model_Training, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->Model_Training, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->Model_Training, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Model_Training_img_1
    ui->Model_Training_img_1 = lv_img_create(ui->Model_Training);
    lv_obj_add_flag(ui->Model_Training_img_1, LV_OBJ_FLAG_CLICKABLE);
    //lv_img_set_src(ui->Model_Training_img_1, &_NXP_Logo_RGB_Colour_alpha_120x40);
    lv_img_set_pivot(ui->Model_Training_img_1, 50,50);
    lv_img_set_angle(ui->Model_Training_img_1, 0);
    lv_obj_set_pos(ui->Model_Training_img_1, 10, 10);
    lv_obj_set_size(ui->Model_Training_img_1, 120, 40);

    //Write style for Model_Training_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->Model_Training_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->Model_Training_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Model_Training_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->Model_Training_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Model_Training_Main_btnMainSwScr
    ui->Model_Training_Main_btnMainSwScr = lv_btn_create(ui->Model_Training);
    ui->Model_Training_Main_btnMainSwScr_label = lv_label_create(ui->Model_Training_Main_btnMainSwScr);
    lv_label_set_text(ui->Model_Training_Main_btnMainSwScr_label, "Start");
    lv_label_set_long_mode(ui->Model_Training_Main_btnMainSwScr_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->Model_Training_Main_btnMainSwScr_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->Model_Training_Main_btnMainSwScr, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->Model_Training_Main_btnMainSwScr_label, LV_PCT(100));
    lv_obj_set_pos(ui->Model_Training_Main_btnMainSwScr, 88, 260);
    lv_obj_set_size(ui->Model_Training_Main_btnMainSwScr, 70, 30);

    //Write style for Model_Training_Main_btnMainSwScr, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->Model_Training_Main_btnMainSwScr, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Model_Training_Main_btnMainSwScr, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->Model_Training_Main_btnMainSwScr, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->Model_Training_Main_btnMainSwScr, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Model_Training_Main_btnMainSwScr, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->Model_Training_Main_btnMainSwScr, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->Model_Training_Main_btnMainSwScr, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Model_Training_Main_btnMainSwScr, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->Model_Training_Main_btnMainSwScr, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->Model_Training_Main_btnMainSwScr, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Model_Training_label_1
    ui->Model_Training_label_1 = lv_label_create(ui->Model_Training);
    lv_label_set_text(ui->Model_Training_label_1, "Trainer");
    lv_label_set_long_mode(ui->Model_Training_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->Model_Training_label_1, 94, 20);
    lv_obj_set_size(ui->Model_Training_label_1, 268, 34);

    //Write style for Model_Training_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->Model_Training_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Model_Training_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->Model_Training_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->Model_Training_label_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->Model_Training_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->Model_Training_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Model_Training_btn_1
    ui->Model_Training_btn_1 = lv_btn_create(ui->Model_Training);
    ui->Model_Training_btn_1_label = lv_label_create(ui->Model_Training_btn_1);
    lv_label_set_text(ui->Model_Training_btn_1_label, "Return");
    lv_label_set_long_mode(ui->Model_Training_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->Model_Training_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->Model_Training_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->Model_Training_btn_1_label, LV_PCT(100));
    lv_obj_set_pos(ui->Model_Training_btn_1, 321, 260);
    lv_obj_set_size(ui->Model_Training_btn_1, 70, 30);

    //Write style for Model_Training_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->Model_Training_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Model_Training_btn_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->Model_Training_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->Model_Training_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Model_Training_btn_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->Model_Training_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->Model_Training_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Model_Training_btn_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->Model_Training_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->Model_Training_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Model_Training_barModelTrainProgress
    ui->Model_Training_barModelTrainProgress = lv_textprogress_create(ui->Model_Training);
    lv_textprogress_set_range_value(ui->Model_Training_barModelTrainProgress, 0, 100, 0, 0);
    lv_textprogress_set_decimal(ui->Model_Training_barModelTrainProgress, 2);
    lv_textprogress_set_value(ui->Model_Training_barModelTrainProgress, 0);
    lv_obj_set_pos(ui->Model_Training_barModelTrainProgress, 46, 184);
    lv_obj_set_size(ui->Model_Training_barModelTrainProgress, 391, 20);

    //Write style for Model_Training_barModelTrainProgress, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->Model_Training_barModelTrainProgress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->Model_Training_barModelTrainProgress, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Model_Training_barModelTrainProgress, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->Model_Training_barModelTrainProgress, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->Model_Training_barModelTrainProgress, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->Model_Training_barModelTrainProgress, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->Model_Training_barModelTrainProgress, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Model_Training_barModelTrainProgress, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->Model_Training_barModelTrainProgress, LV_GRAD_DIR_VER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->Model_Training_barModelTrainProgress, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->Model_Training_barModelTrainProgress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->Model_Training_barModelTrainProgress, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->Model_Training_barModelTrainProgress, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->Model_Training_barModelTrainProgress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->Model_Training_barModelTrainProgress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->Model_Training_barModelTrainProgress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of Model_Training.


    //Update current screen layout.
    lv_obj_update_layout(ui->Model_Training);

    //Init events for screen.
    events_init_Model_Training(ui);
}
