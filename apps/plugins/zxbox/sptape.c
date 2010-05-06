/*
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "zxmisc.h"

#include "sptape.h"
#include "tapefile.h"

#include "spperif.h"
#include "z80.h"
#include "interf.h"
#include "spconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAXLINELEN 256

int spt_auto_stop = 1;

static int playing = 0;
static int paused = 0;

static char tapename[MAXFILENAME];
static int tapetype;

#define EARBIT 0x40

#define IMPBUFLEN 1024

static int ingroup;
static int segtype;

static int lastdatak;

static int currseg;

static void stop_playing(void)
{
  if(playing) close_tapefile();
  playing = 0;
}

static void pause_playing(void)
{
  if(playing) {
    paused = 1;
    playing = 0;
  }
}

static void unpause_playing(void)
{
  if(paused) {
    paused = 0;
    playing = 1;
    segtype = SEG_END;
  }
}

#define MAXDESCLEN 256

static void put_seg_desc(void)
{
  if(segtype != SEG_VIRTUAL) {
    if(segtype > SEG_END) {
      if(!ingroup) {
    long len;
    int i, ml;
    char *me;
    
    len = get_seglen();
    
    me = msgbuf;
    rb->snprintf(me,MAXDESCLEN, "%4d: ", currseg);
    me = me+rb->strlen(me);
    if(segtype >= SEG_DATA && len) {
      rb->snprintf(me,MAXDESCLEN, "%5ld bytes, ", len);
      me = me+rb->strlen(me);
    }
    
    ml = 0;
    for(i = 0; seg_desc[i]; i++) {
      if(seg_desc[i] == '\n') {
        *me = '\0';
        put_msg(msgbuf);
        
        me = msgbuf;
        rb->snprintf(me,MAXDESCLEN, "      ");
        me = me+rb->strlen(me);
        ml = 0;
      }
      else {
        if(ml < MAXDESCLEN) *me++ = seg_desc[i];
        ml++;
      }
    }
    *me = '\0';
    put_msg(msgbuf);
      }
      else {
    rb->snprintf(msgbuf,MAXDESCLEN, "%4d:", currseg);
    put_tmp_msg(msgbuf);
      }
    }
    else put_msg(seg_desc);
  }
#ifdef DEBUG_TAPE
  else fprintf(stderr, "virtual   segment\n");
#endif    
}

static void get_next_segment(void)
{
  int propseg;
  
  do {
    propseg = 1;

    segtype = next_segment();
    currseg = segment_pos();

    put_seg_desc();
    
    switch(segtype) {
    case SEG_ERROR:
    case SEG_END:
      stop_playing();
      break;
      
    case SEG_STOP:
      pause_playing();
      put_msg(" * Tape paused; Press Ctrl-o to restart * ");
      break;
      
    case SEG_SKIP:
      propseg = 0;
      break;
      
    case SEG_GRP_BEG:
      ingroup = 1;
      propseg = 0;
      break;
      
    case SEG_GRP_END:
      ingroup = 0;
      propseg = 0;
      break;
    }
  } while(!propseg);
  
  lastdatak = 0;
}


void play_tape(void)
{
  static dbyte impbuf[IMPBUFLEN];

  static int clevel;
  static dbyte *impbufp;
  static int impbufrem;
  static long imprem;
  
  static int cleared_buffers = 1;
 
  int tsn;
  dbyte *ibp;
  byte *tsp;
  int ibr;
  long ir;
  int cl;
  signed char *op;
  int ov;
  int ca;

  tsp = sp_tape_impinfo;
  op  = sp_tape_sound;
  tsn = TMNUM;
  
  if(!playing) {
    if(cleared_buffers) return;

    sp_playing_tape = 0;

    if(!clevel) {
      ca = CHKTICK;
      clevel = ~clevel;
    }
    else {
      ca = 0;
      cleared_buffers = 1;
    }
    imprem = CHKTICK * TMNUM;
  }
  else if(!sp_playing_tape) {
    sp_playing_tape = 1;
    cleared_buffers = 0;
    
    impbufrem = 0;
    imprem = 0;
    clevel = get_level() ? ~(0) : 0;
    if(clevel) ca = 0;
    else ca = 1;
  }
  else ca = 0;

#ifdef DEBUG_TAPE
  if(((clevel ? 1 : 0) ^ 
      (DANM(ula_inport) & EARBIT ? 1 : 0) ^ 
      (DANM(imp_change) ? 1 : 0) ^ 
      (ca ? 1 : 0)) == 0) 
    fprintf(stderr, "Levels don't match %i %i\n", imprem, impbufrem);
#endif

  cl = clevel;
  ibr = impbufrem;
  ir = imprem;
  ibp = impbufp;

  if(cl) ov = CHKTICK/2;
  else ov = -(CHKTICK/2);

  do {
    if(ir > 0) {
      *tsp++ = ca;
      *op++ = ov;
      ir -= CHKTICK;
      tsn--;
      if(!tsn) goto done;

      if(cl) ov = CHKTICK/2;
      else   ov = -(CHKTICK/2);

      while(ir > 0) {
    *tsp++ = 0;
    *op++ = ov;
    ir -= CHKTICK;
    tsn--;
    if(!tsn) goto done;
      }
      ca = 0;
    }
    if(ibr) {
      if(!ca) {
    if(cl) {
      ov += ir; 
      ca = (CHKTICK/2) - ov + 1;
    }
    else {
      ov -= ir;
      ca = ov + (CHKTICK/2) + 1;
    }
      }
      else {
    ca = 0;
    if(cl) ov += ir;
    else   ov -= ir;
      }
      ir += *ibp++;
      ibr--;
      cl = ~cl;
    }
    else {
      ibp = impbuf;
      do {
    ibr = next_imps(impbuf, IMPBUFLEN, CHKTICK * tsn);
    if(ibr) break;
    get_next_segment();
    if(!playing) {
      if(!cl) {
        if(ca) ca = 0;
        else   ca = CHKTICK;
        cl = ~cl;
      }
      ir = tsn*CHKTICK;
      ov = -(CHKTICK/2);
      break;
    }
      } while(1);
    }

  } while(1);

 done:

  clevel = cl;
  impbufrem = ibr;
  imprem = ir;
  impbufp = ibp;

  if(segtype >= SEG_DATA) { 
    int datak;
    
    datak = (int) (get_segpos() / 1000);
    if(datak > lastdatak) {
      if(ingroup) rb->snprintf(msgbuf,MAXDESCLEN, "%4d: ", currseg);
      else        rb->snprintf(msgbuf,MAXDESCLEN,  "      ");
      rb->snprintf(msgbuf+rb->strlen(msgbuf),MAXDESCLEN,  "%3dk", datak);
      put_tmp_msg(msgbuf);

      lastdatak = datak;
    }
  }
}

/*

2168 
2168  (9-10)

667
735   (2-4)

855
855   (3-5)    

1710
1710  (7-9)

945   (4-5)

hosszu: 7..9
rovid:  2..5

*/

