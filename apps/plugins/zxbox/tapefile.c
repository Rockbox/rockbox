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

/* This module deals with the different tape file formats (.TAP and .TZX)  */
/* 'sptape.c' uses the functions provided by this module. */


#include "tapefile.h"
#include "tapef_p.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "inttypes.h"
#include "zxconfig.h"
#include "helpers.h"
#define max(x, y) ((x) > (y) ? (x) : (y))

#define DESC_LEN 256

char seg_desc[DESC_LEN];

#define TZXMAJPROG 1
#define TZXMINPROG 2

/*static FILE *tapefp = NULL;*/
static int tapefd=-1;

static dbyte segi, currsegi;
static int   segbeg;

static int   endtype, endnext, endplay;
static dbyte endpause;
static int finished;

static long firstseg_offs;

static struct tape_options tapeopt;

long tf_segoffs;
struct tapeinfo tf_tpi;

static dbyte loopctr, loopbeg;
static dbyte callctr, callbeg;

#define ST_NORM  0
#define ST_PSEQ  1
#define ST_DIRE  2
#define ST_MISC  3

#define PL_NONE    0
#define PL_PAUSE   1
#define PL_LEADER  2
#define PL_DATA    3
#define PL_END     4
#define PL_PSEQ    5
#define PL_DIRE    6

#define IMP_1MS   3500

static dbyte lead_pause;
static int playstate = PL_NONE;
static int currlev;

#define DEF_LEAD_PAUSE 2000

struct seginfo tf_cseg;

struct tzxblock {
  int type;
  int lenbytes;
  int lenmul;
  int hlen;        
};

#define NUMBLOCKID 0x60
/* changed NONE because of warinigs */
/*#define NONE 0*/
#define NONE 0,0,0,0
#define COMM 1
#define STAN 2


#define RBUFLEN 1024

static byte rbuf[RBUFLEN];

/* Table containing information on TZX blocks */

static struct tzxblock tzxb[NUMBLOCKID] = {
  { NONE },                    /* ID: 00 */
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },

  { NONE },                    /* ID: 08 */
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },

  { COMM, 2, 1, 0x04 },        /* ID: 10 */
  { COMM, 3, 1, 0x12 },
  { COMM, 0, 1, 0x04 },
  { COMM, 1, 2, 0x01 },
  { COMM, 3, 1, 0x0A },
  { COMM, 3, 1, 0x08 },
  { NONE },
  { NONE },

  { NONE },                    /* ID: 18 */
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },

  { COMM, 0, 1, 0x02 },        /* ID: 20 */
  { COMM, 1, 1, 0x01 },
  { COMM, 0, 1, 0x00 },
  { COMM, 0, 1, 0x02 },
  { COMM, 0, 1, 0x02 },
  { COMM, 0, 1, 0x00 },
  { COMM, 2, 2, 0x02 },
  { COMM, 0, 1, 0x00 },

  { COMM, 2, 1, 0x02 },        /* ID: 28 */
  { NONE },
  { STAN, 0, 1, 0x00 },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },

  { COMM, 1, 1, 0x01 },        /* ID: 30 */
  { COMM, 1, 1, 0x02 },
  { COMM, 2, 1, 0x02 },
  { COMM, 1, 3, 0x01 },
  { COMM, 0, 1, 0x08 },
  { COMM, 4, 1, 0x14 },
  { NONE },
  { NONE },

  { NONE },                    /* ID: 38 */
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },

  { COMM, 3, 1, 0x04 },        /* ID: 40 */
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },

  { NONE },                    /* ID: 48 */
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },

  { NONE },                    /* ID: 50 */
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE },

  { NONE },                    /* ID: 58 */
  { NONE },
  { COMM, 0, 1, 0x09 },
  { NONE },
  { NONE },
  { NONE },
  { NONE },
  { NONE }
};


#define PTRDIFF(pe, ps) ((int) (((long) (pe) - (long) (ps)) / sizeof(*pe)))

