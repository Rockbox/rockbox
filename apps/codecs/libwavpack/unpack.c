////////////////////////////////////////////////////////////////////////////
//			     **** WAVPACK ****				  //
//		    Hybrid Lossless Wavefile Compressor			  //
//		Copyright (c) 1998 - 2004 Conifer Software.		  //
//			    All Rights Reserved.			  //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// unpack.c

// This module actually handles the decompression of the audio data, except
// for the entropy decoding which is handled by the words.c module. For
// maximum efficiency, the conversion is isolated to tight loops that handle
// an entire buffer.

#include "wavpack.h"

#include <string.h>
#include <math.h>

#define LOSSY_MUTE

//////////////////////////////// local macros /////////////////////////////////

#define apply_weight_i(weight, sample) ((weight * sample + 512) >> 10)

#define apply_weight_f(weight, sample) (((((sample & 0xffff) * weight) >> 9) + \
    (((sample & ~0xffff) >> 9) * weight) + 1) >> 1)

#define apply_weight(weight, sample) (sample != (short) sample ? \
    apply_weight_f (weight, sample) : apply_weight_i (weight, sample))

#define update_weight(weight, delta, source, result) \
    if (source && result) weight -= ((((source ^ result) >> 30) & 2) - 1) * delta;

#define update_weight_clip(weight, delta, source, result) \
    if (source && result && ((source ^ result) < 0 ? (weight -= delta) < -1024 : (weight += delta) > 1024)) \
	weight = weight < 0 ? -1024 : 1024;

///////////////////////////// executable code ////////////////////////////////

// This function initializes everything required to unpack a WavPack block
// and must be called before unpack_samples() is called to obtain audio data.
// It is assumed that the WavpackHeader has been read into the wps->wphdr
// (in the current WavpackStream). This is where all the metadata blocks are
// scanned up to the one containing the audio bitstream.

int unpack_init (WavpackContext *wpc)
{
    WavpackStream *wps = &wpc->stream;
    WavpackMetadata wpmd;

    if (wps->wphdr.block_samples && wps->wphdr.block_index != (ulong) -1)
	wps->sample_index = wps->wphdr.block_index;

    wps->mute_error = FALSE;
    wps->crc = 0xffffffff;
    CLEAR (wps->wvbits);
    CLEAR (wps->decorr_passes);
    CLEAR (wps->w);

    while (read_metadata_buff (wpc, &wpmd)) {
	if (!process_metadata (wpc, &wpmd)) {
	    strcpy (wpc->error_message, "invalid metadata!");
	    return FALSE;
	}

	if (wpmd.id == ID_WV_BITSTREAM)
	    break;
    }

    if (wps->wphdr.block_samples && !bs_is_open (&wps->wvbits)) {
	strcpy (wpc->error_message, "invalid WavPack file!");
	return FALSE;
    }

    if (wps->wphdr.block_samples) {
	if ((wps->wphdr.flags & INT32_DATA) && wps->int32_sent_bits)
	    wpc->lossy_blocks = TRUE;

	if ((wps->wphdr.flags & FLOAT_DATA) &&
	    wps->float_flags & (FLOAT_EXCEPTIONS | FLOAT_ZEROS_SENT | FLOAT_SHIFT_SENT | FLOAT_SHIFT_SAME))
		wpc->lossy_blocks = TRUE;
    }

    return TRUE;
}

// This function initialzes the main bitstream for audio samples, which must
// be in the "wv" file.

int init_wv_bitstream (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    WavpackStream *wps = &wpc->stream;

    if (wpmd->data)
	bs_open_read (&wps->wvbits, wpmd->data, (char *) wpmd->data + wpmd->byte_length, NULL, 0);
    else if (wpmd->byte_length)
	bs_open_read (&wps->wvbits, wpc->read_buffer, wpc->read_buffer + sizeof (wpc->read_buffer),
	    wpc->infile, wpmd->byte_length + (wpmd->byte_length & 1));

    return TRUE;
}

