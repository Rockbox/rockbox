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
#include "mkimxboot.h"
#include "sb.h"
#include "dualboot.h"
#include "md5.h"
#include "elf.h"

/* abstract structure to represent a Rockbox firmware. It can be a scrambled file
 * or an ELF file or whatever. */
struct rb_fw_t
{
    int nr_insts;
    struct sb_inst_t *insts;
    int entry_idx;
};

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
    /* Version string */
    const char *version;
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
    [VARIANT_ZENXFISTYLE_RECOVERY] = "ZEN X-Fi Style Recovery",
    [VARIANT_ZENSTYLE_RECOVERY] = "ZEN Style 100/300 Recovery",
};

static const struct imx_md5sum_t imx_sums[] =
{
    /** Fuze+ */
    {
        /* Version 2.38.6 */
        MODEL_FUZEPLUS, "c3e27620a877dc6b200b97dcb3e0ecc7", "2.38.6",
        { [VARIANT_DEFAULT] = { 0, 34652624 } }
    },
    /** Zen X-Fi2 */
    {
        /* Version 1.23.01 */
        MODEL_ZENXFI2, "e37e2c24abdff8e624d0a29f79157850", "1.23.01",
        {
            [VARIANT_ZENXFI2_RECOVERY] = { 602128, 684192},
            [VARIANT_ZENXFI2_NAND] = { 1286320, 42406608 },
            [VARIANT_ZENXFI2_SD] = { 43692928, 42304208 }
        }
    },
    {
        /* Version 1.23.01e */
        MODEL_ZENXFI2, "2beff2168212d332f13cfc36ca46989d", "1.23.01e",
        {
            [VARIANT_ZENXFI2_RECOVERY] = { 0x93010, 684192},
            [VARIANT_ZENXFI2_NAND] = { 0x13a0b0, 42410704 },
            [VARIANT_ZENXFI2_SD] = { 0x29ac380, 42304208 }
        }
    },
    /** Zen X-Fi3 */
    {
        /* Version 1.00.15e */
        MODEL_ZENXFI3, "658a24eeef5f7186ca731085d8822a87", "1.00.15e",
        { [VARIANT_DEFAULT] = {0, 18110576} }
    },
    {
        /* Version 1.00.22e */
        MODEL_ZENXFI3, "a5114cd45ea4554ec221f51a71083862", "1.00.22e",
        { [VARIANT_DEFAULT] = {0, 18110576} }
    },
    {
        /* Version 1.00.25 */
        MODEL_ZENXFI3, "a41a3a78f86a4ac2879d194c6d528059", "1.00.25",
        { [VARIANT_DEFAULT] = {0, 18110576 } }
    },
    {
        /* Version 1.00.25e */
        MODEL_ZENXFI3, "c180f57e2b2d62620f87a1d853f349ff", "1.00.25e",
        { [VARIANT_DEFAULT] = {0, 18110576 } }
    },
    /** Zen X-Fi Style */
    {
        /* Version 1.03.04e */
        MODEL_ZENXFISTYLE, "32a731b7f714e9f99a95991003759c98", "1.03.04",
        {
            [VARIANT_DEFAULT] = {842960, 29876944},
            [VARIANT_ZENXFISTYLE_RECOVERY] = {610272, 232688},
        }
    },
    {
        /* Version 1.03.04e */
        MODEL_ZENXFISTYLE, "2c7ee52d9984d85dd39aa49b3331e66c", "1.03.04e",
        {
            [VARIANT_DEFAULT] = {842960, 29876944},
            [VARIANT_ZENXFISTYLE_RECOVERY] = {610272, 232688},
        }
    },
    {
        /* Version 1.03.04e */
        MODEL_ZENSTYLE, "dbebec8fe666412061d9740ff68605dd", "1.03.04e",
        {
            [VARIANT_DEFAULT] = {758848, 6641344},
            [VARIANT_ZENSTYLE_RECOVERY] = {610272, 148576},
        }
    },
    /** Sony NWZ-E370 */
    {
        /* Version 1.00.00 */
        MODEL_NWZE370, "a615fdb70b3e1bfb0355a5bc2bf237ab", "1.00.00",
        { [VARIANT_DEFAULT] = {0, 16056320 } }
    },
    {
        /* Version 1.00.01 */
        MODEL_NWZE370, "ee83f3c6026cbcc07097867f06fd585f", "1.00.01",
        { [VARIANT_DEFAULT] = {0, 16515072 } }
    },
    /** Sony NWZ-E360 */
    {
        /* Version 1.00.00 */
        MODEL_NWZE360, "d0047f8a87d456a0032297b3c802a1ff", "1.00.00",
        { [VARIANT_DEFAULT] = {0, 20652032 } }
    },
    /** Sony NWZ-E380 */
    {
        /* Version 1.00.00 */
        MODEL_NWZE370, "412f8ccd453195c0bebcc1fd8376322f", "1.00.00",
        { [VARIANT_DEFAULT] = {0, 16429056 } }
    }
};

