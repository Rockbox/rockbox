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

 function: single-block PCM synthesis
 last mod: $Id$

 ********************************************************************/

#include <stdio.h>
#include "ogg.h"
#include "ivorbiscodec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"
#include "os.h"


static ogg_int32_t *ipcm_vect[CHANNELS] IBSS_ATTR;
int32_t staticbuffer[16384];

int vorbis_synthesis(vorbis_block *vb,ogg_packet *op,int decodep)
    ICODE_ATTR_TREMOR_NOT_MDCT;
int vorbis_synthesis(vorbis_block *vb,ogg_packet *op,int decodep){
  vorbis_dsp_state     *vd=vb->vd;
  private_state        *b=(private_state *)vd->backend_state;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  oggpack_buffer       *opb=&vb->opb;
  int                   type,mode,i;
 
  /* first things first.  Make sure decode is ready */
  _vorbis_block_ripcord(vb);
  oggpack_readinit(opb,op->packet);

  /* Check the packet type */
  if(oggpack_read(opb,1)!=0){
    /* Oops.  This is not an audio data packet */
    return(OV_ENOTAUDIO);
  }

  /* read our mode and pre/post windowsize */
  mode=oggpack_read(opb,b->modebits);
  if(mode==-1)return(OV_EBADPACKET);
  
  vb->mode=mode;
  vb->W=ci->mode_param[mode]->blockflag;
  if(vb->W){
    vb->lW=oggpack_read(opb,1);
    vb->nW=oggpack_read(opb,1);
    if(vb->nW==-1)   return(OV_EBADPACKET);
  }else{
    vb->lW=0;
    vb->nW=0;
  }
  
  /* more setup */
  vb->granulepos=op->granulepos;
  vb->sequence=op->packetno-3; /* first block is third packet */
  vb->eofflag=op->e_o_s;

  if(decodep && vi->channels<=CHANNELS)
  {
    vb->pcm = ipcm_vect;

    /* set pcm end point */
    vb->pcmend=ci->blocksizes[vb->W];
    /* use statically allocated buffer */
    if(vd->reset_pcmb || vb->pcm[0]==NULL)
    {
      /* one-time initialisation at codec start
         NOT for every block synthesis start
         allows us to flip between buffers once initialised
         by simply flipping pointers */
      for(i=0; i<vi->channels; i++)
        vb->pcm[i] = &vd->first_pcm[i*ci->blocksizes[1]];
        
    }
    vd->reset_pcmb = false;
      
    /* unpack_header enforces range checking */
    type=ci->map_type[ci->mode_param[mode]->mapping];
    return(_mapping_P[type]->inverse(vb,b->mode[mode]));
  }else{
    /* no pcm */
    vb->pcmend=0;
    vb->pcm=NULL;
    
    return(0);
  }
}

long vorbis_packet_blocksize(vorbis_info *vi,ogg_packet *op){
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  oggpack_buffer       opb;
  int                  mode;
 
  oggpack_readinit(&opb,op->packet);

  /* Check the packet type */
  if(oggpack_read(&opb,1)!=0){
    /* Oops.  This is not an audio data packet */
    return(OV_ENOTAUDIO);
  }

  {
    int modebits=0;
    int v=ci->modes;
    while(v>1){
      modebits++;
      v>>=1;
    }

    /* read our mode and pre/post windowsize */
    mode=oggpack_read(&opb,modebits);
  }
  if(mode==-1)return(OV_EBADPACKET);
  return(ci->blocksizes[ci->mode_param[mode]->blockflag]);
}


