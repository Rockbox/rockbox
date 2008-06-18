/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Dave Hooper
 *
 * This particular source code file is licensed under the X11 license. See the
 * bottom of the COPYING file for details on this license.
 *
 * Original code from http://www.beermex.com/@spc/ihpfirm.src.zip
 * Details at http://www.rockbox.org/twiki/bin/view/Main/IriverToolsGuide
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iriver.h"

const unsigned char munge[] = {
  0x7a, 0x36, 0xc4, 0x43, 0x49, 0x6b, 0x35, 0x4e, 0xa3, 0x46, 0x25, 0x84,
  0x4d, 0x73, 0x74, 0x61
};

const unsigned char header_modify[] = "* IHPFIRM-DECODED ";

const char * const models[] = { "iHP-100", "iHP-120/iHP-140", "H300 series",
                                NULL };

/* aligns with models array;  expected min firmware size */
const unsigned int firmware_minsize[] = { 0x100000, 0x100000, 0x200000 };
/* aligns with models array;  expected max firmware size */
const unsigned int firmware_maxsize[] = { 0x200000, 0x200000, 0x400000 };

const unsigned char header[][16] = {
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0x20, 0x03, 0x08, 0x27, 0x24, 0x00, 0x02, 0x30, 0x19, 0x17, 0x65, 0x73,
    0x85, 0x32, 0x83, 0x22 },
  { 0x20, 0x04, 0x03, 0x27, 0x20, 0x50, 0x01, 0x70, 0x80, 0x30, 0x80, 0x06,
    0x30, 0x19, 0x17, 0x65 }
};

static int testheader( const unsigned char * const data )
{
    const unsigned char * const d = data+16;
    const char * const * m = models;
    int ind = 0;
    while( *m )
    {
        if( memcmp( header[ ind ], d, 16 ) == 0 )
            return ind;
        ind++;
        m++;
    };
    return -1;
}

static void modifyheader( unsigned char * data )
{
    const unsigned char * h = header_modify;
    int i;
    for( i=0; i<512; i++ )
    {
        if( *h == '\0' )
            h = header_modify;
        *data++ ^= *h++;
    };
}

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

int iriver_decode(const char *infile_name, const char *outfile_name, BOOL modify,
                  enum striptype stripmode )
{
    FILE * infile = NULL;
    FILE * outfile = NULL;
    int i = -1;
    unsigned char headerdata[512];
    unsigned long dwLength1, dwLength2, dwLength3, fp = 0;
    unsigned char blockdata[16+16];
    unsigned char out[16];
    unsigned char newmunge;
    signed long lenread;
    int s = 0;
    unsigned char * pChecksums, * ppChecksums = 0;
    unsigned char ck;

    infile = openinfile(infile_name);
    outfile = openoutfile(outfile_name);

    lenread = fread( headerdata, 1, 512, infile );
    if( lenread != 512 )
    {
        fprintf( stderr, "This doesn't look like a valid encrypted iHP "
                 "firmware - reason: header length\n" );
        fclose(infile);
        fclose(outfile);
        return -1;
    };

    i = testheader( headerdata );
    if( i == -1 )
    {
        fprintf( stderr, "This firmware is for an unknown model, or is not"
                 " a valid encrypted iHP firmware\n" );
        fclose(infile);
        fclose(outfile);
        return -2;
    };
    fprintf( stderr, "Model %s\n", models[ i ] );

    dwLength1 = headerdata[0] | (headerdata[1]<<8) |
        (headerdata[2]<<16) | (headerdata[3]<<24);
    dwLength2 = headerdata[4] | (headerdata[5]<<8) |
        (headerdata[6]<<16) | (headerdata[7]<<24);
    dwLength3 = headerdata[8] | (headerdata[9]<<8) |
        (headerdata[10]<<16) | (headerdata[11]<<24);