static struct crypto_key_t zero_key =
{
    .method = CRYPTO_KEY,
    .u.key = {0}
};

static const struct imx_model_desc_t imx_models[] =
{
    [MODEL_FUZEPLUS] = {"Fuze+",  dualboot_fuzeplus, sizeof(dualboot_fuzeplus), "fuz+", 72,
                          1, &zero_key, 0, 0x40000000 },
    [MODEL_ZENXFI2] = {"Zen X-Fi2", dualboot_zenxfi2, sizeof(dualboot_zenxfi2), "zxf2", 82,
                       1, &zero_key, 0, 0x40000000 },
    [MODEL_ZENXFI3] = {"Zen X-Fi3", dualboot_zenxfi3, sizeof(dualboot_zenxfi3), "zxf3", 83,
                       1, &zero_key, 0, 0x40000000 },
    [MODEL_ZENXFISTYLE] = {"Zen X-Fi Style", dualboot_zenxfistyle, sizeof(dualboot_zenxfistyle), "zxfs", 94,
                       1, &zero_key, 0, 0x40000000 },
    [MODEL_ZENSTYLE] = {"Zen Style 100/300", NULL, 0, "", -1,
                       1, &zero_key, 0, 0x40000000 },
    [MODEL_NWZE370] = {"NWZ-E370", dualboot_nwze370, sizeof(dualboot_nwze370), "e370", 88,
                       1, &zero_key, 0, 0x40000000 },
    [MODEL_NWZE360] = {"NWZ-E360", dualboot_nwze360, sizeof(dualboot_nwze360), "e360", 89,
                       1, &zero_key, 0, 0x40000000 },
};

#define NR_IMX_SUMS     (sizeof(imx_sums) / sizeof(imx_sums[0]))
#define NR_IMX_MODELS   (sizeof(imx_models) / sizeof(imx_models[0]))

#define MAGIC_ROCK      0x726f636b /* 'rock' */
#define MAGIC_RECOVERY  0xfee1dead
#define MAGIC_NORMAL    0xcafebabe
#define MAGIC_CHARGE    0x67726863 /* 'chrg' */

static int rb_fw_get_sb_inst_count(struct rb_fw_t *fw)
{
    return fw->nr_insts;
}

/* fill sb instruction for the firmware, fill fill rb_fw_get_sb_inst_count() instructions */
static void rb_fw_fill_sb(struct rb_fw_t *fw, struct sb_inst_t *inst,
    uint32_t entry_arg)
{
    memcpy(inst, fw->insts, fw->nr_insts * sizeof(struct sb_inst_t));
    /* copy data if needed */
    for(int i = 0; i < fw->nr_insts; i++)
        if(fw->insts[i].inst == SB_INST_LOAD)
            fw->insts[i].data = memdup(fw->insts[i].data, fw->insts[i].size);
    /* replace call argument of the entry point */
    inst[fw->entry_idx].argument = entry_arg;
}

