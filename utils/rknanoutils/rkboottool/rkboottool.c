#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include "misc.h"
#include "elf.h"

#define cprintf(col, ...) do {color(col); printf(__VA_ARGS__); }while(0)

bool g_debug = false;

typedef uint8_t packed_bcd_uint8_t;
typedef uint16_t packed_bcd_uint16_t;

/**
 * RKnanoFW
 * contains resources and code stages
 */

struct rknano_date_t
{
    packed_bcd_uint16_t year;
    packed_bcd_uint8_t mday;
    packed_bcd_uint8_t month;
};

struct rknano_version_t
{
    packed_bcd_uint16_t major;
    packed_bcd_uint16_t minor;
    packed_bcd_uint16_t rev;
};

struct rknano_image_t
{
    uint16_t width;
    uint16_t height;
    uint8_t data[0];
};

struct rknano_blob_t
{
    uint32_t offset;
    uint32_t size;
};

#define VENDOR_NAME_SIZE    32
#define MODEL_NAME_SIZE     32
#define MAX_NR_STAGES       4
#define MAX_NR_FONTS        10
#define MAX_NR_GBK          5
#define MAX_NR_STRTBL       10
#define MAX_NR_IMAGERES     10
#define MAX_NR_UNK          10
#define MAGIC_RKNANOFW      "RKnanoFW"
#define MAGIC_RKNANOFW_SIZE 8

struct rknano_header_t
{
    struct rknano_date_t date;
    struct rknano_version_t version;
    uint8_t unk6[6];
    char vendor[VENDOR_NAME_SIZE];
    char model[MODEL_NAME_SIZE];
    uint32_t nr_stages;
    struct rknano_blob_t stage[MAX_NR_STAGES];
    uint32_t nr_fonts;
    struct rknano_blob_t font[MAX_NR_FONTS];
    uint32_t nr_gbk;
    struct rknano_blob_t gbk[MAX_NR_GBK];
    uint32_t nr_strtbl;
    struct rknano_blob_t strtbl[MAX_NR_STRTBL];
    uint32_t nr_imageres;
    struct rknano_blob_t imageres[MAX_NR_IMAGERES];
    uint32_t nr_unk;
    struct rknano_blob_t unk[MAX_NR_UNK];
    uint32_t pad;
    uint32_t size;
    char magic[MAGIC_RKNANOFW_SIZE];
};

char *g_out_prefix = NULL;

static void encode_page(uint8_t *inpg, uint8_t *outpg, const int size)
{

uint8_t key[] = {
        0x7C, 0x4E, 0x03, 0x04,
        0x55, 0x05, 0x09, 0x07,
        0x2D, 0x2C, 0x7B, 0x38,
        0x17, 0x0D, 0x17, 0x11
};
        int i, i3, x, val, idx;

        uint8_t key1[0x100];
        uint8_t key2[0x100];

        for (i=0; i<0x100; i++) {
                key1[i] = i;
                key2[i] = key[i&0xf];
        }

        i3 = 0;
        for (i=0; i<0x100; i++) {
                x = key1[i];
                i3 = key1[i] + i3;
                i3 += key2[i];
                i3 &= 0xff;
                key1[i] = key1[i3];
                key1[i3] = x;
        }

        idx = 0;
        for (i=0; i<size; i++) {
                x = key1[(i+1) & 0xff];
                val = x;
                idx = (x + idx) & 0xff;
                key1[(i+1) & 0xff] = key1[idx];
                key1[idx] = (x & 0xff);
                val = (key1[(i+1)&0xff] + x) & 0xff;
                val = key1[val];
                outpg[i] = val ^ inpg[i];
        }
}

static uint16_t crc(uint8_t *buf, int size)
{
    uint16_t result = 65535;
    for(; size; buf++, size--)
    {
        for(int bit = 128; bit; bit >>= 1)
        {
            if(result & 0x8000)
                result = (2 * result) ^ 0x1021;
            else
                result *= 2;
            if(*buf & bit)
                result ^= 0x1021;
        }
    }
    return result;
}

/* scramble mode */
enum {
    NO_ENC,
    CONTINOUS_ENC, /* scramble whole block at once */
    PAGE_ENC       /* nand bootloader is scrambled in 0x200 chunks */
};

