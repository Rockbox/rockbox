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
#include "mkimxboot.h"
#include "sb.h"
#include "dualboot.h"
#include "md5.h"

struct imx_fw_variant_desc_t
{
    /* Offset within file */
    size_t offset;
    /* Total size of the firmware */
    size_t size;
};

struct imx_md5sum_t
{
    /* Device model */
    enum imx_model_t model;
    /* md5sum of the file */
    char *md5sum;
    /* Variant descriptions */
    struct imx_fw_variant_desc_t fw_variants[VARIANT_COUNT];
};

struct imx_model_desc_t
{
    /* Descriptive name of this model */
    const char *model_name;
    /* Dualboot code for this model */
    const unsigned char *dualboot;
    /* Size of dualboot functions for this model */
    int dualboot_size;
    /* Model name used in the Rockbox header in ".sansa" files - these match the
       -add parameter to the "scramble" tool */
    const char *rb_model_name;
    /* Model number used to initialise the checksum in the Rockbox header in
       ".sansa" files - these are the same as MODEL_NUMBER in config-target.h */
    const int rb_model_num;
    /* Number of keys needed to decrypt/encrypt */
    int nr_keys;
    /* Array of keys */
    struct crypto_key_t *keys;
    /* Dualboot load address */
    uint32_t dualboot_addr;
    /* Bootloader load address */
    uint32_t bootloader_addr;
};

static const char *imx_fw_variant[] =
{
    [VARIANT_DEFAULT] = "default",
    [VARIANT_ZENXFI2_RECOVERY] = "ZEN X-Fi2 Recovery",
    [VARIANT_ZENXFI2_NAND] = "ZEN X-Fi2 NAND",
    [VARIANT_ZENXFI2_SD] = "ZEN X-Fi2 eMMC/SD",
};

static const struct imx_md5sum_t imx_sums[] =
{
    {
        /* Version 2.38.6 */
        MODEL_FUZEPLUS, "c3e27620a877dc6b200b97dcb3e0ecc7",
        { [VARIANT_DEFAULT] = { 0, 34652624 } }
    },
    {
        /* Version 1.23.01e */
        MODEL_ZENXFI2, "e37e2c24abdff8e624d0a29f79157850",
    },
    {
        /* Version 1.23.01e */
        MODEL_ZENXFI2, "2beff2168212d332f13cfc36ca46989d",
        { [VARIANT_ZENXFI2_RECOVERY] = { 0x93010, 684192},
          [VARIANT_ZENXFI2_NAND] = { 0x13a0b0, 42410704 },
          [VARIANT_ZENXFI2_SD] = { 0x29ac380, 42304208 }
        }
    },
    {
        /* Version 1.00.22e */
        MODEL_ZENXFI3, "658a24eeef5f7186ca731085d8822a87",
        { [VARIANT_DEFAULT] = {0, 18110576} }
    },
};

static struct crypto_key_t zero_key =
{
    .method = CRYPTO_KEY,
    .u.key = {0}
};

static const struct imx_model_desc_t imx_models[] =
{
    [MODEL_FUZEPLUS] = { "Fuze+",  dualboot_fuzeplus, sizeof(dualboot_fuzeplus), "fuz+", 72,
                          1, &zero_key, 0, 0x40000000 },
    [MODEL_ZENXFI2] = {"Zen X-Fi2", dualboot_zenxfi2, sizeof(dualboot_zenxfi2), "zxf2", 82,
                       1, &zero_key, 0, 0x40000000 },
    [MODEL_ZENXFI3] = {"Zen X-Fi3", dualboot_zenxfi3, sizeof(dualboot_zenxfi3), "zxf3", 83,
                       1, &zero_key, 0, 0x40000000 },
};

#define NR_IMX_SUMS     (sizeof(imx_sums) / sizeof(imx_sums[0]))
#define NR_IMX_MODELS   (sizeof(imx_models) / sizeof(imx_models[0]))

#define MAGIC_ROCK      0x726f636b /* 'rock' */
#define MAGIC_RECOVERY  0xfee1dead
#define MAGIC_NORMAL    0xcafebabe

static enum imx_error_t patch_std_zero_host_play(int jump_before, int model,
 enum imx_output_type_t type, struct sb_file_t *sb_file, void *boot, size_t boot_sz)
{
    /* We assume the file has three boot sections: ____, host, play and one
     * resource section rsrc.
     *
     * Dual Boot:
     * ----------
     * We patch the file by inserting the dualboot code before the <jump_before>th
     * call in the ____ section. We give it as argument the section name 'rock'
     * and add a section called 'rock' after rsrc which contains the bootloader.
     *
     * Single Boot & Recovery:
     * -----------------------
     * We patch the file by inserting the bootloader code after the <jump_before>th
     * call in the ____ section and get rid of everything else. In recovery mode,
     * we give 0xfee1dead as argument */

