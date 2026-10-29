/*
 * Copyright 2023 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "framework_sensor_hal.h"
#include "framework_algorithm_hal.h"
#include "framework_sensor_manager.h"
#include "fsl_debug_console.h"
#include "TimeSeries.h"

#define SIMULATOR_TASK_NAME       "SIMULATOR"
#define SIMULATOR_TASK_STACK      1024
#define SIMULATOR_TASK_PRIORITY   9
#define SIMULATOR_HEADER_LENGTH   24
#define PK_START_REQ_DATA_LEN     72
#define PK_START_REQ_LEN          (SIMULATOR_HEADER_LENGTH + PK_START_REQ_DATA_LEN)

typedef enum _hal_transfer_packet_type_t
{
    PK_START_REQ = 1,
    PK_START_RES,
    PK_SAMPLE_REQ,
    PK_SAMPLE_RES,
    PK_END_REQ,
    PK_END_RES,
    FIRMWARE_RESPONSE = 0xff,
} hal_transfer_packet_type_t;

typedef enum _hal_simulator_packet_type_t
{
    kSIMULATOR_PACKET_Wait,
    kSIMULATOR_PACKET_Header,
    kSIMULATOR_PACKET_Data,
} hal_simulator_packet_type_t;

typedef enum _hal_simulator_status_t
{
    kSIMULATORStatus_Success,
    kSIMULATORtatus_TUMagicError,
    kSIMULATORStatus_TUCRC32Error,
    kSIMULATORStatus_PacketCRC32Error,
    kSIMULATORStatus_PacketValid,
    kSIMULATORStatus_PacketError,
    kSIMULATORStatus_PacketShort,
    kSIMULATORStatus_AlgoBusy,
} hal_simulator_status_t;

typedef struct _hal_simulator_transfer_t
{
    uint8_t *nibbleBuf;
    uint8_t *packetBuf;
    int nibbleLen;
    int dataLen;
    int readOffset;
    int packetOffset;
    uint8_t stage;
} hal_simulator_transfer_t;

typedef struct _hal_header_transfer_unit_t
{
    uint8_t tuMagic[3];
    uint8_t pktType;
    uint32_t dataLen;
    uint32_t pktId;
    uint32_t pktCrc;
    uint32_t reserved;
    uint32_t tuCrc;
} hal_header_transfer_unit_t;

static uint32_t s_ptkID;
static uint32_t s_ptkIndex;
static uint32_t table[0x100];

static const uint8_t TU_MAGIC[] = {0x53, 0x79, 0x4c};
static sensor_hal_callback_t callback;

static uint8_t s_simulator_nibble_buffer[SIMULATOR_HEADER_LENGTH];
static float *p_simulator_packet_buffer;
static float *p_buf_for_algo;
static hal_simulator_transfer_t s_simulator_transfer;
static hal_header_transfer_unit_t s_header_unit = {0};
TaskHandle_t s_simulator_handler;

static unsigned int s_simulator_pkt_cnt = 0;
static unsigned int startTime           = 0;
static unsigned int endTime             = 0;
static uint8_t *s_CDCACM_send_buf       = NULL;
static uint32_t s_CDCACM_send_buf_len   = 0;

static uint8_t *s_CDCACM_receive_buf     = NULL;
static uint32_t s_CDCACM_receive_buf_len = 0;

static volatile uint8_t data_req_flag = 0;

extern void USB_DeviceApplicationInit(void);
extern void GetCDCACMSendBufInfo(uint8_t **buf, uint32_t *size);
extern void GetCDCACMReceiveBufInfo(uint8_t **buf, uint32_t *size);
extern int8_t CDCACMSend(uint8_t *data, uint32_t size);
extern int8_t CDCACMReceive(uint8_t *buf, uint32_t size);

static uint32_t crc32_for_byte(uint32_t r)
{
    for (int j = 0; j < 8; ++j)
    {
        r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
    }

    return r ^ (uint32_t)0xFF000000L;
}

static void crc32(const void *data, size_t numBytes, uint32_t *crc)
{
    memset(table, 0x00, sizeof(table));

    if (!*table)
    {
        for (size_t i = 0; i < 0x100; ++i)
        {
            table[i] = crc32_for_byte(i);
        }
    }

    for (size_t i = 0; i < numBytes; ++i)
    {
        *crc = table[(uint8_t)*crc ^ ((uint8_t *)data)[i]] ^ *crc >> 8;
    }
}

static hal_simulator_status_t _simulator_parse_header(uint8_t *headerBuf, hal_header_transfer_unit_t *pHtUnit)
{
    hal_simulator_status_t status = kSIMULATORStatus_Success;
    uint32_t crc_res              = 0;

    /* transfer magic */
    if (memcmp(headerBuf, TU_MAGIC, sizeof(TU_MAGIC)) != 0)
    {
        return kSIMULATORtatus_TUMagicError;
    }
    memcpy(pHtUnit->tuMagic, headerBuf, 3);

    /* packet type */
    pHtUnit->pktType = headerBuf[3];

    /* packet length */
    pHtUnit->dataLen = *((uint32_t *)(headerBuf + 4));

    /* packet id */
    pHtUnit->pktId = *((uint32_t *)(headerBuf + 8));

    /* packet crc */
    pHtUnit->pktCrc = *((uint32_t *)(headerBuf + 12));

    /* packet reserved */
    pHtUnit->reserved = *((uint32_t *)(headerBuf + 16));

    /* transfer unit crc */
    pHtUnit->tuCrc = *((uint32_t *)(headerBuf + 20));
    crc32(headerBuf, SIMULATOR_HEADER_LENGTH - 4, &crc_res);
    if (crc_res != pHtUnit->tuCrc)
    {
        return kSIMULATORStatus_TUCRC32Error;
    }

    return status;
}

