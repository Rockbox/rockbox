#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "panic.h"

#define BLOCK_SIZE 512

static FILE* file;

int ata_read_sectors(unsigned long start, unsigned char count, void* buf)
{
    int i;
    for (i=0; i<count; i++ )
        DEBUGF("[Reading block 0x%lx]\n",start+i); 

    if(fseek(file,start*BLOCK_SIZE,SEEK_SET)) {
        perror("fseek");
        return -1;
    }
    if(!fread(buf,BLOCK_SIZE,count,file)) {
        printf("Failed reading %d blocks starting at block 0x%lx\n",count,start); 
        perror("fread");
        panicf("Disk error\n");
        return -2;
    }
    return 0;
}

int ata_write_sectors(unsigned long start, unsigned char count, void* buf)
{
    int i;
    for (i=0; i<count; i++ )
        DEBUGF("[Writing block 0x%lx]\n",start+i); 

    if (start == 0)
        panicf("Writing on sector 0!\n");

    if(fseek(file,start*BLOCK_SIZE,SEEK_SET)) {
        perror("fseek");
        return -1;
        panicf("Disk error\n");
    }
    if(!fwrite(buf,BLOCK_SIZE,count,file)) {
        perror("fwrite");
        return -2;
        panicf("Disk error\n");
    }
    return 0;
}

int ata_init(char* filename)
{
    if (!filename)
        filename = "disk.img";
    /* check disk size */
    file=fopen(filename,"r+");
    if(!file) {
        fprintf(stderr, "read_disk() - Could not find \"%s\"\n",filename);
        return -1;
    }
    return 0;
}

void ata_exit(void)
{
    fclose(file);
}