    /* Do not override real key and IV */
    sb_file->override_crypto_iv = false;
    sb_file->override_real_key = false;

    /* first locate the good instruction */
    struct sb_section_t *sec = &sb_file->sections[0];
    int jump_idx = 0;
    while(jump_idx < sec->nr_insts && jump_before > 0)
        if(sec->insts[jump_idx++].inst == SB_INST_CALL)
            jump_before--;
    if(jump_idx == sec->nr_insts)
    {
        printf("[ERR] Cannot locate call in section ____\n");
        return IMX_DONT_KNOW_HOW_TO_PATCH;
    }

    if(type == IMX_DUALBOOT)
    {
        /* create a new instruction array with a hole for two instructions */
        struct sb_inst_t *new_insts = xmalloc(sizeof(struct sb_inst_t) * (sec->nr_insts + 2));
        memcpy(new_insts, sec->insts, sizeof(struct sb_inst_t) * jump_idx);
        memcpy(new_insts + jump_idx + 2, sec->insts + jump_idx,
            sizeof(struct sb_inst_t) * (sec->nr_insts - jump_idx));
        /* first instruction is be a load */
        struct sb_inst_t *load = &new_insts[jump_idx];
        memset(load, 0, sizeof(struct sb_inst_t));
        load->inst = SB_INST_LOAD;
        load->size = imx_models[model].dualboot_size;
        load->addr = imx_models[model].dualboot_addr;
        /* duplicate memory because it will be free'd */
        load->data = memdup(imx_models[model].dualboot, imx_models[model].dualboot_size);
        /* second instruction is a call */
        struct sb_inst_t *call = &new_insts[jump_idx + 1];
        memset(call, 0, sizeof(struct sb_inst_t));
        call->inst = SB_INST_CALL;
        call->addr = imx_models[model].dualboot_addr;
        call->argument = MAGIC_ROCK;
        /* free old instruction array */
        free(sec->insts);
        sec->insts = new_insts;
        sec->nr_insts += 2;

        /* create a new section */
        struct sb_section_t rock_sec;
        memset(&rock_sec, 0, sizeof(rock_sec));
        /* section has two instructions: load and call */
        rock_sec.identifier = MAGIC_ROCK;
        rock_sec.alignment = BLOCK_SIZE;
        rock_sec.nr_insts = 2;
        rock_sec.insts = xmalloc(2 * sizeof(struct sb_inst_t));
        memset(rock_sec.insts, 0, 2 * sizeof(struct sb_inst_t));
        rock_sec.insts[0].inst = SB_INST_LOAD;
        rock_sec.insts[0].size = boot_sz;
        rock_sec.insts[0].data = memdup(boot, boot_sz);
        rock_sec.insts[0].addr = imx_models[model].bootloader_addr;
        rock_sec.insts[1].inst = SB_INST_JUMP;
        rock_sec.insts[1].addr = imx_models[model].bootloader_addr;
        rock_sec.insts[1].argument = MAGIC_NORMAL;

        sb_file->sections = augment_array(sb_file->sections,
            sizeof(struct sb_section_t), sb_file->nr_sections,
            &rock_sec, 1);
        sb_file->nr_sections++;

        return IMX_SUCCESS;
    }
    else if(type == IMX_SINGLEBOOT || type == IMX_RECOVERY)
    {
        bool recovery = type == IMX_RECOVERY;
        /* remove everything after the call and add two instructions: load and call */
        struct sb_inst_t *new_insts = xmalloc(sizeof(struct sb_inst_t) * (jump_idx + 2));
        memcpy(new_insts, sec->insts, sizeof(struct sb_inst_t) * jump_idx);
        for(int i = jump_idx; i < sec->nr_insts; i++)
            sb_free_instruction(sec->insts[i]);
        memset(new_insts + jump_idx, 0, 2 * sizeof(struct sb_inst_t));
        new_insts[jump_idx + 0].inst = SB_INST_LOAD;
        new_insts[jump_idx + 0].size = boot_sz;
        new_insts[jump_idx + 0].data = memdup(boot, boot_sz);
        new_insts[jump_idx + 0].addr = imx_models[model].bootloader_addr;
        new_insts[jump_idx + 1].inst = SB_INST_JUMP;
        new_insts[jump_idx + 1].addr = imx_models[model].bootloader_addr;
        new_insts[jump_idx + 1].argument = recovery ? MAGIC_RECOVERY : MAGIC_NORMAL;
        
        free(sec->insts);
        sec->insts = new_insts;
        sec->nr_insts = jump_idx + 2;
        /* remove all other sections */
        for(int i = 1; i < sb_file->nr_sections; i++)
            sb_free_section(sb_file->sections[i]);
        struct sb_section_t *new_sec = xmalloc(sizeof(struct sb_section_t));
        memcpy(new_sec, &sb_file->sections[0], sizeof(struct sb_section_t));
        free(sb_file->sections);
        sb_file->sections = new_sec;
        sb_file->nr_sections = 1;

        return IMX_SUCCESS;
    }
    else
    {
        printf("[ERR] Bad output type !\n");
        return IMX_DONT_KNOW_HOW_TO_PATCH;
    }
}

