/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 * CRC-7 table and overall shape from tools/mkspl-x1000.c,
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

/* Build an X1600 SPL flash image (PM 34.8, Table 34-4).
 *
 * 0x000..0x007  SPI signature and magic
 * 0x008         Flash type: 0x00 = NAND, 0xaa = NOR
 * 0x009         Code CRC-7
 * 0x00a         Pages per block / 32
 * 0x00b         Bytes per page / 1024
 * 0x00c..0x00d  Total image length, little-endian
 * 0x00e         Parameter-block CRC-7
 * 0x00f         Header CRC-7 (first 15 bytes)
 * 0x100..0x1ff  SPL parameters
 * 0x200..0x7ff  Secure-boot key
 * 0x800..       SPL code, up to 24 KiB
 *
 * The CRC recurrence is table[(crc << 1) ^ byte]. The three checksums match
 * the saved R1 vendor image; -check verifies them against another image.
 *
 * Defaults retain the vendor parameter block and key bytes. Whether the
 * BootROM accepts -noparams or -nullkey has not been tested on the R1. The
 * parameter CRC covers 256 bytes; the saved image does not distinguish this
 * from a 16-byte CRC because both produce the same value. */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define SPL_HEADER_SIZE     512
#define SPL_KEY_SIZE        1536
#define SPL_CODE_OFFSET     (SPL_HEADER_SIZE + SPL_KEY_SIZE)    /* 0x800 */

/* PM 34.5: "SPL.text (not more than 24K, and 512 byte aligned)" */
#define SPL_CODE_SIZE       (24 * 1024)
#define SPL_MAX_SIZE        (SPL_CODE_OFFSET + SPL_CODE_SIZE)   /* 0x6800 */

/* PM 34.6: the parameter block lives at 0x100 and is 256 bytes. */
#define SPL_PARAMS_OFFSET   0x100
#define SPL_PARAMS_SIZE     256

#define FLASH_TYPE_NAND     0x00
#define FLASH_TYPE_NOR      0xaa

static const uint8_t flash_sig_magic[8] =
    {0x06, 0x05, 0x04, 0x03, 0x02, 0x55, 0xaa, 0x55};

/* Offsets within the 16-byte flag table. */
enum {
    FLAG_TYPE     = 0x08,
    FLAG_CRC7     = 0x09,
    FLAG_PPB      = 0x0a,
    FLAG_BPP      = 0x0b,
    FLAG_LEN_LSB  = 0x0c,
    FLAG_LEN_MSB  = 0x0d,
    FLAG_ENV_CRC  = 0x0e,
    FLAG_HEAD_CRC = 0x0f,
};

/* CRC-7 used throughout the flash header. Table lifted verbatim from
 * tools/mkspl-x1000.c; verified to reproduce all three CRCs in the vendor
 * X1600 image, so the X1000 and X1600 BootROMs use the same polynomial. */
static uint8_t crc7(const uint8_t* buf, size_t len)
{
    static const uint8_t t[256] = {
        0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
        0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
        0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
        0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
        0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
        0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
        0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
        0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
        0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
        0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
        0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
        0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
        0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
        0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
        0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
        0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
        0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
        0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
        0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
        0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
        0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
        0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
        0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
        0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
        0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
        0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
        0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
        0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
        0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
        0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
        0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
        0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
    };

    uint8_t crc = 0;
    while(len--)
        crc = t[(crc << 1) ^ *buf++];
    return crc;
}