static char tzxheader[] = {'Z','X','T','a','p','e','!',0x1A};

static int readbuf(void *ptr, int size, /*FILE *fp*/ int fd)
{
    /*return (int) fread(ptr, 1, (size_t) size, tapefp);*/
    return (int) rb->read(fd, ptr, (size_t) size);
}

static void premature(struct seginfo *csp)
{
  csp->segtype = SEG_ERROR;
  rb->snprintf(seg_desc,DESC_LEN, "Premature end of segment");
}

static int read_tzx_header(byte *hb, struct seginfo *csp)
{
  int res;  
  int segid, seght;
  int lenoffs, lenbytes, lenmul, lenadd;
  int hlen;
  long length;
  byte *hip;
  
  segid = getc(tapefd);
  if(segid == EOF) {
    csp->segtype = SEG_END;
    rb->snprintf(seg_desc,DESC_LEN, "End of Tape");
    return 0;
  }
  
  hb[0] = (byte) segid;
  
  if(segid < NUMBLOCKID) seght = tzxb[segid].type;
  else seght = 0; /* was NONE here*/
  
  if(seght == COMM) {
    lenbytes = tzxb[segid].lenbytes;
    lenmul   = tzxb[segid].lenmul;
    hlen     = tzxb[segid].hlen;
    lenadd   = hlen;
    lenoffs  = hlen - lenbytes;
  }
  else {
    lenoffs = 0x00;
    lenbytes = 4;
    lenmul = 1;
    lenadd = 0x00;
    hlen = 0x04;
  }
  
  if(seght == STAN) hlen += tzxb[segid].hlen;

  hip = hb+1;
  res = readbuf(hip, hlen, tapefd);
  if(res != hlen) {
    premature(csp);
    return 0;
  }
  length = 0;
  for(;lenbytes; lenbytes--) 
    length = (length << 8) + hip[lenoffs + lenbytes - 1];
  
  length = (length * lenmul) + lenadd - hlen;
  
  csp->len = length;
  return 1;
}

static int read_tap_header(byte *hb, struct seginfo *csp)
{
  int res;

  res = readbuf(hb, 2, tapefd);
  if(res < 2) {
    if(res == 0) {
      csp->segtype = SEG_END;
      rb->snprintf(seg_desc,DESC_LEN, "End of Tape");
    }
    else premature(csp);
    return 0;
  }
  csp->len = DBYTE(hb, 0);
  return 1;
}

static int read_header(byte *hb, struct seginfo *csp)
{
  segbeg = 0;
  csp->ptr = 0;

  csp->segtype = SEG_OTHER;
  if(tf_tpi.type == TAP_TAP) 
    return read_tap_header(hb, csp);
  else if(tf_tpi.type == TAP_TZX) 
    return read_tzx_header(hb, csp);

  return 0;
}

static void isbeg(void)
{
  segbeg = 1;
  tf_cseg.len = tf_cseg.ptr = 0;
}


static int end_seg(struct seginfo *csp)
{
  if(!segbeg) {
    if(csp->len != csp->ptr) {
      /*fseek(tapefp, tf_cseg.len - tf_cseg.ptr - 1, SEEK_CUR);*/
        rb->lseek(tapefd, tf_cseg.len - tf_cseg.ptr - 1, SEEK_CUR);

      if(getc(tapefd) == EOF) {
        premature(csp);
        return 0;
      }
    }
    segi++;
    isbeg();
  }
  playstate = PL_NONE;
  return 1;
}


static int jump_to_segment(int newsegi, struct seginfo *csp)
{
  if(newsegi <= segi) {
    segi = 0;
    isbeg();
    /*fseek(tapefp, firstseg_offs, SEEK_SET);*/
    rb->lseek(tapefd, firstseg_offs, SEEK_SET);
  }
  else if(!end_seg(csp)) return 0;

  while(segi != newsegi) {
    if(!read_header(rbuf, csp)) return 0;
    if(!end_seg(csp)) return 0;
  }
  return 1;
}


