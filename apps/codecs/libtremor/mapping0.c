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

 function: channel mapping 0 implementation

 ********************************************************************/

#include "config-tremor.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "ivorbiscodec.h"
#include "codeclib.h"
#include "codec_internal.h"
#include "codebook.h"
#include "window.h"
#include "registry.h"
#include "misc.h"
#include <codecs/lib/codeclib.h>

/* simplistic, wasteful way of doing this (unique lookup for each
   mode/submapping); there should be a central repository for
   identical lookups.  That will require minor work, so I'm putting it
   off as low priority.

   Why a lookup for each backend in a given mode?  Because the
   blocksize is set by the mode, and low backend lookups may require
   parameters from other areas of the mode/mapping */

typedef struct {
  vorbis_info_mode *mode;
  vorbis_info_mapping0 *map;

  vorbis_look_floor **floor_look;

  vorbis_look_residue **residue_look;

  vorbis_func_floor **floor_func;
  vorbis_func_residue **residue_func;

  int ch;
  long lastframe; /* if a different mode is called, we need to 
                     invalidate decay */
} vorbis_look_mapping0;

static void mapping0_free_info(vorbis_info_mapping *i){
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)i;
  if(info){
    memset(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

static void mapping0_free_look(vorbis_look_mapping *look){
  int i;
  vorbis_look_mapping0 *l=(vorbis_look_mapping0 *)look;
  if(l){

    for(i=0;i<l->map->submaps;i++){
      l->floor_func[i]->free_look(l->floor_look[i]);
      l->residue_func[i]->free_look(l->residue_look[i]);
    }

    _ogg_free(l->floor_func);
    _ogg_free(l->residue_func);
    _ogg_free(l->floor_look);
    _ogg_free(l->residue_look);
    memset(l,0,sizeof(*l));
    _ogg_free(l);
  }
}

static vorbis_look_mapping *mapping0_look(vorbis_dsp_state *vd,vorbis_info_mode *vm,
                          vorbis_info_mapping *m){
  int i;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)_ogg_calloc(1,sizeof(*look));
  vorbis_info_mapping0 *info=look->map=(vorbis_info_mapping0 *)m;
  look->mode=vm;
  
  look->floor_look=(vorbis_look_floor **)_ogg_calloc(info->submaps,sizeof(*look->floor_look));

  look->residue_look=(vorbis_look_residue **)_ogg_calloc(info->submaps,sizeof(*look->residue_look));

  look->floor_func=(vorbis_func_floor **)_ogg_calloc(info->submaps,sizeof(*look->floor_func));
  look->residue_func=(vorbis_func_residue **)_ogg_calloc(info->submaps,sizeof(*look->residue_func));
  
  for(i=0;i<info->submaps;i++){
    int floornum=info->floorsubmap[i];
    int resnum=info->residuesubmap[i];

    look->floor_func[i]=_floor_P[ci->floor_type[floornum]];
    look->floor_look[i]=look->floor_func[i]->
      look(vd,vm,ci->floor_param[floornum]);
    look->residue_func[i]=_residue_P[ci->residue_type[resnum]];
    look->residue_look[i]=look->residue_func[i]->
      look(vd,vm,ci->residue_param[resnum]);
    
  }

  look->ch=vi->channels;

  return(look);
}

static int ilog(unsigned int v){
  int ret=0;
  if(v)--v;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}


/* also responsible for range checking */
static vorbis_info_mapping *mapping0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)_ogg_calloc(1,sizeof(*info));
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  memset(info,0,sizeof(*info));

  if(oggpack_read(opb,1))
    info->submaps=oggpack_read(opb,4)+1;
  else
    info->submaps=1;

  if(oggpack_read(opb,1)){
    info->coupling_steps=oggpack_read(opb,8)+1;

    for(i=0;i<info->coupling_steps;i++){
      int testM=info->coupling_mag[i]=oggpack_read(opb,ilog(vi->channels));
      int testA=info->coupling_ang[i]=oggpack_read(opb,ilog(vi->channels));

      if(testM<0 || 
         testA<0 || 
         testM==testA || 
         testM>=vi->channels ||
         testA>=vi->channels) goto err_out;
    }

  }

  if(oggpack_read(opb,2)>0)goto err_out; /* 2,3:reserved */
    
  if(info->submaps>1){
    for(i=0;i<vi->channels;i++){
      info->chmuxlist[i]=oggpack_read(opb,4);
      if(info->chmuxlist[i]>=info->submaps)goto err_out;
    }
  }
  for(i=0;i<info->submaps;i++){
    int temp=oggpack_read(opb,8);
    if(temp>=ci->times)goto err_out;
    info->floorsubmap[i]=oggpack_read(opb,8);
    if(info->floorsubmap[i]>=ci->floors)goto err_out;
    info->residuesubmap[i]=oggpack_read(opb,8);
    if(info->residuesubmap[i]>=ci->residues)goto err_out;
  }

  return info;

 err_out:
  mapping0_free_info(info);
  return(NULL);
}

