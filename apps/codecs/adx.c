/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006-2008 Adam Gashlin (hcs)
 * Copyright (C) 2006 Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "codeclib.h"
#include "inttypes.h"
#include "math.h"

CODEC_HEADER

/* Maximum number of bytes to process in one iteration */
#define WAV_CHUNK_SIZE (1024*2)

/* Number of times to loop looped tracks when repeat is disabled */
#define LOOP_TIMES 2

/* Length of fade-out for looped tracks (milliseconds) */
#define FADE_LENGTH 10000L

/* Default high pass filter cutoff frequency is 500 Hz.
 * Others can be set, but the default is nearly always used,
 * and there is no way to determine if another was used, anyway.
 */
const long cutoff = 500;

static int16_t samples[WAV_CHUNK_SIZE] IBSS_ATTR;

/* fixed point stuff from apps/plugins/lib/fixedpoint.c */

/* Inverse gain of circular cordic rotation in s0.31 format. */
static const long cordic_circular_gain = 0xb2458939; /* 0.607252929 */

/* Table of values of atan(2^-i) in 0.32 format fractions of pi where pi = 0xffffffff / 2 */
static const unsigned long atan_table[] = {
    0x1fffffff, /* +0.785398163 (or pi/4) */
    0x12e4051d, /* +0.463647609 */
    0x09fb385b, /* +0.244978663 */
    0x051111d4, /* +0.124354995 */
    0x028b0d43, /* +0.062418810 */
    0x0145d7e1, /* +0.031239833 */
    0x00a2f61e, /* +0.015623729 */
    0x00517c55, /* +0.007812341 */
    0x0028be53, /* +0.003906230 */
    0x00145f2e, /* +0.001953123 */
    0x000a2f98, /* +0.000976562 */
    0x000517cc, /* +0.000488281 */
    0x00028be6, /* +0.000244141 */
    0x000145f3, /* +0.000122070 */
    0x0000a2f9, /* +0.000061035 */
    0x0000517c, /* +0.000030518 */
    0x000028be, /* +0.000015259 */
    0x0000145f, /* +0.000007629 */
    0x00000a2f, /* +0.000003815 */
    0x00000517, /* +0.000001907 */
    0x0000028b, /* +0.000000954 */
    0x00000145, /* +0.000000477 */
    0x000000a2, /* +0.000000238 */
    0x00000051, /* +0.000000119 */
    0x00000028, /* +0.000000060 */
    0x00000014, /* +0.000000030 */
    0x0000000a, /* +0.000000015 */
    0x00000005, /* +0.000000007 */
    0x00000002, /* +0.000000004 */
    0x00000001, /* +0.000000002 */
    0x00000000, /* +0.000000001 */
    0x00000000, /* +0.000000000 */
};

/**
 * Implements sin and cos using CORDIC rotation.
 *
 * @param phase has range from 0 to 0xffffffff, representing 0 and
 *        2*pi respectively.
 * @param cos return address for cos
 * @return sin of phase, value is a signed value from LONG_MIN to LONG_MAX,
 *         representing -1 and 1 respectively. 
 */
static long fsincos(unsigned long phase, long *cos)
{
    int32_t x, x1, y, y1;
    unsigned long z, z1;
    int i;

    /* Setup initial vector */
    x = cordic_circular_gain;
    y = 0;
    z = phase;

    /* The phase has to be somewhere between 0..pi for this to work right */
    if (z < 0xffffffff / 4) {
        /* z in first quadrant, z += pi/2 to correct */
        x = -x;
        z += 0xffffffff / 4;
    } else if (z < 3 * (0xffffffff / 4)) {
        /* z in third quadrant, z -= pi/2 to correct */
        z -= 0xffffffff / 4;
    } else {
        /* z in fourth quadrant, z -= 3pi/2 to correct */
        x = -x;
        z -= 3 * (0xffffffff / 4);
    }

    /* Each iteration adds roughly 1-bit of extra precision */
    for (i = 0; i < 31; i++) {
        x1 = x >> i;
        y1 = y >> i;
        z1 = atan_table[i];

        /* Decided which direction to rotate vector. Pivot point is pi/2 */
        if (z >= 0xffffffff / 4) {
            x -= y1;
            y += x1;
            z -= z1;
        } else {
            x += y1;
            y -= x1;
            z += z1;
        }
    }

    if (cos)
        *cos = x;

    return y;
}

