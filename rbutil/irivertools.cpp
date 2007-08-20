/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: irivertools.cpp
 *
 * Copyright (C) 2007 Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "irivertools.h"


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

/* begin mkboot.c excerpt */
unsigned char image[0x400000 + 0x220 + 0x400000/0x200];

bool mkboot(wxString infile, wxString outfile,wxString bootloader,int origin)
{
    wxString err;
    int i;
    int len,bllen;
    int actual_length, total_length, binary_length, num_chksums;

    memset(image, 0xff, sizeof(image));

    /* First, read the iriver original firmware into the image */
    wxFile f;
    if(!f.Open(infile))
    {
        ERR_DIALOG(wxT("Could not open: ") + infile, wxT("mkboot"));
        return false;
    }
    i = f.Read(image,16);
    if(i < 16) {
        ERR_DIALOG(wxT("reading header failed"), wxT("mkboot"));
        return false;
    }

    /* This is the length of the binary image without the scrambling
       overhead (but including the ESTFBINR header) */
    binary_length = image[4] + (image[5] << 8) +
        (image[6] << 16) + (image[7] << 24);

    /* Read the rest of the binary data, but not the checksum block */
    len = binary_length+0x200-16;
    i = f.Read(image+16, len);
    if(i < len) {
        ERR_DIALOG(wxT("reading firmware failed"),wxT("mkboot"));
        return false;
    }

    f.Close();
    /* Now, read the boot loader into the image */
    if(!f.Open(bootloader))
    {
        ERR_DIALOG(wxT("Could not open: ") + bootloader, wxT("mkboot"));
        return false;
    }

    bllen = f.Length();

    i = f.Read(image+0x220 + origin, bllen);
    if(i < bllen) {
       ERR_DIALOG(wxT("reading bootloader failed"), wxT("mkboot"));
       return false;
    }

    f.Close();

    if(!f.Open(outfile,wxFile::write))
    {
        ERR_DIALOG(wxT("Could not open: ") + outfile, wxT("mkboot"));
        return false;
    }

    /* Patch the reset vector to start the boot loader */
    image[0x220 + 4] = image[origin + 0x220 + 4];
    image[0x220 + 5] = image[origin + 0x220 + 5];
    image[0x220 + 6] = image[origin + 0x220 + 6];
    image[0x220 + 7] = image[origin + 0x220 + 7];

    /* This is the actual length of the binary, excluding all headers */
    actual_length = origin + bllen;

    /* Patch the ESTFBINR header */
    image[0x20c] = (actual_length >> 24) & 0xff;
    image[0x20d] = (actual_length >> 16) & 0xff;
    image[0x20e] = (actual_length >> 8) & 0xff;
    image[0x20f] = actual_length & 0xff;

    image[0x21c] = (actual_length >> 24) & 0xff;
    image[0x21d] = (actual_length >> 16) & 0xff;
    image[0x21e] = (actual_length >> 8) & 0xff;
    image[0x21f] = actual_length & 0xff;

    /* This is the length of the binary, including the ESTFBINR header and
       rounded up to the nearest 0x200 boundary */
    binary_length = (actual_length + 0x20 + 0x1ff) & 0xfffffe00;

    /* The number of checksums, i.e number of 0x200 byte blocks */
    num_chksums = binary_length / 0x200;

    /* The total file length, including all headers and checksums */
    total_length = binary_length + num_chksums + 0x200;

    /* Patch the scrambler header with the new length info */
    image[0] = total_length & 0xff;
    image[1] = (total_length >> 8) & 0xff;
    image[2] = (total_length >> 16) & 0xff;
    image[3] = (total_length >> 24) & 0xff;

    image[4] = binary_length & 0xff;
    image[5] = (binary_length >> 8) & 0xff;
    image[6] = (binary_length >> 16) & 0xff;
    image[7] = (binary_length >> 24) & 0xff;

    image[8] = num_chksums & 0xff;
    image[9] = (num_chksums >> 8) & 0xff;
    image[10] = (num_chksums >> 16) & 0xff;
    image[11] = (num_chksums >> 24) & 0xff;

    i = f.Write(image,total_length);
    if(i < total_length) {
        ERR_DIALOG(wxT("writing bootloader failed"), wxT("mkboot"));
        return false;
    }

    f.Close();

    return true;
}

/* end mkboot.c excerpt */


int intable(char *md5, struct sumpairs *table, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (strncmp(md5, table[i].unpatched, 32) == 0) {
            return i;
        }
    }
    return -1;
}




