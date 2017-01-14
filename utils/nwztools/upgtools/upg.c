/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#include "upg.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "md5.h"

struct nwz_model_t g_model_list[] =
{
    { "nwz-a10", true, "2572f4a7b8c1a08aeb5142ce9cb834d6" },
    { "nw-a20", true, "d91a61c7263bafc626e9a5b66f983c0b" },
    { "nwz-e450", true, "8a01b624bfbfde4a1662a1772220e3c5" },
    { "nwz-e460", true, "89d813f8f966efdebd9c9e0ea98156d2" },
    { "nwz-a860", true, "a7c4af6c28b8900a783f307c1ba538c5" },
    { "nwz-a850", true, "a2efb9168616c2e84d78291295c1aa5d" },
    { "nwz-e470", true, "e4144baaa2707913f17b5634034262c4" },
    { "nwz-e580", true, "6e25f79812eca7ceed04819d833e80af" },
    { "nwz-s750", true, "6d4f4d9adec781baf197e6255cedd0f6" },
    { "nw-zx100", true, "cdda8d5e5360fd4373154388743f84d2" },
    /* The following keys were obtained by brute forcing firmware upgrades,
     * someone with a device needs to confirm that they work */
    { "nw-a820", false, "0c9869c268e0eaa6d1ba62daab09cebc" },
    /* The following models use a different encryption, but we put the KAS here
     * to not forget them */
    { "nw-a30", false, "c40d91e7efff3e3aa5c8831dd85526fe4972086283419c8cd8fa3b7dcd39" },
    { "nw-wm1", false, "e8d171a5d92f35eed9658c03fb9f86a169591659851fd7c49525f587a70b" },
    { 0 }
};

static int digit_value(char c)
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char hex_digit(unsigned v)
{
    return (v < 10) ? v + '0' : (v < 16) ? v - 10 + 'a' : 'x';
}

int decrypt_keysig(const char kas[NWZ_KAS_SIZE], char key[NWZ_KEY_SIZE],
    char sig[NWZ_SIG_SIZE])
{
    uint8_t src[NWZ_KAS_SIZE / 2];
    for(int index = 0; index < NWZ_KAS_SIZE / 2; index++)
    {
        int a = digit_value(kas[index * 2]);
        int b = digit_value(kas[index * 2 + 1]);
        if(a < 0 || b < 0)
            return -1;
        src[index] = a << 4 | b;
    }
    fwp_setkey("ed295076");
    fwp_crypt(src, sizeof(src), 1);
    memcpy(key, src, NWZ_KEY_SIZE);
    memcpy(sig, src + NWZ_KEY_SIZE, NWZ_SIG_SIZE);
    return 0;
}

void encrypt_keysig(char kas[NWZ_KEY_SIZE],
    const char key[NWZ_SIG_SIZE], const char sig[NWZ_KAS_SIZE])
{
    uint8_t src[NWZ_KAS_SIZE / 2];
    fwp_setkey("ed295076");
    memcpy(src, key, NWZ_KEY_SIZE);
    memcpy(src + NWZ_KEY_SIZE, sig, NWZ_SIG_SIZE);
    fwp_crypt(src, sizeof(src), 0);
    for(int i = 0; i < NWZ_KAS_SIZE / 2; i++)
    {
        kas[2 * i] = hex_digit((src[i] >> 4) & 0xf);
        kas[2 * i + 1] = hex_digit(src[i] & 0xf);
    }
}