#define MICBIT 0x08


#define RC_NONE    0
#define RC_LEADER  1
#define RC_SYNC    2
#define RC_DATA    3

#define MAXLEN      TMNUM

#define LEADER_MIN  9
#define LEADER_MAX  10
#define SYNC_MIN    2
#define SYNC_MAX    4

#define BIT0_MIN    3
#define BIT0_MAX    5
#define BIT1_MIN    7
#define BIT1_MAX    9

#define LEADER_MIN_COUNT 512
#if 0
static int rec_segment;
static int rec_state = RC_NONE;
static byte *recbuf = NULL;
static const char *waitchars = "-\\|/";
#endif
static int recording = 0;



void rec_tape(void)
{
#if 0
  static byte lastmic = 0;
  static int lastlen = 0;
  static int whole;
  static int leadercount;
  static byte data;
  static byte parity;
  static int bitnum;
  static int bytecount;
  static int recbufsize;
  static int firsthalf;
  static int frameswait = 0;

  int tsl;
  byte *fep;
  int thishalf;


  if(!recording) return;

  for(fep = sp_fe_outport_time, tsl = TMNUM; tsl; fep++, tsl--) {
    lastlen++;
    if((*fep & MICBIT) == lastmic) {
      if(lastlen < MAXLEN) continue;
    }
    else lastmic = ~lastmic & MICBIT;
    
    switch(rec_state) {
    case RC_NONE:
      if(lastlen >= LEADER_MIN && lastlen <= LEADER_MAX) {
    rec_state = RC_LEADER;

    leadercount = 0;
    break;
      }
      if((frameswait++ & 15)) break;
      frameswait &= 0x3F;
      sprintf(msgbuf, "  %s: WAITING %c", 
          tapename, waitchars[(frameswait >> 4)]);
      put_tmp_msg(msgbuf);
      break;
    case RC_LEADER:
      if(lastlen >= LEADER_MIN && lastlen <= LEADER_MAX) {
    leadercount++;
    if(leadercount == LEADER_MIN_COUNT) {
      sprintf(msgbuf, "  %s: LEADER", tapename);
      put_tmp_msg(msgbuf);
    }
    break;
      }
      if(leadercount >= LEADER_MIN_COUNT && 
     lastlen >= SYNC_MIN && lastlen <= SYNC_MAX) rec_state = RC_SYNC;
      else rec_state = RC_NONE;
      break;
    case RC_SYNC:
      if(lastlen >= SYNC_MIN && lastlen <= SYNC_MAX) {
    rec_state = RC_DATA;
    whole = 0;
    data = 0;
    bitnum = 0;
    bytecount = 0;
    recbuf = NULL;
    recbufsize = 0;
    parity = 0;

    sprintf(msgbuf, "  %s: DATA", tapename);
    put_tmp_msg(msgbuf);
      }
      else rec_state = RC_NONE;
      break;
    case RC_DATA:
      thishalf = -1;
      if(lastlen >= BIT0_MIN && lastlen <= BIT0_MAX) thishalf = 0;
      else if(lastlen >= BIT1_MIN && lastlen <= BIT1_MAX) thishalf = 1;
      
      if(thishalf < 0 || (whole && firsthalf != thishalf)) {
    char filename[11];
    int filesize;
    int filetype;

    sprintf(msgbuf, "%s: %03d", tapename, rec_segment);
    if(bytecount >= 1) {
      sprintf(msgbuf+strlen(msgbuf),
          " %02X %5d %3s", recbuf[0], bytecount-2, 
         parity == 0 ? "OK" : "ERR");
      if(recbuf[0] == 0 && bytecount - 2 >= 17) {
        filetype = recbuf[1];
        strncpy(filename, (char*) recbuf+2, 10);
        filename[10] = '\0';
        filesize = recbuf[12] + (recbuf[13] << 8);
        
        sprintf(msgbuf+strlen(msgbuf),
            "  %02X %10s %5i", filetype, filename, filesize);
      }
    }
    put_msg(msgbuf);

    putc(bytecount & 0xFF, tapefp);
    putc((bytecount >> 8) & 0xFF, tapefp);
    
    fwrite(recbuf, 1, (size_t) bytecount, tapefp);
    fflush(tapefp);

    rec_segment++;
    free(recbuf);
    recbuf = NULL;
    rec_state = RC_NONE;
    break;
      }
      
      if(!whole) {
    whole = 1;
    firsthalf = thishalf;
      }
      else {
    whole = 0;
    data |= thishalf;
    bitnum++;
    
    if(bitnum == 8) {
      bitnum = 0;
      if(recbufsize <= bytecount) {
        recbufsize += 1024;
        recbuf = realloc(recbuf, (size_t) recbufsize);
        if(recbuf == NULL) {
          //fprintf(stderr, "Out of memory\n");
          exit(1);
        }
      }
      recbuf[bytecount] = data;
      parity = parity ^ data;
      data = 0;
      bytecount++;
      
      if(!(bytecount & 1023)) {
        sprintf(msgbuf, "  %s: DATA %i kB", 
            tapename, bytecount >> 10);
        put_tmp_msg(msgbuf);
      }
    }
    data <<= 1;
      }
      break;
    }

    lastlen = 0;
  }
#endif
}