static void _simulator_create_header(
    uint8_t *htUnit, uint32_t crc, uint32_t len, uint8_t type, uint32_t pktId, uint32_t result)
{
    uint32_t crc_res = 0;

    /* transfer magic */
    memcpy(htUnit, TU_MAGIC, sizeof(TU_MAGIC));

    /* packet type */
    htUnit[3] = type;

    /* data length */
    *((uint32_t *)(htUnit + 4)) = len;

    /* packet id */
    *((uint32_t *)(htUnit + 8)) = pktId;

    /* packet crc */
    *((uint32_t *)(htUnit + 12)) = crc;

    /* packet reserved */
    *((uint32_t *)(htUnit + 16)) = result;

    /* transfer unit crc */
    crc_res = 0;
    crc32(htUnit, SIMULATOR_HEADER_LENGTH - 4, &crc_res);
    *((uint32_t *)(htUnit + 20)) = crc_res;
}

static hal_simulator_status_t _simulator_send_packet(
    uint8_t *data, uint32_t len, uint8_t type, uint32_t pktId, uint32_t result)
{
    hal_simulator_status_t status = kSIMULATORStatus_Success;

    //    PRINTF("_simulator_send_packet++\r\n");
    uint32_t crc = 0;

    if (len > 0)
    {
        crc32(data, len, &crc);
    }

    _simulator_create_header(s_CDCACM_send_buf, crc, len, type, pktId, result);

    status = CDCACMSend(s_CDCACM_send_buf, SIMULATOR_HEADER_LENGTH);

    //    PRINTF("_simulator_send_packet [%d]--\r\n", status);
    return status;
}

typedef enum {
    RSP_NONE = 1,
    RSP_PENDING = 2
} rsp_status_t;

typedef struct {
    uint8_t *data;
    uint32_t len;
    uint8_t type;
    uint32_t pktId;
    uint32_t result;
    rsp_status_t status;
} simulator_rsp_t;

static simulator_rsp_t rsp = {.status = RSP_NONE};

