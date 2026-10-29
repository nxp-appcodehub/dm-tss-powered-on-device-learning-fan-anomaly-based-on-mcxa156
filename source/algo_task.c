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

#include <stddef.h>
#include <string.h>

#include "tss_app.h"

#if defined(__GNUC__)
extern uint8_t __base_m_model[];
#endif

/*! @brief Flash sector holding the model record: erased as a whole and rewritten. */
#define MODEL_FLASH_SECTOR_SIZE (0x2000U)

/*! @brief Largest flash page the staging buffer below can serve. */
#define MODEL_FLASH_PAGE_MAX (256U)

#define MODEL_MAGIC (0x53564dFF) //

typedef struct model_save_weight
{
	uint32_t magic;
	uint32_t crc;
	uint32_t wei_len;
	float weight[APP_TSS_MODEL_MAX_FLOATS];
}model_save_weight_t;

model_save_weight_t model_wei;
float sensor_data[APP_TSS_WINDOW_FLOATS];
uint8_t train_flag = 0;
uint8_t inference_flag = 0;
uint32_t train_cnt = 0;
static flash_config_t s_flashDriver;
static int train_from_beginning = 0;
static uint32_t train_repeat_count = 1;

/*! @brief Staging buffer for one flash page; the flash API programs whole pages only. */
static uint8_t s_flashPage[MODEL_FLASH_PAGE_MAX];

extern void app_gui_update_score(float score, float health_thres);
void app_gui_update_progress_bar(int percent);


#define CRC32_POLY 0x04C11DB7

uint32_t crc32(const uint8_t *data, size_t length) {
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
	uint32_t image_len;
	uint32_t offset;
	int status;
	const tss_algo_attribute_t *attr = APP_TSS_Attribute();

	if (NULL == attr)
	{
		return;
	}

	memset(&s_flashDriver, 0, sizeof(flash_config_t));
	FLASH_API->flash_init(&s_flashDriver);

	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashBlockBaseAddr, &pflashBlockBase);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashSectorSize, &pflashSectorSize);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashTotalSize, &pflashTotalSize);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashPageSize, &PflashPageSize);

	if ((0U == PflashPageSize) || (PflashPageSize > MODEL_FLASH_PAGE_MAX))
	{
		PRINTF("Unsupported flash page size %u\r\n", PflashPageSize);
		return;
	}

	/*! Record header followed by the exported weights, without the unused tail. */
	image_len = (uint32_t)offsetof(model_save_weight_t, weight) + (attr->model_size * (uint32_t)sizeof(float));
	if (image_len > MODEL_FLASH_SECTOR_SIZE)
	{
		PRINTF("Model record of %u bytes does not fit the flash sector\r\n", image_len);
		return;
	}

	model_wei.wei_len = attr->model_size;
	model_wei.magic = MODEL_MAGIC;
	model_wei.crc = crc32((const uint8_t *)model_wei.weight, attr->model_size * sizeof(float));

	taskENTER_CRITICAL();
	// total 2K sector erase, the model must be little than this

	status = FLASH_API->flash_erase_sector(&s_flashDriver, (uint32_t)__base_m_model, MODEL_FLASH_SECTOR_SIZE, kFLASH_ApiEraseKey);
	if (status != kStatus_Success)
	{
		PRINTF("erase flash failed %d\r\n",status);
		taskEXIT_CRITICAL();
		return;
	}

	/*! Program page by page; the last page is padded with the erased value. */
	for (offset = 0; offset < image_len; offset += PflashPageSize)
	{
		uint32_t chunk = image_len - offset;

		if (chunk > PflashPageSize)
		{
			chunk = PflashPageSize;
		}

		memset(s_flashPage, 0xFF, PflashPageSize);
		memcpy(s_flashPage, (const uint8_t *)&model_wei + offset, chunk);

		status = FLASH_API->flash_program_page(&s_flashDriver, (uint32_t)(__base_m_model + offset), s_flashPage, PflashPageSize);
		if (status != kStatus_Success)
		{
			PRINTF("write flash failed %d\r\n",status);
			taskEXIT_CRITICAL();
			return;
		}
	}
	PRINTF("Save Model to flash sucess\r\n");
	taskEXIT_CRITICAL();
}


void algo_change_train_from_beggining(int flag)
{
	train_from_beginning = flag;
}

void algo_set_train_repeat_count(int count)
{
	if(count > 0 && count < 6)
		train_repeat_count = count;
}