static void die(const char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

static void put32(uint8_t* p, uint32_t v)
{
    p[0] = v & 0xff;
    p[1] = (v >> 8) & 0xff;
    p[2] = (v >> 16) & 0xff;
    p[3] = (v >> 24) & 0xff;
}

static uint32_t get32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ------------------------------------------------------------------------
 * SPL parameter block (PM 34.6).
 *
 * Everything below is copied out of the HiBy R1's own vendor image,
 * mtd0[0x100..0x16f]. Decoded, it is:
 *
 *   spl_length = 0x00004400  (17408; the 512-byte-aligned image size)
 *   cpu_freq   = 624000000   (matches CPAPCR M=52 N=1 OD1=2 off a 24 MHz XTAL)
 *   +0x0c      = 0x95073310  (same value as descriptor 2's CPCCR; the PM calls
 *                             this field "Reserved")
 *   nand_timing[0..3] = 14060e03 000f0c06 00000000 00001e00
 *
 *   descriptors, { saddr, paddr, value, poll_h, poll_l }, saddr/paddr are
 *   CPM-relative (the PM: "the higher 16bit address is 0xb000"):
 *     0x0010 0x0010 0x034051c1 poll_h=0x0000000c   CPAPCR, wait for lock bits
 *     0x0000 0x00d4 0x55773310 poll_l=0x00000007   CPCCR, wait via CPCSR
 *     0x0000 0x00d4 0x95073310 poll_h=0xf0000000   CPCCR again
 *     0x002c 0x002c 0x60000007 poll_l=0x10000000   DDRCDR, wait for !BUSY
 *     0x0074 0x0074 0x2000000f poll_l=0x10000000   SFCCDR, wait for !BUSY
 *     0xffff ...                                   terminator
 *
 * Reproducing it byte-for-byte means the BootROM does exactly what it does for
 * the vendor SPL, which is the only configuration this device is known to boot
 * in. If you change the SPL size you MUST update spl_length; the tool does
 * that for you below.
 * ------------------------------------------------------------------------ */
static const uint8_t vendor_params[SPL_PARAMS_SIZE] = {
    /* 0x00 */ 'I','N','G','E',  0x00,0x44,0x00,0x00,  0x00,0x7c,0x31,0x25,  0x10,0x33,0x07,0x95,
    /* 0x10 */ 0x03,0x0e,0x06,0x14,  0x06,0x0c,0x0f,0x00,  0x00,0x00,0x00,0x00,  0x00,0x1e,0x00,0x00,
    /* 0x20 */ 0x10,0x00,0x10,0x00,  0xc1,0x51,0x40,0x03,  0x0c,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
    /* 0x30 */ 0x00,0x00,0xd4,0x00,  0x10,0x33,0x77,0x55,  0x00,0x00,0x00,0x00,  0x07,0x00,0x00,0x00,
    /* 0x40 */ 0x00,0x00,0xd4,0x00,  0x10,0x33,0x07,0x95,  0x00,0x00,0x00,0xf0,  0x00,0x00,0x00,0x00,
    /* 0x50 */ 0x2c,0x00,0x2c,0x00,  0x07,0x00,0x00,0x60,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x10,
    /* 0x60 */ 0x74,0x00,0x74,0x00,  0x0f,0x00,0x00,0x20,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x10,
    /* 0x70 */ 0xff,0xff,0xff,0xff,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,  0x00,0x00,0x00,0x00,
    /* rest zero */
};

/* The two bytes the vendor image has at the start of the SC_Boot key area. */
static const uint8_t vendor_key_prefix[2] = {0x11, 0x11};

/* Offset of the "burned ddr id" in the signature block.
 *
 * The vendor image has 0a 00 0a 00 here, i.e. the value 10 stored twice as
 * LE16. The vendor SPL reads it from 0x80001080 (offset 0x80 of its own loaded
 * signature), checks that the two halves match, and prints "invalid burned ddr
 * id" if they do not -- then carries on with its single compiled-in DDR
 * parameter table regardless. It is not covered by any of the three CRCs.
 *
 * Rockbox's SPL has no use for it (ddr-x1600.c hardcodes the table), so it is
 * omitted by default; -ddrid=N exists so that a byte-exact reproduction of the
 * vendor image can be built as a self-test:
 *
 *   mkspl-x1600 -type=nand -ppb=2 -bpp=2 -ddrid=10 spl-code.bin out.img
 *   cmp <(head -c 17332 mtd0.bin) out.img
 */
#define SIG_DDR_ID_OFFSET   0x80

struct options {
    int flash_type;
    int pages_per_block;
    int bytes_per_page;
    int emit_params;
    int null_key;
    long ddr_id;        /* < 0 = do not write the field */
};

static void build_image(uint8_t* image, uint32_t total_len,
                        const uint8_t* code, uint32_t code_len,
                        const struct options* o)
{
    memset(image, 0, total_len);

    /* flag table */
    memcpy(image, flash_sig_magic, 8);
    image[FLAG_TYPE]    = o->flash_type & 0xff;
    image[FLAG_PPB]     = o->pages_per_block & 0xff;
    image[FLAG_BPP]     = o->bytes_per_page & 0xff;
    image[FLAG_LEN_LSB] = total_len & 0xff;
    image[FLAG_LEN_MSB] = (total_len >> 8) & 0xff;

    /* parameter block */
    if(o->emit_params) {
        memcpy(&image[SPL_PARAMS_OFFSET], vendor_params, SPL_PARAMS_SIZE);
        /* SPL length must be the 512-byte-aligned image size (PM 34.6:
         * "The length of SPL in bytes, and must be 512 byte aligned"). */
        put32(&image[SPL_PARAMS_OFFSET + 4], (total_len + 511) & ~511u);
    }

    /* "burned ddr id", two identical LE16 halves */
    if(o->ddr_id >= 0) {
        image[SIG_DDR_ID_OFFSET + 0] = o->ddr_id & 0xff;
        image[SIG_DDR_ID_OFFSET + 1] = (o->ddr_id >> 8) & 0xff;
        image[SIG_DDR_ID_OFFSET + 2] = o->ddr_id & 0xff;
        image[SIG_DDR_ID_OFFSET + 3] = (o->ddr_id >> 8) & 0xff;
    }

    /* SC_Boot key */
    if(!o->null_key)
        memcpy(&image[SPL_HEADER_SIZE], vendor_key_prefix,
               sizeof(vendor_key_prefix));

    /* code */
    memcpy(&image[SPL_CODE_OFFSET], code, code_len);

    /* CRCs, in dependency order: code and env first, then the head CRC over
     * the completed first fifteen bytes. */
    image[FLAG_CRC7]     = crc7(&image[SPL_CODE_OFFSET], code_len);
    image[FLAG_ENV_CRC]  = crc7(&image[SPL_PARAMS_OFFSET], SPL_PARAMS_SIZE);
    image[FLAG_HEAD_CRC] = crc7(image, 15);
}

static uint8_t* read_file(const char* filename, uint32_t* rlength, long maxlen)
{
    FILE* f = fopen(filename, "rb");
    if(!f)
        die("cannot open input file: %s", filename);

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if(sz < 0)
        die("error seeking in input file");
    if(maxlen > 0 && sz > maxlen)
        die("input is too big (size: %ld, max: %ld)", sz, maxlen);

    uint8_t* buf = malloc(sz ? sz : 1);
    if(!buf)
        die("out of memory");

    fseek(f, 0, SEEK_SET);
    if(sz > 0 && fread(buf, sz, 1, f) != 1)
        die("error reading input file");

    fclose(f);
    *rlength = (uint32_t)sz;
    return buf;
}

/* Re-verify an existing image. Works on a Rockbox image or on a raw mtd0
 * dump, and is the only self-test this tool has. */
static int check_image(const char* filename)
{
    uint32_t len;
    uint8_t* d = read_file(filename, &len, 0);
    int bad = 0;

    if(len < SPL_CODE_OFFSET)
        die("%s: too short to be an SPL image (%u bytes)", filename, len);

    if(memcmp(d, flash_sig_magic, 8) != 0) {
        printf("magic:     BAD\n");
        bad = 1;
    } else {
        printf("magic:     ok\n");
    }

    uint32_t total = d[FLAG_LEN_LSB] | (d[FLAG_LEN_MSB] << 8);
    printf("type:      0x%02x (%s)\n", d[FLAG_TYPE],
           d[FLAG_TYPE] == FLASH_TYPE_NAND ? "SPI-NAND" :
           d[FLAG_TYPE] == FLASH_TYPE_NOR  ? "SPI-NOR"  : "UNKNOWN");
    printf("ppb:       %u  (pages per block = %u)\n", d[FLAG_PPB], d[FLAG_PPB] * 32u);
    printf("bpp:       %u  (bytes per page  = %u)\n", d[FLAG_BPP], d[FLAG_BPP] * 1024u);
    printf("Len:       0x%04x (%u), code = %u bytes\n",
           total, total, total > SPL_CODE_OFFSET ? total - SPL_CODE_OFFSET : 0);

    if(total < SPL_CODE_OFFSET || total > len) {
        printf("Len:       BAD (file is %u bytes)\n", len);
        return 1;
    }

    struct { const char* name; int off; uint8_t got; } c[3] = {
        { "code crc7", FLAG_CRC7,
          crc7(&d[SPL_CODE_OFFSET], total - SPL_CODE_OFFSET) },
        { "env crc7 ", FLAG_ENV_CRC,
          crc7(&d[SPL_PARAMS_OFFSET], SPL_PARAMS_SIZE) },
        { "head crc7", FLAG_HEAD_CRC, crc7(d, 15) },
    };
    for(int i = 0; i < 3; ++i) {
        int ok = (c[i].got == d[c[i].off]);
        printf("%s: stored 0x%02x computed 0x%02x  %s\n",
               c[i].name, d[c[i].off], c[i].got, ok ? "ok" : "MISMATCH");
        if(!ok)
            bad = 1;
    }

    if(memcmp(&d[SPL_PARAMS_OFFSET], "INGE", 4) == 0) {
        printf("params:    INGE present, spl_length = 0x%08x, cpu_freq = %u Hz\n",
               get32(&d[SPL_PARAMS_OFFSET + 4]),
               get32(&d[SPL_PARAMS_OFFSET + 8]));
        for(int i = 0; i < 14; ++i) {
            const uint8_t* e = &d[SPL_PARAMS_OFFSET + 0x20 + 16 * i];
            unsigned saddr = e[0] | (e[1] << 8);
            if(saddr == 0xffff)
                break;
            printf("  desc[%2d] CPM+0x%04x = 0x%08x, poll CPM+0x%04x "
                   "high 0x%08x low 0x%08x\n",
                   i, saddr, get32(&e[4]), (unsigned)(e[2] | (e[3] << 8)),
                   get32(&e[8]), get32(&e[12]));
        }
    } else {
        printf("params:    absent (BootROM will use defaults)\n");
    }

    free(d);
    return bad;
}

static void usage(void)
{
    fprintf(stderr,
        "usage: mkspl-x1600 -type=nand|nor [-ppb=N] [-bpp=N]\n"
        "                   [-noparams] [-nullkey] [-ddrid=N]\n"
        "                   <spl.bin> <out.img>\n"
        "       mkspl-x1600 -check <image>\n"
        "\n"
        "  -ppb=N      pages per block / 32   (W25N01GV: 2)\n"
        "  -bpp=N      bytes per page / 1024  (W25N01GV: 2)\n"
        "  -noparams   omit the INGE parameter block at 0x100\n"
        "  -nullkey    write an all-zero SC_Boot key area\n"
        "  -ddrid=N    write the vendor's \"burned ddr id\" at signature+0x80\n"
        "  -check      re-verify an existing image (also works on mtd0 dumps)\n");
    exit(1);
}

int main(int argc, const char* argv[])
{
    struct options o = {
        .flash_type = -1,
        .pages_per_block = -1,
        .bytes_per_page = -1,
        .emit_params = 1,
        .null_key = 0,
        .ddr_id = -1,
    };
    uint8_t* code = NULL;
    uint32_t code_len = 0;
    const char* outfile_name = NULL;

    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "-check")) {
            if(i + 1 >= argc)
                usage();
            return check_image(argv[i + 1]);
        } else if(!strncmp(argv[i], "-type=", 6)) {
            if(!strcmp(argv[i] + 6, "nand"))
                o.flash_type = FLASH_TYPE_NAND;
            else if(!strcmp(argv[i] + 6, "nor"))
                o.flash_type = FLASH_TYPE_NOR;
            else
                die("invalid type: %s", argv[i] + 6);
        } else if(!strncmp(argv[i], "-ppb=", 5)) {
            o.pages_per_block = atoi(argv[i] + 5);
        } else if(!strncmp(argv[i], "-bpp=", 5)) {
            o.bytes_per_page = atoi(argv[i] + 5);
        } else if(!strcmp(argv[i], "-noparams")) {
            o.emit_params = 0;
        } else if(!strcmp(argv[i], "-nullkey")) {
            o.null_key = 1;
        } else if(!strncmp(argv[i], "-ddrid=", 7)) {
            o.ddr_id = strtol(argv[i] + 7, NULL, 0);
            if(o.ddr_id < 0 || o.ddr_id > 0xffff)
                die("invalid -ddrid (must fit in 16 bits)");
        } else if(argv[i][0] == '-') {
            usage();
        } else if(code == NULL) {
            code = read_file(argv[i], &code_len, SPL_CODE_SIZE);
        } else if(!outfile_name) {
            outfile_name = argv[i];
        } else {
            die("too many arguments: %s", argv[i]);
        }
    }

    if(o.flash_type < 0)
        die("must specify -type");

    if(o.flash_type == FLASH_TYPE_NAND) {
        if(o.pages_per_block < 0)
            die("must specify -ppb with -type=nand");
        if(o.bytes_per_page < 0)
            die("must specify -bpp with -type=nand");
        if(o.pages_per_block > 0xff)
            die("invalid ppb (it is pages_per_block/32, so 2 for a W25N01GV)");
        if(o.bytes_per_page > 0xff)
            die("invalid bpp (it is bytes_per_page/1024, so 2 for a W25N01GV)");
    } else {
        o.pages_per_block = 0;
        o.bytes_per_page = 0;
    }

    if(!code)
        die("no input file given");
    if(!outfile_name)
        die("no output file given");
    if(code_len == 0)
        die("input file is empty");

    uint32_t total_len = SPL_CODE_OFFSET + code_len;
    if(total_len > SPL_MAX_SIZE)
        die("image too big: %u bytes, max %u", total_len, SPL_MAX_SIZE);
    if(total_len > 0xffff)
        die("image too big for the 16-bit Len field: %u bytes", total_len);

    uint8_t* image = malloc(total_len);
    if(!image)
        die("out of memory");

    build_image(image, total_len, code, code_len, &o);

    FILE* out = fopen(outfile_name, "wb");
    if(!out)
        die("cannot open output file: %s", outfile_name);
    if(fwrite(image, total_len, 1, out) != 1)
        die("error writing output");
    fclose(out);

    free(image);
    free(code);
    return 0;
}