static enum imx_error_t patch_std_zero_host_play(int jump_before, int model,
    enum imx_output_type_t type, struct sb_file_t *sb_file, struct rb_fw_t boot_fw)
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

    /* used to manipulate entries */
    int nr_boot_inst = rb_fw_get_sb_inst_count(&boot_fw);

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
        /* section can have any number of instructions */
        rock_sec.identifier = MAGIC_ROCK;
        rock_sec.alignment = BLOCK_SIZE;
        rock_sec.nr_insts = nr_boot_inst;
        rock_sec.insts = xmalloc(nr_boot_inst * sizeof(struct sb_inst_t));
        rb_fw_fill_sb(&boot_fw, rock_sec.insts, MAGIC_NORMAL);

        sb_file->sections = augment_array(sb_file->sections,
            sizeof(struct sb_section_t), sb_file->nr_sections,
            &rock_sec, 1);
        sb_file->nr_sections++;

        return IMX_SUCCESS;
    }
    else if(type == IMX_SINGLEBOOT || type == IMX_RECOVERY)
    {
        bool recovery = type == IMX_RECOVERY;
        /* remove everything after the call and add instructions for firmware */
        struct sb_inst_t *new_insts = xmalloc(sizeof(struct sb_inst_t) * (jump_idx + nr_boot_inst));
        memcpy(new_insts, sec->insts, sizeof(struct sb_inst_t) * jump_idx);
        for(int i = jump_idx; i < sec->nr_insts; i++)
            sb_free_instruction(sec->insts[i]);
        rb_fw_fill_sb(&boot_fw, &new_insts[jump_idx], recovery ? MAGIC_RECOVERY : MAGIC_NORMAL);

        free(sec->insts);
        sec->insts = new_insts;
        sec->nr_insts = jump_idx + nr_boot_inst;
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
    else if(type == IMX_CHARGE)
    {
        /* throw away everything except the dualboot stub with a special argument */
        struct sb_inst_t *new_insts = xmalloc(sizeof(struct sb_inst_t) * 2);
        /* first instruction is be a load */
        struct sb_inst_t *load = &new_insts[0];
        memset(load, 0, sizeof(struct sb_inst_t));
        load->inst = SB_INST_LOAD;
        load->size = imx_models[model].dualboot_size;
        load->addr = imx_models[model].dualboot_addr;
        /* duplicate memory because it will be free'd */
        load->data = memdup(imx_models[model].dualboot, imx_models[model].dualboot_size);
        /* second instruction is a call */
        struct sb_inst_t *call = &new_insts[1];
        memset(call, 0, sizeof(struct sb_inst_t));
        call->inst = SB_INST_CALL;
        call->addr = imx_models[model].dualboot_addr;
        call->argument = MAGIC_CHARGE;
        /* free old instruction array */
        free(sec->insts);
        sec->insts = new_insts;
        sec->nr_insts = 2;
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

static enum imx_error_t parse_subversion(const char *s, const char *end, uint16_t *ver)
{
    int len = (end == NULL) ? strlen(s) : end - s;
    if(len > 4)
    {
        printf("[ERR] Bad subversion override '%s' (too long)\n", s);
        return IMX_ERROR;
    }
    *ver = 0;
    for(int i = 0; i < len; i++)
    {
        if(!isdigit(s[i]))
        {
            printf("[ERR] Bad subversion override '%s' (not a digit)\n", s);
            return IMX_ERROR;
        }
        *ver = *ver << 4 | (s[i] - '0');
    }
    return IMX_SUCCESS;
}

static enum imx_error_t parse_version(const char *s, struct sb_version_t *ver)
{
    const char *dot1 = strchr(s, '.');
    if(dot1 == NULL)
    {
        printf("[ERR] Bad version override '%s' (missing dot)\n", s);
        return IMX_ERROR;
    }
    const char *dot2 = strchr(dot1 + 1, '.');
    if(dot2 == NULL)
    {
        printf("[ERR] Bad version override '%s' (missing second dot)\n", s);
        return IMX_ERROR;
    }
    enum imx_error_t ret = parse_subversion(s, dot1, &ver->major);
    if(ret != IMX_SUCCESS) return ret;
    ret = parse_subversion(dot1 + 1, dot2, &ver->minor);
    if(ret != IMX_SUCCESS) return ret;
    ret = parse_subversion(dot2 + 1, NULL, &ver->revision);
    if(ret != IMX_SUCCESS) return ret;
    return IMX_SUCCESS;
}

static enum imx_error_t patch_firmware(enum imx_model_t model,
    enum imx_firmware_variant_t variant, enum imx_output_type_t type,
    struct sb_file_t *sb_file, struct rb_fw_t boot_fw,
    const char *force_version)
{
    if(force_version)
    {
        enum imx_error_t err = parse_version(force_version, &sb_file->product_ver);
        if(err != IMX_SUCCESS) return err;
        err = parse_version(force_version, &sb_file->component_ver);
        if(err != IMX_SUCCESS) return err;
    }
    switch(model)
    {
        case MODEL_FUZEPLUS:
            /* The Fuze+ uses the standard ____, host, play sections, patch after third
             * call in ____ section */
            return patch_std_zero_host_play(3, model, type, sb_file, boot_fw);
        case MODEL_ZENXFI3:
            /* The ZEN X-Fi3 uses the standard ____, hSst, pSay sections, patch after third
             * call in ____ section. Although sections names use the S variant, they are standard. */
            return patch_std_zero_host_play(3, model, type, sb_file, boot_fw);
        case MODEL_NWZE360:
        case MODEL_NWZE370:
            /* The NWZ-E360/E370 uses the standard ____, host, play sections, patch after first
             * call in ____ section. */
            return patch_std_zero_host_play(1, model, type, sb_file, boot_fw);
        case MODEL_ZENXFI2:
            /* The ZEN X-Fi2 has two types of firmware: recovery and normal.
             * Normal uses the standard ___, host, play sections and recovery only ____ */
            switch(variant)
            {
                case VARIANT_ZENXFI2_RECOVERY:
                case VARIANT_ZENXFI2_NAND:
                case VARIANT_ZENXFI2_SD:
                    return patch_std_zero_host_play(1, model, type, sb_file, boot_fw);
                default:
                    return IMX_DONT_KNOW_HOW_TO_PATCH;
            }
            break;
        case MODEL_ZENXFISTYLE:
            /* The ZEN X-Fi Style uses the standard ____, host, play sections, patch after first
             * call in ____ section. */
            return patch_std_zero_host_play(1, model, type, sb_file, boot_fw);
        default:
            return IMX_DONT_KNOW_HOW_TO_PATCH;
    }
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
        printf("%s  md5sum=%s -> idx=%d, ver=%s\n", prefix, imx_sums[i].md5sum,
            imx_sums[i].model, imx_sums[i].version);
        for(int j = 0; j < VARIANT_COUNT; j++)
            if(imx_sums[i].fw_variants[j].size)
                printf("%s    variant=%d -> offset=%#x size=%#x\n", prefix,
                    j, (unsigned)imx_sums[i].fw_variants[j].offset,
                    (unsigned)imx_sums[i].fw_variants[j].size);
    }
}

/* find an entry into imx_sums which matches the MD5 sum of a file */
static enum imx_error_t find_model_by_md5sum(uint8_t file_md5sum[16], int *md5_idx)
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
            {
                printf("[ERR][INTERNAL] Bad checksum format: %s\n", imx_sums[i].md5sum);
                return IMX_ERROR;
            }
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
    *md5_idx = i;
    return IMX_SUCCESS;
}

