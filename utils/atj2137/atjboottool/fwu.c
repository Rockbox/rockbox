/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Amaury Pouly
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
#include <string.h>
#include <stdlib.h>
#include "misc.h"
#include "fwu.h"
#include "afi.h"

#define check_field(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); return 1; } \
    else { cprintf(RED, str_ok); }

#define FWU_SIG_SIZE    16
#define FWU_BLOCK_SIZE  512

struct fwu_hdr_t
{
    uint8_t sig[FWU_SIG_SIZE];
    uint32_t fw_size;
    uint32_t block_size;// always 512
    uint8_t version;
    uint8_t unk;
    uint8_t sig2[FWU_SIG_SIZE];
} __attribute__((packed));

const uint8_t g_fwu_signature[FWU_SIG_SIZE] =
{
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x75
};

struct version_desc_t
{
    uint8_t version;
    uint8_t value;
    uint8_t unk;
    uint8_t sig2[FWU_SIG_SIZE];
};

struct version_desc_t g_version[] =
{
    { 1, 0xd, 0xd0, { 0x76, 0x5c, 0x50, 0x94, 0x69, 0xb0, 0xa7, 0x03, 0x10, 0xf1, 0x7e, 0xdb, 0x88, 0x90, 0x86, 0x9d } },
    { 1, 0xe, 0xd0, { 0x92, 0x22, 0x7a, 0x77, 0x08, 0x67, 0xae, 0x06, 0x16, 0x06, 0xb8, 0x65, 0xa6, 0x42, 0xf7, 0X52 } },
    { 3, 0x7e, 0xe1, { 0x3f, 0xad, 0xf8, 0xb0, 0x2e, 0xaf, 0x67, 0x49, 0xb9, 0x85, 0x5f, 0x63, 0x4e, 0x5e, 0x8e, 0x2e } },
};

#define NR_VERSIONS (int)(sizeof(g_version)/sizeof(g_version[0]))

typedef struct ptr_bundle_t
{
    uint32_t *ptrA;
    uint32_t *ptrB;
}ptr_bundle_t;

struct block_A_info_t
{
    int nr_bits;
    uint16_t field_2;
    int nr_words;
    int nr_dwords_x12;
    uint32_t *ptr6; // size
    uint32_t *ptr7; // size
    uint32_t *ptr5; // size
    uint32_t size;
    uint32_t field_1C;
    ptr_bundle_t ptr1;
    uint32_t *ptr3; // size
    uint32_t *ptr4; //  size
    int nr_words2;
    uint32_t field_34;
    int nr_dwords_x8;
    int nr_bytes;
    int nr_bytes2;
    int nr_dwords_m1;
    int nr_dwords_x2_m1;
    int nr_dwords_x2;
    int nr_dwords;
    uint32_t field_54;
    uint32_t field_58;
};

struct block_A_info_t g_decode_A_info;
uint8_t g_subblock_A[0x128];
uint8_t g_key_B[20];
uint8_t g_perm_B[258];
uint8_t g_crypto_info_byte;
uint8_t *g_decode_buffer;
uint8_t *g_decode_buffer2;
void *g_decode_buffer3;

#include "atj_tables.h"

void compute_checksum(uint8_t *buf, size_t size, uint8_t t[20])
{
    memset(t, 0, 20);

    for(size_t i = 0; i < size; i++)
        t[i % 20] ^= buf[i];
    for(int i = 0; i < 20; i++)
        t[i] = ~t[i];
}

int check_block(uint8_t *buf, uint8_t ref[20], unsigned size)
{
    uint8_t t[20];
    compute_checksum(buf, size, t);

    return memcmp(ref, t, 20);
}


int get_version(uint8_t *buf, unsigned long size)
{
    (void) size;
    struct fwu_hdr_t *hdr = (void *)buf;
    for(int i = 0; i < NR_VERSIONS; i++)
        if(hdr->version == g_version[i].value)
            return i;
    return -1;
}

static int decode_block_A(uint8_t block[1020])
{
    uint8_t *p = &g_check_block_A_table[32 * (block[998] & 0x1f)];
    uint8_t key[32];

    for(int i = 0; i < 20; i++)
    {
        block[1000 + i] ^= p[i];
        key[i] = block[1000 + i];
    }
    for(int i = 20; i < 32; i++)
        key[i] = key[i - 20];
    
    for(int i = 0; i < 992; i++)
        block[i] ^= key[i % 32] ^ g_check_block_A_table[i];
    
    return check_block(block - 1, block + 1000, 1001);
}

static void compute_perm(uint8_t *keybuf, size_t size, uint8_t perm[258])
{
    for(int i = 0; i < 256; i++)
        perm[i] = i;
    perm[256] = perm[257] = 0;
    uint8_t idx = 0;
    for(int i = 0; i < 256; i++)
    {
        uint8_t v = perm[i];
        idx = (v + keybuf[i % size] + idx) % 256;
        perm[i] = perm[idx];
        perm[idx] = v;
    }
}

