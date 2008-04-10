/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Will Robertson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>

/* Entry point (and load address) for the main Rockbox bootloader */
#define BL_ENTRY_POINT 0x8a000000


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

static uint32_t calc_csum(const unsigned char* pb, int cb)
{
    uint32_t l = 0;
    while (cb--)
        l += *pb++;
    return l;
}

static void put_uint32le(uint32_t x, unsigned char* p)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

int gigabeat_s_code(char *infile, char *outfile)
{
    FILE *in, *out;
    unsigned int size;
    unsigned int newsize;
    unsigned char* buf;

    in = openinfile(infile);
    out = openoutfile(outfile);

    /* Step 1: Load the binary file into memory */
    fseek(in, 0, SEEK_END);
    size = ftell(in);

    /* 15 bytes for header, 16 for signature bypass, 
     * 12 for record header, 12 for footer */
    newsize = 15 + 16 + 12 + size + 12;
    buf = malloc(newsize);
    if(buf == NULL) {
        fprintf(stderr, "Not enough memory to perform the requested operation. Aborting.\n" );
        return 0;
    }
    fseek(in, 0, SEEK_SET);
    fread(buf + 43, size, 1, in);
    fclose(in);

    /* Step 2: Create the file header */
    sprintf((char *)buf, "B000FF\n");
    put_uint32le(0x88200000, buf+7);
    /* If the value below is too small, the update will attempt to flash.
     * Be careful when changing this (leaving it as is won't cause issues) */
    put_uint32le(0xCC0CD8, buf+11); 

    /* Step 3: Add the signature bypass record */
    put_uint32le(0x88065A10, buf+15);
    put_uint32le(4, buf+19);
    put_uint32le(0xE3A00001, buf+27);
    put_uint32le(calc_csum(buf+27,4), buf+23);

    /* Step 4: Create a record for the actual code */
    put_uint32le(BL_ENTRY_POINT, buf+31);
    put_uint32le(size, buf+35);
    put_uint32le(calc_csum(buf + 43, size), buf+39);

    /* Step 5: Write the footer */
    put_uint32le(0, buf+newsize-12);
    put_uint32le(BL_ENTRY_POINT, buf+newsize-8);
    put_uint32le(0, buf+newsize-4);

    /* Step 6: Write the resulting file */
    fwrite(buf, newsize, 1, out);
    fclose(out);
    
    fprintf(stderr, "File processed successfully\n" );

    return(0);
}