static int next_data(void)
{
  int res;
  if(tf_cseg.ptr == tf_cseg.len) return DAT_END;
  
  res = getc(tapefd);
  if(res == EOF) {
    rb->snprintf(seg_desc, DESC_LEN,"Premature end of segment");
    return DAT_ERR;
  }
  tf_cseg.ptr++;
  return res;
}


static void normal_segment(struct seginfo *csp)
{
  rb->snprintf(seg_desc,DESC_LEN, "Data");
  csp->type    = ST_NORM;
  csp->segtype = SEG_DATA;
  csp->pulse   = 2168;   /* 2016 */
  csp->num     = 3220;
  csp->sync1p  = 667;
  csp->sync2p  = 735;
  csp->zerop   = 855;   /* 672 */
  csp->onep    = 1710;  /* 1568 */
  csp->bused   = 8;
}


static int interpret_tzx_header(byte *hb, struct seginfo *csp)
{
  int res;
  int segid;
  byte *hip;
  int offs;
  dbyte dtmp;

  segid = hb[0];
  hip = hb+1;
  
  switch(segid) {
  case 0x10:
    normal_segment(csp);
    csp->pause   = DBYTE(hip, 0x00);
    break;
    
  case 0x11:
    rb->snprintf(seg_desc,DESC_LEN, "Turbo Data");
    csp->type    = ST_NORM;
    csp->segtype = SEG_DATA_TURBO;
    csp->pulse   = DBYTE(hip, 0x00);
    csp->sync1p  = DBYTE(hip, 0x02);
    csp->sync2p  = DBYTE(hip, 0x04);
    csp->zerop   = DBYTE(hip, 0x06);
    csp->onep    = DBYTE(hip, 0x08);
    csp->num     = DBYTE(hip, 0x0A);
    csp->bused   = BYTE(hip, 0x0C);
    csp->pause   = DBYTE(hip, 0x0D);
    break;
    
  case 0x12:
    rb->snprintf(seg_desc,DESC_LEN, "Pure Tone");
    csp->type    = ST_NORM;
    csp->segtype = SEG_OTHER;
    csp->pulse   = DBYTE(hip, 0x00);
    csp->num     = DBYTE(hip, 0x02);
    csp->sync1p  = 0;
    csp->sync2p  = 0;
    csp->zerop   = 0;
    csp->onep    = 0;
    csp->bused   = 0;
    csp->pause   = 0;
    break;
    
  case 0x13:
    rb->snprintf(seg_desc,DESC_LEN, "Pulse Sequence");
    csp->type    = ST_PSEQ;
    csp->segtype = SEG_OTHER;
    csp->pause   = 0;
    break;
    
  case 0x14:
    rb->snprintf(seg_desc,DESC_LEN, "Pure Data");
    csp->type    = ST_NORM;
    csp->segtype = SEG_DATA_PURE;
    csp->zerop   = DBYTE(hip, 0x00);
    csp->onep    = DBYTE(hip, 0x02);
    csp->bused   = BYTE(hip, 0x04);
    csp->pause   = DBYTE(hip, 0x05);
    csp->pulse   = 0;
    csp->num     = 0;
    csp->sync1p  = 0;
    csp->sync2p  = 0;
    break;
    
  case 0x15:
    rb->snprintf(seg_desc,DESC_LEN, "Direct Recording");
    csp->type    = ST_DIRE;
    csp->segtype = SEG_OTHER; 
    csp->pulse   = DBYTE(hip, 0x00);
    csp->pause   = DBYTE(hip, 0x02);
    csp->bused   = BYTE(hip, 0x04);
    break;
    
  case 0x20:
    dtmp = DBYTE(hip, 0x00);
    if(dtmp == 0) {
      if(!tapeopt.stoppause) {
    csp->type = ST_MISC;
    csp->segtype = SEG_STOP;
      }
      else {
    csp->pause = tapeopt.stoppause * 1000;
    csp->type = ST_NORM;
    csp->segtype = SEG_PAUSE;
      }
      rb->snprintf(seg_desc,DESC_LEN, "Stop the Tape Mark");
    }
    else {
      csp->pause = dtmp;
      csp->type = ST_NORM;
      csp->segtype = SEG_PAUSE;
      rb->snprintf(seg_desc,DESC_LEN, "Pause for %i.%03is", 
          csp->pause / 1000, csp->pause % 1000);
    }
    csp->pulse   = 0;
    csp->num     = 0;
    csp->sync1p  = 0;
    csp->sync2p  = 0;
    csp->zerop   = 0;
    csp->onep    = 0;
    csp->bused   = 0;
    break;
    
  case 0x21:
    csp->type    = ST_MISC;
    csp->segtype = SEG_GRP_BEG;
    res = readbuf(rbuf, csp->len, tapefd);
    if(res != (int) csp->len) {
      premature(csp);
      return 0;
    }
    csp->ptr += csp->len;
    {
      int blen;
      rb->snprintf(seg_desc,DESC_LEN, "Begin Group: ");
      blen = (int) rb->strlen(seg_desc);
      rb->strlcpy(seg_desc+blen, (char *) rbuf, (unsigned) csp->len + 1);
    }
    break;

  case 0x22:
    rb->snprintf(seg_desc,DESC_LEN, "End Group");
    csp->type    = ST_MISC;
    csp->segtype = SEG_GRP_END;
    break;

  case 0x23:
    offs = (signed short) DBYTE(hip, 0x00);
    if(offs == 0) {
      rb->snprintf(seg_desc,DESC_LEN, "Infinite loop");
      csp->type = ST_MISC;
      csp->segtype = SEG_STOP;
    }
    else {
      csp->type = ST_MISC;
      csp->segtype = SEG_SKIP;
      rb->snprintf(seg_desc,DESC_LEN, "Jump to %i", segi+offs);
      jump_to_segment(segi + offs, csp);
    }
    break;

  case 0x24:
    loopctr = DBYTE(hip, 0x00);
    rb->snprintf(seg_desc,DESC_LEN, "Loop %i times", loopctr);
    loopbeg = segi+1;
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    break;

  case 0x25:
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    if(loopctr) loopctr--;
    if(loopctr) {
      jump_to_segment(loopbeg, csp);
      rb->snprintf(seg_desc,DESC_LEN, "Loop to: %i", loopbeg);
    }
    else rb->snprintf(seg_desc,DESC_LEN, "Loop End");
    break;

  case 0x26:
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    dtmp = DBYTE(hip, 0x00);
    if(callctr < dtmp) {
      int offset;
      callbeg = segi;
      /*fseek(tapefp, callctr*2, SEEK_CUR);*/
      rb->lseek(tapefd, callctr*2, SEEK_CUR);
      csp->ptr += callctr*2;
      res = readbuf(rbuf, 2, tapefd);
      if(res != 2) {
    premature(csp);
    return 0;
      }
      csp->ptr += 2;
      offset = (signed short) DBYTE(rbuf, 0x00);
      rb->snprintf(seg_desc,DESC_LEN, "Call to %i", segi+offset);
      jump_to_segment(segi+offset, csp);
      callctr++;
    }
    else {
      callctr = 0;
      rb->snprintf(seg_desc,DESC_LEN, "Call Sequence End");
    }
    break;

  case 0x27:
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    rb->snprintf(seg_desc,DESC_LEN, "Return");
    if(callctr > 0) jump_to_segment(callbeg, csp);
    break;

  case 0x28:
    rb->snprintf(seg_desc,DESC_LEN, "Selection (Not yet supported)");
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    break;

  case 0x2A:
    if(tapeopt.machine == MACHINE_48) { 
      rb->snprintf(seg_desc,DESC_LEN, "Stop the Tape in 48k Mode (Stopped)");
      csp->type = ST_MISC;
      csp->segtype = SEG_STOP;
    }
    else {
      rb->snprintf(seg_desc,DESC_LEN, "Stop the Tape in 48k Mode (Not Stopped)");
      csp->type = ST_MISC;
      csp->segtype = SEG_SKIP;
    }
    break;

  case 0x31:
  case 0x30:
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    res = readbuf(rbuf, csp->len, tapefd);
    if(res != (int) csp->len) {
      premature(csp);
      return 0;
    }
    csp->ptr += csp->len;
    rb->strlcpy(seg_desc, (char *) rbuf, (unsigned) csp->len + 1);
    break;

  case 0x32:
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    {
      int numstr, i;
      
      i = 0;
      numstr = next_data();
      for(;numstr > 0; numstr--) {
    int tlen, tid, b;

    tid = next_data();
    tlen = next_data();
    if(tid < 0 || tlen < 0) return 0;
    
    for(; tlen; tlen--) {
      b = next_data();
      if(b < 0) return 0;
      seg_desc[i++] = b;
    }
    seg_desc[i++] = '\n';
      }
      seg_desc[i] = '\0';
    }
    break;
    
  case 0x33:
    rb->snprintf(seg_desc,DESC_LEN, "Hardware Information (Not yet supported)");
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    break;

  case 0x34:
    rb->snprintf(seg_desc, DESC_LEN,"Emulation Information (Not yet supported)");
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    break;

  case 0x35:
    rb->snprintf(seg_desc,DESC_LEN, "Custom Information (Not yet supported)");
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    break;

  case 0x40:
    rb->snprintf(seg_desc, DESC_LEN,"Snapshot (Not yet supported)");
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    break;

  case 0x5A:
    rb->snprintf(seg_desc, DESC_LEN,"Tapefile Concatenation Point");
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    
  default:
    csp->type    = ST_MISC;
    csp->segtype = SEG_SKIP;
    rb->snprintf(seg_desc,DESC_LEN, "Unknown TZX block (id: %02X, version: %i.%02i)", 
        segid, tf_tpi.tzxmajver, tf_tpi.tzxminver);
    break;
  }

  return 1;
}

