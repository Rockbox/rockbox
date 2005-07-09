////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2004 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wputils.c

// This module provides a high-level interface for decoding WavPack 4.0 audio
// streams and files. WavPack data is read with a stream reading callback. No
// direct seeking is provided for, but it is possible to start decoding
// anywhere in a WavPack stream. In this case, WavPack will be able to provide
// the sample-accurate position when it synchs with the data and begins
// decoding.

#include "wavpack.h"

#include <string.h>

static void strcpy_loc (char *dst, char *src) { while ((*dst++ = *src++) != 0); }

///////////////////////////// local table storage ////////////////////////////

const ulong sample_rates [] = { 6000, 8000, 9600, 11025, 12000, 16000, 22050,
    24000, 32000, 44100, 48000, 64000, 88200, 96000, 192000 };

///////////////////////////// executable code ////////////////////////////////

static ulong read_next_header (read_stream infile, WavpackHeader *wphdr);
        
// This function reads data from the specified stream in search of a valid
// WavPack 4.0 audio block. If this fails in 1 megabyte (or an invalid or
// unsupported WavPack block is encountered) then an appropriate message is
// copied to "error" and NULL is returned, otherwise a pointer to a
// WavpackContext structure is returned (which is used to call all other
// functions in this module). This can be initiated at the beginning of a
// WavPack file, or anywhere inside a WavPack file. To determine the exact
// position within the file use WavpackGetSampleIndex(). For demonstration
// purposes this uses a single static copy of the WavpackContext structure,
// so obviously it cannot be used for more than one file at a time. Also,
// this function will not handle "correction" files, plays only the first
// two channels of multi-channel files, and is limited in resolution in some
// large integer or floating point files (but always provides at least 24 bits
// of resolution).

static WavpackContext wpc IDATA_ATTR;

WavpackContext *WavpackOpenFileInput (read_stream infile, char *error)
{
    WavpackStream *wps = &wpc.stream;
    ulong bcount;

    CLEAR (wpc);
    wpc.infile = infile;
    wpc.total_samples = (ulong) -1;
    wpc.norm_offset = 0;
    wpc.open_flags = 0;

    // open the source file for reading and store the size

    while (!wps->wphdr.block_samples) {

        bcount = read_next_header (wpc.infile, &wps->wphdr);

        if (bcount == (ulong) -1) {
            strcpy_loc (error, "invalid WavPack file!");
            return NULL;
        }

        if ((wps->wphdr.flags & UNKNOWN_FLAGS) || wps->wphdr.version < 0x402 || wps->wphdr.version > 0x40f) {
            strcpy_loc (error, "invalid WavPack file!");
            return NULL;
        }

        if (wps->wphdr.block_samples && wps->wphdr.total_samples != (ulong) -1)
            wpc.total_samples = wps->wphdr.total_samples;

        if (!unpack_init (&wpc)) {
            strcpy_loc (error, wpc.error_message [0] ? wpc.error_message :
                "invalid WavPack file!");

            return NULL;
        }
    }

    wpc.config.flags &= ~0xff;
    wpc.config.flags |= wps->wphdr.flags & 0xff;
    wpc.config.bytes_per_sample = (wps->wphdr.flags & BYTES_STORED) + 1;
    wpc.config.float_norm_exp = wps->float_norm_exp;

    wpc.config.bits_per_sample = (wpc.config.bytes_per_sample * 8) - 
        ((wps->wphdr.flags & SHIFT_MASK) >> SHIFT_LSB);

    if (!wpc.config.sample_rate) {
        if (!wps || !wps->wphdr.block_samples || (wps->wphdr.flags & SRATE_MASK) == SRATE_MASK)
            wpc.config.sample_rate = 44100;
        else
            wpc.config.sample_rate = sample_rates [(wps->wphdr.flags & SRATE_MASK) >> SRATE_LSB];
    }

    if (!wpc.config.num_channels) {
        wpc.config.num_channels = (wps->wphdr.flags & MONO_FLAG) ? 1 : 2;
        wpc.config.channel_mask = 0x5 - wpc.config.num_channels;
    }

    if (!(wps->wphdr.flags & FINAL_BLOCK))
        wpc.reduced_channels = (wps->wphdr.flags & MONO_FLAG) ? 1 : 2;

    return &wpc;
}

