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

struct nwz_model_t g_model_list[] =
{
    { "nwz-a10", true, "2572f4a7b8c1a08aeb5142ce9cb834d6" },
    { "nw-a20", true, "d91a61c7263bafc626e9a5b66f983c0b" },
    { "nwz-e350", true, "8a01b624bfbfde4a1662a1772220e3c5" },
    { "nwz-e450", true, "8a01b624bfbfde4a1662a1772220e3c5" },
    { "nwz-e460", true, "89d813f8f966efdebd9c9e0ea98156d2" },
    { "nwz-a860", true, "a7c4af6c28b8900a783f307c1ba538c5" },
    { "nwz-a850", true, "a2efb9168616c2e84d78291295c1aa5d" },
    { "nwz-a840", true, "78033fe79a67786fd79fbc138c865c68" },
    { "nwz-e470", true, "e4144baaa2707913f17b5634034262c4" },
    { "nwz-e580", true, "6e25f79812eca7ceed04819d833e80af" },
    { "nwz-s750", true, "6d4f4d9adec781baf197e6255cedd0f6" },
    { "nwz-x1000",true, "efafdee4c628fa7536de0b878cfe23af" },
    { "nw-zx100", true, "cdda8d5e5360fd4373154388743f84d2" },
    /* The following models use a different encryption */
    { "nw-wm1a", true, "e8d171a5d92f35eed9658c03fb9f86a169591659851fd7c49525f587a70b526c" },
    { "nw-wm1z", true, "2b07114f06d0f63b8ef8e31c8bc9332c7bd70281f7f8d2f80dab58cd36f82c82" },
    { "nw-zx300", true, "3ab5bbcb999463c50aaa957496b066c6b76a25f4505bf5b42c0bc4815cbe3db6" },
    { "nw-a30", true, "c40d91e7efff3e3aa5c8831dd85526fe4972086283419c8cd8fa3b7dcd39dee4" },
    { "nw-a40", true, "a0d2b1317794074aff77dd2afb9c7aa6b28d6cf24a5e5eb60df87a87eb562de5" },
    { "nw-a50", true, "dd49de9dab2bce5a59090c01049576d537af6a313e5c0a2c24353937a87352d6" },
    { "dmp-z1", true, "2b07114f06d0f63b8ef8e31c8bc9332c7bd70281f7f8d2f80dab58cd36f82c82" },
    /* The following keys were obtained by brute forcing firmware upgrades,
     * someone with a device needs to confirm that they work */
    { "nw-a820", false, "0c9869c268e0eaa6d1ba62daab09cebc" },
    { "nw-s10", false, "20f65807a9506f9bc591123cea2c2291" },
    { "nwz-s610", false, "fe14a16d7c5c52cf56846d04305f994c"},
    { 0 }
};

/* KEY/IV for pre-WM1/A30 models */
static uint8_t g_des_passkey[9] = "ed295076";
/* device after WM1/NW-A30 */
static uint8_t g_aes_passkey[17] = "9cc4419c8bef488c";
static uint8_t g_aes_iv[17] = "6063ce1efa1d543a";

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

int decrypt_keysig(const char *kas, char **key, char **sig)
{
    int len = strlen(kas);
    if(len % 2)
        return -1; /* length must be a multiple of two */
    uint8_t *src = malloc(len / 2);
    for(int index = 0; index < len / 2; index++)
    {
        int a = digit_value(kas[index * 2]);
        int b = digit_value(kas[index * 2 + 1]);
        if(a < 0 || b < 0)
            return -1; /* bad digit */
        src[index] = a << 4 | b;
    }
    if(*key == NULL)
        *key = malloc(len / 4 + 1);
    if(*sig == NULL)
        *sig = malloc(len / 4 + 1);

    if(len == 32)
    {
        /* Device before WM1/NW-A30 use DES */
        des_ecb_dec_set_key(g_des_passkey);
        des_ecb_dec(src, len / 2, src);
    }
    else if(len == 64)
    {
        /* device after WM1/NW-A30 */
        aes_cbc_dec_set_key_iv(g_aes_passkey, g_aes_iv);
        aes_cbc_dec(src, len / 2, src);
    }
    else
    {
        free(src);
        return -1;
    }
    memcpy(*key, src, len / 4);
    (*key)[len / 4] = 0;
    memcpy(*sig, src + len / 4, len / 4);
    (*sig)[len / 4] = 0;
    free(src);
    return 0;
}

