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

#include "plugin.h"

#if (CONFIG_HWCODEC == MASNONE) && !defined(SIMULATOR)
/* software codec platforms, not for simulator */

#include <codecs/libFLAC/include/FLAC/seekable_stream_decoder.h>

#include "lib/xxx2wav.h" /* Helper functions common to test decoders */

#define FLAC_MAX_SUPPORTED_BLOCKSIZE 4608
#define FLAC_MAX_SUPPORTED_CHANNELS  2

static struct plugin_api* rb;

/* Called when the FLAC decoder needs some FLAC data to decode */
FLAC__SeekableStreamDecoderReadStatus flac_read_handler(const FLAC__SeekableStreamDecoder *dec, 
 FLAC__byte buffer[], unsigned *bytes, void *data)
{   (void)dec;

    file_info_struct *p = (file_info_struct *) data;

    if (p->curpos >= p->filesize) {
      return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }

    rb->memcpy(buffer,&filebuf[p->curpos],*bytes);
    p->curpos+=*bytes;
  
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

/* Called when the FLAC decoder has some decoded PCM data to write */
FLAC__StreamDecoderWriteStatus flac_write_handler(const FLAC__SeekableStreamDecoder *dec, 
  					          const FLAC__Frame *frame, 
					          const FLAC__int32 * const buf[], 
					          void *data)
{
    unsigned int c_samp, c_chan, d_samp;
    file_info_struct *p = (file_info_struct *) data;
    uint32_t data_size = frame->header.blocksize * frame->header.channels * (p->bitspersample / 8);
    uint32_t samples = frame->header.blocksize;

    // FIXME: This should not be on the stack!
    static unsigned char ldb[FLAC_MAX_SUPPORTED_BLOCKSIZE*FLAC_MAX_SUPPORTED_CHANNELS*2];

    if (samples*frame->header.channels > (FLAC_MAX_SUPPORTED_BLOCKSIZE*FLAC_MAX_SUPPORTED_CHANNELS)) {
      // ERROR!!!
#ifdef SIMULATOR
      fprintf(stderr,"ERROR: samples*frame->header.channels=%d\n",samples*frame->header.channels);
#endif
      return(FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE);
    }

    (void)dec;
    (void)data_size;
    for(c_samp = d_samp = 0; c_samp < samples; c_samp++) {
	for(c_chan = 0; c_chan < frame->header.channels; c_chan++, d_samp++) {
	    ldb[d_samp*2] = buf[c_chan][c_samp]&0xff;
	    ldb[(d_samp*2)+1] = (buf[c_chan][c_samp]&0xff00)>>8;
	}
    } 

    rb->write(p->outfile,ldb,data_size);

    p->current_sample += samples;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void flac_metadata_handler(const FLAC__SeekableStreamDecoder *dec, 
                           const FLAC__StreamMetadata *meta, void *data)
{
    file_info_struct *p = (file_info_struct *) data;
    (void)dec;

    if(meta->type == FLAC__METADATA_TYPE_STREAMINFO) {
	p->bitspersample = meta->data.stream_info.bits_per_sample;
	p->samplerate = meta->data.stream_info.sample_rate;
	p->channels = meta->data.stream_info.channels;
//	FLAC__ASSERT(meta->data.stream_info.total_samples < 0x100000000); /* we can handle < 4 gigasamples */
	p->total_samples = (unsigned) 
	    (meta->data.stream_info.total_samples & 0xffffffff);
	p->current_sample = 0;
    }
}


void flac_error_handler(const FLAC__SeekableStreamDecoder *dec, 
                        FLAC__StreamDecoderErrorStatus status, void *data)
{
    (void)dec;
    (void)status;
    (void)data;
}

FLAC__SeekableStreamDecoderSeekStatus flac_seek_handler (const FLAC__SeekableStreamDecoder *decoder, 
                                                         FLAC__uint64 absolute_byte_offset,
                                                         void *client_data)
{
  (void)decoder;
  file_info_struct *p = (file_info_struct *) client_data;
  rb->lseek(p->infile,SEEK_SET,absolute_byte_offset);
  return(FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK);
}

FLAC__SeekableStreamDecoderTellStatus flac_tell_handler (const FLAC__SeekableStreamDecoder *decoder, 
                                                         FLAC__uint64 *absolute_byte_offset, void *client_data)
{ 
  file_info_struct *p = (file_info_struct *) client_data;

  (void)decoder;
  *absolute_byte_offset=rb->lseek(p->infile,SEEK_CUR,0);
  return(FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK);
}

FLAC__SeekableStreamDecoderLengthStatus flac_length_handler (const FLAC__SeekableStreamDecoder *decoder, 
                                                         FLAC__uint64 *stream_length, void *client_data)
{
  file_info_struct *p = (file_info_struct *) client_data;

  (void)decoder;
  *stream_length=p->filesize;
  return(FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK);
}

FLAC__bool flac_eof_handler (const FLAC__SeekableStreamDecoder *decoder, 
                             void *client_data)
{
  file_info_struct *p = (file_info_struct *) client_data;

  (void)decoder;
  if (p->curpos >= p->filesize) {
    return(true);
  } else {
    return(false);
  }
}


/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* file)
{
  FLAC__SeekableStreamDecoder* flacDecoder;
  file_info_struct file_info;

  TEST_PLUGIN_API(api);

  /* if you are using a global api pointer, don't forget to copy it!
     otherwise you will get lovely "I04: IllInstr" errors... :-) */
  rb = api;


  /* This function sets up the buffers and reads the file into RAM */

  if (local_init(file,"/flactest.wav",&file_info,api)) {
    return PLUGIN_ERROR;
  }

  /* Create a decoder instance */

  flacDecoder=FLAC__seekable_stream_decoder_new();

  /* Set up the decoder and the callback functions - this must be done before init */

  /* The following are required for stream_decoder and higher */
  FLAC__seekable_stream_decoder_set_client_data(flacDecoder,&file_info);
  FLAC__seekable_stream_decoder_set_write_callback(flacDecoder, flac_write_handler);
  FLAC__seekable_stream_decoder_set_read_callback(flacDecoder, flac_read_handler);
  FLAC__seekable_stream_decoder_set_metadata_callback(flacDecoder, flac_metadata_handler);
  FLAC__seekable_stream_decoder_set_error_callback(flacDecoder, flac_error_handler);
  FLAC__seekable_stream_decoder_set_metadata_respond(flacDecoder, FLAC__METADATA_TYPE_STREAMINFO);

  /* The following are only for the seekable_stream_decoder */
  FLAC__seekable_stream_decoder_set_seek_callback(flacDecoder, flac_seek_handler);
  FLAC__seekable_stream_decoder_set_tell_callback(flacDecoder, flac_tell_handler);
  FLAC__seekable_stream_decoder_set_length_callback(flacDecoder, flac_length_handler);
  FLAC__seekable_stream_decoder_set_eof_callback(flacDecoder, flac_eof_handler);

  if (FLAC__seekable_stream_decoder_init(flacDecoder)) {
    return PLUGIN_ERROR;
  }

  /* The first thing to do is to parse the metadata */
  FLAC__seekable_stream_decoder_process_until_end_of_metadata(flacDecoder);

  file_info.frames_decoded=0;
  file_info.start_tick=*(rb->current_tick);
  rb->button_clear_queue();

  while (FLAC__seekable_stream_decoder_get_state(flacDecoder)!=2) {
    FLAC__seekable_stream_decoder_process_single(flacDecoder);
    file_info.frames_decoded++;

    display_status(&file_info);

    if (rb->button_get(false)!=BUTTON_NONE) {
      close_wav(&file_info);
      return PLUGIN_OK;
    }
  }

  close_wav(&file_info);
  rb->splash(HZ*2, true, "FINISHED!");

  /* Flush internal buffers etc */
//No need for this.  flacResult=FLAC__seekable_stream_decoder_reset(flacDecoder);

  //  audio_close();

  return PLUGIN_OK;
}
#endif /* CONFIG_HWCODEC == MASNONE */
