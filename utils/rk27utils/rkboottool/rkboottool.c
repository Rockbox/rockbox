#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "v0.3"

/* time field stucture */
struct rktime_t
{
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
};

/* Rock27Boot.bin header structure */
struct rkboot_info_t
{
    char     sign[32];
    uint8_t  check_values[16];
    struct   rktime_t time;
    uint32_t ui_master_version;
    uint32_t ui_slave_version;
    uint32_t s1_offset;
    int32_t  s1_len;
    uint32_t s2_offset;
    int32_t  s2_len;
    uint32_t s3_offset;
    int32_t  s3_len;
    uint32_t s4_offset;
    int32_t  s4_len;
    uint32_t version_flag;
};

/* actions */
enum {
      NONE = 0,
      INFO = 1,
      EXTRACT = 2,
      SCRAMBLE = 4
};

/* scramble mode */
enum {
      CONTINOUS_ENC, /* scramble whole block at once */
      PAGE_ENC       /* nand bootloader is scrambled in 0x200 chunks */
};

/* scrambling/descrambling reverse engineered by AleMaxx */
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

static void *binary_extract(FILE *fp, uint32_t offset, uint32_t len, int descramble, int encode_mode)
{
    void *buff, *buff_ptr;
    uint32_t ret;

    if ((fp == NULL) || len == 0)
        return NULL;

        /* allocate buff */
        if ((buff = malloc(len)) == NULL)
            return NULL;

        /* seek to the begining of the data */
        fseek(fp, offset, SEEK_SET);

        /* read into the buffer */
        ret = fread(buff, 1, len, fp);

        if (ret != len)
        {
            free(buff);
            return NULL;
        }

        /* descramble */
        if ( descramble )
        {
            buff_ptr = buff;
            if (encode_mode == PAGE_ENC)
            {
                while (len >= 0x200)
                {
                    encode_page((uint8_t *)buff_ptr,
                                (uint8_t *)buff_ptr,
                                0x200);

                    buff_ptr += 0x200;
                    len -= 0x200;
                }
            }
            encode_page((uint8_t *)buff_ptr, (uint8_t *)buff_ptr, len);
        }

    return buff;
}

static void usage(void)
{
    printf("Usage: rkboottool [options] Rock27Boot.bin\n");
    printf("-h|--help         This help message\n");
    printf("-e|--extract      Extract binary images from Rock27Boot.bin file\n");
    printf("-d|--descramble   Descramble extracted binary images\n");
    printf("-i|--info         Print info about Rock27Boot.bin file\n");
    printf("\n");
    printf("Usually you would like to use -d -e together to obtain raw binary\n");
    printf("(out files rkboot_s1.bin, rkboot_s2.bin, rkboot_s3.bin, rkboot_s4.bin)\n");
}

