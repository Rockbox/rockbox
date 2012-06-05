#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flat.h"
#include "bflt.h"

int main(int argv, char **argc)
{
    int i=1;
    int fd;
    char *bflt_file;
    struct flat_hdr header;

    void *dram;
    void *iram;

    uint32_t iram_base = 0;
    uint32_t dram_base = 0;
    uint32_t iram_size = 0;
    uint32_t dram_size = 0x100000; /* 1MB */

    while (i<argv)
    {
        if (strcmp(argc[i],"-dram") == 0)
        {
            i++;
            if (i != argv)
                dram_base = strtoul(argc[i++],NULL,0);
        }
        else if (strcmp(argc[i], "-iram") == 0)
        {
            i++;
            if (i != argv)
                iram_base = strtoul(argc[i++],NULL,0);
        }
        else if (strcmp(argc[i], "-dram_size") == 0)
        {
            i++;
            if (i != argv)
                dram_size = strtoul(argc[i++],NULL,0);
        }
        else if (strcmp(argc[i], "-iram_size") == 0)
        {
            i++;
            if (i != argv)
                iram_size = strtoul(argc[i++],NULL,0);
        }
        else if (strcmp(argc[i], "-bflt") == 0)
        {
            i++;
            if (i != argv)
                bflt_file = argc[i++];
        }
        else
        {
            fprintf(stderr, "Unknown argument\n");
            return 1;
        }                
    }

    /* open bflt file */
    fd = open(bflt_file, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Can't open %s file\n", bflt_file);
        return 2;
    }

    if (read_header(fd, &header) != 0)
    {
        fprintf(stderr, "Can't read bflt header\n");
        close(fd);
        return 3;
    }

    if (check_header(&header) != 0)
    {
        fprintf(stderr, "Bad bflt header\n");
        close(fd);
        return 4;
    }

    /* create virtual memories */
    dram = malloc(dram_size);
    if (dram == NULL)
    {
        fprintf(stderr, "Can't allocate 0x%0x bytes of dram mem\n", dram_size);
        close(fd);
        return 5;
    }

    memset(dram, 0, dram_size);

    if (iram_size)
    {
        iram = malloc(iram_size);
        if (iram == NULL)
        {
            fprintf(stderr, "Cant allocate 0x%x bytes of iram mem\n", iram_size);
            close(fd);
            free(dram);
            return 6;
        }

        memset(iram, 0, iram_size);
    }

    if (copy_segments(fd, &header, dram, dram_size, iram, iram_size))
    {
        fprintf(stderr, "Error copying segments\n");
        close(fd);
        free(dram);
        if (iram_size)
            free(iram);

        return 7;
    }

    if (process_relocs(fd, &header, dram, dram_base, iram, iram_base) != 0)
    {
        fprintf(stderr, "Error processing relocations\n");
        free(dram);
        if (iram_size)
            free(iram);

        return 8;
    }

    close(fd);

    fd = open("dram.bin", O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
    if (fd < 0)
    {
        fprintf(stderr, "Can't open output file\n");
        free(dram);
        if (iram_size)
            free(iram);

        return 9;
    }

    if (write(fd, dram, dram_size) != dram_size)
    {
        fprintf(stderr, "Error writing dram.bin output file\n");
        close(fd);
        free(dram);
        if (iram_size)
            free(iram);

        return 10;
    }

    close(fd);

    if (iram_size)
    {
        fd = open("iram.bin", O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
        if (fd < 0)
        {
            fprintf(stderr, "Can't open output file\n");
            free(dram);
            free(iram);

            return 11;
        }

        if (write(fd, iram, iram_size) != iram_size)
        {
            fprintf(stderr, "Error writing iram.bin output file\n");
            close(fd);
            free(dram);
            free(iram);

            return 12;
        }
    }    
        close(fd);
        free(dram);
        if (iram_size)
            free(iram);

    return 0;
}
