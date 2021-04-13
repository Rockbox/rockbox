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

#include "jztool.h"
#include <string.h>

#define IMAGE_ADDR  0
#define IMAGE_SIZE  (128 * 1024)
#define SPL_OFFSET  0
#define SPL_SIZE    (12 * 1024)
#define BOOT_OFFSET (26 * 1024)
#define BOOT_SIZE   (102 * 1024)

int jz_fiiom3k_readboot(jz_usbdev* dev, jz_buffer** bufptr)
{
    jz_buffer* buf = jz_buffer_alloc(IMAGE_SIZE, NULL);
    if(!buf)
        return JZ_ERR_OUT_OF_MEMORY;

    int rc = jz_x1000_read_flash(dev, IMAGE_ADDR, buf->size, buf->data);
    if(rc < 0) {
        jz_buffer_free(buf);
        return rc;
    }

    *bufptr = buf;
    return JZ_SUCCESS;
}

int jz_fiiom3k_writeboot(jz_usbdev* dev, size_t image_size, const void* image_buf)
{
    int rc = jz_identify_fiiom3k_bootimage(image_buf, image_size);
    if(rc < 0 || image_size != IMAGE_SIZE)
        return JZ_ERR_BAD_FILE_FORMAT;

    rc = jz_x1000_write_flash(dev, IMAGE_ADDR, image_size, image_buf);
    if(rc < 0)
        return rc;

    return JZ_SUCCESS;
}

int jz_fiiom3k_patchboot(jz_context* jz, void* image_buf, size_t image_size,
                         const void* spl_buf, size_t spl_size,
                         const void* boot_buf, size_t boot_size)
{
    int rc = jz_identify_fiiom3k_bootimage(image_buf, image_size);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "Boot image is invalid: %d", rc);
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    rc = jz_identify_x1000_spl(spl_buf, spl_size);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "SPL image is invalid: %d", rc);
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    if(spl_size > SPL_SIZE) {
        jz_log(jz, JZ_LOG_ERROR, "SPL is too big");
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    rc = jz_identify_scramble_image(boot_buf, boot_size);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "Bootloader image is invalid: %d", rc);
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    if(boot_size > BOOT_SIZE) {
        jz_log(jz, JZ_LOG_ERROR, "Bootloader is too big");
        return JZ_ERR_BAD_FILE_FORMAT;
    }

    uint8_t* imgdat = (uint8_t*)image_buf;
    memset(&imgdat[SPL_OFFSET], 0xff, SPL_SIZE);
    memcpy(&imgdat[SPL_OFFSET], spl_buf, spl_size);
    memset(&imgdat[BOOT_OFFSET], 0xff, BOOT_SIZE);
    memcpy(&imgdat[BOOT_OFFSET], boot_buf, boot_size);
    return JZ_SUCCESS;
}

#define IMGBUF  0
#define SPLBUF  1
#define BOOTBUF 2
#define NUMBUFS 3
#define IMGBUF_NAME  "image"
#define SPLBUF_NAME  "spl"
#define BOOTBUF_NAME "bootloader"
#define FIIOM3K_INIT_WORKSTATE {0}

struct fiiom3k_workstate {
    jz_usbdev* dev;
    jz_buffer* bufs[NUMBUFS];
};

static void fiiom3k_action_cleanup(struct fiiom3k_workstate* state)
{
    for(int i = 0; i < NUMBUFS; ++i)
        if(state->bufs[i])
            jz_buffer_free(state->bufs[i]);

    if(state->dev)
        jz_usb_close(state->dev);
}

static int fiiom3k_action_loadbuf(jz_context* jz, jz_paramlist* pl,
                                  struct fiiom3k_workstate* state, int idx)
{
    const char* const paramnames[] = {IMGBUF_NAME, SPLBUF_NAME, BOOTBUF_NAME};

    if(state->bufs[idx])
        return JZ_SUCCESS;

    const char* filename = jz_paramlist_get(pl, paramnames[idx]);
    if(!filename) {
        jz_log(jz, JZ_LOG_ERROR, "Missing required parameter '%s'", paramnames[idx]);
        return JZ_ERR_OTHER;
    }

    int rc = jz_buffer_load(&state->bufs[idx], filename);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "Error reading '%s' file (%d): %s", paramnames[idx], rc, filename);
        return rc;
    }

    return JZ_SUCCESS;
}

static int fiiom3k_action_setup(jz_context* jz, jz_paramlist* pl,
                                struct fiiom3k_workstate* state)
{
    const jz_device_info* info = jz_get_device_info(JZ_DEVICE_FIIOM3K);
    if(!info)
        return JZ_ERR_OTHER;

    int rc = fiiom3k_action_loadbuf(jz, pl, state, SPLBUF);
    if(rc < 0)
        return rc;

