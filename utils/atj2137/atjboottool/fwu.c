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

#define check_field_soft(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); } \
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

struct fwu_crypto_hdr_t
{
    uint8_t field0[16];
    uint8_t unk;
    uint8_t key[32];
} __attribute__((packed));

struct fwu_sector0_tail_t
{
    uint8_t unk_2;
    uint32_t unk_x808;
    uint32_t unk_8;
    uint8_t key_B[16];
    uint8_t guid[16];
    uint8_t unk_190;
    uint8_t super_secret_xor[16];
    uint8_t timestamp[8];
    uint8_t unk_0;
    uint8_t guid_filler[20];
    uint8_t unk_1;
    uint8_t check[20];
} __attribute__((packed));

struct fwu_block_A_hdr_t
{
    uint16_t block_A_size;
    uint8_t unk_0_a;
    uint8_t unk_1_a;
    uint8_t key_B[16];
    uint8_t guid_filler[256];
    uint8_t ec_sz;
    uint8_t unk_0_b;
    uint32_t unk_5;
    uint32_t unk_x505;
    uint16_t unk_1_b;
    uint8_t timestamp[8];
} __attribute__((packed));

struct fwu_block_B_hdr_t
{
    uint16_t block_B_size;
    uint8_t unk_1_a;
    uint16_t unk_1_b;
    uint8_t timestamp[8];
    uint16_t guid_filler_size;
} __attribute__((packed));

struct fwu_tail_t
{
    uint8_t length; /* in blocks? it's always 1 */
    uint8_t type; /* always 7 */
    uint8_t reserved[14];
    uint32_t fwu_checksum;
    uint32_t flags; /* always 0x55aa55aa */
    uint8_t desc[8]; /* always 'FwuTail' */
    uint8_t fwu_crc_checksum[32]; /* always 0 */
    uint8_t reserved2[444];
    uint32_t fwutail_checksum;
} __attribute__((packed));

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

typedef struct ec_point_t
{
    uint32_t *x;
    uint32_t *y;
}ec_point_t;

struct ec_info_t
{
    int nr_bits;
    int point_size;
    uint32_t *ec_a; // size
    uint32_t *ec_b; // size
    uint32_t *field_poly; // size
    uint32_t size;
    ec_point_t pt_G;
    ec_point_t pt_kG; // calculated ECIES public key
    uint32_t field_bits;
    int size_x2;
    int nr_bytes;
    int nr_dwords_m1;
    int nr_dwords_x2;
    int nr_dwords_x2_m1;
    int nr_dwords;
};

struct ec_info_t g_ec_info;
struct fwu_block_A_hdr_t g_subblock_A;
uint8_t g_key_B[20];
uint8_t g_rc4_S[258];
uint8_t g_field_sz_byte;
ec_point_t g_public_key; // from block A
uint32_t *g_private_key; // from block B

#include "atj_tables.h"
#include <ctype.h>

void print_hex(const char *name, void *buf, size_t sz)
{
    if(name)
        cprintf(BLUE, "%s\n", name);
    uint8_t *p = buf;
    for(size_t i = 0; i < sz; i += 16)
    {
        if(name)
            cprintf(OFF, "  ");
        for(size_t j = i; j < i + 16; j++)
            if(j < sz)
                cprintf(YELLOW, "%02x ", p[j]);
            else
                cprintf(OFF, "   ");
        cprintf(RED, " |");
        for(size_t j = i; j < i + 16; j++)
            cprintf(GREEN, "%c", (j < sz && isprint(p[j])) ? p[j] : '.');
        cprintf(RED, "|\n");
    }
}

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
    uint8_t *p = &g_decode_A_table[32 * (block[998] & 31)];
    uint8_t key[32];

    for(int i = 0; i < 20; i++)
    {
        block[1000 + i] ^= p[i];
        key[i] = block[1000 + i];
    }
    for(int i = 20; i < 32; i++)
        key[i] = key[i - 20];

    for(int i = 0; i < 31 * 32; i++)
        block[i] ^= key[i % 32] ^ g_decode_A_table[i];

    // FIXME dereferencing block - 1 is undefined behavior in standard C
    return check_block(block - 1, block + 1000, 1001);
}

// https://en.wikipedia.org/wiki/RC4#Key-scheduling_algorithm_(KSA)
static void rc4_key_schedule(uint8_t *key, size_t keylength, uint8_t S[258])
{
    for(int i = 0; i < 256; i++)
        S[i] = i;
    S[256] = S[257] = 0;
    uint8_t j = 0;
    for(int i = 0; i < 256; i++)
    {
        j = (j + S[i] + key[i % keylength]) % 256;
        uint8_t tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
    }
}