    if( dwLength1 < firmware_minsize[ i ] ||
        dwLength1 > firmware_maxsize[ i ] ||
        dwLength2 < firmware_minsize[ i ] ||
        dwLength2 > dwLength1 ||
        dwLength3 > dwLength1 ||
        dwLength2>>9 != dwLength3 ||
        dwLength2+dwLength3+512 != dwLength1 )
    {
        fprintf( stderr, "This doesn't look like a valid encrypted "
                 "iHP firmware - reason: file 'length' data\n" );
        fclose(infile);
        fclose(outfile);
        return -3;
    };

    pChecksums = ppChecksums = (unsigned char *)( malloc( dwLength3 ) );

    if( modify )
    {
        modifyheader( headerdata );
    };

    if( stripmode == STRIP_NONE )
        fwrite( headerdata, 512, 1, outfile );

    memset( blockdata, 0, 16 );

    ck = 0;
    while( ( fp < dwLength2 ) &&
           ( lenread = fread( blockdata+16, 1, 16, infile ) ) == 16 )
    {
        fp += 16;

        for( i=0; i<16; ++i )
        {
            newmunge = blockdata[16+i] ^ munge[i];
            out[i] = newmunge ^ blockdata[i];
            blockdata[i] = newmunge;
            ck += out[i];
        }

        if( fp > ESTF_SIZE || stripmode != STRIP_HEADER_CHECKSUM_ESTF )
        {
            fwrite( out+4, 1, 12, outfile );
            fwrite( out, 1, 4, outfile );
        }
        else
        {
            if( ESTF_SIZE - fp < 16 )
            {
                memcpy( out+4, blockdata+16, 12 );
                memcpy( out, blockdata+28, 4 );
                fwrite( blockdata+16+ESTF_SIZE-fp, 1, ESTF_SIZE-fp, outfile );
            }
        }


        if( s == 496 )
        {
            s = 0;
            memset( blockdata, 0, 16 );
            *ppChecksums++ = ck;
            ck = 0;
        }
        else
            s+=16;
    };

    if( fp != dwLength2 )
    {
        fprintf( stderr, "This doesn't look like a valid encrypted "
                 "iHP firmware - reason: 'length2' mismatch\n" );
        fclose(infile);
        fclose(outfile);
        return -4;
    };

    fp = 0;
    ppChecksums = pChecksums;
    while( ( fp < dwLength3 ) &&
           ( lenread = fread( blockdata, 1, 32, infile ) ) > 0 )
    {
        fp += lenread;
        if( stripmode == STRIP_NONE )
            fwrite( blockdata, 1, lenread, outfile );
        if( memcmp( ppChecksums, blockdata, lenread ) != 0 )
        {
            fprintf( stderr, "This doesn't look like a valid encrypted "
                     "iHP firmware - reason: Checksum mismatch!" );
            fclose(infile);
            fclose(outfile);
            return -5;
        };
        ppChecksums += lenread;
    };

    if( fp != dwLength3 )
    {
        fprintf( stderr, "This doesn't look like a valid encrypted "
                 "iHP firmware - reason: 'length3' mismatch\n" );
        fclose(infile);
        fclose(outfile);
        return -6;
    };


    fprintf( stderr, "File decoded correctly and all checksums matched!\n" );
    switch( stripmode )
    {
        default:
        case STRIP_NONE:
            fprintf(stderr, "Output file contains all headers and "
                    "checksums\n");
            break;
        case STRIP_HEADER_CHECKSUM:
            fprintf( stderr, "NB:  output file contains only ESTFBINR header"
                     " and decoded firmware code\n" );
            break;
        case STRIP_HEADER_CHECKSUM_ESTF:
            fprintf( stderr, "NB:  output file contains only raw decoded "
                     "firmware code\n" );
            break;
    };

    return 0;
}

