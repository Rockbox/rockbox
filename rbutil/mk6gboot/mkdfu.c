/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) Cástor Muñoz
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "mkdfu.h"

/* Header for ARM code binaries */
#include "dualboot.h"

/* Win32 compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif

const struct ipod_models ipod_identity[] = {
    [MODEL_IPOD6G] = {
        "Classic 6G", "ip6g", 71,
        dualboot_install_ipod6g,   sizeof(dualboot_install_ipod6g),
        dualboot_uninstall_ipod6g, sizeof(dualboot_uninstall_ipod6g) },
};

struct Im3Info s5l8702hdr =
{
    .ident          = IM3_IDENT,
    .version        = IM3_VERSION,
    .enc_type       = 3,
    .enc34.sign_off = "\x00\x03\x00\x00",
    .enc34.cert_off = "\x50\xF8\xFF\xFF",  /* -0x800 + CERT_OFFSET */
    .enc34.cert_sz  = "\xBA\x02\x00\x00",  /* CERT_SIZE */
    .info_sign      = "\xC2\x54\x51\x31\xDC\xC0\x84\xA4"
                      "\x7F\xD1\x45\x08\xE8\xFF\xE8\x1D",
};

unsigned char s5l8702pwnage[CERT_SIZE] =
{
    "\x30\x82\x00\x7A\x30\x66\x02\x00\x30\x0D\x06\x09\x2A\x86\x48\x86"
    "\xF7\x0D\x01\x01\x05\x05\x00\x30\x0B\x31\x09\x30\x07\x06\x03\x55"
    "\x04\x03\x13\x00\x30\x1E\x17\x0D\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x17\x0D\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x30\x0B\x31\x09\x30\x07\x06\x03\x55\x04\x03\x13"
    "\x00\x30\x19\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01"
    "\x05\x00\x03\x08\x00\x30\x05\x02\x01\x00\x02\x00\x30\x0D\x06\x09"
    "\x2A\x86\x48\x86\xF7\x0D\x01\x01\x05\x05\x00\x03\x01\x00\x30\x82"
    "\x00\x7A\x30\x66\x02\x00\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D"
    "\x01\x01\x05\x05\x00\x30\x0B\x31\x09\x30\x07\x06\x03\x55\x04\x03"
    "\x13\x00\x30\x1E\x17\x0D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x17\x0D\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x30\x0B\x31\x09\x30\x07\x06\x03\x55\x04\x03\x13\x00\x30"
    "\x19\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01\x05\x00"
    "\x03\x08\x00\x30\x05\x02\x01\x00\x02\x00\x30\x0D\x06\x09\x2A\x86"
    "\x48\x86\xF7\x0D\x01\x01\x05\x05\x00\x03\x01\x00\x30\x82\x01\xBA"
    "\x30\x50\x02\x00\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01"
    "\x05\x05\x00\x30\x00\x30\x1E\x17\x0D\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x17\x0D\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x30\x00\x30\x19\x30\x0D\x06\x09\x2A\x86\x48"
    "\x86\xF7\x0D\x01\x01\x01\x05\x00\x03\x08\x00\x30\x05\x02\x01\x00"
    "\x02\x00\x30\x0D\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01\x01\x05\x05"
    "\x00\x03\x82\x01\x55\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
};

