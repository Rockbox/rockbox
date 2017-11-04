/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
 * Based on zenutils by Rasmus Ry <rasmus.ry{at}gmail.com>
 * Copyright (C) 2013 by Amaury Pouly
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
#include "mkzenboot.h"
#include "utils.h"
#include "dualboot.h"

/**
 * Keys used by players
 */
static const char null_key_v1[]  = "CTL:N0MAD|PDE0.SIGN.";
static const char null_key_v2[]  = "CTL:N0MAD|PDE0.DPMP.";
static const char null_key_v3[]  = "CTL:N0MAD|PDE0.DPFP.";
static const char null_key_v4[]  = "CTL:Z3N07|PDE0.DPMP.";

static const char tl_zvm_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Vision:M";
static const char tl_zvm60_key[] = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Vision:M (D"
                                   "VP-HD0004)";
static const char tl_zen_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative ZEN";
static const char tl_zenxf_key[] = "1sN0TM3D az u~may th1nk*"
                                   "Creative ZEN X-Fi";
static const char tl_zenmo_key[] = "1sN0TM3D az u~may th1nk*"
                                   "Creative ZEN Mozaic";
static const char tl_zv_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Vision";
static const char tl_zvw_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative ZEN Vision W";
static const char tl_zm_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Micro";
static const char tl_zmp_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen MicroPhoto";
static const char tl_zs_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Sleek";
static const char tl_zsp_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Sleek Photo";
static const char tl_zt_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Touch";
static const char tl_zx_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "NOMAD Jukebox Zen Xtra";
static const char tl_zenv_key[]  = "1sN0TM3D az u~may th1nk*"
                                   "Creative ZEN V";
static const char tl_zenvp_key[] = "1sN0TM3D az u~may th1nk*"
                                   "Creative ZEN V Plus";
static const char tl_zenvv_key[] = "1sN0TM3D az u~may th1nk*"
                                   "Creative ZEN V (Video)";

struct player_info_t
{
    const char* name;
    const char* null_key;  /* HMAC-SHA1 key */
    const char* tl_key;    /* BlowFish key */
    bool big_endian;
    char *cinf;
};

static struct player_info_t zen_players[] =
{
    {"Zen Vision:M", null_key_v2, tl_zvm_key, false, NULL},
    {"Zen Vision:M 60GB", null_key_v2, tl_zvm60_key, false, NULL},
    {"Zen", null_key_v4, tl_zen_key, false, "Creative ZEN"},
    {"Zen X-Fi", null_key_v4, tl_zenxf_key, false, "Creative ZEN X-Fi"},
    {"Zen Mozaic", null_key_v4, tl_zenmo_key, false, "Creative ZEN Mozaic"},
    {"Zen Vision", null_key_v2, tl_zv_key, false, NULL},
    {"Zen Vision W", null_key_v2, tl_zvw_key, false, NULL},
    {"Zen Micro", null_key_v1, tl_zm_key, true, NULL},
    {"Zen MicroPhoto", null_key_v1, tl_zmp_key, true, NULL},
    {"Zen Sleek", null_key_v1, tl_zs_key, true, NULL},
    {"Zen SleekPhoto", null_key_v1, tl_zsp_key, true, NULL},
    {"Zen Touch", null_key_v1, tl_zt_key, true, NULL},
    {"Zen Xtra", null_key_v1, tl_zx_key, true, NULL},
    {"Zen V", null_key_v3, tl_zenv_key, false, "Creative ZEN V"},
    {"Zen V Plus", null_key_v3, tl_zenvp_key, false, NULL},
    {"Zen V Video", null_key_v3, tl_zenvv_key, false, NULL},
    {NULL, NULL, NULL, false, NULL}
};

/**
 * Information on how to patch firmwares
 */
struct zen_model_desc_t
{
    /* Descriptive name of this model (must match player in zen_players[]) */
    const char *model_name;
    /* Model name used in the Rockbox header in ".zen" files - these match the
       -add parameter to the "scramble" tool */
    const char *rb_model_name;
    /* Model number used to initialise the checksum in the Rockbox header in
       ".zen" files - these are the same as MODEL_NUMBER in config-target.h */
    const int rb_model_num;
    /* Bootloader load address */
    uint32_t bootloader_addr;
    /* Dualboot code for this model */
    const unsigned char *dualboot;
    /* Size of dualboot functions for this model */
    int dualboot_size;
};

/* keep this consistent with the address in dualboot.lds */
static const struct zen_model_desc_t zen_models[] =
{
    [MODEL_UNKNOWN] =
    {
        "Unknown", "    ", 0, 0, NULL, 0
    },
    [MODEL_ZENV] =
    {
        "Zen V", "zenv", 92, 0x61000000, dualboot_zenv, sizeof(dualboot_zenv)
    },
    [MODEL_ZENXFI] =
    {
        "Zen X-Fi", "zxfi", 86, 0x41000000, dualboot_zenxfi, sizeof(dualboot_zenxfi)
    },
    [MODEL_ZENMOZAIC] =
    {
        "Zen Mozaic", "zmoz", 87, 0x41000000, dualboot_zenmozaic, sizeof(dualboot_zenmozaic)
    },
    [MODEL_ZEN] =
    {
        "Zen", "zen", 90, 0x41000000, dualboot_zen, sizeof(dualboot_zen)
    },
};

/**
 * MD5 knowledge base
 */

struct zen_md5sum_t
{
    /* Device model */
    enum zen_model_t model;
    /* md5sum of the file */
    char *md5sum;
    /* Version string */
    const char *version;
};

static const struct zen_md5sum_t zen_sums[] =
{
    /** Zen Mozaic */
    {
        /* Version 1.06.01 */
       MODEL_ZENMOZAIC, "8441402a8db9f92659b05f05c0abe8fb", "1.06.01"
    },
    {
        /* Version 1.06.01e */
        MODEL_ZENMOZAIC, "88a856f8273b2bc3fcacf1f067a44aa8", "1.06.01e"
    },
    /** Zen X-Fi */
    {
        /* Version 1.04.08e */
        MODEL_ZENXFI, "f07e2e75069289a2aa14c6583bd9643b", "1.04.08e"
    },
    {
        /* Version 1.04.08 */
        MODEL_ZENXFI, "c3cddf8468d8c8982e93aa9986c5a152", "1.04.08"
    },
    /** Zen V */
    {
        /* Version 1.32.01e */
        MODEL_ZENV, "2f6d3e619557583c30132ac87221bc3e", "1.32.01e"
    },
    /** Zen */
    {
        /* Version 1.21.03e */
        MODEL_ZEN, "1fe28f587f87ac3c280281db28c42465", "1.21.03e"
    }
};

#define NR_ZEN_PLAYERS  (sizeof(zen_players) / sizeof(zen_players[0]))
#define NR_ZEN_SUMS     (sizeof(zen_sums) / sizeof(zen_sums[0]))
#define NR_ZEN_MODELS   (sizeof(zen_models) / sizeof(zen_models[0]))

#define MAGIC_ROCK      0x726f636b /* 'rock' */
#define MAGIC_RECOVERY  0xfee1dead
#define MAGIC_NORMAL    0xcafebabe

/**
 * Stolen from various places in our codebase
 */

/**
 * EDOC file format
 */
struct edoc_header_t
{
    char magic[4];
    uint32_t total_size;
    uint32_t zero;
};

struct edoc_section_header_t
{
    uint32_t addr;
    uint32_t size;
    uint32_t checksum;
};

uint32_t edoc_checksum(void *buffer, size_t size)
{
    uint32_t c = 0;
    uint32_t *p = buffer;
    while(size >= 4)
    {
        c += *p + (*p >> 16);
        p++;
        size -= 4;
    }
    if(size != 0)
        printf("[WARN] EDOC Checksum section size is not a multiple of 4 bytes, result is undefined!\n");
    return c & 0xffff;
}

#define errorf(err, ...) do { printf(__VA_ARGS__); return err; } while(0)

/**
 * How does patching code work
 * ---------------------------
 *
 * All Creative firmwares work the same: they start at 0 and the code sequence at
 * 0 always contains the vector table with ldr with offsets:
 *   0:       e59ff018        ldr     pc, [pc, #24]   ; 0x20
 *   4:       e59ff018        ldr     pc, [pc, #24]   ; 0x24
 *   8:       e59ff018        ldr     pc, [pc, #24]   ; 0x28
 *   c:       e59ff018        ldr     pc, [pc, #24]   ; 0x2c
 *  10:       e59ff018        ldr     pc, [pc, #24]   ; 0x30
 *  14:       e59ff018        ldr     pc, [pc, #24]   ; 0x34
 *  18:       e59ff018        ldr     pc, [pc, #24]   ; 0x38
 *  1c:       e59ff018        ldr     pc, [pc, #24]   ; 0x3c
 *  20:       0000dbd4        .word start
 *  24:       0000dcac        .word undef_instr_handler
 *  28:       0000dcb0        .word software_int_handler
 *  2c:       0000dcb4        .word prefetch_abort_handler
 *  30:       0000dcb8        .word data_abort_handler
 *  34:       0000dcbc        .word reserved_handler
 *  38:       0000dcc0        .word irq_handler
 *  3c:       0000dd08        .word fiq_handler
 *
 * To build a dual-boot image, we modify the start address to point to some
 * code we added to the image. Specifically we first add the stub, then
 * the rockbox image. We also write the old start address to this
 * stub so that it can either decide to run rockbox or patch back the
 * start address and jump to 0.
 * Singleboot and recovery is handled the same way except that both targets use
 * the same address and we drop the OF, so we create a fake vector table!
 */

struct dualboot_footer_t
{
    uint32_t magic;
    uint32_t of_addr;
    uint32_t rb_addr;
    uint32_t boot_arg;
} __attribute__((packed));

#define FOOTER_MAGIC    0x1ceb00da

static enum zen_error_t create_fake_image(uint8_t **fw, uint32_t *fw_size)
{
    /** We need to create a fake EDOC image, so first a header and one section
     * header with one data chunk. */
    /** The fake image is as follows:
     *   0:       e59ff018        ldr     pc, [pc, #24]   ; 0x20
     *   4:       e59ff018        ldr     pc, [pc, #24]   ; 0x24
     *   8:       e59ff018        ldr     pc, [pc, #24]   ; 0x28
     *   c:       e59ff018        ldr     pc, [pc, #24]   ; 0x2c
     *  10:       e59ff018        ldr     pc, [pc, #24]   ; 0x30
     *  14:       e59ff018        ldr     pc, [pc, #24]   ; 0x34
     *  18:       e59ff018        ldr     pc, [pc, #24]   ; 0x38
     *  1c:       e59ff018        ldr     pc, [pc, #24]   ; 0x3c
     *  20:       00000040        .word   hang
     *  24:       00000040        .word   hang
     *  28:       00000040        .word   hang
     *  2c:       00000040        .word   hang
     *  30:       00000040        .word   hang
     *  34:       00000040        .word   hang
     *  38:       00000040        .word   hang
     *  3c:       00000040        .word   hang
     *  40 <hang>:
     *  40:       eafffffe        b       40 <hang> */
    *fw_size = sizeof(struct edoc_header_t) + sizeof(struct edoc_section_header_t) + 0x44;
    *fw = malloc(*fw_size);
    if(*fw == NULL)
        errorf(ZEN_ERROR, "[ERR] Allocation failed");
    struct edoc_header_t *hdr = (void *)*fw;
    memcpy(hdr->magic, "EDOC", 4);
    hdr->total_size = *fw_size - sizeof(struct edoc_header_t) + 4;
    hdr->zero = 0;
    struct edoc_section_header_t *sec = (void *)(hdr + 1);
    sec->addr = 0;
    sec->size = 0x44;
    uint32_t *p = (void *)(sec + 1);
    p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = p[7] = 0xe59ff018;
    p[8] = p[9] = p[10] = p[11] = p[12] = p[13] = p[14] = p[15] = 0x40;
    p[16] = 0xeafffffe;
    sec->checksum = edoc_checksum(p, 0x44);
    return ZEN_SUCCESS;
}

static enum zen_error_t patch_firmware(uint8_t **fw, uint32_t *fw_size,
    void *boot, size_t boot_size, struct zen_option_t opt)
{
    /* check if dualboot stub is available */
    const void *dualboot = zen_models[opt.model].dualboot;
    int dualboot_size = zen_models[opt.model].dualboot_size;
    uint32_t dualboot_addr = zen_models[opt.model].bootloader_addr;
    if(dualboot == NULL)
        errorf(ZEN_DONT_KNOW_HOW_TO_PATCH, "[ERR] I don't have a dualboot stub for this model\n");
    /* if not asked to dualboot, drop OF and create a fake image */
    if(opt.output != ZEN_DUALBOOT)
    {
        enum zen_error_t ret = create_fake_image(fw, fw_size);
        if(ret != ZEN_SUCCESS)
            return ret;
    }
    /* compute final image size: add stub + bootloader in one block as a section */
    int extra_size = sizeof(struct edoc_section_header_t) + dualboot_size + boot_size;
    *fw_size += extra_size;
    *fw = realloc(*fw, *fw_size);
    if(*fw == NULL)
        errorf(ZEN_ERROR, "[ERR] Allocation failed");
    /* sanity check */
    struct edoc_header_t *hdr = (void *)*fw;
    if(memcmp(hdr->magic, "EDOC", 4) != 0)
        errorf(ZEN_FW_INVALID, "[ERR] Firmware doesn't use EDOC format\n");
    /* validate image and find OF start addr */
    uint32_t of_addr = 0;
    struct edoc_section_header_t *sec_hdr = (void *)(hdr + 1);
    while((void *)sec_hdr - (void *)&hdr->zero < hdr->total_size)
    {
        if(sec_hdr->checksum != edoc_checksum(sec_hdr + 1, sec_hdr->size))
            errorf(ZEN_FW_INVALID, "[ERR] Firmware checksum error\n");
        if(sec_hdr->addr == 0)
        {
            uint32_t *start_vector = ((void *)(sec_hdr + 1) + 0x20);
            /* extract address */
            of_addr = *(uint32_t *)start_vector;
            /* patch vector */
            *start_vector = dualboot_addr;
            /* fix checksum */
            sec_hdr->checksum = edoc_checksum(sec_hdr + 1, sec_hdr->size);
        }
        sec_hdr = (void *)(sec_hdr + 1) + sec_hdr->size;
    }
    if(of_addr == 0)
        errorf(ZEN_FW_INVALID, "[ERR] Firmware doesn't have the expected format\n");
    printf("[INFO] OF start address: %#x\n", of_addr);
    /* add extra section */
    sec_hdr->addr = dualboot_addr;
    sec_hdr->size = dualboot_size + boot_size;
    /* add extra data */
    memcpy(sec_hdr + 1, dualboot, dualboot_size);
    memcpy((void *)(sec_hdr + 1) + dualboot_size, boot, boot_size);
    /* locate and patch dualboot footer */
    struct dualboot_footer_t *footer = (void *)(sec_hdr + 1) + dualboot_size -
        sizeof(struct dualboot_footer_t);
    if(footer->magic != FOOTER_MAGIC)
        errorf(ZEN_FW_INVALID, "[ERR] Footer magic mismatch\n");
    uint32_t rb_addr = dualboot_addr + dualboot_size;
    printf("[INFO] RB start address: %#x\n", rb_addr);
    footer->of_addr = opt.output == ZEN_DUALBOOT ? of_addr : rb_addr;
    footer->rb_addr = rb_addr;
    footer->boot_arg = opt.output == ZEN_RECOVERY ? 0xfee1dead : 0xcafebabe;
    printf("[INFO] Footer: 0x%08x 0x%08x 0x%08x\n", footer->of_addr, footer->rb_addr,
        footer->boot_arg);
    /* fix image */
    sec_hdr->checksum = edoc_checksum(sec_hdr + 1, sec_hdr->size);
    hdr->total_size += extra_size;
    return ZEN_SUCCESS;
}

struct player_info_t *get_player_info(enum zen_model_t model)
{
    for(int i = 0; zen_players[i].name; i++)
        if(strcmp(zen_models[model].model_name, zen_players[i].name) == 0)
            return &zen_players[i];
    return NULL;
}

enum zen_error_t build_firmware(void *exec, size_t exec_size, void *boot, size_t boot_size,
    const char *outfile, struct zen_option_t opt)
{
    uint8_t *buffer = exec;
    /** find player info */
    struct player_info_t *player = get_player_info(opt.model);
    if(player == NULL)
        errorf(ZEN_UNSUPPORTED, "[ERR] There is no player info for this model\n");
    if(player->big_endian)
        errorf(ZEN_UNSUPPORTED, "[ERR] Big-endian players are currently unsupported\n");

    /** Find Win32 PE .data section */
    uint32_t data_ptr;
    uint32_t data_size;
    enum zen_error_t err = find_pe_data(exec, exec_size, &data_ptr, &data_size);
    if(err != ZEN_SUCCESS)
        errorf(err, "[ERR] Cannot find .data section\n");
    printf("[INFO] .data section is at 0x%x with size 0x%x\n", data_ptr, data_size);

    /** look for firmware and key in data section */
    uint32_t fw_offset = find_firmware_offset(&buffer[data_ptr], data_size);
    if(fw_offset == 0)
        errorf(ZEN_FW_INVALID, "[ERR] Couldn't find firmware offset\n");
    uint32_t fw_size = le2int(&buffer[data_ptr + fw_offset]);
    printf("[INFO] Firmware offset is at 0x%x with size 0x%x\n", data_ptr + fw_offset, fw_size);
    const char *fw_key = find_firmware_key(exec, exec_size);
    if(fw_key == NULL)
        errorf(ZEN_FW_INVALID, "[ERR] Couldn't find firmware key\n");
    printf("[INFO] Firmware key is %s\n", fw_key);

    /** descramble firmware */
    printf("[INFO] Descrambling firmware... ");
    if(!crypt_firmware(fw_key, &buffer[data_ptr + fw_offset + 4], fw_size))
        errorf(ZEN_ERROR, "Fail!\n");
    else
        printf("Done!\n");
    /** decompress it */
    uint8_t *out_buffer = malloc(fw_size * 2);
    if(out_buffer == NULL)
        errorf(ZEN_ERROR, "[ERR] Couldn't allocate memory");
    memset(out_buffer, 0, fw_size * 2);
    printf("[INFO] Decompressing firmware... ");
    char *err_msg;
    if(!inflate_to_buffer(&buffer[data_ptr + fw_offset + 4], fw_size, out_buffer,
            fw_size * 2, &err_msg))
        errorf(ZEN_ERROR, "Fail!\n[ERR] ZLib error: %s\n", err_msg);
    else
        printf("Done!\n");

    /** check format and resize the buffer */
    if(memcmp(out_buffer, "FFIC", 4) != 0)
        errorf(ZEN_FW_INVALID, "[ERR] CIFF header doesn't match\n");
    uint32_t ciff_size = le2int(&out_buffer[4]) + 8 + 28; /* CIFF block + NULL block*/
    printf("[INFO] Total firmware size: %d\n", ciff_size);
    out_buffer = realloc(out_buffer, ciff_size);
    if(out_buffer == NULL)
        errorf(ZEN_ERROR, "[ERR] Cannot resize memory block\n");

    /** look for firmware file */
    printf("[INFO] Locating encoded block... ");
    uint32_t fw_off = 8;
    uint8_t *cinf_ptr = NULL;
    while(memcmp(&out_buffer[fw_off], " LT\xa9", 4) != 0 && fw_off < ciff_size)
    {
        if(memcmp(&out_buffer[fw_off], "FNIC", 4) == 0)
        {
            cinf_ptr = &out_buffer[fw_off + 8];
            fw_off += 4 + 4 + 96;
        }
        else if(memcmp(&out_buffer[fw_off], "ATAD", 4) == 0)
        {
            fw_off += 4;
            fw_off += le2int(&out_buffer[fw_off]);
            fw_off += 4;
        }
        else
            errorf(ZEN_FW_INVALID, "Fail!\n[ERR] Unknown block\n");
    }
    if(fw_off >= ciff_size || memcmp(&out_buffer[fw_off], " LT\xa9", 4) != 0)
        errorf(ZEN_FW_INVALID, "Fail!\n[ERR] Couldn't find encoded block\n");
    if(!cinf_ptr)
        errorf(ZEN_FW_INVALID, "Fail!\n[ERR] Couldn't find CINF\n");
    printf("Done!\n");

    /** validate player if possible */
    printf("[INFO] Checking player model...");
    if(player->cinf)
    {
        char cinf_ascii[96];
        for(int j = 0; j < 96; j++)
            cinf_ascii[j] = *(unsigned short *)&cinf_ptr[2 * j];
        if(strncmp(cinf_ascii, player->cinf, 96) != 0)
            errorf(ZEN_FW_MISMATCH, "Fail!\n[ERR] Player mismatch: CINF indicates '%s' instead of '%s'\n",
                cinf_ascii, player->cinf);
        else
            printf("Done!\n");
    }
    else
        printf("Bypass!\n");

    /** decrypt firmware */
    printf("[INFO] Decrypting encoded block... ");
    uint32_t iv[2];
    iv[0] = 0;
    iv[1] = swap(le2int(&out_buffer[fw_off + 4]));
    if(!bf_cbc_decrypt((unsigned char*)player->tl_key, strlen(player->tl_key) + 1,
            &out_buffer[fw_off + 8], le2int(&out_buffer[fw_off + 4]), (const unsigned char*)&iv))
        errorf(ZEN_ERROR, "Fail!\n[ERR] Couldn't decrypt encoded block\n");
    printf("Done!\n");

    /** sanity checks on firmware */
    uint32_t jrm_size = le2int(&out_buffer[fw_off + 8]);
    if(jrm_size > le2int(&out_buffer[fw_off + 4]) * 3)
        errorf(ZEN_FW_INVALID, "[ERR] Decrypted length of encoded block is unexpectedly large: 0x%08x\n", jrm_size);
    printf("[INFO] Firmware size: %d\n", jrm_size);
    uint8_t *jrm = malloc(jrm_size);
    if(jrm == NULL)
        errorf(ZEN_ERROR, "[ERR] Couldn't allocate memory\n");
    memset(buffer, 0, jrm_size);

    /** decompress firmware */
    printf("[INFO] Decompressing encoded block... ");
    if(!cenc_decode(&out_buffer[fw_off + 12], le2int(&out_buffer[fw_off + 4]) - 4, jrm, jrm_size))
        errorf(ZEN_ERROR, "Fail!\n[ERR] Couldn't decompress the encoded block\n");
    printf("Done!\n");

    /** Copy OF because patching might modify it */
    void *jrm_save = malloc(jrm_size);
    uint32_t jrm_save_size = jrm_size;
    if(jrm_save == NULL)
        errorf(ZEN_ERROR, "[ERR] Couldn't allocate memory");
    memcpy(jrm_save, jrm, jrm_size);

    /** Patch firmware */
    err = patch_firmware(&jrm, &jrm_size, boot, boot_size, opt);
    if(err != ZEN_SUCCESS)
        errorf(err, "[ERR] Couldn't patch firmware\n");

    /** Rebuild archive */
    bool keep_old_bits = opt.output == ZEN_DUALBOOT || opt.output == ZEN_MIXEDBOOT;
    bool keep_of = opt.output == ZEN_MIXEDBOOT;
    /* if we keep old stuff, keep everything up to LT block, otherwise just CIFF header */
    uint32_t off = keep_old_bits ? fw_off : 8;
    /* move the rest of the archive if keeping old stuff */
    if(keep_old_bits)
    {
        uint32_t copy_off = fw_off + 8 + le2int(&out_buffer[fw_off + 4]);
        uint32_t copy_size = ciff_size - fw_off - 8 - le2int(&out_buffer[fw_off + 4]) - 28;
        memmove(&out_buffer[off], &out_buffer[copy_off], copy_size);
        off += copy_size;
    }
    /* if we keep the OF, put a copy of it after renaming it to Hcreativeos.jrm */
    if(keep_of)
    {
        out_buffer = realloc(out_buffer, off + jrm_save_size + 40);
        if(out_buffer == NULL)
            errorf(ZEN_ERROR, "[ERR] Couldn't resize memory block\n");
        printf("[INFO] Renaming encoded block to Hcreativeos.jrm... ");
        memcpy(&out_buffer[off], "ATAD", 4);
        int2le(jrm_save_size + 32, &out_buffer[off + 4]);
        memset(&out_buffer[off + 8], 0, 32);
        memcpy(&out_buffer[off + 8], "H\0c\0r\0e\0a\0t\0i\0v\0e\0o\0s\0.\0j\0r\0m", 30);
        memcpy(&out_buffer[off + 40], jrm_save, jrm_save_size);
        off += jrm_save_size + 40;
        printf("Done!\n");
    }
    /* put modified firmware */
    out_buffer = realloc(out_buffer, off + jrm_size + 40);
    if(out_buffer == NULL)
        errorf(ZEN_ERROR, "[ERR] Couldn't resize memory block\n");
    printf("[INFO] Adding Hjukebox2.jrm... ");
    memcpy(&out_buffer[off], "ATAD", 4);
    int2le(jrm_size + 32, &out_buffer[off + 4]);
    memset(&out_buffer[off + 8], 0, 32);
    memcpy(&out_buffer[off + 8], "H\0j\0u\0k\0e\0b\0o\0x\0""2\0.\0j\0r\0m", 26);
    memcpy(&out_buffer[off + 40], jrm, jrm_size);
    off += jrm_size + 40;
    printf("Done!\n");

    /** fix header */
    int2le(off - 8, &out_buffer[4]);

    /** update checksum */
    printf("[INFO] Updating checksum... ");
    out_buffer = realloc(out_buffer, off + 28);
    if(out_buffer == NULL)
        errorf(ZEN_ERROR, "[ERR] Couldn't resize memory block\n");
    memcpy(&out_buffer[off], "LLUN", 4);
    int2le(20, &out_buffer[off + 4]);
    hmac_sha1((unsigned char*)player->null_key, strlen(player->null_key), out_buffer,
        off, &out_buffer[off + 8]);
    off += 28;
    printf("Done!\n");

    err = write_file(outfile, out_buffer, off);

    free(jrm);
    free(jrm_save);
    free(out_buffer);
    return err;
}

/* find an entry into zen_sums which matches the MD5 sum of a file */
static enum zen_error_t find_model_by_md5sum(uint8_t file_md5sum[16], int *md5_idx)
{
    int i = 0;
    while(i < NR_ZEN_SUMS)
    {
        uint8_t md5[20];
        if(strlen(zen_sums[i].md5sum) != 32)
            errorf(ZEN_ERROR, "[ERR][INTERNAL] Invalid MD5 sum in zen_sums\n");
        for(int j = 0; j < 16; j++)
        {
            uint8_t a, b;
            if(convxdigit(zen_sums[i].md5sum[2 * j], &a) || convxdigit(zen_sums[i].md5sum[2 * j + 1], &b))
                errorf(ZEN_ERROR, "[ERR][INTERNAL] Bad checksum format: %s\n", zen_sums[i].md5sum);
            md5[j] = (a << 4) | b;
        }
        if(memcmp(file_md5sum, md5, 16) == 0)
            break;
        i++;
    }
    if(i == NR_ZEN_SUMS)
        errorf(ZEN_NO_MATCH, "[ERR] MD5 sum doesn't match any known file\n");
    *md5_idx = i;
    return ZEN_SUCCESS;
}

enum zen_error_t mkzenboot(const char *infile, const char *bootfile,
    const char *outfile, struct zen_option_t opt)
{
    /* determine firmware model */
    void *fw;
    size_t fw_size;
    enum zen_error_t err = read_file(infile, &fw, &fw_size);
    uint8_t file_md5sum[16];
    err = compute_md5sum_buf(fw, fw_size, file_md5sum);
    if(err != ZEN_SUCCESS)
    {
        free(fw);
        return err;
    }
    printf("[INFO] MD5 sum of the file: ");
    for(int i = 0; i < 16; i++)
        printf("%02X ", file_md5sum[i]);
    printf("\n");
    if(opt.model == MODEL_UNKNOWN)
    {
        int idx;
        err = find_model_by_md5sum(file_md5sum, &idx);
        if(err != ZEN_SUCCESS)
        {
            free(fw);
            errorf(err, "[ERR] Cannot determine model type\n");
        }
        opt.model = zen_sums[idx].model;
        printf("[INFO] MD5 matches %s, version %s\n",
            zen_models[opt.model].model_name, zen_sums[idx].version);
    }
    printf("[INFO] Model is: %s\n", zen_models[opt.model].model_name);
    /* load rockbox file */
    uint8_t *boot;
    size_t boot_size;
    err = read_file(bootfile, (void **)&boot, &boot_size);
    if(err != ZEN_SUCCESS)
    {
        free(fw);
        errorf(err, "[ERR] Cannot read boot file\n");
    }
    /* validate checksum */
    if(memcmp(boot + 4, zen_models[opt.model].rb_model_name, 4) != 0)
    {
        free(fw);
        free(boot);
        errorf(ZEN_BOOT_MISMATCH, "[ERR] Boot model mismatch\n");
    }
    printf("[INFO] Bootloader file matches model\n");
    uint32_t sum = zen_models[opt.model].rb_model_num;
    for(int i = 8; i < boot_size; i++)
        sum += boot[i];
    if(sum != be2int(boot))
    {
        free(fw);
        free(boot);
        errorf(ZEN_BOOT_CHECKSUM_ERROR, "[ERR] Checksum mismatch\n");
    }
    printf("[INFO] Bootloader file checksum is correct\n");
    /* produce file */
    err = build_firmware(fw, fw_size, boot + 8, boot_size - 8, outfile, opt);
    free(boot);
    free(fw);
    return err;
}