static enum imx_error_t patch_firmware(enum imx_model_t model,
    enum imx_firmware_variant_t variant, enum imx_output_type_t type,
    struct sb_file_t *sb_file, void *boot, size_t boot_sz)
{
    switch(model)
    {
        case MODEL_FUZEPLUS:
            /* The Fuze+ uses the standard ____, host, play sections, patch after third
             * call in ____ section */
            return patch_std_zero_host_play(3, model, type, sb_file, boot, boot_sz);
        case MODEL_ZENXFI3:
            /* The ZEN X-Fi3 uses the standard ____, hSst, pSay sections, patch after third
             * call in ____ section. Although sections names use the S variant, they are standard. */
            return patch_std_zero_host_play(3, model, type, sb_file, boot, boot_sz);
        case MODEL_ZENXFI2:
            /* The ZEN X-Fi2 has two types of firmware: recovery and normal.
             * Normal uses the standard ___, host, play sections and recovery only ____ */
            switch(variant)
            {
                case VARIANT_ZENXFI2_RECOVERY:
                case VARIANT_ZENXFI2_NAND:
                case VARIANT_ZENXFI2_SD:
                    return patch_std_zero_host_play(1, model, type, sb_file, boot, boot_sz);
                default:
                    return IMX_DONT_KNOW_HOW_TO_PATCH;
            }
            break;
        default:
            return IMX_DONT_KNOW_HOW_TO_PATCH;
    }
}

static void imx_printf(void *user, bool error, color_t c, const char *fmt, ...)
{
    (void) user;
    (void) c;
    va_list args;
    va_start(args, fmt);
    /*
    if(error)
        printf("[ERR] ");
    else
        printf("[INFO] ");
    */
    vprintf(fmt, args);
    va_end(args);
}