void encrypt_keysig(char **kas, const char *key, const char *sig)
{
    int len = strlen(key);
    if(len != strlen(sig))
        abort();
    uint8_t *src = malloc(len * 2);
    memcpy(src, key, len);
    memcpy(src + len, sig, len);
    if(len == 8)
    {
        des_ecb_enc_set_key(g_des_passkey);
        des_ecb_enc(src, len * 2, src);
    }
    else if(len == 16)
    {
        aes_cbc_enc_set_key_iv(g_aes_passkey, g_aes_iv);
        aes_cbc_enc(src, len * 2, src);
    }
    else
        abort();
    if(*kas == NULL)
        *kas = malloc(len * 4 + 1);
    for(int i = 0; i < len * 2; i++)
    {
        (*kas)[2 * i] = hex_digit((src[i] >> 4) & 0xf);
        (*kas)[2 * i + 1] = hex_digit(src[i] & 0xf);
    }
    (*kas)[len * 4] = 0;
    free(src);
}

struct upg_file_t *upg_read_memory(void *buf, size_t size, const char *key,
    const char *sig, void *u, generic_printf_t printf)
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

    int key_len = strlen(key);

    /* check MD5 */
    uint8_t actual_md5[NWZ_MD5_SIZE];
    MD5_CalculateDigest(actual_md5, (md5 + 1), size - NWZ_MD5_SIZE);
    if(memcmp(actual_md5, md5->md5, NWZ_MD5_SIZE) != 0)
    {
        cprintf(RED, "Mismatch\n");
        err_printf(GREY, "MD5 Mismatch\n");
        return NULL;
    }
    cprintf(RED, "Ok\n");

    bool is_v2 = false;
    void *hdr = (void *)(md5 + 1);
    /* decrypt just the header */
    if(key_len == 8)
    {
        des_ecb_dec_set_key((uint8_t *)key);
        des_ecb_dec(hdr, sizeof(struct upg_header_t), hdr);
    }
    else if(key_len == 16)
    {
        aes_ecb_dec_set_key((uint8_t *)key);
        aes_ecb_dec(hdr, sizeof(struct upg_header_v2_t), hdr);
        is_v2 = true;
    }
    else
    {
        cprintf(GREY, "I don't know how to decrypt with a key of length %d\n", key_len);
        return NULL;
    }

    cprintf(BLUE, "Header\n");
    int nr_files = 0;
    void *content = NULL;
    if(!is_v2)
    {
        struct upg_header_t *hdr_v1 = hdr;
        cprintf_field("  Signature: ", "");
        for(int i = 0; i < 8; i++)
            cprintf_field("", "%c", isprint(hdr_v1->sig[i]) ? hdr_v1->sig[i] : '.');
        if(sig)
        {
            if(memcmp(hdr_v1->sig, sig, 8) != 0)
            {
                cprintf(RED, " Mismatch\n");
                err_printf(GREY, "Signature Mismatch\n");
                return NULL;
            }
            cprintf(RED, " Ok\n");
        }
        else
            cprintf(RED, " Can't check\n");
        cprintf_field("  Files: ", "%d\n", hdr_v1->nr_files);
        if(hdr_v1->pad != 0)
            cprintf_field("  Pad: ", "0x%x\n", hdr_v1->pad);

        nr_files = hdr_v1->nr_files;
        content = hdr_v1 + 1;
    }
    else
    {
        struct upg_header_v2_t *hdr_v2 = hdr;
        cprintf_field("  Signature: ", "");
        for(int i = 0; i < 16; i++)
            cprintf_field("", "%c", isprint(hdr_v2->sig[i]) ? hdr_v2->sig[i] : '.');
        if(sig)
        {
            if(memcmp(hdr_v2->sig, sig, 16) != 0)
            {
                cprintf(RED, " Mismatch\n");
                err_printf(GREY, "Signature Mismatch\n");
                return NULL;
            }
            cprintf(RED, " Ok\n");
        }
        else
            cprintf(RED, " Can't check\n");
        cprintf_field("  Files: ", "%d\n", hdr_v2->nr_files);
        if(hdr_v2->pad[0] != 0 || hdr_v2->pad[1] != 0 || hdr_v2->pad[2] != 0)
            cprintf_field("  Pad: ", "0x%x 0x%x 0x%x\n", hdr_v2->pad[0], hdr_v2->pad[1], hdr_v2->pad[2]);

        nr_files = hdr_v2->nr_files;
        content = hdr_v2 + 1;
    }

    /* create file */
    struct upg_file_t *file = malloc(sizeof(struct upg_file_t));
    memset(file, 0, sizeof(struct upg_file_t));
    file->nr_files = nr_files;
    file->files = malloc(sizeof(struct upg_file_entry_t) * file->nr_files);

    /* decrypt the file list */
    if(key_len == 8)
        des_ecb_dec(content, sizeof(struct upg_entry_t) * nr_files, content);
    else if(key_len == 16)
        aes_ecb_dec(content, sizeof(struct upg_entry_v2_t) * nr_files, content);

    /* Extract files */
    cprintf(BLUE, "Files\n");
    struct upg_entry_t *entry_v1 = content;
    struct upg_entry_v2_t *entry_v2 = content;
    for(unsigned i = 0; i < nr_files; i++)
    {
        uint32_t offset, size;
        cprintf(GREY, "  File");
        cprintf(RED, " %d\n", i);
        if(!is_v2)
        {
            offset = entry_v1[i].offset;
            size = entry_v1[i].size;
        }
        else
        {
            offset = entry_v2[i].offset;
            size = entry_v2[i].size;
        }
        cprintf_field("    Offset: ", "0x%x\n", offset);
        cprintf_field("    Size: ", "0x%x\n", size);
        if(is_v2 && (entry_v2[i].pad[0] != 0 || entry_v2[i].pad[1] != 0))
            cprintf_field("    Pad:", " %x %x\n", entry_v2[i].pad[0], entry_v2[i].pad[1]);

        /* decrypt file content, we round up the size to make sure it's a multiple of 8/16 */
        if(key_len == 8)
            des_ecb_dec(buf + offset, ROUND_UP(size, 8), buf + offset);
        else if(key_len == 16)
        {
            aes_cbc_dec_set_key_iv((uint8_t *)key, (uint8_t *)g_aes_iv);
            aes_cbc_dec(buf + offset, ROUND_UP(size, 16), buf + offset);
        }

        /* in V2 of the format, some entries can be compressed using zlib but there is no marker for
         * that; instead the OF has the fwpup program that can extract the nth entry of the archive
         * and takes an optional -z flag to specify whether to uncompress(). Hence we don't support
         * that at the moment. */
        memset(&file->files[i], 0, sizeof(struct upg_file_entry_t));
        file->files[i].size = size;
        file->files[i].data = malloc(file->files[i].size);
        memcpy(file->files[i].data, buf + offset, size);
    }

    return file;
}

