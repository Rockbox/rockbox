/*
 * filter.h
 *
 * Description:  TTAv1 filter functions
 * Developed by: Alexander Djourik <ald@true-audio.com>
 *               Pavel Zhilin <pzh@true-audio.com>
 *
 * Copyright (c) 2004 True Audio Software. All rights reserved.
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the True Audio Software nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FILTER_H
#define FILTER_H

///////// Filter Settings //////////
static int flt_set[3] = {10, 9, 10};

#if defined(CPU_ARM) || defined(CPU_COLDFIRE)
int hybrid_filter(fltst *fs, int *in); /* implements in filter_arm.S */

#else

static inline void
memshl (register int *pA) {
    register int *pB = pA + 16;

    *pA++ = *pB++;
    *pA++ = *pB++;
    *pA++ = *pB++;
    *pA++ = *pB++;
    *pA++ = *pB++;
    *pA++ = *pB++;
    *pA++ = *pB++;
    *pA   = *pB;
}

static inline void
hybrid_filter (fltst *fs, int *in) {
    register int *pA = fs->dl + fs->index;
    register int *pB = fs->qm;
    register int *pM = fs->dx + fs->index;
    register int sum = fs->round;

    if (!fs->error) {
        sum += *pA++ * *pB++;
        sum += *pA++ * *pB++;
        sum += *pA++ * *pB++;
        sum += *pA++ * *pB++;
        sum += *pA++ * *pB++;
        sum += *pA++ * *pB++;
        sum += *pA++ * *pB++;
        sum += *pA   * *pB;
        pM += 8;
    } else if (fs->error < 0) {
        sum += *pA++ * (*pB++ -= *pM++);
        sum += *pA++ * (*pB++ -= *pM++);
        sum += *pA++ * (*pB++ -= *pM++);
        sum += *pA++ * (*pB++ -= *pM++);
        sum += *pA++ * (*pB++ -= *pM++);
        sum += *pA++ * (*pB++ -= *pM++);
        sum += *pA++ * (*pB++ -= *pM++);
        sum += *pA   * (*pB   -= *pM++);
    } else {
        sum += *pA++ * (*pB++ += *pM++);
        sum += *pA++ * (*pB++ += *pM++);
        sum += *pA++ * (*pB++ += *pM++);
        sum += *pA++ * (*pB++ += *pM++);
        sum += *pA++ * (*pB++ += *pM++);
        sum += *pA++ * (*pB++ += *pM++);
        sum += *pA++ * (*pB++ += *pM++);
        sum += *pA   * (*pB   += *pM++);
    }

    pB = pA++;
    fs->error = *in;
    *in += (sum >> fs->shift);
    *pA = *in;

    *pM-- = ((*pB-- >> 30) | 1) << 2;
    *pM-- = ((*pB-- >> 30) | 1) << 1;
    *pM-- = ((*pB-- >> 30) | 1) << 1;
    *pM   = ((*pB   >> 30) | 1);

    *(pA-1) = *(pA-0) - *(pA-1);
    *(pA-2) = *(pA-1) - *(pA-2);
    *(pA-3) = *(pA-2) - *(pA-3);

    /*
     * Rockbox speciffic
     *     in order to make speed up, memshl() is executed at the rate once every 16 times. 
     */
    if (++fs->index == 16)
    {
        memshl (fs->dl);
        memshl (fs->dx);
        fs->index = 0;
    }
}
#endif

static inline void
filter_init (fltst *fs, int shift) {
    ci->memset (fs, 0, sizeof(fltst));
    fs->shift = shift;
    fs->round = 1 << (shift - 1);
    fs->index = 0;
}

#endif /* FILTER_H */
