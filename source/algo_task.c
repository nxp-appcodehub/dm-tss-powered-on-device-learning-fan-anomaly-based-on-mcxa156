/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "fsl_debug_console.h"
#include "lvgl_support.h"
#include "lvgl.h"
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"
#include "lvgl_demo_utils.h"
#include "lvgl_freertos.h"
#include "board.h"
#include "fsl_romapi.h"

#include "TimeSeries.h"

#if defined(__GNUC__)
extern uint8_t __base_m_model[];
#endif

#define MODEL_MAGIC (0x53564dFF) //

typedef struct model_save_weight
{
	uint32_t magic;
	uint32_t crc;
	uint32_t wei_len;
	float weight[MODEL_SIZE];
}model_save_weight_t;

model_save_weight_t model_wei;
float sensor_data[TSS_INPUT_DATA_LEN*TSS_INPUT_DATA_DIM];
static float model_weight_buffer[MODEL_SIZE];
uint8_t train_flag = 0;
uint8_t inference_flag = 0;
uint32_t train_cnt = 0;
static flash_config_t s_flashDriver;


extern void app_gui_update_score(float score, float health_thres);
void app_gui_update_progress_bar(int percent);


#define CRC32_POLY 0x04C11DB7

uint32_t crc32(uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    size_t i, j;

    for (i = 0; i < length; i++) {
        crc ^= (uint32_t)data[i];

        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLY;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

void save_model_to_flash()
{
	uint32_t pflashBlockBase  = 0U;
	uint32_t pflashTotalSize  = 0U;
	uint32_t pflashSectorSize = 0U;
	uint32_t PflashPageSize   = 0U;
	int status;

	memset(&s_flashDriver, 0, sizeof(flash_config_t));
	FLASH_API->flash_init(&s_flashDriver);

	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashBlockBaseAddr, &pflashBlockBase);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashSectorSize, &pflashSectorSize);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashTotalSize, &pflashTotalSize);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashPageSize, &PflashPageSize);

	model_wei.wei_len = MODEL_SIZE;
	model_wei.magic = MODEL_MAGIC;
	model_wei.crc = crc32((uint8_t*)model_wei.weight, MODEL_SIZE);

	taskENTER_CRITICAL();
	// total 2K sector erase, the model must be little than this

	status = FLASH_API->flash_erase_sector(&s_flashDriver, __base_m_model, 0x2000, kFLASH_ApiEraseKey);
	if (status != kStatus_Success)
	{
		PRINTF("erase flash failed %d\r\n",status);
		taskEXIT_CRITICAL();
		return;
	}

	uint8_t *model = (uint8_t*)&model_wei;
	uint32_t model_len = sizeof(model_wei);
	for(int i = 0; i < model_len; i += 256){

		status = FLASH_API->flash_program_page(&s_flashDriver, __base_m_model + i, (uint8_t*)model + i, 256);
		if (status != kStatus_Success)
		{
			PRINTF("write flash failed %d\r\n",status);
			break;
		}

	}
	PRINTF("Save Model to flash sucess\r\n");
	taskEXIT_CRITICAL();
}

void app_algo_task(void* parameters)
{
	QueueHandle_t qh_NewSample = (QueueHandle_t)parameters;
	float *model_weigth_buffer;
	//load weight buffer from flash
	model_save_weight_t *saved_wei = __base_m_model;
	if ((saved_wei->magic == MODEL_MAGIC) && (MODEL_SIZE == saved_wei->wei_len))
	{
		uint32_t crc = crc32(saved_wei->weight,saved_wei->wei_len);
		if (crc == saved_wei->crc)
		{
			PRINTF("Load custom model in flash\r\n");
			model_weigth_buffer = (float *)saved_wei->weight;
		}
	}
	else
	{
		PRINTF("Load default model\r\n");
		model_weigth_buffer = model_buffer;
	}

	int status = tss_ad_init(model_weigth_buffer);
	if (status != TSS_SUCCESS)
	{
		PRINTF("Tss init failed %d\r\n",status);
		return;
	}

	inference_flag = 1;
	float score;
	uint32_t tick_sm;
	while(1)
	{
		xQueueReceive(qh_NewSample, sensor_data, portMAX_DELAY);

		if (inference_flag)
		{
			tick_sm = xTaskGetTickCount();
			status = tss_ad_predict(sensor_data,&score);
			if (status != TSS_SUCCESS)
			{
				PRINTF("tss_ad_predict failed %d\r\n",status);
			}
			PRINTF("predict %f, dur:%dms\r\n",score,xTaskGetTickCount()-tick_sm);
			//report score
			app_gui_update_score(score,TSS_RECOMMEND_THRESHOLD);
		}
		else if (train_flag)
		{
			if (train_cnt == 0)
				tick_sm = xTaskGetTickCount();
			status = tss_ad_learn(sensor_data);
			train_cnt ++;
			//update train progress
			app_gui_update_progress_bar((int)((train_cnt*100) / TSS_RECOMMEND_LEARNING_SAMPLE_NUM));
			PRINTF("Train %d times\r\n",train_cnt);

			if (train_cnt == TSS_RECOMMEND_LEARNING_SAMPLE_NUM)
			{
				train_flag = 0;
				train_cnt = 0;
				//update train progress
				tss_ad_export(model_wei.weight);
				//save weight to flash
				save_model_to_flash();
				PRINTF("Model train take %d ms\r\n",xTaskGetTickCount()-tick_sm);
			}
		}

	}
}

void algo_start_train()
{
	train_flag = 1;
	inference_flag = 0;
}

void algo_stop_train()
{
	train_flag = 0;
	inference_flag = 1;
}