static int testheader( const unsigned char * const data )
{
    const unsigned char * const d = data+16;
    const char * const * m = models;
    int index = 0;
    while( *m )
    {
        if( memcmp( header[ index ], d, 16 ) == 0 )
            return index;
        index++;
        m++;
    };
    return -1;
};

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
};

int iriver_decode(wxString infile_name, wxString outfile_name, unsigned int modify,
                  enum striptype stripmode )
{
    wxString err;
    wxFile infile;
    wxFile outfile;
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

    if(!infile.Open(infile_name))
    {
        ERR_DIALOG(wxT("Could not open: ") + infile_name, wxT("iriver_decode"));
        return -1;
    }
    if(!outfile.Open(outfile_name,wxFile::write))
    {
        ERR_DIALOG(wxT("Could not open: ") + outfile_name,
                   wxT("iriver_decode"));
        return -1;
    }
    lenread = infile.Read( headerdata, 512);
    if( lenread != 512 )
    {
        ERR_DIALOG(wxT("This doesn't look like a valid encrypted iHP "
                 "firmware - reason: header length\n"),wxT("iriver_decode"));

        infile.Close();
        outfile.Close();
        return -1;
    };

    i = testheader( headerdata );
    if( i == -1 )
    {
        ERR_DIALOG( wxT( "This firmware is for an unknown model, or is not"
                 " a valid encrypted iHP firmware\n" ),wxT("iriver_decode"));
        infile.Close();
        outfile.Close();
        return -1;
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
        ERR_DIALOG( wxT( "This doesn't look like a valid encrypted "
                 "iHP firmware - reason: file 'length' data\n" ),wxT("iriver_decode"));
        infile.Close();
        outfile.Close();
        return -1;
    };

    pChecksums = ppChecksums = (unsigned char *)( malloc( dwLength3 ) );

    if( modify )
    {
        modifyheader( headerdata );
    };

    if( stripmode == STRIP_NONE )
        outfile.Write( headerdata, 512);

    memset( blockdata, 0, 16 );

