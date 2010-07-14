#include <inttypes.h>
#include "wmapro_mdct.h"
#include "mdct_tables.h"    /* for sincos_lookup_wmap */
#include "../lib/mdct_lookup.h" /* for revtab */
#include "../lib/fft.h"     /* for FFT data structures */
#include "codeclib.h"

#include "../lib/codeclib_misc.h" /* for XNPROD31 */
#include "wmapro_math.h"

void imdct_half(unsigned int nbits, int32_t *output, const int32_t *input){
    int k, n8, n4, n2, n, j;
    const int32_t *in1, *in2;
    FFTComplex *z = (FFTComplex *)output;

    n = 1 << nbits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;
    
    const int32_t *T = sincos_lookup_wmap + ((n2) - (1<<7));

    /* pre rotation */
    const int revtab_shift = (14- nbits);
    in1 = input;
    in2 = input + n2 - 1;
    for(k = 0; k < n4; k++) {
        j=revtab[k]>>revtab_shift;
        XNPROD31(*in2<<2, *in1<<2, T[1]<<14, T[0]<<14, &z[j].re, &z[j].im );
        in1 += 2;
        in2 -= 2;
        T += 2;
    }

    ff_fft_calc_c(nbits-2, z);

    /* post rotation + reordering */
    T = sincos_lookup_wmap + ((n2) - (1<<7)) + n4;
    const int32_t *V = T;
    for(k = 0; k < n8; k++) {
        int32_t r0, i0, r1, i1;
        XNPROD31(z[n8-k-1].im, z[n8-k-1].re, T[0]<<8, T[1]<<8, &r0, &i1 );
        XNPROD31(z[n8+k  ].im, z[n8+k  ].re, V[0]<<8, V[1]<<8, &r1, &i0 );
        z[n8-k-1].re = r0;
        z[n8-k-1].im = i0;
        z[n8+k  ].re = r1;
        z[n8+k  ].im = i1;
        T-=2;
        V+=2;
    }
}
