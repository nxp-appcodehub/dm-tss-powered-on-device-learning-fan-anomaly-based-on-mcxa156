/*
 * Copyright 2023 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fsl_debug_console.h"

#include "framework_algorithm_hal.h"
#include "framework_output_hal.h"
#include "framework_output_manager.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_sd.h"
#include "fsl_sd_disk.h"

#include "ff.h"
#include "diskio.h"

/*! @brief Task stack size. */
#define CARDDETECT_TASK_STACK_SIZE (1024U)
/*! @brief Task stack priority. */
#define CARDDETECT_TASK_PRIORITY (configMAX_PRIORITIES - 1U)

#define SD_FILE_ACCESS_TASK_STACK_SIZE (1024U)
#define SD_FILE_ACCESS_TASK_PRIORITY (configMAX_PRIORITIES - 1U)

#define SD_INFO_DIR "/sd_info"
#define SD_INFO_PATH "/sd_info/info.bin"

typedef struct _msg_t
{
    int32_t index;
    int32_t size;
    void*   data;
} msg_t;

static const TCHAR driverNumberBuffer[3U] = {SDDISK + '0', ':', '/'};

static FATFS g_fileSystem; /* File system object */
/*! @brief SD card detect flag  */
static volatile bool s_cardInserted     = false;
static volatile bool s_cardInsertStatus = false;

/*! @brief Card semaphore  */
static SemaphoreHandle_t s_cardDetectSemaphore = NULL;
static SemaphoreHandle_t s_lock = NULL;
static QueueHandle_t      s_msgQueue;

#define FILE_WRITE_SIZE 8*1024
static uint8_t fileBufWrite[2][FILE_WRITE_SIZE];
static int32_t fileBufOffset = 0;
static uint8_t filePingPong = 0;
static int32_t fileAccessCnt = 0;

static void _sdcard_detect_callBack(bool isInserted, void *userData)
{
    s_cardInsertStatus = isInserted;
    xSemaphoreGiveFromISR(s_cardDetectSemaphore, NULL);
}

static int _lock(SemaphoreHandle_t lock)
{
    if (lock == NULL)
    {
        return -1;
    }

    if (pdTRUE != xSemaphoreTake(lock, portMAX_DELAY))
    {
        return -1;
    }

    return 0;
}

static int _unlock(SemaphoreHandle_t lock)
{
    if (lock == NULL)
    {
        return -1;
    }

    if (pdTRUE != xSemaphoreGive(lock))
    {
        return -1;
    }

    return 0;
}

static status_t _make_fatfs()
{
    FRESULT error;
    BYTE work[FF_MAX_SS];

    _lock(s_lock);

    if (f_mount(&g_fileSystem, driverNumberBuffer, 0U))
    {
        PRINTF("Mount volume failed.\r\n");
        return kStatus_Fail;
    }

#if (FF_FS_RPATH >= 2U)
    error = f_chdrive((char const *)&driverNumberBuffer[0U]);
    if (error)
    {
        PRINTF("Change drive failed.\r\n");
        return kStatus_Fail;
    }
#endif

#if FF_USE_MKFS
    PRINTF("\r\nMake file system......The time may be long if the card capacity is big.\r\n");
    if (f_mkfs(driverNumberBuffer, 0, work, sizeof work))
    {
        PRINTF("Make file system failed.\r\n");
        return kStatus_Fail;
    }

    PRINTF("\r\nMake file system finished.\r\n");
#endif /* FF_USE_MKFS */

    error = f_mkdir(_T(SD_INFO_DIR));
    if (error)
    {
        if (error == FR_EXIST)
        {
            PRINTF("Directory exists.\r\n");
        }
        else
        {
            PRINTF("Make directory failed.\r\n");
            return kStatus_Fail;
        }
    }

    _unlock(s_lock);

    return kStatus_Success;
}

static void _card_detect_task(void *pvParameters)
{
    s_cardDetectSemaphore = xSemaphoreCreateBinary();

    BOARD_SD_Config(&g_sd, _sdcard_detect_callBack, NULL);

    /* SD host init function */
    if (SD_HostInit(&g_sd) == kStatus_Success)
    {
        while (true)
        {
            /* take card detect semaphore */
            if (xSemaphoreTake(s_cardDetectSemaphore, portMAX_DELAY) == pdTRUE)
            {
                if (s_cardInserted != s_cardInsertStatus)
                {
                    s_cardInserted = s_cardInsertStatus;

                    if (s_cardInserted)
                    {
                        PRINTF("\r\nCard inserted.\r\n");
                        /* power off card */
                        SD_SetCardPower(&g_sd, false);
                        /* power on the card */
                        SD_SetCardPower(&g_sd, true);
                        /* make file system */
                        if (_make_fatfs() != kStatus_Success)
                        {
                            continue;
                        }
                    }
                }

                if (!s_cardInserted)
                {
                    PRINTF("\r\nPlease insert a card into board.\r\n");
                }
            }
        }
    }
    else
    {
        PRINTF("\r\nSD host init fail\r\n");
    }

    vTaskSuspend(NULL);
}