    jz_log(jz, JZ_LOG_DETAIL, "Open USB device %04x:%04x",
           (unsigned int)info->vendor_id, (unsigned int)info->product_id);
    rc = jz_usb_open(jz, &state->dev, info->vendor_id, info->product_id);
    if(rc < 0)
        return rc;

    jz_log(jz, JZ_LOG_DETAIL, "Setup device for flash access");
    jz_buffer* splbuf = state->bufs[SPLBUF];
    return jz_x1000_setup(state->dev, splbuf->size, splbuf->data);
}

int jz_fiiom3k_install(jz_context* jz, jz_paramlist* pl)
{
    struct fiiom3k_workstate state = FIIOM3K_INIT_WORKSTATE;
    int rc;

    rc = fiiom3k_action_loadbuf(jz, pl, &state, BOOTBUF);
    if(rc < 0)
        goto error;

    rc = fiiom3k_action_setup(jz, pl, &state);
    if(rc < 0)
        goto error;

    jz_log(jz, JZ_LOG_DETAIL, "Reading boot image from device");
    rc = jz_fiiom3k_readboot(state.dev, &state.bufs[IMGBUF]);
    if(rc < 0)
        goto error;

    jz_buffer* img_buf = state.bufs[IMGBUF];
    const char* backupfile = jz_paramlist_get(pl, "backup");
    const char* without_backup = jz_paramlist_get(pl, "without-backup");
    if(backupfile) {
        jz_log(jz, JZ_LOG_DETAIL, "Backup original boot image to file: %s", backupfile);
        rc = jz_buffer_save(img_buf, backupfile);
        if(rc < 0) {
            jz_log(jz, JZ_LOG_ERROR, "Error saving backup image file (%d): %s", rc, backupfile);
            goto error;
        }
    } else if(!without_backup || strcmp(without_backup, "yes")) {
        jz_log(jz, JZ_LOG_ERROR, "No --backup option given and --without-backup yes not specified");
        jz_log(jz, JZ_LOG_ERROR, "Refusing to flash a new bootloader without taking a backup");
        goto error;
    }

    jz_log(jz, JZ_LOG_DETAIL, "Patching image with new SPL/bootloader");
    jz_buffer* boot_buf = state.bufs[BOOTBUF];
    jz_buffer* spl_buf = state.bufs[SPLBUF];
    rc = jz_fiiom3k_patchboot(jz, img_buf->data, img_buf->size,
                              spl_buf->data, spl_buf->size,
                              boot_buf->data, boot_buf->size);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "Error patching image: %d", rc);
        goto error;
    }

    jz_log(jz, JZ_LOG_DETAIL, "Writing patched image to device");
    rc = jz_fiiom3k_writeboot(state.dev, img_buf->size, img_buf->data);
    if(rc < 0)
        goto error;

    rc = JZ_SUCCESS;

  error:
    fiiom3k_action_cleanup(&state);
    return rc;
}

int jz_fiiom3k_backup(jz_context* jz, jz_paramlist* pl)
{
    struct fiiom3k_workstate state = FIIOM3K_INIT_WORKSTATE;
    int rc;

    const char* outfile_path = jz_paramlist_get(pl, IMGBUF_NAME);
    if(!outfile_path) {
        jz_log(jz, JZ_LOG_ERROR, "Missing required parameter '%s'", IMGBUF_NAME);
        rc = JZ_ERR_OTHER;
        goto error;
    }

    rc = fiiom3k_action_setup(jz, pl, &state);
    if(rc < 0)
        goto error;

    rc = jz_fiiom3k_readboot(state.dev, &state.bufs[IMGBUF]);
    if(rc < 0)
        goto error;

    rc = jz_buffer_save(state.bufs[IMGBUF], outfile_path);
    if(rc < 0) {
        jz_log(jz, JZ_LOG_ERROR, "Error writing '%s' file (%d): %s", IMGBUF_NAME, rc, outfile_path);
        goto error;
    }

    rc = JZ_SUCCESS;

  error:
    fiiom3k_action_cleanup(&state);
    return rc;
}

int jz_fiiom3k_restore(jz_context* jz, jz_paramlist* pl)
{
    struct fiiom3k_workstate state = FIIOM3K_INIT_WORKSTATE;
    int rc;

    rc = fiiom3k_action_loadbuf(jz, pl, &state, IMGBUF);
    if(rc < 0)
        goto error;

    rc = fiiom3k_action_setup(jz, pl, &state);
    if(rc < 0)
        goto error;

    jz_buffer* img_buf = state.bufs[IMGBUF];
    rc = jz_fiiom3k_writeboot(state.dev, img_buf->size, img_buf->data);
    if(rc < 0)
        goto error;

    rc = JZ_SUCCESS;

  error:
    fiiom3k_action_cleanup(&state);
    return rc;
}
