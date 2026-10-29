/*
 * Copyright 2023, 2025 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdlib.h>
#include <string.h>

#include "fsl_debug_console.h"
#include "fsl_component_serial_manager.h"
#include "fsl_shell.h"

#include "framework_input_hal.h"
#include "framework_sensor_hal.h"
#include "framework_algorithm_hal.h"
#include "framework_input_manager.h"

#define EXECUTION_PARAMETER_LOGGER                            "log"
#define EXECUTION_PARAMETER_LEARNING                          "learn"
#define EXECUTION_PARAMETER_RESET_MODEL                       "reset"
#define EXECUTION_PARAMETER_PREDICTION                        "pre"
#define SETUP_PARAMETER_OUTPUT_DATA_RATE                      "odr"
#define SETUP_PARAMETER_FULL_SCALE_RANGE                      "fsr"
#define SETUP_PARAMETER_SAMPLE_COUNT                          "sc"
#define SETUP_PARAMETER_PREDICTION_THRESHOLD                  "thold"
#define SETUP_PARAMETER_MULTI_SENSOR                          "sensor"
#define GET_PARAMETER_SYSTEM_INFO                             "info"
#define GET_PARAMETER_TSS_STATE                              "state"
#define GET_PARAMETER_SENSOR_CONFIG_RANGE                     "range"
#define GET_PARAMETER_SENSOR_MULTI_SENSOR                     "sensors"

static input_hal_callback_t callback;

SDK_ALIGN(static uint8_t s_shellHandleBuffer[SHELL_HANDLE_SIZE], 4);

static shell_handle_t s_shellHandle;

extern serial_handle_t g_serialHandle;

static shell_status_t SHELL_StartCommand(shell_handle_t shellHandle, int32_t argc, char **argv);

static shell_status_t SHELL_StopCommand(shell_handle_t shellHandle, int32_t argc, char **argv);

static shell_status_t SHELL_SetupCommand(shell_handle_t shellHandle, int32_t argc, char **argv);

static shell_status_t SHELL_LoadLibCommand(shell_handle_t shellHandle, int32_t argc, char **argv);

static shell_status_t SHELL_GetCommand(shell_handle_t shellHandle, int32_t argc, char **argv);

static SHELL_COMMAND_DEFINE(start,
                            "\r\n\"start <" \
                            EXECUTION_PARAMETER_LOGGER "|" \
                            EXECUTION_PARAMETER_LEARNING "|" \
                            EXECUTION_PARAMETER_PREDICTION \
                            ">: Start an execution\r\n" \
                            " - " EXECUTION_PARAMETER_LOGGER        ": Start data logger\r\n" \
                            " - " EXECUTION_PARAMETER_LEARNING      ": Start Time Series learning\r\n" \
                            " - " EXECUTION_PARAMETER_RESET_MODEL   ": Start Time Series knowledge reset\r\n" \
                            " - " EXECUTION_PARAMETER_PREDICTION    ": Start Time Series prediction\r\n",
                            SHELL_StartCommand,
                            1);

static SHELL_COMMAND_DEFINE(stop,
                            "\r\n\"stop\": Stop ongoing execution\r\n",
                            SHELL_StopCommand,
                            0);

static SHELL_COMMAND_DEFINE(setup,
                            "\r\n\"setup <" \
                            SETUP_PARAMETER_OUTPUT_DATA_RATE "|" \
                            SETUP_PARAMETER_FULL_SCALE_RANGE "|" \
                            SETUP_PARAMETER_SAMPLE_COUNT "|" \
                            SETUP_PARAMETER_MULTI_SENSOR "|" \
                            SETUP_PARAMETER_PREDICTION_THRESHOLD \
                            "> <value>: Setup a parameter\r\n" \
                            " - " SETUP_PARAMETER_OUTPUT_DATA_RATE      ": Setup output data rate\r\n" \
                            " - " SETUP_PARAMETER_FULL_SCALE_RANGE      ": Setup full scale range\r\n" \
                            " - " SETUP_PARAMETER_SAMPLE_COUNT          ": Setup sample count\r\n" \
                            " - " SETUP_PARAMETER_MULTI_SENSOR			": Setup target sensor with ID NO.(listed by 'get sensors' command)\r\n" \
                            " - " SETUP_PARAMETER_PREDICTION_THRESHOLD  ": Setup prediction threshold (Anomaly Detection Only)\r\n",
                            SHELL_SetupCommand,
                            2);

static SHELL_COMMAND_DEFINE(loadlib,
                            "\r\n\"loadlib <load|alloc> <allocsize | line>:"
                            " dynamic load a lib to allocated memory\r\n"
                            " - alloc size: allocate memory of size bytes\r\n"
                            " - load line: load one line of lib to memory\r\n",
                            SHELL_LoadLibCommand,
                            2);

static SHELL_COMMAND_DEFINE(get,
                            "\r\n\"get <" \
                            GET_PARAMETER_SYSTEM_INFO "|" \
                            GET_PARAMETER_TSS_STATE "|" \
                            GET_PARAMETER_SENSOR_CONFIG_RANGE "|" \
                            GET_PARAMETER_SENSOR_MULTI_SENSOR \
                            ">: Get board information\r\n" \
                            " - " GET_PARAMETER_SYSTEM_INFO            ": Get system information\r\n" \
                            " - " GET_PARAMETER_TSS_STATE              ": Get current Time Series state\r\n"  \
                            " - " GET_PARAMETER_SENSOR_CONFIG_RANGE    ": Get parameter range\r\n" \
                            " - " GET_PARAMETER_SENSOR_MULTI_SENSOR	   ": Get supported multi sensors\r\n",
                            SHELL_GetCommand,
                            1);

static shell_status_t SHELL_StartCommand(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    if (strcmp(argv[1], EXECUTION_PARAMETER_LOGGER) == 0)
    {
        algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
        event->id = ALGORITHM_EVENT_ID_START_DATA_LOGGER;
        callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
    }
    else if (strcmp(argv[1], EXECUTION_PARAMETER_LEARNING) == 0)
    {
        algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
        event->id = ALGORITHM_EVENT_ID_START_TSS_LEARNING;
        callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
    }
    else if (strcmp(argv[1], EXECUTION_PARAMETER_RESET_MODEL) == 0)
    {
        algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
        event->id = ALGORITHM_EVENT_ID_RESET_TSS_MODEL;
        callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
    }
    else if (strcmp(argv[1], EXECUTION_PARAMETER_PREDICTION) == 0)
    {
        algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
        event->id = ALGORITHM_EVENT_ID_START_TSS_PREDICTION;
        callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
    }
    else
    {
        SHELL_Printf(shellHandle, "Incorrect parameter!\r\n");
        return kStatus_SHELL_Error;
    }

    return kStatus_SHELL_Success;
}

static shell_status_t SHELL_StopCommand(shell_handle_t shellHandle, int32_t argc, char **argv)
{

    algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
    event->id = ALGORITHM_EVENT_ID_STOP;
    callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
    return kStatus_SHELL_Success;
}

static shell_status_t SHELL_SetupCommand(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    if (strcmp(argv[1], SETUP_PARAMETER_OUTPUT_DATA_RATE) == 0)
    {
        sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
        event->id = SENSOR_EVENT_ID_SETUP_OUTPUT_DATA_RATE;
        event->value = atoi(argv[2]);
        callback(FRAMEWORK_COMPONENT_ID_SENSOR, event);
    }
    else if (strcmp(argv[1], SETUP_PARAMETER_FULL_SCALE_RANGE) == 0)
    {
        sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
        event->id = SENSOR_EVENT_ID_SETUP_FULL_SCALE_RANGE;
        event->value = atoi(argv[2]);
        callback(FRAMEWORK_COMPONENT_ID_SENSOR, event);
    }
    else if (strcmp(argv[1], SETUP_PARAMETER_SAMPLE_COUNT) == 0)
    {
        sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
        event->id = SENSOR_EVENT_ID_SETUP_SAMPLE_COUNT;
        event->value = atoi(argv[2]);
        callback(FRAMEWORK_COMPONENT_ID_SENSOR, event);
    }
    else if (strcmp(argv[1], SETUP_PARAMETER_MULTI_SENSOR) == 0)
    {
        sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
        event->id = SENSOR_EVENT_ID_SETUP_MULTI_SENSOR;
        event->value = atoi(argv[2]);
        if (event->value >= MAX_SENSOR_NUM )
        {
            SHELL_Printf(shellHandle, "Incorrect parameter!\r\n");
            return kStatus_SHELL_Error;
        }
        callback(FRAMEWORK_COMPONENT_ID_SENSOR, event);
    }
    else if (strcmp(argv[1], SETUP_PARAMETER_PREDICTION_THRESHOLD) == 0)
    {
        algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
        event->id = ALGORITHM_EVENT_ID_SETUP_PREDICTION_THRESHOLD;
        event->value = atof(argv[2]);
        callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
    }
    else
    {
        SHELL_Printf(shellHandle, "Incorrect parameter!\r\n");
        return kStatus_SHELL_Error;
    }
    return kStatus_SHELL_Success;
}

static shell_status_t SHELL_GetCommand(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    if (strcmp(argv[1], GET_PARAMETER_SYSTEM_INFO) == 0)
    {
        sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
        event->id = SENSOR_EVENT_ID_GET_SYSTEM_INFO;
        callback(FRAMEWORK_COMPONENT_ID_SENSOR, event);
    }
    else if (strcmp(argv[1], GET_PARAMETER_TSS_STATE) == 0)
    {
        sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
        event->id = ALGORITHM_EVENT_ID_GET_TSS_STATE;
        callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
    }
    else if (strcmp(argv[1], GET_PARAMETER_SENSOR_CONFIG_RANGE) == 0)
    {
        sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
        event->id = SENSOR_EVENT_ID_GET_CONFIG_RANGE;
        callback(FRAMEWORK_COMPONENT_ID_SENSOR, event);
    }
    else if (strcmp(argv[1], GET_PARAMETER_SENSOR_MULTI_SENSOR) == 0)
    {
        sensor_event_t *event = pvPortMalloc(sizeof(sensor_event_t));
        event->id = SENSOR_EVENT_ID_GET_MULTI_SENSOR;
        callback(FRAMEWORK_COMPONENT_ID_SENSOR, event);
    }
    else
    {
        SHELL_Printf(shellHandle, "Incorrect parameter!\r\n");
        return kStatus_SHELL_Error;
    }
    return kStatus_SHELL_Success;
}

#define IHEX_TYPE_DATA              0
#define IHEX_TYPE_EOF               1
#define IHEX_TYPE_EXT_LINEAR_ADDR   4
#define IHEX_TYPE_START_ADDR        5

static int one_digit(const char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    else if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }

    return -1;
}

static int one_byte(const char **str, int *sum)
{
    const char *s = *str;
    int high = one_digit(s[0]);
    int low = one_digit(s[1]);

    // advance two bytes in the record
    *str = s + 2;

    if (high < 0 || low < 0)
    {
        return -1;
    }

    int data = high << 4 | low;
    if (sum)
    {
        *sum += data;
    }
    return data;
}

static bool match_checksum(const char *s, int sum)
{
    int checksum = one_byte(&s, NULL);

    if ((uint8_t)(~sum + 1) != checksum)
    {
        return false;
    }

    return true;
}

static void start_dynamic_load(uint32_t entry_addr)
{
    algorithm_event_t *event = pvPortMalloc(sizeof(algorithm_event_t));
    event->id = ALGORITHM_EVENT_ID_DYNAMIC_LOAD;
    event->lib_entry = entry_addr;

    callback(FRAMEWORK_COMPONENT_ID_ALGORITHM, event);
}

static shell_status_t SHELL_LoadLibCommand(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    static uint32_t addr_high = -1; // high 16bit linear address
    static uint32_t entry_addr = 0; // the lib entry address
    static void *lib_mem;
    const char *s;

    if (strncmp(argv[1], "load", sizeof("load")) == 0)
    {
        s = argv[2];

        if (s[0] == '0')
        {
            // load the default algorithm lib
            start_dynamic_load(0);
            return kStatus_SHELL_Success;
        }

        if (s[0] == ':')
        {
            uint8_t len, type;
            int sum = 0;
            uint32_t addr_low = 0;

            s++;
            len = one_byte(&s, &sum);
            addr_low = one_byte(&s, &sum);
            addr_low = addr_low << 8 | one_byte(&s, &sum);
            type = one_byte(&s, &sum);

            switch (type)
            {
                case IHEX_TYPE_EXT_LINEAR_ADDR:
                {
                    addr_high = one_byte(&s, &sum);
                    addr_high = addr_high << 24 | one_byte(&s, &sum) << 16;

                    if (!match_checksum(s, sum))
                    {
                        // wrong checksum
                        addr_high = -1;
                    }
                    break;
                }

                case IHEX_TYPE_DATA:
                {
                    uint8_t data[32];

                    if (addr_high == -1)
                    {
                        // haven't received the EXT_LINEAR_ADDR record
                        return kStatus_SHELL_Error;
                    }

                    for (int i = 0; i < len; i++)
                    {
                        data[i] = one_byte(&s, &sum);
                    }

                    if (match_checksum(s, sum))
                    {
                        uint32_t addr = addr_high | addr_low;
                        memcpy((void *)addr, data, len);
                    }
                    break;
                }

                case IHEX_TYPE_START_ADDR:
                {
                    for (int i = 0; i < len; i++)
                    {
                        entry_addr = entry_addr << 8 | one_byte(&s, &sum);
                    }

                    if (!match_checksum(s, sum))
                    {
                        entry_addr = 0;
                    }
                    break;
                }

                case IHEX_TYPE_EOF:
                {
                    if (match_checksum(s, sum))
                    {
                        // receive EOF
                        // notify algorithm device to switch lib
                        start_dynamic_load(entry_addr);
                    }

                    // reset to default state, waiting for next hex file
                    addr_high = -1;
                    entry_addr = 0;
                    break;
                }

                default:
                    return kStatus_SHELL_Error;
            }

            return kStatus_SHELL_Success;
        }
    }
    else if (strncmp(argv[1], "alloc", sizeof("alloc")) == 0)
    {
        unsigned size = strtoul(argv[2], NULL, 16);

        if (size <= 128 * 1024)
        {
            vPortFree(lib_mem);
            lib_mem = pvPortMalloc(size);
            if (lib_mem)
            {
                memset(lib_mem, 0, size);
            }
        }

        SHELL_Printf(shellHandle, "alloc at 0x%p\r\n", lib_mem);
        return kStatus_SHELL_Success;
    }

    return kStatus_SHELL_Error;
}

static int32_t shell_uart_init(input_hal_callback_t cb)
{
    callback = cb;

    s_shellHandle = &s_shellHandleBuffer[0];

    DbgConsole_Flush();

    SHELL_Init(s_shellHandle, g_serialHandle, "SHELL>> ");

    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(start));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(stop));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(setup));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(loadlib));
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(get));

    return 0;
}

const static input_hal_t input_hal_shell_uart =
{
    .name = "Shell UART",
    .init = shell_uart_init,
    .start = NULL,
};

int32_t register_input_hal_shell_uart()
{
    framework_input_manager_register_hal(&input_hal_shell_uart);

    return 0;
}
