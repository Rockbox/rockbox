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

/* Decodes up to n Rice residuals with parameter k, returns how many it
   produced, and leaves gb->index where the C reader would have.  Handles only
   the common case; the caller decodes the remainder in C. */
int flac_rice_decode_arm(int32_t *out, int n, int k, GetBitContext *gb);

/* Channel decorrelation for each stereo mode, and the shift up to
   FLAC_OUTPUT_DEPTH: rewrites ch0 and ch1 in place as left and right. */
void flac_decorrelate_indep_arm(int32_t *ch0, int32_t *ch1, int n, int scale);
void flac_decorrelate_ls_arm(int32_t *ch0, int32_t *ch1, int n, int scale);
void flac_decorrelate_rs_arm(int32_t *ch0, int32_t *ch1, int n, int scale);
void flac_decorrelate_ms_arm(int32_t *ch0, int32_t *ch1, int n, int scale);

#endif
