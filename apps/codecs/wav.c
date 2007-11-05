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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "codeclib.h"
#include "inttypes.h"

CODEC_HEADER

/* Macro that sign extends an unsigned byte */
#define SE(x) ((int32_t)((int8_t)(x)))

/* This codec support WAVE files with the following formats:
 * - PCM, up to 32 bits, supporting 32 bits playback when useful.
 * - ALAW and MULAW (16 bits compressed on 8 bits).
 * - DVI_ADPCM (16 bits compressed on 3 or 4 bits).
 * 
 *  For a good documentation on WAVE files, see:
 *  http://www.tsp.ece.mcgill.ca/MMSP/Documents/AudioFormats/WAVE/WAVE.html
 *  and
 *  http://www.sonicspot.com/guide/wavefiles.html
 *
 *  For sample WAV files, see:
 *  http://www.tsp.ece.mcgill.ca/MMSP/Documents/AudioFormats/WAVE/Samples.html
 *
 * The most common formats seem to be PCM, ADPCM, DVI_ADPCM, IEEE_FLOAT,
 *  ALAW and MULAW
 */

/* These constants are from RFC 2361. */
enum
{
    WAVE_FORMAT_UNKNOWN = 0x0000, /* Microsoft Unknown Wave Format */
    WAVE_FORMAT_PCM = 0x0001,   /* Microsoft PCM Format */
    WAVE_FORMAT_ADPCM = 0x0002, /* Microsoft ADPCM Format */
    WAVE_FORMAT_IEEE_FLOAT = 0x0003, /* IEEE Float */
    WAVE_FORMAT_VSELP = 0x0004, /* Compaq Computer's VSELP */
    WAVE_FORMAT_IBM_CVSD = 0x0005, /* IBM CVSD */
    WAVE_FORMAT_ALAW = 0x0006,  /* Microsoft ALAW */
    WAVE_FORMAT_MULAW = 0x0007, /* Microsoft MULAW */
    WAVE_FORMAT_OKI_ADPCM = 0x0010, /* OKI ADPCM */
    WAVE_FORMAT_DVI_ADPCM = 0x0011, /* Intel's DVI ADPCM */
    WAVE_FORMAT_MEDIASPACE_ADPCM = 0x0012, /* Videologic's MediaSpace ADPCM */
    WAVE_FORMAT_SIERRA_ADPCM = 0x0013, /* Sierra ADPCM */
    WAVE_FORMAT_G723_ADPCM = 0x0014, /* G.723 ADPCM */
    WAVE_FORMAT_DIGISTD = 0x0015, /* DSP Solutions' DIGISTD */
    WAVE_FORMAT_DIGIFIX = 0x0016, /* DSP Solutions' DIGIFIX */
    WAVE_FORMAT_DIALOGIC_OKI_ADPCM = 0x0017, /* Dialogic OKI ADPCM */
    WAVE_FORMAT_MEDIAVISION_ADPCM = 0x0018, /* MediaVision ADPCM */
    WAVE_FORMAT_CU_CODEC = 0x0019, /* HP CU */
    WAVE_FORMAT_YAMAHA_ADPCM = 0x0020, /* Yamaha ADPCM */
    WAVE_FORMAT_SONARC = 0x0021, /* Speech Compression's Sonarc */
    WAVE_FORMAT_DSP_TRUESPEECH = 0x0022, /* DSP Group's True Speech */
    WAVE_FORMAT_ECHOSC1 = 0x0023, /* Echo Speech's EchoSC1 */
    WAVE_FORMAT_AUDIOFILE_AF36 = 0x0024, /* Audiofile AF36 */
    WAVE_FORMAT_APTX = 0x0025,  /* APTX */
    WAVE_FORMAT_DOLBY_AC2 = 0x0030, /* Dolby AC2 */
    WAVE_FORMAT_GSM610 = 0x0031, /* GSM610 */
    WAVE_FORMAT_MSNAUDIO = 0x0032, /* MSNAudio */
    WAVE_FORMAT_ANTEX_ADPCME = 0x0033, /* Antex ADPCME */