static uint32_t get_uint32le(unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static uint32_t get_uint32be(unsigned char* p)
{
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static void put_uint32le(unsigned char* p, uint32_t x)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

#define ERROR(format, ...) \
    do { \
        fprintf(stderr, format, __VA_ARGS__); \
        goto error; \
    } while(0)

size_t load_boot_file(char *filename, unsigned char** buf,
                                const struct ipod_models** model)
{
    int fd = -1;
    unsigned char header[8];
    int i;
    struct stat s;
    int bufsize;
    uint32_t sum;

    fd = open(filename, O_RDONLY|O_BINARY);
    if (fd < 0)
        ERROR("[ERR]  Could not open %s for reading\n", filename);

    /* Read Rockbox header */
    if (read(fd, header, sizeof(header)) != sizeof(header))
        ERROR("[ERR]  Could not read file %s\n", filename);

    for (i = 0; i < NUM_MODELS; i++)
        if (memcmp(ipod_identity[i].rb_model_name, header + 4, 4) == 0)
            break;

    if (i == NUM_MODELS)
        ERROR("[ERR]  Model name \"%4.4s\" unknown. "
                "Is this really a rockbox bootloader?\n", header + 4);

    *model = &ipod_identity[i];

    if (fstat(fd, &s) < 0)
        ERROR("[ERR]  Checking filesize of input file %s\n", filename);

    bufsize = s.st_size - sizeof(header);

    *buf = malloc(bufsize);
    if (*buf == NULL)
        ERROR("[ERR]  Could not allocate memory for %s\n", filename);

    if (read(fd, *buf, bufsize) != bufsize)
        ERROR("[ERR]  Could not read file %s\n", filename);

    /* Check checksum */
    sum = (*model)->rb_model_num;
    for (i = 0; i < bufsize; i++) {
         /* add 8 unsigned bits but keep a 32 bit sum */
         sum += (*buf)[i];
    }

    if (sum != get_uint32be(header))
        ERROR("[ERR]  Checksum mismatch in %s\n", filename);

    close(fd);
    return bufsize;

error:
    if (fd >= 0)
        close(fd);
    return -1;
}

unsigned char dfu_buf[DFU_MAXSIZE];

int mkdfu(char *infile, char* outfile, int uninstall)
{
    int fd = -1;
    const struct ipod_models *model;
    unsigned char *bl_buf;
    size_t bl_size;
    uint32_t cert_off, cert_sz;
    off_t cur_off;

    bl_size = load_boot_file(infile, &bl_buf, &model);

    if (bl_size == -1)
        goto error;

    printf("[INFO] RB bootloader:\n");
    printf("[INFO]  size:        %lu\n", bl_size);
    printf("[INFO]  model name:  %s\n", model->model_name);
    printf("[INFO]  RB name:     %s\n", model->rb_model_name);
    printf("[INfO]  RB num:      %d\n", model->rb_model_num);

    cert_off = get_uint32le(s5l8702hdr.enc34.cert_off);
    cert_sz  = get_uint32le(s5l8702hdr.enc34.cert_sz);

#if 0
    uint32_t sign_off = get_uint32le(s5l8702hdr.enc34.sign_off);

    printf("[DEBUG] DFU IM3 container:\n");
    printf("[DEBUG]  sign offset:  0x%x\n", sign_off);
    printf("[DEBUG]  cert offset:  0x%x\n", cert_off);
    printf("[DEBUG]  cert size:    0x%x\n", cert_sz);
    printf("[DEBUG] BIN_OFFSET = 0x%x\n", BIN_OFFSET);
    printf("[DEBUG] MAX_PAYLOAD = 0x%x\n", MAX_PAYLOAD);
    printf("[DEBUG] installer sz = %d\n", model->dualboot_install_size);
    printf("[DEBUG] uninstaller sz = %d\n", model->dualboot_uninstall_size);

    if (model->dualboot_install_size > MAX_PAYLOAD)
        ERROR("[ERR]  dualboot-install() too big for target %s\n", model->model_name);
    if (model->dualboot_uninstall_size > MAX_PAYLOAD)
        ERROR("[ERR]  dualboot-uninstall() too big for target %s\n", model->model_name);
    if (memcmp(model->dualboot_install + model->dualboot_install_size - IM3INFO_SZ, IM3_IDENT, 4))
        ERROR("[ERR]  bad dualboot_install() for target %s\n", model->model_name);
    if (CERT_SIZE != cert_sz)
        ERROR("[ERR]  CERT_SIZE mismatch (0x%x != 0x%x)\n", CERT_SIZE, cert_sz);
    if (CERT_OFFSET != cert_off + IM3HDR_SZ)
        ERROR("[ERR]  CERT_OFFSET mismatch (0x%x != 0x%x)\n", CERT_OFFSET, cert_off + IM3HDR_SZ);
#endif

    memcpy(dfu_buf, &s5l8702hdr, sizeof(s5l8702hdr));

    cur_off = IM3HDR_SZ + cert_off;

    memcpy(dfu_buf + cur_off, s5l8702pwnage, sizeof(s5l8702pwnage));

    /* set entry point */
    cur_off += cert_sz;
    put_uint32le(dfu_buf + cur_off - 4, DFU_LOADADDR + BIN_OFFSET);

    cur_off = BIN_OFFSET;

    if (uninstall)
    {
        /* copy the dualboot uninstaller binary */
        memcpy(dfu_buf + cur_off, model->dualboot_uninstall,
                                    model->dualboot_uninstall_size);
        cur_off += (model->dualboot_uninstall_size + 0xf) & ~0xf;
    }
    else
    {
        /* copy the dualboot installer binary */
        memcpy(dfu_buf + cur_off, model->dualboot_install,
                                    model->dualboot_install_size);

        /* point to the start of the included IM3 header info */
        cur_off += model->dualboot_install_size - IM3INFO_SZ;
        struct Im3Info *bl_info = (struct Im3Info*)(dfu_buf + cur_off);

        /* add bootloader binary */
        cur_off += IM3HDR_SZ;
        if (cur_off + bl_size > DFU_MAXSIZE)
            ERROR("[ERR]  Not enought space for bootloader %s\n", infile);
        memcpy(dfu_buf + cur_off, bl_buf, bl_size);

        /* IM3 data should be padded to 16 */
        int padded_bl_size = ((bl_size + 0xf) & ~0xf);
        put_uint32le(bl_info->data_sz, padded_bl_size);
        cur_off += padded_bl_size;
    }

    /* write output file */
    size_t dfu_size = cur_off;
    fd = open(outfile, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 0666);
    if (fd < 0)
        ERROR("[ERR]  Could not open %s for writing\n", outfile);
    if (write(fd, dfu_buf, dfu_size) != dfu_size)
        ERROR("[ERR]  Could not write file %s\n", outfile);
    close(fd);

    printf("[INFO] Created file %s, %ld bytes\n", outfile, dfu_size);
    printf("[INFO] Done!\n");

    close(fd);
    return 0;

error:
    if (fd >= 0)
        close(fd);
    return 1;
}
