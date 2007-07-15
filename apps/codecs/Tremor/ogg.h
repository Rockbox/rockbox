/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2003    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: subsumed libogg includes

 ********************************************************************/
#ifndef _OGG_H
#define _OGG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "os_types.h"

typedef struct ogg_buffer_state{
  struct ogg_buffer    *unused_buffers;
  struct ogg_reference *unused_references;
  int                   outstanding;
  int                   shutdown;
} ogg_buffer_state;

typedef struct ogg_buffer {
  unsigned char      *data;
  long                size;
  int                 refcount;
  
  union {
    ogg_buffer_state  *owner;
    struct ogg_buffer *next;
  } ptr;
} ogg_buffer;

typedef struct ogg_reference {
  ogg_buffer    *buffer;
  long           begin;
  long           length;

  struct ogg_reference *next;
} ogg_reference;

typedef struct oggpack_buffer {
  int            headbit;
  unsigned char *headptr;
  long           headend;

  /* memory management */
  ogg_reference *head;
  ogg_reference *tail;

  /* render the byte/bit counter API constant time */
  long              count; /* doesn't count the tail */
} oggpack_buffer;

typedef struct oggbyte_buffer {
  ogg_reference *baseref;

  ogg_reference *ref;
  unsigned char *ptr;
  long           pos;
  long           end;
} oggbyte_buffer;

typedef struct ogg_sync_state {
  /* decode memory management pool */
  ogg_buffer_state *bufferpool;

  /* stream buffers */
  ogg_reference    *fifo_head;
  ogg_reference    *fifo_tail;
  long              fifo_fill;

  /* stream sync management */
  int               unsynced;
  int               headerbytes;
  int               bodybytes;

} ogg_sync_state;

typedef struct ogg_stream_state {
  ogg_reference *header_head;
  ogg_reference *header_tail;
  ogg_reference *body_head;
  ogg_reference *body_tail;

  int            e_o_s;    /* set when we have buffered the last
                              packet in the logical bitstream */
  int            b_o_s;    /* set after we've written the initial page
                              of a logical bitstream */
  long           serialno;
  long           pageno;
  ogg_int64_t    packetno; /* sequence number for decode; the framing
                              knows where there's a hole in the data,
                              but we need coupling so that the codec
                              (which is in a seperate abstraction
                              layer) also knows about the gap */
  ogg_int64_t    granulepos;

  int            lacing_fill;
  ogg_uint32_t   body_fill;

  /* decode-side state data */
  int            holeflag;
  int            spanflag;
  int            clearflag;
  int            laceptr;
  ogg_uint32_t   body_fill_next;
  
} ogg_stream_state;

typedef struct {
  ogg_reference *packet;
  long           bytes;
  long           b_o_s;
  long           e_o_s;
  ogg_int64_t    granulepos;
  ogg_int64_t    packetno;     /* sequence number for decode; the framing
                                  knows where there's a hole in the data,
                                  but we need coupling so that the codec
                                  (which is in a seperate abstraction
                                  layer) also knows about the gap */
} ogg_packet;

typedef struct {
  ogg_reference *header;
  int            header_len;
  ogg_reference *body;
  long           body_len;
} ogg_page;

/* Ogg BITSTREAM PRIMITIVES: bitstream ************************/

extern void  oggpack_readinit(oggpack_buffer *b,ogg_reference *r);
extern long  oggpack_look_full(oggpack_buffer *b,int bits);
extern long  oggpack_read(oggpack_buffer *b,int bits);

/* Inline a few, often called functions */

/* mark read process as having run off the end */
static inline void _adv_halt(oggpack_buffer *b){
  b->headptr=b->head->buffer->data+b->head->begin+b->head->length;
  b->headend=-1;
  b->headbit=0;
}

/* spans forward, skipping as many bytes as headend is negative; if
   headend is zero, simply finds next byte.  If we're up to the end
   of the buffer, leaves headend at zero.  If we've read past the end,
   halt the decode process. */