int iriver_encode(const char *infile_name, const char *outfile_name, BOOL modify )
{
    FILE * infile = NULL;
    FILE * outfile = NULL;
    int i = -1;
    unsigned char headerdata[512];
    unsigned long dwLength1, dwLength2, dwLength3, fp = 0;
    unsigned char blockdata[16+16];
    unsigned char out[16];
    unsigned char newmunge;
    signed long lenread;
    int s = 0;
    unsigned char * pChecksums, * ppChecksums;
    unsigned char ck;

    infile = openinfile(infile_name);
    outfile = openoutfile(outfile_name);

    lenread = fread( headerdata, 1, 512, infile );
    if( lenread != 512 )
    {
        fprintf( stderr, "This doesn't look like a valid decoded "
                 "iHP firmware - reason: header length\n" );
        fclose(infile);
        fclose(outfile);
        return -1;
    };

    if( modify )
    {
        modifyheader( headerdata ); /* reversible */
    };

    i = testheader( headerdata );
    if( i == -1 )
    {
        fprintf( stderr, "This firmware is for an unknown model, or is not"
                 " a valid decoded iHP firmware\n" );
        fclose(infile);
        fclose(outfile);
        return -2;
    };
    fprintf( stderr, "Model %s\n", models[ i ] );

    dwLength1 = headerdata[0] | (headerdata[1]<<8) |
        (headerdata[2]<<16) | (headerdata[3]<<24);
    dwLength2 = headerdata[4] | (headerdata[5]<<8) |
        (headerdata[6]<<16) | (headerdata[7]<<24);
    dwLength3 = headerdata[8] | (headerdata[9]<<8) |
        (headerdata[10]<<16) | (headerdata[11]<<24);

    if( dwLength1 < firmware_minsize[i] ||
        dwLength1 > firmware_maxsize[i] ||
        dwLength2 < firmware_minsize[i] ||
        dwLength2 > dwLength1 ||
        dwLength3 > dwLength1 ||
        dwLength2+dwLength3+512 != dwLength1 )
    {
        fprintf( stderr, "This doesn't look like a valid decoded iHP"
                 " firmware - reason: file 'length' data\n" );
        fclose(infile);
        fclose(outfile);
        return -3;
    };

    pChecksums = ppChecksums = (unsigned char *)( malloc( dwLength3 ) );

    fwrite( headerdata, 512, 1, outfile );

    memset( blockdata, 0, 16 );
    ck = 0;
    while( ( fp < dwLength2 ) &&
           ( lenread = fread( blockdata+16, 1, 16, infile ) ) == 16 )
    {
        fp += 16;
        for( i=0; i<16; ++i )
        {
            newmunge = blockdata[16+((12+i)&0xf)] ^ blockdata[i];
            out[i] = newmunge ^ munge[i];
            ck += blockdata[16+i];
            blockdata[i] = newmunge;
        };
        fwrite( out, 1, 16, outfile );

        if( s == 496 )
        {
            s = 0;
            memset( blockdata, 0, 16 );
            *ppChecksums++ = ck;
            ck = 0;
        }
        else
            s+=16;
    };

    if( fp != dwLength2 )
    {
        fprintf( stderr, "This doesn't look like a valid decoded "
                 "iHP firmware - reason: 'length1' mismatch\n" );
        fclose(infile);
        fclose(outfile);
        return -4;
    };

    /* write out remainder w/out applying descrambler */
    fp = 0;
    lenread = dwLength3;
    ppChecksums = pChecksums;
    while( ( fp < dwLength3) &&
           ( lenread = fwrite( ppChecksums, 1, lenread, outfile ) ) > 0 )
    {
        fp += lenread;
        ppChecksums += lenread;
        lenread = dwLength3 - fp;
    };

    if( fp != dwLength3 )
    {
        fprintf( stderr, "This doesn't look like a valid decoded "
                 "iHP firmware - reason: 'length2' mismatch\n" );
        fclose(infile);
        fclose(outfile);
        return -5;
    };

    fprintf( stderr, "File encoded successfully and checksum table built!\n" );

    fclose(infile);
    fclose(outfile);
    return 0;
}