struct upg_file_t *upg_read_memory(void *buf, size_t size, char key[NWZ_KEY_SIZE],
    char *sig, void *u, generic_printf_t printf)
{
#define cprintf(col, ...) printf(u, false, col, __VA_ARGS__)
#define cprintf_field(str1, ...) do{ cprintf(GREEN, str1); cprintf(YELLOW, __VA_ARGS__); }while(0)
#define err_printf(col, ...) printf(u, true, col, __VA_ARGS__)
    struct upg_md5_t *md5 = buf;
    cprintf(BLUE, "Preliminary\n");
    cprintf(GREEN, "  MD5: ");
    for(int i = 0; i < NWZ_MD5_SIZE; i++)
        cprintf(YELLOW, "%02x", md5->md5[i]);
    cprintf(OFF, " ");

    /* check MD5 */
    uint8_t actual_md5[NWZ_MD5_SIZE];
    MD5_CalculateDigest(actual_md5, (md5 + 1), size - sizeof(struct upg_header_t));
    if(memcmp(actual_md5, md5->md5, NWZ_MD5_SIZE) != 0)
    {
        cprintf(RED, "Mismatch\n");
        err_printf(GREY, "MD5 Mismatch\n");
        return NULL;
    }
    cprintf(RED, "Ok\n");

    struct upg_header_t *hdr = (void *)(md5 + 1);
    /* decrypt the whole file at once */
    fwp_read(hdr, size - sizeof(*md5), hdr, (void *)key);

    cprintf(BLUE, "Header\n");
    cprintf_field("  Signature:", " ");
    for(int i = 0; i < NWZ_SIG_SIZE; i++)
        cprintf(YELLOW, "%c", isprint(hdr->sig[i]) ? hdr->sig[i] : '.');
    if(sig)
    {
        if(memcmp(hdr->sig, sig, NWZ_SIG_SIZE) != 0)
        {
            cprintf(RED, "Mismatch\n");
            err_printf(GREY, "Signature Mismatch\n");
            return NULL;
        }
        cprintf(RED, " Ok\n");
    }
    else
        cprintf(RED, " Can't check\n");
    cprintf_field("  Files: ", "%d\n", hdr->nr_files);
    cprintf_field("  Pad: ", "0x%x\n", hdr->pad);


    /* Do a first pass to decrypt in-place */
    cprintf(BLUE, "Files\n");
    struct upg_entry_t *entry = (void *)(hdr + 1);
    for(unsigned i = 0; i < hdr->nr_files; i++, entry++)
    {
        cprintf(GREY, "  File");
        cprintf(RED, " %d\n", i);
        cprintf_field("    Offset: ", "0x%x\n", entry->offset);
        cprintf_field("    Size: ", "0x%x\n", entry->size);
    }
    /* Do a second pass to create the file structure */
    /* create file */
    struct upg_file_t *file = malloc(sizeof(struct upg_file_t));
    memset(file, 0, sizeof(struct upg_file_t));
    file->nr_files = hdr->nr_files;
    file->files = malloc(sizeof(struct upg_file_entry_t) * file->nr_files);

    entry = (void *)(hdr + 1);
    for(unsigned i = 0; i < hdr->nr_files; i++, entry++)
    {
        memset(&file->files[i], 0, sizeof(struct upg_file_entry_t));
        file->files[i].size = entry->size;
        file->files[i].data = malloc(file->files[i].size);
        memcpy(file->files[i].data, buf + entry->offset, entry->size);
    }

    return file;
}

void *upg_write_memory(struct upg_file_t *file, char key[NWZ_KEY_SIZE],
    char sig[NWZ_SIG_SIZE], size_t *out_size, void *u, generic_printf_t printf)
{
    if(file->nr_files == 0)
    {
        err_printf(GREY, "A UPG file must have at least one file\n");
        return NULL;
    }
    /* compute total size and create buffer */
    size_t tot_size = sizeof(struct upg_md5_t) + sizeof(struct upg_header_t)
        + file->nr_files * sizeof(struct upg_entry_t);
    for(int i = 0; i < file->nr_files; i++)
        tot_size += ROUND_UP(file->files[i].size, 8);
    /* allocate buffer */
    void *buf = malloc(tot_size);

    /* create md5 context, we push data to the context as we create it */
    struct upg_md5_t *md5 = buf;
    memset(md5, 0, sizeof(*md5));
    /* create the encrypted signature and header */
    struct upg_header_t *hdr = (void *)(md5 + 1);
    memcpy(hdr->sig, sig, NWZ_SIG_SIZE);
    hdr->nr_files = file->nr_files;
    hdr->pad = 0;

    /* create file headers */
    size_t offset = sizeof(*md5) + sizeof(*hdr) + file->nr_files * sizeof(struct upg_entry_t);
    struct upg_entry_t *entry = (void *)(hdr + 1);
    cprintf(BLUE, "Files\n");
    for(int i = 0; i < file->nr_files; i++)
    {
        entry[i].offset = offset;
        entry[i].size = file->files[i].size;
        offset += ROUND_UP(entry[i].size, 8); /* pad each file to a multiple of 8 for encryption */

        cprintf(GREY, "  File");
        cprintf(RED, " %d\n", i);
        cprintf_field("    Offset: ", "0x%lx\n", entry[i].offset);
        cprintf_field("    Size: ", "0x%lx\n", entry[i].size);
    }

    /* add file data */
    for(int i = 0; i < file->nr_files; i++)
    {
        /* copy data to buffer, and then encrypt in-place */
        size_t r_size = ROUND_UP(file->files[i].size, 8);
        void *data_ptr = (uint8_t *)buf + entry[i].offset;
        memset(data_ptr, 0, r_size); /* the padding will be zero 0 */
        memcpy(data_ptr, file->files[i].data, file->files[i].size);
    }
    /* encrypt everything and hash everything */
    fwp_write(hdr, tot_size - sizeof(*md5), hdr, (void *)key);
    /* write final MD5 */
    MD5_CalculateDigest(md5->md5, (void *)hdr, tot_size - sizeof(*md5));
    *out_size = tot_size;
    return buf;
}

struct upg_file_t *upg_new(void)
{
    struct upg_file_t *f = malloc(sizeof(struct upg_file_t));
    memset(f, 0, sizeof(struct upg_file_t));
    return f;
}

void upg_append(struct upg_file_t *file, void *data, size_t size)
{
    file->files = realloc(file->files, (file->nr_files + 1) * sizeof(struct upg_file_entry_t));
    file->files[file->nr_files].data = data;
    file->files[file->nr_files].size = size;
    file->nr_files++;
}

void upg_free(struct upg_file_t *file)
{
    if(file)
    {
        for(int i = 0; i < file->nr_files; i++)
            free(file->files[i].data);
        free(file->files);
    }
    free(file);
}