/* read a file to a buffer */
static enum imx_error_t read_file(const char *file, void **buffer, size_t *size)
{
    FILE *f = fopen(file, "rb");
    if(f == NULL)
    {
        printf("[ERR] Cannot open file '%s' for reading: %m\n", file);
        return IMX_OPEN_ERROR;
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *buffer = xmalloc(*size);
    if(fread(*buffer, *size, 1, f) != 1)
    {
        free(*buffer);
        fclose(f);
        printf("[ERR] Cannot read file '%s': %m\n", file);
        return IMX_READ_ERROR;
    }
    fclose(f);
    return IMX_SUCCESS;
}

/* write a file from a buffer */
static enum imx_error_t write_file(const char *file, void *buffer, size_t size)
{
    FILE *f = fopen(file, "wb");
    if(f == NULL)
    {
        printf("[ERR] Cannot open file '%s' for writing: %m\n", file);
        return IMX_OPEN_ERROR;
    }
    if(fwrite(buffer, size, 1, f) != 1)
    {
        fclose(f);
        printf("[ERR] Cannot write file '%s': %m\n", file);
        return IMX_WRITE_ERROR;
    }
    fclose(f);
    return IMX_SUCCESS;
}

/* compute MD5 sum of a buffer */
static enum imx_error_t compute_md5sum_buf(void *buf, size_t sz, uint8_t file_md5sum[16])
{
    md5_context ctx;
    md5_starts(&ctx);
    md5_update(&ctx, buf, sz);
    md5_finish(&ctx, file_md5sum);
    return IMX_SUCCESS;
}

/* compute MD5 sum of a buffer */
static enum imx_error_t compute_soft_md5sum_buf(struct sb_file_t *sb, uint8_t file_md5sum[16])
{
    md5_context ctx;
    md5_starts(&ctx);
#define hash(obj) \
    md5_update(&ctx, (void *)&obj, sizeof(obj))
    /* various header fiels */
    hash(sb->timestamp);
    hash(sb->drive_tag);
    hash(sb->drive_tag);
    hash(sb->first_boot_sec_id);
    hash(sb->flags);
    hash(sb->product_ver);
    hash(sb->component_ver);

