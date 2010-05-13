/*
 * ttalib.h
 *
 * Description:  TTAv1 player library prototypes
 * Developed by: Alexander Djourik <ald@true-audio.com>
 *               Pavel Zhilin <pzh@true-audio.com>
 *
 * Copyright (c) 2004 True Audio Software. All rights reserved.
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the True Audio Software nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TTALIB_H_
#define TTALIB_H_

#define MAX_BPS         24   /* Max supported Bit resolution */
#define MAX_NCH         2    /* Max supported number of channels (Rockbox changes: 8 -> 2) */

#ifndef MAXLINE
#define MAX_LINE        1024
#endif

/* decoded pcm sample depth (sample 28bit + sign 1bit) */
#define TTA_OUTPUT_DEPTH 29

/* return codes */
#define NO_ERROR        0    /* No errors found               */
#define OPEN_ERROR      1    /* Can't open file               */
#define FORMAT_ERROR    2    /* Unknown TTA format version    */
#define PLAYER_ERROR    3    /* Not supported file format     */
#define FILE_ERROR      4    /* File is corrupted             */
#define READ_ERROR      5    /* Can't read from file          */
#define MEMORY_ERROR    6    /* Insufficient memory available */

/* Rockbox speciffic: does not use FRAME_TIME */
/* #define FRAME_TIME           1.04489795918367346939 */
#define MULTIPLY_FRAME_TIME(x)  (256 * (x) / 245) /* = FRAME_TIME * x */
#define SEEK_STEP               (int)MULTIPLY_FRAME_TIME(1000) /* (FRAME_TIME * 1000) */

#define ISO_BUFFER_LENGTH       (1024*32)
#define ISO_NBUFFERS            (8)
#define ISO_BUFFERS_SIZE        (4096) /* (ISO_BUFFER_LENGTH*ISO_NBUFFERS) */
#define PCM_BUFFER_LENGTH       (4608)
#define MAX_SEEK_TABLE_SIZE     (4096)

typedef struct {
    /* FILE        *HANDLE;     // file handle       (Rockbox does not use)   */
    unsigned int   FILESIZE;    /* compressed size                            */
    unsigned short NCH;         /* number of channels                         */
    unsigned short BPS;         /* bits per sample                            */
    unsigned short BSIZE;       /* byte size                                  */
    unsigned short FORMAT;      /* audio format                               */
    unsigned int   SAMPLERATE;  /* samplerate (sps)                           */
    unsigned int   DATALENGTH;  /* data length in samples                     */
    unsigned int   FRAMELEN;    /* frame length                               */
    unsigned int   LENGTH;      /* playback time (sec)                        */
    unsigned int   STATE;       /* return code                                */
    unsigned int   DATAPOS;     /* size of ID3v2 header                       */
    unsigned int   BITRATE;     /* average bitrate (kbps)                     */
    /* double      COMPRESS;    // compression ratio (Rockbox does not use)   */
    /* id3_info    ID3;         // ID3 information   (Rockbox does not use)   */
} tta_info;

enum tta_seek_type
{
   TTA_SEEK_TIME,
   TTA_SEEK_POS,
};

/*********************** Library functions *************************/
/* Rockbox speciffic: open_tta_file() does not use */

/* Rockbox speciffic: It is used in place of open_tta_file(). */
int set_tta_info(              // FUNCTION: set tta file info structure
        tta_info *info);       // file info structure
/*
 * RETURN VALUE
 * This function returns 0 if success. Otherwise, -1 is returned
 * and the variable STATE of the currently using info structure
 * is set to indicate the error.
 *
 */

/* Rockbox speciffic: close_tta_file() does not use */

/* Rockbox speciffic: set_position() change arguments and return value. */
/*
 * FUNCTION: sets playback position
 * pos: seek position
 *           seek_time_ms / SEEK_STEP (when type is TTA_SEEK_TIME)
 *           file position            (when type is TTA_SEEK_POS)
 */
int set_position (
        unsigned int pos,
        enum tta_seek_type type);

/*
 * RETURN VALUE
 * This function returns the seeked data position (>= 0) if success. Otherwise, -1 is returned
 * and the variable STATE of the currently using info structure
 * is set to indicate the error.
 *
 */

int    player_init (        // FUNCTION: initializes TTA player
           tta_info *info); // file info structure
/*
 * RETURN VALUE
 * This function returns 0 if success. Otherwise, -1 is returned
 * and the variable STATE of the currently using info structure
 * is set to indicate the error.
 *
 */

void    player_stop (void);     // FUNCTION: destroys memory pools

/* Rockbox speciffic: unsigned char -> int32_t */
int    get_samples (            // FUNCTION: decode PCM_BUFFER_LENGTH samples
           int32_t  *buffer);   // into the current PCM buffer position

/*
 * RETURN VALUE
 * This function returns the number of samples successfully decoded.
 * Otherwise, -1 is returned and the variable STATE of the currently
 * using info structure is set to indicate the error.
 *
 */

const char *get_error_str (int error); // FUNCTION: get error description

#endif /* TTALIB_H_ */
