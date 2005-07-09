////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2005 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// pack.c

// This module actually handles the compression of the audio data, except for
// the entropy coding which is handled by the words? modules. For efficiency,
// the conversion is isolated to tight loops that handle an entire buffer.

#include "wavpack.h"

#include <string.h>

// This flag provides faster encoding speed at the expense of more code. The
// improvement applies to 16-bit stereo lossless only.

//////////////////////////////// local tables ///////////////////////////////

// These two tables specify the characteristics of the decorrelation filters.
// Each term represents one layer of the sequential filter, where positive
// values indicate the relative sample involved from the same channel (1=prev),
// 17 & 18 are special functions using the previous 2 samples, and negative
// values indicate cross channel decorrelation (in stereo only).

static const char default_terms [] = { 18,18,2,3,-2,0 };
static const char high_terms [] = { 18,18,2,3,-2,18,2,4,7,5,3,6,0 };
static const char fast_terms [] = { 17,17,0 };

///////////////////////////// executable code ////////////////////////////////

// This function initializes everything required to pack WavPack bitstreams
// and must be called BEFORE any other function in this module.

void pack_init (WavpackContext *wpc)
{
    WavpackStream *wps = &wpc->stream;
    ulong flags = wps->wphdr.flags;
    struct decorr_pass *dpp;
    const char *term_string;
    int ti;

    wps->sample_index = 0;
    CLEAR (wps->decorr_passes);

    if (wpc->config.flags & CONFIG_HIGH_FLAG)
        term_string = high_terms;
    else if (wpc->config.flags & CONFIG_FAST_FLAG)
        term_string = fast_terms;
    else
        term_string = default_terms;

    for (dpp = wps->decorr_passes, ti = 0; term_string [ti]; ti++)
        if (term_string [ti] >= 0 || (flags & CROSS_DECORR)) {
            dpp->term = term_string [ti];
            dpp++->delta = 2;
        }
        else if (!(flags & MONO_FLAG)) {
            dpp->term = -3;
            dpp++->delta = 2;
        }

    wps->num_terms = dpp - wps->decorr_passes;
    init_words (wps);
}

// Allocate room for and copy the decorrelation terms from the decorr_passes
// array into the specified metadata structure. Both the actual term id and
// the delta are packed into single characters.

