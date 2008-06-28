/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
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

/*
This tool converts the rdf file to the binary data used in the dict plugin.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

/* maximum word lenght, has to be the same in dict.c */
#define WORDLEN 32

/* struckt packing */
#ifdef __GNUC__
#define STRUCT_PACKED __attribute__((packed))
#else
#define STRUCT_PACKED
#pragma pack (push, 2)
#endif


struct word
{
    char word[WORDLEN];
    long offset;
} STRUCT_PACKED;

/* convert offsets here, not on device. */
long reverse (long N) {
    unsigned char B[4];
    B[0] = (N & 0x000000FF) >> 0;
    B[1] = (N & 0x0000FF00) >> 8;
    B[2] = (N & 0x00FF0000) >> 16;
    B[3] = (N & 0xFF000000) >> 24;
    return ((B[0] << 24) | (B[1] << 16) | (B[2] << 8) | (B[3] << 0));
}


int main()
{
    FILE *in, *idx_out, *desc_out;
    struct word w;
    char buf[10000];
    long cur_offset = 0;

    in = fopen("dict.preparsed", "r");
    idx_out = fopen("dict.index", "wb");
    desc_out = fopen("dict.desc", "wb");

    if (in == NULL || idx_out == NULL || desc_out == NULL)
    {
        fprintf(stderr, "Error: Some files couldn't be opened\n");
        return 1;
    }

    while (fgets(buf, sizeof buf, in) != NULL)
    {
        /* It is safe to use strtok here */
        const char *word = strtok(buf, "\t");
        const char *desc = strtok(NULL, "\t");

        if (word == NULL || desc == NULL)
        {
            fprintf(stderr, "Parse error!\n");
            fprintf(stderr, "word: %s\ndesc: %s\n", word, desc);

            return 2;
        }

        /* We will null-terminate the words */
        strncpy(w.word, word, WORDLEN - 1);
        w.offset = reverse(cur_offset);
        fwrite(&w, sizeof(struct word), 1, idx_out);

        while (1)
        {
            int len = strlen(desc);
            cur_offset += len;
            fwrite(desc, len, 1, desc_out);

            desc = strtok(NULL, "\t");
            if (desc == NULL)
                break ;

            cur_offset++;
            fwrite("\n", 1, 1, desc_out);

        }
    }

    return 0;
}

