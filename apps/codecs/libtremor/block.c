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

 function: PCM data vector blocking, windowing and dis/reassembly

 ********************************************************************/

#include "config-tremor.h"
#include <stdio.h>
#include <string.h>
#include "ogg.h"
#include "ivorbiscodec.h"
#include "codec_internal.h"

#include "window.h"
#include "registry.h"
#include "misc.h"
//#include <codecs/lib/codeclib.h>

static int ilog(unsigned int v){
  int ret=0;
  if(v)--v;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static ogg_int32_t* _pcmp [CHANNELS] IBSS_ATTR;
static ogg_int32_t* _pcmbp[CHANNELS] IBSS_ATTR;
static ogg_int32_t* _pcmret[CHANNELS] IBSS_ATTR;

/* pcm accumulator examples (not exhaustive):

 <-------------- lW ---------------->
                   <--------------- W ---------------->
:            .....|.....       _______________         |
:        .'''     |     '''_---      |       |\        |
:.....'''         |_____--- '''......|       | \_______|
:.................|__________________|_______|__|______|
                  |<------ Sl ------>|      > Sr <     |endW
                  |beginSl           |endSl  |  |endSr   
                  |beginW            |endlW  |beginSr


                      |< lW >|       
                   <--------------- W ---------------->
                  |   |  ..  ______________            |
                  |   | '  `/        |     ---_        |
                  |___.'___/`.       |         ---_____| 
                  |_______|__|_______|_________________|
                  |      >|Sl|<      |<------ Sr ----->|endW
                  |       |  |endSl  |beginSr          |endSr
                  |beginW |  |endlW                     
                  mult[0] |beginSl                     mult[n]

 <-------------- lW ----------------->
                          |<--W-->|                               
:            ..............  ___  |   |                    
:        .'''             |`/   \ |   |                       
:.....'''                 |/`....\|...|                    
:.........................|___|___|___|                  
                          |Sl |Sr |endW    
                          |   |   |endSr
                          |   |beginSr
                          |   |endSl
                          |beginSl
                          |beginW
*/

/* block abstraction setup *********************************************/

#ifndef WORD_ALIGN
#define WORD_ALIGN 8
#endif

int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb){
  memset(vb,0,sizeof(*vb));
  vb->vd=v;
  vb->localalloc=0;
  vb->localstore=NULL;
  
  return(0);
}

void *_vorbis_block_alloc(vorbis_block *vb,long bytes){
  bytes=(bytes+(WORD_ALIGN-1)) & ~(WORD_ALIGN-1);
  if(bytes+vb->localtop>vb->localalloc){
    /* can't just _ogg_realloc... there are outstanding pointers */
    if(vb->localstore){
      struct alloc_chain *link=(struct alloc_chain *)_ogg_malloc(sizeof(*link));
      vb->totaluse+=vb->localtop;
      link->next=vb->reap;
      link->ptr=vb->localstore;
      vb->reap=link;
    }
    /* highly conservative */
    vb->localalloc=bytes;
    vb->localstore=_ogg_malloc(vb->localalloc);
    vb->localtop=0;
  }
  {
    void *ret=(void *)(((char *)vb->localstore)+vb->localtop);
    vb->localtop+=bytes;
    return ret;
  }
}

/* reap the chain, pull the ripcord */
void _vorbis_block_ripcord(vorbis_block *vb){
  /* reap the chain */
  struct alloc_chain *reap=vb->reap;
  while(reap){
    struct alloc_chain *next=reap->next;
    _ogg_free(reap->ptr);
    memset(reap,0,sizeof(*reap));
    _ogg_free(reap);
    reap=next;
  }
  /* consolidate storage */
  if(vb->totaluse){
    vb->localstore=_ogg_realloc(vb->localstore,vb->totaluse+vb->localalloc);
    vb->localalloc+=vb->totaluse;
    vb->totaluse=0;
  }

  /* pull the ripcord */
  vb->localtop=0;
  vb->reap=NULL;
}

int vorbis_block_clear(vorbis_block *vb){
  _vorbis_block_ripcord(vb);
  if(vb->localstore)_ogg_free(vb->localstore);

  memset(vb,0,sizeof(*vb));
  return(0);
}