#ifdef _ARM_ASSEM_
#define MAGANG( _mag, _ang )\
{\
      register int temp;\
      asm( "cmp %[mag], #0\n\t"\
           "cmpgt %[ang], #0\n\t"\
           "subgt %[ang], %[mag], %[ang]\n\t"\
           "bgt 1f\n\t"\
           "cmp %[mag], #0\n\t"\
           "cmple %[ang], #0\n\t"\
           "addgt %[temp], %[mag], %[ang]\n\t"\
           "suble %[temp], %[mag], %[ang]\n\t"\
           "cmp %[ang], #0\n\t"\
           "movle %[ang], %[mag]\n\t"\
           "movle %[mag], %[temp]\n\t"\
           "movgt %[ang], %[temp]\n\t"\
           "1:\n\t"\
           : [mag] "+r" ( ( _mag ) ), [ang] "+r" ( ( _ang ) ), [temp] "=&r" (temp)\
           :\
           : "cc" );\
}         

static inline void channel_couple(ogg_int32_t *pcmM, ogg_int32_t *pcmA, int n)
{
    ogg_int32_t * const pcmMend = pcmM + n/2;
    while(LIKELY(pcmM < pcmMend))
    {
      register int M0 asm("r2"),M1 asm("r3"),M2 asm("r4"),M3 asm("r5");
      register int A0 asm("r6"),A1 asm("r7"),A2 asm("r8"),A3 asm("r9");
      asm volatile( "ldmia %[pcmM], {%[M0], %[M1], %[M2], %[M3]}\n\t"
                    "ldmia %[pcmA], {%[A0], %[A1], %[A2], %[A3]}\n\t"
                    : [M0] "=r" (M0), [M1] "=r" (M1), [M2] "=r" (M2), [M3] "=r" (M3),
                      [A0] "=r" (A0), [A1] "=r" (A1), [A2] "=r" (A2), [A3] "=r" (A3)
                    : [pcmM] "r" (pcmM), [pcmA] "r" (pcmA) );
      MAGANG( M0, A0 );
      MAGANG( M1, A1 );
      MAGANG( M2, A2 );
      MAGANG( M3, A3 );
      asm volatile( "stmia %[pcmM]!, {%[M0], %[M1], %[M2], %[M3]}\n\t"
                    "stmia %[pcmA]!, {%[A0], %[A1], %[A2], %[A3]}\n\t"
                    : [pcmM] "+r" (pcmM), [pcmA] "+r" (pcmA)
                    : [M0] "r" (M0), [M1] "r" (M1), [M2] "r" (M2), [M3] "r" (M3),
                      [A0] "r" (A0), [A1] "r" (A1), [A2] "r" (A2), [A3] "r" (A3) );
    }
}    
#else
static inline void channel_couple(ogg_int32_t *pcmM, ogg_int32_t *pcmA, int n)
{
    int j;
    for(j=0;j<n/2;j++){
      ogg_int32_t mag = pcmM[j], ang = pcmA[j];
      if(mag>0)
        if(ang>0)
          pcmA[j]=mag-ang;
        else{
          pcmA[j]=mag;
          pcmM[j]=mag+ang;
        }
      else
        if(ang>0)
          pcmA[j]=mag+ang;
        else{
          pcmA[j]=mag;
          pcmM[j]=mag-ang;
        }
    }
}
#endif

