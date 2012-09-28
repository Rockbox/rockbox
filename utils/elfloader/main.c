#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elfload.h"

static void help(void)
{
    printf("This utility will process supplied ELF file\n"
           "and create runtime binary image as if it was\n"
           "loaded under dram and iram addresses\n"
           "Relocations are resolved when needed\n\n"
           "The utility will output dram.bin (and iram.bin if iram\n"
           "is referenced in ELF\n"
           "\nValid options:\n"
           "-dram        - vma of dram (default 0x00000000)\n"
           "-dram_size   - size of dram memory (default 0x10000)\n"
           "-iram        - vma of iram (default 0x80000000)\n"
           "-iram_size   - size of iram memory (default 0x1000)\n"
           "-uncache_bae - uncached mem alias address (default 0x00000000)\n"
           "-elf         - elf file to process\n");
}

int main(int argv, char **argc)
{
    int i=1;
    int fd;
    char *elf_file;
    struct mem_info_t mem = {.dram = (void *)0,
                             .dram_img = NULL,  /* host side ptr */
                             .dram_size = 0x10000,
                             .dram_runtime_used = 0,
                             .dram_load_used = 0,
                             .iram = (void *)0x80000000,
                             .iram_img = NULL,  /* host side ptr */
                             .iram_size = 0x1000,
                             .iram_runtime_used = 0,
                             .uncache_base = 0};

    if (argv < 2)
    {
        help();
        return 0;
    }

    while (i<argv)
    {
        if (strcmp(argc[i],"-dram") == 0)
        {
            i++;
            if (i != argv)
                mem.dram = (void *)strtoul(argc[i++],NULL,0);
        }
        else if (strcmp(argc[i], "-iram") == 0)
        {
            i++;
            if (i != argv)
                mem.iram = (void *)strtoul(argc[i++],NULL,0);
        }
        else if (strcmp(argc[i], "-dram_size") == 0)
        {
            i++;
            if (i != argv)
                mem.dram_size = strtoul(argc[i++],NULL,0);
        }
        else if (strcmp(argc[i], "-iram_size") == 0)
        {
            i++;
            if (i != argv)
                mem.iram_size = strtoul(argc[i++],NULL,0);
        }
        else if (strcmp(argc[i], "-elf") == 0)
        {
            i++;
            if (i != argv)
                elf_file = argc[i++];
        }
        else if (strcmp(argc[i], "-uncache_base") == 0)
        {
            i++;
            if (i != argv)
                mem.uncache_base = strtoul(argc[i++],NULL,0);
        }
        else
        {
            help();
            return 1;
        }                
    }

    mem.dram_img = malloc(mem.dram_size);
    mem.iram_img = malloc(mem.iram_size);

    printf("dram addr: 0x%08x size: 0x%x\n", (uintptr_t)mem.dram, mem.dram_size);
    printf("iram addr: 0x%08x size: 0x%x\n", (uintptr_t)mem.iram, mem.iram_size);
    printf("uncache base: 0x%08x\n", mem.uncache_base);

    elf_open(elf_file, &mem);

    fd = open("dram.bin", O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);

    if (fd < 0)
    {
        fprintf(stderr, "Can't open output file dram.bin for writing\n");
        goto free;
    }

    if (write(fd, mem.dram_img, mem.dram_runtime_used) != mem.dram_runtime_used)
    {
        fprintf(stderr, "Error writing dram.bin output file\n");
    }

    close(fd);

    if (mem.iram_runtime_used)
    {
        fd = open("iram.bin", O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
        if (fd < 0)
        {
            fprintf(stderr, "Can't open output file iram.bin for writing\n");
            goto free;
        }

        if (write(fd, mem.iram_img, mem.iram_runtime_used) != mem.iram_runtime_used)
        {
            fprintf(stderr, "Error writing iram.bin output file\n");
        }
    }

    close(fd);

free:
    if (mem.dram_img)
        free(mem.dram_img);

    if (mem.iram_img)
        free(mem.iram_img);

return 0;
}
