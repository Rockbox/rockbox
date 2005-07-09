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

#include <codecs/libwavpack/wavpack.h>

#define SAMPLES_PER_BLOCK 22050

static struct plugin_api* rb;

void *memset(void *s, int c, size_t n) {
  return(rb->memset(s,c,n));
}

void *memcpy(void *dest, const void *src, size_t n) {
  return(rb->memcpy(dest,src,n));
}

static char *audiobuf;
static int audiobuflen;

static struct wav_header {
    char ckID [4];                      /* RIFF chuck header */
    long ckSize;
    char formType [4];

    char fmt_ckID [4];                 /* format chunk header */
    long fmt_ckSize;
    ushort FormatTag, NumChannels;
    ulong SampleRate, BytesPerSecond;
    ushort BlockAlign, BitsPerSample;

    char data_ckID [4];                 /* data chunk header */
    long data_ckSize;
} raw_header, native_header;

#define WAV_HEADER_FORMAT "4L44LSSLLSS4L"

static void wvupdate (long start_tick,
                      long sample_rate,
                      ulong total_samples,
                      ulong samples_converted,
                      ulong bytes_read,
                      ulong bytes_written)
{
    long elapsed_ticks = *rb->current_tick - start_tick;
    int compression = 0, progress = 0, realtime = 0;
    char buf[32];

    if (total_samples)
        progress = (int)(((long long) samples_converted * 100 +
            (total_samples/2)) / total_samples);

    if (elapsed_ticks)
        realtime = (int)(((long long) samples_converted * 100 * HZ /
            sample_rate + (elapsed_ticks/2)) / elapsed_ticks);

    if (bytes_read)
        compression = (int)(((long long)(bytes_read - bytes_written) * 100 +
            (bytes_read/2)) / bytes_read);

    rb->snprintf(buf, 32, "elapsed time: %d secs", (elapsed_ticks + (HZ/2)) / HZ);
    rb->lcd_puts(0, 2, buf);

    rb->snprintf(buf, 32, "progress: %d%%", progress);
    rb->lcd_puts(0, 4, buf);

    rb->snprintf(buf, 32, "realtime: %d%%  ", realtime);
    rb->lcd_puts(0, 6, buf);

    rb->snprintf(buf, 32, "compression: %d%%  ", compression);
    rb->lcd_puts(0, 8, buf);

#ifdef HAVE_LCD_BITMAP
    rb->lcd_update();
#endif
}

#define TEMP_SAMPLES 4096

static long temp_buffer [TEMP_SAMPLES] IDATA_ATTR;