// https://en.wikipedia.org/wiki/RC4#Pseudo-random_generation_algorithm_(PRGA)
static void rc4_stream_cipher(uint8_t *buf, size_t size, uint8_t S[258])
{
    uint8_t i = S[256];
    uint8_t j = S[257];
    for(size_t k = 0; k < size; k++)
    {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        uint8_t tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
        buf[k] ^= S[(S[i] + S[j]) % 256];
    }
}

static void rc4_cipher_block(uint8_t *keybuf, int keysize,
    uint8_t *buf, int bufsize, uint8_t S[258])
{
    rc4_key_schedule(keybuf, keysize, S);
    rc4_stream_cipher(buf, bufsize, S);
}

static void rc4_key_swap(uint8_t *inbuf, uint8_t *outbuf, size_t size, int swap)
{
    memcpy(outbuf, inbuf, size);
    int a = swap & 0xf;
    int b = (swap >> 4) + 16;
    uint8_t v = outbuf[a];
    outbuf[a] = outbuf[b];
    outbuf[b] = v;
}

static void rc4_key_swap_and_decode(uint8_t keybuf[32], int swap,
    uint8_t *buf, int bufsize, uint8_t S[258])
{
    uint8_t keybuf_interm[32];

    rc4_key_swap(keybuf, keybuf_interm, 32, swap);
    rc4_cipher_block(keybuf_interm, 32, buf, bufsize, S);
}

static void gf_zero(void *buf, size_t size_dwords)
{
    memset(buf, 0, 4 * size_dwords);
}

static void set_bit(int bit_pos, uint32_t *buf)
{
    buf[bit_pos / 32] |= 1 << (bit_pos % 32);
}

static int fill_ec_info(uint8_t sz)
{
    if(sz == 2) sz = 233;
    else if(sz == 3) sz = 163;
    else return 1;

    g_ec_info.nr_bits = sz;
    g_ec_info.nr_bytes = sz / 8 + (sz % 8 != 0);
    g_ec_info.point_size = 2 * g_ec_info.nr_bytes;
    g_ec_info.nr_dwords = sz / 32 + (sz % 32 != 0);
    g_ec_info.size = 4 * g_ec_info.nr_dwords;
    g_ec_info.size_x2 = 8 * g_ec_info.nr_dwords;
    g_ec_info.nr_dwords_m1 = g_ec_info.nr_dwords - 1;
    g_ec_info.nr_dwords_x2 = 2 * g_ec_info.nr_dwords;
    g_ec_info.nr_dwords_x2_m1 = g_ec_info.nr_dwords_x2 - 1;
    g_ec_info.pt_G.x = malloc(4 * g_ec_info.nr_dwords);
    g_ec_info.pt_G.y = malloc(g_ec_info.size);
    g_ec_info.pt_kG.x = malloc(g_ec_info.size);
    g_ec_info.pt_kG.y = malloc(g_ec_info.size);
    g_ec_info.field_poly = malloc(g_ec_info.size);
    g_ec_info.ec_a = malloc(g_ec_info.size);
    g_ec_info.ec_b = malloc(g_ec_info.size);

    cprintf(BLUE, "  Elliptic curve info:\n");
    cprintf_field("    Field Bits: ", "%d\n", g_ec_info.nr_bits);
    cprintf_field("    Field Bytes: ", "%d\n", g_ec_info.nr_bytes);
    cprintf_field("    Point Size: ", "%d\n", g_ec_info.point_size);
    cprintf_field("    Field DWords: ", "%d\n", g_ec_info.nr_dwords);
    cprintf_field("    Size: ", "%d\n", g_ec_info.size);

    return 0;
}