static void stop_recording(void)
{
#if 0
  if(recording) {
    recording = 0;
    free(recbuf);
    recbuf = NULL;

    rb->close(tapefp);
  }
#endif
}

static void restart_playing(void)
{
  int res;
  struct tape_options tapeopt;

  if(tapetype < 0) {
    tapetype = TAP_TZX;
    res = open_tapefile(tapename, tapetype);
    if(!res) {
      tapetype = TAP_TAP;
      res = open_tapefile(tapename, tapetype);
    }
  }
  else res = open_tapefile(tapename, tapetype);

  if(!res) {
    put_msg(seg_desc);
    return;
  }

  INITTAPEOPT(tapeopt);
#ifndef DEBUG_Z80
  tapeopt.blanknoise = 1;
#endif
  set_tapefile_options(&tapeopt);

  if(currseg) {
    res = goto_segment(currseg);
    if(!res) {
      put_msg(seg_desc);
      return;
    }
  }

  playing = 1;
  segtype = SEG_END;
}


void pause_play(void)
{
  if(playing) {
    pause_playing();
    put_msg(" * Tape paused * ");
    goto_segment(currseg);
  }
  else unpause_playing();
}


void start_play_file_type(char *name, int seg, int type)
{
  int filetype = FT_TAPEFILE;

  rb->strlcpy(tapename, name, MAXFILENAME-10 + 1);

  currseg = seg;
  tapetype = type;
  
  spcf_find_file_type(tapename, &filetype, &tapetype);
  if(currseg < 0) currseg = 0;

  ingroup = 0;
  restart_playing();
}

