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



void setup_scr_Main(lv_ui *ui)
{
    //Write codes Main
    ui->Main = lv_obj_create(NULL);
    lv_obj_set_size(ui->Main, 480, 320);
    lv_obj_set_scrollbar_mode(ui->Main, LV_SCROLLBAR_MODE_OFF);

    //Write style for Main, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->Main, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Main, lv_color_hex(0x599cc7), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->Main, LV_GRAD_DIR_HOR, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui->Main, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_main_stop(ui->Main, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_stop(ui->Main, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Main_img_1
    ui->Main_img_1 = lv_img_create(ui->Main);
    lv_obj_add_flag(ui->Main_img_1, LV_OBJ_FLAG_CLICKABLE);
    //lv_img_set_src(ui->Main_img_1, &_NXP_Logo_RGB_Colour_alpha_120x40);
    lv_img_set_pivot(ui->Main_img_1, 50,50);
    lv_img_set_angle(ui->Main_img_1, 0);
    lv_obj_set_pos(ui->Main_img_1, 10, 10);
    lv_obj_set_size(ui->Main_img_1, 120, 40);

    //Write style for Main_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->Main_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->Main_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Main_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->Main_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Main_Main_img_status
    ui->Main_Main_img_status = lv_img_create(ui->Main);
    lv_obj_add_flag(ui->Main_Main_img_status, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->Main_Main_img_status, &_fan_normal_on_alpha_160x160);
    lv_img_set_pivot(ui->Main_Main_img_status, 50,50);
    lv_img_set_angle(ui->Main_Main_img_status, 0);
    lv_obj_set_pos(ui->Main_Main_img_status, 282, 28);
    lv_obj_set_size(ui->Main_Main_img_status, 160, 160);

    //Write style for Main_Main_img_status, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->Main_Main_img_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->Main_Main_img_status, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Main_Main_img_status, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->Main_Main_img_status, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Main_Main_chart_1
    ui->Main_Main_chart_1 = lv_chart_create(ui->Main);
    lv_chart_set_type(ui->Main_Main_chart_1, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(ui->Main_Main_chart_1, 5, 4);
    lv_chart_set_point_count(ui->Main_Main_chart_1, 5);
    lv_chart_set_range(ui->Main_Main_chart_1, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_range(ui->Main_Main_chart_1, LV_CHART_AXIS_SECONDARY_Y, 0, 100);



    lv_obj_set_pos(ui->Main_Main_chart_1, 224, 214);
    lv_obj_set_size(ui->Main_Main_chart_1, 240, 104);
    lv_obj_set_scrollbar_mode(ui->Main_Main_chart_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for Main_Main_chart_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->Main_Main_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Main_Main_chart_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->Main_Main_chart_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->Main_Main_chart_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->Main_Main_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->Main_Main_chart_1, lv_color_hex(0xe8e8e8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->Main_Main_chart_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Main_Main_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->Main_Main_chart_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->Main_Main_chart_1, lv_color_hex(0xe8e8e8), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->Main_Main_chart_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->Main_Main_chart_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write style for Main_Main_chart_1, Part: LV_PART_TICKS, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->Main_Main_chart_1, lv_color_hex(0x151212), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Main_Main_chart_1, &lv_font_montserratMedium_12, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->Main_Main_chart_1, 255, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(ui->Main_Main_chart_1, 2, LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(ui->Main_Main_chart_1, lv_color_hex(0xe8e8e8), LV_PART_TICKS|LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(ui->Main_Main_chart_1, 255, LV_PART_TICKS|LV_STATE_DEFAULT);

    ui->Main_Main_chart_Label = lv_label_create(ui->Main);
	lv_label_set_text(ui->Main_Main_chart_Label, "Health:0.00");
	lv_label_set_long_mode(ui->Main_Main_chart_Label, LV_LABEL_LONG_WRAP);
	lv_obj_set_pos(ui->Main_Main_chart_Label, 224, 190);
	lv_obj_set_size(ui->Main_Main_chart_Label, 200, 16);

	//Write style for Main_Main_chart_Label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
	lv_obj_set_style_border_width(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_radius(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(ui->Main_Main_chart_Label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(ui->Main_Main_chart_Label, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_opa(ui->Main_Main_chart_Label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_letter_space(ui->Main_Main_chart_Label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_line_space(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(ui->Main_Main_chart_Label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
	lv_obj_set_style_shadow_width(ui->Main_Main_chart_Label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Main_Main_btnMainSwScr
    ui->Main_Main_btnMainSwScr = lv_btn_create(ui->Main);
    ui->Main_Main_btnMainSwScr_label = lv_label_create(ui->Main_Main_btnMainSwScr);
    lv_label_set_text(ui->Main_Main_btnMainSwScr_label, "Train");
    lv_label_set_long_mode(ui->Main_Main_btnMainSwScr_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->Main_Main_btnMainSwScr_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->Main_Main_btnMainSwScr, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->Main_Main_btnMainSwScr_label, LV_PCT(100));
    lv_obj_set_pos(ui->Main_Main_btnMainSwScr, 14, 270);
    lv_obj_set_size(ui->Main_Main_btnMainSwScr, 70, 30);

    //Write style for Main_Main_btnMainSwScr, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->Main_Main_btnMainSwScr, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Main_Main_btnMainSwScr, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->Main_Main_btnMainSwScr, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->Main_Main_btnMainSwScr, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Main_Main_btnMainSwScr, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->Main_Main_btnMainSwScr, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->Main_Main_btnMainSwScr, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Main_Main_btnMainSwScr, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->Main_Main_btnMainSwScr, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->Main_Main_btnMainSwScr, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Main_btn_Data_collect
    ui->Main_btn_Data_collect = lv_btn_create(ui->Main);
    lv_obj_add_flag(ui->Main_btn_Data_collect, LV_OBJ_FLAG_CHECKABLE);
    ui->Main_btn_Data_collect_label = lv_label_create(ui->Main_btn_Data_collect);
    lv_label_set_text(ui->Main_btn_Data_collect_label, "Data Collect");
    lv_label_set_long_mode(ui->Main_btn_Data_collect_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->Main_btn_Data_collect_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->Main_btn_Data_collect, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->Main_btn_Data_collect_label, LV_PCT(100));
    lv_obj_set_pos(ui->Main_btn_Data_collect, 103, 270);
    lv_obj_set_size(ui->Main_btn_Data_collect, 110, 30);

    //Write style for Main_btn_Data_collect, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->Main_btn_Data_collect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->Main_btn_Data_collect, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->Main_btn_Data_collect, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->Main_btn_Data_collect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Main_btn_Data_collect, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->Main_btn_Data_collect, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->Main_btn_Data_collect, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Main_btn_Data_collect, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->Main_btn_Data_collect, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->Main_btn_Data_collect, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes Main_label_1
    ui->Main_label_1 = lv_label_create(ui->Main);
    lv_label_set_text(ui->Main_label_1, "Anomaly Detect\n\nPowered by  \n\neIQ TSS");
    lv_label_set_long_mode(ui->Main_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->Main_label_1, 29, 85);
    lv_obj_set_size(ui->Main_label_1, 166, 107);

    //Write style for Main_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->Main_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->Main_label_1, &lv_font_montserratMedium_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->Main_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->Main_label_1, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->Main_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->Main_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of Main.


    //Update current screen layout.
    lv_obj_update_layout(ui->Main);

    //Init events for screen.
    events_init_Main(ui);

    Setup_main_windows();
}
