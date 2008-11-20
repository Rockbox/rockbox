/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: toplevel libogg include
 last mod: $Id$

 ********************************************************************/
#ifndef _OGG_H
#define _OGG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "codeclib.h"

typedef short spx_ogg_int16_t;
typedef unsigned short spx_ogg_uint16_t;
typedef int spx_ogg_int32_t;
typedef unsigned int spx_ogg_uint32_t;
typedef long long spx_ogg_int64_t;


#define _spx_ogg_malloc  codec_malloc
#define _spx_ogg_calloc  codec_calloc
#define _spx_ogg_realloc codec_realloc
#define _spx_ogg_free    codec_free


typedef struct {
  long endbyte;
  int  endbit;

  unsigned char *buffer;
  unsigned char *ptr;
  long storage;
} oggpack_buffer;

/* spx_ogg_page is used to encapsulate the data in one Ogg bitstream page *****/

typedef struct {
  unsigned char *header;
  long header_len;
  unsigned char *body;
  long body_len;
} spx_ogg_page;

/* spx_ogg_stream_state contains the current encode/decode state of a logical
   Ogg bitstream **********************************************************/

typedef struct {
  unsigned char   *body_data;    /* bytes from packet bodies */
  long    body_storage;          /* storage elements allocated */
  long    body_fill;             /* elements stored; fill mark */
  long    body_returned;         /* elements of fill returned */


  int     *lacing_vals;      /* The values that will go to the segment table */
  spx_ogg_int64_t *granule_vals; /* granulepos values for headers. Not compact
				this way, but it is simple coupled to the
				lacing fifo */
  long    lacing_storage;
  long    lacing_fill;
  long    lacing_packet;
  long    lacing_returned;

  unsigned char    header[282];      /* working space for header encode */
  int              header_fill;

  int     e_o_s;          /* set when we have buffered the last packet in the
                             logical bitstream */
  int     b_o_s;          /* set after we've written the initial page
                             of a logical bitstream */
  long    serialno;
  long    pageno;
  spx_ogg_int64_t  packetno;      /* sequence number for decode; the framing
                             knows where there's a hole in the data,
                             but we need coupling so that the codec
                             (which is in a seperate abstraction
                             layer) also knows about the gap */
  spx_ogg_int64_t   granulepos;

} spx_ogg_stream_state;

/* spx_ogg_packet is used to encapsulate the data and metadata belonging
   to a single raw Ogg/Vorbis packet *************************************/

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  b_o_s;
  long  e_o_s;

  spx_ogg_int64_t  granulepos;
  
  spx_ogg_int64_t  packetno;     /* sequence number for decode; the framing
				knows where there's a hole in the data,
				but we need coupling so that the codec
				(which is in a seperate abstraction
				layer) also knows about the gap */
} spx_ogg_packet;

typedef struct {
  unsigned char *data;
  int storage;
  int fill;
  int returned;

  int unsynced;
  int headerbytes;
  int bodybytes;
} spx_ogg_sync_state;

/* Ogg BITSTREAM PRIMITIVES: bitstream ************************/

extern void  oggpack_writeinit(oggpack_buffer *b);
extern void  oggpack_writetrunc(oggpack_buffer *b,long bits);
extern void  oggpack_writealign(oggpack_buffer *b);
extern void  oggpack_writecopy(oggpack_buffer *b,void *source,long bits);
extern void  oggpack_reset(oggpack_buffer *b);
extern void  oggpack_writeclear(oggpack_buffer *b);
extern void  oggpack_readinit(oggpack_buffer *b,unsigned char *buf,int bytes);
extern void  oggpack_write(oggpack_buffer *b,unsigned long value,int bits);
extern long  oggpack_look(oggpack_buffer *b,int bits);
extern long  oggpack_look1(oggpack_buffer *b);
extern void  oggpack_adv(oggpack_buffer *b,int bits);
extern void  oggpack_adv1(oggpack_buffer *b);
extern long  oggpack_read(oggpack_buffer *b,int bits);
extern long  oggpack_read1(oggpack_buffer *b);
extern long  oggpack_bytes(oggpack_buffer *b);
extern long  oggpack_bits(oggpack_buffer *b);
extern unsigned char *oggpack_get_buffer(oggpack_buffer *b);

