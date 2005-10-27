#ifndef _FLAC_COLDFIRE_H
#define _FLAC_COLDFIRE_H

#include "bitstream.h"

void lpc_decode_emac(int blocksize, int qlevel, int pred_order, int32_t* data, int* coeffs);

#endif
