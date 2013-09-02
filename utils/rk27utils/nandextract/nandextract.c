#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "libbch.h"

#define SECTOR_DATA_SIZE 512
#define SECTOR_META_SIZE 3
#define SECTOR_ECC_SIZE 13
#define SECTOR_SIZE (SECTOR_DATA_SIZE + SECTOR_META_SIZE + SECTOR_ECC_SIZE)

/* scramble mode */
enum {
      CONTINOUS_ENC, /* scramble whole block at once */
      PAGE_ENC       /* nand bootloader is scrambled in 0x200 chunks */
};

static uint8_t reverse_bits(uint8_t b)
{
	return (((b & 0x80) >> 7)|
		((b & 0x40) >> 5)|
		((b & 0x20) >> 3)|
		((b & 0x10) >> 1)|
		((b & 0x08) << 1)|
		((b & 0x04) << 3)|
		((b & 0x02) << 5)|
		((b & 0x01) << 7));
}

static int libbch_decode_sec(struct bch_control *bch, uint8_t *inbuf, uint8_t *outbuf)
{
	unsigned int errloc[8];
	static const uint8_t mask[13] = {
		0x4e, 0x8c, 0x9d, 0x52,
		0x2d, 0x6c, 0x7c, 0xcb,
		0xc3, 0x12, 0x14, 0x19,
		0x37,
	};
	
	int i, err_num = 0;
	
	/* ecc masking polynomial */
	for (i=0; i<SECTOR_ECC_SIZE; i++)
		inbuf[SECTOR_DATA_SIZE+SECTOR_META_SIZE+i] ^= mask[i];
		
	/* fix ordering of input bits */
	for (i = 0; i < SECTOR_SIZE; i++)
		inbuf[i] = reverse_bits(inbuf[i]);

	err_num = decode_bch(bch, inbuf,
	                     (SECTOR_SIZE - SECTOR_ECC_SIZE),
	                     &inbuf[SECTOR_SIZE - SECTOR_ECC_SIZE],
	                     NULL, NULL, errloc);
	
	/* apply fixups */
	for(i=0; i<err_num; i++)
		inbuf[errloc[i]/8] ^= 1 << (errloc[i] % 8);

	/* reverse bits back (data part only), remining bytes are scratched */		
	for (i = 0; i < SECTOR_DATA_SIZE; i++)
		outbuf[i] = reverse_bits(inbuf[i]);
		
	return err_num;
}

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

/* returns offset in bytes of the sector
 * NOTE: bootrom assumes 4 secs per page (regardles of actual pagesize)
 */
static int offset(int sec_num, int page_size, int rom)
{
    int sec_per_page, page_num, page_offset;

    if (rom)
        sec_per_page = 4;
    else
    	sec_per_page = page_size / SECTOR_SIZE;

    page_num = sec_num / sec_per_page;
    page_offset = sec_num % sec_per_page;

    printf("Sec per page: %d\n", sec_per_page);
    printf("Page num: %d\n", page_num);
    printf("Offset in page (sec): %d\n", page_offset);
    printf("Offset in file (bytes): %d (0x%0x)\n", (page_num * page_size) +
                                                   (page_offset * SECTOR_SIZE),
                                                   (page_num * page_size) +
                                                   (page_offset * SECTOR_SIZE));

    return ((page_num * page_size) + (page_offset * SECTOR_SIZE));
}

static int sector_read(FILE *fp, void *buff, int sec_num, int nand_page_size, struct bch_control *bch)
{
    int ret;
    int file_offset = offset(sec_num, nand_page_size, 1);
    uint8_t inbuf[SECTOR_SIZE];  
    uint8_t outbuf[SECTOR_SIZE];

    if (fp == NULL)
        return -1;

   /* seek to the begining of the data */
    fseek(fp, file_offset, SEEK_SET);

    /* read into the buffer */
    ret = fread(inbuf, 1, SECTOR_SIZE, fp);

    if (ret != SECTOR_SIZE)
    {
        return -2;
    }
        
    ret = libbch_decode_sec(bch, inbuf, outbuf);
    
    if (ret)
    {
		printf("LIBBCH Data %d error(s) in sector %d\n", ret, sec_num);
	}
		
    memcpy(buff, outbuf, SECTOR_DATA_SIZE);
    return ret;
}

int main (int argc, char **argv)
{
    FILE *ifp, *ofp;
    void *obuf;
    int i, size, sector, num_sectors, nand_page_size;
    char *infile, *outfile, *ptr;

	struct bch_control *bch;
	
    if (argc < 6)
    {
        printf("Usage: %s infile outfile start_sector num_sectors nand_page_size\n", argv[0]);
        return 0;
    }
    
    infile = argv[1];
    outfile = argv[2];
    sector = atoi(argv[3]);
    num_sectors = atoi(argv[4]);
    nand_page_size = atoi(argv[5]);

    size = SECTOR_DATA_SIZE * num_sectors;    
    obuf = malloc(size);
    
    if (obuf == NULL)
    {
		printf("Error allocating %d bytes of buffer\n", size);
		return -1;
    }
    
    ifp = fopen(infile, "rb");
    
    if (ifp == NULL)
    {
		printf("Cannot open %s file\n", infile);
		free(obuf);
		return -2;
    }
    
    ofp = fopen(outfile, "wb");    
    
    if (ifp == NULL)
    {
		printf("Cannot open %s file\n", outfile);
		fclose(ifp);
		free(obuf);
		return -3;
    }

    bch = init_bch(13, 8, 0x25af);
     
    ptr = (char *)obuf;
    for(i=0; i<num_sectors; i++)
    {
		sector_read(ifp, ptr, sector++, nand_page_size, bch);
		encode_page((uint8_t *)ptr, (uint8_t *)ptr, SECTOR_DATA_SIZE);
		ptr += SECTOR_DATA_SIZE;
    }

    fwrite(obuf, 1, size, ofp);

    fclose(ifp);
    fclose(ofp);
    free(obuf);
    
    free_bch(bch); 
    return 0;
}