static int process_block_A(uint8_t block[1024])
{
    cprintf(BLUE, "Block A\n");
    int ret = decode_block_A(block + 4);
    cprintf(GREEN, "  Check: ");
    check_field(ret, 0, "Pass\n", "Fail\n");
    // print_hex("BlockA", block, 1024);

    memcpy(&g_subblock_A, block, sizeof(g_subblock_A));
    // assert(offsetof(struct fwu_block_A_hdr_t, ec_sz) == 276);
    ret = fill_ec_info(g_subblock_A.ec_sz);
    cprintf(GREEN, "  Info: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    int tmp = 2 * g_ec_info.nr_bytes + 38;
    int offset = 1004 - tmp + 5;
    g_field_sz_byte = block[offset - 1];
    g_public_key.x = malloc(g_ec_info.size);
    g_public_key.y = malloc(g_ec_info.size);

    memset(g_public_key.x, 0, g_ec_info.size);
    memset(g_public_key.y, 0, g_ec_info.size);

    memcpy(g_public_key.x, &block[offset], g_ec_info.nr_bytes);
    int offset2 = g_ec_info.nr_bytes + offset;
    memcpy(g_public_key.y, &block[offset2], g_ec_info.nr_bytes);


    // assert(offsetof(struct fwu_block_A_hdr_t, unk_1_b) == 286);
    cprintf_field("  Word: ", "%d ", g_subblock_A.unk_1_b);
    check_field(g_subblock_A.unk_1_b, 1, "Ok\n", "Mismatch\n");

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
    rc4_cipher_block(g_key_B, 20, buf, size, g_rc4_S);
}

static int find_last_bit_set(uint32_t *buf, bool a)
{
    int i = a ? g_ec_info.nr_dwords_m1 : g_ec_info.nr_dwords_x2_m1;

    while(i >= 0 && buf[i] == 0)
        i--;
    if(i < 0)
        return -1;
    for(int j = 31; j >= 0; j--)
        if(buf[i] & (1 << j))
            return 32 * i + j;
    return -1; // unreachable
}

static void gf_copy(uint32_t *to, uint32_t *from)
{
    for(int i = 0; i < g_ec_info.nr_dwords; i++)
        to[i] = from[i];
}

static void gf_swap(uint32_t *a, uint32_t *b)
{
    for(int i = 0; i < g_ec_info.nr_dwords; i++)
    {
        uint32_t c = a[i];
        a[i] = b[i];
        b[i] = c;
    }
}

static void shift_left(uint32_t *buf, int nr_bits)
{
    for(int i = g_ec_info.nr_dwords_m1; i >= 0; i--)
        buf[i + (nr_bits / 32)] = buf[i];
    memset(buf, 0, 4 * (nr_bits / 32));

    size_t size = g_ec_info.nr_dwords + (nr_bits + 31) / 32;
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

static void gf_add_x2(uint32_t *res, uint32_t *a, uint32_t *b)
{
    for(int i = 0; i < g_ec_info.nr_dwords_x2; i++)
        res[i] = a[i] ^ b[i];
}

static void print_poly(const char *name, uint32_t *poly, int nr_dwords)
{
    bool first = true;
    cprintf(RED, "%s", name);
    for(int dw = 0; dw < nr_dwords; dw++)
    {
        for(int i = 0; i < 32; i++)
        {
            if(!(poly[dw] & (1 << i)))
                continue;
            if(first)
                first = false;
            else
                cprintf(OFF, "+");
            cprintf(OFF, "x^%d", dw * 32 + i);
        }
    }
    cprintf(OFF, "\n");
}

/* https://en.wikipedia.org/wiki/Extended_Euclidean_algorithm#Simple_algebraic_field_extensions
 * invariant: p * s + a * t == r -> a * t == r (mod p)
 * loop until only lowest bit set (r == 1) -> inverse in t */
static void gf_inverse(uint32_t *newt, uint32_t *val)
{
    uint32_t *tmp = malloc(g_ec_info.size_x2);
    uint32_t *r = malloc(g_ec_info.size_x2);
    uint32_t *newr = malloc(g_ec_info.size_x2);
    uint32_t *t = malloc(g_ec_info.size_x2);
    gf_zero(tmp, g_ec_info.nr_dwords_x2);
    /* newt := 1 */
    gf_zero(newt, g_ec_info.nr_dwords);
    *newt = 1;
    /* t := 0 */
    gf_zero(t, g_ec_info.nr_dwords);
    /* newr := a */
    gf_copy(newr, val);
    /* r := p */
    gf_copy(r, g_ec_info.field_poly);

    for(int i = find_last_bit_set(newr, 1); i; i = find_last_bit_set(newr, 1))
    {
        /* pos := degree(newr) - degree(r) */
        int pos = i - find_last_bit_set(r, 1);
        if(pos < 0)
        {
            gf_swap(newr, r);
            gf_swap(newt, t);
            pos = -pos;
        }
        /* newr := newr - x^pos * r */
        gf_copy(tmp, r);
        shift_left(tmp, pos);
        gf_add_x2(newr, newr, tmp);
        /* newt := newt - x^pos * t */
        gf_copy(tmp, t);
        shift_left(tmp, pos);
        gf_add_x2(newt, newt, tmp);
    }
    free(tmp);
    free(r);
    free(newr);
    free(t);
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
static void gf_mult(uint32_t *res, uint32_t *a2, uint32_t *a3)
{
    uint32_t *tmp2 = malloc(g_ec_info.size_x2);
    gf_zero(tmp2, g_ec_info.nr_dwords_x2);
    gf_copy(tmp2, a3);

    int pos = g_ec_info.nr_dwords;
    uint32_t mask = 1;
    for(int i = 0; i < 32; i++)
    {
        for(int j = 0; j < g_ec_info.nr_dwords; j++)
        {
            if(a2[j] & mask)
                for(int k = 0; k < pos; k++)
                    res[j + k] ^= tmp2[k];
        }
        shift_left_one(tmp2);
        mask <<= 1;
        pos = find_last_bit_set(tmp2, 0) / 32 + 1;
    }
    free(tmp2);
}
#else
static void gf_mult(uint32_t *res, uint32_t *a2, uint32_t *a3)
{
    for(int i = 0; i < 32 * g_ec_info.nr_dwords; i++)
        for(int j = 0; j < 32 * g_ec_info.nr_dwords; j++)
        {
            int k = i + j;
            uint32_t v1 = (a2[i / 32] >> (i % 32)) & 1;
            uint32_t v2 = (a3[j / 32] >> (j % 32)) & 1;
            res[k / 32] ^= (v1 * v2) << (k % 32);
        }
}
#endif

// https://en.wikipedia.org/wiki/Polynomial_long_division#Pseudocode
static void gf_mod(uint32_t *r, uint32_t *field_poly)
{
    uint32_t *tmp = malloc(g_ec_info.size_x2);
    int deg_d = g_ec_info.field_bits;
    int deg_r = find_last_bit_set(r, 0);
    /* i := degree(lead(r) / lead(d)) */
    for(int i = deg_r - deg_d; i >= 0; i = find_last_bit_set(r, 0) - deg_d)
    {
        /* r := r - x^i * d */
        gf_zero(tmp, g_ec_info.nr_dwords_x2);
        gf_copy(tmp, field_poly);
        shift_left(tmp, i);
        gf_add_x2(r, r, tmp);
    }
    free(tmp);
}

static void gf_add(uint32_t *res, uint32_t *a, uint32_t *b)
{
    for(int i = 0; i < g_ec_info.nr_dwords; i++)
        res[i] = a[i] ^ b[i];
}

static void print_point(const char *name, ec_point_t *ptr)
{
    cprintf(BLUE, "%s\n", name);
    print_poly("  x: ", ptr->x, g_ec_info.nr_dwords);
    print_poly("  y: ", ptr->y, g_ec_info.nr_dwords);
}

static uint32_t g_gf_one[9] =
{
    1, 0, 0, 0, 0, 0, 0, 0, 0
};

static void ec_double(ec_point_t *point, ec_point_t *res)
{
    uint32_t *v2 = malloc(g_ec_info.size_x2);
    uint32_t *v3 = malloc(g_ec_info.size_x2);
    uint32_t *v4 = malloc(g_ec_info.size_x2);
    uint32_t *v5 = malloc(g_ec_info.size_x2);
    uint32_t *v6 = malloc(g_ec_info.size_x2);
    gf_zero(res->x, g_ec_info.nr_dwords);
    gf_zero(res->y, g_ec_info.nr_dwords);
    gf_zero(v3, g_ec_info.nr_dwords_x2);
    gf_zero(v6, g_ec_info.nr_dwords_x2);
    gf_zero(v4, g_ec_info.nr_dwords_x2);
    /* v4 := 1/x */
    gf_inverse(v4, point->x);
    gf_zero(v5, g_ec_info.nr_dwords_x2);
    /* v5 := y/x */
    gf_mult(v5, v4, point->y);
    gf_mod(v5, g_ec_info.field_poly);
    /* v2 := x + y/x   (lambda) */
    gf_add(v2, point->x, v5);
    /* v4 := ec_a + lambda */
    gf_add(v4, v2, g_ec_info.ec_a);
    gf_zero(v3, g_ec_info.nr_dwords_x2);
    /* v3 := lambda^2 */
    gf_mult(v3, v2, v2);
    gf_mod(v3, g_ec_info.field_poly);
    /* x' := lambda + lambda^2 + ec_a */
    gf_add(res->x, v4, v3);
    gf_zero(v5, g_ec_info.nr_dwords_x2);
    /* v4 := lambda + g_gf_one */
    gf_add(v4, v2, g_gf_one);
    /* v5 := (lambda + 1) * x' = lambda.x' + x' */
    gf_mult(v5, v4, res->x);
    gf_mod(v5, g_ec_info.field_poly);
    gf_zero(v6, g_ec_info.nr_dwords_x2);
    /* v6 := x1^2 */
    gf_mult(v6, point->x, point->x);
    gf_mod(v6, g_ec_info.field_poly);
    /* y' = (lambda + g_gf_one) * x + x^2 = x^2 + lambda.x + x */
    gf_add(res->y, v5, v6);
    free(v2);
    free(v3);
    free(v4);
    free(v5);
    free(v6);
}

static void ec_add(ec_point_t *a1, ec_point_t *a2, ec_point_t *res)
{
    uint32_t *v3 = malloc(g_ec_info.size_x2);
    uint32_t *v4 = malloc(g_ec_info.size_x2);
    uint32_t *v5 = malloc(g_ec_info.size_x2);
    uint32_t *v6 = malloc(g_ec_info.size_x2);
    uint32_t *v7 = malloc(g_ec_info.size_x2);
    gf_zero(res->x, g_ec_info.nr_dwords);
    gf_zero(res->y, g_ec_info.nr_dwords);
    gf_zero(v4, g_ec_info.nr_dwords_x2);
    gf_zero(v7, g_ec_info.nr_dwords_x2);
    /* v5 = y1 + y2 */
    gf_add(v5, a1->y, a2->y);
    /* v6 = x1 + x2 */
    gf_add(v6, a1->x, a2->x);
    /* v7 = 1/(x1 + x2) */
    gf_inverse(v7, v6);
    gf_zero(v3, g_ec_info.nr_dwords_x2);
    /* v3 = (y1 + y2) / (x1 + x2)    (lambda) */
    gf_mult(v3, v7, v5);
    gf_mod(v3, g_ec_info.field_poly);
    /* v5 = lambda + ec_a */
    gf_add(v5, v3, g_ec_info.ec_a);
    gf_zero(v4, g_ec_info.nr_dwords_x2);
    /* v4 = lambda^2 */
    gf_mult(v4, v3, v3);
    gf_mod(v4, g_ec_info.field_poly);
    /* v7 = lambda^2 + lambda + ec_a */
    gf_add(v7, v5, v4);
    /* x' = ec_a + x1 + x2 + lambda + lambda^2 */
    gf_add(res->x, v7, v6);
    /* v5 = x1 + x' */
    gf_add(v5, a1->x, res->x);
    /* v6 = x' + y1 */
    gf_add(v6, res->x, a1->y);
    gf_zero(v7, g_ec_info.nr_dwords_x2);
    /* v7 = (x1 + x').lambda */
    gf_mult(v7, v5, v3);
    gf_mod(v7, g_ec_info.field_poly);
    /* y' = (x1 + x').lambda + x' + y1 */
    gf_add(res->y, v7, v6);
    free(v3);
    free(v4);
    free(v5);
    free(v6);
    free(v7);
}

static int ec_mult(uint32_t *n, ec_point_t *point, ec_point_t *res)
{
    ec_point_t res_others;

    res_others.x = malloc(g_ec_info.size);
    res_others.y = malloc(g_ec_info.size);
    gf_zero(res->x, g_ec_info.nr_dwords);
    gf_zero(res->y, g_ec_info.nr_dwords);
    gf_zero(res_others.x, g_ec_info.nr_dwords);
    gf_zero(res_others.y, g_ec_info.nr_dwords);
    int pos = find_last_bit_set(n, 1);

    /* res_other := point */
    gf_copy(res_others.x, point->x);
    gf_copy(res_others.y, point->y);

    /* for all bit from SZ-1 downto 0 */
    for(int bit = (pos % 32) - 1; bit >= 0; bit--)
    {
        /* res := 2 * res_other */
        ec_double(&res_others, res);
        /* res_other := res = 2 * res_other */
        gf_copy(res_others.x, res->x);
        gf_copy(res_others.y, res->y);
        /* if bit of n is set */
        if(n[pos / 32] & (1 << bit))
        {
            /* res := res_other + point */
            ec_add(&res_others, point, res);
            gf_copy(res_others.x, res->x);
            gf_copy(res_others.y, res->y);
        }
    }
    /* same but optimized */
    for(int i = pos / 32 - 1; i >= 0; i--)
    {
        for(int bit = 31; bit >= 0; bit--)
        {
            ec_double(&res_others, res);
            gf_copy(res_others.x, res->x);
            gf_copy(res_others.y, res->y);
            if(n[i] & (1 << bit))
            {
                ec_add(&res_others, point, res);
                gf_copy(res_others.x, res->x);
                gf_copy(res_others.y, res->y);
            }
        }
    }
    gf_copy(res->x, res_others.x);
    gf_copy(res->y, res_others.y);
    free(res_others.x);
    free(res_others.y);
    return 0;
}

static void xor_with_point(uint8_t *buf, ec_point_t *point)
{
    /*
    int sz = g_ec_info.nr_bytes - 1;
    if(sz <= 32)
    {
        for(int i = 0; i < sz; i++)
            buf[i] ^= point->x[i];
        for(int i = sz; i < 32; i++)
            buf[i] ^= point->y[i - sz];
    }
    else
        for(int i = 0; i < 32; i++)
            buf[i] ^= point->x[i];
    */
    uint8_t *ptrA = (uint8_t *)point->x;
    uint8_t *ptrB = (uint8_t *)point->y;
    int sz = MIN(g_ec_info.nr_bytes - 1, 32);
    for(int i = 0; i < sz; i++)
        buf[i] ^= ptrA[i];
    for(int i = sz; i < 32; i++)
        buf[i] ^= ptrB[i - sz];
}

// https://en.wikipedia.org/wiki/Integrated_Encryption_Scheme#Formal_description_of_ECIES
static int xor_with_shared_secret(uint8_t *buf, ec_point_t *pt_rG, uint32_t *private_key)
{
    ec_point_t shared_secret;

    shared_secret.x = malloc(g_ec_info.size);
    shared_secret.y = malloc(g_ec_info.size);
    gf_zero(shared_secret.x, g_ec_info.nr_dwords);
    gf_zero(shared_secret.y, g_ec_info.nr_dwords);
    int ret = ec_mult(private_key, pt_rG, &shared_secret);
    if(ret == 0)
        xor_with_point(buf, &shared_secret);
    free(shared_secret.x);
    free(shared_secret.y);
    return ret;
}

static int set_field_poly(uint32_t *field_poly, int field_sz)
{
    gf_zero(field_poly, g_ec_info.nr_dwords);
    g_ec_info.field_bits = 0;
    if(field_sz == 4)
    {
        set_bit(0, field_poly);
        set_bit(74, field_poly);
        set_bit(233, field_poly);
        g_ec_info.field_bits = 233;
        return 0;
    }
    else if (field_sz == 5)
    {
        set_bit(0, field_poly);
        set_bit(3, field_poly);
        set_bit(6, field_poly);
        set_bit(7, field_poly);
        set_bit(163, field_poly);
        g_ec_info.field_bits = 163;
        return 0;
    }
    else
        return 1;
}

static int ec_init(ec_point_t *ec_G, char field_sz)
{
    int ret = set_field_poly(g_ec_info.field_poly, field_sz);
    if(ret) return ret;
    if(field_sz == 4)
    {
        gf_copy(ec_G->x, g_sect233k1_G_x);
        gf_copy(ec_G->y, g_sect233k1_G_y);
        gf_copy(g_ec_info.ec_a, g_sect233k1_a); // zero
        gf_copy(g_ec_info.ec_b, g_sect233k1_b); // never used
        return 0;
    }
    else if(field_sz == 6 ) // yet to find even a single specimen
    {
        gf_copy(ec_G->x, g_sect163r2_G_x);
        gf_copy(ec_G->y, g_sect163r2_G_y);
        gf_copy(g_ec_info.ec_a, g_sect163r2_a);
        gf_copy(g_ec_info.ec_b, g_sect163r2_b);
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
    struct fwu_block_B_hdr_t *p_hdr = (void *)block;

    cprintf(BLUE, "Block B\n");
    decode_block_B(block + 3, g_subblock_A.key_B, 492 - 3);
    cprintf_field("  Word: ", "%d ", p_hdr->unk_1_b);
    check_field(p_hdr->unk_1_b, 1, "Ok\n", "Mismatch\n");

    int ret = check_block(block, block + 492, 492);
    cprintf(GREEN, "  Check: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    g_private_key = malloc(g_ec_info.size);
    memset(g_private_key, 0, g_ec_info.size);
    int offset = sizeof *p_hdr + p_hdr->guid_filler_size + 1;
    memcpy(g_private_key, &block[offset], g_ec_info.nr_bytes);

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

    *blockA = buf[0x1ee] & 15;
    *blockB = buf[0x1fe] & 15;
    size_t offsetA = 512 * (1 + *blockA);
    size_t offsetB = 512 * (1 + *blockB);

    cprintf(BLUE, "Crypto\n");
    cprintf_field("  Block A: ", "0x%zx\n", 512 + offsetA);
    cprintf_field("  Block B: ", "0x%zx\n", 512 + offsetA + 1024 + offsetB);

    memcpy(bigblock, &buf[512 + offsetA], sizeof(bigblock));

    int ret = process_block_A(bigblock);
    if(ret != 0)
        return ret;

    memcpy(smallblock, &buf[512 + offsetA + 1024 + offsetB], sizeof(smallblock));
    ret = process_block_B(smallblock);
    if(ret != 0)
        return ret;

    cprintf(BLUE, "Main\n");

    struct fwu_crypto_hdr_t crypto_hdr;
    memcpy(&crypto_hdr, buf + sizeof(struct fwu_hdr_t), sizeof(crypto_hdr));
    cprintf_field("  Byte: ", "%d ", crypto_hdr.unk);
    check_field(crypto_hdr.unk, 3, "Ok\n", "Mismatch\n");

    size_t offset = sizeof(struct fwu_hdr_t) + sizeof(struct fwu_crypto_hdr_t);
    ec_point_t pt_rG;
    pt_rG.x = malloc(g_ec_info.size);
    pt_rG.y = malloc(g_ec_info.size);
    memset(pt_rG.x, 0, g_ec_info.size);
    memset(pt_rG.y, 0, g_ec_info.size);
    memcpy(pt_rG.x, buf + offset, g_ec_info.nr_bytes);
    memcpy(pt_rG.y, buf + offset + g_ec_info.nr_bytes, g_ec_info.nr_bytes);

    ret = ec_init(&g_ec_info.pt_G, g_field_sz_byte);
    cprintf(GREEN, "  Elliptic curve init: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    ec_mult(g_private_key, &g_ec_info.pt_G, &g_ec_info.pt_kG);
    cprintf(GREEN, "  Public key check: ");
    if (memcmp(g_public_key.x, g_ec_info.pt_kG.x, g_ec_info.nr_bytes) ||
        memcmp(g_public_key.y, g_ec_info.pt_kG.y, g_ec_info.nr_bytes))
    {
        cprintf(RED, "Fail\n");
        return 1;
    }
    else
        cprintf(RED, "Pass\n");

    ret = xor_with_shared_secret(crypto_hdr.key, &pt_rG, g_private_key);
    cprintf(GREEN, "  ECIES decryption: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    memcpy(keybuf, crypto_hdr.key, 32);
    offset += g_ec_info.point_size;

    rc4_key_swap_and_decode(keybuf, 0, &buf[offset], 512 - offset, g_rc4_S);

    int pos = *(uint16_t *)&buf[offset];
    cprintf_field("  Filler size: ", "%d ", pos);
    int tmp = offset + sizeof(struct fwu_sector0_tail_t);
    check_field(pos, 510 - tmp, "Ok\n", "Mismatch\n");

    struct fwu_sector0_tail_t tail;
    memcpy(&tail, &buf[offset + 2 + pos], sizeof(tail));

    cprintf_field("  Byte: ", "%d ", tail.unk_2);
    check_field(tail.unk_2, 2, "Ok\n", "Invalid\n");
    cprintf_field("  DWord: ", "0x%x ", tail.unk_x808);
    check_field(tail.unk_x808, 0x808, "Ok\n", "Invalid\n");
    cprintf_field("  DWord: ", "%d ", tail.unk_8);
    check_field(tail.unk_8, 8, "Ok\n", "Invalid\n");
    cprintf_field("  Byte: ", "%d ", tail.unk_190);
    check_field(tail.unk_190, 190, "Ok\n", "Invalid\n");

    /* encode super secret at random position in guid stream, never used */
    memset(blo, 0, 512);
    create_guid(smallblock, 476 * 8);
    memcpy(smallblock + 476, tail.super_secret_xor, 16);
    compute_checksum(smallblock, 492, blo + 492);
    int bsz = blo[500];
    memcpy(blo, smallblock, bsz);
    memcpy(blo + bsz, tail.super_secret_xor, 16);
    memcpy(blo + bsz + 16, smallblock + bsz, 476 - bsz);
    rc4_cipher_block(blo + 492, 16, blo, 492, g_rc4_S);

    ret = check_block(buf + sizeof(struct fwu_hdr_t), tail.check, 492 - sizeof(struct fwu_hdr_t));
    cprintf(GREEN, "  Check: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    ret = memcmp(g_subblock_A.key_B, tail.key_B, 16);
    cprintf(GREEN, "  Compare: ");
    check_field(ret, 0, "Pass\n", "Fail\n");

    /*
    ret = memcmp(tail.guid, zero, sizeof(zero));
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
    struct fwu_hdr_t *hdr = (void *)buf;
    memset(keybuf, 0, sizeof(keybuf));
    int ret = get_key_fwu_v3(*size, buf, &blockA, &blockB, keybuf, block);
    if(ret != 0)
        return ret;

    size_t file_size = *size;
    /* the input buffer is reorganized based on two offsets (blockA and blockB),
     * skip 2048 bytes of data used for crypto init */
    *size = hdr->fw_size; /* use firmware size, not file size */
    *size -= 512 + 1024 + 512; /* sector0 + blockA + blockB */
    uint8_t *tmpbuf = malloc(*size);
    memset(tmpbuf, 0, *size);
    int offsetA = 512 * (1 + blockA);
    int offsetB = 512 * (1 + blockB);
    memcpy(tmpbuf, buf + 512, offsetA);
    memcpy(tmpbuf + offsetA, buf + 512 + offsetA + 1024, offsetB);
    memcpy(tmpbuf + offsetA + offsetB,
        buf + 512 + offsetA + 1024 + offsetB + 512, *size - offsetA - offsetB);
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
     * use the same version above). */
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
        rc4_key_schedule(keybuf, 32, g_rc4_S);
        rc4_stream_cipher(tmpbuf, *size, g_rc4_S);
        memcpy(buf, tmpbuf, *size);
    }

    return 0;
}

uint32_t fwu_checksum(void *buf, size_t size)
{
    if(size % 4)
        cprintf(GREY, "WARNING: checksum of buffer whose length is not a multiple of 4");
    uint32_t *p = buf;
    uint32_t sum = 0;
    for(size_t i = 0; i < size / 4; i++)
        sum += *p++;
    return sum;
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

    cprintf_field("  Version: ", "0x%x ", hdr->version);
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

    /* check whether the firmware has a FwuTail (as far as I know, there is no flag anywhere that
     * indicates its presence or not) */
    struct fwu_tail_t *tail = (void *)(buf + hdr->fw_size - sizeof(struct fwu_tail_t));
    if(tail->flags == 0x55aa55aa && strcmp((char *)tail->desc, "FwuTail") == 0)
    {
        cprintf(BLUE, "Tail\n");
        cprintf_field("  Length: ", "%d ", tail->length);
        check_field_soft(tail->length, 1, "Ok\n", "Fail\n");
        cprintf_field("  Type: ", "%d ", tail->type);
        check_field_soft(tail->type, 7, "Ok\n", "Fail\n");
        cprintf_field("  FW checksum: ", "%x ", tail->fwu_checksum);
        check_field_soft(fwu_checksum(buf, hdr->fw_size - sizeof(struct fwu_tail_t)),
            tail->fwu_checksum, "Ok\n", "Mismatch\n");
        cprintf(GREEN, "  FW CRC Checksum: ");
        for(unsigned i = 0; i < sizeof(tail->fwu_crc_checksum); i++)
            cprintf(YELLOW, "%02x", tail->fwu_crc_checksum[i]);
        cprintf(RED, " Ignored (should be 0)\n");
        cprintf_field("  Tail checksum: ", "%x ", tail->fwutail_checksum);
        check_field_soft(fwu_checksum(tail, sizeof(struct fwu_tail_t) - 4),
            tail->fwutail_checksum, "Ok\n", "Mismatch\n");
        /* if it has a tail, the firmware size includes it, so we need to decrease it to avoid
         * "decrypting" the tail and output garbage */
        hdr->fw_size -= sizeof(struct fwu_tail_t);
    }
    else
        cprintf(BLUE, "Firmware does not seem to have a tail\n");

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