// This function obtains general information about an open file and returns
// a mask with the following bit values:

// MODE_LOSSLESS:  file is lossless (pure lossless only)
// MODE_HYBRID:  file is hybrid mode (lossy part only)
// MODE_FLOAT:  audio data is 32-bit ieee floating point
// MODE_HIGH:  file was created in "high" mode (information only)
// MODE_FAST:  file was created in "fast" mode (information only)

int WavpackGetMode (WavpackContext *wpc)
{
    int mode = 0;

    if (wpc) {
        if (wpc->config.flags & CONFIG_HYBRID_FLAG)
            mode |= MODE_HYBRID;
        else if (!(wpc->config.flags & CONFIG_LOSSY_MODE))
            mode |= MODE_LOSSLESS;

        if (wpc->lossy_blocks)
            mode &= ~MODE_LOSSLESS;

        if (wpc->config.flags & CONFIG_FLOAT_DATA)
            mode |= MODE_FLOAT;

        if (wpc->config.flags & CONFIG_HIGH_FLAG)
            mode |= MODE_HIGH;

        if (wpc->config.flags & CONFIG_FAST_FLAG)
            mode |= MODE_FAST;
    }

    return mode;
}

// Unpack the specified number of samples from the current file position.
// Note that "samples" here refers to "complete" samples, which would be
// 2 longs for stereo files. The audio data is returned right-justified in
// 32-bit longs in the endian mode native to the executing processor. So,
// if the original data was 16-bit, then the values returned would be
// +/-32k. Floating point data can also be returned if the source was
// floating point data (and this is normalized to +/-1.0). The actual number
// of samples unpacked is returned, which should be equal to the number
// requested unless the end of fle is encountered or an error occurs.

ulong WavpackUnpackSamples (WavpackContext *wpc, long *buffer, ulong samples)
{
    WavpackStream *wps = &wpc->stream;
    ulong bcount, samples_unpacked = 0, samples_to_unpack;
    int num_channels = wpc->config.num_channels;

    while (samples) {
        if (!wps->wphdr.block_samples || !(wps->wphdr.flags & INITIAL_BLOCK) ||
            wps->sample_index >= wps->wphdr.block_index + wps->wphdr.block_samples) {
                bcount = read_next_header (wpc->infile, &wps->wphdr);

                if (bcount == (ulong) -1)
                    break;

                if (wps->wphdr.version < 0x402 || wps->wphdr.version > 0x40f) {
                    strcpy_loc (wpc->error_message, "invalid WavPack file!");
                    break;
                }

                if (!wps->wphdr.block_samples || wps->sample_index == wps->wphdr.block_index)
                    if (!unpack_init (wpc))
                        break;
        }

        if (!wps->wphdr.block_samples || !(wps->wphdr.flags & INITIAL_BLOCK) ||
            wps->sample_index >= wps->wphdr.block_index + wps->wphdr.block_samples)
                continue;

        if (wps->sample_index < wps->wphdr.block_index) {
            samples_to_unpack = wps->wphdr.block_index - wps->sample_index;

            if (samples_to_unpack > samples)
                samples_to_unpack = samples;

            wps->sample_index += samples_to_unpack;
            samples_unpacked += samples_to_unpack;
            samples -= samples_to_unpack;

            if (wpc->reduced_channels)
                samples_to_unpack *= wpc->reduced_channels;
            else
                samples_to_unpack *= num_channels;

            while (samples_to_unpack--)
                *buffer++ = 0;

            continue;
        }

        samples_to_unpack = wps->wphdr.block_index + wps->wphdr.block_samples - wps->sample_index;

        if (samples_to_unpack > samples)
            samples_to_unpack = samples;

        unpack_samples (wpc, buffer, samples_to_unpack);

        if (wpc->reduced_channels)
            buffer += samples_to_unpack * wpc->reduced_channels;
        else
            buffer += samples_to_unpack * num_channels;

        samples_unpacked += samples_to_unpack;
        samples -= samples_to_unpack;

        if (wps->sample_index == wps->wphdr.block_index + wps->wphdr.block_samples) {
            if (check_crc_error (wpc))
                wpc->crc_errors++;
        }

        if (wps->sample_index == wpc->total_samples)
            break;
    }

    return samples_unpacked;
}

