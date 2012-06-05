#define _BSD_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <endian.h>
#include <string.h>
#include "flat.h"

#define DEBUGF(x) fprintf(stderr, x)

static inline void endian_fix32(uint32_t * tofix, size_t count)
{
    /* bFLT is always big endian */
    size_t i;
    for (i=0; i<count; i++)
        tofix[i] = be32toh(tofix[i]);
}

int read_header(int fd, struct flat_hdr * header)
{
    lseek(fd, 0, SEEK_SET);
    size_t bytes_read = read(fd, header, sizeof(struct flat_hdr));
    endian_fix32(&header->rev, ( &header->build_date - &header->rev ) + 1);

    if (bytes_read == sizeof(struct flat_hdr))
    {
        return 0;
    }
    else
    {
        DEBUGF("Error reading header");
        return 1;
    }
}

int check_header(struct flat_hdr * header) {
    if (memcmp(header->magic, "bFLT", 4) != 0)
    {
        DEBUGF("Magic number does not match");
        return 1;
    }

    if (header->rev != FLAT_VERSION)
    {
        DEBUGF("Version number does not match");
        return 2;
    }

    /* check for unsupported flags */
    if (header->flags & (FLAT_FLAG_GZIP | FLAT_FLAG_GZDATA))
    {
        DEBUGF("Unsupported flags detected - GZip'd data is not supported");
        return 3;
    }

    return 0;
}

int copy_segments(int fd, struct flat_hdr * header, void * dram, ssize_t dram_size, void * iram, ssize_t iram_size)
{
    /* each segment follows on one after the other */
    /* [   .text   ][.icode]     [   .data   ]   [.idata][   .bss   ]        [.ibss]         */
    /* ^entry       ^icode_start ^data_start ^data_end  ^.idata_end ^bss_end       ^ibss_end */

    ssize_t text_size = header->icode_start - header->entry;
    ssize_t icode_size = header->data_start - header->icode_start;
    ssize_t data_size = header->data_end - header->data_start;
    ssize_t idata_size = header->idata_end - header->data_end;
    ssize_t bss_size = header->bss_end - header->idata_end;
    ssize_t ibss_size = header->ibss_end - header->bss_end;

    ssize_t dram_blob_size = text_size + data_size + bss_size;
    ssize_t iram_blob_size = icode_size + idata_size + ibss_size;

    if ((dram_blob_size > dram_size) || (iram_blob_size > iram_size))
    {
        DEBUGF("Not enough memory to run binary");
        return 1;
    }

    /* skip bFLT header */
    lseek(fd, header->entry, SEEK_SET);
    /* now copy segments one by one */
    if (read(fd, dram, text_size) != text_size)
    {
        DEBUGF("Error reading .text segment");
        return 2;
    }

    if (read(fd, iram, icode_size) != icode_size)
    {
        DEBUGF("Error reading .icode segment");
        return 3;
    }

    if (read(fd, dram+text_size, data_size) != data_size)
    {
        DEBUGF("Error reading .data segment");
        return 4;
    }

    if (read(fd, iram+icode_size, idata_size) != idata_size)
    {
        DEBUGF("Error reading .idata segment");
        return 5;
    }

    return 0;
}

int process_relocs(int fd, struct flat_hdr * header, void * dram, uint32_t dram_base, void * iram, uint32_t iram_base)
{
    uint32_t *fixme;
    uint32_t reloc;
    size_t i;

    ssize_t text_size = header->icode_start - header->entry;
    ssize_t icode_size = header->data_start - header->icode_start;
    ssize_t data_size = header->data_end - header->data_start;
    /* ssize_t idata_size = header->idata_end - header->data_end; */

    if (!header->reloc_count)
    {
        DEBUGF("No relocation needed");
        return 0;
    }

    lseek(fd, header->reloc_start, SEEK_SET);

    for (i=0; i<header->reloc_count; i++)
    {
        read(fd, &reloc, sizeof(reloc));
        reloc = be32toh(reloc);

        if (reloc < header->icode_start)
        {
            /* .text */
            fixme = (uint32_t *)((uintptr_t)dram + reloc);
            *fixme = be32toh(*fixme) + dram_base;
        }
        else if (reloc < header->data_start)
        {
            /* .icode */
            fixme = (uint32_t *)((uintptr_t)iram + reloc - text_size);
            *fixme = be32toh(*fixme) + (iram_base - text_size);
        }
        else if (reloc < header->data_end)
        {
            /* .data */
            fixme = (uint32_t *)((uintptr_t)dram + reloc - icode_size);
            *fixme = be32toh(*fixme) + (iram_base - icode_size);
        }
        else
        {
            /* .idata */
            fixme = (uint32_t *)((uintptr_t)iram + reloc - text_size - data_size);
            *fixme = be32toh(*fixme) + (iram_base - text_size - data_size);
        }
    }

    return 0;
}

