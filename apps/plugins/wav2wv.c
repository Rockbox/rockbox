/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 David Bryant
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#ifdef RB_PROFILE
#include "lib/profile_plugin.h"
#endif

#include <codecs/libwavpack/wavpack.h>

PLUGIN_HEADER

#define SAMPLES_PER_BLOCK 22050

static const struct plugin_api* rb;

void *memset(void *s, int c, size_t n) {
  return(rb->memset(s,c,n));
}

void *memcpy(void *dest, const void *src, size_t n) {
  return(rb->memcpy(dest,src,n));
}

static char *audiobuf;
static ssize_t audiobuflen;

static struct wav_header {
    char ckID [4];                      /* RIFF chuck header */
    int32_t ckSize;
    char formType [4];

    char fmt_ckID [4];                 /* format chunk header */
    int32_t fmt_ckSize;
    ushort FormatTag, NumChannels;
    uint32_t SampleRate, BytesPerSecond;
    ushort BlockAlign, BitsPerSample;

    char data_ckID [4];                 /* data chunk header */
    int32_t data_ckSize;
} raw_header, native_header;

#define WAV_HEADER_FORMAT "4L44LSSLLSS4L"

static void wvupdate (int32_t start_tick,
                      int32_t sample_rate,
                      uint32_t total_samples,
                      uint32_t samples_converted,
                      uint32_t bytes_read,
                      uint32_t bytes_written)
{
    int32_t elapsed_ticks = *rb->current_tick - start_tick;
    int compression = 0, progress = 0, realtime = 0;
    char buf[32];

    if (total_samples)
        progress = (int)(((int64_t) samples_converted * 100 +
            (total_samples/2)) / total_samples);

    if (elapsed_ticks)
        realtime = (int)(((int64_t) samples_converted * 100 * HZ /
            sample_rate + (elapsed_ticks/2)) / elapsed_ticks);

    if (bytes_read)
        compression = (int)(((int64_t)(bytes_read - bytes_written) * 100 +
            (bytes_read/2)) / bytes_read);

    rb->snprintf(buf, 32, "elapsed time: %ld secs",
                 (long)(elapsed_ticks + (HZ/2)) / HZ);
    rb->lcd_puts(0, 2, (unsigned char *)buf);

    rb->snprintf(buf, 32, "progress: %d%%", progress);
    rb->lcd_puts(0, 4, (unsigned char *)buf);

    rb->snprintf(buf, 32, "realtime: %d%%  ", realtime);
    rb->lcd_puts(0, 6, (unsigned char *)buf);

    rb->snprintf(buf, 32, "compression: %d%%  ", compression);
    rb->lcd_puts(0, 8, (unsigned char *)buf);

#ifdef HAVE_LCD_BITMAP
    rb->lcd_update();
#endif
}

#define TEMP_SAMPLES 4096

static int32_t temp_buffer [TEMP_SAMPLES] IDATA_ATTR;

