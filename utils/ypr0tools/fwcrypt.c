/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Lorenzo Miori
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <openssl/md5.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdint.h>
#include "common.h"

// TODO serious error handling

int main(int argc, char **argv)
{
    FILE* component_handle = NULL;
    FILE* output_file = NULL;
    FILE* rev_info_file = NULL;
    int error = 0;

    struct firmware_data fw;
    memset(&fw, 0, sizeof(fw));

    if (argc < 2)
    {
        printf("Crypts Samsung YP-R0/YP-R1 ROM file format\n"
               "Usage: fwcrypt <output ROM file path\n"
        );
        return 1;
    }

    // open the output file for write
    output_file = fopen(argv[1], "wb");
    if(output_file == NULL)
    {
        printf("Cannot open file for writing: %m\n");
        return SAMSUNG_WRITE_ERROR;
    }

    // write generic header
    rev_info_file = fopen("RevisionInfo.txt", "rb");
    if (rev_info_file != NULL)
    {
        for (int i = 0; i < GENERIC_HEADER_LINES; i++)
        {
            char header[MAX_HEADER_LEN];
            error += fgets(header, MAX_HEADER_LEN, rev_info_file) == NULL;
            error += fprintf(output_file, "%s", header) != (signed)strlen(header);
        }
        fclose(rev_info_file);
    }
    else
    {
        // write some generic information
        error += fprintf(output_file, YPR0_VERSION) != strlen(YPR0_VERSION);
        error += fprintf(output_file, YPR0_TARGET) != strlen(YPR0_TARGET);
        error += fprintf(output_file, YPR0_USER) != strlen(YPR0_USER);
        error += fprintf(output_file, YPR0_DIR) != strlen(YPR0_DIR);
        error += fprintf(output_file, YPR0_TIME) != strlen(YPR0_TIME);
    }

    if(error != 0)
    {
        printf("Cannot write generic header\n");
        return SAMSUNG_WRITE_ERROR;
    }

    for (int i = 0; i < YPR0_COMPONENTS_COUNT; i++)
    {
        component_handle = fopen(firmware_filenames[i], "rb");
        fw.component_size[i] = get_filesize(component_handle);
        fw.component_data[i] = malloc(fw.component_size[i] * sizeof(char));
        fread(fw.component_data[i], sizeof(char), fw.component_size[i], component_handle);
        fclose(component_handle);
        
        // compute checksum
        md5sum(fw.component_checksum[i], fw.component_data[i], fw.component_size[i]);
        printf("%s : size(%ld),checksum(%s)\n", firmware_components[i], fw.component_size[i], fw.component_checksum[i]);
        // write metadata header to file
        if (fprintf(output_file, "%s : size(%ld),checksum(%s)\n", firmware_components[i], fw.component_size[i], fw.component_checksum[i]) < 0)
        {
            printf("Error writing to output file.\n");
        }
    }

    // write final data to the firmware file
    for (int i = 0; i < YPR0_COMPONENTS_COUNT; i++)
    {
        /* the bootloader needs to be patched: add checksum of the components */
        if (strcmp("MBoot", firmware_components[i]) == 0)
        {
             int index=MBOOT_CHECKSUM_OFFSET;
             for (int z = 0; z < YPR0_COMPONENTS_COUNT; z++)
             {
                index += sprintf(fw.component_data[i] + index, "%ld:%s\n", fw.component_size[z], fw.component_checksum[z]);
             }
        }
        // crypt data
        cyclic_xor(fw.component_data[i], fw.component_size[i], g_yp_key, sizeof(g_yp_key));
        // write data
        size_t written = fwrite(fw.component_data[i], sizeof(char), fw.component_size[i], output_file);
        if (written != fw.component_size[i])
        {
            printf("%s: error writing data to file. Written %ld bytes\n", firmware_components[i], written);
        }
        // padding
        if (i < (YPR0_COMPONENTS_COUNT-1))
            fputs("\0\0\0\0", output_file);
    }

    // free the big amount of memory
    for (int i = 0; i < YPR0_COMPONENTS_COUNT; i++)
    {
        free(fw.component_data[i]);
    }

    fclose(output_file);
}