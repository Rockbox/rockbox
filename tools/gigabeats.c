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
};

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
};

unsigned long calc_csum(const unsigned char* pb, int cb)
{
    unsigned long l = 0;
    while (cb--)
        l += *pb++;
    return l;
}

int gigabeat_s_code(char *infile, char *outfile)
{
    FILE *in, *out;
    unsigned long size = 0;
    unsigned long data;
    int imagelength;

    in = openinfile(infile);
    out = openoutfile(outfile);

    fseek(in, 0, SEEK_END);
    size = ftell(in);
    fseek(in, 0, SEEK_SET);
    unsigned long *binptr = malloc(size);
    if(binptr == NULL) {
        fprintf(stderr, "Not enough memory to perform the requested operation. Aborting.\n" );
        return 0;
    }
    fread(binptr, size/4, 4, in);
    /* 15 bytes for header, three 12 byte headers, the data for the first three
     * records, 12 byte header for code, code and the 12 byte footer
     * However, the original nk.bin's length doesn't correspond with
     * the length of the file, so I don't know what's up...
     */

    unsigned long header[2];
    header[0] = 0x88200000;
    /* header[1] = 15 + 12 + 4 + 12 + 8 + 12 + 4 + 12 + size + 12; */
    header[1] = 0xCC0CD8; /* The bootloader checks this value and compares */
    fwrite("B000FF\n", 7, 1, out);
    fwrite(header, sizeof(header), 1, out);
    
    unsigned long record[4];
    unsigned long extra;

    /*First record*/
    record[0] = 0x88200000;
    record[1] = 4;
    record[2] = 0x1eb;
    record[3] = 0xEA0003FE;
    fwrite(record, sizeof(record), 1, out);

    /*Second record*/
    record[0] = 0x88200040;
    record[1] = 8;
    record[2] = 0x3e9;
    record[3] = 0x43454345;
    extra = 0x88EBF274;
    fwrite(record, sizeof(record), 1, out);
    fwrite(&extra, sizeof(extra), 1, out);

    /*Third record*/
    record[0] = 0x88200048;
    record[1] = 4;
    record[2] = 0x231;
    record[3] = 0x00CBF274;
    fwrite(record, sizeof(record), 1, out);

    /*Signature bypass record*/
    unsigned long magic = 0xE3A00001;
    record[0] = 0x88065A10;
    record[1] = 4;
    record[2] = calc_csum((unsigned char*)&magic,4);
    record[3] = magic;
    fwrite(record, sizeof(record), 1, out);

    /*The actual code*/
    header[0] = 0x88201000;
    header[1] = size;
    extra = calc_csum((unsigned char*)binptr, size);
    fwrite(header, sizeof(header), 1, out);
    fwrite(&extra, sizeof(extra), 1, out);
    fwrite(binptr, size, 1, out);

    /* Table of contents. It's a start, but it still won't boot.
     * Looks like it needs the file/module info as well... */
    binptr[0] = 0x02000000;
    binptr[1] = 0x02000000;
    binptr[2] = 0x88040000;
    binptr[3] = 0x88076338;
    binptr[4] = 0x1;
    binptr[5] = 0x88080000;
    binptr[6] = 0x8809C000;
    binptr[7] = 0x88100000;
    binptr[8] = 0x0;
    binptr[9] = 0x0;
    binptr[10] = 0x0;
    binptr[11] = 0x0;
    binptr[12] = 0x0;
    binptr[13] = 0x0;
    binptr[14] = 0x80808080;
    binptr[15] = 0x0;
    binptr[16] = 0x0;
    binptr[17] = 0x201C2;
    binptr[18] = 0x88050940;
    binptr[19] = 0x0;
    binptr[20] = 0x0;
    header[0] = 0x88EBF274;
    header[1] = 0x54;
    extra = calc_csum((unsigned char*)binptr, 84);
    fwrite(header, sizeof(header), 1, out);
    fwrite(&extra, sizeof(extra), 1, out);
    fwrite(binptr, 84, 1, out);

    /*The footer*/
    header[0] = 0;
    header[1] = 0x88201000;
    extra = 0;
    fwrite(header, sizeof(header), 1, out);
    fwrite(&extra, sizeof(extra), 1, out);
    
    fprintf(stderr, "File processed successfully\n" );

    fclose(in);
    fclose(out);
    return(0);
}