void start_play(void)
{
  char *name;
  int t;
  int seg;

  if(playing || paused || recording) {
    put_msg(" * Stop the tape first! * ");
    return;
  }

  put_msg("Enter tape file path:");
  name = spif_get_tape_fileinfo(&seg, &t);
  if(name == NULL) return;

  start_play_file_type(name, seg, -1); 
}


void stop_play(void)
{

  if(playing || paused) {
    put_msg(" * Stopped playing * ");

    if(playing) stop_playing();
    if(paused) paused = 0;
  }
  else if(recording) {
#if 0
    sprintf(msgbuf, " * Stopped recording tape `%s' * ", tapename);
    put_msg(msgbuf);
#endif
    stop_recording();
  }
}

void start_rec(void)
{
#if 0
  char *name;
  
  if(playing || paused || recording) return;

  put_msg("Enter tape file to record (default: '.tap'):");

  name = spif_get_filename();
  if(name == NULL) return;

  strncpy(tapename, name, MAXFILENAME-10);
  tapename[MAXFILENAME-10] = '\0';
  
  if(!check_ext(tapename, "tap")) add_extension(tapename, "tap");
  
  tapefp = fopen(tapename, "ab");
  if(tapefp == NULL) {
    sprintf(msgbuf, "Could not open tape file `%s', %s",
        tapename, strerror(errno));
    put_msg(msgbuf);
    return;
  }
  
  recording = 1;
  rec_segment = 0;
  rec_state = RC_NONE;

  sprintf(msgbuf, 
      "Recordind tape file `%s'. To stop press Ctrl-s", tapename);
  put_msg(msgbuf);
#endif
}

#include "spkey_p.h"

#define CF  0x01
#define ZF  0x40

void qload(void)
{
  byte type, parity;
  dbyte length, dest, dtmp;
  int verify, success, firstbyte;
  int nextdata;

  if(recording) {
    put_msg("Can't quick load tape, because recording");
    return;
  }

  do {
    if(!playing) {
      if(paused) unpause_playing();
      else {
    start_play();
      }
    }
    if(!playing) {
      put_msg("Not quick loading tape");
      return;
    }
    while(playing && (segtype != SEG_DATA || get_segpos() > 0)) 
      get_next_segment();

  } while(!playing);

  dtmp = AFBK;
  AFBK = AF;
  AF = dtmp;
  
  type = RA;
  verify = !(RF & CF);
  length = DE;
  firstbyte = !(RF & ZF);
  dest = IX;
  
  parity = 0;
  success = 0;

  do {
    nextdata = next_byte();
    if(nextdata < 0) break;

    parity ^= nextdata;
    
    if(!length) {
      if(!parity) success = 1;
      break;
    }

    if(firstbyte) {
      firstbyte = 0;
      if(nextdata != type) break;
    }
    else {
      if(!verify) {
    if(dest >= 0x4000) DANM(mem)[dest] = nextdata;
      }
      else {
    if(DANM(mem)[dest] != nextdata) break;
      }
      dest++;
      length--;
    }
  } while(1);

  if(success) RF |= (CF | ZF);
  else RF &= ~(CF | ZF);
  IX = dest;
  DE = length;

  PC = SA_LD_RET;
  DANM(iff1) = DANM(iff2) = 1;
  DANM(haltstate) = 1;
  
  sp_init_screen_mark();

  if(spt_auto_stop) pause_playing();
}