static void write_decorr_terms (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int tcount = wps->num_terms;
    struct decorr_pass *dpp;
    char *byteptr;

    byteptr = wpmd->data = wpmd->temp_data;
    wpmd->id = ID_DECORR_TERMS;

    for (dpp = wps->decorr_passes; tcount--; ++dpp)
        *byteptr++ = ((dpp->term + 5) & 0x1f) | ((dpp->delta << 5) & 0xe0);

    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Allocate room for and copy the decorrelation term weights from the
// decorr_passes array into the specified metadata structure. The weights
// range +/-1024, but are rounded and truncated to fit in signed chars for
// metadata storage. Weights are separate for the two channels

static void write_decorr_weights (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int tcount = wps->num_terms;
    struct decorr_pass *dpp;
    char *byteptr;

    byteptr = wpmd->data = wpmd->temp_data;
    wpmd->id = ID_DECORR_WEIGHTS;

    for (dpp = wps->decorr_passes; tcount--; ++dpp) {
        dpp->weight_A = restore_weight (*byteptr++ = store_weight (dpp->weight_A));

        if (!(wps->wphdr.flags & MONO_FLAG))
            dpp->weight_B = restore_weight (*byteptr++ = store_weight (dpp->weight_B));
    }

    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Allocate room for and copy the decorrelation samples from the decorr_passes
// array into the specified metadata structure. The samples are signed 32-bit
// values, but are converted to signed log2 values for storage in metadata.
// Values are stored for both channels and are specified from the first term
// with unspecified samples set to zero. The number of samples stored varies
// with the actual term value, so those must obviously be specified before
// these in the metadata list. Any number of terms can have their samples
// specified from no terms to all the terms, however I have found that
// sending more than the first term's samples is a waste. The "wcount"
// variable can be set to the number of terms to have their samples stored.

static void write_decorr_samples (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int tcount = wps->num_terms, wcount = 1, temp;
    struct decorr_pass *dpp;
    uchar *byteptr;

    byteptr = wpmd->data = wpmd->temp_data;
    wpmd->id = ID_DECORR_SAMPLES;

    for (dpp = wps->decorr_passes; tcount--; ++dpp)
        if (wcount) {
            if (dpp->term > MAX_TERM) {
                dpp->samples_A [0] = exp2s (temp = log2s (dpp->samples_A [0]));
                *byteptr++ = temp;
                *byteptr++ = temp >> 8;
                dpp->samples_A [1] = exp2s (temp = log2s (dpp->samples_A [1]));
                *byteptr++ = temp;
                *byteptr++ = temp >> 8;

                if (!(wps->wphdr.flags & MONO_FLAG)) {
                    dpp->samples_B [0] = exp2s (temp = log2s (dpp->samples_B [0]));
                    *byteptr++ = temp;
                    *byteptr++ = temp >> 8;
                    dpp->samples_B [1] = exp2s (temp = log2s (dpp->samples_B [1]));
                    *byteptr++ = temp;
                    *byteptr++ = temp >> 8;
                }
            }
            else if (dpp->term < 0) {
                dpp->samples_A [0] = exp2s (temp = log2s (dpp->samples_A [0]));
                *byteptr++ = temp;
                *byteptr++ = temp >> 8;
                dpp->samples_B [0] = exp2s (temp = log2s (dpp->samples_B [0]));
                *byteptr++ = temp;
                *byteptr++ = temp >> 8;
            }
            else {
                int m = 0, cnt = dpp->term;

                while (cnt--) {
                    dpp->samples_A [m] = exp2s (temp = log2s (dpp->samples_A [m]));
                    *byteptr++ = temp;
                    *byteptr++ = temp >> 8;

                    if (!(wps->wphdr.flags & MONO_FLAG)) {
                        dpp->samples_B [m] = exp2s (temp = log2s (dpp->samples_B [m]));
                        *byteptr++ = temp;
                        *byteptr++ = temp >> 8;
                    }

                    m++;
                }
            }

            wcount--;
        }
        else {
            CLEAR (dpp->samples_A);
            CLEAR (dpp->samples_B);
        }

    wpmd->byte_length = byteptr - (uchar *) wpmd->data;
}

// Allocate room for and copy the configuration information into the specified
// metadata structure. Currently, we just store the upper 3 bytes of
// config.flags and only in the first block of audio data. Note that this is
// for informational purposes not required for playback or decoding (like
// whether high or fast mode was specified).

static void write_config_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    char *byteptr;

    byteptr = wpmd->data = wpmd->temp_data;
    wpmd->id = ID_CONFIG_BLOCK;
    *byteptr++ = (char) (wpc->config.flags >> 8);
    *byteptr++ = (char) (wpc->config.flags >> 16);
    *byteptr++ = (char) (wpc->config.flags >> 24);
    wpmd->byte_length = byteptr - (char *) wpmd->data;
}

// Pack an entire block of samples (either mono or stereo) into a completed
// WavPack block. It is assumed that there is sufficient space for the
// completed block at "wps->blockbuff" and that "wps->blockend" points to the
// end of the available space. A return value of FALSE indicates an error.
// Any unsent metadata is transmitted first, then required metadata for this
// block is sent, and finally the compressed integer data is sent. If a "wpx"
// stream is required for floating point data or large integer data, then this
// must be handled outside this function. To find out how much data was written
// the caller must look at the ckSize field of the written WavpackHeader, NOT
// the one in the WavpackStream.

int pack_start_block (WavpackContext *wpc)
{
    WavpackStream *wps = &wpc->stream;
    WavpackMetadata wpmd;

    memcpy (wps->blockbuff, &wps->wphdr, sizeof (WavpackHeader));

    ((WavpackHeader *) wps->blockbuff)->ckSize = sizeof (WavpackHeader) - 8;
    ((WavpackHeader *) wps->blockbuff)->block_index = wps->sample_index;
    ((WavpackHeader *) wps->blockbuff)->block_samples = 0;
    ((WavpackHeader *) wps->blockbuff)->crc = 0xffffffff;

    if (wpc->wrapper_bytes) {
        wpmd.id = ID_RIFF_HEADER;
        wpmd.byte_length = wpc->wrapper_bytes;
        wpmd.data = wpc->wrapper_data;
        copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
        free_metadata (&wpmd);
        wpc->wrapper_data = NULL;
        wpc->wrapper_bytes = 0;
    }

    write_decorr_terms (wps, &wpmd);
    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
    free_metadata (&wpmd);

    write_decorr_weights (wps, &wpmd);
    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
    free_metadata (&wpmd);

    write_decorr_samples (wps, &wpmd);
    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
    free_metadata (&wpmd);

    write_entropy_vars (wps, &wpmd);
    copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
    free_metadata (&wpmd);

    if ((wps->wphdr.flags & INITIAL_BLOCK) && !wps->sample_index) {
        write_config_info (wpc, &wpmd);
        copy_metadata (&wpmd, wps->blockbuff, wps->blockend);
        free_metadata (&wpmd);
    }

    bs_open_write (&wps->wvbits, wps->blockbuff + ((WavpackHeader *) wps->blockbuff)->ckSize + 12, wps->blockend);

    return TRUE;
}

static void decorr_stereo_pass (struct decorr_pass *dpp, long *bptr, long *eptr, int m);
static void decorr_stereo_pass_18 (struct decorr_pass *dpp, long *bptr, long *eptr);
static void decorr_stereo_pass_17 (struct decorr_pass *dpp, long *bptr, long *eptr);
static void decorr_stereo_pass_m2 (struct decorr_pass *dpp, long *bptr, long *eptr);

int pack_samples (WavpackContext *wpc, long *buffer, ulong sample_count)
{
    WavpackStream *wps = &wpc->stream;
    ulong flags = wps->wphdr.flags;
    struct decorr_pass *dpp;
    long *bptr, *eptr;
    int tcount, m;
    ulong crc;

    if (!sample_count)
        return TRUE;

    eptr = buffer + sample_count * ((flags & MONO_FLAG) ? 1 : 2);
    m = ((WavpackHeader *) wps->blockbuff)->block_samples & (MAX_TERM - 1);
    crc = ((WavpackHeader *) wps->blockbuff)->crc;

    /////////////////////// handle lossless mono mode /////////////////////////

    if (!(flags & HYBRID_FLAG) && (flags & MONO_FLAG))
        for (bptr = buffer; bptr < eptr;) {
            long code;

            crc = crc * 3 + (code = *bptr);

            for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
                long sam;

                if (dpp->term > MAX_TERM) {
                    if (dpp->term & 1)
                        sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
                    else
                        sam = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;

                    dpp->samples_A [1] = dpp->samples_A [0];
                    dpp->samples_A [0] = code;
                }
                else {
                    sam = dpp->samples_A [m];
                    dpp->samples_A [(m + dpp->term) & (MAX_TERM - 1)] = code;
                }

                code -= apply_weight_i (dpp->weight_A, sam);
                update_weight (dpp->weight_A, 2, sam, code);
            }

            m = (m + 1) & (MAX_TERM - 1);
            *bptr++ = code;
        }

    //////////////////// handle the lossless stereo mode //////////////////////

    else if (!(flags & HYBRID_FLAG) && !(flags & MONO_FLAG)) {
        if (flags & JOINT_STEREO)
            for (bptr = buffer; bptr < eptr; bptr += 2) {
                crc = crc * 9 + (bptr [0] * 3) + bptr [1];
                bptr [1] += ((bptr [0] -= bptr [1]) >> 1);
            }
        else
            for (bptr = buffer; bptr < eptr; bptr += 2)
                crc = crc * 9 + (bptr [0] * 3) + bptr [1];

        for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount-- ; dpp++) {
            if (dpp->term == 17)
                decorr_stereo_pass_17 (dpp, buffer, eptr);
            else if (dpp->term == 18)
                decorr_stereo_pass_18 (dpp, buffer, eptr);
            else if (dpp->term >= 1 && dpp->term <= 7)
                decorr_stereo_pass (dpp, buffer, eptr, m);
            else if (dpp->term == -2)
                decorr_stereo_pass_m2 (dpp, buffer, eptr);
        }
    }

    send_words (buffer, sample_count, flags, &wps->w, &wps->wvbits);
    ((WavpackHeader *) wps->blockbuff)->crc = crc;
    ((WavpackHeader *) wps->blockbuff)->block_samples += sample_count;
    wps->sample_index += sample_count;

    return TRUE;
}

