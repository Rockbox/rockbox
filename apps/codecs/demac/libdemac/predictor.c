/*

libdemac - A Monkey's Audio decoder

$Id:$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#include <inttypes.h>
#include <string.h>

#include "parser.h"
#include "predictor.h"

#include "vector_math32.h"

/* Return 0 if x is zero, -1 if x is positive, 1 if x is negative */
#define SIGN(x) (x) ? (((x) > 0) ? -1 : 1) : 0

static const int32_t initial_coeffs[4] = {
  360, 317, -109, 98
};

static void init_predictor(struct predictor_t* p)
{
    /* Zero the history buffers */
    memset(p->historybuffer, 0, (PREDICTOR_ORDER*4) * sizeof(int32_t));
    p->delayA = p->historybuffer + PREDICTOR_ORDER*4;
    p->delayB = p->historybuffer + PREDICTOR_ORDER*3;
    p->adaptcoeffsA = p->historybuffer + PREDICTOR_ORDER*2;
    p->adaptcoeffsB = p->historybuffer + PREDICTOR_ORDER;

    /* Initialise and zero the co-efficients */
    memcpy(p->coeffsA, initial_coeffs, sizeof(initial_coeffs));
    memset(p->coeffsB, 0, sizeof(p->coeffsB));

    p->filterA = 0;
    p->filterB = 0;
    
    p->lastA = 0;
}

static int do_predictor_decode(struct predictor_t* p, int32_t A, int32_t B)
{
    int32_t predictionA, predictionB, currentA;

    p->delayA[0] = p->lastA;
    p->delayA[-1] = p->delayA[0] - p->delayA[-1];

    predictionA = scalarproduct4_rev32(p->coeffsA,p->delayA);

    /*  Apply a scaled first-order filter compression */
    p->delayB[0] = B - ((p->filterB * 31) >> 5);
    p->filterB = B;

    p->delayB[-1] = p->delayB[0] - p->delayB[-1];

    predictionB = scalarproduct5_rev32(p->coeffsB,p->delayB);

    currentA = A + ((predictionA + (predictionB >> 1)) >> 10);

    p->adaptcoeffsA[0] = SIGN(p->delayA[0]);
    p->adaptcoeffsA[-1] = SIGN(p->delayA[-1]);

    p->adaptcoeffsB[0] = SIGN(p->delayB[0]);
    p->adaptcoeffsB[-1] = SIGN(p->delayB[-1]);

    if (A > 0) 
    {
        vector_sub4_rev32(p->coeffsA, p->adaptcoeffsA);
        vector_sub5_rev32(p->coeffsB, p->adaptcoeffsB);
    }
    else if (A < 0) 
    {
        vector_add4_rev32(p->coeffsA, p->adaptcoeffsA);
        vector_add5_rev32(p->coeffsB, p->adaptcoeffsB);
    }

    p->delayA++;
    p->delayB++;
    p->adaptcoeffsA++;
    p->adaptcoeffsB++;

    /* Have we filled the history buffer? */
    if (p->delayA == p->historybuffer + HISTORY_SIZE + (PREDICTOR_ORDER*4)) {
        memmove(p->historybuffer, p->delayA - (PREDICTOR_ORDER*4), 
                (PREDICTOR_ORDER*4) * sizeof(int32_t));
        p->delayA = p->historybuffer + PREDICTOR_ORDER*4;
        p->delayB = p->historybuffer + PREDICTOR_ORDER*3;
        p->adaptcoeffsA = p->historybuffer + PREDICTOR_ORDER*2;
        p->adaptcoeffsB = p->historybuffer + PREDICTOR_ORDER;
    }

    p->lastA = currentA;
    p->filterA =  currentA + ((p->filterA * 31) >> 5);

    return p->filterA;
}

static int32_t X;

void init_predictor_decoder(struct ape_ctx_t* ape_ctx)
{
    X = 0;

    init_predictor(&ape_ctx->predictorY);
    init_predictor(&ape_ctx->predictorX);
}

int predictor_decode_stereo(struct ape_ctx_t* ape_ctx, int32_t* decoded0, int32_t* decoded1, int count) ICODE_ATTR;
int predictor_decode_stereo(struct ape_ctx_t* ape_ctx, int32_t* decoded0, int32_t* decoded1, int count)
{
    while (count--)
    {
        *decoded0 = do_predictor_decode(&ape_ctx->predictorY, *decoded0, X);
        X = do_predictor_decode(&ape_ctx->predictorX, *decoded1, *(decoded0)++);
        *(decoded1++) = X;
    }

    return 0;
}

int predictor_decode_mono(struct ape_ctx_t* ape_ctx, int32_t* decoded0, int count)
{
    struct predictor_t* p = &ape_ctx->predictorY;
    int32_t predictionA, currentA, A;

    currentA = p->lastA;

    while (count--)
    {
        A = *decoded0;

        p->delayA[0] = currentA;
        p->delayA[-1] = p->delayA[0] - p->delayA[-1];

        predictionA = (p->delayA[0] * p->coeffsA[0]) + 
                      (p->delayA[-1] * p->coeffsA[1]) + 
                      (p->delayA[-2] * p->coeffsA[2]) + 
                      (p->delayA[-3] * p->coeffsA[3]);

        currentA = A + (predictionA >> 10);

        p->adaptcoeffsA[0] = SIGN(p->delayA[0]);
        p->adaptcoeffsA[-1] = SIGN(p->delayA[-1]);
        
        if (A > 0) 
        {
            p->coeffsA[0] -= p->adaptcoeffsA[0];
            p->coeffsA[1] -= p->adaptcoeffsA[-1];
            p->coeffsA[2] -= p->adaptcoeffsA[-2];
            p->coeffsA[3] -= p->adaptcoeffsA[-3];
        }
        else if (A < 0) 
        {
            p->coeffsA[0] += p->adaptcoeffsA[0];
            p->coeffsA[1] += p->adaptcoeffsA[-1];
            p->coeffsA[2] += p->adaptcoeffsA[-2];
            p->coeffsA[3] += p->adaptcoeffsA[-3];
        }

        p->delayA++;
        p->adaptcoeffsA++;

        /* Have we filled the history buffer? */
        if (p->delayA == p->historybuffer + HISTORY_SIZE + (PREDICTOR_ORDER*4)) {
            memmove(p->historybuffer, p->delayA - (PREDICTOR_ORDER*4), 
                    (PREDICTOR_ORDER*4) * sizeof(int32_t));
            p->delayA = p->historybuffer + PREDICTOR_ORDER*4;
            p->adaptcoeffsA = p->historybuffer + PREDICTOR_ORDER*2;
        }

        p->filterA =  currentA + ((p->filterA * 31) >> 5);
        *(decoded0++) = p->filterA;
    }

    p->lastA = currentA;

    return 0;
}
