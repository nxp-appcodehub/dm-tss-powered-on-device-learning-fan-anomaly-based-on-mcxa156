/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"
#include "fsl_debug_console.h"
static int fanState = 1;
lv_chart_series_t* g_pSeriesAry[3];
lv_timer_t *g_pTmrFanRotate;
bool bscrMainInit = false;
extern const lv_img_dsc_t _fan_normal_on_alpha_160x160;
extern const lv_img_dsc_t Main_Main_img_statusfan_abnormal ;
static float fanHealth = 0.0;
// define a simple function to format the value
void format_int(uint8_t *buf, uint32_t value, uint32_t max_bits){
    switch(max_bits){
        case 4:
            *buf++ = (value / 1000) + '0';
            value %= 1000;
        case 3:
            *buf++ = (value / 100) + '0';
            value %= 100;
        case 2:
            *buf++ = (value / 10) + '0';
            value %= 10;
        case 1:
            *buf++ = (value) + '0';
        default:
            break;
    }
}
void format_decimal(uint8_t *buf, float value){
    uint32_t sign_bit = 0;
    if(value < 0){
        buf[sign_bit] = '-';
        sign_bit = 1;
        value = -value;
    }
    format_int(buf+sign_bit, value, 2);
    buf[sign_bit+2] = '.';
    format_int(buf+sign_bit+2+1, ((value - (uint32_t)value)*1000), 3);
}

void ShowHealthWaveform(float health) {
	lv_ui* ui = &guider_ui;


    uint32_t uHealth = (uint32_t)(health * 100.0f + 0.5f);

    lv_chart_set_next_value(ui->Main_Main_chart_1, g_pSeriesAry[0], uHealth);

    uint8_t buffer[7+3+1+3+1] = "Health:"; // one sign bit, 2 int, 1., 3 decimal

    format_decimal(buffer+7, health);
    lv_label_set_text(ui->Main_Main_chart_Label, (const char*)buffer);
}

void OnTmr_FanRotate(lv_timer_t* pTmr)
{
    lv_ui* ui = &guider_ui;
	// note: angle unit is 0.1 degree
    lv_obj_t* pFan = (lv_obj_t*)pTmr->user_data;

    uint16_t angle = lv_img_get_angle(pFan);

    if (fanState == 0) {
    	lv_img_set_src(pFan,&Main_Main_img_statusfan_abnormal);
    } else {
    	lv_img_set_src(pFan,&_fan_normal_on_alpha_160x160);
    }
    angle = (angle + 150) % 3600;
    lv_img_set_angle(pFan, angle);

    ShowHealthWaveform(fanHealth);
}

void app_gui_fan_icon_stop_ratate()
{
	if (g_pTmrFanRotate)
		lv_timer_del(g_pTmrFanRotate);
	g_pTmrFanRotate = NULL;
}


void SetChartInitData(void)
{
    lv_ui* ui = &guider_ui;
	lv_chart_series_t * Main_Main_chart_1_0 = lv_chart_add_series(ui->Main_Main_chart_1, lv_color_make(0x00, 0x00, 0x00), LV_CHART_AXIS_PRIMARY_Y);
	// lv_chart_series_t * Main_chart_1_1 = lv_chart_add_series(ui->Main_chart_1, lv_color_make(0xcc, 0x2d, 0x2d), LV_CHART_AXIS_PRIMARY_Y);
    // lv_chart_series_t * Main_chart_1_2 = lv_chart_add_series(ui->Main_chart_1, lv_color_make(0x37, 0xa5, 0x18), LV_CHART_AXIS_PRIMARY_Y);
    g_pSeriesAry[0] = Main_Main_chart_1_0;
    // g_pSeriesAry[1] = Main_chart_1_1;
    // g_pSeriesAry[2] = Main_chart_1_2;
    lv_chart_set_point_count(ui->Main_Main_chart_1, 20);
    lv_chart_set_div_line_count(ui->Main_Main_chart_1, 5, 4);
}

void app_gui_update_score(float score, float health_thres)
{
	if (score < health_thres)
		fanState = 0;
	else
		fanState = 1;

	fanHealth= score;

}

void app_gui_update_progress_bar(int percent)
{
	lv_ui* ui = &guider_ui;
	PRINTF("Progress %d\r\n",percent);
	lv_textprogress_set_value(ui->Model_Training_barModelTrainProgress, percent);
}

void SetupFanRotating(void)
{
    lv_img_set_pivot(guider_ui.Main_Main_img_status, 80,80);
    g_pTmrFanRotate = lv_timer_create(OnTmr_FanRotate, 150, guider_ui.Main_Main_img_status);
}

void Setup_main_windows()
{

	{
		SetupFanRotating();
		SetChartInitData();
		bscrMainInit = true;
	}
}
