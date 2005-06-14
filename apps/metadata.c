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
#include "mp3data.h"
#include "logf.h"

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

/* Get metadata for track - return false if parsing showed problems with the
   file that would prevent playback. */

bool get_metadata(struct track_info* track, int fd, const char* trackname,
                  bool v1first) {
  unsigned long totalsamples,bytespersample,channels,bitspersample,numbytes;
  unsigned char* buf;
  int i,j,eof;
  int rc;
    
  /* Load codec specific track tag information. */
  switch (track->codectype) {
  case AFMT_MPA_L2:
  case AFMT_MPA_L3:
      /* Should check the return value. */
      mp3info(&track->id3, trackname, v1first);
      lseek(fd, 0, SEEK_SET);

      /* This is too slow to execute on some files. */
      get_mp3file_info(fd, &track->mp3data);
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
          lseek(fd, i, SEEK_CUR);

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

      /* Ogg stores integers in little-endian format. */
      track->id3.filesize=filesize(fd);
      track->id3.frequency=buf[40]|(buf[41]<<8)|(buf[42]<<16)|(buf[43]<<24);
      channels=buf[39];

      /* We now need to search for the last page in the file - identified by 
	   by ('O','g','g','S',0) and retrieve totalsamples */

      lseek(fd, -32*1024, SEEK_END);
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
            if (i < (j-10)) {
              totalsamples=(buf[i+6])|(buf[i+7]<<8)|(buf[i+8]<<16)|(buf[i+9]<<24);
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
      track->id3.frequency = 44100;

      if ((buf [20] | buf [21] | buf [22] | buf [23]) &&
          (buf [12] & buf [13] & buf [14] & buf [15]) != 0xff) {
              totalsamples = (buf[15] << 24) | (buf[14] << 16) | (buf[13] << 8) | buf[12];
              track->id3.length = (totalsamples + 220) / 441 * 10;
              track->id3.bitrate = filesize (fd) /
                  (track->id3.length / 8);
      }

      lseek (fd, 0, SEEK_SET);
      strncpy (track->id3.path, trackname, sizeof (track->id3.path));
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
