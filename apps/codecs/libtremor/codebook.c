/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: basic codebook pack/unpack/code/decode operations

 ********************************************************************/

#include "config-tremor.h"
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "ivorbiscodec.h"
#include "codebook.h"
#include "misc.h"
#include "os.h"

/* unpacks a codebook from the packet buffer into the codebook struct,
   readies the codebook auxiliary structures for decode *************/
int vorbis_staticbook_unpack(oggpack_buffer *opb,static_codebook *s){
  long i,j;
  memset(s,0,sizeof(*s));

  /* make sure alignment is correct */
  if(oggpack_read(opb,24)!=0x564342)goto _eofout;

  /* first the basic parameters */
  s->dim=oggpack_read(opb,16);
  s->entries=oggpack_read(opb,24);
  if(s->entries==-1)goto _eofout;

  /* codeword ordering.... length ordered or unordered? */
  switch((int)oggpack_read(opb,1)){
  case 0:
    /* unordered */
    s->lengthlist=(long *)_ogg_malloc(sizeof(*s->lengthlist)*s->entries);

    /* allocated but unused entries? */
    if(oggpack_read(opb,1)){
      /* yes, unused entries */

      for(i=0;i<s->entries;i++){
    if(oggpack_read(opb,1)){
      long num=oggpack_read(opb,5);
      if(num==-1)goto _eofout;
      s->lengthlist[i]=num+1;
    }else
      s->lengthlist[i]=0;
      }
    }else{
      /* all entries used; no tagging */
      for(i=0;i<s->entries;i++){
    long num=oggpack_read(opb,5);
    if(num==-1)goto _eofout;
    s->lengthlist[i]=num+1;
      }
    }
    
    break;
  case 1:
    /* ordered */
    {
      long length=oggpack_read(opb,5)+1;
      s->lengthlist=(long *)_ogg_malloc(sizeof(*s->lengthlist)*s->entries);

      for(i=0;i<s->entries;){
    long num=oggpack_read(opb,_ilog(s->entries-i));
    if(num==-1)goto _eofout;
    for(j=0;j<num && i<s->entries;j++,i++)
      s->lengthlist[i]=length;
    length++;
      }
    }
    break;
  default:
    /* EOF */
    return(-1);
  }
  
  /* Do we have a mapping to unpack? */
  switch((s->maptype=oggpack_read(opb,4))){
  case 0:
    /* no mapping */
    break;
  case 1: case 2:
    /* implicitly populated value mapping */
    /* explicitly populated value mapping */

    s->q_min=oggpack_read(opb,32);
    s->q_delta=oggpack_read(opb,32);
    s->q_quant=oggpack_read(opb,4)+1;
    s->q_sequencep=oggpack_read(opb,1);

    {
      int quantvals=0;
      switch(s->maptype){
      case 1:
    quantvals=_book_maptype1_quantvals(s);
    break;
      case 2:
    quantvals=s->entries*s->dim;
    break;
      }
      
      /* quantized values */
      s->quantlist=(long *)_ogg_malloc(sizeof(*s->quantlist)*quantvals);
      for(i=0;i<quantvals;i++)
    s->quantlist[i]=oggpack_read(opb,s->q_quant);
      
      if(quantvals&&s->quantlist[quantvals-1]==-1)goto _eofout;
    }
    break;
  default:
    goto _errout;
  }

  /* all set */
  return(0);
  
 _errout:
 _eofout:
  vorbis_staticbook_clear(s);
  return(-1); 
}

/* the 'eliminate the decode tree' optimization actually requires the
   codewords to be MSb first, not LSb.  This is an annoying inelegancy
   (and one of the first places where carefully thought out design
   turned out to be wrong; Vorbis II and future Ogg codecs should go
   to an MSb bitpacker), but not actually the huge hit it appears to
   be.  The first-stage decode table catches most words so that
   bitreverse is not in the main execution path. */

