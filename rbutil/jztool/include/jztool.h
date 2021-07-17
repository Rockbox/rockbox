/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef JZTOOL_H
#define JZTOOL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Types, enumerations, etc
 */

#define JZ_CPUINFO_BUFLEN 9

typedef struct jz_context       jz_context;
typedef struct jz_usbdev        jz_usbdev;
typedef struct jz_device_info   jz_device_info;
typedef struct jz_cpu_info      jz_cpu_info;
typedef struct jz_buffer        jz_buffer;

typedef enum jz_error           jz_error;
typedef enum jz_identify_error  jz_identify_error;
typedef enum jz_log_level       jz_log_level;
typedef enum jz_device_type     jz_device_type;
typedef enum jz_cpu_type        jz_cpu_type;

typedef void(*jz_log_cb)(jz_log_level, const char*);

enum jz_error {
    JZ_SUCCESS = 0,
    JZ_ERR_OUT_OF_MEMORY = -1,
    JZ_ERR_OPEN_FILE = -2,
    JZ_ERR_FILE_IO = -3,
    JZ_ERR_USB = -4,
    JZ_ERR_NO_DEVICE = -5,
    JZ_ERR_BAD_FILE_FORMAT = -6,
    JZ_ERR_FLASH_ERROR = -7,
    JZ_ERR_OTHER = -99,
};

enum jz_identify_error {
    JZ_IDERR_OTHER = -1,
    JZ_IDERR_WRONG_SIZE = -2,
    JZ_IDERR_BAD_HEADER = -3,
    JZ_IDERR_BAD_CHECKSUM = -4,
    JZ_IDERR_UNRECOGNIZED_MODEL = -5,
};

enum jz_log_level {
    JZ_LOG_IGNORE  = -1,
    JZ_LOG_ERROR   = 0,
    JZ_LOG_WARNING = 1,
    JZ_LOG_NOTICE  = 2,
    JZ_LOG_DETAIL  = 3,
    JZ_LOG_DEBUG   = 4,
};

enum jz_device_type {
    JZ_DEVICE_FIIOM3K,
    JZ_DEVICE_SHANLINGQ1,
    JZ_DEVICE_EROSQ,
    JZ_NUM_DEVICES,
};

enum jz_cpu_type {
    JZ_CPU_X1000,
    JZ_NUM_CPUS,
};

struct jz_device_info {
    /* internal device name and file extension */
    const char* name;
    const char* file_ext;

    /* human-readable name */
    const char* description;

    /* device and CPU type */
    jz_device_type device_type;
    jz_cpu_type cpu_type;

    /* USB IDs of the device in mass storage mode */
    uint16_t vendor_id;
    uint16_t product_id;
};

struct jz_cpu_info {
    /* CPU info string, as reported by the boot ROM */
    const char* info_str;

    /* USB IDs of the boot ROM */
    uint16_t vendor_id;
    uint16_t product_id;

    /* default addresses for running binaries */
    uint32_t stage1_load_addr;
    uint32_t stage1_exec_addr;
    uint32_t stage2_load_addr;
    uint32_t stage2_exec_addr;
};

struct jz_buffer {
    size_t size;
    uint8_t* data;
};

/******************************************************************************
 * Library context and general functions
 */

jz_context* jz_context_create(void);
void jz_context_destroy(jz_context* jz);

void jz_context_set_user_data(jz_context* jz, void* ptr);
void* jz_context_get_user_data(jz_context* jz);

void jz_context_set_log_cb(jz_context* jz, jz_log_cb cb);
void jz_context_set_log_level(jz_context* jz, jz_log_level lev);

void jz_log(jz_context* jz, jz_log_level lev, const char* fmt, ...);
void jz_log_cb_stderr(jz_log_level lev, const char* msg);

void jz_sleepms(int ms);

/******************************************************************************
 * Device and file info
 */

const jz_device_info* jz_get_device_info(jz_device_type type);
const jz_device_info* jz_get_device_info_named(const char* name);
const jz_device_info* jz_get_device_info_indexed(int index);

const jz_cpu_info* jz_get_cpu_info(jz_cpu_type type);
const jz_cpu_info* jz_get_cpu_info_named(const char* info_str);

int jz_identify_x1000_spl(const void* data, size_t len);
int jz_identify_scramble_image(const void* data, size_t len);

/******************************************************************************
 * USB boot ROM protocol
 */

int jz_usb_open(jz_context* jz, jz_usbdev** devptr, uint16_t vend_id, uint16_t prod_id);
void jz_usb_close(jz_usbdev* dev);

int jz_usb_send(jz_usbdev* dev, uint32_t addr, size_t len, const void* data);
int jz_usb_recv(jz_usbdev* dev, uint32_t addr, size_t len, void* data);
int jz_usb_start1(jz_usbdev* dev, uint32_t addr);
int jz_usb_start2(jz_usbdev* dev, uint32_t addr);
int jz_usb_flush_caches(jz_usbdev* dev);
int jz_usb_get_cpu_info(jz_usbdev* dev, char* buffer, size_t buflen);

/******************************************************************************
 * Rockbox loader (all functions are model-specific, see docs)
 */

int jz_x1000_boot(jz_usbdev* dev, jz_device_type type, const char* filename);

/******************************************************************************
 * Buffer API and other functions
 */

jz_buffer* jz_buffer_alloc(size_t size, const void* data);
void jz_buffer_free(jz_buffer* buf);

int jz_buffer_load(jz_buffer** buf, const char* filename);
int jz_buffer_save(jz_buffer* buf, const char* filename);

jz_buffer* jz_ucl_unpack(const uint8_t* src, uint32_t src_len, uint32_t* dst_len);

/******************************************************************************
 * END
 */

#ifdef __cplusplus
}
#endif

#endif /* JZTOOL_H */