    WAVE_FORMAT_MPEG = 0x0050,  /* MPEG */
    WAVE_FORMAT_MPEGLAYER3 = 0x0055, /* MPEG layer 3 */
    WAVE_FORMAT_LUCENT_G723 = 0x0059, /* Lucent G.723 */
    WAVE_FORMAT_G726_ADPCM = 0x0064, /* G.726 ADPCM */
    WAVE_FORMAT_G722_ADPCM = 0x0065, /* G.722 ADPCM */

    IBM_FORMAT_MULAW = 0x0101,  /* same as WAVE_FORMAT_MULAW */
    IBM_FORMAT_ALAW = 0x0102,   /* same as WAVE_FORMAT_ALAW */
    IBM_FORMAT_ADPCM = 0x0103,

    WAVE_FORMAT_CREATIVE_ADPCM = 0x0200,

    WAVE_FORMAT_EXTENSIBLE = 0xFFFE
};

/* Maximum number of bytes to process in one iteration */
/* for 44.1kHz stereo 16bits, this represents 0.023s ~= 1/50s */
#define WAV_CHUNK_SIZE (1024*2)

static const int16_t alaw2linear16[256] ICONST_ATTR = {
     -5504,   -5248,   -6016,   -5760,   -4480,   -4224,   -4992,
     -4736,   -7552,   -7296,   -8064,   -7808,   -6528,   -6272,
     -7040,   -6784,   -2752,   -2624,   -3008,   -2880,   -2240,
     -2112,   -2496,   -2368,   -3776,   -3648,   -4032,   -3904,
     -3264,   -3136,   -3520,   -3392,  -22016,  -20992,  -24064,
    -23040,  -17920,  -16896,  -19968,  -18944,  -30208,  -29184,
    -32256,  -31232,  -26112,  -25088,  -28160,  -27136,  -11008,
    -10496,  -12032,  -11520,   -8960,   -8448,   -9984,   -9472,
    -15104,  -14592,  -16128,  -15616,  -13056,  -12544,  -14080,
    -13568,    -344,    -328,    -376,    -360,    -280,    -264,
      -312,    -296,    -472,    -456,    -504,    -488,    -408,
      -392,    -440,    -424,     -88,     -72,    -120,    -104,
       -24,      -8,     -56,     -40,    -216,    -200,    -248,
      -232,    -152,    -136,    -184,    -168,   -1376,   -1312,
     -1504,   -1440,   -1120,   -1056,   -1248,   -1184,   -1888,
     -1824,   -2016,   -1952,   -1632,   -1568,   -1760,   -1696,
      -688,    -656,    -752,    -720,    -560,    -528,    -624,
      -592,    -944,    -912,   -1008,    -976,    -816,    -784,
      -880,    -848,    5504,    5248,    6016,    5760,    4480,
      4224,    4992,    4736,    7552,    7296,    8064,    7808,
      6528,    6272,    7040,    6784,    2752,    2624,    3008,
      2880,    2240,    2112,    2496,    2368,    3776,    3648,
      4032,    3904,    3264,    3136,    3520,    3392,   22016,
     20992,   24064,   23040,   17920,   16896,   19968,   18944,
     30208,   29184,   32256,   31232,   26112,   25088,   28160,
     27136,   11008,   10496,   12032,   11520,    8960,    8448,
      9984,    9472,   15104,   14592,   16128,   15616,   13056,
     12544,   14080,   13568,     344,     328,     376,     360,
       280,     264,     312,     296,     472,     456,     504,
       488,     408,     392,     440,     424,      88,      72,
       120,     104,      24,       8,      56,      40,     216,
       200,     248,     232,     152,     136,     184,     168,
      1376,    1312,    1504,    1440,    1120,    1056,    1248,
      1184,    1888,    1824,    2016,    1952,    1632,    1568,
      1760,    1696,     688,     656,     752,     720,     560,
       528,     624,     592,     944,     912,    1008,     976,
       816,     784,     880,     848
};