static int _vds_init(vorbis_dsp_state *v,vorbis_info *vi){
  int i;
  long b_size[2];
  LOOKUP_TNC *iramposw;
  
  codec_setup_info *ci=(codec_setup_info *)vi->codec_setup;
  private_state *b=NULL;

  memset(v,0,sizeof(*v));
  v->reset_pcmb=true;
  b=(private_state *)(v->backend_state=_ogg_calloc(1,sizeof(*b)));

  v->vi=vi;
  b->modebits=ilog(ci->modes);
  
  /* allocate IRAM buffer for the PCM data generated by synthesis */
  iram_malloc_init();
  v->first_pcm=(ogg_int32_t *)iram_malloc(vi->channels*ci->blocksizes[1]*sizeof(ogg_int32_t));
  /* when can't allocate IRAM buffer, allocate normal RAM buffer */
  if(v->first_pcm == NULL){
    v->first_pcm=(ogg_int32_t *)_ogg_malloc(vi->channels*ci->blocksizes[1]*sizeof(ogg_int32_t));
  }
  
  v->centerW=0;
  
  /* Vorbis I uses only window type 0 */
  b_size[0]=ci->blocksizes[0]/2;
  b_size[1]=ci->blocksizes[1]/2;
  b->window[0]=_vorbis_window(0,b_size[0]);
  b->window[1]=_vorbis_window(0,b_size[1]);
  
  /* allocate IRAM buffer for window tables too, if sufficient iram available */
  /* give preference to the larger window over the smaller window
     (on the assumption that both windows are equally likely used) */
  for(i=1; i>=0; i--){
    iramposw=(LOOKUP_TNC *)iram_malloc(b_size[i]*sizeof(LOOKUP_TNC));
    if(iramposw!=NULL) {
      memcpy(iramposw, b->window[i], b_size[i]*sizeof(LOOKUP_TNC));
      b->window[i]=iramposw;
    }
  }

  /* finish the codebooks */
  if(!ci->fullbooks){
    ci->fullbooks=(codebook *)_ogg_calloc(ci->books,sizeof(*ci->fullbooks));
    for(i=0;i<ci->books;i++){
      vorbis_book_init_decode(ci->fullbooks+i,ci->book_param[i]);
      /* decode codebooks are now standalone after init */
      vorbis_staticbook_destroy(ci->book_param[i]);
      ci->book_param[i]=NULL;
    }
  }

  /* if we can get away with it, put a double buffer into IRAM too, so that
     overlap-add runs iram-to-iram and we avoid needing to memcpy */
  v->pcm_storage=ci->blocksizes[1];
  v->pcm=_pcmp;
  v->pcmret=_pcmret;
  v->pcmb=_pcmbp;

  _pcmp[0]=NULL;
  _pcmp[1]=NULL;
  _pcmbp[0]=NULL;
  _pcmbp[1]=NULL;
  if(NULL != (v->iram_double_pcm = iram_malloc(vi->channels*v->pcm_storage*sizeof(ogg_int32_t))))
  {
    /* one-time initialisation at codec start or on switch from
       blocksizes greater than IRAM_PCM_END to sizes that fit */
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=&v->iram_double_pcm[i*v->pcm_storage];
  }
  else
  {
    /* one-time initialisation at codec start or on switch from
       blocksizes that fit in IRAM_PCM_END to those that don't */
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=(ogg_int32_t *)_ogg_calloc(v->pcm_storage,sizeof(*v->pcm[i])); 
  }  

  /* all 1 (large block) or 0 (small block) */
  /* explicitly set for the sake of clarity */
  v->lW=0; /* previous window size */
  v->W=0;  /* current window size */

  /* initialize all the mapping/backend lookups */
  b->mode=(vorbis_look_mapping **)_ogg_calloc(ci->modes,sizeof(*b->mode));
  for(i=0;i<ci->modes;i++){
    int mapnum=ci->mode_param[i]->mapping;
    int maptype=ci->map_type[mapnum];
    b->mode[i]=_mapping_P[maptype]->look(v,ci->mode_param[i],
                                         ci->map_param[mapnum]);
  }

  return(0);
}