static void decode_perm(uint8_t *buf, size_t size, uint8_t perm[258])
{
    uint8_t idxa = perm[256];
    uint8_t idxb = perm[257];
    for(size_t i = 0; i < size; i++)
    {
        idxa = (idxa + 1) % 256;
        uint8_t v = perm[idxa];
        idxb = (idxb + v) % 256;
        perm[idxa] = perm[idxb];
        perm[idxb] = v;
        buf[i] ^= perm[(v + perm[idxa]) % 256];
    }
}

static void decode_block_with_perm(uint8_t *keybuf, int keysize,
    uint8_t *buf, int bufsize, uint8_t perm[258])
{
    compute_perm(keybuf, keysize, perm);
    decode_perm(buf, bufsize, perm);
}

static void apply_perm(uint8_t *inbuf, uint8_t *outbuf, size_t size, int swap)
{
    memcpy(outbuf, inbuf, size);
    int a = swap & 0xf;
    int b = (swap >> 4) + 16;
    uint8_t v = outbuf[a];
    outbuf[a] = outbuf[b];
    outbuf[b] = v;
}

static void decode_block_with_swap(uint8_t keybuf[32], int swap,
    uint8_t *buf, int bufsize, uint8_t perm[258])
{
    uint8_t keybuf_interm[32];

    apply_perm(keybuf, keybuf_interm, 32, swap);
    decode_block_with_perm(keybuf_interm, 32, buf, bufsize, perm);
}

static void clear_memory(void *buf, size_t size_dwords)
{
    memset(buf, 0, 4 * size_dwords);
}

static void set_bit(int bit_pos, uint32_t *buf)
{
    buf[bit_pos / 32] |= 1 << (bit_pos % 32);
}

static int fill_decode_info(uint8_t sz)
{
    if(sz == 2) sz = 233;
    else if(sz == 3) sz = 163;
    else return 1;

    g_decode_A_info.nr_bits = sz;
    g_decode_A_info.nr_bytes2 = sz / 8 + (sz % 8 != 0);
    g_decode_A_info.nr_words = 2 * g_decode_A_info.nr_bytes2;
    g_decode_A_info.nr_bytes = sz / 8 + (sz % 8 != 0);
    g_decode_A_info.nr_words2 = 2 * g_decode_A_info.nr_bytes2;
    g_decode_A_info.nr_dwords = sz / 32 + (sz % 32 != 0);
    g_decode_A_info.size = 4 * g_decode_A_info.nr_dwords;
    g_decode_A_info.nr_dwords_x8 = 8 * g_decode_A_info.nr_dwords;
    g_decode_A_info.nr_dwords_m1 = g_decode_A_info.nr_dwords - 1;
    g_decode_A_info.nr_dwords_x2 = 2 * g_decode_A_info.nr_dwords;
    g_decode_A_info.nr_dwords_x2_m1 = g_decode_A_info.nr_dwords_x2 - 1;
    g_decode_A_info.nr_dwords_x12 = 12 * g_decode_A_info.nr_dwords;
    g_decode_A_info.ptr1.ptrA = malloc(4 * g_decode_A_info.nr_dwords);
    g_decode_A_info.ptr1.ptrB = malloc(g_decode_A_info.size);
    g_decode_A_info.ptr3 = malloc(g_decode_A_info.size);
    g_decode_A_info.ptr4 = malloc(g_decode_A_info.size);
    g_decode_A_info.ptr5 = malloc(g_decode_A_info.size);
    g_decode_A_info.ptr6 = malloc(g_decode_A_info.size);
    g_decode_A_info.ptr7 = malloc(g_decode_A_info.size);

    cprintf(BLUE, "  Decode Info:\n");
    cprintf_field("    Nr Bits: ", "%d\n", g_decode_A_info.nr_bits);
    cprintf_field("    Nr Bytes: ", "%d\n", g_decode_A_info.nr_bytes);
    cprintf_field("    Nr Bytes 2: ", "%d\n", g_decode_A_info.nr_bytes2);
    cprintf_field("    Nr Words: ", "%d\n", g_decode_A_info.nr_words);
    cprintf_field("    Nr Words 2: ", "%d\n", g_decode_A_info.nr_words2);
    cprintf_field("    Nr DWords: ", "%d\n", g_decode_A_info.nr_dwords);
    cprintf_field("    Size: ", "%d\n", g_decode_A_info.size);

    return 0;
}

