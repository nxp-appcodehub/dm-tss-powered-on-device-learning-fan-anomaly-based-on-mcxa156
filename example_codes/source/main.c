/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"

#include "framework_sensor_manager.h"
#include "framework_input_manager.h"
#include "framework_output_manager.h"
#include "framework_algorithm_manager.h"
#include "lptmr.h"

const char *firmware_version = "12.9.2025";
#define MCUXSDK_VERSION_FULL_STR  "2.16.100"

static int32_t create_framework_managers()
{
    framework_input_manager_create();
    framework_output_manager_create();
    framework_sensor_manager_create();
    framework_algorithm_manager_create();

    return 0;
}

static void system_task(void *param)
{
    extern int32_t register_hal_devices();
    register_hal_devices();

    create_framework_managers();

    vTaskDelete(NULL);
}

int main(void)
{
    extern int32_t hardware_init();
    hardware_init();
    LPTMR_init();

    if (xTaskCreate(system_task, "System Task", configMINIMAL_STACK_SIZE + 1000, NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        PRINTF("%s: xTaskCreate failed!\r\n", __func__);
        return -1;
    }

    PRINTF("\r\nWelcome to Time Series Studio Sample Application (%s %s)\r\n\r\n", __DATE__, __TIME__);
    PRINTF("MCUXpresso SDK version: %s\r\n", MCUXSDK_VERSION_FULL_STR);

    vTaskStartScheduler();

    return 0 ;
}