int vorbis_synthesis_restart(vorbis_dsp_state *v){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci;
  int i;

  if(!v->backend_state)return -1;
  if(!vi)return -1;
  ci=vi->codec_setup;
  if(!ci)return -1;

  v->centerW=0;
  v->pcm_current=0;
  
  v->pcm_returned=-1;
  v->granulepos=-1;
  v->sequence=-1;
  ((private_state *)(v->backend_state))->sample_count=-1;
  
  /* indicate to synthesis code that buffer pointers no longer valid
     (if we're using double pcm buffer) and will need to reset them */
  v->reset_pcmb = true;
  /* also reset our copy of the double buffer pointers if we have one */
  if(v->iram_double_pcm)
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=&v->iram_double_pcm[i*v->pcm_storage];

  return(0);
}

int vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi){
  _vds_init(v,vi);
  vorbis_synthesis_restart(v);

  return(0);
}

void vorbis_dsp_clear(vorbis_dsp_state *v){
  int i;
  if(v){
    vorbis_info *vi=v->vi;
    codec_setup_info *ci=(codec_setup_info *)(vi?vi->codec_setup:NULL);
    private_state *b=(private_state *)v->backend_state;

    if(NULL == v->iram_double_pcm && vi != NULL)
    {
      /* pcm buffer came from oggmalloc rather than iram */
      for(i=0;i<vi->channels;i++)
        if(v->pcm[i])_ogg_free(v->pcm[i]);
    }

    /* free mode lookups; these are actually vorbis_look_mapping structs */
    if(ci){
      for(i=0;i<ci->modes;i++){
        int mapnum=ci->mode_param[i]->mapping;
        int maptype=ci->map_type[mapnum];
        if(b && b->mode)_mapping_P[maptype]->free_look(b->mode[i]);
      }
    }

    if(b){
      if(b->mode)_ogg_free(b->mode);    
      _ogg_free(b);
    }
    
    memset(v,0,sizeof(*v));
  }
}

/* Unlike in analysis, the window is only partially applied for each
   block.  The time domain envelope is not yet handled at the point of
   calling (as it relies on the previous block). */

int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb)
    ICODE_ATTR;