// Read decorrelation terms from specified metadata block into the
// decorr_passes array. The terms range from -3 to 8, plus 17 & 18;
// other values are reserved and generate errors for now. The delta
// ranges from 0 to 7 with all values valid. Note that the terms are
// stored in the opposite order in the decorr_passes array compared
// to packing.

int read_decorr_terms (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int termcnt = wpmd->byte_length;
    uchar *byteptr = wpmd->data;
    struct decorr_pass *dpp;

    if (termcnt > MAX_NTERMS)
	return FALSE;

    wps->num_terms = termcnt;

    for (dpp = wps->decorr_passes + termcnt - 1; termcnt--; dpp--) {
	dpp->term = (int)(*byteptr & 0x1f) - 5;
	dpp->delta = (*byteptr++ >> 5) & 0x7;

	if (!dpp->term || dpp->term < -3 || (dpp->term > MAX_TERM && dpp->term < 17) || dpp->term > 18)
	    return FALSE;
    }

    return TRUE;
}

// Read decorrelation weights from specified metadata block into the
// decorr_passes array. The weights range +/-1024, but are rounded and
// truncated to fit in signed chars for metadata storage. Weights are
// separate for the two channels and are specified from the "last" term
// (first during encode). Unspecified weights are set to zero.

int read_decorr_weights (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int termcnt = wpmd->byte_length, tcount;
    char *byteptr = wpmd->data;
    struct decorr_pass *dpp;

    if (!(wps->wphdr.flags & MONO_FLAG))
	termcnt /= 2;

    if (termcnt > wps->num_terms)
	return FALSE;

    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
	dpp->weight_A = dpp->weight_B = 0;

    while (--dpp >= wps->decorr_passes && termcnt--) {
	dpp->weight_A = restore_weight (*byteptr++);

	if (!(wps->wphdr.flags & MONO_FLAG))
	    dpp->weight_B = restore_weight (*byteptr++);
    }

    return TRUE;
}

// Read decorrelation samples from specified metadata block into the
// decorr_passes array. The samples are signed 32-bit values, but are
// converted to signed log2 values for storage in metadata. Values are
// stored for both channels and are specified from the "last" term
// (first during encode) with unspecified samples set to zero. The
// number of samples stored varies with the actual term value, so
// those must obviously come first in the metadata.

int read_decorr_samples (WavpackStream *wps, WavpackMetadata *wpmd)
{
    uchar *byteptr = wpmd->data;
    uchar *endptr = byteptr + wpmd->byte_length;
    struct decorr_pass *dpp;
    int tcount;

    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
	CLEAR (dpp->samples_A);
	CLEAR (dpp->samples_B);
    }

    if (wps->wphdr.version == 0x402 && (wps->wphdr.flags & HYBRID_FLAG)) {
	byteptr += 2;

	if (!(wps->wphdr.flags & MONO_FLAG))
	    byteptr += 2;
    }

    while (dpp-- > wps->decorr_passes && byteptr < endptr)
	if (dpp->term > MAX_TERM) {
	    dpp->samples_A [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	    dpp->samples_A [1] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
	    byteptr += 4;

	    if (!(wps->wphdr.flags & MONO_FLAG)) {
		dpp->samples_B [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
		dpp->samples_B [1] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
		byteptr += 4;
	    }
	}
	else if (dpp->term < 0) {
	    dpp->samples_A [0] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
	    dpp->samples_B [0] = exp2s ((short)(byteptr [2] + (byteptr [3] << 8)));
	    byteptr += 4;
	}
	else {
	    int m = 0, cnt = dpp->term;

	    while (cnt--) {
		dpp->samples_A [m] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
		byteptr += 2;

		if (!(wps->wphdr.flags & MONO_FLAG)) {
		    dpp->samples_B [m] = exp2s ((short)(byteptr [0] + (byteptr [1] << 8)));
		    byteptr += 2;
		}

		m++;
	    }
	}

    return byteptr == endptr;
}

// Read the int32 data from the specified metadata into the specified stream.
// This data is used for integer data that has more than 24 bits of magnitude
// or, in some cases, used to eliminate redundant bits from any audio stream.

int read_int32_info (WavpackStream *wps, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    char *byteptr = wpmd->data;

    if (bytecnt != 4)
	return FALSE;

    wps->int32_sent_bits = *byteptr++;
    wps->int32_zeros = *byteptr++;
    wps->int32_ones = *byteptr++;
    wps->int32_dups = *byteptr;
    return TRUE;
}

// Read multichannel information from metadata. The first byte is the total
// number of channels and the following bytes represent the channel_mask
// as described for Microsoft WAVEFORMATEX.

int read_channel_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length, shift = 0;
    char *byteptr = wpmd->data;
    ulong mask = 0;

    if (!bytecnt || bytecnt > 5)
	return FALSE;

    wpc->config.num_channels = *byteptr++;

    while (--bytecnt) {
	mask |= (ulong) *byteptr++ << shift;
	shift += 8;
    }

    wpc->config.channel_mask = mask;
    return TRUE;
}

