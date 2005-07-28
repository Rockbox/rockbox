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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "metadata.h"
#include "mp3_playback.h"
#include "logf.h"
#include "atoi.h"
#include "replaygain.h"
#include "debug.h"

/* Simple file type probing by looking filename extension. */
int probe_file_format(const char *filename)
{
    char *suffix;
    
    suffix = strrchr(filename, '.');
    if (suffix == NULL)
        return AFMT_UNKNOWN;
    suffix += 1;
    
    if (!strcasecmp("mp1", suffix))
        return AFMT_MPA_L1;
    else if (!strcasecmp("mp2", suffix))
        return AFMT_MPA_L2;
    else if (!strcasecmp("mpa", suffix))
        return AFMT_MPA_L2;
    else if (!strcasecmp("mp3", suffix))
        return AFMT_MPA_L3;
    else if (!strcasecmp("ogg", suffix))
        return AFMT_OGG_VORBIS;
    else if (!strcasecmp("wav", suffix))
        return AFMT_PCM_WAV;
    else if (!strcasecmp("flac", suffix))
        return AFMT_FLAC;
    else if (!strcasecmp("mpc", suffix))
        return AFMT_MPC;
    else if (!strcasecmp("aac", suffix))
        return AFMT_AAC;
    else if (!strcasecmp("ape", suffix))
        return AFMT_APE;
    else if (!strcasecmp("wma", suffix))
        return AFMT_WMA;
    else if ((!strcasecmp("a52", suffix)) || (!strcasecmp("ac3", suffix)))
        return AFMT_A52;
    else if (!strcasecmp("rm", suffix))
        return AFMT_REAL;
    else if (!strcasecmp("wv", suffix))
        return AFMT_WAVPACK;
        
    return AFMT_UNKNOWN;
        
}

unsigned short a52_bitrates[]={32,40,48,56,64,80,96,
                               112,128,160,192,224,256,320,
                               384,448,512,576,640};

/* Only store frame sizes for 44.1KHz - others are simply multiples 
   of the bitrate */
unsigned short a52_441framesizes[]=
      {69*2,70*2,87*2,88*2,104*2,105*2,121*2,122*2,
       139*2,140*2,174*2,175*2,208*2,209*2,243*2,244*2,
       278*2,279*2,348*2,349*2,417*2,418*2,487*2,488*2,
       557*2,558*2,696*2,697*2,835*2,836*2,975*2,976*2,
       1114*2,1115*2,1253*2,1254*2,1393*2,1394*2};

const long wavpack_sample_rates [] = { 6000, 8000, 9600, 11025, 12000, 16000,
    22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000, 192000 };

/* Get metadata for track - return false if parsing showed problems with the
   file that would prevent playback. */

static bool get_apetag_info (struct mp3entry *entry, int fd);

static bool get_vorbis_comments (struct mp3entry *entry, size_t bytes_remaining, int fd);

static void little_endian_to_native (void *data, char *format);