static int wav2wv (char *filename)
{
    int in_fd, out_fd, num_chans, error = false, last_buttons;
    unsigned long total_bytes_read = 0, total_bytes_written = 0;
    unsigned long total_samples, samples_remaining;
    long *input_buffer = (long *) audiobuf;
    unsigned char *output_buffer = audiobuf + 0x100000;
    char *extension, save_a;
    WavpackConfig config;
    WavpackContext *wpc;
    long start_tick;

    rb->lcd_clear_display();
    rb->lcd_puts_scroll(0, 0, filename);
#ifdef HAVE_LCD_BITMAP
    rb->lcd_update();
#endif

    last_buttons = rb->button_status ();
    start_tick = *rb->current_tick;
    extension = filename + rb->strlen (filename) - 3;

    if (rb->strcasecmp (extension, "wav")) {
        rb->splash(HZ*2, true, "only for wav files!");
        return 1;
    }

    in_fd = rb->open(filename, O_RDONLY);

    if (in_fd < 0) {
        rb->splash(HZ*2, true, "could not open file!");
        return true;
    }

    if (rb->read (in_fd, &raw_header, sizeof (raw_header)) != sizeof (raw_header)) {
        rb->splash(HZ*2, true, "could not read file!");
        return true;
    }

    total_bytes_read += sizeof (raw_header);
    rb->memcpy (&native_header, &raw_header, sizeof (raw_header));
    little_endian_to_native (&native_header, WAV_HEADER_FORMAT);

	if (rb->strncmp (native_header.ckID, "RIFF", 4) ||
	    rb->strncmp (native_header.fmt_ckID, "fmt ", 4) ||
	    rb->strncmp (native_header.data_ckID, "data", 4) ||
        native_header.FormatTag != 1 || native_header.BitsPerSample != 16) {
            rb->splash(HZ*2, true, "incompatible wav file!");
            return true;
    }

    wpc = WavpackOpenFileOutput ();

    rb->memset (&config, 0, sizeof (config));
    config.bits_per_sample = 16;
    config.bytes_per_sample = 2;
    config.sample_rate = native_header.SampleRate;
    num_chans = config.num_channels = native_header.NumChannels;
	total_samples = native_header.data_ckSize / native_header.BlockAlign;

//  config.flags |= CONFIG_HIGH_FLAG;

    if (!WavpackSetConfiguration (wpc, &config, total_samples)) {
        rb->splash(HZ*2, true, "internal error!");
        rb->close (in_fd);
        return true;
    }

    WavpackAddWrapper (wpc, &raw_header, sizeof (raw_header));
    save_a = extension [1];
    extension [1] = extension [2];
    extension [2] = 0;

    out_fd = rb->creat (filename, O_WRONLY);

    extension [2] = extension [1];
    extension [1] = save_a;

    if (out_fd < 0) {
        rb->splash(HZ*2, true, "could not create file!");
        rb->close (in_fd);
        return true;
    }

    wvupdate (start_tick, native_header.SampleRate, total_samples, 0, 0, 0);

    for (samples_remaining = total_samples; samples_remaining;) {
        unsigned long samples_count, samples_to_pack, bytes_count;
        int cnt, buttons;
        long value, *lp;
        char *cp;
 
        samples_count = SAMPLES_PER_BLOCK;

        if (samples_count > samples_remaining)
            samples_count = samples_remaining;

        bytes_count = samples_count * num_chans * 2;

        if (rb->read (in_fd, input_buffer, bytes_count) != (long) bytes_count) {
            rb->splash(HZ*2, true, "could not read file!");
            error = true;
            break;
        }

        total_bytes_read += bytes_count;
        WavpackStartBlock (wpc, output_buffer, output_buffer + 0x100000);
        samples_to_pack = samples_count;
        cp = (char *) input_buffer;

        while (samples_to_pack) {
            unsigned long samples_this_pass = TEMP_SAMPLES / num_chans;

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
                rb->splash(HZ*2, true, "internal error!");
                error = true;
                break;
            }

            samples_to_pack -= samples_this_pass;
        }

        if (error)
            break;

        bytes_count = WavpackFinishBlock (wpc);

        if (rb->write (out_fd, output_buffer, bytes_count) != (long) bytes_count) {
            rb->splash(HZ*2, true, "could not write file!");
            error = true;
            break;
        }

        total_bytes_written += bytes_count;
        samples_remaining -= samples_count;

        wvupdate (start_tick, native_header.SampleRate, total_samples,
            total_samples - samples_remaining, total_bytes_read, total_bytes_written);

        buttons = rb->button_status ();

        if (last_buttons == BUTTON_NONE && buttons != BUTTON_NONE) {
            rb->splash(HZ*2, true, "operation aborted!");
            error = true;
            break;
        }
        else
            last_buttons = buttons;
    }

    rb->close (out_fd);
    rb->close (in_fd);

    if (error) {
        save_a = extension [1];
        extension [1] = extension [2];
        extension [2] = 0;
        rb->remove (filename);
        extension [2] = extension [1];
        extension [1] = save_a;
    }
    else
        rb->splash(HZ*3, true, "operation successful");

    return error;
}

enum plugin_status plugin_start(struct plugin_api* api, void *parameter)
{
    TEST_PLUGIN_API(api);

    rb = api;

    if (!parameter)
        return PLUGIN_ERROR;

    audiobuf = rb->plugin_get_audio_buffer(&audiobuflen);

    if (audiobuflen < 0x200000) {
        rb->splash(HZ*2, true, "not enough memory!");
        return PLUGIN_ERROR;
    }
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    wav2wv (parameter);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    return PLUGIN_OK;
}