static const int16_t ulaw2linear16[256] ICONST_ATTR = {
    -32124,  -31100,  -30076,  -29052,  -28028,  -27004,  -25980,
    -24956,  -23932,  -22908,  -21884,  -20860,  -19836,  -18812,
    -17788,  -16764,  -15996,  -15484,  -14972,  -14460,  -13948,
    -13436,  -12924,  -12412,  -11900,  -11388,  -10876,  -10364,
     -9852,   -9340,   -8828,   -8316,   -7932,   -7676,   -7420,
     -7164,   -6908,   -6652,   -6396,   -6140,   -5884,   -5628,
     -5372,   -5116,   -4860,   -4604,   -4348,   -4092,   -3900,
     -3772,   -3644,   -3516,   -3388,   -3260,   -3132,   -3004,
     -2876,   -2748,   -2620,   -2492,   -2364,   -2236,   -2108,
     -1980,   -1884,   -1820,   -1756,   -1692,   -1628,   -1564,
     -1500,   -1436,   -1372,   -1308,   -1244,   -1180,   -1116,
     -1052,    -988,    -924,    -876,    -844,    -812,    -780,
      -748,    -716,    -684,    -652,    -620,    -588,    -556,
      -524,    -492,    -460,    -428,    -396,    -372,    -356,
      -340,    -324,    -308,    -292,    -276,    -260,    -244,
      -228,    -212,    -196,    -180,    -164,    -148,    -132,
      -120,    -112,    -104,     -96,     -88,     -80,     -72,
       -64,     -56,     -48,     -40,     -32,     -24,     -16,
        -8,       0,   32124,   31100,   30076,   29052,   28028,
     27004,   25980,   24956,   23932,   22908,   21884,   20860,
     19836,   18812,   17788,   16764,   15996,   15484,   14972,
     14460,   13948,   13436,   12924,   12412,   11900,   11388,
     10876,   10364,    9852,    9340,    8828,    8316,    7932,
      7676,    7420,    7164,    6908,    6652,    6396,    6140,
      5884,    5628,    5372,    5116,    4860,    4604,    4348,
      4092,    3900,    3772,    3644,    3516,    3388,    3260,
      3132,    3004,    2876,    2748,    2620,    2492,    2364,
      2236,    2108,    1980,    1884,    1820,    1756,    1692,
      1628,    1564,    1500,    1436,    1372,    1308,    1244,
      1180,    1116,    1052,     988,     924,     876,     844,
       812,     780,     748,     716,     684,     652,     620,
       588,     556,     524,     492,     460,     428,     396,
       372,     356,     340,     324,     308,     292,     276,
       260,     244,     228,     212,     196,     180,     164,
       148,     132,     120,     112,     104,      96,      88,
        80,      72,      64,      56,      48,      40,      32,
        24,      16,       8,       0
};

static const uint16_t dvi_adpcm_steptab[89] ICONST_ATTR = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767 };

static const int dvi_adpcm_indextab4[8] ICONST_ATTR = {
    -1, -1, -1, -1, 2, 4, 6, 8 };

static const int dvi_adpcm_indextab3[4] ICONST_ATTR = { -1, -1, 1, 2 };

static int32_t samples[WAV_CHUNK_SIZE] IBSS_ATTR;

static enum codec_status
decode_dvi_adpcm(struct codec_api *ci,
                 const uint8_t *buf,
                 int n,
                 uint16_t channels, uint16_t bitspersample,
                 int32_t *pcmout,
                 size_t *pcmoutsize);

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    uint32_t numbytes, bytesdone;
    uint32_t totalsamples = 0;
    uint16_t channels = 0;
    uint16_t samplesperblock = 0;
    int bytespersample = 0;
    uint16_t bitspersample;
    uint32_t i;
    size_t n;
    int bufcount;
    int endofstream;
    unsigned char *buf;
    uint8_t *wavbuf;
    long chunksize;
    uint16_t formattag = 0;
    uint16_t blockalign = 0;
    uint32_t avgbytespersec = 0;
    off_t firstblockposn;     /* position of the first block in file */
    
 
    /* Generic codec initialisation */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 28);
    ci->configure(CODEC_SET_FILEBUF_WATERMARK, 1024*512);
  