bool get_metadata(struct track_info* track, int fd, const char* trackname,
                  bool v1first) {
  unsigned long totalsamples,bytespersample,channels,bitspersample,numbytes;
  unsigned long serialno=0, last_serialno=0;
  int bytesperframe;
  unsigned char* buf;
  int i,j,eof;
  int rc;
  int segments; /* for Vorbis*/
  size_t bytes_remaining = 0; /* for Vorbis */

    
  /* Load codec specific track tag information. */
  switch (track->id3.codectype) {
  case AFMT_MPA_L1:
  case AFMT_MPA_L2:
  case AFMT_MPA_L3:
      /* Should check the return value. */
      mp3info(&track->id3, trackname, v1first);
      lseek(fd, 0, SEEK_SET);

      /*
      logf("T:%s", track->id3.title);
      logf("L:%d", track->id3.length);
      logf("O:%d", track->id3.first_frame_offset);
      logf("F:%d", track->id3.frequency);
      */
      track->taginfo_ready = true;
      break ;

  case AFMT_PCM_WAV:
      /* Use the trackname part of the id3 structure as a temporary buffer */
      buf=track->id3.path;

      lseek(fd, 0, SEEK_SET);

      rc = read(fd, buf, 44);
      if (rc < 44) {
        return false;
      }

      if ((memcmp(buf,"RIFF",4)!=0) || 
          (memcmp(&buf[8],"WAVEfmt",7)!=0)) {
        logf("%s is not a WAV file\n",trackname);
        return(false);
      }

      /* FIX: Correctly parse WAV header - we assume canonical 
         44-byte header */

      bitspersample=buf[34];
      channels=buf[22];

      if ((bitspersample!=16) || (channels != 2)) {
        logf("Unsupported WAV file - %d bitspersample, %d channels\n",
             bitspersample,channels);
        return(false);
      }

      bytespersample=((bitspersample/8)*channels);
      numbytes=(buf[40]|(buf[41]<<8)|(buf[42]<<16)|(buf[43]<<24));
      totalsamples=numbytes/bytespersample;

      track->id3.vbr=false;   /* All WAV files are CBR */
      track->id3.filesize=filesize(fd);
      track->id3.frequency=buf[24]|(buf[25]<<8)|(buf[26]<<16)|(buf[27]<<24);

      /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
      track->id3.length=(totalsamples/track->id3.frequency)*1000;
      track->id3.bitrate=(track->id3.frequency*bytespersample)/(1000/8);

      lseek(fd, 0, SEEK_SET);
      strncpy(track->id3.path,trackname,sizeof(track->id3.path));
      track->taginfo_ready = true;

      break;

  case AFMT_FLAC:
      /* A simple parser to read vital metadata from a FLAC file - length, frequency, bitrate etc. */
      /* This code should either be moved to a seperate file, or discarded in favour of the libFLAC code */
      /* The FLAC stream specification can be found at http://flac.sourceforge.net/format.html#stream */

      /* Use the trackname part of the id3 structure as a temporary buffer */
      buf=track->id3.path;

      lseek(fd, 0, SEEK_SET);

      rc = read(fd, buf, 4);
      if (rc < 4) {
        return false;
      }

      if (memcmp(buf,"fLaC",4)!=0) {          
        logf("%s is not a FLAC file\n",trackname);
        return(false);
      }

      while (1) {
        rc = read(fd, buf, 4);
        i = (buf[1]<<16)|(buf[2]<<8)|buf[3];  /* The length of the block */

        if ((buf[0]&0x7f)==0) {    /* 0 is the STREAMINFO block */
          rc = read(fd, buf, i);  /* FIXME: Don't trust the value of i */
          if (rc < 0) {
            return false;
          }
          track->id3.vbr=true;   /* All FLAC files are VBR */
          track->id3.filesize=filesize(fd);

          track->id3.frequency=(buf[10]<<12)|(buf[11]<<4)|((buf[12]&0xf0)>>4);

          /* NOT NEEDED: bitspersample=(((buf[12]&0x01)<<4)|((buf[13]&0xf0)>>4))+1; */

          /* totalsamples is a 36-bit field, but we assume <= 32 bits are used */
          totalsamples=(buf[14]<<24)|(buf[15]<<16)|(buf[16]<<8)|buf[17];

          /* Calculate track length (in ms) and estimate the bitrate (in kbit/s) */
          track->id3.length=(totalsamples/track->id3.frequency)*1000;
          track->id3.bitrate=(filesize(fd)*8)/track->id3.length;
        } else if ((buf[0]&0x7f)==4) {     /* 4 is the VORBIS_COMMENT block */

          /* The next i bytes of the file contain the VORBIS COMMENTS - just skip them for now. */
          //lseek(fd, i, SEEK_CUR);
          if (!get_vorbis_comments(&(track->id3), i, fd)) {
              return false;
          }

        } else {
          if (buf[0]&0x80) { /* If we have reached the last metadata block, abort. */
            break;
          } else {
            lseek(fd, i, SEEK_CUR);   /* Skip to next metadata block */
          }
        }
      }

      lseek(fd, 0, SEEK_SET);
      strncpy(track->id3.path,trackname,sizeof(track->id3.path));
      track->taginfo_ready = true;
      break;

  case AFMT_OGG_VORBIS:
      /* A simple parser to read vital metadata from an Ogg Vorbis file */

      /* An Ogg File is split into pages, each starting with the string 
         "OggS".  Each page has a timestamp (in PCM samples) referred to as
         the "granule position".

         An Ogg Vorbis has the following structure:
          1) Identification header (containing samplerate, numchannels, etc)
          2) Comment header - containing the Vorbis Comments
          3) Setup header - containing codec setup information
          4) Many audio packets...

      */

      /* Use the trackname part of the id3 structure as a temporary buffer */
      buf=track->id3.path;

      lseek(fd, 0, SEEK_SET);

      rc = read(fd, buf, 58);
      if (rc < 4) {
        return false;
      }

      if ((memcmp(buf,"OggS",4)!=0) || (memcmp(&buf[29],"vorbis",6)!=0)) {
        logf("%s is not an Ogg Vorbis file\n",trackname);
        return(false);
      }

      /* We need to ensure the serial number from this page is the
       * same as the one from the last page (since we only support
       * a single bitstream).
       */
      serialno = buf[14]|(buf[15]<<8)|(buf[16]<<16)|(buf[17]<<24);

      /* Ogg stores integers in little-endian format. */
      track->id3.filesize=filesize(fd);
      track->id3.frequency=buf[40]|(buf[41]<<8)|(buf[42]<<16)|(buf[43]<<24);
      channels=buf[39];

      /* Comments are in second Ogg page */
      if ( lseek(fd, 58, SEEK_SET) < 0 ) {
          return false;
      }

      /* Minimum header length for Ogg pages is 27 */
      if (read(fd, buf, 27) < 27) {
          return false;
      }

      if (memcmp(buf,"OggS",4)!=0) {
        logf("1: Not an Ogg Vorbis file");
        return(false);
      }

      segments=buf[26];
      /* read in segment table */
      if (read(fd, buf, segments) < segments) {
          return false;
      }

      /* The second packet in a vorbis stream is the comment packet.  It *may*
       * extend beyond the second page, but usually does not.  Here we find the
       * length of the comment packet (or the rest of the page if the comment
       * packet extends to the third page).
       */
      for (i = 0; i < segments; i++) {
          bytes_remaining += buf[i];
          /* The last segment of a packet is always < 255 bytes */
          if (buf[i] < 255) {
              break;
          }
      }

      /* Now read in packet header (type and id string) */
      if(read(fd, buf, 7) < 7) {
          return false;
      }

      /* The first byte of a packet is the packet type; comment packets are
       * type 3.
       */
      if ((buf[0] != 3) || (memcmp(buf + 1,"vorbis",6)!=0)) {
          logf("Not a vorbis comment packet");
          return false;
      }

      bytes_remaining -= 7;

      if ( !get_vorbis_comments(&(track->id3), bytes_remaining, fd) ) {
        logf("get_vorbis_comments failed");
        return(false);
      }

      /* We now need to search for the last page in the file - identified by 
       by ('O','g','g','S',0) and retrieve totalsamples */

      lseek(fd, -64*1024, SEEK_END);    /* A page is always < 64 kB */
      eof=0;
      j=0; /* The number of bytes currently in buffer */
      i=0;
      totalsamples=0;
      while (!eof) {
        rc = read(fd, &buf[j], MAX_PATH-j);
        if (rc <= 0) { 
          eof=1;
        } else { 
          j+=rc;
        }
        /* Inefficient (but simple) search */
        i=0;
        while (i < (j-5)) {
          if (memcmp(&buf[i],"OggS",5)==0) {
            if (i < (j-17)) {
              /* Note that this only reads the low 32 bits of a 64 bit value */
              totalsamples=(buf[i+6])|(buf[i+7]<<8)|(buf[i+8]<<16)|(buf[i+9]<<24);
              last_serialno=(buf[i+14])|(buf[i+15]<<8)|(buf[i+16]<<16)|(buf[i+17]<<24);
              j=0;  /* We can discard the rest of the buffer */
            } else {
              break;
            }
          } else {
            i++;
          }
        }
        if (i < (j-5)) {
          /* Move OggS to start of buffer */
          while(i>0) buf[i--]=buf[j--];
        } else {
          j=0;
        }
      }

      /* This file has mutiple vorbis bitstreams (or is corrupt) */
      /* FIXME we should display an error here */
      if (serialno != last_serialno) {
              track->taginfo_ready=false;
              logf("serialno mismatch");
              logf("%ld", serialno);
              logf("%ld", last_serialno);
              return false;
      }
  
      track->id3.samples=totalsamples;
      track->id3.length=(totalsamples/track->id3.frequency)*1000;

      /* The following calculation should use datasize, not filesize (i.e. exclude comments etc) */
      track->id3.bitrate=(filesize(fd)*8)/track->id3.length;
      track->id3.vbr=true;

      lseek(fd, 0, SEEK_SET);
      strncpy(track->id3.path,trackname,sizeof(track->id3.path));
      track->taginfo_ready = true;
      break;

  case AFMT_WAVPACK:
      /* A simple parser to read basic information from a WavPack file.
       * This will fail on WavPack files that don't have the WavPack header
       * as the first thing (i.e. self-extracting WavPack files) or WavPack
       * files that have so much extra RIFF data stored in the first block
       * that they don't have samples (very rare, I would think).
       */

      /* Use the trackname part of the id3 structure as a temporary buffer */
      buf=track->id3.path;

      lseek(fd, 0, SEEK_SET);

      rc = read(fd, buf, 32);
      if (rc < 32) {
          return false;
      }

      if (memcmp (buf, "wvpk", 4) != 0 || buf [9] != 4 || buf [8] < 2) {          
          logf ("%s is not a WavPack file\n", trackname);
          return (false);
      }

      track->id3.vbr = true;   /* All WavPack files are VBR */
      track->id3.filesize = filesize (fd);

      if ((buf [20] | buf [21] | buf [22] | buf [23]) &&
          (buf [12] & buf [13] & buf [14] & buf [15]) != 0xff) {
              int srindx = ((buf [26] >> 7) & 1) + ((buf [27] << 1) & 14);

              if (srindx == 15)
                  track->id3.frequency = 44100;
              else
                  track->id3.frequency = wavpack_sample_rates [srindx];

              totalsamples = (buf[15] << 24) | (buf[14] << 16) | (buf[13] << 8) | buf[12];
              track->id3.length = totalsamples / (track->id3.frequency / 100) * 10;
              track->id3.bitrate = filesize (fd) /
                  (track->id3.length / 8);
      }

      get_apetag_info (&track->id3, fd);    /* use any apetag info we find */
      lseek (fd, 0, SEEK_SET);
      strncpy (track->id3.path, trackname, sizeof (track->id3.path));
      track->taginfo_ready = true;
      break;

  case AFMT_A52:
      /* Use the trackname part of the id3 structure as a temporary buffer */
      buf=track->id3.path;

      lseek(fd, 0, SEEK_SET);

      /* We just need the first 5 bytes */
      rc = read(fd, buf, 5);
      if (rc < 5) {
        return false;
      }

      if ((buf[0]!=0x0b) || (buf[1]!=0x77)) { 
         logf("%s is not an A52/AC3 file\n",trackname);
         return false;
      }

      i = buf[4]&0x3e;
      if (i > 36) {
        logf("A52: Invalid frmsizecod: %d\n",i);
        return false;
      }
      track->id3.bitrate=a52_bitrates[i>>1];

      track->id3.vbr=false;
      track->id3.filesize = filesize (fd);

      switch (buf[4]&0xc0) {
        case 0x00: 
          track->id3.frequency=48000; 
          bytesperframe=track->id3.bitrate*2*2;
          break;
        case 0x40: 
          track->id3.frequency=44100;
          bytesperframe=a52_441framesizes[i];
          break;
        case 0x80: 
          track->id3.frequency=32000; 
          bytesperframe=track->id3.bitrate*3*2;
          break;
        default: 
          logf("A52: Invalid samplerate code: 0x%02x\n",buf[4]&0xc0);
          return false;
          break;
      }

      /* One A52 frame contains 6 blocks, each containing 256 samples */
      totalsamples=(track->filesize/bytesperframe)*6*256;

      track->id3.length=(totalsamples/track->id3.frequency)*1000;

      lseek(fd, 0, SEEK_SET);
      strncpy(track->id3.path,trackname,sizeof(track->id3.path));
      track->taginfo_ready = true;
      break;

  /* If we don't know how to read the metadata, just store the filename */
  default:
      strncpy(track->id3.path,trackname,sizeof(track->id3.path));
      track->taginfo_ready = true;
      break;
  }

  return true;
}

