#ifndef _WMAPRO_MDCT_H_
#define _WMAPRO_MDCT_H_

#include <inttypes.h>

void imdct_half(unsigned int nbits, int32_t *output, const int32_t *input);

#endif