static int process_block_A(uint8_t block[1024])
{
    cprintf(BLUE, "Block A\n");
    int ret = decode_block_A(block + 4);
    cprintf(GREEN, "  Check: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    memcpy(g_subblock_A, block, sizeof(g_subblock_A));
    ret = fill_decode_info(g_subblock_A[276]);
    cprintf(GREEN, "  Info: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    int tmp = 2 * g_decode_A_info.nr_bytes2 + 38;
    int offset = 1004 - tmp + 5;
    g_crypto_info_byte = block[offset - 1];
    g_decode_buffer = malloc(g_decode_A_info.size);
    g_decode_buffer2 = malloc(g_decode_A_info.size);
    
    memset(g_decode_buffer, 0, g_decode_A_info.size);
    memset(g_decode_buffer2, 0, g_decode_A_info.size);

    memcpy(g_decode_buffer, &block[offset], g_decode_A_info.nr_bytes2);
    int offset2 = g_decode_A_info.nr_bytes2 + offset;
    memcpy(g_decode_buffer2, &block[offset2], g_decode_A_info.nr_bytes2);


    cprintf_field("  Word: ", "%d ", *(uint16_t *)&g_subblock_A[286]);
    check_field(*(uint16_t *)&g_subblock_A[286], 1, "Ok\n", "Mismatch\n");
    
    return 0;
}

static void decode_key_B(uint8_t buf[20], uint8_t buf2[16], uint8_t key[20])
{
    for(int i = 0; i < 20; i++)
    {
        uint8_t v = buf[i] ^ g_decode_B_table[i];
        key[i] = v;
        buf[i] = v ^ buf2[i % 16];
    }
}

static void decode_block_B(uint8_t *buf, uint8_t key[16], size_t size)
{
    decode_key_B(&buf[size], key, g_key_B);
    decode_block_with_perm(g_key_B, 20, buf, size, g_perm_B);
}

static int find_last_bit_set(uint32_t *buf, bool a)
{
    int i = a ? g_decode_A_info.nr_dwords_m1 : g_decode_A_info.nr_dwords_x2_m1;

    while(i >= 0 && buf[i] == 0)
        i--;
    if(i < 0)
        return -1;
    for(int j = 31; j >= 0; j--)
        if(buf[i] & (1 << j))
            return 32 * i + j;
    return -1; // unreachable
}

static void xor_with_ptrs(uint8_t *buf, ptr_bundle_t *ptrs)
{
    /*
    int sz = g_decode_A_info.nr_bytes2 - 1;
    if(sz <= 32)
    {
        for(int i = 0; i < sz; i++)
            buf[i] ^= ptrs->ptrA[i];
        for(int i = sz; i < 32; i++)
            buf[i] ^= ptrs->ptrB[i - sz];
    }
    else
        for(int i = 0; i < 32; i++)
            buf[i] ^= ptrs->ptrA[i];
    */
    uint8_t *ptrA = (uint8_t *)ptrs->ptrA;
    uint8_t *ptrB = (uint8_t *)ptrs->ptrB;
    int sz = MIN(g_decode_A_info.nr_bytes2 - 1, 32);
    for(int i = 0; i < sz; i++)
        buf[i] ^= ptrA[i];
    for(int i = sz; i < 32; i++)
        buf[i] ^= ptrB[i - sz];
}

static void copy_memory(uint32_t *to, uint32_t *from)
{
    for(int i = 0; i < g_decode_A_info.nr_dwords; i++)
        to[i] = from[i];
}

static void swap_memory(uint32_t *a, uint32_t *b)
{
    for(int i = 0; i < g_decode_A_info.nr_dwords; i++)
    {
        uint32_t c = a[i];
        a[i] = b[i];
        b[i] = c;
    }
}

static void shift_left(uint32_t *buf, int nr_bits)
{
    for(int i = g_decode_A_info.nr_dwords_m1; i >= 0; i--)
        buf[i + (nr_bits / 32)] = buf[i];
    memset(buf, 0, 4 * (nr_bits / 32));

    size_t size = g_decode_A_info.nr_dwords + (nr_bits + 31) / 32;
    nr_bits = nr_bits % 32;

    uint32_t acc = 0;
    for(size_t i = 0; i < size; i++)
    {
        uint32_t new_val = buf[i] << nr_bits | acc;
        /* WARNING if nr_bits = 0 then the right shift by 32 is undefined and so
         * the following code could break. The additional AND catches this case
         * and make sure the result is 0 */
        acc = ((1 << nr_bits) - 1) & (buf[i] >> (32 - nr_bits));
        buf[i] = new_val;
    }
}

static void xor_big(uint32_t *res, uint32_t *a, uint32_t *b)
{
    for(int i = 0; i < g_decode_A_info.nr_dwords_x2; i++)
        res[i] = a[i] ^ b[i];
}

static void decode_with_xor(uint32_t *res, uint32_t *key)
{
    uint32_t *tmp = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *copy = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *copy_arg = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *tmp2 = malloc(g_decode_A_info.nr_dwords_x8);
    clear_memory(tmp, g_decode_A_info.nr_dwords_x2);
    clear_memory(res, g_decode_A_info.nr_dwords);
    *res = 1;
    clear_memory(tmp2, g_decode_A_info.nr_dwords);
    copy_memory(copy_arg, key);
    copy_memory(copy, (uint32_t *)g_decode_A_info.ptr5);

    for(int i = find_last_bit_set(copy_arg, 1); i; i = find_last_bit_set(copy_arg, 1))
    {
        int pos = i - find_last_bit_set(copy, 1);
        if(pos < 0)
        {
            swap_memory(copy_arg, copy);
            swap_memory(res, tmp2);
            pos = -pos;
        }
        copy_memory(tmp, copy);
        shift_left(tmp, pos);
        xor_big(copy_arg, copy_arg, tmp);
        copy_memory(tmp, tmp2);
        shift_left(tmp, pos);
        xor_big(res, res, tmp);
    }
    free(tmp);
    free(copy);
    free(copy_arg);
    free(tmp2);
}

static void shift_left_one(uint32_t *a)
{
    int pos = find_last_bit_set(a, 0) / 32 + 1;
    if(pos <= 0)
        return;
    uint32_t v = 0;
    for(int i = 0; i < pos; i++)
    {
        uint32_t new_val = v | a[i] << 1;
        v = a[i] >> 31;
        a[i] = new_val;
    }
    if(v)
        a[pos] = v;
}


#if 1
static void xor_mult(uint32_t *a1, uint32_t *a2, uint32_t *a3)
{
    uint32_t *tmp2 = malloc(g_decode_A_info.nr_dwords_x8);
    clear_memory(tmp2, g_decode_A_info.nr_dwords_x2);
    copy_memory(tmp2, a3);

    int pos = g_decode_A_info.nr_dwords;
    uint32_t mask = 1;
    for(int i = 0; i < 32; i++)
    {
        for(int j = 0; j < g_decode_A_info.nr_dwords; j++)
        {
            if(a2[j] & mask)
                for(int k = 0; k < pos; k++)
                    a1[j + k] ^= tmp2[k];
        }
        shift_left_one(tmp2);
        mask <<= 1;
        pos = find_last_bit_set(tmp2, 0) / 32 + 1;
    }
    free(tmp2);
}
#else
static void xor_mult(uint32_t *a1, uint32_t *a2, uint32_t *a3)
{
    for(int i = 0; i < 32 * g_decode_A_info.nr_dwords; i++)
        for(int j = 0; j < 32 * g_decode_A_info.nr_dwords; j++)
        {
            int k = i + j;
            uint32_t v1 = (a2[i / 32] >> (i % 32)) & 1;
            uint32_t v2 = (a3[j / 32] >> (j % 32)) & 1;
            a1[k / 32] ^= (v1 * v2) << (k % 32);
        }
}
#endif

static void xor_mult_high(uint32_t *a1, uint32_t *buf, uint32_t *a3)
{
    (void) a1;
    uint32_t *tmp = malloc(g_decode_A_info.nr_dwords_x8);
    int v4 = g_decode_A_info.field_34;
    int pos = find_last_bit_set(buf, 0);
    for(int i = pos - v4; i >= 0; i = find_last_bit_set(buf, 0) - v4)
    {
        clear_memory(tmp, g_decode_A_info.nr_dwords_x2);
        copy_memory(tmp, a3);
        shift_left(tmp, i);
        xor_big(buf, buf, tmp);
    }
    free(tmp);
}

static void xor_small(uint32_t *res, uint32_t *a, uint32_t *b)
{
    for(int i = 0; i < g_decode_A_info.nr_dwords; i++)
        res[i] = a[i] ^ b[i];
}

static void crypto(ptr_bundle_t *a1, ptr_bundle_t *a2)
{
    uint32_t *v2 = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *v3 = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *v4 = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *v5 = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *v6 = malloc(g_decode_A_info.nr_dwords_x8);
    clear_memory(a2->ptrA, g_decode_A_info.nr_dwords);
    clear_memory(a2->ptrB, g_decode_A_info.nr_dwords);
    clear_memory(v3, g_decode_A_info.nr_dwords_x2);
    clear_memory(v6, g_decode_A_info.nr_dwords_x2);
    clear_memory(v4, g_decode_A_info.nr_dwords_x2);
    decode_with_xor(v4, a1->ptrA);
    clear_memory(v5, g_decode_A_info.nr_dwords_x2);

    xor_mult(v5, v4, a1->ptrB);
    xor_mult_high(v5, v5, g_decode_A_info.ptr5);
    xor_small(v2, a1->ptrA, v5);
    xor_small(v4, v2, g_decode_A_info.ptr6);
    clear_memory(v3, g_decode_A_info.nr_dwords_x2);
    xor_mult(v3, v2, v2);
    xor_mult_high(v3, v3, g_decode_A_info.ptr5);
    xor_small(a2->ptrA, v4, v3);
    clear_memory(v5, g_decode_A_info.nr_dwords_x2);
    xor_small(v4, v2, g_xor_key);
    xor_mult(v5, v4, a2->ptrA);
    xor_mult_high(v5, v5, g_decode_A_info.ptr5);
    clear_memory(v6, g_decode_A_info.nr_dwords_x2);
    xor_mult(v6, a1->ptrA, a1->ptrA);
    xor_mult_high(v6, v6, g_decode_A_info.ptr5);
    xor_small(a2->ptrB, v5, v6);
    free(v2);
    free(v3);
    free(v4);
    free(v5);
    free(v6);
}

static void crypto2(ptr_bundle_t *a1, ptr_bundle_t *a2, ptr_bundle_t *a3)
{
    uint32_t *v3 = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *v4 = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *v5 = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *v6 = malloc(g_decode_A_info.nr_dwords_x8);
    uint32_t *v7 = malloc(g_decode_A_info.nr_dwords_x8);
    clear_memory(a3->ptrA, g_decode_A_info.nr_dwords);
    clear_memory(a3->ptrB, g_decode_A_info.nr_dwords);
    clear_memory(v4, g_decode_A_info.nr_dwords_x2);
    clear_memory(v7, g_decode_A_info.nr_dwords_x2);
    xor_small(v5, a1->ptrB, a2->ptrB);
    xor_small(v6, a1->ptrA, a2->ptrA);
    decode_with_xor(v7, v6);
    clear_memory(v3, g_decode_A_info.nr_dwords_x2);
    xor_mult(v3, v7, v5);
    xor_mult_high(v3, v3, g_decode_A_info.ptr5);
    xor_small(v5, v3, g_decode_A_info.ptr6);
    clear_memory(v4, g_decode_A_info.nr_dwords_x2);
    xor_mult(v4, v3, v3);
    xor_mult_high(v4, v4, g_decode_A_info.ptr5);
    xor_small(v7, v5, v4);
    xor_small(a3->ptrA, v7, v6);
    xor_small(v5, a1->ptrA, a3->ptrA);
    xor_small(v6, a3->ptrA, a1->ptrB);
    clear_memory(v7, g_decode_A_info.nr_dwords_x2);
    xor_mult(v7, v5, v3);
    xor_mult_high(v7, v7, g_decode_A_info.ptr5);
    xor_small(a3->ptrB, v7, v6);
    free(v3);
    free(v4);
    free(v5);
    free(v6);
    free(v7);
}

static int crypto3(uint32_t *a1, ptr_bundle_t *ptrs_alt, ptr_bundle_t *ptrs)
{
    ptr_bundle_t ptrs_others;

    ptrs_others.ptrA = malloc(g_decode_A_info.size);
    ptrs_others.ptrB = malloc(g_decode_A_info.size);
    clear_memory(ptrs->ptrA, g_decode_A_info.nr_dwords);
    clear_memory(ptrs->ptrB, g_decode_A_info.nr_dwords);
    clear_memory(ptrs_others.ptrA, g_decode_A_info.nr_dwords);
    clear_memory(ptrs_others.ptrB, g_decode_A_info.nr_dwords);
    int pos = find_last_bit_set(a1, 1);
    
    copy_memory(ptrs_others.ptrA, ptrs_alt->ptrA);
    copy_memory(ptrs_others.ptrB, ptrs_alt->ptrB);
    for(int bit = (pos % 32) - 1; bit >= 0; bit--)
    {
        crypto(&ptrs_others, ptrs);
        copy_memory(ptrs_others.ptrA, ptrs->ptrA);
        copy_memory(ptrs_others.ptrB, ptrs->ptrB);
        if(a1[pos / 32] & (1 << bit))
        {
            crypto2(&ptrs_others, ptrs_alt, ptrs);
            copy_memory(ptrs_others.ptrA, ptrs->ptrA);
            copy_memory(ptrs_others.ptrB, ptrs->ptrB);
        }
    }
    for(int i = pos / 32 - 1; i >= 0; i--)
    {
        for(int bit = 31; bit >= 0; bit--)
        {
            crypto(&ptrs_others, ptrs);
            copy_memory(ptrs_others.ptrA, ptrs->ptrA);
            copy_memory(ptrs_others.ptrB, ptrs->ptrB);
            if(a1[i] & (1 << bit))
            {
                crypto2(&ptrs_others, ptrs_alt, ptrs);
                copy_memory(ptrs_others.ptrA, ptrs->ptrA);
                copy_memory(ptrs_others.ptrB, ptrs->ptrB);
            }
        }
    }
    copy_memory(ptrs->ptrA, ptrs_others.ptrA);
    copy_memory(ptrs->ptrB, ptrs_others.ptrB);
    free(ptrs_others.ptrA);
    free(ptrs_others.ptrB);
    return 0;
}

static int crypto4(uint8_t *a1, ptr_bundle_t *ptrs, uint32_t *a3)
{
    ptr_bundle_t ptrs_others;
    
    ptrs_others.ptrA = malloc(g_decode_A_info.size);
    ptrs_others.ptrB = malloc(g_decode_A_info.size);
    clear_memory(ptrs_others.ptrA, g_decode_A_info.nr_dwords);
    clear_memory(ptrs_others.ptrB, g_decode_A_info.nr_dwords);
    int ret = crypto3(a3, ptrs, &ptrs_others);
    if(ret == 0)
        xor_with_ptrs(a1, &ptrs_others);
    free(ptrs_others.ptrA);
    free(ptrs_others.ptrB);
    return ret;
}

static int crypto_bits(uint32_t *buf, int a2)
{
    clear_memory(buf, g_decode_A_info.nr_dwords);
    g_decode_A_info.field_34 = 0;
    if(a2 == 4)
    {
        set_bit(0, buf);
        set_bit(74, buf);
        set_bit(233, buf);
        g_decode_A_info.field_34 = 233;
        return 0;
    }
    else if (a2 == 5)
    {
        set_bit(0, buf);
        set_bit(3, buf);
        set_bit(6, buf);
        set_bit(7, buf);
        set_bit(163, buf);
        g_decode_A_info.field_34 = 163;
        return 0;
    }
    else
        return 1;
}

static int crypto_bits_copy(ptr_bundle_t *a1, char a2)
{
    int ret = crypto_bits(g_decode_A_info.ptr5, a2);
    if(ret) return ret;
    if(a2 == 4)
    {
        copy_memory(a1->ptrA, g_crypto_table);
        copy_memory(a1->ptrB, g_crypto_table2);
        copy_memory(g_decode_A_info.ptr6, g_crypto_data);
        copy_memory(g_decode_A_info.ptr7, g_crypto_key6);
        return 0;
    }
    else if ( a2 == 5 )
    {
        copy_memory(a1->ptrA, g_crypto_key3);
        copy_memory(a1->ptrB, g_crypto_key4);
        copy_memory(g_decode_A_info.ptr6, g_crypto_data3);
        copy_memory(g_decode_A_info.ptr7, g_crypto_key5);
        return 0;
    }
    else
        return 1;
}

static void create_guid(void *uid, int bit_size)
{
    uint8_t *p = uid;
    for(int i = 0; i < bit_size / 8; i++)
        p[i] = rand() % 256;
}

static int process_block_B(uint8_t block[512])
{
    cprintf(BLUE, "Block B\n");
    decode_block_B(block + 3, g_subblock_A + 4, 489);
    cprintf_field("  Word: ", "%d ", *(uint16_t *)(block + 3));
    check_field(*(uint16_t *)(block + 3), 1, "Ok\n", "Mismatch\n");

    int ret = check_block(block, block + 492, 492);
    cprintf(GREEN, "  Check: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    g_decode_buffer3 = malloc(g_decode_A_info.size);
    memset(g_decode_buffer3, 0, g_decode_A_info.size);
    int offset = *(uint16_t *)(block + 13) + 16;
    memcpy(g_decode_buffer3, &block[offset], g_decode_A_info.nr_bytes2);

    return 0;
}

static int get_key_fwu_v3(size_t size, uint8_t *buf, uint8_t *blockA, uint8_t *blockB,
    uint8_t *keybuf, uint8_t *blo)
{
    (void) size;
    uint8_t smallblock[512];
    uint8_t bigblock[1024];

    memset(smallblock, 0, sizeof(smallblock));
    memset(bigblock, 0, sizeof(bigblock));

    uint8_t ba = buf[0x1ee] & 0xf;
    uint8_t bb = buf[0x1fe] & 0xf;

    cprintf_field("  Block A: ", "%d\n", ba + 2);
    cprintf_field("  Block B: ", "%d\n", ba + bb + 5);

    *blockA = buf[494] & 0xf;
    *blockB = buf[510] & 0xf;
    memcpy(bigblock, &buf[512 * (*blockA + 2)], sizeof(bigblock));

    int ret = process_block_A(bigblock);
    if(ret != 0)
        return ret;

    memcpy(smallblock, &buf[512 * (*blockA + *blockB + 5)], sizeof(smallblock));
    ret = process_block_B(smallblock);
    if(ret != 0)
        return ret;

    cprintf(BLUE, "Main\n");

    // WARNING you need more that 48 because 17+32 > 48 !! (see code below) */
    uint8_t smallbuf[50];
    memcpy(smallbuf, buf + 42, sizeof(smallbuf));
    cprintf_field("  Byte: ", "%d ", smallbuf[16]);
    check_field(smallbuf[16], 3, "Ok\n", "Mismatch\n");

    ptr_bundle_t ptrs;
    ptrs.ptrA = malloc(g_decode_A_info.size);
    ptrs.ptrB = malloc(g_decode_A_info.size);
    memset(ptrs.ptrA, 0, g_decode_A_info.size);
    memset(ptrs.ptrB, 0, g_decode_A_info.size);
    memcpy(ptrs.ptrA, buf + 91, g_decode_A_info.nr_bytes2);
    memcpy(ptrs.ptrB, buf + 91 + g_decode_A_info.nr_bytes2, g_decode_A_info.nr_bytes2);

    ret = crypto_bits_copy(&g_decode_A_info.ptr1, g_crypto_info_byte);
    cprintf(GREEN, "  Crypto bits copy: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    ret = crypto4(smallbuf + 17, &ptrs, g_decode_buffer3);
    cprintf(GREEN, "  Crypto 4: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    memcpy(keybuf, &smallbuf[17], 32);
    int offset = g_decode_A_info.nr_words + 91;
    
    decode_block_with_swap(keybuf, 0, &buf[offset], 512 - offset, g_perm_B);

    int pos = *(uint16_t *)&buf[offset];
    cprintf_field("  Word: ", "%d ", pos);
    int tmp = g_decode_A_info.nr_words2 + 199;
    check_field(pos, 510 - tmp, "Ok\n", "Mismatch\n");

    uint8_t midbuf[108];
    memcpy(midbuf, &buf[pos + offset + 2], sizeof(midbuf));

    cprintf_field("  Byte: ", "%d ", midbuf[0]);
    check_field(midbuf[0], 2, "Ok\n", "Invalid\n");
    cprintf_field("  DWord: ", "%d ", *(uint32_t *)&midbuf[1]);
    check_field(*(uint32_t *)&midbuf[1], 2056, "Ok\n", "Invalid\n");
    cprintf_field("  DWord: ", "%d ", *(uint32_t *)&midbuf[5]);
    check_field(*(uint32_t *)&midbuf[5], 8, "Ok\n", "Invalid\n");
    cprintf_field("  Byte: ", "%d ", midbuf[41]);
    check_field(midbuf[41], 190, "Ok\n", "Invalid\n");

    memset(blo, 0, 512);
    create_guid(smallblock, 3808);
    memcpy(smallblock + 476, midbuf + 42, 16);
    compute_checksum(smallblock, 492, blo + 492);
    int bsz = blo[500];
    memcpy(blo, smallblock, bsz);
    memcpy(blo + bsz, midbuf + 42, 16);
    memcpy(blo + bsz + 16, smallblock + bsz, 476 - bsz);

    decode_block_with_perm(blo + 492, 16, blo, 492, g_perm_B);
    ret = check_block(buf + 42, midbuf + 88, 450);
    cprintf(GREEN, "  Decode block: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    ret = memcmp(g_subblock_A + 4, midbuf + 9, 16);
    cprintf(GREEN, "  Compare: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    /*
    ret = memcmp(midbuf + 25, zero, sizeof(zero));
    cprintf(GREEN, "  Sanity: ");
    check_field(ret, 0, "Pass\n", "Fail\n");
    */

    return 0;
}

/* stolen from https://github.com/nfd/atj2127decrypt, I have no idea from where
 * he got this sequence of code. This code is really weird, I copy verbatim
 * his authors comment below. */
uint32_t atj2127_key[] =
{
    0x42146ea2, 0x892c8e85, 0x9f9f6d27, 0x545fedc3,
    0x09e5c0ca, 0x2dfa7e61, 0x4e5322e6, 0xb19185b9
};

/* decrypt a 512-byte sector */
static void atj2127_decrypt_sector(void *inbuf, size_t size,
    uint32_t session_key[8], int rounds_to_perform)
{
    uint32_t key[8];
    for(int i = 0; i < 8; i++)
        key[i] = atj2127_key[i] ^ session_key[i];
    uint32_t *buf = inbuf;
    if(size % 32)
        cprintf(GREY, "Size is not a multiple of 32!!!\n");
    while(rounds_to_perform > 0)
    {
        uint32_t rollover = buf[7] ^ session_key[7];

        buf[0] ^= key[1];
        buf[1] ^= key[2];
        buf[2] ^= key[3];
        buf[3] ^= key[4];
        buf[4] ^= key[5];
        buf[5] ^= key[6];
        buf[6] ^= key[7];
        buf[7] ^= key[1] ^ key[4];

        key[1] = key[2];
        key[2] = key[3];
        key[3] = key[4];
        key[4] = key[5];
        key[5] = key[6];
        key[6] = key[7];
        key[7] = rollover;

        buf += 8;
        rounds_to_perform -= 1;
    }
}

static void atj2127_decrypt(uint8_t *dst, const uint8_t *src, size_t size,
    uint8_t keybuf[32], int rounds_to_perform)
{
    cprintf(BLUE, "ATJ2127:\n");
    cprintf_field("  Rounds: ", "%d\n", rounds_to_perform);
    while(size > 0)
    {
        int sec_sz = MIN(size, 512);
        memcpy(dst, src, sec_sz);
        atj2127_decrypt_sector(dst, sec_sz, (uint32_t *)keybuf, rounds_to_perform);
        src += sec_sz;
        dst += sec_sz;
        size -= sec_sz;
    }
}

static int decrypt_fwu_v3(uint8_t *buf, size_t *size, uint8_t block[512], enum fwu_mode_t mode)
{
    uint8_t blockA;
    uint8_t blockB;
    uint8_t keybuf[32];
    memset(keybuf, 0, sizeof(keybuf));
    int ret = get_key_fwu_v3(*size, buf, &blockA, &blockB, keybuf, block);
    if(ret != 0)
        return ret;

    size_t file_size = *size;
    /* the input buffer is reorganized based on two offsets (blockA and blockB),
     * skip 2048 bytes of data used for crypto init */
    *size -= 2048;
    uint8_t *tmpbuf = malloc(*size);
    memset(tmpbuf, 0, *size);
    int offsetA = (blockA + 1) << 9;
    int offsetB = (blockB + 1) << 9;
    memcpy(tmpbuf, buf + 512, offsetA);
    memcpy(tmpbuf + offsetA, buf + offsetA + 1536, offsetB);
    memcpy(tmpbuf + offsetA + offsetB,
        buf + offsetA + 1536 + offsetB + 512, *size - offsetA - offsetB);
    /* stolen from https://github.com/nfd/atj2127decrypt, I have no idea from where
     * he got this sequence of code. This code is really weird, I copy verbatim
     * his authors comment below.
     *
     * This is really weird. This is passed to the decrypt-sector function and
     * determines how much of each 512-byte sector to decrypt, where for every
     * 32MB of size above the first 32MB, one 32 byte chunk of each sector
     * (starting from the end) will remain unencrypted, up to a maximum of 480
     * bytes of plaintext. Was this a speed-related thing? It just seems
     * completely bizarre. */

    /* NOTE: the original code uses the file length to determine how much
     * to encrypt and not the size reported in the header. Since
     * the file size can be different from the size reported in the header
     * (the infamous 512 bytes described above), this might be wrong. */
    int rounds_to_perform = 16 - (file_size >> 0x19);
    if(rounds_to_perform <= 0)
        rounds_to_perform = 1;
    /* the ATJ213x and ATJ2127 do not use the same encryption at this point, and I
     * don't see any obvious way to tell which encryption is used (since they
     * use the same version above). The only difference is that ATJ2127 images
     * I have seen have an extra 512 bytes at the end file (ie the actual file
     * is 512 bytes larger than indicated by the header) but I don't know if this
     * is the case for all files. Thus, unless the user force encryption mode,
     * try both and see if one looks like an AFI file. To guess which one to use,
     * decrypt the first sector and see if it looks like an AFI file */
    bool is_atj2127 = false;
    if(mode == FWU_AUTO)
    {
        uint8_t hdr_buf[512];
        atj2127_decrypt(hdr_buf, tmpbuf, sizeof(hdr_buf), keybuf, rounds_to_perform);
        is_atj2127 = afi_check(hdr_buf, sizeof(hdr_buf));
        if(is_atj2127)
            cprintf(BLUE, "File looks like an ATJ2127 firmware\n");
        else
            cprintf(BLUE, "File does not looks like an ATJ2127 firmware\n");
    }
    else if(mode == FWU_ATJ2127)
        is_atj2127 = true;

    if(is_atj2127)
        atj2127_decrypt(buf, tmpbuf, *size, keybuf, rounds_to_perform);
    else
    {
        compute_perm(keybuf, 32, g_perm_B);
        decode_perm(tmpbuf, *size, g_perm_B);
        memcpy(buf, tmpbuf, *size);
    }

    return 0;
}

int fwu_decrypt(uint8_t *buf, size_t *size, enum fwu_mode_t mode)
{
    struct fwu_hdr_t *hdr = (void *)buf;

    if(*size < sizeof(struct fwu_hdr_t))
    {
        cprintf(GREY, "File too small\n");
        return 1;
    }
    cprintf(BLUE, "Header\n");
    cprintf(GREEN, "  Signature:");
    for(int i = 0; i < FWU_SIG_SIZE; i++)
        cprintf(YELLOW, " %02x", hdr->sig[i]);
    if(memcmp(hdr->sig, g_fwu_signature, FWU_SIG_SIZE) == 0)
        cprintf(RED, " Ok\n");
    else
    {
        cprintf(RED, " Mismatch\n");
        return 1;
    }

    cprintf_field("  FW size: ", "%d ", hdr->fw_size);
    if(hdr->fw_size == *size)
        cprintf(RED, " Ok\n");
    else if(hdr->fw_size < *size)
        cprintf(RED, " Ok (file greater than firmware)\n");
    else
    {
        cprintf(RED, " Error (file too small)\n");
        return 1;
    }

    cprintf_field("  Block size: ", "%d ", hdr->block_size);
    check_field(hdr->block_size, FWU_BLOCK_SIZE, "Ok\n", "Invalid\n");

    cprintf_field("  Version: ", "%x ", hdr->version);
    int ver = get_version(buf, *size);
    if(ver < 0)
    {
        cprintf(RED, "(Unknown)\n");
        return 1;
    }
    else
        cprintf(RED, "(Ver. %d)\n", g_version[ver].version);

    cprintf_field("  Unknown: ", "0x%x ", hdr->unk);
    check_field(hdr->unk, g_version[ver].unk, "Ok\n", "Invalid\n");

    cprintf(GREEN, "  Signature:");
    for(int i = 0; i < FWU_SIG_SIZE; i++)
        cprintf(YELLOW, " %02x", hdr->sig2[i]);
    if(memcmp(hdr->sig2, g_version[ver].sig2, FWU_SIG_SIZE) == 0)
        cprintf(RED, " Ok\n");
    else
    {
        cprintf(RED, " Mismatch\n");
        return 2;
    }

    if(g_version[ver].version == 3)
    {
        uint8_t block[512];
        memset(block, 0, sizeof(block));
        return decrypt_fwu_v3(buf, size, block, mode);
    }
    else
    {
        cprintf(GREY, "Unsupported version: %d\n", g_version[ver].version);
        return 1;
    }

}

bool fwu_check(uint8_t *buf, size_t size)
{
    struct fwu_hdr_t *hdr = (void *)buf;

    if(size < sizeof(struct fwu_hdr_t))
        return false;
    return memcmp(hdr->sig, g_fwu_signature, FWU_SIG_SIZE) == 0;
}