static void decorr_stereo_pass (struct decorr_pass *dpp, long *bptr, long *eptr, int m)
{
    int k = (m + dpp->term) & (MAX_TERM - 1);
    long sam;

    while (bptr < eptr) {
        dpp->samples_A [k] = bptr [0];
        bptr [0] -= apply_weight_i (dpp->weight_A, (sam = dpp->samples_A [m]));
        update_weight (dpp->weight_A, 2, sam, bptr [0]);
        bptr++;
        dpp->samples_B [k] = bptr [0];
        bptr [0] -= apply_weight_i (dpp->weight_B, (sam = dpp->samples_B [m]));
        update_weight (dpp->weight_B, 2, sam, bptr [0]);
        bptr++;
        m = (m + 1) & (MAX_TERM - 1);
        k = (k + 1) & (MAX_TERM - 1);
    }
}

static void decorr_stereo_pass_18 (struct decorr_pass *dpp, long *bptr, long *eptr)
{
    long sam;

    while (bptr < eptr) {
        sam = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
        dpp->samples_A [1] = dpp->samples_A [0];
        dpp->samples_A [0] = bptr [0];
        bptr [0] -= apply_weight_i (dpp->weight_A, sam);
        update_weight (dpp->weight_A, 2, sam, bptr [0]);
        bptr++;
        sam = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
        dpp->samples_B [1] = dpp->samples_B [0];
        dpp->samples_B [0] = bptr [0];
        bptr [0] -= apply_weight_i (dpp->weight_B, sam);
        update_weight (dpp->weight_B, 2, sam, bptr [0]);
        bptr++;
    }
}

