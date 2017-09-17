/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "mknwzboot.h"
#include "upg.h"

#include "install_script.h"
#include "uninstall_script.h"

struct nwz_model_desc_t
{
    /* Descriptive name of this model */
    const char *model_name;
    /* Model name used in the Rockbox header in ".sansa" files - these match the
       -add parameter to the "scramble" tool */
    const char *rb_model_name;
    /* Model number used to initialise the checksum in the Rockbox header in
       ".sansa" files - these are the same as MODEL_NUMBER in config-target.h */
    const int rb_model_num;
    /* Codename used in upgtool */
    const char *codename;
};

static const struct nwz_model_desc_t nwz_models[] =
{
    { "Sony NWZ-E350 Series", "e350", 109, "nwz-e350" },
    { "Sony NWZ-E450 Series", "e450", 100, "nwz-e450" },
    { "Sony NWZ-E460 Series", "e460", 101, "nwz-e460" },
    { "Sony NWZ-E470 Series", "e470", 103, "nwz-e470" },
    { "Sony NWZ-E580 Series", "e580", 102, "nwz-e580" },
    { "Sony NWZ-A10 Series", "a10", 104, "nwz-a10" },
    { "Sony NW-A20 Series", "a20", 106, "nw-a20" },
    { "Sony NWZ-A860 Series", "a860", 107, "nwz-a860" },
    { "Sony NWZ-S750 Series", "s750", 108, "nwz-s750" },
};

#define NR_NWZ_MODELS     (sizeof(nwz_models) / sizeof(nwz_models[0]))

void dump_nwz_dev_info(const char *prefix)
{
    printf("%smknwzboot models:\n", prefix);
    for(int i = 0; i < NR_NWZ_MODELS; i++)
    {
        printf("%s  %s: rb_model=%s rb_num=%d codename=%s\n", prefix,
            nwz_models[i].model_name, nwz_models[i].rb_model_name,
            nwz_models[i].rb_model_num, nwz_models[i].codename);
    }
}