void *upg_write_memory(struct upg_file_t *file, const char *key,
    const char *sig, size_t *out_size, void *u, generic_printf_t printf)
{
    int key_len = strlen(key);
    if(strlen(sig) != key_len)
    {
        err_printf(GREY, "The key must have the same length as the signature\n");
        return NULL;
    }
    if(file->nr_files == 0)
    {
        err_printf(GREY, "A UPG file must have at least one file\n");
        return NULL;
    }

    bool is_v2 = false;
    size_t min_chunk_size, hdr_sz, ent_sz;
    if(key_len == 8)
    {
        min_chunk_size = 8;
        hdr_sz = sizeof(struct upg_header_t);
        ent_sz = sizeof(struct upg_entry_t);
    }
    else if(key_len == 16)
    {
        min_chunk_size = 4096; /* experimentally, V2 UPG files always seem to have 4k sizes */
        hdr_sz = sizeof(struct upg_header_v2_t);
        ent_sz = sizeof(struct upg_entry_v2_t);
        is_v2 = true;
    }
    else
    {
        cprintf(GREY, "I don't know how to decrypt with a key of length %s\n", key_len);
        return NULL;
    }

    /* V2 wants at least two files, the second of which is supposed to contain MD5 sums */
    if(is_v2 && file->nr_files == 1)
    {
        err_printf(RED, "This will probably not work: the firmware updater for this device expects at least two files in the archive.\n");
        err_printf(RED, "The first one is a shell script and the second is a MD5 file. You can probably put whatever you want in this file,\n");
        err_printf(RED, "even make it empty, but it needs to be there.\n");
        /* let it run just in case */
    }

    /* compute total size and create buffer */
    size_t tot_hdr_siz = hdr_sz + file->nr_files * ent_sz;
    size_t tot_size = sizeof(struct upg_md5_t) + tot_hdr_siz;
    for(int i = 0; i < file->nr_files; i++)
        tot_size += ROUND_UP(file->files[i].size, min_chunk_size);
    /* allocate buffer */
    void *buf = malloc(tot_size);

    /* create md5 context, we push data to the context as we create it */
    struct upg_md5_t *md5 = buf;
    memset(md5, 0, sizeof(*md5));
    /* create the encrypted signature and header */
    if(!is_v2)
    {
        struct upg_header_t *hdr = (void *)(md5 + 1);
        memcpy(hdr->sig, sig, 8);
        hdr->nr_files = file->nr_files;
        hdr->pad = 0;
    }
    else
    {
        struct upg_header_v2_t *hdr = (void *)(md5 + 1);
        memcpy(hdr->sig, sig, 16);
        hdr->nr_files = file->nr_files;
        hdr->pad[0] = hdr->pad[1] = hdr->pad[2] = 0;
    }

    /* create file headers */
    size_t offset = sizeof(struct upg_md5_t) + tot_hdr_siz;
    struct upg_entry_t *entry_v1 = (void *)((uint8_t *)(md5 + 1) + hdr_sz);
    struct upg_entry_v2_t *entry_v2 = (void *)entry_v1;
    cprintf(BLUE, "Files\n");
    for(int i = 0; i < file->nr_files; i++)
    {
        cprintf(GREY, "  File");
        cprintf(RED, " %d\n", i);
        cprintf_field("    Offset: ", "0x%lx\n", offset);
        cprintf_field("    Size: ", "0x%lx\n", file->files[i].size);
        /* copy data to buffer, with padding */
        size_t r_size = ROUND_UP(file->files[i].size, min_chunk_size);
        void *data_ptr = (uint8_t *)buf + offset;
        memset(data_ptr, 0, r_size); /* the padding will be zero 0 */
        memcpy(data_ptr, file->files[i].data, file->files[i].size);
        /* encrypt in-place */
        if(!is_v2)
        {
            des_ecb_enc_set_key((uint8_t *)key);
            des_ecb_enc(data_ptr, r_size, data_ptr);
        }
        else
        {
            aes_cbc_enc_set_key_iv((uint8_t *)key, (uint8_t *)g_aes_iv);
            aes_cbc_enc(data_ptr, r_size, data_ptr);
        }
        /* write header */
        if(!is_v2)
        {
            entry_v1[i].offset = offset;
            entry_v1[i].size = file->files[i].size;
        }
        else
        {
            entry_v2[i].offset = offset;
            entry_v2[i].size = r_size; /* the V2 seems to write the padded size here */
            entry_v2[i].pad[0] = entry_v2[i].pad[1] = 0;
        }

        offset += r_size;
    }
    /* encrypt headers */
    if(!is_v2)
    {
        des_ecb_enc_set_key((uint8_t *)key);
        des_ecb_enc(md5 + 1, tot_hdr_siz, md5 + 1);
    }
    else
    {
        aes_ecb_enc_set_key((uint8_t *)key);
        aes_ecb_enc(md5 + 1, tot_hdr_siz, md5 + 1);
    }
    /* compute MD5 of the whole file */
    MD5_CalculateDigest(md5->md5, md5 + 1, tot_size - sizeof(*md5));
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