static int interpret_header(byte *hb, struct seginfo *csp)
{
  if(tf_tpi.type == TAP_TAP) {
    normal_segment(csp);
    csp->pause   = DEF_LEAD_PAUSE;

    return 1;
  }
  else if(tf_tpi.type == TAP_TZX) 
    return interpret_tzx_header(hb, csp);

  return 0;
}  

byte *tf_get_block(int i)
{
  seg_desc[0] = '\0';

  if(jump_to_segment(i, &tf_cseg)) {
    tf_segoffs = ftell(tapefd);

    if(read_header(rbuf, &tf_cseg) && 
       interpret_header(rbuf, &tf_cseg)) return rbuf;
  }
  return NULL;
}


int next_byte(void)
{
  playstate = PL_NONE;
  return next_data();
}

#define DPULSE(v1,v2) (*impbuf++=(v1), *impbuf++=(v2), timelen-=(v1)+(v2))
#define PULSE(v) (*impbuf++=(v), currlev = !currlev, timelen-=(v))

int next_imps(unsigned short *impbuf, int buflen, long timelen)
{
  static int toput;
  static int bitrem;
  static dbyte dirpulse;
  unsigned short *impbufend, *impbufstart;
  
  impbufstart = impbuf;
  impbufend = impbuf + buflen;
  
  while(impbuf < impbufend - 1 && timelen > 0) {
    switch(playstate) {
      
    case PL_PAUSE:
      if(currlev && lead_pause) {
    PULSE(IMP_1MS);
    lead_pause --;
      }
      else if(lead_pause > 10) {
    if(tapeopt.blanknoise && !(rb->rand() % 64)) 
      DPULSE(IMP_1MS * 10 - 1000, 1000); 
    else 
      DPULSE(IMP_1MS * 10, 0);
    lead_pause -= 10;
      }
      else if(lead_pause) {
    DPULSE(IMP_1MS, 0);
    lead_pause --;
      }
      else {
    if(tf_cseg.num || tf_cseg.sync1p || tf_cseg.sync2p ||
       tf_cseg.ptr != tf_cseg.len) finished = 0;

    switch (tf_cseg.type) {
    case ST_NORM: playstate = PL_LEADER; break;
    case ST_DIRE: playstate = PL_DIRE;   dirpulse = 0; break;
    case ST_PSEQ: playstate = PL_PSEQ;   break;
    default:      playstate = PL_NONE;
    }
      }
      break;

    case PL_LEADER:
      if(tf_cseg.num >= 2) {
    DPULSE(tf_cseg.pulse, tf_cseg.pulse);
    tf_cseg.num -= 2;
      }
      else
      if(tf_cseg.num) {
    PULSE(tf_cseg.pulse);
    tf_cseg.num --;
      }
      else { /* PL_SYNC */
    if(tf_cseg.sync1p || tf_cseg.sync2p) 
      DPULSE(tf_cseg.sync1p, tf_cseg.sync2p);
    bitrem = 0;
    playstate = PL_DATA;
      }
      break;
      
    case PL_DATA:
      if(!bitrem) {
    toput = next_data();
    if(toput < 0) {
      playstate = PL_END;
      break;
    }
    if(tf_cseg.ptr != tf_cseg.len) {
      if(timelen > 16 * max(tf_cseg.onep, tf_cseg.zerop) && 
         impbuf <= impbufend - 16) {
        int p1, p2, br, tp;
        
        p1 = tf_cseg.onep;
        p2 = tf_cseg.zerop;
        br = 8;
        tp = toput;
        
        while(br) {
          if(tp & 0x80) DPULSE(p1, p1);
          else          DPULSE(p2, p2);
          br--;
          tp <<= 1;
        }
        bitrem = 0;
        break;
      }
      bitrem = 8;
    }
    else {
      bitrem = tf_cseg.bused;
      if(!bitrem) break;
    }
      }
      if(toput & 0x80) DPULSE(tf_cseg.onep, tf_cseg.onep);
      else             DPULSE(tf_cseg.zerop, tf_cseg.zerop);
      bitrem--, toput <<= 1;
      break;

    case PL_PSEQ:
      {
    int b1, b2;
    dbyte pulse1, pulse2;
    b1 = next_data();
    b2 = next_data();
    if(b1 < 0 || b2 < 0) {
      playstate = PL_END;
      break;
    }
    pulse1 = b1 + (b2 << 8);

    b1 = next_data();
    b2 = next_data();
    if(b1 < 0 || b2 < 0) {
      PULSE(pulse1);
      playstate = PL_END;
      break;
    }
    pulse2 = b1 + (b2 << 8);
    DPULSE(pulse1, pulse2);
      }
      break;

    case PL_DIRE:
      for(;;) {
    if(!bitrem) {
      toput = next_data();
      if(toput < 0) {
        playstate = PL_END;
        DPULSE(dirpulse, 0);
        break;
      }
      if(tf_cseg.ptr != tf_cseg.len) bitrem = 8;
      else {
        bitrem = tf_cseg.bused;
        if(!bitrem) break;
      }
    }
    bitrem--;
    toput <<= 1;
    if(((toput & 0x0100) ^ (currlev ? 0x0100 : 0x00))) {
      PULSE(dirpulse);
      dirpulse = tf_cseg.pulse;
      break;
    }
    dirpulse += tf_cseg.pulse;
    if(dirpulse >= 0x8000) {
      DPULSE(dirpulse, 0);
      dirpulse = 0;
      break;
    }
      }
      break;

    case PL_END:
      if(tf_cseg.pause) {
    PULSE(IMP_1MS);
    tf_cseg.pause--;
    if(currlev) PULSE(0);
    finished = 1;
      }
      playstate = PL_NONE;
      break;
      
    case PL_NONE:
    default:
      return PTRDIFF(impbuf, impbufstart);
    }
  }

  return PTRDIFF(impbuf, impbufstart);
}