static inline ogg_uint32_t bitreverse(register ogg_uint32_t x)
{
  unsigned tmp, ret;
#ifdef CPU_ARM
#if ARM_ARCH >= 6
  unsigned mask = 0x0f0f0f0f;
#else
  unsigned mask = 0x00ff00ff;
#endif
  asm (
#if ARM_ARCH >= 6
    "rev     %[r], %[x]              \n"  /* swap halfwords and bytes */
    "and     %[t], %[m], %[r]        \n"  /* Sequence is one instruction */
    "eor     %[r], %[t], %[r]        \n"  /*   longer than on <= ARMv5, but */
    "mov     %[t], %[t], lsl #4      \n"  /*   interlock free */
    "orr     %[r], %[t], %[r], lsr #4\n"  /* nibbles swapped */
    "eor     %[m], %[m], %[m], lsl #2\n"  /* mask = 0x33333333 */
    "and     %[t], %[m], %[r]        \n"
    "eor     %[r], %[t], %[r]        \n"
    "mov     %[t], %[t], lsl #2      \n"
    "orr     %[r], %[t], %[r], lsr #2\n"  /* dibits swapped */
    "eor     %[m], %[m], %[m], lsl #1\n"  /* mask = 0x55555555 */
    "and     %[t], %[m], %[r]        \n"
    "eor     %[r], %[t], %[r]        \n"
    "mov     %[t], %[t], lsl #1      \n"
    "orr     %[r], %[t], %[r], lsr #1\n"  /* bits swapped */
#else /* ARM_ARCH <= 5 */
    "mov     %[r], %[x], ror #16     \n"  /* swap halfwords */
    "and     %[t], %[m], %[r], lsr #8\n"
    "eor     %[r], %[r], %[t], lsl #8\n"
    "orr     %[r], %[t], %[r], lsl #8\n"  /* bytes swapped */
    "eor     %[m], %[m], %[m], lsl #4\n"  /* mask = 0x0f0f0f0f */
    "and     %[t], %[m], %[r], lsr #4\n"
    "eor     %[r], %[r], %[t], lsl #4\n"
    "orr     %[r], %[t], %[r], lsl #4\n"  /* nibbles swapped */
    "eor     %[m], %[m], %[m], lsl #2\n"  /* mask = 0x33333333 */
    "and     %[t], %[m], %[r], lsr #2\n"
    "eor     %[r], %[r], %[t], lsl #2\n"
    "orr     %[r], %[t], %[r], lsl #2\n"  /* dibits swapped */
    "eor     %[m], %[m], %[m], lsl #1\n"  /* mask = 0x55555555 */
    "and     %[t], %[m], %[r], lsr #1\n"
    "eor     %[r], %[r], %[t], lsl #1\n"
    "orr     %[r], %[t], %[r], lsl #1\n"  /* bits swapped */
#endif /* ARM_ARCH */
    : /* outputs */
    [m]"+r"(mask),
    [r]"=r"(ret),
    [t]"=r"(tmp)
    : /* inputs */
    [x]"r"(x)
  );
#else /* !CPU_ARM */

#ifdef CPU_COLDFIRE
  ret = x;
  asm ("swap  %[r]" : [r] "+r" (ret));   /* swap halfwords */
#else
  ret = (x>>16) | (x<<16);
#endif
  tmp = ret & 0x00ff00ff;
  ret ^= tmp;
  ret = (ret >> 8) | (tmp << 8);         /* bytes swapped */
  tmp = ret & 0x0f0f0f0f;
  ret ^= tmp;
  ret = (ret >> 4) | (tmp << 4);         /* 4-bit units swapped */
  tmp = ret & 0x33333333;
  ret ^= tmp;
  ret = (ret >> 2) | (tmp << 2);         /* 2-bit units swapped */
  tmp = ret & 0x55555555;
  ret ^= tmp;
  ret = (ret >> 1) | (tmp << 1);         /* done */
#endif /* !CPU_ARM */
  return ret;
}

STIN long decode_packed_entry_number(codebook *book, 
                          oggpack_buffer *b){
  int  read=book->dec_maxlength;
  long lo,hi;
  long lok = oggpack_look(b,book->dec_firsttablen);
 
  if (LIKELY(lok >= 0)) {
    long entry = book->dec_firsttable[lok];
    if(UNLIKELY(entry&0x80000000UL)){
      lo=(entry>>15)&0x7fff;
      hi=book->used_entries-(entry&0x7fff);
    }else{
      oggpack_adv(b, book->dec_codelengths[entry-1]);
      return(entry-1);
    }
  }else{
    lo=0;
    hi=book->used_entries;
  }

  lok = oggpack_look(b, read);

  while(lok<0 && read>1)
    lok = oggpack_look(b, --read);

  if(lok<0){
    oggpack_adv(b,1); /* force eop */
    return -1;
  }

  /* bisect search for the codeword in the ordered list */
  {
    ogg_uint32_t testword=bitreverse((ogg_uint32_t)lok);

    while(hi-lo>1){
      long p=(hi-lo)>>1;
      long test=book->codelist[lo+p]>testword;    
      lo+=p&(test-1);
      hi-=p&(-test);
    }

    if(book->dec_codelengths[lo]<=read){
      oggpack_adv(b, book->dec_codelengths[lo]);
      return(lo);
    }
  }
  
  oggpack_adv(b, read+1);
  return(-1);
}

