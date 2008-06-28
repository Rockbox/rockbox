/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Christian Hack
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
#include <stdlib.h>

extern void int2le(unsigned int val, unsigned char* addr);
extern unsigned int le2int(unsigned char* buf);

static FILE * openinfile( const char * filename )
{
    FILE * F = fopen( filename, "rb" );
    if( F == NULL )
    {
        fprintf( stderr, "Couldn't open input file %s\n", filename );
        perror( "Error was " );
        exit( -1 );
    };
    return F;
}

static FILE * openoutfile( const char * filename )
{
    FILE * F = fopen( filename, "wb" );
    if( F == NULL )
    {
        fprintf( stderr, "Couldn't open output file %s\n", filename );
        perror( "Error was " );
        exit( -1 );
    };
    return F;
}

int gigabeat_code(char *infile, char *outfile)
{
    FILE *in, *out;
    unsigned long size = 0;
    unsigned long bytes_read;
    unsigned char buf[4];
    unsigned long data;
    unsigned long key = 0x19751217;

    in = openinfile(infile);
    out = openoutfile(outfile);

    while (!feof(in)) {
        bytes_read = fread(buf, 1, 4, in);

        /* Read in little-endian */
        data = le2int(buf);
        
        data = data ^ key;

        key = key + (key << 1);
        key = key + 0x19751217;

        size += bytes_read;

        /* Write out little-endian */
        int2le(data, buf);

        fwrite(buf, 1, bytes_read, out);
    }

    fprintf(stderr, "File processed successfully\n" );

    fclose(in);
    fclose(out);
    return(0);
}