int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=(codec_setup_info *)vi->codec_setup;
  private_state *b=v->backend_state;
  int j;
  bool iram_pcm_doublebuffer = (NULL != v->iram_double_pcm);

  if(v->pcm_current>v->pcm_returned  && v->pcm_returned!=-1)return(OV_EINVAL);

  v->lW=v->W;
  v->W=vb->W;
  v->nW=-1;

  if((v->sequence==-1)||
     (v->sequence+1 != vb->sequence)){
    v->granulepos=-1; /* out of sequence; lose count */
    b->sample_count=-1;
  }

  v->sequence=vb->sequence;
  int n=ci->blocksizes[v->W]/2;
  int ln=ci->blocksizes[v->lW]/2;
  
  if(LIKELY(vb->pcm)){ /* no pcm to process if vorbis_synthesis_trackonly 
                          was called on block */
    int prevCenter;
    int n0=ci->blocksizes[0]/2;
    int n1=ci->blocksizes[1]/2;

    if(iram_pcm_doublebuffer)
    {
      prevCenter = ln;
    }
    else 
    {
      prevCenter = v->centerW;
      v->centerW = n1 - v->centerW;
    }
    
    /* overlap/add PCM */
    /* nb nothing to overlap with on first block so don't bother */ 
    if(LIKELY(v->pcm_returned!=-1))
    {    
      for(j=0;j<vi->channels;j++)
      {
        ogg_int32_t *pcm=v->pcm[j]+prevCenter;
        ogg_int32_t *p=vb->pcm[j];
      
        /* the overlap/add section */
        if(v->lW == v->W)
        {
            /* large/large or small/small */
            vect_add_right_left(pcm,p,n);
            v->pcmb[j]=pcm;
        }
        else if (!v->W)
        {
            /* large/small */
            vect_add_right_left(pcm + (n1-n0)/2, p, n0);
            v->pcmb[j]=pcm;
        }
        else
        {
            /* small/large */
            p += (n1-n0)/2;
            vect_add_left_right(p,pcm,n0);
            v->pcmb[j]=p;
        }
      }
    }

    /* the copy section */
    if(iram_pcm_doublebuffer)
    {
      /* just flip the pointers over as we have a double buffer in iram */
      ogg_int32_t *p;
      p=v->pcm[0];
      v->pcm[0]=vb->pcm[0];
      vb->pcm[0] = p;
      p=v->pcm[1];
      v->pcm[1]=vb->pcm[1];
      vb->pcm[1] = p;
    }
    else
    {        
      for(j=0;j<vi->channels;j++)
      {
        /* at best only vb->pcm is in iram, and that's where we do the
           synthesis, so we copy out the right-hand subframe of last
           synthesis into (noniram) local buffer so we can still do
           synth in iram */
        vect_copy(v->pcm[j]+v->centerW, vb->pcm[j]+n, n);
      }
    }
    
    /* deal with initial packet state; we do this using the explicit
       pcm_returned==-1 flag otherwise we're sensitive to first block
       being short or long */

    if(v->pcm_returned==-1){
      v->pcm_returned=0;
      v->pcm_current=0;
    }else{
      v->pcm_returned=0;
      v->pcm_current=(n+ln)/2;
    }

  }
    
  /* track the frame number... This is for convenience, but also
     making sure our last packet doesn't end with added padding.  If
     the last packet is partial, the number of samples we'll have to
     return will be past the vb->granulepos.
     
     This is not foolproof!  It will be confused if we begin
     decoding at the last page after a seek or hole.  In that case,
     we don't have a starting point to judge where the last frame
     is.  For this reason, vorbisfile will always try to make sure
     it reads the last two marked pages in proper sequence */
  
  if(b->sample_count==-1){
    b->sample_count=0;
  }else{
    b->sample_count+=(n+ln)/2;
  }
    
  if(v->granulepos==-1){
    if(vb->granulepos!=-1){ /* only set if we have a position to set to */
      
      v->granulepos=vb->granulepos;
      
      /* is this a short page? */
      if(b->sample_count>v->granulepos){
        /* corner case; if this is both the first and last audio page,
           then spec says the end is cut, not beginning */
        if(vb->eofflag){
          /* trim the end */
          /* no preceeding granulepos; assume we started at zero (we'd
             have to in a short single-page stream) */
          /* granulepos could be -1 due to a seek, but that would result
             in a long coun`t, not short count */
          
          v->pcm_current-=(b->sample_count-v->granulepos);
        }else{
          /* trim the beginning */
          v->pcm_returned+=(b->sample_count-v->granulepos);
          if(v->pcm_returned>v->pcm_current)
            v->pcm_returned=v->pcm_current;
        }
        
      }
      
    }
  }else{
    v->granulepos+=(n+ln)/2;
    if(vb->granulepos!=-1 && v->granulepos!=vb->granulepos){
      
      if(v->granulepos>vb->granulepos){
        long extra=v->granulepos-vb->granulepos;
        
        if(extra)
          if(vb->eofflag){
            /* partial last frame.  Strip the extra samples off */
            v->pcm_current-=extra;
          } /* else {Shouldn't happen *unless* the bitstream is out of
               spec.  Either way, believe the bitstream } */
      } /* else {Shouldn't happen *unless* the bitstream is out of
           spec.  Either way, believe the bitstream } */
      v->granulepos=vb->granulepos;
    }
  }
  
  /* Update, cleanup */
  
  if(vb->eofflag)v->eofflag=1;
  return(0);
}

/* pcm==NULL indicates we just want the pending samples, no more */
int vorbis_synthesis_pcmout(vorbis_dsp_state *v,ogg_int32_t ***pcm) ICODE_ATTR;
int vorbis_synthesis_pcmout(vorbis_dsp_state *v,ogg_int32_t ***pcm){
  vorbis_info *vi=v->vi;
  if(v->pcm_returned>-1 && v->pcm_returned<v->pcm_current){
    if(pcm){
      int i;
      for(i=0;i<vi->channels;i++)
        v->pcmret[i]=v->pcmb[i]+v->pcm_returned;
      *pcm=v->pcmret;
    }
    return(v->pcm_current-v->pcm_returned);
  }
  return(0);
}

int vorbis_synthesis_read(vorbis_dsp_state *v,int bytes){
  if(bytes && v->pcm_returned+bytes>v->pcm_current)return(OV_EINVAL);
  v->pcm_returned+=bytes;
  return(0);
}

