#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "panic.h"

#define BLOCK_SIZE 512

static FILE* file;

int ata_read_sectors(unsigned long start, int count, void* buf)
{
    if ( count > 1 )
        DEBUGF("[Reading %d blocks: 0x%lx to 0x%lx]\n",
               count, start, start+count-1); 
    else
        DEBUGF("[Reading block 0x%lx]\n", start); 

    if(fseek(file,start*BLOCK_SIZE,SEEK_SET)) {
        perror("fseek");
        return -1;
    }
    if(!fread(buf,BLOCK_SIZE,count,file)) {
        DEBUGF("ata_write_sectors(0x%x, 0x%x, 0x%x)\n", start, count, buf );
        perror("fread");
        panicf("Disk error\n");
    }
    return 0;
}

int ata_write_sectors(unsigned long start, int count, void* buf)
{
    if ( count > 1 )
        DEBUGF("[Writing %d blocks: 0x%lx to 0x%lx]\n",
               count, start, start+count-1); 
    else
        DEBUGF("[Writing block 0x%lx]\n", start); 

    if (start == 0)
        panicf("Writing on sector 0!\n");

    if(fseek(file,start*BLOCK_SIZE,SEEK_SET)) {
        perror("fseek");
        return -1;
    }
    if(!fwrite(buf,BLOCK_SIZE,count,file)) {
        DEBUGF("ata_write_sectors(0x%x, 0x%x, 0x%x)\n", start, count, buf );
        perror("fwrite");
        panicf("Disk error\n");
    }
    return 0;
}

int ata_init(char* filename)
{
    if (!filename)
        filename = "disk.img";
    /* check disk size */
    file=fopen(filename,"rb+");
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