// Get total number of samples contained in the WavPack file, or -1 if unknown

ulong WavpackGetNumSamples (WavpackContext *wpc)
{
    return wpc ? wpc->total_samples : (ulong) -1;
}

// Get the current sample index position, or -1 if unknown

ulong WavpackGetSampleIndex (WavpackContext *wpc)
{
    if (wpc)
        return wpc->stream.sample_index;

    return (ulong) -1;
}

// Get the number of errors encountered so far

int WavpackGetNumErrors (WavpackContext *wpc)
{
    return wpc ? wpc->crc_errors : 0;
}

// return TRUE if any uncorrected lossy blocks were actually written or read

int WavpackLossyBlocks (WavpackContext *wpc)
{
    return wpc ? wpc->lossy_blocks : 0;
}

// Returns the sample rate of the specified WavPack file

ulong WavpackGetSampleRate (WavpackContext *wpc)
{
    return wpc ? wpc->config.sample_rate : 44100;
}

// Returns the number of channels of the specified WavPack file. Note that
// this is the actual number of channels contained in the file, but this
// version can only decode the first two.

int WavpackGetNumChannels (WavpackContext *wpc)
{
    return wpc ? wpc->config.num_channels : 2;
}

// Returns the actual number of valid bits per sample contained in the
// original file, which may or may not be a multiple of 8. Floating data
// always has 32 bits, integers may be from 1 to 32 bits each. When this
// value is not a multiple of 8, then the "extra" bits are located in the
// LSBs of the results. That is, values are right justified when unpacked
// into longs, but are left justified in the number of bytes used by the
// original data.

int WavpackGetBitsPerSample (WavpackContext *wpc)
{
    return wpc ? wpc->config.bits_per_sample : 16;
}

// Returns the number of bytes used for each sample (1 to 4) in the original
// file. This is required information for the user of this module because the
// audio data is returned in the LOWER bytes of the long buffer and must be
// left-shifted 8, 16, or 24 bits if normalized longs are required.

int WavpackGetBytesPerSample (WavpackContext *wpc)
{
    return wpc ? wpc->config.bytes_per_sample : 2;
}

// This function will return the actual number of channels decoded from the
// file (which may or may not be less than the actual number of channels, but
// will always be 1 or 2). Normally, this will be the front left and right
// channels of a multi-channel file.

int WavpackGetReducedChannels (WavpackContext *wpc)
{
    if (wpc)
        return wpc->reduced_channels ? wpc->reduced_channels : wpc->config.num_channels;
    else
        return 2;
}

// Read from current file position until a valid 32-byte WavPack 4.0 header is
// found and read into the specified pointer. The number of bytes skipped is
// returned. If no WavPack header is found within 1 meg, then a -1 is returned
// to indicate the error. No additional bytes are read past the header and it
// is returned in the processor's native endian mode. Seeking is not required.

static ulong read_next_header (read_stream infile, WavpackHeader *wphdr)
{
    char buffer [sizeof (*wphdr)], *sp = buffer + sizeof (*wphdr), *ep = sp;
    ulong bytes_skipped = 0;
    int bleft;

    while (1) {
        if (sp < ep) {
            bleft = ep - sp;
            memcpy (buffer, sp, bleft);
        }
        else
            bleft = 0;

        if (infile (buffer + bleft, sizeof (*wphdr) - bleft) != (long) sizeof (*wphdr) - bleft)
            return -1;

        sp = buffer;

        if (*sp++ == 'w' && *sp == 'v' && *++sp == 'p' && *++sp == 'k' &&
            !(*++sp & 1) && sp [2] < 16 && !sp [3] && sp [5] == 4 && sp [4] >= 2 && sp [4] <= 0xf) {
                memcpy (wphdr, buffer, sizeof (*wphdr));
                little_endian_to_native (wphdr, WavpackHeaderFormat);
                return bytes_skipped;
            }

        while (sp < ep && *sp != 'w')
            sp++;

        if ((bytes_skipped += sp - buffer) > 1024 * 1024)
            return -1;
    }
}