/************************* APE TAG HANDLING CODE ****************************/

/*
 * This is a first pass at APEv2 tag handling. I'm not sure if this should
 * reside here, but I wanted to modify as little as possible since I don't
 * have a feel for the complete system. It may be that APEv2 tags should be
 * added to the ID3 handling code in the firmware directory. APEv2 tags are
 * used in WavPack files and Musepack files by default, however they are
 * also used in MP3 files sometimes (by Foobar2000). Also, WavPack files can
 * also use ID3v1 tags (but not ID3v2), so it seems like some universal tag
 * handler might be a reasonable approach.
 *
 * This code does not currently handle APEv1 tags, but I believe that this
 * is not a problem because they were only used in Monkey's Audio files which
 * will probably never be playable in RockBox (and certainly not by this CPU).
 */

#define APETAG_HEADER_FORMAT "8LLLL"
#define APETAG_HEADER_LENGTH 32
#define APETAG_DATA_LIMIT 4096

struct apetag_header {
    char id [8];
    long version, length, item_count, flags;
    char res [8];
};

static struct apetag {
    struct apetag_header header;
    char data [APETAG_DATA_LIMIT];
} temp_apetag;

static int get_apetag_item (struct apetag *tag,
                            const char *item,
                            char *value,
                            int size);

static int load_apetag (int fd, struct apetag *tag);
static void UTF8ToAnsi (unsigned char *pUTF8);