static int wav2wv(const char *infile)
{
    int in_fd, out_fd, num_chans, error = false, last_buttons;
    unsigned int32_t total_bytes_read = 0, total_bytes_written = 0;
    unsigned int32_t total_samples, samples_remaining;
    int32_t *input_buffer = (int32_t *) audiobuf;
    unsigned char *output_buffer = (unsigned char *)(audiobuf + 0x100000);
    char outfile[MAX_PATH];
    const char *inextension;
    char *outextension;
    WavpackConfig config;
    WavpackContext *wpc;
    int32_t start_tick;

    rb->lcd_clear_display();
    rb->lcd_puts_scroll(0, 0, (unsigned char *)infile);
#ifdef HAVE_LCD_BITMAP
    rb->lcd_update();
#endif

    last_buttons = rb->button_status ();
    start_tick = *rb->current_tick;
    inextension = infile + rb->strlen(infile) - 3;
    if (rb->strcasecmp (inextension, "wav")) {
        rb->splash(HZ*2, "only for wav files!");
        return 1;
    }

    in_fd = rb->open(infile, O_RDONLY);

    if (in_fd < 0) {
        rb->splash(HZ*2, "could not open file!");
        return true;
    }

    if (rb->read (in_fd, &raw_header, sizeof (raw_header)) != sizeof (raw_header)) {
        rb->splash(HZ*2, "could not read file!");
        return true;
    }

    total_bytes_read += sizeof (raw_header);
    rb->memcpy (&native_header, &raw_header, sizeof (raw_header));
    little_endian_to_native (&native_header, WAV_HEADER_FORMAT);

	if (rb->strncmp (native_header.ckID, "RIFF", 4) ||
	    rb->strncmp (native_header.fmt_ckID, "fmt ", 4) ||
	    rb->strncmp (native_header.data_ckID, "data", 4) ||
        native_header.FormatTag != 1 || native_header.BitsPerSample != 16) {
            rb->splash(HZ*2, "incompatible wav file!");
            return true;
    }

    wpc = WavpackOpenFileOutput ();

    rb->memset (&config, 0, sizeof (config));
    config.bits_per_sample = 16;
    config.bytes_per_sample = 2;
    config.sample_rate = native_header.SampleRate;
    num_chans = config.num_channels = native_header.NumChannels;
	total_samples = native_header.data_ckSize / native_header.BlockAlign;

/*  config.flags |= CONFIG_HIGH_FLAG; */

    if (!WavpackSetConfiguration (wpc, &config, total_samples)) {
        rb->splash(HZ*2, "internal error!");
        rb->close (in_fd);
        return true;
    }

    WavpackAddWrapper (wpc, &raw_header, sizeof (raw_header));

    rb->strcpy(outfile, infile);
    outextension = outfile + rb->strlen(outfile) - 3; 
    outextension[1] = outextension[2];
    outextension[2] = 0;
    out_fd = rb->creat(outfile);

    if (out_fd < 0) {
        rb->splash(HZ*2, "could not create file!");
        rb->close (in_fd);
        return true;
    }

    wvupdate (start_tick, native_header.SampleRate, total_samples, 0, 0, 0);

    for (samples_remaining = total_samples; samples_remaining;) {
        unsigned int32_t samples_count, samples_to_pack, bytes_count;
        int cnt, buttons;
        int32_t value, *lp;
        signed char *cp;
 
        samples_count = SAMPLES_PER_BLOCK;

        if (samples_count > samples_remaining)
            samples_count = samples_remaining;

        bytes_count = samples_count * num_chans * 2;

        if (rb->read (in_fd, input_buffer, bytes_count) != (int32_t) bytes_count) {
            rb->splash(HZ*2, "could not read file!");
            error = true;
            break;
        }

        total_bytes_read += bytes_count;
        WavpackStartBlock (wpc, output_buffer, output_buffer + 0x100000);
        samples_to_pack = samples_count;
        cp = (signed char *) input_buffer;

        while (samples_to_pack) {
            unsigned int32_t samples_this_pass = TEMP_SAMPLES / num_chans;

            if (samples_this_pass > samples_to_pack)
                samples_this_pass = samples_to_pack;

            lp = temp_buffer;
            cnt = samples_this_pass;

            if (num_chans == 2)
                while (cnt--) {
                    value = *cp++ & 0xff;
                    value += *cp++ << 8;
                    *lp++ = value;
                    value = *cp++ & 0xff;
                    value += *cp++ << 8;
                    *lp++ = value;
                } 
            else
                while (cnt--) {
                    value = *cp++ & 0xff;
                    value += *cp++ << 8;
                    *lp++ = value;
                } 

            if (!WavpackPackSamples (wpc, temp_buffer, samples_this_pass)) {
                rb->splash(HZ*2, "internal error!");
                error = true;
                break;
            }

            samples_to_pack -= samples_this_pass;
        }

        if (error)
            break;

        bytes_count = WavpackFinishBlock (wpc);

        if (rb->write (out_fd, output_buffer, bytes_count) != (int32_t) bytes_count) {
            rb->splash(HZ*2, "could not write file!");
            error = true;
            break;
        }

        total_bytes_written += bytes_count;
        samples_remaining -= samples_count;

        wvupdate (start_tick, native_header.SampleRate, total_samples,
            total_samples - samples_remaining, total_bytes_read, total_bytes_written);

        buttons = rb->button_status ();

        if (last_buttons == BUTTON_NONE && buttons != BUTTON_NONE) {
            rb->splash(HZ*2, "operation aborted!");
            error = true;
            break;
        }
        else
            last_buttons = buttons;
    }

    rb->close (out_fd);
    rb->close (in_fd);

    if (error) {
        rb->remove (outfile);
    }
    else
        rb->splash(HZ*3, "operation successful");

    return error;
}

enum plugin_status plugin_start(const struct plugin_api* api, const void *parameter)
{
#ifdef RB_PROFILE
    /* This doesn't start profiling or anything, it just gives the
     * profiling functions that are compiled in someplace to call,
     * this is needed here to let this compile with profiling support
     * since it calls code from a codec that is compiled with profiling
     * support */
    profile_init(api);
#endif

    rb = api;

    if (!parameter)
        return PLUGIN_ERROR;

    audiobuf = rb->plugin_get_audio_buffer((size_t *)&audiobuflen);

    if (audiobuflen < 0x200000) {
        rb->splash(HZ*2, "not enough memory!");
        return PLUGIN_ERROR;
    }
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    wav2wv (parameter);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    /* Return PLUGIN_USB_CONNECTED to force a file-tree refresh */
    return PLUGIN_USB_CONNECTED;
}
