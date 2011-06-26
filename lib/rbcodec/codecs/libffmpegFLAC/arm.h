#ifndef _FLAC_ARM_H
#define _FLAC_ARM_H

#include "bitstream.h"

void lpc_decode_arm(int blocksize, int qlevel, int pred_order, int32_t* data, int* coeffs);

#endif