int next_segment(void)
{
  if(endnext) {
    endnext = 0;
    tf_cseg.segtype = endtype;
    tf_cseg.pause = endpause;
    playstate = endplay;
    return tf_cseg.segtype;
  }

  seg_desc[0] = '\0';
  lead_pause = tf_cseg.pause;
  
  if(end_seg(&tf_cseg)) {
    currsegi = segi;
    if(read_header(rbuf, &tf_cseg)) interpret_header(rbuf, &tf_cseg);
  }

  if(tf_cseg.segtype >= SEG_DATA) {
    playstate = PL_PAUSE;
    if(lead_pause) finished = 1;
  }
  else playstate = PL_NONE;
  
  if(tf_cseg.segtype <= SEG_STOP && !finished) {
    endnext = 1;
    endtype = tf_cseg.segtype;
    endpause = tf_cseg.pause;
    endplay  = playstate;
    if(lead_pause > 0) lead_pause--;

    tf_cseg.pause = 1;
    tf_cseg.segtype = SEG_VIRTUAL;
    playstate = PL_END;
  }

  return tf_cseg.segtype;
}

int goto_segment(int at_seg)
{
  int res;

  res = jump_to_segment(at_seg, &tf_cseg);
  tf_cseg.pause = DEF_LEAD_PAUSE;

  return res;
}