static inline void _span(oggpack_buffer *b){
  while(b->headend<1){
    if(b->head->next){
      b->count+=b->head->length;
      b->head=b->head->next;
      b->headptr=b->head->buffer->data+b->head->begin-b->headend; 
      b->headend+=b->head->length;      
    }else{
      /* we've either met the end of decode, or gone past it. halt
         only if we're past */
      if(b->headend<0 || b->headbit)
        /* read has fallen off the end */
        _adv_halt(b);

      break;
    }
  }
}

/* limited to 32 at a time */
static inline void oggpack_adv(oggpack_buffer *b,int bits){
  bits+=b->headbit;
  b->headbit=bits&7;
  b->headptr+=bits/8;
  if((b->headend-=((unsigned)bits)/8)<1)_span(b);
}

static inline long oggpack_look(oggpack_buffer *b, int bits){
  if(bits+b->headbit < b->headend<<3){
    extern const unsigned long oggpack_mask[];
    unsigned long m=oggpack_mask[bits];
    unsigned long ret=-1;

    bits+=b->headbit;
    ret=b->headptr[0]>>b->headbit;
    if(bits>8){
      ret|=b->headptr[1]<<(8-b->headbit);  
      if(bits>16){
        ret|=b->headptr[2]<<(16-b->headbit);  
        if(bits>24){
          ret|=b->headptr[3]<<(24-b->headbit);  
          if(bits>32 && b->headbit)
            ret|=b->headptr[4]<<(32-b->headbit);
        }
      }
    }
    return ret&m;
  }else{
    return oggpack_look_full(b, bits);
  }
}

/* Ogg BITSTREAM PRIMITIVES: decoding **************************/

extern ogg_sync_state *ogg_sync_create(void);
extern int      ogg_sync_destroy(ogg_sync_state *oy);
extern int      ogg_sync_reset(ogg_sync_state *oy);

extern unsigned char *ogg_sync_bufferin(ogg_sync_state *oy, long size);
extern int      ogg_sync_wrote(ogg_sync_state *oy, long bytes);
extern long     ogg_sync_pageseek(ogg_sync_state *oy,ogg_page *og);
extern int      ogg_stream_pagein(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op);
extern int      ogg_stream_packetpeek(ogg_stream_state *os,ogg_packet *op);

/* Ogg BITSTREAM PRIMITIVES: general ***************************/

extern ogg_stream_state *ogg_stream_create(int serialno);
extern int      ogg_stream_destroy(ogg_stream_state *os);
extern int      ogg_stream_reset(ogg_stream_state *os);
extern int      ogg_stream_reset_serialno(ogg_stream_state *os,int serialno);
extern int      ogg_stream_eos(ogg_stream_state *os);

extern int      ogg_page_checksum_set(ogg_page *og);

extern int      ogg_page_version(ogg_page *og);
extern int      ogg_page_continued(ogg_page *og);
extern int      ogg_page_bos(ogg_page *og);
extern int      ogg_page_eos(ogg_page *og);
extern ogg_int64_t  ogg_page_granulepos(ogg_page *og);
extern ogg_uint32_t ogg_page_serialno(ogg_page *og);
extern ogg_uint32_t ogg_page_pageno(ogg_page *og);
extern int      ogg_page_getbuffer(ogg_page *og, unsigned char **buffer);

extern int      ogg_packet_release(ogg_packet *op);
extern int      ogg_page_release(ogg_page *og);

extern void     ogg_page_dup(ogg_page *d, ogg_page *s);

/* Ogg BITSTREAM PRIMITIVES: return codes ***************************/

#define  OGG_SUCCESS   0

#define  OGG_HOLE     -10
#define  OGG_SPAN     -11
#define  OGG_EVERSION -12
#define  OGG_ESERIAL  -13
#define  OGG_EINVAL   -14
#define  OGG_EEOS     -15


#ifdef __cplusplus
}
#endif

#endif  /* _OGG_H */