static hal_simulator_status_t _simulator_send_packet_wrapper(void)
{
    return _simulator_send_packet(rsp.data, rsp.len, rsp.type, rsp.pktId, rsp.result);
}

static hal_simulator_status_t _simulator_parse_data(uint8_t *dataBuf, hal_header_transfer_unit_t *pHtUnit)
{
    hal_simulator_status_t status = kSIMULATORStatus_Success;
    uint32_t crc_res              = 0;
    static uint32_t sample_cnt    = 0;
    static uint32_t axis_channel  = 0;
    crc32(dataBuf, pHtUnit->dataLen, &crc_res);
    if (crc_res != pHtUnit->pktCrc)
    {
        return kSIMULATORStatus_PacketCRC32Error;
    }

    s_ptkID    = pHtUnit->pktId;
    s_ptkIndex = pHtUnit->reserved;

    /* handle all kinds of packet type */
    switch (pHtUnit->pktType)
    {
        case PK_START_REQ:
        {
            startTime = 0; // Time_Current();
            PRINTF("\r\n");
            PRINTF(":DATA:CLIP_START:%s:[OK]\r\n", dataBuf);
            PRINTF("[%d]:Receive start packet [%d]\r\n", startTime, 0);
            s_simulator_pkt_cnt = 0;

            sample_cnt   = *(uint32_t *)(dataBuf + 64);
            axis_channel = *(uint32_t *)(dataBuf + 68);
        }
        break;

        case PK_SAMPLE_REQ:
        {

            // notify algorithm to process
            if (data_req_flag == 1)
            {
                s_simulator_pkt_cnt++;
                data_req_flag = 0;

                memcpy(p_buf_for_algo, dataBuf, pHtUnit->dataLen);
                sensor_payload_t *payload = pvPortMalloc(sizeof(sensor_payload_t));
                payload->sample_size      = pHtUnit->dataLen / sample_cnt;
                payload->axis_size        = axis_channel;
                payload->data             = p_buf_for_algo;
                rsp.status = RSP_PENDING;
                callback(SENSOR_EVENT_ID_DATA_READY, payload);
            }
            else
            {
                status = kSIMULATORStatus_AlgoBusy;
                PRINTF("algo device busy\r\n");
            }

        }
        break;

        case PK_END_REQ:
        {
            endTime = 1; // Time_Current();
            PRINTF("[DATA]:[CLIP_END]:[%s] [OK]\r\n", dataBuf);
            PRINTF("[%d]:[%d.%d.%d]Receive end packet [%d]\r\n", endTime, (endTime - startTime) / 1000000,
                   ((endTime - startTime) / 1000) % 1000, ((endTime - startTime) % 1000), s_simulator_pkt_cnt);
            vTaskDelay(pdMS_TO_TICKS(100));
            //            _system_reset();
        }

        break;
        default:
            break;
    }

    return status;
}