static uint32_t get_uint32be(unsigned char *p)
{
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

void dump_imx_dev_info(const char *prefix)
{
    printf("%smkimxboot models:\n", prefix);
    for(int i = 0; i < NR_IMX_MODELS; i++)
    {
        printf("%s  %s: idx=%d rb_model=%s rb_num=%d\n", prefix,
            imx_models[i].model_name, i, imx_models[i].rb_model_name,
            imx_models[i].rb_model_num);
    }
    printf("%smkimxboot variants:\n", prefix);
    for(int i = 0; i < VARIANT_COUNT; i++)
    {
        printf("%s  %d: %s\n", prefix, i, imx_fw_variant[i]);
    }
    printf("%smkimxboot mapping:\n", prefix);
    for(int i = 0; i < NR_IMX_SUMS; i++)
    {
        printf("%s  md5sum=%s -> idx=%d\n", prefix, imx_sums[i].md5sum,
            imx_sums[i].model);
        for(int j = 0; j < VARIANT_COUNT; j++)
            if(imx_sums[i].fw_variants[j].size)
                printf("%s    variant=%d -> offset=%#x size=%#x\n", prefix,
                    j, (unsigned)imx_sums[i].fw_variants[j].offset,
                    (unsigned)imx_sums[i].fw_variants[j].size);
    }
}

enum imx_error_t mkimxboot(const char *infile, const char *bootfile,
    const char *outfile, struct imx_option_t opt)
{
    /* Dump tables */
    if(opt.fw_variant > VARIANT_COUNT) {
        return IMX_ERROR;
    }
    dump_imx_dev_info("[INFO] ");
    /* compute MD5 sum of the file */
    uint8_t file_md5sum[16];
    do
    {
        FILE *f = fopen(infile, "rb");
        if(f == NULL)
        {
            printf("[ERR] Cannot open input file\n");
            return IMX_OPEN_ERROR;
        }
        fseek(f, 0, SEEK_END);
        size_t sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        void *buf = xmalloc(sz);
        if(fread(buf, sz, 1, f) != 1)
        {
            fclose(f);
            free(buf);
            printf("[ERR] Cannot read file\n");
            return IMX_READ_ERROR;
        }
        fclose(f);
        md5_context ctx;
        md5_starts(&ctx);
        md5_update(&ctx, buf, sz);
        md5_finish(&ctx, file_md5sum);
        free(buf);
    }while(0);
    printf("[INFO] MD5 sum of the file: ");
    print_hex(file_md5sum, 16, true);
    /* find model */
    enum imx_model_t model;
    int md5_idx;
    do
    {
        int i = 0;
        while(i < NR_IMX_SUMS)
        {
            uint8_t md5[20];
            if(strlen(imx_sums[i].md5sum) != 32)
            {
                printf("[INFO] Invalid MD5 sum in imx_sums\n");
                return IMX_ERROR;
            }
            for(int j = 0; j < 16; j++)
            {
                byte a, b;
                if(convxdigit(imx_sums[i].md5sum[2 * j], &a) || convxdigit(imx_sums[i].md5sum[2 * j + 1], &b))
                    return false;
                md5[j] = (a << 4) | b;
            }
            if(memcmp(file_md5sum, md5, 16) == 0)
                break;
            i++;
        }
        if(i == NR_IMX_SUMS)
        {
            printf("[ERR] MD5 sum doesn't match any known file\n");
            return IMX_NO_MATCH;
        }
        model = imx_sums[i].model;
        md5_idx = i;
    }while(0);
    printf("[INFO] File is for model %d (%s)\n", model, imx_models[model].model_name);
    /* load rockbox file */
    uint8_t *boot;
    size_t boot_size;
    do
    {
        FILE *f = fopen(bootfile, "rb");
        if(f == NULL)
        {
            printf("[ERR] Cannot open boot file\n");
            return IMX_OPEN_ERROR;
        }
        fseek(f, 0, SEEK_END);
        boot_size = ftell(f);
        fseek(f, 0, SEEK_SET);
        boot = xmalloc(boot_size);
        if(fread(boot, boot_size, 1, f) != 1)
        {
            free(boot);
            fclose(f);
            printf("[ERR] Cannot read boot file\n");
            return IMX_READ_ERROR;
        }
        fclose(f);
    }while(0);
    /* Check boot file */
    do
    {
        if(boot_size < 8)
        {
            printf("[ERR] Bootloader file is too small to be valid\n");
            free(boot);
            return IMX_BOOT_INVALID;
        }
        /* check model name */
        uint8_t *name = boot + 4;
        if(memcmp(name, imx_models[model].rb_model_name, 4) != 0)
        {
            printf("[ERR] Bootloader model doesn't match found model for input file\n");
            free(boot);
            return IMX_BOOT_MISMATCH;
        }
        /* check checksum */
        uint32_t sum = imx_models[model].rb_model_num;
        for(int i = 8; i < boot_size; i++)
            sum += boot[i];
        if(sum != get_uint32be(boot))
        {
            printf("[ERR] Bootloader checksum mismatch\n");
            free(boot);
            return IMX_BOOT_CHECKSUM_ERROR;
        }
    }while(0);
    /* load OF file */
    struct sb_file_t *sb_file;
    do
    {
        if(imx_sums[md5_idx].fw_variants[opt.fw_variant].size == 0)
        {
            printf("[ERR] Input file does not contain variant '%s'\n", imx_fw_variant[opt.fw_variant]);
            free(boot);
            return IMX_VARIANT_MISMATCH;
        }
        enum sb_error_t err;
        g_debug = opt.debug;
        clear_keys();
        add_keys(imx_models[model].keys, imx_models[model].nr_keys);
        sb_file = sb_read_file_ex(infile, imx_sums[md5_idx].fw_variants[opt.fw_variant].offset,
            imx_sums[md5_idx].fw_variants[opt.fw_variant].size, false, NULL, &imx_printf, &err);
        if(sb_file == NULL)
        {
            clear_keys();
            free(boot);
            return IMX_FIRST_SB_ERROR + err;
        }
    }while(0);
    /* produce file */
    enum imx_error_t ret = patch_firmware(model, opt.fw_variant, opt.output, sb_file, boot + 8, boot_size - 8);
    if(ret == IMX_SUCCESS)
        ret = sb_write_file(sb_file, outfile);

    clear_keys();
    free(boot);
    sb_free(sb_file);
    return ret;
}