/**
 * Fixed point square root via Newton-Raphson.
 * @param a square root argument.
 * @param fracbits specifies number of fractional bits in argument.
 * @return Square root of argument in same fixed point format as input. 
 */
static long fsqrt(long a, unsigned int fracbits)
{
    long b = a/2 + (1 << fracbits); /* initial approximation */
    unsigned n;
    const unsigned iterations = 8;  /* bumped up from 4 as it wasn't
                                       nearly enough for 28 fractional bits */

    for (n = 0; n < iterations; ++n)
        b = (b + (long)(((long long)(a) << fracbits)/b))/2;

    return b;
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    int channels;
    int sampleswritten, i;
    uint8_t *buf;
    int32_t ch1_1, ch1_2, ch2_1, ch2_2; /* ADPCM history */
    size_t n;
    int endofstream; /* end of stream flag */
    uint32_t avgbytespersec;
    int looping; /* looping flag */
    int loop_count; /* number of loops done so far */
    int fade_count; /*  countdown for fadeout */
    int fade_frames; /* length of fade in frames */
    off_t start_adr, end_adr; /* loop points */
    off_t chanstart, bufoff;
    /*long coef1=0x7298L,coef2=-0x3350L;*/
    long coef1, coef2;

    /* Generic codec initialisation */
    /* we only render 16 bits */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 16);
  