static void _simulator_handle(void *param)
{
    uint32_t curRecSize;
    hal_simulator_status_t status = kSIMULATORStatus_Success;

    while (1)
    {
        xTaskNotifyWaitIndexed(0, 0x00, 0xFFFFFFFF, &curRecSize, portMAX_DELAY);

        switch (s_simulator_transfer.stage)
        {
            case kSIMULATOR_PACKET_Header:
            {
                if (curRecSize >= s_simulator_transfer.nibbleLen)
                {
                    // copy the header
                    memcpy(s_simulator_transfer.nibbleBuf, s_CDCACM_receive_buf, s_simulator_transfer.nibbleLen);
                    status = _simulator_parse_header(s_simulator_transfer.nibbleBuf, &s_header_unit);
                    // PRINTF("HEADER_PACKET: [%d] curRecSize:%d\r\n", status, curRecSize);

                    if (status == kSIMULATORStatus_Success)
                    {
                        s_simulator_transfer.readOffset = 0;
                        s_simulator_transfer.stage      = kSIMULATOR_PACKET_Data;
                        s_simulator_transfer.dataLen    = s_header_unit.dataLen;
                        int payload_len                 = curRecSize - s_simulator_transfer.nibbleLen;
                        // PRINTF("  dataLen:%d  nibbleLen:%d payload_len:%d\r\n", s_header_unit.dataLen,
                        // s_simulator_transfer.nibbleLen, payload_len);

                        if (payload_len > 0)
                        {
                            s_simulator_transfer.packetOffset = payload_len;
                            memcpy(s_simulator_transfer.packetBuf,
                                   s_CDCACM_receive_buf + s_simulator_transfer.nibbleLen, payload_len);
                        }

                        if (curRecSize == (s_header_unit.dataLen + s_simulator_transfer.nibbleLen))
                        {
                            // PRINTF("Completed pkt[%d] received\r\n", s_header_unit.pktType);
                            rsp.data = NULL;
                            rsp.len = 0;
                            rsp.pktId = s_header_unit.pktId;
                            rsp.result = kSIMULATORStatus_Success;
                            rsp.type = PK_START_RES;
                            status = _simulator_parse_data(s_simulator_transfer.packetBuf, &s_header_unit);

                            s_simulator_transfer.stage        = kSIMULATOR_PACKET_Header;
                            s_simulator_transfer.dataLen      = 0;
                            s_simulator_transfer.packetOffset = 0;
                            if (s_header_unit.pktType != PK_SAMPLE_REQ || status != kSIMULATORStatus_Success)
                            {
                                _simulator_send_packet(NULL, 0, PK_START_RES, s_header_unit.pktId,
                                                                            status);
                            }
                        }
                        else
                        {
                            // PRINTF("Partial pkt[%d] received\r\n", s_header_unit.pktType);
                        }
                    }
                    else
                    {
                        curRecSize = 0;
                        memset(s_simulator_transfer.nibbleBuf, 0, s_simulator_transfer.nibbleLen);
                        PRINTF("HEADER PACKET PARSE FAILED[%d]\r\n", s_simulator_transfer.nibbleLen);
                        s_simulator_transfer.stage        = kSIMULATOR_PACKET_Header;
                        s_simulator_transfer.dataLen      = 0;
                        s_simulator_transfer.packetOffset = 0;
                        _simulator_send_packet(NULL, 0, PK_START_RES, -1, kSIMULATORStatus_TUCRC32Error);

                    }
                }
                else
                {
                    PRINTF("Receive len [%d] less than header len [%d]\r\n", curRecSize,
                           s_simulator_transfer.nibbleLen);
                }
            }
            break;

            case kSIMULATOR_PACKET_Data:
            {
                // PRINTF("DATA_PACKET: [%d] recvSize:%d packetOffset:%d type:%d\r\n", status, epCbParam->length,
                // s_simulator_transfer.packetOffset, s_header_unit.pktType);
                if ((curRecSize + s_simulator_transfer.packetOffset) <= s_simulator_transfer.dataLen)
                {
                    // PRINTF("Completed pkt[%d] received\r\n", s_header_unit.pktType);
                    memcpy(s_simulator_transfer.packetBuf + s_simulator_transfer.packetOffset, s_CDCACM_receive_buf,
                           curRecSize);
                    s_simulator_transfer.packetOffset += curRecSize;
                    rsp.data = NULL;
                    rsp.len = 0;
                    rsp.pktId = s_header_unit.pktId;
                    rsp.result = kSIMULATORStatus_Success;
                    rsp.type = PK_SAMPLE_RES;

                    if (s_simulator_transfer.packetOffset == s_simulator_transfer.dataLen)
                    {
                        status = _simulator_parse_data(s_simulator_transfer.packetBuf, &s_header_unit);
                        if (s_header_unit.pktType != PK_SAMPLE_REQ || status != kSIMULATORStatus_Success)
                        {
                                _simulator_send_packet(NULL, 0, PK_SAMPLE_RES, s_header_unit.pktId, kSIMULATORStatus_Success);
                        }

                        // reset the the receive state
                        s_simulator_transfer.stage        = kSIMULATOR_PACKET_Header;
                        s_simulator_transfer.dataLen      = 0;
                        s_simulator_transfer.packetOffset = 0;
                    }
                }
            }

            default:
                break;
        }

        if (curRecSize)
        {
            /* Schedule buffer for next receive event */
            //        	int retry = 0;
            while (1)
            {
                int ret = CDCACMReceive(s_CDCACM_receive_buf, s_CDCACM_receive_buf_len);

                if (ret == 0 || ret == -2)
                {
                    // PRINTF("Receiving schedule successful\r\n");
                    break;
                }
                else
                {
                    PRINTF("CDC ACM receive schedule failed");
                    while (1)
                        ; // Never reach here
                }
            }
        }
    }
}