int main (int argc, char **argv)
{
    struct rkboot_info_t rkboot_info;
    FILE *fp_in, *fp_out;
    int32_t i = 0, action = NONE;
    int32_t ret;
    void *buff;
    char *in_filename = NULL;

    if ( argc < 2 )
    {
        usage();
        return -1;
    }

    /* print banner */
    fprintf(stderr,"rkboottool " VERSION "\n");
    fprintf(stderr,"(C) Marcin Bukat 2011\n");
    fprintf(stderr,"This is free software; see the source for copying conditions.  There is NO\n");
    fprintf(stderr,"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

    /* arguments handling */
    while (i < argc)
    {
        if ((strcmp(argv[i],"-i")==0) || (strcmp(argv[i],"--info")==0))
        {
            action |= INFO;
        }
        else if ((strcmp(argv[i],"-e")==0) || (strcmp(argv[i],"--extract")==0))
        {
            action |= EXTRACT;
        }
        else if ((strcmp(argv[i],"-d")==0) || (strcmp(argv[i],"--descramble")==0))
        {
            action |= SCRAMBLE;
        }
        else if ((strcmp(argv[i],"-h")==0) || (strcmp(argv[i],"--help")==0))
        {
            usage();
            return 0;
        }
        else if ( argv[i][0] != '-' )
        {
            /* file argument */
            in_filename = argv[i];
        }
        i++;
    }

    if ( (fp_in = fopen(in_filename, "rb")) == NULL )
    {
        fprintf(stderr, "error: can't open %s file for reading\n", in_filename);
        return -1;
    }

    ret = fread(&rkboot_info, 1, sizeof(rkboot_info), fp_in);

    if (ret != sizeof(rkboot_info))
    {
        fclose(fp_in);
        fprintf(stderr, "error: can't read %s file header\n", in_filename);
        fprintf(stderr, "read %d, expected %lu\n",
			ret, (unsigned long)sizeof(rkboot_info));
        return -2;
    }

    if (action & INFO)
    {
        printf("file: %s\n", in_filename);
        printf("signature: %s\n", rkboot_info.sign);
        printf("check bytes: ");
        for (i = 0; i < 16; i++)
            printf("0x%0x ", rkboot_info.check_values[i]);

        printf("\n");
        printf("timestamp %d.%d.%d %d:%d:%d\n", rkboot_info.time.day,
                                                rkboot_info.time.month,
                                                rkboot_info.time.year,
                                                rkboot_info.time.hour,
                                                rkboot_info.time.minute,
                                                rkboot_info.time.second);
        printf("UI master version: 0x%0x\n", rkboot_info.ui_master_version);
        printf("UI slave version: 0x%0x\n", rkboot_info.ui_slave_version);
        printf("s1 data offset: 0x%0x\n", rkboot_info.s1_offset);
        printf("s1 data len: 0x%0x\n", rkboot_info.s1_len);
        printf("s2 offset: 0x%0x\n", rkboot_info.s2_offset);
        printf("s2 len: 0x%0x\n", rkboot_info.s2_len);
        printf("s3 offset: 0x%0x\n", rkboot_info.s3_offset);
        printf("s3 len: 0x%0x\n", rkboot_info.s3_len);
        printf("s4 offset: 0x%0x\n", rkboot_info.s4_offset);
        printf("s4 len: 0x%0x\n", rkboot_info.s4_len);
        printf("UI version flag: 0x%0x\n", rkboot_info.version_flag);
    }

    if (action & EXTRACT)
    {
        /* first stage */
        buff = binary_extract(fp_in, rkboot_info.s1_offset,
                                     rkboot_info.s1_len,
                                     action & SCRAMBLE,
                                     CONTINOUS_ENC);

        if ( buff == NULL )
        {
            fclose(fp_in);
            fprintf(stderr, "error: can't extract image\n");
            return -2;
        }

        /* output */
        if ((fp_out = fopen("rkboot_s1.bin", "wb")) == NULL)
        {
            free(buff);
            fclose(fp_in);
            fprintf(stderr, "[error]: can't open rkboot_s1.bin for writing\n");
            return -3;
        }

        fwrite(buff, 1, rkboot_info.s1_len, fp_out);
 
        fprintf(stderr, "[info]: extracted rkboot_s1.bin file\n");
        free(buff);
        fclose(fp_out);

        /* second stage */
        buff = binary_extract(fp_in, rkboot_info.s2_offset,
                                     rkboot_info.s2_len,
                                     action & SCRAMBLE,
                                     CONTINOUS_ENC);

        if ( buff == NULL )
        {
            fclose(fp_in);
            fprintf(stderr, "error: can't extract image\n");
            return -2;
        }

        if ((fp_out = fopen("rkboot_s2.bin", "wb")) == NULL)
        {
            free(buff);
            fclose(fp_in);
            fprintf(stderr, "[error]: can't open rkboot_s2.bin for writing\n");
            return -4;
        }

        fwrite(buff, 1, rkboot_info.s2_len, fp_out);

        fprintf(stderr, "[info]: extracted rkboot_s2.bin file\n");
        free(buff);
        fclose(fp_out);

        /* third stage */
        buff = binary_extract(fp_in, rkboot_info.s3_offset,
                                     rkboot_info.s3_len,
                                     action & SCRAMBLE,
                                     PAGE_ENC);
        if ( buff == NULL )
        {
            fclose(fp_in);
            fprintf(stderr, "[error]: can't extract image.\n");
            return -2;
        }

        if ((fp_out = fopen("rkboot_s3.bin", "wb")) == NULL)
        {
            free(buff);
            fclose(fp_in);
            fprintf(stderr, "[error]: can't open rkboot_s3.bin for writing\n");
            return -4;
        }

        fwrite(buff, 1, rkboot_info.s3_len, fp_out);

        fprintf(stderr, "[info]: extracted rkboot_s3.bin file\n");
        free(buff);
        fclose(fp_out);

        /* forth stage */
        buff = binary_extract(fp_in, rkboot_info.s4_offset,
                                     rkboot_info.s4_len,
                                     action & SCRAMBLE,
                                     CONTINOUS_ENC);
        if ( buff == NULL )
        {
            fclose(fp_in);
            fprintf(stderr, "[error]: can't extract image\n");
            return -2;
        }

        if ((fp_out = fopen("rkboot_s4.bin", "wb")) == NULL)
        {
            free(buff);
            fclose(fp_in);
            fprintf(stderr, "[error]: can't open rkboot_s4.bin for writing\n");
            return -4;
        }

        fwrite(buff, 1, rkboot_info.s4_len, fp_out);

        fprintf(stderr, "[info]: extracted rkboot_s4.bin file\n");
        free(buff);
        fclose(fp_out);
    }

    fclose(fp_in);
    return 0;
}

