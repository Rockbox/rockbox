#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "misc.h"

#define cprintf(col, ...) do {color(col); printf(__VA_ARGS__); }while(0)

bool g_debug = false;

typedef uint8_t packed_bcd_uint8_t;
typedef uint16_t packed_bcd_uint16_t;

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
            if(result & 0x80)
                result = (2 * result) ^ 0x1021;
            else
                result *= 2;
            if(*buf & bit)
                result ^= 0x1021;
        }
    }
    return result;
}

static void save_blob(const struct rknano_blob_t *b, void *buf, uint32_t size,
    char *name, int suffix, bool descramble)
{
    if(g_out_prefix == NULL || b->size == 0 || b->offset + b->size > size)
        return;
    char *path = malloc(strlen(g_out_prefix) + strlen(name) + 32);
    sprintf(path, "%s%s%d.bin", g_out_prefix, name, suffix);
    FILE *f = fopen(path, "wb");
    uint8_t *ptr = buf + b->offset;
    if(descramble)
    {
        ptr = malloc(b->size);
        encode_page(buf + b->offset, ptr, b->size);
    }
    
    if(f)
    {
        fwrite(ptr, b->size, 1, f);
        fclose(f);
    }

    if(descramble)
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
        save_blob(&hdr->stage[i], buf, size, "stage", i, false);
    }
    cprintf(BLUE, "Fonts\n");
    for(unsigned i = 0; i < hdr->nr_fonts; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->font[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->font[i], buf, size, "font", i, false);
    }
    cprintf(BLUE, "GBK\n");
    for(unsigned i = 0; i < hdr->nr_gbk; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->gbk[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->gbk[i], buf, size, "gbk", i, false);
    }
    cprintf(BLUE, "String Tables\n");
    for(unsigned i = 0; i < hdr->nr_strtbl; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->strtbl[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->strtbl[i], buf, size, "strtbl", i, false);
    }
    cprintf(BLUE, "Image Resources\n");
    for(unsigned i = 0; i < hdr->nr_imageres; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->imageres[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->imageres[i], buf, size, "imgres", i, false);
    }
    cprintf(BLUE, "Unknown\n");
    for(unsigned i = 0; i < hdr->nr_unk; i++)
    {
        cprintf(GREEN, "  %i: ", i);
        print_blob_interval(&hdr->unk[i]);
        cprintf(OFF, "\n");
        save_blob(&hdr->unk[i], buf, size, "unk", i, false);
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

struct rknano_stage_header_t
{
    uint32_t addr;
} __attribute__((packed));

struct rknano_stage_section_t
{
    uint32_t a;
    uint32_t code_pa;
    uint32_t code_va;
    uint32_t code_sz;
    uint32_t data_pa;
    uint32_t data_va;
    uint32_t data_sz;
    uint32_t bss_end_va;
} __attribute__((packed));

static int do_nanostage_image(uint8_t *buf, unsigned long size)
{
    if(size < sizeof(struct rknano_stage_section_t))
        return 1;
    struct rknano_stage_header_t *hdr = (void *)buf;

    cprintf(BLUE, "Header\n");
    cprintf(GREEN, "  Base Address: ");
    cprintf(YELLOW, "%#x\n", hdr->addr);
    
    struct rknano_stage_section_t *sec = (void *)(hdr + 1);
    void *end = buf + size;

    int i = 0;
    while((void *)sec < end && (sec->code_sz || sec->bss_end_va))
    {
        cprintf(BLUE, "Section %d\n", i);
        cprintf(GREEN, "  Something: ");
        cprintf(YELLOW, "%#x\n", sec->a);
        cprintf(GREEN, "  Code: ");
        cprintf(YELLOW, "%#x", sec->code_pa);
        cprintf(BLUE, " |--> ");
        cprintf(YELLOW, "%#x", sec->code_va);
        cprintf(RED, "-(code)-");
        cprintf(YELLOW, "%#x\n", sec->code_va + sec->code_sz);

        cprintf(GREEN, "  Data: ");
        cprintf(YELLOW, "%#x", sec->data_pa);
        cprintf(BLUE, " |--> ");
        cprintf(YELLOW, "%#x", sec->data_va);
        cprintf(RED, "-(data)-");
        cprintf(YELLOW, "%#x", sec->data_va + sec->data_sz);
        cprintf(RED, "-(bss)-");
        cprintf(YELLOW, "%#x\n", sec->bss_end_va);

        sec++;
        i++;
    }

    return 0;
}

#define MAGIC_BOOT      "BOOT"
#define MAGIC_BOOT_SIZE 4

struct rknano_boot_header_t
{
    char magic[MAGIC_BOOT_SIZE];
    uint16_t field_4;
    uint32_t field_6;
    uint32_t field_A;
    uint16_t field_E;
    uint8_t field_10[5];
    uint32_t field_15;
    uint8_t field_19;
    uint32_t field_1A;
    uint8_t field_1E[2];
    uint32_t field_20;
    uint8_t field_24[2];
    uint32_t field_26;
    uint8_t field_2A[10];
    uint32_t field_34;
} __attribute__((packed));

static int do_boot_image(uint8_t *buf, unsigned long size)
{
    if(sizeof(struct rknano_boot_header_t) != 0x38)
        printf("aie");
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
#define print(str, name) cprintf(GREEN, "  "str": ");cprintf(YELLOW, "%#x\n", (unsigned)hdr->name)
#define print_arr(str, name, sz) \
    cprintf(GREEN, "  "str":");for(int i = 0; i < sz; i++)cprintf(YELLOW, " %#x", (unsigned)hdr->name[i]);printf("\n")

    print("field_4", field_4);
    print("field_6", field_6);
    print("field_A", field_A);
    print("field_E", field_E);
    print_arr("field_10", field_10, 5);
    print("field_15", field_15);
    print("field_19", field_19);
    print("field_1A", field_1A);
    print_arr("field_1E", field_1E, 2);
    print("field_20", field_20);
    print_arr("field_24", field_24, 2);
    print("field_26", field_26);
    print_arr("field_2A", field_2A, 10);
    print("field_34", field_34);
    cprintf(GREEN, "Value: ");
    cprintf(YELLOW, "%#x\n", *(unsigned long *)((uint8_t *)hdr + hdr->field_34 - 10));

    return 0;
}

typedef struct rknano_blob_t rkfw_blob_t;

#define MAGIC_RKFW      "RKFW"
#define MAGIC_RKFW_SIZE 4

struct rkfw_header_t
{
    char magic[MAGIC_RKFW_SIZE];
    uint16_t hdr_size; // UNSURE
    uint32_t field_6;
    uint32_t field_A;
    uint16_t field_E;
    uint8_t field_10[5];
    uint32_t field_15;
    rkfw_blob_t loader;
    rkfw_blob_t update;
    uint8_t pad[60];
    uint8_t field_65;
} __attribute__((packed));

static int do_rkfw_image(uint8_t *buf, unsigned long size)
{
    if(sizeof(struct rkfw_header_t) != 0x66)
        printf("aie");
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

    cprintf(GREEN, "  Loader: ");
    print_blob_interval(&hdr->loader);
    cprintf(OFF, "\n");
    save_blob(&hdr->loader, buf, size, "loader", 0, false);

    cprintf(GREEN, "  Update: ");
    print_blob_interval(&hdr->update);
    cprintf(OFF, "\n");
    save_blob(&hdr->update, buf, size, "update", 0, false);

    print("hdr_size", hdr_size);
    print("field_6", field_6);
    print("field_A", field_A);
    print("field_E", field_E);
    print_arr("field_10", field_10, 5);
    print("field_15", field_15);
    print_arr("pad", pad, 60);
    print("field_65", field_65);

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

    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"rkfw", no_argument, 0, '9'},
            {"rknanofw", no_argument, 0, 'n'},
            {"rknanostage", no_argument, 0, 's'},
            {"rkboot", no_argument, 0, 'b'},
            {"no-color", no_argument, 0, 'c'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?d9nscbo:", long_options, NULL);
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
            default:
                abort();
        }
    }

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    if(!try_nanostage && !try_rkfw && !try_nanofw && !try_boot)
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
    cprintf(GREY, "No valid format found!\n");
    Lsuccess:
    free(buf);

    return 0;
}