static void decorr_stereo_pass_m2 (struct decorr_pass *dpp, long *bptr, long *eptr)
{
    long sam_A, sam_B;

    for (; bptr < eptr; bptr += 2) {
        sam_A = bptr [1];
        sam_B = dpp->samples_B [0];
        dpp->samples_B [0] = bptr [0];
        bptr [0] -= apply_weight_i (dpp->weight_A, sam_A);
        update_weight_clip (dpp->weight_A, 2, sam_A, bptr [0]);
        bptr [1] -= apply_weight_i (dpp->weight_B, sam_B);
        update_weight_clip (dpp->weight_B, 2, sam_B, bptr [1]);
    }
}

static void decorr_stereo_pass_17 (struct decorr_pass *dpp, long *bptr, long *eptr)
{
    long sam;

    while (bptr < eptr) {
		sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
        dpp->samples_A [1] = dpp->samples_A [0];
        dpp->samples_A [0] = bptr [0];
        bptr [0] -= apply_weight_i (dpp->weight_A, sam);
        update_weight (dpp->weight_A, 2, sam, bptr [0]);
        bptr++;
		sam = 2 * dpp->samples_B [0] - dpp->samples_B [1];
        dpp->samples_B [1] = dpp->samples_B [0];
        dpp->samples_B [0] = bptr [0];
        bptr [0] -= apply_weight_i (dpp->weight_B, sam);
        update_weight (dpp->weight_B, 2, sam, bptr [0]);
        bptr++;
    }
}

int pack_finish_block (WavpackContext *wpc)
{
    WavpackStream *wps = &wpc->stream;
    struct decorr_pass *dpp;
    ulong data_count;
    int tcount, m;

    m = ((WavpackHeader *) wps->blockbuff)->block_samples & (MAX_TERM - 1);

    if (m)
        for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
            if (dpp->term > 0 && dpp->term <= MAX_TERM) {
                long temp_A [MAX_TERM], temp_B [MAX_TERM];
                int k;

                memcpy (temp_A, dpp->samples_A, sizeof (dpp->samples_A));
                memcpy (temp_B, dpp->samples_B, sizeof (dpp->samples_B));

                for (k = 0; k < MAX_TERM; k++) {
                    dpp->samples_A [k] = temp_A [m];
                    dpp->samples_B [k] = temp_B [m];
                    m = (m + 1) & (MAX_TERM - 1);
                }
            }

    flush_word (&wps->w, &wps->wvbits);
    data_count = bs_close_write (&wps->wvbits);

    if (data_count) {
        if (data_count != (ulong) -1) {
            uchar *cptr = wps->blockbuff + ((WavpackHeader *) wps->blockbuff)->ckSize + 8;

            *cptr++ = ID_WV_BITSTREAM | ID_LARGE;
            *cptr++ = data_count >> 1;
            *cptr++ = data_count >> 9;
            *cptr++ = data_count >> 17;
            ((WavpackHeader *) wps->blockbuff)->ckSize += data_count + 4;
        }
        else
            return FALSE;
    }

    return TRUE;
}
