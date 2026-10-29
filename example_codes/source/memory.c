/*
 * Copyright 2023 NXP.
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that you
 * have read, and that you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain, install, activate or
 * otherwise use the software.
 */

#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"

void *__wrap_realloc(void *p, size_t size);

void *__wrap_calloc(size_t nmemb, size_t size);

void *__wrap_malloc(size_t size);

void __wrap_free(void *p);

void *__wrap_realloc(void *p, size_t size)
{
    if (p == NULL)
    {
        return __wrap_malloc(size);
    }

    void *new_p = pvPortMalloc(size + sizeof(uint32_t));
    *(uint32_t *)new_p = size;
    new_p += 4;
    uint32_t old_size = *(uint32_t *)(p - 4);
    uint32_t copy_size = old_size < size ? old_size : size;
    memcpy(new_p, p, copy_size);
    vPortFree(p - 4);
    return new_p;
}

void *__wrap_calloc(size_t nmemb, size_t size)
{
    size_t real_size = nmemb * size;
    void *p = pvPortMalloc(real_size + sizeof(uint32_t));
    *(uint32_t *)p = real_size;
    p += 4;
    memset(p, 0, real_size);
    return p;
}

void *__wrap_malloc(size_t size)
{
    void *p = pvPortMalloc(size + sizeof(uint32_t));
    *(uint32_t *)p = size;
    return p + 4;
}

void __wrap_free(void *p)
{
    if (p == NULL)
    {
        return;
    }

    vPortFree((uint8_t *)p - 4);
}