static void save_blob(const struct rknano_blob_t *b, void *buf, uint32_t size,
    char *name, int suffix, int enc_mode)
{
    if(g_out_prefix == NULL || b->size == 0 || b->offset + b->size > size)
        return;
    char *path = malloc(strlen(g_out_prefix) + strlen(name) + 32);
    if(suffix >= 0)
        sprintf(path, "%s%s%d.bin", g_out_prefix, name, suffix);
    else
        sprintf(path, "%s%s.bin", g_out_prefix, name);
    FILE *f = fopen(path, "wb");
    uint8_t *ptr = buf + b->offset;
    if(enc_mode != NO_ENC)
    {
        ptr = malloc(b->size);
        int len = b->size;
        uint8_t *buff_ptr = buf + b->offset;
        uint8_t *out_ptr = ptr;
        if(enc_mode == PAGE_ENC)
        {
            while(len >= 0x200)
            {
                encode_page(buff_ptr, out_ptr, 0x200);
                buff_ptr += 0x200;
                out_ptr += 0x200;
                len -= 0x200;
            }
        }
        encode_page(buff_ptr, out_ptr, len);
    }

    if(f)
    {
        fwrite(ptr, b->size, 1, f);
        fclose(f);
    }

    if(enc_mode != NO_ENC)
        free(ptr);
}

static void print_blob_interval(const struct rknano_blob_t *b)
{
    cprintf(YELLOW, "%#x -> %#x", b->offset, b->offset + b->size);
}

static int do_nanofw_image(uint8_t *buf, unsigned long size)
{
    if(size < sizeof(struct rknano_header_t))
        return 1;
    struct rknano_header_t *hdr = (void *)buf;
    if(size < hdr->size)
        return 1;
    if(strncmp(hdr->magic, MAGIC_RKNANOFW, MAGIC_RKNANOFW_SIZE))
        return 1;

    cprintf(BLUE, "Header\n");
    cprintf(GREEN, "  Date: ");
    cprintf(YELLOW, "%x/%x/%x\n", hdr->date.mday, hdr->date.month, hdr->date.year);
    cprintf(GREEN, "  Version: ");
    cprintf(YELLOW, "%x.%x.%x\n", hdr->version.major, hdr->version.minor, hdr->version.rev);
    cprintf(GREEN, "  Vendor: ");
    cprintf(YELLOW, "%s\n", hdr->vendor);
    cprintf(GREEN, "  Model: ");
    cprintf(YELLOW, "%s\n", hdr->model);
    cprintf(GREEN, "  Pad: ");
    for(int i = 0; i < 6; i++)
        cprintf(YELLOW, " %02x", hdr->unk6[i]);
    cprintf(YELLOW, "\n");
    cprintf(BLUE, "Stages\n");
    for(unsigned i = 0; i < hdr->nr_stages; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->stage[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->stage[i], buf, size, "stage", i, i == 3 ? NO_ENC : CONTINOUS_ENC);
    }
    cprintf(BLUE, "Fonts\n");
    for(unsigned i = 0; i < hdr->nr_fonts; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->font[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->font[i], buf, size, "font", i, NO_ENC);
    }
    cprintf(BLUE, "GBK\n");
    for(unsigned i = 0; i < hdr->nr_gbk; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->gbk[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->gbk[i], buf, size, "gbk", i, NO_ENC);
    }
    cprintf(BLUE, "String Tables\n");
    for(unsigned i = 0; i < hdr->nr_strtbl; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->strtbl[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->strtbl[i], buf, size, "strtbl", i, NO_ENC);
    }
    cprintf(BLUE, "Image Resources\n");
    for(unsigned i = 0; i < hdr->nr_imageres; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->imageres[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->imageres[i], buf, size, "imgres", i, NO_ENC);
    }
    cprintf(BLUE, "Unknown\n");
    for(unsigned i = 0; i < hdr->nr_unk; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->unk[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->unk[i], buf, size, "unk", i, NO_ENC);
    }
    cprintf(BLUE, "Other\n");
    cprintf(GREEN, "  Size: ");
    cprintf(YELLOW, "%#x\n", hdr->size);
    cprintf(GREEN, "  Magic: ");
    cprintf(YELLOW, "%." STR(MAGIC_RKNANOFW_SIZE) "s  ", hdr->magic);
    if(strncmp(hdr->magic, MAGIC_RKNANOFW, MAGIC_RKNANOFW_SIZE) == 0)
        cprintf(RED, "OK\n");
    else
        cprintf(RED, "Mismatch\n");

    return 0;
}

/**
 * RKNano stage
 * contains code and memory mapping
 */

struct rknano_stage_header_t
{
    uint32_t addr;
    uint32_t count;
} __attribute__((packed));

