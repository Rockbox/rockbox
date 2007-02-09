/* Copyright (C) 2002 Jean-Marc Valin */
/**
   @file math_approx.h
   @brief Various math approximation functions for Speex
*/
/*
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

#ifndef MATH_APPROX_H
#define MATH_APPROX_H


#include "misc.h"

#ifdef FIXED_POINT
spx_word16_t spx_cos(spx_word16_t x);
spx_word16_t spx_sqrt(spx_word32_t x);
float spx_sqrtf(float arg);
spx_word16_t spx_acos(spx_word16_t x);
float spx_floor(float x);
float spx_exp(float x);
extern const float exp_lookup_int[];
/** Returns: Math.exp((idx-10) / 8.0) Range:0-32*/
static inline float spx_exp_lookup(int xf){
	return exp_lookup_int[xf];
}
//Placeholders:
float pow(float a,float b);
float log(float l);
float fabs(float l);
float sin(float l);
//float floor(float l);

#define floor spx_floor
#define exp spx_exp
#define sqrt spx_sqrt
#define acos spx_acos
#define cos spx_cos

#else
#define spx_sqrt sqrt
#define spx_acos acos
#endif

#endif
