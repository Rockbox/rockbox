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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

static char* output_dir = NULL;
static FILE* input_file = NULL;
static struct firmware_data fw;

static void cleanup(void)
{
    for (int i = 0; i < YPR0_COMPONENTS_COUNT; i++)
    {
        free(fw.component_data[i]);
    }
}

static void die(int error)
{
    if (input_file != NULL)
        fclose(input_file);
    free(output_dir);
    cleanup();
    exit(error);
}

int main(int argc, char **argv)
{
    FILE* component_handle = NULL;
    FILE* rev_info_file = NULL;
    char* tmp_path = malloc(MAX_PATH);
    int error = 0;
    bool md5sum_error = false;

    memset(&fw, 0, sizeof(fw));

    if (argc < 2)
    {
        printf("Decrypts Samsung YP-R0/YP-R1 ROM file format\n"
               "Usage: fwdecrypt <ROM file path\n"
        );
        return 1;
    }

    output_dir = malloc(MAX_PATH);
    output_dir[0] = '\0';
    if (argc > 2)
    {
        strcpy(output_dir, argv[2]);
    }

    /* open the output file for write */
    input_file = fopen(argv[1], "rb");
    if (input_file == NULL)
    {
        fprintf(stderr, "Cannot open file for reading: %m\n");
        die(SAMSUNG_READ_ERROR);
    }

    /* read some generic information */
    join_path(tmp_path, output_dir, "RevisionInfo.txt");
    rev_info_file = fopen(tmp_path, "w");
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

    if (error != 0)
    {
        fprintf(stderr, "Cannot write generic header\n");
        die(SAMSUNG_WRITE_ERROR);
    }

    /* read metadata */
    for (int i = 0; i < YPR0_COMPONENTS_COUNT; i++)
    {
        char metadata[MAX_HEADER_LEN];
        error += fgets(metadata, MAX_HEADER_LEN, input_file) == NULL;
        error += sscanf(metadata, "%*s : size(%ld),checksum(%s)",
                        &fw.component_size[i], fw.component_checksum[i]) != 2;
        /* strip last ")" */
        fw.component_checksum[i][strlen(fw.component_checksum[i])-1] = '\0';
        printf("%s: %ld bytes -- MD5 %s\n", firmware_components[i],
               fw.component_size[i], fw.component_checksum[i]);
    }

    /* We start from the end because ROM header could have a different
     * line count or extra new-lines (noticed in some hacked ROMs)
     */
    size_t current_pos = get_filesize(input_file);
    for (int i = YPR0_COMPONENTS_COUNT-1; i >= 0; i--)
    {

        fw.component_data[i] = malloc(fw.component_size[i]);
        current_pos -= fw.component_size[i];
        fseek(input_file, current_pos, SEEK_SET);
        size_t bread = fread(fw.component_data[i], 1, fw.component_size[i], input_file);
        if (bread != fw.component_size[i])
            fprintf(stderr, "%s: Read size mismatch: read %ld bytes, expected %ld bytes\n",
                   firmware_components[i], bread, fw.component_size[i]);

        /* decrypt data */
        cyclic_xor(fw.component_data[i], fw.component_size[i], g_yp_key, sizeof(g_yp_key));

        /* unpatch bootloader */
        if (strcmp("MBoot", firmware_components[i]) == 0)
        {
            memset(fw.component_data[i] + MBOOT_CHECKSUM_OFFSET, 0, MBOOT_CHECKSUM_LENGTH);
        }

        char md5sum_decrypted[MD5_DIGEST_LENGTH*2+1];

        md5sum(md5sum_decrypted, fw.component_data[i], fw.component_size[i]);

        if (strcmp(md5sum_decrypted, fw.component_checksum[i]) != 0)
        {
            printf("%s: FAIL (md5sum doesn't match)\n", firmware_components[i]);
            md5sum_error = true;
        }

        join_path(tmp_path, output_dir, firmware_filenames[i]);
        component_handle = fopen(tmp_path, "wb");

        if (component_handle == NULL)
        {
            fprintf(stderr, "Error opening file for writing. Is the directory valid and writeable?\n");
            die(SAMSUNG_WRITE_ERROR);
        }

        fwrite(fw.component_data[i], 1, fw.component_size[i], component_handle);
        fclose(component_handle);

    }

    if (md5sum_error)
        die(SAMSUNG_MD5_ERROR);
    die(SAMSUNG_SUCCESS);
}
