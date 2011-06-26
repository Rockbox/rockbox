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

 function: window functions

 ********************************************************************/

#include "config-tremor.h"
#include <string.h>
#include <math.h>
#include "os.h"
#include "misc.h"
#include "window.h"
#include "window_lookup.h"

const void *_vorbis_window(int type, int left){

  switch(type){
  case 0:

    switch(left){
    case 32:
      return vwin64;
    case 64:
      return vwin128;
    case 128:
      return vwin256;
    case 256:
      return vwin512;
    case 512:
      return vwin1024;
    case 1024:
      return vwin2048;
    case 2048:
      return vwin4096;
    case 4096:
      return vwin8192;
    default:
      return(0);
    }
    break;
  default:
    return(0);
  }
}
#if 0
void _vorbis_apply_window(ogg_int32_t *d,const void *window_p[2],
                          long *blocksizes,
                          int lW,int W,int nW){
  LOOKUP_T *window[2]={window_p[0],window_p[1]};
  long n=blocksizes[W];
  long ln=blocksizes[lW];
  long rn=blocksizes[nW];

  long leftbegin=n/4-ln/4;
  long leftend=leftbegin+ln/2;

  long rightbegin=n/2+n/4-rn/4;
  long rightend=rightbegin+rn/2;

  /* Following memset is not required - we are careful to only overlap/add the
     regions that geniunely overlap in the window region, and the portions
     outside that region are not added (so don't need to be zerod). see block.c
     memset((void *)&d[0], 0, sizeof(ogg_int32_t)*leftbegin); */

  ogg_vect_mult_fw(&d[leftbegin], &window[lW][0], leftend-leftbegin);
  ogg_vect_mult_bw(&d[rightbegin], &window[nW][rn/2-1], rightend-rightbegin);

  /* Again - memset not needed
     memset((void *)&d[rightend], 0, sizeof(ogg_int32_t)*(n-rightend)); */
}
#endif
