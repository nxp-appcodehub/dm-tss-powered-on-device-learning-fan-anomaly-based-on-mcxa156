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

extern void sensor_task_start_data_collection(uint8_t start_flag);

void setup_scr_DS_Collection(lv_ui *ui)
{
    //Write codes DS_Collection
    ui->DS_Collection = lv_obj_create(NULL);
    lv_obj_set_size(ui->DS_Collection, 480, 320);
    lv_obj_set_scrollbar_mode(ui->DS_Collection, LV_SCROLLBAR_MODE_OFF);

    //Write style for DS_Collection, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DS_Collection, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DS_Collection, lv_color_hex(0x599cc7), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DS_Collection, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->DS_Collection, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->DS_Collection, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->DS_Collection, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DS_Collection_img_1
    ui->DS_Collection_img_1 = lv_img_create(ui->DS_Collection);
    lv_obj_add_flag(ui->DS_Collection_img_1, LV_OBJ_FLAG_CLICKABLE);
    //lv_img_set_src(ui->DS_Collection_img_1, &_NXP_Logo_RGB_Colour_alpha_120x40);
    lv_img_set_pivot(ui->DS_Collection_img_1, 50,50);
    lv_img_set_angle(ui->DS_Collection_img_1, 0);
    lv_obj_set_pos(ui->DS_Collection_img_1, 10, 10);
    lv_obj_set_size(ui->DS_Collection_img_1, 120, 40);

    //Write style for DS_Collection_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->DS_Collection_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->DS_Collection_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DS_Collection_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->DS_Collection_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DS_Collection_label_1
    ui->DS_Collection_label_1 = lv_label_create(ui->DS_Collection);
    lv_label_set_text(ui->DS_Collection_label_1, "Data Collector");
    lv_label_set_long_mode(ui->DS_Collection_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->DS_Collection_label_1, 94, 20);
    lv_obj_set_size(ui->DS_Collection_label_1, 268, 34);

    //Write style for DS_Collection_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DS_Collection_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DS_Collection_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DS_Collection_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DS_Collection_label_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DS_Collection_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DS_Collection_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DS_Collection_btn_1
    ui->DS_Collection_btn_1 = lv_btn_create(ui->DS_Collection);
    ui->DS_Collection_btn_1_label = lv_label_create(ui->DS_Collection_btn_1);
    lv_label_set_text(ui->DS_Collection_btn_1_label, "Return");
    lv_label_set_long_mode(ui->DS_Collection_btn_1_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->DS_Collection_btn_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->DS_Collection_btn_1, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->DS_Collection_btn_1_label, LV_PCT(100));
    lv_obj_set_pos(ui->DS_Collection_btn_1, 192, 248);
    lv_obj_set_size(ui->DS_Collection_btn_1, 70, 30);

    //Write style for DS_Collection_btn_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->DS_Collection_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->DS_Collection_btn_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->DS_Collection_btn_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->DS_Collection_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DS_Collection_btn_1, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DS_Collection_btn_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DS_Collection_btn_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DS_Collection_btn_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DS_Collection_btn_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DS_Collection_btn_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes DS_Collection_label_2
    ui->DS_Collection_label_2 = lv_label_create(ui->DS_Collection);
    lv_label_set_text(ui->DS_Collection_label_2, "UART Baudrate : 230400\n\nData format: \n\nx0  y0  z0\nx1  y1  z1\nx2  y2  z2\n...\n");
    lv_label_set_long_mode(ui->DS_Collection_label_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->DS_Collection_label_2, 111, 71);
    lv_obj_set_size(ui->DS_Collection_label_2, 264, 143);

    //Write style for DS_Collection_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->DS_Collection_label_2, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->DS_Collection_label_2, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->DS_Collection_label_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->DS_Collection_label_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->DS_Collection_label_2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->DS_Collection_label_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of DS_Collection.


    //Update current screen layout.
    lv_obj_update_layout(ui->DS_Collection);

    //Init events for screen.
    events_init_DS_Collection(ui);

    sensor_task_start_data_collection(1);
}