/*
 * This function searches the specified file for an APEv2 tag and uses any
 * information found there to populate the appropriate fields in the specified
 * mp3entry structure. A temporary buffer is used to hold the tag during this
 * operation. For now, the actual string data that needs to be held during the
 * life of the track entry is stored in the "id3v2buf" field (which should not
 * be used for any file that has an APEv2 tag). This limits the total space
 * for the artist, title, album, composer and genre strings to 300 characters.
 */

static bool get_apetag_info (struct mp3entry *entry, int fd)
{
    int rem_space = sizeof (entry->id3v2buf), str_space;
    char *temp_buffer = entry->id3v2buf;

    if (rem_space <= 1 || !load_apetag (fd, &temp_apetag))
        return false;

    if (get_apetag_item (&temp_apetag, "year", temp_buffer, rem_space))
        entry->year = atoi (temp_buffer);

    if (get_apetag_item (&temp_apetag, "track", temp_buffer, rem_space))
        entry->tracknum = atoi (temp_buffer);

    if (get_apetag_item (&temp_apetag, "replaygain_track_peak", temp_buffer, rem_space))
        entry->track_peak = get_replaypeak (temp_buffer);

    if (get_apetag_item (&temp_apetag, "replaygain_album_peak", temp_buffer, rem_space))
        entry->album_peak = get_replaypeak (temp_buffer);

    if (rem_space > 1 &&
        get_apetag_item (&temp_apetag, "replaygain_track_gain", temp_buffer, rem_space)) {
            entry->track_gain = get_replaygain (entry->track_gain_string = temp_buffer);
            str_space = strlen (temp_buffer) + 1;
            temp_buffer += str_space;
            rem_space -= str_space;
    }

    if (rem_space > 1 &&
        get_apetag_item (&temp_apetag, "replaygain_album_gain", temp_buffer, rem_space)) {
            entry->album_gain = get_replaygain (entry->album_gain_string = temp_buffer);
            str_space = strlen (temp_buffer) + 1;
            temp_buffer += str_space;
            rem_space -= str_space;
    }

    if (rem_space > 1 &&
        get_apetag_item (&temp_apetag, "artist", temp_buffer, rem_space)) {
            UTF8ToAnsi (entry->artist = temp_buffer);
            str_space = strlen (temp_buffer) + 1;
            temp_buffer += str_space;
            rem_space -= str_space;
    }

    if (rem_space > 1 &&
        get_apetag_item (&temp_apetag, "title", temp_buffer, rem_space)) {
            UTF8ToAnsi (entry->title = temp_buffer);
            str_space = strlen (temp_buffer) + 1;
            temp_buffer += str_space;
            rem_space -= str_space;
    }

    if (rem_space > 1 &&
        get_apetag_item (&temp_apetag, "album", temp_buffer, rem_space)) {
            UTF8ToAnsi (entry->album = temp_buffer);
            str_space = strlen (temp_buffer) + 1;
            temp_buffer += str_space;
            rem_space -= str_space;
    }

    if (rem_space > 1 &&
        get_apetag_item (&temp_apetag, "genre", temp_buffer, rem_space)) {
            UTF8ToAnsi (entry->genre_string = temp_buffer);
            str_space = strlen (temp_buffer) + 1;
            temp_buffer += str_space;
            rem_space -= str_space;
    }

    if (rem_space > 1 &&
        get_apetag_item (&temp_apetag, "composer", temp_buffer, rem_space))
            UTF8ToAnsi (entry->composer = temp_buffer);

    return true;
}