next_track:
    if (codec_init()) {
        i = CODEC_ERROR;
        goto exit;
    }

    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);
    
    /* Need to save offset for later use (cleared indirectly by advance_buffer) */
    bytesdone = ci->id3->offset;

    /* get RIFF chunk header */
    buf = ci->request_buffer(&n, 12);
    if (n < 12) {
        i = CODEC_ERROR;
        goto done;
    }
    if ((memcmp(buf, "RIFF", 4) != 0) || (memcmp(&buf[8], "WAVE", 4) != 0)) {
        i = CODEC_ERROR;
        goto done;
    }

    /* advance to first WAVE chunk */
    ci->advance_buffer(12);

    firstblockposn = 12;
    bitspersample = 0;
    numbytes = 0;
    totalsamples = 0;

    /* iterate over WAVE chunks until the 'data' chunk, which should be after the 'fmt ' chunk */
    while (true) {
        /* get WAVE chunk header */
        buf = ci->request_buffer(&n, 1024);
        if (n < 8) {
            /* no more chunks, 'data' chunk must not have been found */
            i = CODEC_ERROR;
            goto done;
        }

        /* chunkSize */
        i = (buf[4]|(buf[5]<<8)|(buf[6]<<16)|(buf[7]<<24));
        if (memcmp(buf, "fmt ", 4) == 0) {
            if (i < 16) {
                DEBUGF("CODEC_ERROR: 'fmt ' chunk size=%lu < 16\n",
                       (unsigned long)i);
                i = CODEC_ERROR;
                goto done;
            }
            /* wFormatTag */
            formattag=buf[8]|(buf[9]<<8);
            /* wChannels */
            channels=buf[10]|(buf[11]<<8);
            /* skipping dwSamplesPerSec */
            /* dwAvgBytesPerSec */
            avgbytespersec = buf[16]|(buf[17]<<8)|(buf[18]<<16)|(buf[19]<<24);
            /* wBlockAlign */
            blockalign=buf[20]|(buf[21]<<8);
            /* wBitsPerSample */
            bitspersample=buf[22]|(buf[23]<<8);
            if (formattag != WAVE_FORMAT_PCM) {
                uint16_t size;
                if (i < 18) {
                    /* this is not a fatal error with some formats,
                     * we'll see later if we can't decode it */
                    DEBUGF("CODEC_WARNING: non-PCM WAVE (formattag=0x%x) "
                           "doesn't have ext. fmt descr (chunksize=%ld<18).\n",
                           formattag, (long)i);
                }
                size = buf[24]|(buf[25]<<8);
                if (formattag == WAVE_FORMAT_DVI_ADPCM) {
                    if (size < 2) {
                        DEBUGF("CODEC_ERROR: dvi_adpcm is missing "
                               "SamplesPerBlock value\n");
                        i = CODEC_ERROR;
                        goto done;
                    }
                    samplesperblock = buf[26]|(buf[27]<<8);
                } else if (formattag == WAVE_FORMAT_EXTENSIBLE) {
                    if (size < 22) {
                        DEBUGF("CODEC_ERROR: WAVE_FORMAT_EXTENSIBLE is "
                               "missing extension\n");
                        i = CODEC_ERROR;
                        goto done;
                    }
                    /* wValidBitsPerSample */
                    bitspersample = buf[26]|(buf[27]<<8);
                    /* skipping dwChannelMask (4bytes) */
                    /* SubFormat (only get the first two bytes) */
                    formattag = buf[32]|(buf[33]<<8);
                }
            }
        } else if (memcmp(buf, "data", 4) == 0) {
            numbytes = i;
            /* advance to start of data */
            ci->advance_buffer(8);
            firstblockposn += 8;
            break;
        } else if (memcmp(buf, "fact", 4) == 0) {
            /* dwSampleLength */
            if (i >= 4)
                totalsamples = (buf[8]|(buf[9]<<8)|(buf[10]<<16)|(buf[11]<<24));
        } else {
            DEBUGF("unknown WAVE chunk: '%c%c%c%c', size=%lu\n",
                   buf[0], buf[1], buf[2], buf[3], (unsigned long)i);
        }

        /* go to next chunk (even chunk sizes must be padded) */
        if (i & 0x01)
            i++;
        ci->advance_buffer(i+8);
        firstblockposn += i + 8;
    }

    if (channels == 0) {
        DEBUGF("CODEC_ERROR: 'fmt ' chunk not found or 0-channels file\n");
        i = CODEC_ERROR;
        goto done;
    }
    if (numbytes == 0) {
        DEBUGF("CODEC_ERROR: 'data' chunk not found or has zero-length\n");
        i = CODEC_ERROR;
        goto done;
    }
    if (formattag != WAVE_FORMAT_PCM && totalsamples == 0) {
        /* This is non-fatal for some formats */
        DEBUGF("CODEC_WARNING: non-PCM WAVE doesn't have a 'fact' chunk\n");
    }
    if (formattag == WAVE_FORMAT_ALAW || formattag == WAVE_FORMAT_MULAW ||
        formattag == IBM_FORMAT_ALAW || formattag == IBM_FORMAT_MULAW) {
        if (bitspersample != 8) {
            DEBUGF("CODEC_ERROR: alaw and mulaw must have 8 bitspersample\n");
            i = CODEC_ERROR;
            goto done;
        }
        bytespersample = channels;
    }
    if (formattag == WAVE_FORMAT_DVI_ADPCM
        && bitspersample != 4 && bitspersample != 3) {
        DEBUGF("CODEC_ERROR: dvi_adpcm must have 3 or 4 bitspersample\n");
        i = CODEC_ERROR;
        goto done;
    }
    if (formattag == WAVE_FORMAT_PCM && bitspersample > 32) {
        DEBUGF("CODEC_ERROR: pcm with more than 32 bitspersample "
               "is unsupported\n");
        i = CODEC_ERROR;
        goto done;
    }

    ci->configure(DSP_SWITCH_FREQUENCY, ci->id3->frequency);
    if (channels == 2) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_INTERLEAVED);
    } else if (channels == 1) {
        ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    } else {
        DEBUGF("CODEC_ERROR: more than 2 channels\n");
        i = CODEC_ERROR;
        goto done;
    }

    if (totalsamples == 0) {
        if (formattag == WAVE_FORMAT_PCM ||
            formattag == WAVE_FORMAT_ALAW || formattag == WAVE_FORMAT_MULAW ||
            formattag == IBM_FORMAT_ALAW || formattag == IBM_FORMAT_MULAW) {
            /* for PCM and derived formats only */
            bytespersample = (((bitspersample - 1)/8 + 1)*channels);
            totalsamples = numbytes/bytespersample;
        } else {
            DEBUGF("CODEC_ERROR: cannot compute totalsamples\n");
            i = CODEC_ERROR;
            goto done;
        }
    }

    /* make sure we're at the correct offset */
    if (bytesdone > (uint32_t) firstblockposn) {
        /* Round down to previous block */
        uint32_t offset = bytesdone - bytesdone % blockalign;

        ci->advance_buffer(offset-firstblockposn);
        bytesdone = offset - firstblockposn;
    } else {
        /* already where we need to be */
        bytesdone = 0;
    }

    /* The main decoder loop */
    endofstream = 0;
    /* chunksize is computed so that one chunk is about 1/50s.
     * this make 4096 for 44.1kHz 16bits stereo.
     * It also has to be a multiple of blockalign */
    chunksize = (1 + avgbytespersec / (50*blockalign))*blockalign;
    /* check that the output buffer is big enough (convert to samplespersec,
       then round to the blockalign multiple below) */
    if (((uint64_t)chunksize*ci->id3->frequency*channels*2)
        /(uint64_t)avgbytespersec >= WAV_CHUNK_SIZE) {
        chunksize = ((uint64_t)WAV_CHUNK_SIZE*avgbytespersec
                     /((uint64_t)ci->id3->frequency*channels*2
                     *blockalign))*blockalign;
    }

    while (!endofstream) {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }

        if (ci->seek_time) {
            uint32_t newpos;

            /* use avgbytespersec to round to the closest blockalign multiple,
               add firstblockposn. 64-bit casts to avoid overflows. */
            newpos = (((uint64_t)avgbytespersec*(ci->seek_time - 1))
                      / (1000LL*blockalign))*blockalign;
            if (newpos > numbytes)
                break;
            if (ci->seek_buffer(firstblockposn + newpos))
                bytesdone = newpos;
            ci->seek_complete();
        }
        wavbuf = (uint8_t *)ci->request_buffer(&n, chunksize);

        if (n == 0)
            break; /* End of stream */

        if (bytesdone + n > numbytes) {
            n = numbytes - bytesdone;
            endofstream = 1;
        }

        if (formattag == WAVE_FORMAT_PCM) {
            if (bitspersample > 24) {
                for (i = 0; i < n; i += 4) {
                    samples[i/4] = (wavbuf[i] >> 3)|
                        (wavbuf[i + 1]<<5)|(wavbuf[i + 2]<<13)|
                        (SE(wavbuf[i + 3])<<21);
                }
                bufcount = n >> 2;
            } else if (bitspersample > 16) {
                for (i = 0; i < n; i += 3) {
                    samples[i/3] = (wavbuf[i]<<5)|
                        (wavbuf[i + 1]<<13)|(SE(wavbuf[i + 2])<<21);
                }
                bufcount = n/3;
            } else if (bitspersample > 8) {
                for (i = 0; i < n; i += 2) {
                    samples[i/2] = (wavbuf[i]<<13)|(SE(wavbuf[i + 1])<<21);
                }
                bufcount = n >> 1;
            } else {
                for (i = 0; i < n; i++) {
                    samples[i] = (wavbuf[i] - 0x80)<<21;
                }
                bufcount = n;
            }

            if (channels == 2)
                bufcount >>= 1;
        } else if (formattag == WAVE_FORMAT_ALAW 
                   || formattag == IBM_FORMAT_ALAW) {
            for (i = 0; i < n; i++)
                samples[i] = alaw2linear16[wavbuf[i]] << 13;

            bufcount = (channels == 2) ? (n >> 1) : n;
        } else if (formattag == WAVE_FORMAT_MULAW
                   || formattag == IBM_FORMAT_MULAW) {
            for (i = 0; i < n; i++)
                samples[i] = ulaw2linear16[wavbuf[i]] << 13;

            bufcount = (channels == 2) ? (n >> 1) : n;
        }
        else if (formattag == WAVE_FORMAT_DVI_ADPCM) {
            unsigned int nblocks = chunksize/blockalign;

            for (i = 0; i < nblocks; i++) {
                size_t decodedsize = samplesperblock*channels;
                if (decode_dvi_adpcm(ci, wavbuf + i*blockalign,
                                     blockalign, channels, bitspersample,
                                     samples + i*samplesperblock*channels,
                                     &decodedsize) != CODEC_OK) {
                    i = CODEC_ERROR;
                    goto done;
                }
            }
            bufcount = nblocks*samplesperblock;
        } else {
            DEBUGF("CODEC_ERROR: unsupported format %x\n", formattag);
            i = CODEC_ERROR;
            goto done;
        }

        ci->pcmbuf_insert(samples, NULL, bufcount);

        ci->advance_buffer(n);
        bytesdone += n;
        if (bytesdone >= numbytes)
            endofstream = 1;
        ci->set_elapsed(bytesdone*1000LL/avgbytespersec);
    }
    i = CODEC_OK;

