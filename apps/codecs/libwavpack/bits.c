////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2004 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// bits.c

// This module provides utilities to support the BitStream structure which is
// used to read and write all WavPack audio data streams. It also contains a
// wrapper for the stream I/O functions and a set of functions dealing with
// endian-ness, both for enhancing portability. Finally, a debug wrapper for
// the malloc() system is provided.

#include "wavpack.h"

#include <string.h>

////////////////////////// Bitstream functions ////////////////////////////////

// Open the specified BitStream and associate with the specified buffer.

static void bs_read (Bitstream *bs);

void bs_open_read (Bitstream *bs, uchar *buffer_start, uchar *buffer_end, read_stream file, ulong file_bytes)
{
    CLEAR (*bs);
    bs->buf = buffer_start;
    bs->end = buffer_end;

    if (file) {
        bs->ptr = bs->end - 1;
        bs->file_bytes = file_bytes;
        bs->file = file;
    }
    else
        bs->ptr = bs->buf - 1;

    bs->wrap = bs_read;
}

// This function is only called from the getbit() and getbits() macros when
// the BitStream has been exhausted and more data is required. Sinve these
// bistreams no longer access files, this function simple sets an error and
// resets the buffer.

static void bs_read (Bitstream *bs)
{
    if (bs->file && bs->file_bytes) {
        ulong bytes_read, bytes_to_read = bs->end - bs->buf;

        if (bytes_to_read > bs->file_bytes)
            bytes_to_read = bs->file_bytes;

        bytes_read = bs->file (bs->buf, bytes_to_read);

        if (bytes_read) {
            bs->end = bs->buf + bytes_read;
            bs->file_bytes -= bytes_read;
        }
        else {
            memset (bs->buf, -1, bs->end - bs->buf);
            bs->error = 1;
        }
    }
    else
        bs->error = 1;

    if (bs->error)
        memset (bs->buf, -1, bs->end - bs->buf);

    bs->ptr = bs->buf;
}

// Open the specified BitStream using the specified buffer pointers. It is
// assumed that enough buffer space has been allocated for all data that will
// be written, otherwise an error will be generated.

static void bs_write (Bitstream *bs);

void bs_open_write (Bitstream *bs, uchar *buffer_start, uchar *buffer_end)
{
    bs->error = bs->sr = bs->bc = 0;
    bs->ptr = bs->buf = buffer_start;
    bs->end = buffer_end;
    bs->wrap = bs_write;
}

// This function is only called from the putbit() and putbits() macros when
// the buffer is full, which is now flagged as an error.

static void bs_write (Bitstream *bs)
{
    bs->ptr = bs->buf;
    bs->error = 1;
}

// This function forces a flushing write of the specified BitStream, and
// returns the total number of bytes written into the buffer.

ulong bs_close_write (Bitstream *bs)
{
    ulong bytes_written;

    if (bs->error)
        return (ulong) -1;

    while (bs->bc || ((bs->ptr - bs->buf) & 1)) putbit_1 (bs);
    bytes_written = bs->ptr - bs->buf;
    CLEAR (*bs);
    return bytes_written;
}

/////////////////////// Endian Correction Routines ////////////////////////////

void little_endian_to_native (void *data, char *format)
{
    uchar *cp = (uchar *) data;
    long temp;

    while (*format) {
        switch (*format) {
            case 'L':
                temp = cp [0] + ((long) cp [1] << 8) + ((long) cp [2] << 16) + ((long) cp [3] << 24);
                * (long *) cp = temp;
                cp += 4;
                break;

            case 'S':
                temp = cp [0] + (cp [1] << 8);
                * (short *) cp = (short) temp;
                cp += 2;
                break;

            default:
                if (*format >= '0' && *format <= '9')
                    cp += *format - '0';

                break;
        }

        format++;
    }
}

void native_to_little_endian (void *data, char *format)
{
    uchar *cp = (uchar *) data;
    long temp;

    while (*format) {
        switch (*format) {
            case 'L':
                temp = * (long *) cp;
                *cp++ = (uchar) temp;
                *cp++ = (uchar) (temp >> 8);
                *cp++ = (uchar) (temp >> 16);
                *cp++ = (uchar) (temp >> 24);
                break;

            case 'S':
                temp = * (short *) cp;
                *cp++ = (uchar) temp;
                *cp++ = (uchar) (temp >> 8);
                break;

            default:
                if (*format >= '0' && *format <= '9')
                    cp += *format - '0';

                break;
        }

        format++;
    }
}