/* read a file to a buffer */
static void *read_file(const char *file, size_t *size)
{
    FILE *f = fopen(file, "rb");
    if(f == NULL)
    {
        printf("[ERR] Cannot open file '%s' for reading: %m\n", file);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *buffer = malloc(*size);
    if(fread(buffer, *size, 1, f) != 1)
    {
        free(buffer);
        fclose(f);
        printf("[ERR] Cannot read file '%s': %m\n", file);
        return NULL;
    }
    fclose(f);
    return buffer;
}

/* write a file from a buffer */
static bool write_file(const char *file, void *buffer, size_t size)
{
    FILE *f = fopen(file, "wb");
    if(f == NULL)
    {
        printf("[ERR] Cannot open file '%s' for writing: %m\n", file);
        return false;
    }
    if(fwrite(buffer, size, 1, f) != 1)
    {
        fclose(f);
        printf("[ERR] Cannot write file '%s': %m\n", file);
        return false;
    }
    fclose(f);
    return true;
}

static unsigned int be2int(unsigned char* buf)
{
   return ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

static int find_model(uint8_t *boot, size_t boot_size)
{
    if(boot_size < 8)
    {
        printf("[ERR] Boot file is too small to be valid\n");
        return -1;
    }
    /* find model by comparing magic scramble value */
    int model = 0;
    for(; model < NR_NWZ_MODELS; model++)
        if(memcmp(boot + 4, nwz_models[model].rb_model_name, 4) == 0)
            break;
    if(model == NR_NWZ_MODELS)
    {
        printf("[ERR] This player is not supported: %.4s\n", boot + 4);
        return -1;
    }
    printf("[INFO] Bootloader file for %s\n", nwz_models[model].model_name);
    /* verify checksum */
    uint32_t sum = nwz_models[model].rb_model_num;
    for(int i = 8; i < boot_size; i++)
        sum += boot[i];
    if(sum != be2int(boot))
    {
        printf("[ERR] Checksum mismatch\n");
        return -1;
    }
    return model;
}

static int find_model2(const char *model_str)
{
    /* since it can be confusing for the user, we accept both rbmodel and codename */
    /* find model by comparing magic scramble value */
    int model = 0;
    for(; model < NR_NWZ_MODELS; model++)
        if(strcmp(nwz_models[model].rb_model_name, model_str) == 0 ||
                strcmp(nwz_models[model].codename, model_str) == 0)
            break;
    if(model == NR_NWZ_MODELS)
    {
        printf("[ERR] Unknown model: %s\n", model_str);
        return -1;
    }
    printf("[INFO] Bootloader file for %s\n", nwz_models[model].model_name);
    return model;
}

static bool get_model_keysig(int model, char key[NWZ_KEY_SIZE], char sig[NWZ_SIG_SIZE])
{
    const char *codename = nwz_models[model].codename;
    for(int i = 0; g_model_list[i].model; i++)
        if(strcmp(g_model_list[i].model, codename) == 0)
        {
            if(decrypt_keysig(g_model_list[i].kas, key, sig) == 0)
                return true;
            printf("[ERR] Cannot decrypt kas '%s'\n", g_model_list[i].kas);
            return false;
        }
    printf("[ERR] Codename '%s' matches to entry in upg database\n", codename);
    return false;
}

void nwz_printf(void *u, bool err, color_t c, const char *f, ...)
{
    (void)err;
    (void)c;
    bool *debug = u;
    va_list args;
    va_start(args, f);
    if(err || *debug)
        vprintf(f, args);
    va_end(args);
}

static void *memdup(void *data, size_t size)
{
    void *buf = malloc(size);
    memcpy(buf, data, size);
    return buf;
}

int mknwzboot(const char *bootfile, const char *outfile, bool debug)
{
    size_t boot_size;
    uint8_t *boot = read_file(bootfile, &boot_size);
    if(boot == NULL)
    {
        printf("[ERR] Cannot open boot file\n");
        return 1;
    }
    /* check that it is a valid scrambled file */
    int model = find_model(boot, boot_size);
    if(model < 0)
    {
        free(boot);
        printf("[ERR] Invalid boot file\n");
        return 2;
    }
    /* find keys */
    char key[NWZ_KEY_SIZE];
    char sig[NWZ_SIG_SIZE];
    if(!get_model_keysig(model, key, sig))
    {
        printf("[ERR][INTERNAL] Cannot get keys for model\n");
        return 3;
    }
    /* create the upg file */
    struct upg_file_t *upg = upg_new();
    /* first file is the install script: we have to copy data because upg_free()
     * will free it */
    upg_append(upg, memdup(install_script, LEN_install_script), LEN_install_script);
    /* second file is the bootloader content (expected to be a tar file): we have
     * to copy data because upg_free() will free it */
    upg_append(upg, memdup(boot + 8, boot_size - 8), boot_size - 8);
    free(boot);
    /* write file to buffer */
    size_t upg_size;
    void *upg_buf = upg_write_memory(upg, key, sig, &upg_size, &debug, nwz_printf);
    upg_free(upg);
    if(upg_buf == NULL)
    {
        printf("[ERR] Cannot create UPG file\n");
        return 4;
    }
    if(!write_file(outfile, upg_buf, upg_size))
    {
        free(upg_buf);
        printf("[ERR] Cannpt write UPG file\n");
        return 5;
    }
    free(upg_buf);
    return 0;
}

int mknwzboot_uninst(const char *model_string, const char *outfile, bool debug)
{
    /* check that it is a valid scrambled file */
    int model = find_model2(model_string);
    if(model < 0)
    {
        printf("[ERR] Invalid model\n");
        return 2;
    }
    /* find keys */
    char key[NWZ_KEY_SIZE];
    char sig[NWZ_SIG_SIZE];
    if(!get_model_keysig(model, key, sig))
    {
        printf("[ERR][INTERNAL] Cannot get keys for model\n");
        return 3;
    }
    /* create the upg file */
    struct upg_file_t *upg = upg_new();
    /* first file is the uninstall script: we have to copy data because upg_free()
     * will free it */
    upg_append(upg, memdup(uninstall_script, LEN_uninstall_script), LEN_uninstall_script);
    /* write file to buffer */
    size_t upg_size;
    void *upg_buf = upg_write_memory(upg, key, sig, &upg_size, &debug, nwz_printf);
    upg_free(upg);
    if(upg_buf == NULL)
    {
        printf("[ERR] Cannot create UPG file\n");
        return 4;
    }
    if(!write_file(outfile, upg_buf, upg_size))
    {
        free(upg_buf);
        printf("[ERR] Cannpt write UPG file\n");
        return 5;
    }
    free(upg_buf);
    return 0;
}
