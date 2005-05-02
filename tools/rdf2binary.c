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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

struct word
{
    char word[WORDLEN];
    long offset;
};

/* convert offsets here, not on device. */
long long_to_big_endian (void* value)
{
    unsigned char* bytes = (unsigned char*) value;
    return (long)bytes[0] | ((long)bytes[1] << 8) |
           ((long)bytes[2] << 16) | ((long)bytes[3] << 24);
}

int main()
{
    FILE *in;
    int idx_out, desc_out;
    struct word w;
    char buf[10000];
    long cur_offset = 0;

    in = fopen("dict.preparsed", "r");
    idx_out = open("dict.index", O_WRONLY | O_CREAT);
    desc_out = open("dict.desc", O_WRONLY | O_CREAT);

    if (in == NULL || idx_out < 0 || desc_out < 0)
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
        w.offset = long_to_big_endian(&cur_offset);
        write(idx_out, &w, sizeof(struct word));

        while (1)
        {
            int len = strlen(desc);
            cur_offset += len;
            write(desc_out, desc, len);

            desc = strtok(NULL, "\t");
            if (desc == NULL)
                break ;

            cur_offset++;
            write(desc_out, "\n", 1);

        }
    }

    return 0;
}