/*
 * Helper function to convert little-endian structures to easily usable native
 * format using a format string (this does nothing on a little-endian machine).
 */ 

static void little_endian_to_native (void *data, char *format)
{
    unsigned char *cp = (unsigned char *) data;
    long temp;

    while (*format) {
        switch (*format) {
            case 'L':
                temp = cp [0] + ((long) cp [1] << 8) + ((long) cp [2] << 16) + ((long) cp [3] << 24);
                * (long *) cp = temp;
                cp += 4;
                break;

            case 'S':
                temp = cp [0] + (cp [1] << 8);
                * (short *) cp = (short) temp;
                cp += 2;
                break;

            default:
                if (*format >= '0' && *format <= '9')
                    cp += *format - '0';

                break;
        }

        format++;
    }
}

/*
 * Attempt to obtain the named string-type item from the specified APEv2 tag.
 * The tag value will be copied to "value" (including an appended terminating
 * NULL) and the length of the string (including the NULL) will be returned.
 * If the data will not fit in the specified "size" then it will be truncated
 * early (but still terminated). If the specified item is not found then 0 is
 * returned and written to the first character of "value". If "value" is
 * passed in as NULL, then the specified size is ignored and the actual size
 * required to store the value is returned.
 *
 * Note that this function does not work on binary tag data; only UTF-8
 * encoded strings. However, numeric data (like ReplayGain) is usually stored
 * as strings.
 *
 * Also, APEv2 tags may have multiple values for a given item and these will
 * all be copied to "value" with NULL separators (this is why the total data
 * size is returned). Of course, it is possible to ignore any additional
 * values by simply using up to the first NULL.
 */