next_track:
    DEBUGF("ADX: next_track\n");
    if (codec_init()) {
        return CODEC_ERROR;
    }
    DEBUGF("ADX: after init\n");
    
    /* init history */
    ch1_1=ch1_2=ch2_1=ch2_2=0;

    /* wait for track info to load */
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);
        
    /* Get header */
    DEBUGF("ADX: request initial buffer\n");
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, 0x38);
    if (!buf || n < 0x38) {
        return CODEC_ERROR;
    }
    bufoff = 0;
    DEBUGF("ADX: read size = %lx\n",(unsigned long)n);

    /* Get file header for starting offset, channel count */
    
    chanstart = ((buf[2] << 8) | buf[3]) + 4;
    channels = buf[7];
    
    /* useful for seeking and reporting current playback position */
    avgbytespersec = ci->id3->frequency * 18 * channels / 32;
    DEBUGF("avgbytespersec=%ld\n",(unsigned long)avgbytespersec);

    /* calculate filter coefficients */

    /**
     * A simple table of these coefficients would be nice, but
     * some very odd frequencies are used and if I'm going to
     * interpolate I might as well just go all the way and
     * calclate them precisely.
     * Speed is not an issue as this only needs to be done once per file.
     */
    {
        const int64_t big28 = 0x10000000LL;
        const int64_t big32 = 0x100000000LL;
        int64_t frequency = ci->id3->frequency;
        int64_t phasemultiple = cutoff*big32/frequency;

        long z;
        int64_t a;
        const int64_t b = (M_SQRT2*big28)-big28;
        int64_t c;
        int64_t d;
        
        fsincos((unsigned long)phasemultiple,&z);

        a = (M_SQRT2*big28)-(z*big28/LONG_MAX);

        /**
         * In the long passed to fsqrt there are only 4 nonfractional bits,
         * which is sufficient here, but this is the only reason why I don't
         * use 32 fractional bits everywhere.
         */
        d = fsqrt((a+b)*(a-b)/big28,28);
        c = (a-d)*big28/b;

        coef1 = (c*8192) >> 28;
        coef2 = (c*c/big28*-4096) >> 28;
        DEBUGF("ADX: samprate=%ld ",(long)frequency);
        DEBUGF("coef1 %04x ",(unsigned int)(coef1*4));
        DEBUGF("coef2 %04x\n",(unsigned int)(coef2*-4));
    }

    /* Get loop data */
    
    looping = 0; start_adr = 0; end_adr = 0;
    if (!memcmp(buf+0x10,"\x01\xF4\x03\x00",4)) {
        /* Soul Calibur 2 style (type 03) */
        DEBUGF("ADX: type 03 found\n");
        /* check if header is too small for loop data */
		if (chanstart-6 < 0x2c) looping=0;
	    else {
		    looping = (buf[0x18]) ||
		              (buf[0x19]) ||
		              (buf[0x1a]) ||
		              (buf[0x1b]);
		    end_adr = (buf[0x28]<<24) |
		              (buf[0x29]<<16) |
		              (buf[0x2a]<<8) |
		              (buf[0x2b]);

		    start_adr = (
		      (buf[0x1c]<<24) |
		      (buf[0x1d]<<16) |
		      (buf[0x1e]<<8) |
		      (buf[0x1f])
		      )/32*channels*18+chanstart;
		}
    } else if (!memcmp(buf+0x10,"\x01\xF4\x04\x00",4)) {
        /* Standard (type 04) */
        DEBUGF("ADX: type 04 found\n");
        /* check if header is too small for loop data */
        if (chanstart-6 < 0x38) looping=0;
		else {
			looping = (buf[0x24]) ||
			          (buf[0x25]) ||
			          (buf[0x26]) ||
			          (buf[0x27]);
		    end_adr = (buf[0x34]<<24) |
		              (buf[0x35]<<16) |
		              (buf[0x36]<<8) |
		              buf[0x37];
			start_adr = (
			  (buf[0x28]<<24) |
			  (buf[0x29]<<16) |
			  (buf[0x2a]<<8) |
			  (buf[0x2b])
			  )/32*channels*18+chanstart;
		}
    } else {
        DEBUGF("ADX: error, couldn't determine ADX type\n");
        return CODEC_ERROR;
    }

    if (looping) {
        DEBUGF("ADX: looped, start: %lx end: %lx\n",start_adr,end_adr);
    } else {
        DEBUGF("ADX: not looped\n");
    }
    
    /* advance to first frame */
    DEBUGF("ADX: first frame at %lx\n",chanstart);
    bufoff = chanstart;

    /* get in position */
    ci->seek_buffer(bufoff);


    /* setup pcm buffer format */
    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    if (channels == 2) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    } else if (channels == 1) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    } else {
        DEBUGF("ADX CODEC_ERROR: more than 2 channels\n");
        return CODEC_ERROR;
    }    

    endofstream = 0;
    loop_count = 0;
    fade_count = -1; /* disable fade */
    fade_frames = 1;

    /* The main decoder loop */
        
    while (!endofstream) {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }
        
        /* do we need to loop? */
        if (bufoff > end_adr-18*channels && looping) {
            DEBUGF("ADX: loop!\n");
            /* check for endless looping */
            if (ci->global_settings->repeat_mode==REPEAT_ONE) {
                loop_count=0;
                fade_count = -1; /* disable fade */
            } else {
                /* otherwise start fade after LOOP_TIMES loops */
                loop_count++;
                if (loop_count >= LOOP_TIMES && fade_count < 0) {
                    /* frames to fade over */
                    fade_frames = FADE_LENGTH*ci->id3->frequency/32/1000;
                    /* volume relative to fade_frames */
                    fade_count = fade_frames;
                    DEBUGF("ADX: fade_frames = %d\n",fade_frames);
                }
            }
            bufoff = start_adr;
            ci->seek_buffer(bufoff);
        }

        /* do we need to seek? */
        if (ci->seek_time) {
            uint32_t newpos;
            
            DEBUGF("ADX: seek to %ldms\n",ci->seek_time);

            endofstream = 0;
            loop_count = 0;
            fade_count = -1; /* disable fade */
            fade_frames = 1;

            newpos = (((uint64_t)avgbytespersec*(ci->seek_time - 1))
                      / (1000LL*18*channels))*(18*channels);
            bufoff = chanstart + newpos;
            while (bufoff > end_adr-18*channels) {
                bufoff-=end_adr-start_adr;
                loop_count++;
            }
            ci->seek_buffer(bufoff);
            ci->seek_complete();
        }

        if (bufoff>ci->filesize-channels*18) break; /* End of stream */
        
        sampleswritten=0;
          
        while (
                /* Is there data left in the file? */
                (bufoff <= ci->filesize-(18*channels)) &&
                /* Is there space in the output buffer? */
                (sampleswritten <= WAV_CHUNK_SIZE-(32*channels)) &&
                /* Should we be looping? */
                ((!looping) || bufoff <= end_adr-18*channels))
        {
            /* decode first/only channel */
            int32_t scale;
            int32_t ch1_0, d;

            /* fetch a frame */
            buf = ci->request_buffer(&n, 18);

            if (!buf || n!=18) {
                DEBUGF("ADX: couldn't get buffer at %lx\n",
                        bufoff);
                return CODEC_ERROR;
            }

            scale = ((buf[0] << 8) | (buf[1])) +1;
  
            for (i = 2; i < 18; i++)
            {
                d = (buf[i] >> 4) & 15;
                if (d & 8) d-= 16;
                ch1_0 = d*scale + ((coef1*ch1_1 + coef2*ch1_2) >> 12);
	            if (ch1_0 > 32767) ch1_0 = 32767;
                else if (ch1_0 < -32768) ch1_0 = -32768;
	            samples[sampleswritten] = ch1_0;
	            sampleswritten+=channels;
                ch1_2 = ch1_1; ch1_1 = ch1_0;

                d = buf[i] & 15;
                if (d & 8) d -= 16;
                ch1_0 = d*scale + ((coef1*ch1_1 + coef2*ch1_2) >> 12);
                if (ch1_0 > 32767) ch1_0 = 32767;
                else if (ch1_0 < -32768) ch1_0 = -32768; 
  	            samples[sampleswritten] = ch1_0;
  	            sampleswritten+=channels;
	            ch1_2 = ch1_1; ch1_1 = ch1_0;
            }
            bufoff+=18;
            ci->advance_buffer(18);
            
            if (channels == 2) {
                /* decode second channel */
                int32_t scale;
                int32_t ch2_0, d;

                buf = ci->request_buffer(&n, 18);

                if (!buf || n!=18) {
                    DEBUGF("ADX: couldn't get buffer at %lx\n",
                            bufoff);
                    return CODEC_ERROR;
                }

                scale = ((buf[0] << 8)|(buf[1]))+1;
  
                sampleswritten-=63;

                for (i = 2; i < 18; i++)
                {
                    d = (buf[i] >> 4) & 15;
                    if (d & 8) d-= 16;
                    ch2_0 = d*scale + ((coef1*ch2_1 + coef2*ch2_2) >> 12);
	                if (ch2_0 > 32767) ch2_0 = 32767;
                    else if (ch2_0 < -32768) ch2_0 = -32768;
	                samples[sampleswritten] = ch2_0;
	                sampleswritten+=2;
                    ch2_2 = ch2_1; ch2_1 = ch2_0;

                    d = buf[i] & 15;
                    if (d & 8) d -= 16;
                    ch2_0 = d*scale + ((coef1*ch2_1 + coef2*ch2_2) >> 12);
                    if (ch2_0 > 32767) ch2_0 = 32767;
                    else if (ch2_0 < -32768) ch2_0 = -32768; 
  	                samples[sampleswritten] = ch2_0;
  	                sampleswritten+=2;
  	                ch2_2 = ch2_1; ch2_1 = ch2_0;
                }
                bufoff+=18;
                ci->advance_buffer(18);
                sampleswritten--; /* go back to first channel's next sample */
            }

            if (fade_count>0) {
                fade_count--;
                for (i=0;i<(channels==1?32:64);i++) samples[sampleswritten-i-1]=
                  ((int32_t)samples[sampleswritten-i-1])*fade_count/fade_frames;
                if (fade_count==0) {endofstream=1; break;}
            }
        }

        if (channels == 2)
            sampleswritten >>= 1; /* make samples/channel */

        ci->pcmbuf_insert(samples, NULL, sampleswritten);
            
        ci->set_elapsed(
           ((end_adr-start_adr)*loop_count + bufoff-chanstart)*
           1000LL/avgbytespersec);
    }

    if (ci->request_next_track())
        goto next_track;

    return CODEC_OK;
}