    ck = 0;
    while( ( fp < dwLength2 ) &&
           ( lenread = infile.Read( blockdata+16, 16) == 16) )
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
            outfile.Write( out+4, 12);
            outfile.Write( out, 4);
        }
        else
        {
            if( ESTF_SIZE - fp < 16 )
            {
                memcpy( out+4, blockdata+16, 12 );
                memcpy( out, blockdata+28, 4 );
                outfile.Write( blockdata+16+ESTF_SIZE-fp, ESTF_SIZE-fp);
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
        ERR_DIALOG( wxT(  "This doesn't look like a valid encrypted "
                 "iHP firmware - reason: 'length2' mismatch\n" ),wxT("iriver_decode"));
        infile.Close();
        outfile.Close();
        return -1;
    };

    fp = 0;
    ppChecksums = pChecksums;
    while( ( fp < dwLength3 ) &&
           ( lenread = infile.Read( blockdata, 32 ) ) > 0 )
    {
        fp += lenread;
        if( stripmode == STRIP_NONE )
            outfile.Write( blockdata, lenread );
        if( memcmp( ppChecksums, blockdata, lenread ) != 0 )
        {
            ERR_DIALOG( wxT(  "This doesn't look like a valid encrypted "
                     "iHP firmware - reason: Checksum mismatch!" ),wxT("iriver_decode"));
            infile.Close();
            outfile.Close();
            return -1;
        };
        ppChecksums += lenread;
    };

    if( fp != dwLength3 )
    {
        ERR_DIALOG(wxT(  "This doesn't look like a valid encrypted "
                 "iHP firmware - reason: 'length3' mismatch\n" ),wxT("iriver_decode"));
        infile.Close();
        outfile.Close();
        return -1;
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

    infile.Close();
    outfile.Close();
    return 0;

};

int iriver_encode(wxString infile_name, wxString outfile_name, unsigned int modify )
{
    wxString err;
    wxFile infile;
    wxFile outfile;
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

    if(!infile.Open(infile_name,wxFile::read))
    {
        ERR_DIALOG(wxT("Could not open: ") + infile_name, wxT("iriver_decode"));
        return -1;
    }
    if(!outfile.Open(outfile_name,wxFile::write))
    {
        ERR_DIALOG(wxT("Could not open: ") + outfile_name,
                   wxT("iriver_decode"));
        return -1;
    }

    lenread = infile.Read( headerdata, 512 );
    if( lenread != 512 )
    {
        ERR_DIALOG(wxT("This doesn't look like a valid decoded "
                 "iHP firmware - reason: header length\n"), wxT("iriver_decode"));
        infile.Close();
        outfile.Close();
    };

    if( modify )
    {
        modifyheader( headerdata ); /* reversible */
    };

    i = testheader( headerdata );
    if( i == -1 )
    {
        ERR_DIALOG(wxT("This firmware is for an unknown model, or is not"
                 " a valid decoded iHP firmware\n"), wxT("iriver_decode"));
       infile.Close();
       outfile.Close();
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
        ERR_DIALOG(wxT("This doesn't look like a valid decoded iHP"
                 " firmware - reason: file 'length' data\n"), wxT("iriver_decode"));
       infile.Close();
       outfile.Close();
    };

    pChecksums = ppChecksums = (unsigned char *)( malloc( dwLength3 ) );

    outfile.Write( headerdata, 512);

    memset( blockdata, 0, 16 );
    ck = 0;
    while( ( fp < dwLength2 ) &&
           ( lenread = infile.Read( blockdata+16, 16) ) == 16 )
    {
        fp += 16;
        for( i=0; i<16; ++i )
        {
            newmunge = blockdata[16+((12+i)&0xf)] ^ blockdata[i];
            out[i] = newmunge ^ munge[i];
            ck += blockdata[16+i];
            blockdata[i] = newmunge;
        };
        outfile.Write( out, 16);

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
        ERR_DIALOG(wxT("This doesn't look like a valid decoded "
                 "iHP firmware - reason: 'length1' mismatch\n"), wxT("iriver_decode"));
        infile.Close();
        outfile.Close();
    };

    /* write out remainder w/out applying descrambler */
    fp = 0;
    lenread = dwLength3;
    ppChecksums = pChecksums;
    while( ( fp < dwLength3) &&
           ( lenread = outfile.Write( ppChecksums, lenread) ) > 0 )
    {
        fp += lenread;
        ppChecksums += lenread;
        lenread = dwLength3 - fp;
    };

    if( fp != dwLength3 )
    {
       ERR_DIALOG(wxT("This doesn't look like a valid decoded "
                 "iHP firmware - reason: 'length2' mismatch\n"), wxT("iriver_decode"));
       infile.Close();
       outfile.Close();
    };

    fprintf( stderr, "File encoded successfully and checksum table built!\n" );

    infile.Close();
    outfile.Close();
    return 0;

};

bool PatchFirmware(wxString firmware,wxString bootloader,int series, int table_entry)
{
    wxString name1, name2, name3;

    char md5sum_str[32];
    struct sumpairs *sums;
    int origin;

    /* get pointer to the correct bootloader.bin */
    switch(series) {
        case 100:
            sums = &h100pairs[0];
            origin = 0x1f0000;
            break;
        case 120:
            sums = &h120pairs[0];
            origin = 0x1f0000;
            break;
        case 300:
            sums = &h300pairs[0];
            origin = 0x3f0000;
            break;
    }

    name1 = gv->stdpaths->GetUserDataDir()
            + wxT("" PATH_SEP "download" PATH_SEP "firmware.bin"),
                                        /* descrambled file */
    name2 = gv->stdpaths->GetUserDataDir()
            + wxT("" PATH_SEP "download" PATH_SEP "new.bin");
                                    /* patched file */
    name3 = gv->stdpaths->GetUserDataDir()
            + wxT("" PATH_SEP "download" PATH_SEP "new.hex");
    if (iriver_decode(firmware, name1, FALSE, STRIP_NONE) == -1) {
        ERR_DIALOG(wxT("Error in descramble"), wxT("Descramble Firmware"));
        wxRemoveFile(name1);
        wxRemoveFile(name2);
        wxRemoveFile(name3);
        return false;
    }
    if (!mkboot(name1, name2, bootloader, origin)) {
        ERR_DIALOG(wxT("Error in patching"),wxT("Patching Firmware"));
        wxRemoveFile(name1);
        wxRemoveFile(name2);
        wxRemoveFile(name3);
        return false;
    }
    if (iriver_encode(name2, name3, FALSE) == -1) {
        ERR_DIALOG(wxT("Error in scramble"),wxT("Scramble Firmware"));
        wxRemoveFile(name1);
        wxRemoveFile(name2);
        wxRemoveFile(name3);
        return false;
    }
    /* now md5sum it */
    if (!FileMD5(name3, md5sum_str)) {
        ERR_DIALOG(wxT("Error in checksumming"),wxT("Checksumming Firmware"));
        wxRemoveFile(name1);
        wxRemoveFile(name2);
        wxRemoveFile(name3);
        return false;
    }
    if (strncmp(sums[table_entry].patched, md5sum_str, 32) == 0) {
        /* delete temp files */
        wxRemoveFile(name1);
        wxRemoveFile(name2);
    }

    return true;
}