/*
 * NOTE this theory has not been tested against actual code, it's still a guess
 * The firmware is too big to fit in memory so it's split into sections,
 * each section having a "virtual address" and a "physical address".
 * Except it gets tricky because the RKNano doesn't have a MMU but a MPU,
 * so most probably the OF divides the memory into regions (8 would match
 * hardware capabilities), each being able to contain one of the sections
 * in the OF file. To gracefully handle jumps between sections, my guess is
 * that the entire OF is linked as a flat image, cut into pieces and
 * then each code section get relocated except for jump/calls outside of it:
 * this will trigger an access fault when trying to access another section, which
 * the OF can trap and then load the corresponding section.
 */

struct rknano_stage_section_t
{
    uint32_t code_va;
    uint32_t code_pa;
    uint32_t code_sz;
    uint32_t data_va;
    uint32_t data_pa;
    uint32_t data_sz;
    uint32_t bss_pa;
    uint32_t bss_sz;
} __attribute__((packed));

static void elf_printf(void *user, bool error, const char *fmt, ...)
{
    if(!g_debug && !error)
        return;
    (void) user;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

static void elf_write(void *user, uint32_t addr, const void *buf, size_t count)
{
    FILE *f = user;
    fseek(f, addr, SEEK_SET);
    fwrite(buf, count, 1, f);
}

static void extract_elf_section(struct elf_params_t *elf)
{
    if(g_out_prefix == NULL)
        return;
    char *filename = xmalloc(strlen(g_out_prefix) + 32);
    sprintf(filename, "%s.elf", g_out_prefix);
    if(g_debug)
        printf("Write stage to %s\n", filename);

    FILE *fd = fopen(filename, "wb");
    free(filename);

    if(fd == NULL)
        return ;
    elf_write_file(elf, elf_write, elf_printf, fd);
    fclose(fd);
}

struct range_t
{
    unsigned long start, size;
    int section;
    int type;
};

int range_cmp(const void *_a, const void *_b)
{
    const struct range_t *a = _a, *b = _b;
    if(a->start == b->start)
        return a->size - b->size;
    return a->start - b->start;
}

#define RANGE_TXT   0
#define RANGE_DAT   1

static int do_nanostage_image(uint8_t *buf, unsigned long size)
{
    if(size < sizeof(struct rknano_stage_section_t))
        return 1;
    struct rknano_stage_header_t *hdr = (void *)buf;
    size_t hdr_size = sizeof(struct rknano_stage_header_t) +
        hdr->count * sizeof(struct rknano_stage_section_t);
    if(size < hdr_size)
        return 1;

    cprintf(BLUE, "Header\n");
    cprintf(GREEN, "  Base Address: ");
    cprintf(YELLOW, "%#08x\n", hdr->addr);
    cprintf(GREEN, "  Section count: ");
    cprintf(YELLOW, "%d\n", hdr->count);

    struct rknano_stage_section_t *sec = (void *)(hdr + 1);
    struct elf_params_t elf;
    elf_init(&elf);
    bool error = false;
    /* track range for overlap */
    struct range_t *ranges = malloc(sizeof(struct range_t) * 2 * hdr->count);
    int nr_ranges = 0;
    for(unsigned i = 0; i < hdr->count; i++, sec++)
    {
        cprintf(BLUE, "Section %d\n", i);
        cprintf(GREEN, "  Code: ");
        cprintf(YELLOW, "0x%08x", sec->code_va);
        cprintf(RED, "-(txt)-");
        cprintf(YELLOW, "0x%08x", sec->code_va + sec->code_sz);
        cprintf(BLUE, " |--> ");
        cprintf(YELLOW, "0x%08x", sec->code_pa);
        cprintf(RED, "-(txt)-");
        cprintf(YELLOW, "0x%08x\n", sec->code_pa + sec->code_sz);

        /* add ranges */
        ranges[nr_ranges].start = sec->code_va;
        ranges[nr_ranges].size = sec->code_sz;
        ranges[nr_ranges].section = i;
        ranges[nr_ranges].type = RANGE_TXT;
        ranges[nr_ranges + 1].start = sec->data_va;
        ranges[nr_ranges + 1].size = sec->data_sz;
        ranges[nr_ranges + 1].section = i;
        ranges[nr_ranges + 1].type = RANGE_DAT;
        nr_ranges += 2;

        cprintf(GREEN, "  Data: ");
        cprintf(YELLOW, "0x%08x", sec->data_va);
        cprintf(RED, "-(dat)-");
        cprintf(YELLOW, "0x%08x", sec->data_va + sec->data_sz);
        cprintf(BLUE, " |--> ");
        cprintf(YELLOW, "0x%08x", sec->data_pa);
        cprintf(RED, "-(dat)-");
        cprintf(YELLOW, "0x%08x\n", sec->data_pa + sec->data_sz);

        cprintf(GREEN, "  Data: ");
        cprintf(RED, "                           ");
        cprintf(BLUE, " |--> ");
        cprintf(YELLOW, "0x%08x", sec->bss_pa);
        cprintf(RED, "-(bss)-");
        cprintf(YELLOW, "0x%08x\n", sec->bss_pa + sec->bss_sz);

#define check_range_(start,sz) \
        ((start) >= hdr_size && (start) + (sz) <= size)
#define check_range(start,sz) \
        ((start) >= hdr->addr && check_range_((start) - hdr->addr, sz))
        /* check ranges */
        if(sec->code_sz != 0 && !check_range(sec->code_va, sec->code_sz))
        {
            cprintf(GREY, "Invalid stage: out of bound code\n");
            error = true;
            break;
        }
        if(sec->data_sz != 0 && !check_range(sec->data_va, sec->data_sz))
        {
            cprintf(GREY, "Invalid stage: out of bound data\n");
            error = true;
            break;
        }
#undef check_range_
#undef check_range

        char buffer[32];
        if(sec->code_sz != 0)
        {
            sprintf(buffer, ".text.%d", i);
            elf_add_load_section(&elf, sec->code_va, sec->code_sz,
                buf + sec->code_va - hdr->addr, buffer);
        }
        if(sec->data_sz != 0)
        {
            sprintf(buffer, ".data.%d", i);
            elf_add_load_section(&elf, sec->data_va, sec->data_sz,
                buf + sec->data_va - hdr->addr, buffer);
        }
    }
    /* sort ranges and check overlap */
    qsort(ranges, nr_ranges, sizeof(struct range_t), range_cmp);
    for(int i = 1; i < nr_ranges; i++)
    {
        if(ranges[i - 1].start + ranges[i - 1].size > ranges[i].start)
        {
            error = true;
            static const char *type[] = {"txt", "dat"};
            cprintf(GREY, "Section overlap: section %d %s intersects section %d %s\n",
                ranges[i - 1].section, type[ranges[i - 1].type], ranges[i].section,
                type[ranges[i].type]);
            break;
        }
    }
    if(!error)
        extract_elf_section(&elf);
    /* FIXME for full information, we could add segments to the ELF file to
     * keep the mapping, but it's unclear if that would do any good */
    elf_release(&elf);
    free(ranges);

    return 0;
}

/**
 * RKNano BOOT
 * contains named bootloader stages
 */

#define MAGIC_BOOT      "BOOT"
#define MAGIC_BOOT_SIZE 4

struct rknano_boot_desc_t
{
    uint8_t count;
    uint32_t offset;
    uint8_t stride;
} __attribute__((packed));

struct rknano_boot_header_t
{
    char magic[MAGIC_BOOT_SIZE];
    uint16_t hdr_size;
    uint32_t version;
    uint32_t unk;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint32_t chip;
    struct rknano_boot_desc_t desc_1;
    struct rknano_boot_desc_t desc_2;
    struct rknano_boot_desc_t desc_4;
    uint8_t field_2B[9];
    uint32_t field_34;
} __attribute__((packed));

#define BOOT_CHIP_RKNANO    0x30

struct rknano_boot_entry_t
{
    uint8_t entry_size; // unsure
    uint32_t unk;
    uint16_t name[20];
    uint32_t offset;
    uint32_t size;
    uint32_t sthg2;
} __attribute__((packed));

uint32_t boot_crc_table[256] =
{
    0x00000000, 0x04C10DB7, 0x09821B6E, 0x0D4316D9,
    0x130436DC, 0x17C53B6B, 0x1A862DB2, 0x1E472005,
    0x26086DB8, 0x22C9600F, 0x2F8A76D6, 0x2B4B7B61,
    0x350C5B64, 0x31CD56D3, 0x3C8E400A, 0x384F4DBD,
    0x4C10DB70, 0x48D1D6C7, 0x4592C01E, 0x4153CDA9,
    0x5F14EDAC, 0x5BD5E01B, 0x5696F6C2, 0x5257FB75,
    0x6A18B6C8, 0x6ED9BB7F, 0x639AADA6, 0x675BA011,
    0x791C8014, 0x7DDD8DA3, 0x709E9B7A, 0x745F96CD,
    0x9821B6E0, 0x9CE0BB57, 0x91A3AD8E, 0x9562A039,
    0x8B25803C, 0x8FE48D8B, 0x82A79B52, 0x866696E5,
    0xBE29DB58, 0xBAE8D6EF, 0xB7ABC036, 0xB36ACD81,
    0xAD2DED84, 0xA9ECE033, 0xA4AFF6EA, 0xA06EFB5D,
    0xD4316D90, 0xD0F06027, 0xDDB376FE, 0xD9727B49,
    0xC7355B4C, 0xC3F456FB, 0xCEB74022, 0xCA764D95,
    0xF2390028, 0xF6F80D9F, 0xFBBB1B46, 0xFF7A16F1,
    0xE13D36F4, 0xE5FC3B43, 0xE8BF2D9A, 0xEC7E202D,
    0x34826077, 0x30436DC0, 0x3D007B19, 0x39C176AE,
    0x278656AB, 0x23475B1C, 0x2E044DC5, 0x2AC54072,
    0x128A0DCF, 0x164B0078, 0x1B0816A1, 0x1FC91B16,
    0x018E3B13, 0x054F36A4, 0x080C207D, 0x0CCD2DCA,
    0x7892BB07, 0x7C53B6B0, 0x7110A069, 0x75D1ADDE,
    0x6B968DDB, 0x6F57806C, 0x621496B5, 0x66D59B02,
    0x5E9AD6BF, 0x5A5BDB08, 0x5718CDD1, 0x53D9C066,
    0x4D9EE063, 0x495FEDD4, 0x441CFB0D, 0x40DDF6BA,
    0xACA3D697, 0xA862DB20, 0xA521CDF9, 0xA1E0C04E,
    0xBFA7E04B, 0xBB66EDFC, 0xB625FB25, 0xB2E4F692,
    0x8AABBB2F, 0x8E6AB698, 0x8329A041, 0x87E8ADF6,
    0x99AF8DF3, 0x9D6E8044, 0x902D969D, 0x94EC9B2A,
    0xE0B30DE7, 0xE4720050, 0xE9311689, 0xEDF01B3E,
    0xF3B73B3B, 0xF776368C, 0xFA352055, 0xFEF42DE2,
    0xC6BB605F, 0xC27A6DE8, 0xCF397B31, 0xCBF87686,
    0xD5BF5683, 0xD17E5B34, 0xDC3D4DED, 0xD8FC405A,
    0x6904C0EE, 0x6DC5CD59, 0x6086DB80, 0x6447D637,
    0x7A00F632, 0x7EC1FB85, 0x7382ED5C, 0x7743E0EB,
    0x4F0CAD56, 0x4BCDA0E1, 0x468EB638, 0x424FBB8F,
    0x5C089B8A, 0x58C9963D, 0x558A80E4, 0x514B8D53,
    0x25141B9E, 0x21D51629, 0x2C9600F0, 0x28570D47,
    0x36102D42, 0x32D120F5, 0x3F92362C, 0x3B533B9B,
    0x031C7626, 0x07DD7B91, 0x0A9E6D48, 0x0E5F60FF,
    0x101840FA, 0x14D94D4D, 0x199A5B94, 0x1D5B5623,
    0xF125760E, 0xF5E47BB9, 0xF8A76D60, 0xFC6660D7,
    0xE22140D2, 0xE6E04D65, 0xEBA35BBC, 0xEF62560B,
    0xD72D1BB6, 0xD3EC1601, 0xDEAF00D8, 0xDA6E0D6F,
    0xC4292D6A, 0xC0E820DD, 0xCDAB3604, 0xC96A3BB3,
    0xBD35AD7E, 0xB9F4A0C9, 0xB4B7B610, 0xB076BBA7,
    0xAE319BA2, 0xAAF09615, 0xA7B380CC, 0xA3728D7B,
    0x9B3DC0C6, 0x9FFCCD71, 0x92BFDBA8, 0x967ED61F,
    0x8839F61A, 0x8CF8FBAD, 0x81BBED74, 0x857AE0C3,
    0x5D86A099, 0x5947AD2E, 0x5404BBF7, 0x50C5B640,
    0x4E829645, 0x4A439BF2, 0x47008D2B, 0x43C1809C,
    0x7B8ECD21, 0x7F4FC096, 0x720CD64F, 0x76CDDBF8,
    0x688AFBFD, 0x6C4BF64A, 0x6108E093, 0x65C9ED24,
    0x11967BE9, 0x1557765E, 0x18146087, 0x1CD56D30,
    0x02924D35, 0x06534082, 0x0B10565B, 0x0FD15BEC,
    0x379E1651, 0x335F1BE6, 0x3E1C0D3F, 0x3ADD0088,
    0x249A208D, 0x205B2D3A, 0x2D183BE3, 0x29D93654,
    0xC5A71679, 0xC1661BCE, 0xCC250D17, 0xC8E400A0,
    0xD6A320A5, 0xD2622D12, 0xDF213BCB, 0xDBE0367C,
    0xE3AF7BC1, 0xE76E7676, 0xEA2D60AF, 0xEEEC6D18,
    0xF0AB4D1D, 0xF46A40AA, 0xF9295673, 0xFDE85BC4,
    0x89B7CD09, 0x8D76C0BE, 0x8035D667, 0x84F4DBD0,
    0x9AB3FBD5, 0x9E72F662, 0x9331E0BB, 0x97F0ED0C,
    0xAFBFA0B1, 0xAB7EAD06, 0xA63DBBDF, 0xA2FCB668,
    0xBCBB966D, 0xB87A9BDA, 0xB5398D03, 0xB1F880B4,
};

static uint32_t boot_crc(uint8_t *buf, int size)
{
    uint32_t crc = 0;
    for(int i = 0; i < size; i++)
        crc = boot_crc_table[buf[i] ^ (crc >> 24)] ^ (crc << 8);
    return crc;
}

wchar_t *from_uni16(uint16_t *str)
{
    static wchar_t buffer[64];
    int i = 0;
    while(str[i])
    {
        buffer[i] = str[i];
        i++;
    }
    return buffer;
}

static int do_boot_desc(uint8_t *buf, unsigned long size,
    struct rknano_boot_desc_t *desc, int desc_idx)
{
    (void) buf;
    (void) size;
    cprintf(BLUE, "Desc %d\n", desc_idx);
    cprintf(GREEN, "  Count: "); cprintf(YELLOW, "%d\n", desc->count);
    cprintf(GREEN, "  Offset: "); cprintf(YELLOW, "%#x\n", desc->offset);
    cprintf(GREEN, "  Stride: "); cprintf(YELLOW, "%#x ", desc->stride);
    if(desc->stride < sizeof(struct rknano_boot_entry_t))
        cprintf(RED, "(too small <%#lx)\n", sizeof(struct rknano_boot_entry_t));
    else
        cprintf(RED, "(OK >=%#lx)\n", sizeof(struct rknano_boot_entry_t));

    for(int i = 0; i < desc->count; i++)
    {
        struct rknano_boot_entry_t *entry = (void *)(buf + desc->offset + i * desc->stride);
        cprintf(BLUE, "  Entry %d\n", i);
        cprintf(GREEN, "    Entry size: "); cprintf(YELLOW, "%#x ", entry->entry_size);
        if(desc->stride < sizeof(struct rknano_boot_entry_t))
            cprintf(RED, "(too small <%#lx)\n", sizeof(struct rknano_boot_entry_t));
        else
            cprintf(RED, "(OK >=%#lx)\n", sizeof(struct rknano_boot_entry_t));
        cprintf(GREEN, "    Unk: "); cprintf(YELLOW, "%#x\n", entry->unk);
        cprintf(GREEN, "    Name: "); cprintf(YELLOW, "%S\n", from_uni16(entry->name));
        cprintf(GREEN, "    Offset: "); cprintf(YELLOW, "%#x\n", entry->offset);
        cprintf(GREEN, "    Size: "); cprintf(YELLOW, "%#x\n", entry->size);
        cprintf(GREEN, "    Sthg 2: "); cprintf(YELLOW, "%#x\n", entry->sthg2);

        struct rknano_blob_t blob;
        blob.offset = entry->offset;
        blob.size = entry->size;
        char name[128];
        sprintf(name, "%d.%S", desc_idx, from_uni16(entry->name));
        save_blob(&blob, buf, size, name, -1, PAGE_ENC);
    }

    return 0;
}

static int do_boot_image(uint8_t *buf, unsigned long size)
{
    if(size < sizeof(struct rknano_boot_header_t))
        return 1;
    struct rknano_boot_header_t *hdr = (void *)buf;
    if(strncmp(hdr->magic, MAGIC_BOOT, MAGIC_BOOT_SIZE))
        return 1;

    cprintf(BLUE, "Header\n");
    cprintf(GREEN, "  Magic: ");
    cprintf(YELLOW, "%." STR(MAGIC_BOOT_SIZE) "s  ", hdr->magic);
    if(strncmp(hdr->magic, MAGIC_BOOT, MAGIC_BOOT_SIZE) == 0)
        cprintf(RED, "OK\n");
    else
        cprintf(RED, "Mismatch\n");

    cprintf(GREEN, "  Header Size: ");
    cprintf(YELLOW, "%#x ", hdr->hdr_size);
    if(hdr->hdr_size >= sizeof(struct rknano_boot_header_t))
        cprintf(RED, "OK\n");
    else
        cprintf(RED, "Mismatch\n");

#define print(str, name) cprintf(GREEN, "  "str": ");cprintf(YELLOW, "%#x\n", (unsigned)hdr->name)
#define print_arr(str, name, sz) \
    cprintf(GREEN, "  "str":");for(int i = 0; i < sz; i++)cprintf(YELLOW, " %#x", (unsigned)hdr->name[i]);printf("\n")

    cprintf(GREEN, "  Version: ");
    cprintf(YELLOW, "%x.%x.%x\n", (hdr->version >> 24) & 0xff,
        (hdr->version >> 16) & 0xff, hdr->version & 0xffff);

    cprintf(GREEN, "  Date: ");
    cprintf(YELLOW, "%d/%d/%d %02d:%02d:%02d\n", hdr->day, hdr->month, hdr->year,
        hdr->hour, hdr->minute, hdr->second);

    cprintf(GREEN, "  Chip: ");
    cprintf(YELLOW, "%#x ", hdr->chip);
    if(hdr->chip == BOOT_CHIP_RKNANO)
        cprintf(RED, "(RKNANO)\n");
    else
        cprintf(RED, "(unknown)\n");

    print_arr("field_2A", field_2B, 9);
    print("field_34", field_34);

    do_boot_desc(buf, size, &hdr->desc_1, 1);
    do_boot_desc(buf, size, &hdr->desc_2, 2);
    do_boot_desc(buf, size, &hdr->desc_4, 4);

    cprintf(BLUE, "Variable Header:\n");
    cprintf(GREEN, "  Value: ");
    cprintf(YELLOW, "%#lx\n", *(unsigned long *)((uint8_t *)hdr + hdr->field_34 - 10));

    /* The last 4 bytes are a 32-bit CRC */
    cprintf(BLUE, "Post Header:\n");
    cprintf(GREEN, "  CRC: ");
    uint32_t crc = *(uint32_t *)(buf + size - 4);
    uint32_t ccrc = boot_crc(buf, size - 4);
    cprintf(YELLOW, "%08x ", crc);
    if(crc == ccrc)
        cprintf(RED, "OK\n");
    else
        cprintf(RED, "Mismatch\n");

    return 0;
}

/**
 * RKFW
 * contains bootloader and update
 */

typedef struct rknano_blob_t rkfw_blob_t;

#define MAGIC_RKFW      "RKFW"
#define MAGIC_RKFW_SIZE 4

struct rkfw_header_t
{
    char magic[MAGIC_RKFW_SIZE];
    uint16_t hdr_size;
    uint32_t version;
    uint32_t code;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint32_t chip;
    rkfw_blob_t loader;
    rkfw_blob_t update;
    uint8_t pad[61];
} __attribute__((packed));

#define RKFW_CHIP_RKNANO    0x30

static int do_rkfw_image(uint8_t *buf, unsigned long size)
{
    if(size < sizeof(struct rkfw_header_t))
        return 1;
    struct rkfw_header_t *hdr = (void *)buf;
    if(strncmp(hdr->magic, MAGIC_RKFW, MAGIC_RKFW_SIZE))
        return 1;

    cprintf(BLUE, "Header\n");
    cprintf(GREEN, "  Magic: ");
    cprintf(YELLOW, "%." STR(MAGIC_RKFW_SIZE) "s  ", hdr->magic);
    if(strncmp(hdr->magic, MAGIC_RKFW, MAGIC_RKFW_SIZE) == 0)
        cprintf(RED, "OK\n");
    else
        cprintf(RED, "Mismatch\n");

    cprintf(GREEN, "  Header size: ");
    cprintf(YELLOW, " %#x ", hdr->hdr_size);
    if(hdr->hdr_size == sizeof(struct rkfw_header_t))
        cprintf(RED, "OK\n");
    else
        cprintf(RED, "Mismatch\n");
    cprintf(GREEN, "  Version: ");
    cprintf(YELLOW, "%x.%x.%x\n", (hdr->version >> 24) & 0xff,
        (hdr->version >> 16) & 0xff, hdr->version & 0xffff);

    cprintf(GREEN, "  Code: ");
    cprintf(YELLOW, "%#x\n", hdr->code);

    cprintf(GREEN, "  Date: ");
    cprintf(YELLOW, "%d/%d/%d %02d:%02d:%02d\n", hdr->day, hdr->month, hdr->year,
        hdr->hour, hdr->minute, hdr->second);

    cprintf(GREEN, "  Chip: ");
    cprintf(YELLOW, "%#x ", hdr->chip);
    if(hdr->chip == RKFW_CHIP_RKNANO)
        cprintf(RED, "(RKNANO)\n");
    else
        cprintf(RED, "(unknown)\n");

    cprintf(GREEN, "  Loader: ");
    print_blob_interval(&hdr->loader);
    cprintf(OFF, "\n");
    save_blob(&hdr->loader, buf, size, "loader", 0, NO_ENC);

    cprintf(GREEN, "  Update: ");
    print_blob_interval(&hdr->update);
    cprintf(OFF, "\n");
    save_blob(&hdr->update, buf, size, "update", 0, NO_ENC);

    print_arr("pad", pad, 61);

    return 0;
}

static int do_rkencode_image(uint8_t *buf, unsigned long size, int enc_mode)
{
    void *ptr = malloc(size);
    int len = size;
    uint8_t *buff_ptr = buf;
    uint8_t *out_ptr = ptr;
    if(enc_mode == PAGE_ENC)
    {
        while(len >= 0x200)
        {
            encode_page(buff_ptr, out_ptr, 0x200);
            buff_ptr += 0x200;
            out_ptr += 0x200;
            len -= 0x200;
        }
    }
    encode_page(buff_ptr, out_ptr, len);

    if(g_out_prefix)
    {
        FILE *f = fopen(g_out_prefix, "wb");
        if(f)
        {
            fwrite(out_ptr, 1, size, f);
            fclose(f);
        }
        else
            printf("Cannot open output file: %m\n");
    }
    free(ptr);

    return 0;
}

static void usage(void)
{
    printf("Usage: rkboottool [options] rknanoboot.bin\n");
    printf("Options:\n");
    printf("  --rkfw\tUnpack a rkfw file\n");
    printf("  --rknanofw\tUnpack a regular RknanoFW file\n");
    printf("  --rkboot\tUnpack a BOOT file\n");
    printf("  --rknanostage\tUnpack a RknanoFW stage file\n");
    printf("  --rkencode\tEncode a raw file in page mode\n");
    printf("  --rkencode2\tEncode a raw file in continuous mode\n");
    printf("  -o <prefix>\tSet output prefix\n");
    printf("The default is to try to guess the format.\n");
    printf("If several formats are specified, all are tried.\n");
    exit(1);
}

int main(int argc, char **argv)
{
    bool try_nanofw = false;
    bool try_rkfw = false;
    bool try_boot = false;
    bool try_nanostage = false;
    bool try_rkencode = false;
    bool try_rkencode2 = false;

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"rkfw", no_argument, 0, '9'},
            {"rknanofw", no_argument, 0, 'n'},
            {"rknanostage", no_argument, 0, 's'},
            {"rkencode", no_argument, 0, 'e'},
            {"rkencode2", no_argument, 0, 'E'},
            {"rkboot", no_argument, 0, 'b'},
            {"no-color", no_argument, 0, 'c'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?d9nscbeEo:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'c':
                enable_color(false);
                break;
            case 'b':
                try_boot = true;
                break;
            case 'n':
                try_nanofw = true;
                break;
            case 'd':
                g_debug = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case '9':
                try_rkfw = true;
                break;
            case 's':
                try_nanostage = true;
                break;
            case 'e':
                try_rkencode = true;
                break;
            case 'E':
                try_rkencode2 = true;
                break;
            default:
                printf("Invalid argument '%c'\n", c);
                abort();
        }
    }

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    if(!try_nanostage && !try_rkfw && !try_nanofw && !try_boot && !try_rkencode && !try_rkencode2)
        try_nanostage = try_rkfw = try_nanofw = try_boot = true;

    FILE *fin = fopen(argv[optind], "r");
    if(fin == NULL)
    {
        perror("Cannot open boot file");
        return 1;
    }
    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    void *buf = malloc(size);
    if(buf == NULL)
    {
        perror("Cannot allocate memory");
        return 1;
    }

    if(fread(buf, size, 1, fin) != 1)
    {
        perror("Cannot read file");
        return 1;
    }

    fclose(fin);

    if(try_nanofw && !do_nanofw_image(buf, size))
        goto Lsuccess;
    if(try_rkfw && !do_rkfw_image(buf, size))
        goto Lsuccess;
    if(try_boot && !do_boot_image(buf, size))
        goto Lsuccess;
    if(try_nanostage && !do_nanostage_image(buf, size))
        goto Lsuccess;
    if(try_rkencode && !do_rkencode_image(buf, size, PAGE_ENC))
        goto Lsuccess;
    if(try_rkencode2 && !do_rkencode_image(buf, size, CONTINOUS_ENC))
        goto Lsuccess;
    cprintf(GREY, "No valid format found!\n");
    Lsuccess:
    free(buf);

    return 0;
}