done:
    if (ci->request_next_track())
        goto next_track;

exit:
    return i;
}

static enum codec_status
decode_dvi_adpcm(struct codec_api *ci,
                 const uint8_t *buf,
                 int n,
                 uint16_t channels, uint16_t bitspersample,
                 int32_t *pcmout,
                 size_t *pcmoutsize)
{
    size_t nsamples = 0;
    int sample[2];
    int samplecode[32][2];
    int i;
    int stepindex[2];
    int c;
    int diff;
    int step;
    int codem;
    int code;

    (void)ci;
    if (bitspersample != 4 && bitspersample != 3) {
        DEBUGF("decode_dvi_adpcm: wrong bitspersample\n");
        return CODEC_ERROR;
    }

    /* decode block header */
    for (c = 0; c < channels && n >= 4; c++) {
        /* decode + push first sample */
        sample[c] = (short)(buf[0]|(buf[1]<<8));/* need cast for sign-extend */
        pcmout[c] = sample[c] << 13;
        nsamples++;
        stepindex[c] = buf[2];
        /* check for step table index overflow */
        if (stepindex[c] > 88) {
            DEBUGF("decode_dvi_adpcm: stepindex[%d]=%d>88\n",c,stepindex[c]);
            return CODEC_ERROR;
        }

        buf += 4;
        n -= 4;
    }
    if (bitspersample == 4) {
        while (n>= channels*4 && (nsamples + 8*channels) <= *pcmoutsize) {
            for (c = 0; c < channels; c++) {
                samplecode[0][c] = buf[0]&0xf;
                samplecode[1][c] = buf[0]>>4;
                samplecode[2][c] = buf[1]&0xf;
                samplecode[3][c] = buf[1]>>4;
                samplecode[4][c] = buf[2]&0xf;
                samplecode[5][c] = buf[2]>>4;
                samplecode[6][c] = buf[3]&0xf;
                samplecode[7][c] = buf[3]>>4;
                buf += 4;
                n -= 4;
            }
            for (i = 0; i < 8; i++) {
                for (c = 0; c < channels; c++) {
                    step = dvi_adpcm_steptab[stepindex[c]];
                    codem = samplecode[i][c];
                    code = codem & 0x07;

                    /* adjust the step table index */
                    stepindex[c] += dvi_adpcm_indextab4[code];
                    /* check for step table index overflow and underflow */
                    if (stepindex[c] > 88)
                        stepindex[c] = 88;
                    else if (stepindex[c] < 0)
                        stepindex[c] = 0;
                    /* calculate the difference */
#ifdef STRICT_IMA
                    diff = 0;
                    if (code & 4)
                        diff += step;
                    step = step >> 1;
                    if (code & 2)
                        diff += step;
                    step = step >> 1;
                    if (code & 1)
                        diff += step;
                    step = step >> 1;
                    diff += step;
#else
                    diff = ((code + code + 1) * step) >> 3; /* faster */
#endif
                    /* check the sign bit */
                    /* check for overflow and underflow errors */
                    if (code != codem) {
                        sample[c] -= diff;
                        if (sample[c] < -32768)
                            sample[c] = -32768;
                    } else {
                        sample[c] += diff;
                        if (sample[c] > 32767)
                            sample[c] = 32767;
                    }
                    /* output the new sample */
                    pcmout[nsamples] = sample[c] << 13;
                    nsamples++;
                }
            }
        }
    } else {                  /* bitspersample == 3 */
        while (n >= channels*12 && (nsamples + 32*channels) <= *pcmoutsize) {
            for (c = 0; c < channels; c++) {
                uint16_t bitstream = 0;
                int bitsread = 0;
                for (i = 0; i < 32 && n > 0; i++) {
                    if (bitsread < 3) {
                        /* read 8 more bits */
                        bitstream |= buf[0]<<bitsread;
                        bitsread += 8;
                        n--;
                        buf++;
                    }
                    samplecode[i][c] = bitstream & 7;
                    bitstream = bitstream>>3;
                    bitsread -= 3;
                }
                if (bitsread != 0) {
                    /* 32*3 = 3 words, so we should end with bitsread==0 */
                    DEBUGF("decode_dvi_adpcm: error in implementation\n");
                    return CODEC_ERROR;
                }
            }
            
            for (i = 0; i < 32; i++) {
                for (c = 0; c < channels; c++) {
                    step = dvi_adpcm_steptab[stepindex[c]];
                    codem = samplecode[i][c];
                    code = codem & 0x03;

                    /* adjust the step table index */
                    stepindex[c] += dvi_adpcm_indextab3[code];
                    /* check for step table index overflow and underflow */
                    if (stepindex[c] > 88)
                        stepindex[c] = 88;
                    else if (stepindex[c] < 0)
                        stepindex[c] = 0;
                    /* calculate the difference */
#ifdef STRICT_IMA
                    diff = 0;
                    if (code & 2)
                        diff += step;
                    step = step >> 1;
                    if (code & 1)
                        diff += step;
                    step = step >> 1;
                    diff += step;
#else
                    diff = ((code + code + 1) * step) >> 3; /* faster */
#endif
                    /* check the sign bit */
                    /* check for overflow and underflow errors */
                    if (code != codem) {
                        sample[c] -= diff;
                        if (sample[c] < -32768)
                            sample[c] = -32768;
                    }
                    else {
                        sample[c] += diff;
                        if (sample[c] > 32767)
                            sample[c] = 32767;
                    }
                    /* output the new sample */
                    pcmout[nsamples] = sample[c] << 13;
                    nsamples++;
                }
            }
        }
    }

    if (nsamples > *pcmoutsize) {
        DEBUGF("decode_dvi_adpcm: output buffer overflow!\n");
        return CODEC_ERROR;
    }
    *pcmoutsize = nsamples;
    if (n != 0) {
        DEBUGF("decode_dvi_adpcm: n=%d unprocessed bytes\n", n);
    }
    return CODEC_OK;
}