static long decode_packed_block(codebook *book, oggpack_buffer *b,
                                long *buf, int n){
  long *bufptr = buf;
  long *bufend = buf + n;

  while (bufptr<bufend) {
    if (b->headend > 8) {
      ogg_uint32_t *ptr;
      unsigned long bit, bitend;
      unsigned long adr;
      ogg_uint32_t cache = 0;
      int cachesize = 0;

      adr = (unsigned long)b->headptr;
      bit = (adr&3)*8+b->headbit;
      ptr = (ogg_uint32_t *)(adr&~3);
      bitend = ((adr&3)+b->headend)*8;
      while (bufptr<bufend){
    long entry, lo, hi;
    if (UNLIKELY(cachesize<book->dec_maxlength)) {
      if (bit-cachesize+32>=bitend)
        break;
      bit-=cachesize;
      cache=letoh32(ptr[bit>>5]) >> (bit&31);
      if (bit&31)
        cache|=letoh32(ptr[(bit>>5)+1]) << (32-(bit&31));
      cachesize=32;
      bit+=32;
    }

    entry=book->dec_firsttable[cache&((1<<book->dec_firsttablen)-1)];
    if(UNLIKELY(entry&0x80000000UL)){
      lo=(entry>>15)&0x7fff;
      hi=book->used_entries-(entry&0x7fff);
      {
        ogg_uint32_t testword=bitreverse((ogg_uint32_t)cache);
        
            while(LIKELY(hi-lo>1)){
              long p=(hi-lo)>>1;
              if (book->codelist[lo+p]>testword)
                hi-=p;
              else
                lo+=p;
            }
            entry=lo;
          }
    }else
      entry--;

    *bufptr++=entry;
    {
      int l=book->dec_codelengths[entry];
      cachesize-=l;
      cache>>=l;
    }
      }

      adr=(unsigned long)b->headptr;
      bit-=(adr&3)*8+cachesize;
      b->headend-=(bit/8);
      b->headptr+=bit/8;
      b->headbit=bit%8;
    } else {
      long r = decode_packed_entry_number(book, b);
      if (r == -1) return bufptr-buf;
      *bufptr++ = r;
    }
  }
  return n;
}


/* Decode side is specced and easier, because we don't need to find
   matches using different criteria; we simply read and map.  There are
   two things we need to do 'depending':
   
   We may need to support interleave.  We don't really, but it's
   convenient to do it here rather than rebuild the vector later.

   Cascades may be additive or multiplicitive; this is not inherent in
   the codebook, but set in the code using the codebook.  Like
   interleaving, it's easiest to do it here.  
   addmul==0 -> declarative (set the value)
   addmul==1 -> additive
   addmul==2 -> multiplicitive */

/* returns the [original, not compacted] entry number or -1 on eof *********/
long vorbis_book_decode(codebook *book, oggpack_buffer *b){
  if(book->used_entries>0){
    long packed_entry=decode_packed_entry_number(book,b);
    if(packed_entry>=0)
      return(book->dec_index[packed_entry]);
  }

  /* if there's no dec_index, the codebook unpacking isn't collapsed */
  return(-1);
}

/* returns 0 on OK or -1 on eof *************************************/
long vorbis_book_decodevs_add(codebook *book,ogg_int32_t *a,
                  oggpack_buffer *b,int n,int point){
  if(book->used_entries>0){  
    int step=n/book->dim;
    long *entry = (long *)alloca(sizeof(*entry)*step);
    ogg_int32_t **t = (ogg_int32_t **)alloca(sizeof(*t)*step);
    int i,j,o;
    int shift=point-book->binarypoint;
    
    if(shift>=0){
      for (i = 0; i < step; i++) {
    entry[i]=decode_packed_entry_number(book,b);
    if(entry[i]==-1)return(-1);
    t[i] = book->valuelist+entry[i]*book->dim;
      }
      for(i=0,o=0;i<book->dim;i++,o+=step)
    for (j=0;j<step;j++)
      a[o+j]+=t[j][i]>>shift;
    }else{
      for (i = 0; i < step; i++) {
    entry[i]=decode_packed_entry_number(book,b);
    if(entry[i]==-1)return(-1);
    t[i] = book->valuelist+entry[i]*book->dim;
      }
      for(i=0,o=0;i<book->dim;i++,o+=step)
    for (j=0;j<step;j++)
      a[o+j]+=t[j][i]<<-shift;
    }
  }
  return(0);
}

long vorbis_book_decodev_add(codebook *book,ogg_int32_t *a,
                 oggpack_buffer *b,int n,int point){
  if(book->used_entries>0){
    int i,j,entry;
    ogg_int32_t *t;
    int shift=point-book->binarypoint;
    
    if(shift>=0){
      for(i=0;i<n;){
    entry = decode_packed_entry_number(book,b);
    if(entry==-1)return(-1);
    t     = book->valuelist+entry*book->dim;
    for (j=0;j<book->dim;)
      a[i++]+=t[j++]>>shift;
      }
    }else{
      shift = -shift;
      for(i=0;i<n;){
    entry = decode_packed_entry_number(book,b);
    if(entry==-1)return(-1);
    t     = book->valuelist+entry*book->dim;
    for (j=0;j<book->dim;)
      a[i++]+=t[j++]<<shift;
      }
    }
  }
  return(0);
}