// Read configuration information from metadata.

int read_config_info (WavpackContext *wpc, WavpackMetadata *wpmd)
{
    int bytecnt = wpmd->byte_length;
    uchar *byteptr = wpmd->data;

    if (bytecnt >= 3) {
	wpc->config.flags &= 0xff;
	wpc->config.flags |= (long) *byteptr++ << 8;
	wpc->config.flags |= (long) *byteptr++ << 16;
	wpc->config.flags |= (long) *byteptr << 24;
    }

    return TRUE;
}

// This monster actually unpacks the WavPack bitstream(s) into the specified
// buffer as 32-bit integers or floats (depending on orignal data). Lossy
// samples will be clipped to their original limits (i.e. 8-bit samples are
// clipped to -128/+127) but are still returned in longs. It is up to the
// caller to potentially reformat this for the final output including any
// multichannel distribution, block alignment or endian compensation. The
// function unpack_init() must have been called and the entire WavPack block
// must still be visible (although wps->blockbuff will not be accessed again).
// For maximum clarity, the function is broken up into segments that handle
// various modes. This makes for a few extra infrequent flag checks, but
// makes the code easier to follow because the nesting does not become so
// deep. For maximum efficiency, the conversion is isolated to tight loops
// that handle an entire buffer. The function returns the total number of
// samples unpacked, which can be less than the number requested if an error
// occurs or the end of the block is reached.

static void fixup_samples (WavpackStream *wps, long *buffer, ulong sample_count);

