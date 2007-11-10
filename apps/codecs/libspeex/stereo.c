/* Copyright (C) 2002 Jean-Marc Valin 
   File: stereo.c

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config-speex.h"
#endif

#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>
#include "math_approx.h"
#include "vq.h"
#include <math.h>

/*float e_ratio_quant[4] = {1, 1.26, 1.587, 2};*/
#ifndef FIXED_POINT
static const float e_ratio_quant[4] = {.25f, .315f, .397f, .5f};
#else
static const spx_word16_t e_ratio_quant[4] = {8192, 10332, 13009, 16384};
#endif

#ifndef SPEEX_DISABLE_ENCODER
void speex_encode_stereo(float *data, int frame_size, SpeexBits *bits)
{
   int i, tmp;
   float e_left=0, e_right=0, e_tot=0;
   float balance, e_ratio;
   for (i=0;i<frame_size;i++)
   {
      e_left  += ((float)data[2*i])*data[2*i];
      e_right += ((float)data[2*i+1])*data[2*i+1];
      data[i] =  .5*(((float)data[2*i])+data[2*i+1]);
      e_tot   += ((float)data[i])*data[i];
   }
   balance=(e_left+1)/(e_right+1);
   e_ratio = e_tot/(1+e_left+e_right);

   /*Quantization*/
   speex_bits_pack(bits, 14, 5);
   speex_bits_pack(bits, SPEEX_INBAND_STEREO, 4);
   
   balance=4*log(balance);

   /*Pack sign*/
   if (balance>0)
      speex_bits_pack(bits, 0, 1);
   else
      speex_bits_pack(bits, 1, 1);
   balance=floor(.5+fabs(balance));
   if (balance>30)
      balance=31;
   
   speex_bits_pack(bits, (int)balance, 5);
   
   /*Quantize energy ratio*/
   tmp=vq_index(&e_ratio, e_ratio_quant, 1, 4);
   speex_bits_pack(bits, tmp, 2);
}

void speex_encode_stereo_int(spx_int16_t *data, int frame_size, SpeexBits *bits)
{
   int i, tmp;
   float e_left=0, e_right=0, e_tot=0;
   float balance, e_ratio;
   for (i=0;i<frame_size;i++)
   {
      e_left  += ((float)data[2*i])*data[2*i];
      e_right += ((float)data[2*i+1])*data[2*i+1];
      data[i] =  .5*(((float)data[2*i])+data[2*i+1]);
      e_tot   += ((float)data[i])*data[i];
   }
   balance=(e_left+1)/(e_right+1);
   e_ratio = e_tot/(1+e_left+e_right);

   /*Quantization*/
   speex_bits_pack(bits, 14, 5);
   speex_bits_pack(bits, SPEEX_INBAND_STEREO, 4);
   
   balance=4*log(balance);

   /*Pack sign*/
   if (balance>0)
      speex_bits_pack(bits, 0, 1);
   else
      speex_bits_pack(bits, 1, 1);
   balance=floor(.5+fabs(balance));
   if (balance>30)
      balance=31;
   
   speex_bits_pack(bits, (int)balance, 5);
   
   /*Quantize energy ratio*/
   tmp=vq_index(&e_ratio, e_ratio_quant, 1, 4);
   speex_bits_pack(bits, tmp, 2);
}
#endif /* SPEEX_DISABLE_ENCODER */

/* We don't want to decode to floats yet, disable */
#if 0
void speex_decode_stereo(float *data, int frame_size, SpeexStereoState *stereo)
{
   float balance, e_ratio;
   int i;
   float e_tot=0, e_left, e_right, e_sum;

   balance=stereo->balance;
   e_ratio=stereo->e_ratio;
   for (i=frame_size-1;i>=0;i--)
   {
      e_tot += ((float)data[i])*data[i];
   }
   e_sum=e_tot/e_ratio;
   e_left  = e_sum*balance / (1+balance);
   e_right = e_sum-e_left;

   e_left  = sqrt(e_left/(e_tot+.01));
   e_right = sqrt(e_right/(e_tot+.01));

   for (i=frame_size-1;i>=0;i--)
   {
      float ftmp=data[i];
      stereo->smooth_left  = .98*stereo->smooth_left  + .02*e_left;
      stereo->smooth_right = .98*stereo->smooth_right + .02*e_right;
      data[2*i] = stereo->smooth_left*ftmp;
      data[2*i+1] = stereo->smooth_right*ftmp;
   }
}
#endif

void speex_decode_stereo_int(spx_int16_t *data, int frame_size, SpeexStereoState *stereo)
{
   int i;
   spx_word32_t balance;
   spx_word16_t e_left, e_right, e_ratio;

   balance=stereo->balance;
   e_ratio=stereo->e_ratio;

   /* These two are Q14, with max value just below 2. */
   e_right = DIV32(QCONST32(1., 22), spx_sqrt(MULT16_32_Q15(e_ratio, ADD32(QCONST32(1., 16), balance))));
   e_left = SHR32(MULT16_16(spx_sqrt(balance), e_right), 8);

   for (i=frame_size-1;i>=0;i--)
   {
      spx_word16_t tmp=data[i];
      stereo->smooth_left = EXTRACT16(PSHR32(MAC16_16(MULT16_16(stereo->smooth_left, QCONST16(0.98, 15)), e_left, QCONST16(0.02, 15)), 15));
      stereo->smooth_right = EXTRACT16(PSHR32(MAC16_16(MULT16_16(stereo->smooth_right, QCONST16(0.98, 15)), e_right, QCONST16(0.02, 15)), 15));
      data[2*i] = MULT16_16_P14(stereo->smooth_left, tmp);
      data[2*i+1] = MULT16_16_P14(stereo->smooth_right, tmp);
   }
}

int speex_std_stereo_request_handler(SpeexBits *bits, void *state, void *data)
{
   SpeexStereoState *stereo;
   spx_word16_t sign=1;
   int tmp;

   stereo = (SpeexStereoState*)data;
   if (speex_bits_unpack_unsigned(bits, 1))
      sign=-1;
   tmp = speex_bits_unpack_unsigned(bits, 5);
#ifndef FIXED_POINT
   stereo->balance = exp(sign*.25*tmp);
#else
   stereo->balance = spx_exp(MULT16_16(sign, SHL16(tmp, 9)));
#endif
   tmp = speex_bits_unpack_unsigned(bits, 2);
   stereo->e_ratio = e_ratio_quant[tmp];

   return 0;
}
