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

/* Various "helper functions" common to all the xxx2wav decoder plugins  */

/* the main data structure of the program */
typedef struct {
    int infile;
    int outfile;
    off_t curpos;
    off_t filesize;
    int samplerate;
    int bitspersample;
    int channels;
    int frames_decoded;
    unsigned long total_samples;
    unsigned long current_sample;
    unsigned long start_tick;
} file_info_struct;

#define MALLOC_BUFSIZE (512*1024)

extern int mem_ptr;
extern int bufsize;
extern unsigned char* mp3buf;     // The actual MP3 buffer from Rockbox
extern unsigned char* mallocbuf;  // 512K from the start of MP3 buffer
extern unsigned char* filebuf;    // The rest of the MP3 buffer

void* codec_malloc(size_t size);
void* codec_calloc(size_t nmemb, size_t size);
void* codec_alloca(size_t size);
void* codec_realloc(void* ptr, size_t size);
void codec_free(void* ptr);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void* memmove(const void *s1, const void *s2, size_t n);

void display_status(file_info_struct* file_info);
int local_init(char* infilename, char* outfilename, file_info_struct* file_info, struct plugin_api* rb);
void close_wav(file_info_struct* file_info);