static void fatfs_sd_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (s_lock == NULL)
    {
        PRINTF("Create fatfs lock failed.\r\n");
    }

    if (pdPASS !=
        xTaskCreate(_card_detect_task, "_card_detect_task", CARDDETECT_TASK_STACK_SIZE, NULL, CARDDETECT_TASK_PRIORITY, NULL))
    {
        PRINTF("Create card detect task failed.\r\n");
    }
}

static int32_t fatfs_sd_mkdir(const char *path)
{
    int32_t ret = 0;

    _lock(s_lock);

    ret = f_mkdir(_T(path));
    if (ret)
    {
        if (ret == FR_EXIST)
        {
            PRINTF("Directory exists.\r\n");
        }
        else
        {
            PRINTF("Make directory failed.\r\n");
            return kStatus_Fail;
        }
    }

    _unlock(s_lock);

    return ret;
}

static int32_t fatfs_sd_append(const char *path, void *buf, unsigned int size, bool overwrite)
{
    int32_t ret = 0;
    FIL fileObj;
    UINT bw = 0;

    _lock(s_lock);

    ret = f_open(&fileObj, _T(path), FA_WRITE);
    if (ret)
    {
        if (ret == FR_EXIST)
        {
            PRINTF("File exists.\r\n");
        }
        /* if file not exist, creat a new file */
        else if (ret == FR_NO_FILE)
        {
            if (f_open(&fileObj, _T(path), (FA_WRITE | FA_CREATE_NEW)) != FR_OK)
            {
                PRINTF("Create file failed.\r\n");
                return -1;
            }
        }
        else
        {
            PRINTF("Open file failed.\r\n");
            return -1;
        }
    }


    if (!overwrite)
    {
        if (f_lseek(&fileObj, fileObj.obj.objsize) != FR_OK)
        {
            PRINTF("lseek file failed.\r\n");
            return -1;
        }
    }

    ret = f_write(&fileObj, buf, size, &bw);
    if ((ret) || (bw != size))
    {
        PRINTF("Write file failed.\r\n");
        return -1;
    }

    f_close(&fileObj);

    _unlock(s_lock);

    return ret;
}

static void _sd_fatfs_file_access(void *pvParameters)
{
    msg_t msg = {0, 0, 0};

    fatfs_sd_init();

    while (1)
    {
        xQueueReceive(s_msgQueue, (void *)&msg, portMAX_DELAY);

        if (s_cardInserted)
        {
            fatfs_sd_append(SD_INFO_PATH, msg.data, msg.size, 0);

            PRINTF("\r\nSD card %s[%d] append %d.\r\n\r\n", SD_INFO_PATH, msg.index, msg.size);
        }
        else
        {
            PRINTF("\r\nPlease insert a card into board.\r\n");
        }

        if (msg.data != NULL)
        {
            msg.size = 0;
            msg.data = NULL;
        }
    }

    vTaskSuspend(NULL);
}

static int32_t sd_fatfs_init()
{
    int32_t ret = 0;

    s_msgQueue = xQueueCreate(10, sizeof(msg_t));
    if (s_msgQueue == NULL)
    {
        PRINTF("Create sd info msg queue failed. \r\n");
        return -1;
    }

    if (pdPASS != xTaskCreate(_sd_fatfs_file_access, "_sd_fatfs_file_access", SD_FILE_ACCESS_TASK_STACK_SIZE, NULL, SD_FILE_ACCESS_TASK_PRIORITY, NULL))
    {
        return -1;
    }

    return ret;
}

static int32_t sd_fatfs_notify(void *param)
{
    algorithm_result_t *result = (algorithm_result_t *)param;

    msg_t msg = {0, 0, 0};

    switch (result->state)
    {
        case ALGORITHM_STATE_LOGGING:
        {
            switch (result->id)
            {
                case ALGORITHM_RESULT_ID_SUCCESS:
                {
                    if (fileBufOffset + result->size < FILE_WRITE_SIZE)
                    {
                        memcpy(fileBufWrite[filePingPong] + fileBufOffset, result->data, result->size);
                        fileBufOffset += result->size;
                    }
                    else
                    {
                        msg.size = fileBufOffset;
                        msg.data = fileBufWrite[filePingPong];

                        fileBufOffset = 0;
                        filePingPong ^= 1;
                        memcpy(fileBufWrite[filePingPong] + fileBufOffset, result->data, result->size);
                        fileBufOffset += result->size;
                    }

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
            break;
        }
    }

    if (msg.size > 0)
    {
        msg.index = fileAccessCnt++;
        if (xQueueSend(s_msgQueue, (void *)&msg, (TickType_t)0) != pdPASS)
        {
            PRINTF("%s: xQueueSend failed!\r\n", __func__);
            return -1;
        }

        msg.size = 0;
        msg.data = NULL;
    }

    return 0;
}

const static output_hal_t output_hal_sd_fatfs =
{
    .name = "sdcard fatfs",
    .init = sd_fatfs_init,
    .start = NULL,
    .notify = sd_fatfs_notify,
};

int32_t register_output_hal_sd_fatfs()
{
    framework_output_manager_register_hal(&output_hal_sd_fatfs);

    return 0;
}
