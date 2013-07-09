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
    FILE* input_file = NULL;
    FILE* rev_info_file = NULL;
    int error = 0;

    struct firmware_data fw;
    memset(&fw, 0, sizeof(fw));

    if (argc < 2)
    {
        printf("Missing arguments: input filename");
        return 1;
    }

    // open the output file for write
    input_file = fopen(argv[1], "rb");
    if(input_file == NULL)
    {
        printf("Cannot open file for writing: %m\n");
        return 1;
    }

    // read some generic information
    rev_info_file = fopen("RevisionInfo.txt", "w");
    for (int i = 0; i < 5; i++)
    {
        char info[MAX_HEADER_LEN];
        error += fgets(info, MAX_HEADER_LEN, input_file) == NULL;
        printf("%s", info);
        if (rev_info_file != NULL)
            fprintf(rev_info_file, "%s", info);
    }
    if (rev_info_file != NULL)
        fclose(rev_info_file);

    if(error != 0)
    {
        printf("Cannot read generic header\n");
        return SAMSUNG_WRITE_ERROR;
    }

    // read metadata
    for (int i = 0; i < YPR0_COMPONENTS_COUNT; i++)
    {
        char metadata[MAX_HEADER_LEN];
        error += fgets(metadata, MAX_HEADER_LEN, input_file) == NULL;
        sscanf(metadata, "%*s : size(%ld),checksum(%s)", &fw.component_size[i], fw.component_checksum[i]);
        // strip last ")"
        fw.component_checksum[i][strlen(fw.component_checksum[i])-1] = '\0';
        printf("%ld -> %s\n", fw.component_size[i], fw.component_checksum[i]);
    }

    for (int i = 0; i < YPR0_COMPONENTS_COUNT; i++)
    {
        fw.component_data[i] = malloc(fw.component_size[i]);
        size_t bread = fread(fw.component_data[i], 1, fw.component_size[i], input_file);
        if (bread != fw.component_size[i])
            printf("Read size mismatch: read %ld, expected %ld", bread, fw.component_size[i]);

        // decrypt data
        cyclic_xor(fw.component_data[i], fw.component_size[i], g_yp_key, sizeof(g_yp_key));

        // unpatch bootloader
        if (strcmp("MBoot", firmware_components[i]) == 0)
        {
            memset(fw.component_data[i] + MBOOT_CHECKSUM_OFFSET, 0, MBOOT_CHECKSUM_LENGTH);
        }

        char md5sum_decrypted[MD5_DIGEST_LENGTH*2+1];

        md5sum(md5sum_decrypted, fw.component_data[i], fw.component_size[i]);

        if (strcmp(md5sum_decrypted, fw.component_checksum[i]) != 0)
            printf("%s: FAIL (md5sum doesn't match)\n", firmware_components[i]);

        component_handle = fopen(firmware_filenames[i], "wb");
        fwrite(fw.component_data[i], 1, fw.component_size[i], component_handle);
        fclose(component_handle);

    }

    // free the big amount of memory
    for (int i = 0; i < YPR0_COMPONENTS_COUNT; i++)
    {
        free(fw.component_data[i]);
    }

    fclose(input_file);
    
    return 0;
}