void algo_restore_model()
{
	uint32_t pflashBlockBase  = 0U;
	uint32_t pflashTotalSize  = 0U;
	uint32_t pflashSectorSize = 0U;
	uint32_t PflashPageSize   = 0U;
	int status;
	const tss_algo_attribute_t *attr = APP_TSS_Init();
	const tss_ad_odl_task_ops_t *ops = APP_TSS_Ops();

	if ((NULL == attr) || (NULL == ops))
	{
		return;
	}

	memset(&s_flashDriver, 0, sizeof(flash_config_t));
	FLASH_API->flash_init(&s_flashDriver);

	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashBlockBaseAddr, &pflashBlockBase);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashSectorSize, &pflashSectorSize);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashTotalSize, &pflashTotalSize);
	FLASH_API->flash_get_property(&s_flashDriver, kFLASH_PropertyPflashPageSize, &PflashPageSize);

	model_wei.wei_len = 0x0;
	model_wei.magic = 0x0;
	model_wei.crc = 0x00;

	taskENTER_CRITICAL();
	// total 2K sector erase, the model must be little than this

	status = FLASH_API->flash_erase_sector(&s_flashDriver, (uint32_t)__base_m_model, MODEL_FLASH_SECTOR_SIZE, kFLASH_ApiEraseKey);
	if (status != kStatus_Success)
	{
		PRINTF("erase flash failed %d\r\n",status);
		taskEXIT_CRITICAL();
		return;
	}

	status = ops->init(attr->model_addr);
	if (status != TSS_SUCCESS)
	{
		PRINTF("Tss init failed %d\r\n",status);
		taskEXIT_CRITICAL();
		return;
	}

	PRINTF("Reset to default model\r\n");
	taskEXIT_CRITICAL();
}

void app_algo_task(void* parameters)
{
	QueueHandle_t qh_NewSample = (QueueHandle_t)parameters;
	const tss_algo_attribute_t *attr = APP_TSS_Init();
	const tss_ad_odl_task_ops_t *ops = APP_TSS_Ops();
	const float *model_weigth_buffer;
	model_save_weight_t *saved_wei = (model_save_weight_t *)__base_m_model;
	float score = 0.0f;
	uint32_t tick_sm;
	int status;

	if ((NULL == attr) || (NULL == ops))
	{
		PRINTF("TSS model unusable\r\n");
		return;
	}

	/*! Start from the weights embedded in the library, then prefer a stored record. */
	model_weigth_buffer = attr->model_addr;

	//load weight buffer from flash
	if ((saved_wei->magic == MODEL_MAGIC) && (attr->model_size == saved_wei->wei_len))
	{
		uint32_t crc = crc32((const uint8_t *)saved_wei->weight, saved_wei->wei_len * sizeof(float));
		if (crc == saved_wei->crc)
		{
			PRINTF("Load custom model in flash\r\n");
			model_weigth_buffer = saved_wei->weight;
		}
		else
		{
			PRINTF("Saved model CRC mismatch, load default model\r\n");
		}
	}
	else
	{
		PRINTF("Load default model\r\n");
	}

	status = ops->init(model_weigth_buffer);
	if (status != TSS_SUCCESS)
	{
		PRINTF("Tss init failed %d\r\n",status);
		return;
	}

	inference_flag = 1;
	while(1)
	{
		xQueueReceive(qh_NewSample, sensor_data, portMAX_DELAY);

		if (inference_flag)
		{
			tick_sm = xTaskGetTickCount();
			status = ops->predict(sensor_data,&score);
			if (status != TSS_SUCCESS)
			{
				PRINTF("tss predict failed %d\r\n",status);
			}
			else
			{
				PRINTF("predict %f, dur:%dms\r\n",score,xTaskGetTickCount()-tick_sm);
				//report score
				app_gui_update_score(score,attr->recommend_threshold);
			}
		}
		else if (train_flag)
		{
			if (train_cnt == 0)
				tick_sm = xTaskGetTickCount();
			status = ops->learn(sensor_data);
			if ((status != TSS_SUCCESS) && (status != TSS_LEARNING_NOT_ENOUGH) &&
			    (status != TSS_RECOMMEND_LEARNING_DONE))
			{
				PRINTF("tss learn failed %d\r\n",status);
				train_flag = 0;
				inference_flag = 1;
				continue;
			}
			train_cnt ++;
			//update train progress
			app_gui_update_progress_bar((int)((train_cnt*100) / (attr->recommend_learning_num*train_repeat_count)));
			PRINTF("Train %d times\r\n",train_cnt);

			if (train_cnt == (attr->recommend_learning_num*train_repeat_count))
			{
				train_flag = 0;
				train_cnt = 0;
				train_repeat_count = 1;
				//update train progress
				ops->export_model(model_wei.weight);
				//save weight to flash
				save_model_to_flash();
				PRINTF("Model train take %d ms\r\n",xTaskGetTickCount()-tick_sm);
			}
		}

	}
}

void algoo_stop_inference()
{
	inference_flag = 0;
}

void algoo_start_inference()
{
	inference_flag = 1;
}

void algo_start_train()
{
	int status;
	const tss_ad_odl_task_ops_t *ops;

	(void)APP_TSS_Init();
	ops = APP_TSS_Ops();
	if (NULL == ops)
	{
		return;
	}

	if (train_from_beginning)
	{
		train_from_beginning = 0;
		status = ops->init(NULL);
		if (status != TSS_SUCCESS)
		{
			PRINTF("Tss init failed %d\r\n",status);
			return;
		}
	}

	train_flag = 1;
	inference_flag = 0;
}

void algo_stop_train()
{
	train_flag = 0;
	inference_flag = 1;
}