static int mapping0_inverse(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=(codec_setup_info *)vi->codec_setup;
  private_state        *b=(private_state *)vd->backend_state;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *info=look->map;

  int                   i,j;
  long                  n=vb->pcmend=ci->blocksizes[vb->W];

  /* bounded mapping arrays instead of using alloca();
     avoids memory leak; we can only deal with stereo anyway */
  ogg_int32_t *pcmbundle[CHANNELS];
  int   zerobundle[CHANNELS];
  int   nonzero[CHANNELS];
  void *floormemo[CHANNELS];

  /* time domain information decode (note that applying the
     information would have to happen later; we'll probably add a
     function entry to the harness for that later */
  /* NOT IMPLEMENTED */

  /* recover the spectral envelope; store it in the PCM vector for now */
  for(i=0;i<vi->channels;i++){
    int submap=info->chmuxlist[i];
    floormemo[i]=look->floor_func[submap]->
      inverse1(vb,look->floor_look[submap]);
    if(floormemo[i])
      nonzero[i]=1;
    else
      nonzero[i]=0;      
    memset(vb->pcm[i],0,sizeof(*vb->pcm[i])*n/2);
  }

  /* channel coupling can 'dirty' the nonzero listing */
  for(i=0;i<info->coupling_steps;i++){
    if(nonzero[info->coupling_mag[i]] ||
       nonzero[info->coupling_ang[i]]){
      nonzero[info->coupling_mag[i]]=1; 
      nonzero[info->coupling_ang[i]]=1; 
    }
  }

  /* recover the residue into our working vectors */
  for(i=0;i<info->submaps;i++){
    int ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(info->chmuxlist[j]==i){
        if(nonzero[j])
          zerobundle[ch_in_bundle]=1;
        else
          zerobundle[ch_in_bundle]=0;
        pcmbundle[ch_in_bundle++]=vb->pcm[j];
      }
    }

    look->residue_func[i]->inverse(vb,look->residue_look[i],
                                   pcmbundle,zerobundle,ch_in_bundle);
  }

  //for(j=0;j<vi->channels;j++)
  //_analysis_output("coupled",seq+j,vb->pcm[j],-8,n/2,0,0);


  /* channel coupling */
  for(i=info->coupling_steps-1;i>=0;i--){
    ogg_int32_t *pcmM=vb->pcm[info->coupling_mag[i]];
    ogg_int32_t *pcmA=vb->pcm[info->coupling_ang[i]];
    channel_couple(pcmM,pcmA,n);
  }

  //for(j=0;j<vi->channels;j++)
  //_analysis_output("residue",seq+j,vb->pcm[j],-8,n/2,0,0);

  //for(j=0;j<vi->channels;j++)
  //_analysis_output("mdct",seq+j,vb->pcm[j],-24,n/2,0,1);

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */

  for(i=0;i<vi->channels;i++){
    ogg_int32_t *pcm=vb->pcm[i];
    int submap=info->chmuxlist[i];
    
    if(nonzero[i]) {
      /* compute and apply spectral envelope */
      look->floor_func[submap]->
        inverse2(vb,look->floor_look[submap],floormemo[i],pcm);
        
      ff_imdct_calc(ci->blocksizes_nbits[vb->W],
                    (int32_t*)pcm,
                    (int32_t*)pcm);
      /* window the data */
      _vorbis_apply_window(pcm,b->window,ci->blocksizes,vb->lW,vb->W,vb->nW);
    }
    else 
      memset(pcm, 0, sizeof(ogg_int32_t)*n);
  }

  //for(j=0;j<vi->channels;j++)
  //_analysis_output("imdct",seq+j,vb->pcm[j],-24,n,0,0);

  //for(j=0;j<vi->channels;j++)
  //_analysis_output("window",seq+j,vb->pcm[j],-24,n,0,0);

  //seq+=vi->channels;
  /* all done! */
  return(0);
}

/* export hooks */
const vorbis_func_mapping mapping0_exportbundle ICONST_ATTR ={
  &mapping0_unpack,
  &mapping0_look,
  &mapping0_free_info,
  &mapping0_free_look,
  &mapping0_inverse
};