// Open context for writing WavPack files. The returned context pointer is used
// in all following calls to the library. A return value of NULL indicates
// that memory could not be allocated for the context.

WavpackContext *WavpackOpenFileOutput (void)
{
    CLEAR (wpc);
    return &wpc;
}

// Set configuration for writing WavPack files. This must be done before
// sending any actual samples, however it is okay to send wrapper or other
// metadata before calling this. The "config" structure contains the following
// required information:

// config->bytes_per_sample     see WavpackGetBytesPerSample() for info
// config->bits_per_sample      see WavpackGetBitsPerSample() for info
// config->num_channels         self evident
// config->sample_rate          self evident

// In addition, the following fields and flags may be set: 

// config->flags:
// --------------
// o CONFIG_HYBRID_FLAG         select hybrid mode (must set bitrate)
// o CONFIG_JOINT_STEREO        select joint stereo (must set override also)
// o CONFIG_JOINT_OVERRIDE      override default joint stereo selection
// o CONFIG_HYBRID_SHAPE        select hybrid noise shaping (set override &
//                                                      shaping_weight != 0.0)
// o CONFIG_SHAPE_OVERRIDE      override default hybrid noise shaping
//                               (set CONFIG_HYBRID_SHAPE and shaping_weight)
// o CONFIG_FAST_FLAG           "fast" compression mode
// o CONFIG_HIGH_FLAG           "high" compression mode
// o CONFIG_BITRATE_KBPS        hybrid bitrate is kbps, not bits / sample

// config->bitrate              hybrid bitrate in either bits/sample or kbps
// config->shaping_weight       hybrid noise shaping coefficient override
// config->float_norm_exp       select floating-point data (127 for +/-1.0)

// If the number of samples to be written is known then it should be passed
// here. If the duration is not known then pass -1. In the case that the size
// is not known (or the writing is terminated early) then it is suggested that
// the application retrieve the first block written and let the library update
// the total samples indication. A function is provided to do this update and
// it should be done to the "correction" file also. If this cannot be done
// (because a pipe is being used, for instance) then a valid WavPack will still
// be created, but when applications want to access that file they will have
// to seek all the way to the end to determine the actual duration. Also, if
// a RIFF header has been included then it should be updated as well or the
// WavPack file will not be directly unpackable to a valid wav file (although
// it will still be usable by itself). A return of FALSE indicates an error.

int WavpackSetConfiguration (WavpackContext *wpc, WavpackConfig *config, ulong total_samples)
{
    WavpackStream *wps = &wpc->stream;
    ulong flags = (config->bytes_per_sample - 1), shift = 0;
    int num_chans = config->num_channels;
    int i;

    if ((wpc->config.flags & CONFIG_HYBRID_FLAG) ||
        wpc->config.float_norm_exp ||
        num_chans < 1 || num_chans > 2)
            return FALSE;

    wpc->total_samples = total_samples;
    wpc->config.sample_rate = config->sample_rate;
    wpc->config.num_channels = config->num_channels;
    wpc->config.bits_per_sample = config->bits_per_sample;
    wpc->config.bytes_per_sample = config->bytes_per_sample;
    wpc->config.flags = config->flags;

    shift = (config->bytes_per_sample * 8) - config->bits_per_sample;

    for (i = 0; i < 15; ++i)
        if (wpc->config.sample_rate == sample_rates [i])
            break;

    flags |= i << SRATE_LSB;
    flags |= shift << SHIFT_LSB;
    flags |= CROSS_DECORR;

    if (!(config->flags & CONFIG_JOINT_OVERRIDE) || (config->flags & CONFIG_JOINT_STEREO))
        flags |= JOINT_STEREO;

    flags |= INITIAL_BLOCK | FINAL_BLOCK;

    if (num_chans == 1) {
        flags &= ~(JOINT_STEREO | CROSS_DECORR | HYBRID_BALANCE);
        flags |= MONO_FLAG;
    }

    flags &= ~MAG_MASK;
    flags += (1 << MAG_LSB) * ((flags & BYTES_STORED) * 8 + 7);

    memcpy (wps->wphdr.ckID, "wvpk", 4);
    wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
    wps->wphdr.total_samples = wpc->total_samples;
    wps->wphdr.version = 0x403;
    wps->wphdr.flags = flags;

    pack_init (wpc);
    return TRUE;
}