long unpack_samples (WavpackContext *wpc, long *buffer, ulong sample_count)
{
    WavpackStream *wps = &wpc->stream;
    ulong flags = wps->wphdr.flags, crc = wps->crc, i;
    long mute_limit = (1L << ((flags & MAG_MASK) >> MAG_LSB)) + 2;
    struct decorr_pass *dpp;
    long read_word, *bptr;
    int tcount, m = 0;

    if (wps->sample_index + sample_count > wps->wphdr.block_index + wps->wphdr.block_samples)
	sample_count = wps->wphdr.block_index + wps->wphdr.block_samples - wps->sample_index;

    if (wps->mute_error) {
	memset (buffer, 0, sample_count * (flags & MONO_FLAG ? 4 : 8));
	wps->sample_index += sample_count;
	return sample_count;
    }

    if (flags & HYBRID_FLAG)
	mute_limit *= 2;

    ///////////////////// handle version 4 mono data /////////////////////////

    if (flags & MONO_FLAG)
	for (bptr = buffer, i = 0; i < sample_count; ++i) {
	    if ((read_word = get_word (wps, 0)) == WORD_EOF)
		break;

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++) {
		long sam, temp;
		int k;

		if (dpp->term > MAX_TERM) {
		    if (dpp->term & 1)
			sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
		    else
			sam = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;

		    dpp->samples_A [1] = dpp->samples_A [0];
		    k = 0;
		}
		else {
		    sam = dpp->samples_A [m];
		    k = (m + dpp->term) & (MAX_TERM - 1);
		}

		temp = apply_weight (dpp->weight_A, sam) + read_word;
		update_weight (dpp->weight_A, dpp->delta, sam, read_word);
		dpp->samples_A [k] = read_word = temp;
	    }

	    if (labs (read_word) > mute_limit)
		break;

	    m = (m + 1) & (MAX_TERM - 1);
	    crc = crc * 3 + read_word;
	    *bptr++ = read_word;
	}

    //////////////////// handle version 4 stereo data ////////////////////////

    else
	for (bptr = buffer, i = 0; i < sample_count; ++i) {
	    long left, right, left2, right2;

	    if ((left = get_word (wps, 0)) == WORD_EOF ||
		(right = get_word (wps, 1)) == WORD_EOF)
		    break;

	    for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
		if (dpp->term > 0) {
		    long sam_A, sam_B;
		    int k;

		    if (dpp->term > MAX_TERM) {
			if (dpp->term & 1) {
			    sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
			    sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
			}
			else {
			    sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
			    sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
			}

			dpp->samples_A [1] = dpp->samples_A [0];
			dpp->samples_B [1] = dpp->samples_B [0];
			k = 0;
		    }
		    else {
			sam_A = dpp->samples_A [m];
			sam_B = dpp->samples_B [m];
			k = (m + dpp->term) & (MAX_TERM - 1);
		    }

		    left2 = apply_weight (dpp->weight_A, sam_A) + left;
		    right2 = apply_weight (dpp->weight_B, sam_B) + right;

		    update_weight (dpp->weight_A, dpp->delta, sam_A, left);
		    update_weight (dpp->weight_B, dpp->delta, sam_B, right);

		    dpp->samples_A [k] = left = left2;
		    dpp->samples_B [k] = right = right2;
		}
		else if (dpp->term == -1) {
		    left2 = left + apply_weight (dpp->weight_A, dpp->samples_A [0]);
		    update_weight_clip (dpp->weight_A, dpp->delta, dpp->samples_A [0], left);
		    left = left2;
		    right2 = right + apply_weight (dpp->weight_B, left2);
		    update_weight_clip (dpp->weight_B, dpp->delta, left2, right);
		    dpp->samples_A [0] = right = right2;
		}
		else {
		    right2 = right + apply_weight (dpp->weight_B, dpp->samples_B [0]);
		    update_weight_clip (dpp->weight_B, dpp->delta, dpp->samples_B [0], right);
		    right = right2;

		    if (dpp->term == -3) {
			right2 = dpp->samples_A [0];
			dpp->samples_A [0] = right;
		    }

		    left2 = left + apply_weight (dpp->weight_A, right2);
		    update_weight_clip (dpp->weight_A, dpp->delta, right2, left);
		    dpp->samples_B [0] = left = left2;
		}

	    m = (m + 1) & (MAX_TERM - 1);

	    if (flags & JOINT_STEREO)
		left += (right -= (left >> 1));

	    if (labs (left) > mute_limit || labs (right) > mute_limit)
		break;

	    crc = (crc * 3 + left) * 3 + right;
	    *bptr++ = left;
	    *bptr++ = right;
	}

    if (i != sample_count) {
	memset (buffer, 0, sample_count * (flags & MONO_FLAG ? 4 : 8));
	wps->mute_error = TRUE;
	i = sample_count;
    }

    while (m--)
	for (tcount = wps->num_terms, dpp = wps->decorr_passes; tcount--; dpp++)
	    if (dpp->term > 0 && dpp->term <= MAX_TERM) {
		long temp = dpp->samples_A [0];
		memcpy (dpp->samples_A, dpp->samples_A + 1, sizeof (dpp->samples_A) - sizeof (dpp->samples_A [0]));
		dpp->samples_A [MAX_TERM - 1] = temp;
		temp = dpp->samples_B [0];
		memcpy (dpp->samples_B, dpp->samples_B + 1, sizeof (dpp->samples_B) - sizeof (dpp->samples_B [0]));
		dpp->samples_B [MAX_TERM - 1] = temp;
	    }

    fixup_samples (wps, buffer, i);

    if (flags & FLOAT_DATA)
	float_normalize (buffer, (flags & MONO_FLAG) ? i : i * 2,
	    127 - wps->float_norm_exp + wpc->norm_offset);

    wps->sample_index += i;
    wps->crc = crc;

    return i;
}