long vorbis_book_decodev_set(codebook *book,ogg_int32_t *a,
                 oggpack_buffer *b,int n,int point){
  if(book->used_entries>0){
    int i,j,entry;
    ogg_int32_t *t;
    int shift=point-book->binarypoint;
    
    if(shift>=0){
      
      for(i=0;i<n;){
    entry = decode_packed_entry_number(book,b);
    if(entry==-1)return(-1);
    t     = book->valuelist+entry*book->dim;
    for (j=0;j<book->dim;){
      a[i++]=t[j++]>>shift;
    }
      }
    }else{
      shift = -shift;
      for(i=0;i<n;){
    entry = decode_packed_entry_number(book,b);
    if(entry==-1)return(-1);
    t     = book->valuelist+entry*book->dim;
    for (j=0;j<book->dim;){
      a[i++]=t[j++]<<shift;
    }
      }
    }
  }else{

    int i,j;
    for(i=0;i<n;){
      for (j=0;j<book->dim;){
    a[i++]=0;
      }
    }
  }
  return(0);
}

static long vorbis_book_decodevv_add_2ch_even(codebook *book,ogg_int32_t **a,
                                              long offset,oggpack_buffer *b,
                                              int n,int point){
  long i,k,chunk,read;
  int shift=point-book->binarypoint;
  long entries[32];
  ogg_int32_t *p0 = &(a[0][offset]);
  ogg_int32_t *p1 = &(a[1][offset]);

  if(shift>=0){
    
    for(i=0;i<n;){
      chunk=32;
      if (chunk*book->dim>(n-i)*2)
        chunk=((n-i)*2+book->dim-1)/book->dim;
      read = decode_packed_block(book,b,entries,chunk);
      for(k=0;k<read;k++){
        const ogg_int32_t *t = book->valuelist+entries[k]*book->dim;
        const ogg_int32_t *u = t+book->dim;
        do{
          *p0++ += *t++>>shift;
          *p1++ += *t++>>shift;
        }while(t<u);
      }
      if (read<chunk)return-1;
      i += read*book->dim/2;
    }
  }else{
    shift = -shift;
    for(i=0;i<n;){
      chunk=32;
      if (chunk*book->dim>(n-i)*2)
        chunk=((n-i)*2+book->dim-1)/book->dim;
      read = decode_packed_block(book,b,entries,chunk);
      for(k=0;k<read;k++){
        const ogg_int32_t *t = book->valuelist+entries[k]*book->dim;
        const ogg_int32_t *u = t+book->dim;
        do{
          *p0++ += *t++<<shift;
          *p1++ += *t++<<shift;
        }while(t<u);
      }
      if (read<chunk)return-1;
      i += read*book->dim/2;
    }
  }
  return(0);
}

long vorbis_book_decodevv_add(codebook *book,ogg_int32_t **a,
                  long offset,int ch,
                  oggpack_buffer *b,int n,int point){
  if(LIKELY(book->used_entries>0)){
    long i,j,k,chunk,read;
    int chptr=0;
    int shift=point-book->binarypoint;
    long entries[32];

    if (!(book->dim&1) && ch==2)
      return vorbis_book_decodevv_add_2ch_even(book,a,offset,b,n,point);

    if(shift>=0){
    
      for(i=offset;i<offset+n;){
        chunk=32;
        if (chunk*book->dim>(offset+n-i)*ch)
          chunk=((offset+n-i)*ch+book->dim-1)/book->dim;
        read = decode_packed_block(book,b,entries,chunk);
        for(k=0;k<read;k++){
      const ogg_int32_t *t = book->valuelist+entries[k]*book->dim;
      for (j=0;j<book->dim;j++){
        a[chptr++][i]+=t[j]>>shift;
        if(chptr==ch){
          chptr=0;
          i++;
        }
      }
        }
        if (read<chunk)return-1;
      }
    }else{
      shift = -shift;
      for(i=offset;i<offset+n;){
        chunk=32;
        if (chunk*book->dim>(offset+n-i)*ch)
          chunk=((offset+n-i)*ch+book->dim-1)/book->dim;
        read = decode_packed_block(book,b,entries,chunk);
        for(k=0;k<read;k++){
      const ogg_int32_t *t = book->valuelist+entries[k]*book->dim;
      for (j=0;j<book->dim;j++){
        a[chptr++][i]+=t[j]<<shift;
        if(chptr==ch){
          chptr=0;
          i++;
        }
      }
        }
        if (read<chunk)return-1;
      }
    }
  }
  return(0);
}