static int get_apetag_item (struct apetag *tag,
                            const char *item,
                            char *value,
                            int size)
{
    if (value && size)
        *value = 0;

    if (tag->header.id [0] == 'A') {
        char *p = tag->data;
        char *q = p + tag->header.length - APETAG_HEADER_LENGTH;
        int i;

        for (i = 0; i < tag->header.item_count; ++i) {
            int vsize, flags, isize;

            vsize = * (long *) p; p += 4;
            flags = * (long *) p; p += 4;
            isize = strlen (p);

            little_endian_to_native (&vsize, "L");
            little_endian_to_native (&flags, "L");

            if (p + isize + vsize + 1 > q)
                break;

            if (isize && vsize && !stricmp (item, p) && !(flags & 6)) {

                if (value) {
                    if (vsize + 1 > size)
                        vsize = size - 1;

                    memcpy (value, p + isize + 1, vsize);
                    value [vsize] = 0;
                }

                return vsize + 1;
            }
            else
                p += isize + vsize + 1;
        }
    }

    return 0;
}

/*
 * Attempt to load an APEv2 tag from the specified file into the specified
 * structure. If the APEv2 tag will not fit into the predefined data size,
 * then the tag is not loaded. A return value of TRUE indicates success.
 */

static int load_apetag (int fd, struct apetag *tag)
{
    if (lseek (fd, -APETAG_HEADER_LENGTH, SEEK_END) == -1 ||
        read (fd, &tag->header, APETAG_HEADER_LENGTH) != APETAG_HEADER_LENGTH ||
        strncmp (tag->header.id, "APETAGEX", 8)) {
            tag->header.id [0] = 0;
            return false;
    }

    little_endian_to_native (&tag->header, APETAG_HEADER_FORMAT);

    if (tag->header.version == 2000 && tag->header.item_count &&
        tag->header.length > APETAG_HEADER_LENGTH &&
        tag->header.length < APETAG_DATA_LIMIT) {

            int data_size = tag->header.length - APETAG_HEADER_LENGTH;

            if (lseek (fd, -tag->header.length, SEEK_END) == -1 ||
                read (fd, tag->data, data_size) != data_size) {
                    tag->header.id [0] = 0;
                    return false;
            }
            else
                return true;
    }

    tag->header.id [0] = 0;
    return false;
}

