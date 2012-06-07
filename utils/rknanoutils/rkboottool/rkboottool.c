#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

#define cprintf(col, ...) do {printf("%s", col); printf(__VA_ARGS__); }while(0)

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

char *prefix = NULL;

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
    if(prefix == NULL || b->size == 0 || b->offset + b->size > size)
        return;
    char *path = malloc(strlen(prefix) + strlen(name) + 32);
    sprintf(path, "%s%s%d.bin", prefix, name, suffix);
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

static int do_image(int argc, char **argv, uint8_t *buf, unsigned long size)
{
    (void) argc;
    (void) argv;
    
    if(size < sizeof(struct rknano_header_t))
        return 1;
    struct rknano_header_t *hdr = (void *)buf;
    if(size < hdr->size)
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

static void usage(void)
{
    printf("Usage: rkboottool [options] rknanoboot.bin out_prefix\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if(argc < 3)
        usage();
    prefix = argv[argc - 1];
    FILE *fin = fopen(argv[argc - 2], "r");
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
    
    return do_image(argc - 1, argv, buf, size);
}

