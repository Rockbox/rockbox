/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#include "libm4a/m4a.h"
#include "libalac/decomp.h"

CODEC_HEADER

int32_t outputbuffer[ALAC_MAX_CHANNELS][ALAC_BLOCKSIZE] IBSS_ATTR;

/* this is the codec entry point */
enum codec_status codec_main(void)
{
  size_t n;
  demux_res_t demux_res;
  stream_t input_stream;
  uint32_t samplesdone;
  uint32_t elapsedtime;
  uint32_t sample_duration;
  uint32_t sample_byte_size;
  int samplesdecoded;
  unsigned int i;
  unsigned char* buffer;
  alac_file alac;
  int retval;

  /* Generic codec initialisation */
  ci->configure(CODEC_SET_FILEBUF_WATERMARK, 1024*512);

  ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);
  ci->configure(DSP_SET_SAMPLE_DEPTH, ALAC_OUTPUT_DEPTH-1);

  next_track:

  if (codec_init()) {
    LOGF("ALAC: Error initialising codec\n");
    retval = CODEC_ERROR;
    goto exit;
  }

  while (!*ci->taginfo_ready && !ci->stop_codec)
    ci->sleep(1);
  
  ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
  codec_set_replaygain(ci->id3);

  stream_create(&input_stream,ci);

  /* if qtmovie_read returns successfully, the stream is up to
   * the movie data, which can be used directly by the decoder */
  if (!qtmovie_read(&input_stream, &demux_res)) {
    LOGF("ALAC: Error initialising file\n");
    retval = CODEC_ERROR;
    goto done;
  }

  /* initialise the sound converter */
  create_alac(demux_res.sound_sample_size, demux_res.num_channels,&alac);
  alac_set_info(&alac, demux_res.codecdata);

  i=0;
  samplesdone=0;
  /* The main decoding loop */
  while (i < demux_res.num_sample_byte_sizes) {
    ci->yield();
    if (ci->stop_codec || ci->new_track) {
      break;
    }

    /* Deal with any pending seek requests */
    if (ci->seek_time) {
      if (alac_seek(&demux_res, &input_stream,
                    ((ci->seek_time-1)/10) * (ci->id3->frequency/100),
                    &samplesdone, (int *)&i)) {
        elapsedtime=(samplesdone*10)/(ci->id3->frequency/100);
        ci->set_elapsed(elapsedtime);
      }
      ci->seek_complete();
    }

    /* Lookup the length (in samples and bytes) of block i */
    if (!get_sample_info(&demux_res, i, &sample_duration, 
                         &sample_byte_size)) {
      LOGF("ALAC: Error in get_sample_info\n");
      retval = CODEC_ERROR;
      goto done;
    }

    /* Request the required number of bytes from the input buffer */

    buffer=ci->request_buffer(&n,sample_byte_size);
    if (n!=sample_byte_size) {
        retval = CODEC_ERROR;
        goto done;
    }

    /* Decode one block - returned samples will be host-endian */
    ci->yield();
    samplesdecoded=alac_decode_frame(&alac, buffer, outputbuffer, ci->yield);

    /* Advance codec buffer n bytes */
    ci->advance_buffer(n);

    /* Output the audio */
    ci->yield();
    ci->pcmbuf_insert(outputbuffer[0], outputbuffer[1], samplesdecoded);

    /* Update the elapsed-time indicator */
    samplesdone+=sample_duration;
    elapsedtime=(samplesdone*10)/(ci->id3->frequency/100);
    ci->set_elapsed(elapsedtime);

    /* Keep track of current position - for resuming */
    ci->set_offset(elapsedtime);

    i++;
  }
  retval = CODEC_OK;

done:
  LOGF("ALAC: Decoded %ld samples\n",samplesdone);

  if (ci->request_next_track())
   goto next_track;

exit:
  return retval;
}