// This is a helper function for unpack_samples() that applies several final
// operations. First, if the data is 32-bit float data, then that conversion
// is done in the float.c module (whether lossy or lossless) and we return.
// Otherwise, if the extended integer data applies, then that operation is
// executed first. If the unpacked data is lossy (and not corrected) then
// it is clipped and shifted in a single operation. Otherwise, if it's
// lossless then the last step is to apply the final shift (if any).

static void fixup_samples (WavpackStream *wps, long *buffer, ulong sample_count)
{
    ulong flags = wps->wphdr.flags;
    int shift = (flags & SHIFT_MASK) >> SHIFT_LSB;

    if (flags & FLOAT_DATA) {
	float_values (wps, buffer, (flags & MONO_FLAG) ? sample_count : sample_count * 2);
	return;
    }

    if (flags & INT32_DATA) {
	ulong count = (flags & MONO_FLAG) ? sample_count : sample_count * 2;
	int sent_bits = wps->int32_sent_bits, zeros = wps->int32_zeros;
	int ones = wps->int32_ones, dups = wps->int32_dups;
//	ulong mask = (1 << sent_bits) - 1;
	long *dptr = buffer;

	if (!(flags & HYBRID_FLAG) && !sent_bits && (zeros + ones + dups))
	    while (count--) {
		if (zeros)
		    *dptr <<= zeros;
		else if (ones)
		    *dptr = ((*dptr + 1) << ones) - 1;
		else if (dups)
		    *dptr = ((*dptr + (*dptr & 1)) << dups) - (*dptr & 1);

		dptr++;
	    }
	else
	    shift += zeros + sent_bits + ones + dups;
    }

    if (flags & HYBRID_FLAG) {
	long min_value, max_value, min_shifted, max_shifted;

	switch (flags & BYTES_STORED) {
	    case 0:
		min_shifted = (min_value = -128 >> shift) << shift;
		max_shifted = (max_value = 127 >> shift) << shift;
		break;

	    case 1:
		min_shifted = (min_value = -32768 >> shift) << shift;
		max_shifted = (max_value = 32767 >> shift) << shift;
		break;

	    case 2:
		min_shifted = (min_value = -8388608 >> shift) << shift;
		max_shifted = (max_value = 8388607 >> shift) << shift;
		break;

	    case 3:
		min_shifted = (min_value = -(long)2147483648 >> shift) << shift;
		max_shifted = (max_value = (long) 2147483647 >> shift) << shift;
		break;
	}

	if (!(flags & MONO_FLAG))
	    sample_count *= 2;

	while (sample_count--) {
	    if (*buffer < min_value)
		*buffer++ = min_shifted;
	    else if (*buffer > max_value)
		*buffer++ = max_shifted;
	    else
		*buffer++ <<= shift;
	}
    }
    else if (shift) {
	if (!(flags & MONO_FLAG))
	    sample_count *= 2;

	while (sample_count--)
	    *buffer++ <<= shift;
    }
}

// This function checks the crc value(s) for an unpacked block, returning the
// number of actual crc errors detected for the block. The block must be
// completely unpacked before this test is valid. For losslessly unpacked
// blocks of float or extended integer data the extended crc is also checked.
// Note that WavPack's crc is not a CCITT approved polynomial algorithm, but
// is a much simpler method that is virtually as robust for real world data.

int check_crc_error (WavpackContext *wpc)
{
    WavpackStream *wps = &wpc->stream;
    int result = 0;

    if (wps->crc != wps->wphdr.crc)
	++result;

    return result;
}