    for(int i = 0; i < sb->nr_sections; i++)
    {
        struct sb_section_t *sec = &sb->sections[i];
        hash(sec->identifier);
        uint32_t flags = sec->other_flags;
        if(!sec->is_data)
            flags |= SECTION_BOOTABLE;
        if(sec->is_cleartext)
            flags |= SECTION_CLEARTEXT;
        hash(flags);

        for(int j = 0; j < sec->nr_insts; j++)
        {
            struct sb_inst_t *inst = &sec->insts[j];
            switch(inst->inst)
            {
                case SB_INST_NOP:
                    /* ignore them totally because they are used for padding */
                    break;
                case SB_INST_LOAD:
                    hash(inst->inst);
                    hash(inst->addr);
                    md5_update(&ctx, inst->data, inst->size);
                    break;
                case SB_INST_FILL:
                    hash(inst->inst);
                    hash(inst->addr);
                    hash(inst->pattern);
                    break;
                case SB_INST_JUMP:
                case SB_INST_CALL:
                    hash(inst->inst);
                    hash(inst->addr);
                    hash(inst->argument);
                    break;
                case SB_INST_MODE:
                    hash(inst->inst);
                    hash(inst->argument);
                    break;
                case SB_INST_DATA:
                    md5_update(&ctx, inst->data, inst->size);
                    break;
                default:
                    printf("[ERR][INTERNAL] Unexpected instruction %d\n", inst->inst);
                    return IMX_ERROR;
            }
        }
    }
#undef hash
    md5_finish(&ctx, file_md5sum);
    return IMX_SUCCESS;
}

/* compute MD5 of a file */
enum imx_error_t compute_md5sum(const char *file, uint8_t file_md5sum[16])
{
    void *buf;
    size_t sz;
    enum imx_error_t err = read_file(file, &buf, &sz);
    if(err != IMX_SUCCESS)
        return err;
    compute_md5sum_buf(buf, sz, file_md5sum);
    free(buf);
    return IMX_SUCCESS;
}

/* compute soft MD5 of a file */
enum imx_error_t compute_soft_md5sum(const char *file, enum imx_model_t model,
    uint8_t soft_md5sum[16])
{
    if(model == MODEL_UNKNOWN)
    {
        printf("[ERR] Cannot compute soft MD5 without knowing the model\n");
        return IMX_ERROR;
    }
    clear_keys();
    add_keys(imx_models[model].keys, imx_models[model].nr_keys);
    /* read file */
    enum sb_error_t err;
    struct sb_file_t *sb = sb_read_file(file, false, NULL, generic_std_printf, &err);
    if(sb == NULL)
    {
        printf("[ERR] Cannot load SB file: %d\n", err);
        return err;
    }
    /* compute sum */
    err = compute_soft_md5sum_buf(sb, soft_md5sum);
    /* release file */
    sb_free(sb);
    return err;
}

static enum imx_error_t load_sb_file(const char *file, int md5_idx,
    struct imx_option_t opt, struct sb_file_t **sb_file)
{
    if(imx_sums[md5_idx].fw_variants[opt.fw_variant].size == 0)
    {
        printf("[ERR] Input file does not contain variant '%s'\n", imx_fw_variant[opt.fw_variant]);
        return IMX_VARIANT_MISMATCH;
    }
    enum imx_model_t model = imx_sums[md5_idx].model;
    enum sb_error_t err;
    g_debug = opt.debug;
    clear_keys();
    add_keys(imx_models[model].keys, imx_models[model].nr_keys);
    *sb_file = sb_read_file_ex(file, imx_sums[md5_idx].fw_variants[opt.fw_variant].offset,
                              imx_sums[md5_idx].fw_variants[opt.fw_variant].size, false, NULL, generic_std_printf, &err);
    if(*sb_file == NULL)
    {
        clear_keys();
        return IMX_FIRST_SB_ERROR + err;
    }
    return IMX_SUCCESS;
}

/* Load a rockbox firwmare from a buffer. Data is copied. Assume firmware is
 * using our scramble format. */
static enum imx_error_t rb_fw_load_buf_scramble(struct rb_fw_t *fw, uint8_t *buf,
    size_t sz, enum imx_model_t model)
{
    if(sz < 8)
    {
        printf("[ERR] Bootloader file is too small to be valid\n");
        return IMX_BOOT_INVALID;
    }
    /* check model name */
    uint8_t *name = buf + 4;
    if(memcmp(name, imx_models[model].rb_model_name, 4) != 0)
    {
        printf("[ERR] Bootloader model doesn't match found model for input file\n");
        return IMX_BOOT_MISMATCH;
    }
    /* check checksum */
    uint32_t sum = imx_models[model].rb_model_num;
    for(int i = 8; i < sz; i++)
        sum += buf[i];
    if(sum != get_uint32be(buf))
    {
        printf("[ERR] Bootloader checksum mismatch\n");
        return IMX_BOOT_CHECKSUM_ERROR;
    }
    /* two instructions: load and jump */
    fw->nr_insts = 2;
    fw->entry_idx = 1;
    fw->insts = xmalloc(fw->nr_insts * sizeof(struct sb_inst_t));
    memset(fw->insts, 0, fw->nr_insts * sizeof(struct sb_inst_t));
    fw->insts[0].inst = SB_INST_LOAD;
    fw->insts[0].addr = imx_models[model].bootloader_addr;
    fw->insts[0].size = sz - 8;
    fw->insts[0].data = memdup(buf + 8, sz - 8);
    fw->insts[1].inst = SB_INST_JUMP;
    fw->insts[1].addr = imx_models[model].bootloader_addr;
    return IMX_SUCCESS;
}

struct elf_user_t
{
    void *buf;
    size_t sz;
};

static bool elf_read(void *user, uint32_t addr, void *buf, size_t count)
{
    struct elf_user_t *u = user;
    if(addr + count <= u->sz)
    {
        memcpy(buf, u->buf + addr, count);
        return true;
    }
    else
        return false;
}

/* Load a rockbox firwmare from a buffer. Data is copied. Assume firmware is
 * using ELF format. */
static enum imx_error_t rb_fw_load_buf_elf(struct rb_fw_t *fw, uint8_t *buf,
    size_t sz, enum imx_model_t model)
{
    struct elf_params_t elf;
    struct elf_user_t user;
    user.buf = buf;
    user.sz = sz;
    elf_init(&elf);
    if(!elf_read_file(&elf, elf_read, generic_std_printf, &user))
    {
        elf_release(&elf);
        printf("[ERR] Error parsing ELF file\n");
        return IMX_BOOT_INVALID;
    }
    fw->nr_insts = elf_get_nr_sections(&elf) + 1;
    fw->insts = xmalloc(fw->nr_insts * sizeof(struct sb_inst_t));
    fw->entry_idx = fw->nr_insts - 1;
    memset(fw->insts, 0, fw->nr_insts * sizeof(struct sb_inst_t));
    struct elf_section_t *sec = elf.first_section;
    for(int i = 0; sec; i++, sec = sec->next)
    {
        fw->insts[i].addr = elf_translate_virtual_address(&elf, sec->addr);
        fw->insts[i].size = sec->size;
        if(sec->type == EST_LOAD)
        {
            fw->insts[i].inst = SB_INST_LOAD;
            fw->insts[i].data = memdup(sec->section, sec->size);
        }
        else if(sec->type == EST_FILL)
        {
            fw->insts[i].inst = SB_INST_FILL;
            fw->insts[i].pattern = sec->pattern;
        }
        else
        {
            printf("[WARN] Warning parsing ELF file: unsupported section type mapped to NOP!\n");
            fw->insts[i].inst = SB_INST_NOP;
        }
    }
    fw->insts[fw->nr_insts - 1].inst = SB_INST_JUMP;
    if(!elf_get_start_addr(&elf, &fw->insts[fw->nr_insts - 1].addr))
    {
        elf_release(&elf);
        printf("[ERROR] Error parsing ELF file: it has no entry point!\n");
        return IMX_BOOT_INVALID;
    }
    elf_release(&elf);
    return IMX_SUCCESS;
}

/* Load a rockbox firwmare from a buffer. Data is copied. */
static enum imx_error_t rb_fw_load_buf(struct rb_fw_t *fw, uint8_t *buf,
    size_t sz, enum imx_model_t model)
{
    /* detect file format */
    if(sz >= 4 && buf[0] == 0x7f && memcmp(buf + 1, "ELF", 3) == 0)
        return rb_fw_load_buf_elf(fw, buf, sz, model);
    else
        return rb_fw_load_buf_scramble(fw, buf, sz, model);
}

/* load a rockbox firmware from a file. */
static enum imx_error_t rb_fw_load(struct rb_fw_t *fw, const char *file,
    enum imx_model_t model)
{
    void *buf;
    size_t sz;
    int ret = read_file(file, &buf, &sz);
    if(ret == IMX_SUCCESS)
    {
        ret = rb_fw_load_buf(fw, buf, sz, model);
        free(buf);
    }
    return ret;
}

/* free rockbox firmware */
static void rb_fw_free(struct rb_fw_t *fw)
{
    for(int i = 0; i < fw->nr_insts; i++)
        sb_free_instruction(fw->insts[i]);
    free(fw->insts);
    memset(fw, 0, sizeof(struct rb_fw_t));
}

enum imx_error_t mkimxboot(const char *infile, const char *bootfile,
    const char *outfile, struct imx_option_t opt)
{
    /* sanity check */
    if(opt.fw_variant > VARIANT_COUNT)
        return IMX_ERROR;
    /* Dump tables */
    dump_imx_dev_info("[INFO] ");
    /* compute MD5 sum of the file */
    uint8_t file_md5sum[16];
    enum imx_error_t ret = compute_md5sum(infile, file_md5sum);
    if(ret != IMX_SUCCESS)
        return ret;
    printf("[INFO] MD5 sum of the file: ");
    for(int i = 0; i < 16; i++)
        printf("%02x", file_md5sum[i]);
    printf("\n");
    /* find model */
    int md5_idx;
    ret = find_model_by_md5sum(file_md5sum, &md5_idx);
    if(ret != IMX_SUCCESS)
        return ret;
    enum imx_model_t model = imx_sums[md5_idx].model;
    printf("[INFO] File is for model %d (%s, version %s)\n", model,
        imx_models[model].model_name, imx_sums[md5_idx].version);
    /* load rockbox file */
    struct rb_fw_t boot_fw;
    ret = rb_fw_load(&boot_fw, bootfile, model);
    if(ret != IMX_SUCCESS)
        return ret;
    /* load OF file */
    struct sb_file_t *sb_file;
    ret = load_sb_file(infile, md5_idx, opt, &sb_file);
    if(ret != IMX_SUCCESS)
    {
        rb_fw_free(&boot_fw);
        return ret;
    }
    /* produce file */
    ret = patch_firmware(model, opt.fw_variant, opt.output,
        sb_file, boot_fw, opt.force_version);
    if(ret == IMX_SUCCESS)
        ret = sb_write_file(sb_file, outfile, NULL, generic_std_printf);

    clear_keys();
    rb_fw_free(&boot_fw);
    sb_free(sb_file);
    return ret;
}

enum imx_error_t extract_firmware(const char *infile,
    enum imx_firmware_variant_t fw_variant, const char *outfile)
{
    /* sanity check */
    if(fw_variant > VARIANT_COUNT)
        return IMX_ERROR;
    /* dump tables */
    dump_imx_dev_info("[INFO] ");
    /* compute MD5 sum of the file */
    void *buf;
    size_t sz;
    uint8_t file_md5sum[16];
    int ret = read_file(infile, &buf, &sz);
    if(ret != IMX_SUCCESS)
        return ret;
    ret = compute_md5sum_buf(buf, sz, file_md5sum);
    if(ret != IMX_SUCCESS)
    {
        free(buf);
        return ret;
    }
    printf("[INFO] MD5 sum of the file: ");
    print_hex(NULL, misc_std_printf, file_md5sum, 16, true);
    /* find model */
    int md5_idx;
    ret = find_model_by_md5sum(file_md5sum, &md5_idx);
    if(ret != IMX_SUCCESS)
    {
        free(buf);
        return ret;
    }
    enum imx_model_t model = imx_sums[md5_idx].model;
    printf("[INFO] File is for model %d (%s, version %s)\n", model,
        imx_models[model].model_name, imx_sums[md5_idx].version);
    /* extract firmware */
    if(imx_sums[md5_idx].fw_variants[fw_variant].size == 0)
    {
        printf("[ERR] Input file does not contain variant '%s'\n", imx_fw_variant[fw_variant]);
        free(buf);
        return IMX_VARIANT_MISMATCH;
    }

    ret = write_file(outfile,
        buf + imx_sums[md5_idx].fw_variants[fw_variant].offset,
        imx_sums[md5_idx].fw_variants[fw_variant].size);
    free(buf);
    return ret;
}