static int32_t simulator_init(sensor_hal_callback_t cb)
{
    const tss_algo_attribute_t *algo_attr = tss_get_task_ops()->algo_attribute();
    p_buf_for_algo = pvPortMalloc(algo_attr->data_len * algo_attr->data_dim * sizeof(float));
    if (p_buf_for_algo == NULL)
    {
        PRINTF("simulator_init: malloc for algo input buffer failed\r\n");
        while (1);
    }

    unsigned int simulator_package_len = (algo_attr->data_len * algo_attr->data_dim + SIMULATOR_HEADER_LENGTH) >
                           PK_START_REQ_LEN ?
                       (algo_attr->data_len * algo_attr->data_dim + SIMULATOR_HEADER_LENGTH) : PK_START_REQ_LEN;

    p_simulator_packet_buffer = pvPortMalloc(simulator_package_len * sizeof(float));
    if (p_simulator_packet_buffer == NULL)
    {
        PRINTF("simulator_init: malloc for simulator packet buffer failed\r\n");
        return -1;
    }

    s_simulator_transfer.nibbleBuf  = s_simulator_nibble_buffer;
    s_simulator_transfer.nibbleLen  = sizeof(s_simulator_nibble_buffer);
    s_simulator_transfer.packetBuf  = (uint8_t *)p_simulator_packet_buffer;
    s_simulator_transfer.dataLen    = 0;
    s_simulator_transfer.readOffset = 0;
    s_simulator_transfer.stage      = kSIMULATOR_PACKET_Header;

    if (xTaskCreate(_simulator_handle, SIMULATOR_TASK_NAME, SIMULATOR_TASK_STACK, &s_simulator_transfer,
                    SIMULATOR_TASK_PRIORITY, &s_simulator_handler) != pdPASS)
    {
        PRINTF("[AudioProcessingOffline] Task creation failed!.");
        while (1)
            ;
    }

    USB_DeviceApplicationInit();

    callback = cb;

    GetCDCACMSendBufInfo(&s_CDCACM_send_buf, &s_CDCACM_send_buf_len);
    GetCDCACMReceiveBufInfo(&s_CDCACM_receive_buf, &s_CDCACM_receive_buf_len);

    return 0;
}

static int32_t simulator_start()
{
    return 0;
}

static int32_t simulator_notify(void *param)
{
    sensor_event_t *event = (sensor_event_t *)param;
    switch (event->id)
    {
        case SENSOR_EVENT_ID_DATA_REQUEST:
            data_req_flag = 1;
            if (rsp.status == RSP_PENDING) {
                rsp.status = RSP_NONE;
                _simulator_send_packet_wrapper();
            }
            else
            {
                PRINTF("DONNOT CONTINUE TO request data. \
                    Dataloader should have receive response with error code!!!");
            }
            break;
        default:
            break;
    }

    return 0;
}

const static sensor_hal_t sensor_hal_simulator = {
    .name   = "simulator sensor",
    .init   = simulator_init,
    .start  = simulator_start,
    .notify = simulator_notify,
};

int32_t register_sensor_hal_simulator()
{
    framework_sensor_manager_register_hal(&sensor_hal_simulator);

    return 0;
}
