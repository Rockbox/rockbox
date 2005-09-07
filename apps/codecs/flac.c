/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codec.h"

#include <codecs/libFLAC/include/FLAC/seekable_stream_decoder.h>
#include <codecs/libFLAC/include/FLAC/format.h>
#include <codecs/libFLAC/include/FLAC/metadata.h>
#include "playback.h"
#include "lib/codeclib.h"
#include "dsp.h"

#define FLAC_MAX_SUPPORTED_BLOCKSIZE 4608
#define FLAC_MAX_SUPPORTED_CHANNELS  2

static uint32_t samplesdone;

static FLAC__StreamMetadata *stream_info;
static FLAC__StreamMetadata *seek_table;
unsigned int metadata_length;

/* Called when the FLAC decoder needs some FLAC data to decode */
FLAC__SeekableStreamDecoderReadStatus flac_read_handler(const FLAC__SeekableStreamDecoder *dec, 
                                                        FLAC__byte buffer[], unsigned *bytes, void *data)
{   
    struct codec_api* ci = (struct codec_api*)data;
    (void)dec;

    *bytes=(unsigned)(ci->read_filebuf(buffer,*bytes));

    if (*bytes==0) {
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    } else {
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
}

static unsigned char pcmbuf[FLAC_MAX_SUPPORTED_BLOCKSIZE*FLAC_MAX_SUPPORTED_CHANNELS*2] IDATA_ATTR;

/* Called when the FLAC decoder has some decoded PCM data to write */
FLAC__StreamDecoderWriteStatus flac_write_handler(const FLAC__SeekableStreamDecoder *dec, 
                              const FLAC__Frame *frame, 
                              const FLAC__int32 * const buf[], 
                              void *data)
{
    struct codec_api* ci = (struct codec_api*)data;
    (void)dec;
    unsigned int c_samp, c_chan, d_samp;
    uint32_t data_size = frame->header.blocksize * frame->header.channels * 2; /* Assume 16-bit words */
    uint32_t samples = frame->header.blocksize;
    int yieldcounter = 0;


    if (samples*frame->header.channels > (FLAC_MAX_SUPPORTED_BLOCKSIZE*FLAC_MAX_SUPPORTED_CHANNELS)) {
        // ERROR!!!
        // DEBUGF("ERROR: samples*frame->header.channels=%d\n",samples*frame->header.channels);
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    (void)dec;
    for (c_samp = d_samp = 0; c_samp < samples; c_samp++) {
        for(c_chan = 0; c_chan < frame->header.channels; c_chan++, d_samp++) {
            pcmbuf[d_samp*2] = (buf[c_chan][c_samp]&0xff00)>>8;
            pcmbuf[(d_samp*2)+1] = buf[c_chan][c_samp]&0xff;
            if (yieldcounter++ == 100) {
                ci->yield();
                yieldcounter = 0;
            }
        }
    }

    samplesdone+=samples;
    ci->set_elapsed(samplesdone/(ci->id3->frequency/1000));
   
    ci->yield();
    while (!ci->pcmbuf_insert(pcmbuf, data_size))
        ci->yield();

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void flac_metadata_handler(const FLAC__SeekableStreamDecoder *dec, 
                           const FLAC__StreamMetadata *meta, void *data)
{
    /* Ignore metadata for now... */
    (void)dec;
    (void)data;

    metadata_length += meta->length;

    if ( meta->type == FLAC__METADATA_TYPE_STREAMINFO ) {
        stream_info = FLAC__metadata_object_clone( meta );
        if ( stream_info == NULL ) {
            //return CODEC_ERROR;
        }
    } else if ( meta->type == FLAC__METADATA_TYPE_SEEKTABLE ) {
        seek_table = FLAC__metadata_object_clone( meta );
        if ( seek_table == NULL ) {
            //return CODEC_ERROR;
        }
    }
}


void flac_error_handler(const FLAC__SeekableStreamDecoder *dec, 
                        FLAC__StreamDecoderErrorStatus status, void *data)
{
    (void)dec;
    (void)status;
    (void)data;
}

FLAC__SeekableStreamDecoderSeekStatus flac_seek_handler(const FLAC__SeekableStreamDecoder *decoder, 
                                                        FLAC__uint64 absolute_byte_offset,
                                                        void *client_data)
{
    (void)decoder;
    struct codec_api* ci = (struct codec_api*)client_data;

    if (ci->seek_buffer(absolute_byte_offset)) {
        return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
    } else {
        return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
    }
}

FLAC__SeekableStreamDecoderTellStatus flac_tell_handler(const FLAC__SeekableStreamDecoder *decoder, 
                                                        FLAC__uint64 *absolute_byte_offset, void *client_data)
{ 
    struct codec_api* ci = (struct codec_api*)client_data;

    (void)decoder;
    *absolute_byte_offset = ci->curpos;
    return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__SeekableStreamDecoderLengthStatus flac_length_handler(const FLAC__SeekableStreamDecoder *decoder, 
                                                            FLAC__uint64 *stream_length, void *client_data)
{
    struct codec_api* ci = (struct codec_api*)client_data;

    (void)decoder;
    *stream_length = ci->filesize;
    return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool flac_eof_handler(const FLAC__SeekableStreamDecoder *decoder, 
                            void *client_data)
{
    struct codec_api* ci = (struct codec_api*)client_data;

    (void)decoder;
    if (ci->curpos >= ci->filesize) {
        return true;
    } else {
        return false;
    }
}

#ifndef SIMULATOR
extern char iramcopy[];
extern char iramstart[];
extern char iramend[];
#endif

FLAC__uint64 find_sample_number(struct codec_api *ci, size_t offset)
{
    FLAC__StreamMetadata_SeekPoint *points;
    FLAC__uint64 prev_sample, next_sample;
    size_t prev_offset, next_offset;
    int percent;

    if (offset >= (ci->id3->filesize - metadata_length)) {
        return stream_info->data.stream_info.total_samples;
    }

    prev_offset = 0;
    prev_sample = 0;
    next_offset = ci->id3->filesize - metadata_length;
    next_sample = stream_info->data.stream_info.total_samples;

    if (seek_table) {
        int left, right, middle;

	    middle = 0; /* Silence compiler warnings */
        points = seek_table->data.seek_table.points;
        left = 0;
        right = seek_table->data.seek_table.num_points - 1;

        /* Do a binary search to find the matching seek point */
        while (left <= right) {
            middle = (left + right) / 2;

            if ((FLAC__uint64)offset < points[middle].stream_offset) {
                right = middle - 1;
            } else if ((FLAC__uint64)offset > points[middle].stream_offset) {
                left = middle + 1;
            } else {
                return points[middle].sample_number;
            }
        }

        /* Didn't find a matching seek point, so get the sample numbers of the
         * seek points to the left and right of offset to make our guess more
         * accurate.  Accuracy depends on how close these sample numbers are to
         * each other.
         */
        if ((unsigned)left >= seek_table->data.seek_table.num_points) {
            prev_offset = points[middle].stream_offset;
            prev_sample = points[middle].sample_number;
        } else if (right < 0) {
            next_offset = points[middle].stream_offset;
            next_sample = points[middle].sample_number;
        } else {
            middle--;
            prev_offset = points[middle].stream_offset;
            prev_sample = points[middle].sample_number;
            next_offset = points[middle+1].stream_offset;
            next_sample = points[middle+1].sample_number;
        }
    }

    /* Either there's no seek table or we didn't find our seek point, so now we
     * have to guess.
     */
    percent = ((offset - prev_offset) * 100) / (next_offset - prev_offset);
    return (FLAC__uint64)(percent * (next_sample - prev_sample) / 100 + prev_sample);
}

/* this is the codec entry point */
enum codec_status codec_start(struct codec_api* api)
{
    struct codec_api* ci = api;
    FLAC__SeekableStreamDecoder* flacDecoder;
    FLAC__uint64 offset;

    TEST_CODEC_API(ci);

#ifndef SIMULATOR
    ci->memcpy(iramstart, iramcopy, iramend-iramstart);
#endif

    ci->configure(CODEC_SET_FILEBUF_LIMIT, (int *)(1024*1024*10));
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, (int *)(1024*512));
    ci->configure(CODEC_SET_FILEBUF_CHUNKSIZE, (int *)(1024*1024));

    ci->configure(CODEC_DSP_ENABLE, (bool *)true);
    ci->configure(DSP_DITHER, (bool *)false);
    ci->configure(DSP_SET_STEREO_MODE, (int *)STEREO_INTERLEAVED);
    ci->configure(DSP_SET_SAMPLE_DEPTH, (int *)(16));
  
next_track:
    metadata_length = 0;
    seek_table = NULL;
    stream_info = NULL;

    if (codec_init(api)) {
        return CODEC_ERROR;
    }

    while (!ci->taginfo_ready)
        ci->yield();
    
    ci->configure(DSP_SET_FREQUENCY, (long *)(ci->id3->frequency));
    codec_set_replaygain(ci->id3);
  
    /* Create a decoder instance */
    flacDecoder = FLAC__seekable_stream_decoder_new();

    /* Set up the decoder and the callback functions - this must be done before init */

    /* The following are required for stream_decoder and higher */
    FLAC__seekable_stream_decoder_set_client_data(flacDecoder,ci);
    FLAC__seekable_stream_decoder_set_write_callback(flacDecoder, flac_write_handler);
    FLAC__seekable_stream_decoder_set_read_callback(flacDecoder, flac_read_handler);
    FLAC__seekable_stream_decoder_set_metadata_callback(flacDecoder, flac_metadata_handler);
    FLAC__seekable_stream_decoder_set_error_callback(flacDecoder, flac_error_handler);
    FLAC__seekable_stream_decoder_set_metadata_respond_all(flacDecoder);

    /* The following are only for the seekable_stream_decoder */
    FLAC__seekable_stream_decoder_set_seek_callback(flacDecoder, flac_seek_handler);
    FLAC__seekable_stream_decoder_set_tell_callback(flacDecoder, flac_tell_handler);
    FLAC__seekable_stream_decoder_set_length_callback(flacDecoder, flac_length_handler);
    FLAC__seekable_stream_decoder_set_eof_callback(flacDecoder, flac_eof_handler);


    /* QUESTION: What do we do when the init fails? */
    if (FLAC__seekable_stream_decoder_init(flacDecoder)) {
        return CODEC_ERROR;
    }

    /* The first thing to do is to parse the metadata */
    FLAC__seekable_stream_decoder_process_until_end_of_metadata(flacDecoder);

    if (ci->id3->offset && stream_info) {
        FLAC__uint64 sample;

        sample = find_sample_number(ci, ci->id3->offset - metadata_length);
        ci->advance_buffer(ci->id3->offset);
        FLAC__seekable_stream_decoder_seek_absolute(flacDecoder, sample);
        FLAC__seekable_stream_decoder_get_decode_position(flacDecoder, &offset);
        ci->set_offset(offset);
        samplesdone = (uint32_t)sample;
        ci->set_elapsed(sample/(ci->id3->frequency/1000));
    } else {
        samplesdone = 0;
        ci->set_elapsed(0);
    }

    /* The main decoder loop */
    while (FLAC__seekable_stream_decoder_get_state(flacDecoder) != FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM) {
        ci->yield();
        if (ci->stop_codec || ci->reload_codec) {
            break;
        }

        if (ci->seek_time) {
            int sample_loc;

            sample_loc = ci->seek_time/1000 * ci->id3->frequency;
            if (FLAC__seekable_stream_decoder_seek_absolute(flacDecoder, sample_loc)) {
                samplesdone = sample_loc;
                ci->set_elapsed(samplesdone/(ci->id3->frequency/1000));
            }
            ci->seek_time = 0;
        }

        FLAC__seekable_stream_decoder_process_single(flacDecoder);
        FLAC__seekable_stream_decoder_get_decode_position(flacDecoder, &offset);
        ci->set_offset(offset);
    }

    /* Flush the libFLAC buffers */
    FLAC__seekable_stream_decoder_finish(flacDecoder);

    if (ci->request_next_track()) {
        if (stream_info) {
            FLAC__metadata_object_delete(stream_info);
        }
        if (seek_table) {
            FLAC__metadata_object_delete(seek_table);
        }
        metadata_length = 0;
        goto next_track;
    }

    return CODEC_OK;
}