/*
 * This is a *VERY* boneheaded attempt to convert UTF-8 unicode character
 * strings to ANSI. It simply maps the 16-bit Unicode characters that are
 * less than 0x100 directly to an 8-bit value, and turns all the rest into
 * question marks. This can be done "in-place" because the resulting string
 * can only get smaller.
 */

static void UTF8ToAnsi (unsigned char *pUTF8)
{
    unsigned char *pAnsi = pUTF8;
    unsigned short widechar = 0;
    int trail_bytes = 0;

    while (*pUTF8) {
        if (*pUTF8 & 0x80) {
            if (*pUTF8 & 0x40) {
                if (trail_bytes) {
                    trail_bytes = 0;
                    *pAnsi++ = widechar < 0x100 ? widechar : '?';
                }
                else {
                    char temp = *pUTF8;

                    while (temp & 0x80) {
                        trail_bytes++;
                        temp <<= 1;
                    }

                    widechar = temp >> trail_bytes--;
                }
            }
            else if (trail_bytes) {
                widechar = (widechar << 6) | (*pUTF8 & 0x3f);

                if (!--trail_bytes)
                    *pAnsi++ = widechar < 0x100 ? widechar : '?';
            }
        }
        else
            *pAnsi++ = *pUTF8;

        pUTF8++;
    }

    *pAnsi = 0;
}