// Add wrapper (currently RIFF only) to WavPack blocks. This should be called
// before sending any audio samples. If the exact contents of the RIFF header
// are not known because, for example, the file duration is uncertain or
// trailing chunks are possible, simply write a "dummy" header of the correct
// length. When all data has been written it will be possible to read the
// first block written and update the header directly. An example of this can
// be found in the Audition filter.

void WavpackAddWrapper (WavpackContext *wpc, void *data, ulong bcount)
{
    wpc->wrapper_data = data;
    wpc->wrapper_bytes = bcount;
}

// Start a WavPack block to be stored in the specified buffer. This must be
// called before calling WavpackPackSamples(). Note that writing CANNOT wrap
// in the buffer; the entire output block must fit in the buffer.

int WavpackStartBlock (WavpackContext *wpc, uchar *begin, uchar *end)
{
    wpc->stream.blockbuff = begin;
    wpc->stream.blockend = end;
    return pack_start_block (wpc);
}

// Pack the specified samples. Samples must be stored in longs in the native
// endian format of the executing processor. The number of samples specified
// indicates composite samples (sometimes called "frames"). So, the actual
// number of data points would be this "sample_count" times the number of
// channels. The caller must decide how many samples to place in each
// WavPack block (1/2 second is common), but this function may be called as
// many times as desired to build the final block (and performs the actual
// compression during the call). A return of FALSE indicates an error.

int WavpackPackSamples (WavpackContext *wpc, long *sample_buffer, ulong sample_count)
{
    if (!sample_count || pack_samples (wpc, sample_buffer, sample_count))
        return TRUE;

    strcpy_loc (wpc->error_message, "output buffer overflowed!");
    return FALSE;
}

// Finish the WavPack block being built, returning the total size of the
// block in bytes. Note that the possible conversion of the WavPack header to
// little-endian takes place here.

ulong WavpackFinishBlock (WavpackContext *wpc)
{
    WavpackStream *wps = &wpc->stream;
    ulong bcount;

    pack_finish_block (wpc);
    bcount = ((WavpackHeader *) wps->blockbuff)->ckSize + 8;
    native_to_little_endian ((WavpackHeader *) wps->blockbuff, WavpackHeaderFormat);

    return bcount;
}

// Given the pointer to the first block written (to either a .wv or .wvc file),
// update the block with the actual number of samples written. This should
// be done if WavpackSetConfiguration() was called with an incorrect number
// of samples (or -1). It is the responsibility of the application to read and
// rewrite the block. An example of this can be found in the Audition filter.

void WavpackUpdateNumSamples (WavpackContext *wpc, void *first_block)
{
    little_endian_to_native (wpc, WavpackHeaderFormat);
    ((WavpackHeader *) first_block)->total_samples = WavpackGetSampleIndex (wpc);
    native_to_little_endian (wpc, WavpackHeaderFormat);
}

// Given the pointer to the first block written to a WavPack file, this
// function returns the location of the stored RIFF header that was originally
// written with WavpackAddWrapper(). This would normally be used to update
// the wav header to indicate that a different number of samples was actually
// written or if additional RIFF chunks are written at the end of the file.
// It is the responsibility of the application to read and rewrite the block.
// An example of this can be found in the Audition filter.

void *WavpackGetWrapperLocation (void *first_block)
{
    if (((uchar *) first_block) [32] == ID_RIFF_HEADER)
        return ((uchar *) first_block) + 34;
    else
        return NULL;
}

