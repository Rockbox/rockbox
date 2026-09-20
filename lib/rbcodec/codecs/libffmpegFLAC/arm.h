#ifndef _FLAC_ARM_H
#define _FLAC_ARM_H

#include "bitstream.h"

void lpc_decode_arm(int blocksize, int qlevel, int pred_order, int32_t* data, int* coeffs);

/* The 64-bit LPC filter, the wide path of decode_subframe_lpc.  Both have the
   same contract as flac_lpc_32_c in decoder.c.  The _narrow form uses the
   ARMv5E packed 16-bit multiplies and is only valid when every history sample
   fits a signed halfword, which bps <= 16 guarantees. */
void flac_lpc_32_arm(int32_t *decoded, int coeffs[], int pred_order,
                     int qlevel, int len);
void flac_lpc_32_arm_narrow(int32_t *decoded, int coeffs[], int pred_order,
                            int qlevel, int len);

#endif