/* This function extracts the information stored in the Vorbis comment header
 * and stores it in id3v2buf of the current track.  Currently the combined
 * lengths of title, genre, album, and artist must be no longer than 296 bytes
 * (the remaining 4 bytes are the null bytes at the end of the strings).  This
 * is wrong, since vorbis comments can be up to 2^32 - 1 bytes long.  In
 * practice I don't think this limitation will cause a problem.
 *
 * According to the docs, a vorbis bitstream *must* have a comment packet even
 * if that packet is empty.  Therefore if this function returns false the
 * bitstream is corrupt and shouldn't be used.
 *
 * Additionally, vorbis comments *may* take up more than one Ogg page, and this
 * only looks at the first page of comments.
 */
static bool get_vorbis_comments (struct mp3entry *entry, size_t bytes_remaining, int fd)
{
    int vendor_length;
    int comment_count;
    int comment_length;
    int i = 0;
    unsigned char temp[300];
    int buffer_remaining = sizeof(entry->id3v2buf) + sizeof(entry->id3v1buf);
    char *buffer = entry->id3v2buf;
    char **p = NULL;

    /* We've read in all header info, now start reading comments */

    /* Set id3v1 genre to 255 (effectively 'none'), otherwise tracks
     * without genre tags will show up as 'Blues'
     */
    entry->genre=255;

    if (read(fd, &vendor_length, 4) < 4) {
        return false;
    }
    little_endian_to_native(&vendor_length, "L");
    lseek(fd, vendor_length, SEEK_CUR);

    if (read(fd, &comment_count, 4) < 4) {
        return false;
    }
    little_endian_to_native(&comment_count, "L");
    bytes_remaining -= (vendor_length + 8);
    if ( bytes_remaining <= 0 ) {
        return true;
    }

    for ( i = 0; i < comment_count; i++ ) {
        int name_length = 0;

        if (bytes_remaining < 4) {
            break;
        }
        bytes_remaining -= 4;

        if (read(fd, &comment_length, 4) < 4) {
            return false;
        }

        little_endian_to_native(&comment_length, "L");

        /* Quit if we've passed the end of the page */
        if ( bytes_remaining < (unsigned)comment_length ) {
            break;
        }
        bytes_remaining -= (comment_length);

        /* Skip comment if it won't fit in buffer */
        if ( (unsigned int)comment_length >= sizeof(temp) ) {
            lseek(fd, comment_length, SEEK_CUR);
            continue;
        }

        if ( read(fd, temp, comment_length) < comment_length ) {
            return false;
        }

        temp[comment_length] = '\0';
        UTF8ToAnsi(temp);
        comment_length = strlen(temp);

        if (strncasecmp(temp, "TITLE=", 6) == 0) {
            name_length = 5;
            p = &(entry->title);
        } else if (strncasecmp(temp, "ALBUM=", 6) == 0) {
            name_length = 5;
            p = &(entry->album);
        } else if (strncasecmp(temp, "ARTIST=", 7) == 0) {
            name_length = 6;
            p = &(entry->artist);
        } else if (strncasecmp(temp, "COMPOSER=", 9) == 0) {
            name_length = 8;
            p = &(entry->composer);
        } else if (strncasecmp(temp, "GENRE=", 6) == 0) {
            name_length = 5;
            p = &(entry->genre_string);
        } else if (strncasecmp(temp, "DATE=", 5) == 0) {
            name_length = 4;
            p = &(entry->year_string);
        } else if (strncasecmp(temp, "TRACKNUMBER=", 12) == 0) {
            name_length = 11;
            p = &(entry->track_string);
        } else {
            int value_length = parse_replaygain(temp, NULL, entry, buffer, 
                buffer_remaining);
            buffer_remaining -= value_length;
            buffer += value_length;
            p = NULL;
        }

        if (p) {
            comment_length -= (name_length + 1);
            if ( comment_length < buffer_remaining ) {
                strncpy(buffer, temp + name_length + 1, comment_length);
                buffer[comment_length] = '\0';
                *p = buffer;
                buffer += comment_length + 1;
                buffer_remaining -= comment_length + 1;
            }
        }
    }

    /* Skip to the end of the block */
    if (bytes_remaining) {
        lseek(fd, bytes_remaining, SEEK_CUR);
    }

    return true;
}