unsigned segment_pos(void)
{
  return currsegi;
}

void close_tapefile(void)
{
  if(tapefd != -1) {
    playstate = PL_NONE;
    rb->close(tapefd);
    tapefd = -1;
  }
}

int open_tapefile(char *name, int type)
{
  int res;
  int ok;

  seg_desc[0] = '\0';
  currlev = 0;

  if(type != TAP_TAP && type != TAP_TZX) {
    rb->snprintf(seg_desc,DESC_LEN, "Illegal tape type");
    return 0;
  }

  /*tapefp = fopen(name, "rb");*/
  tapefd = rb->open(name, O_RDONLY);
  if(tapefd < 0 ) {
    /*rb->snprintf(seg_desc,DESC_LEN, "Could not open `%s': %s", name, strerror(errno));*/
    return 0;
  }
  
  tf_tpi.type = type;
  tf_cseg.pause = DEF_LEAD_PAUSE;
  INITTAPEOPT(tapeopt);

  currsegi = segi = 0;
  isbeg();

  firstseg_offs = 0;
  
  ok = 1;
  
  if(tf_tpi.type == TAP_TZX) {
    
    firstseg_offs = 10;
    res = readbuf(rbuf, 10, tapefd);
    if(res == 10 && rb->strncasecmp((char *)rbuf, tzxheader, 8) == 0) {
      tf_tpi.tzxmajver = rbuf[8];
      tf_tpi.tzxminver = rbuf[9];
      
      if(tf_tpi.tzxmajver > TZXMAJPROG) {
    rb->snprintf(seg_desc, DESC_LEN,
        "Cannot handle TZX file version (%i.%02i)", 
        tf_tpi.tzxmajver, tf_tpi.tzxminver);
    ok = 0;
      }
    }
    else {
      rb->snprintf(seg_desc,DESC_LEN, "Illegal TZX file header");
      ok = 0;
    }
  }
  
  if(!ok) {
    close_tapefile();
    return 0;
  }
  endnext = 0;

  loopctr = 0;
  callctr = 0;

  return 1;
}

int get_level(void)
{
  return currlev;
}

long get_seglen(void)
{
  return tf_cseg.len;
}

long get_segpos(void)
{
  return tf_cseg.ptr;
}

void set_tapefile_options(struct tape_options *to)
{
  rb->memcpy(&tapeopt, to, sizeof(tapeopt));
}