extern void  oggpackB_writeinit(oggpack_buffer *b);
extern void  oggpackB_writetrunc(oggpack_buffer *b,long bits);
extern void  oggpackB_writealign(oggpack_buffer *b);
extern void  oggpackB_writecopy(oggpack_buffer *b,void *source,long bits);
extern void  oggpackB_reset(oggpack_buffer *b);
extern void  oggpackB_writeclear(oggpack_buffer *b);
extern void  oggpackB_readinit(oggpack_buffer *b,unsigned char *buf,int bytes);
extern void  oggpackB_write(oggpack_buffer *b,unsigned long value,int bits);
extern long  oggpackB_look(oggpack_buffer *b,int bits);
extern long  oggpackB_look1(oggpack_buffer *b);
extern void  oggpackB_adv(oggpack_buffer *b,int bits);
extern void  oggpackB_adv1(oggpack_buffer *b);
extern long  oggpackB_read(oggpack_buffer *b,int bits);
extern long  oggpackB_read1(oggpack_buffer *b);
extern long  oggpackB_bytes(oggpack_buffer *b);
extern long  oggpackB_bits(oggpack_buffer *b);
extern unsigned char *oggpackB_get_buffer(oggpack_buffer *b);

/* Ogg BITSTREAM PRIMITIVES: encoding **************************/

extern int      spx_ogg_stream_packetin(spx_ogg_stream_state *os, spx_ogg_packet *op);
extern int      spx_ogg_stream_pageout(spx_ogg_stream_state *os, spx_ogg_page *og);
extern int      spx_ogg_stream_flush(spx_ogg_stream_state *os, spx_ogg_page *og);

/* Ogg BITSTREAM PRIMITIVES: decoding **************************/

extern int      spx_ogg_sync_init(spx_ogg_sync_state *oy);
extern int      spx_ogg_sync_clear(spx_ogg_sync_state *oy);
extern int      spx_ogg_sync_reset(spx_ogg_sync_state *oy);
extern int	spx_ogg_sync_destroy(spx_ogg_sync_state *oy);

extern void     spx_ogg_alloc_buffer(spx_ogg_sync_state *oy, long size);
extern char    *spx_ogg_sync_buffer(spx_ogg_sync_state *oy, long size);
extern int      spx_ogg_sync_wrote(spx_ogg_sync_state *oy, long bytes);
extern long     spx_ogg_sync_pageseek(spx_ogg_sync_state *oy,spx_ogg_page *og);
extern int      spx_ogg_sync_pageout(spx_ogg_sync_state *oy, spx_ogg_page *og);
extern int      spx_ogg_stream_pagein(spx_ogg_stream_state *os, spx_ogg_page *og);
extern int      spx_ogg_stream_packetout(spx_ogg_stream_state *os,spx_ogg_packet *op);
extern int      spx_ogg_stream_packetpeek(spx_ogg_stream_state *os,spx_ogg_packet *op);

/* Ogg BITSTREAM PRIMITIVES: general ***************************/

extern int      spx_ogg_stream_init(spx_ogg_stream_state *os,int serialno);
extern int      spx_ogg_stream_clear(spx_ogg_stream_state *os);
extern int      spx_ogg_stream_reset(spx_ogg_stream_state *os);
extern int      spx_ogg_stream_reset_serialno(spx_ogg_stream_state *os,int serialno);
extern int      spx_ogg_stream_destroy(spx_ogg_stream_state *os);
extern int      spx_ogg_stream_eos(spx_ogg_stream_state *os);

extern void     spx_ogg_page_checksum_set(spx_ogg_page *og);

extern int      spx_ogg_page_version(spx_ogg_page *og);
extern int      spx_ogg_page_continued(spx_ogg_page *og);
extern int      spx_ogg_page_bos(spx_ogg_page *og);
extern int      spx_ogg_page_eos(spx_ogg_page *og);
extern spx_ogg_int64_t  spx_ogg_page_granulepos(spx_ogg_page *og);
extern int      spx_ogg_page_serialno(spx_ogg_page *og);
extern long     spx_ogg_page_pageno(spx_ogg_page *og);
extern int      spx_ogg_page_packets(spx_ogg_page *og);

extern void     spx_ogg_packet_clear(spx_ogg_packet *op);


#ifdef __cplusplus
}
#endif

#endif  /* _OGG_H */






