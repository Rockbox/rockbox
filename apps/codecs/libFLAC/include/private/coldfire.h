#ifndef SIMULATOR
#ifndef _FLAC_COLDFIRE_H
#define _FLAC_COLDFIRE_H

#include <FLAC/ordinals.h>

#define MACL(x, y, acc) \
	asm volatile ("mac.l %0, %1, %%" #acc \
	              : : "ad" ((x)), "ad" ((y)));

#define MACL_SHIFT(x, y, shift, acc) \
	asm volatile ("mac.l %0, %1, #" #shift ", %%" #acc \
	              : : "ad" ((x)), "ad" ((y)));

#define MSACL(x, y, acc) \
	asm volatile ("msac.l %0, %1, %%" #acc \
	    : : "ad" ((x)), "ad" ((y)));

#define MSACL_SHIFT(x, y, shift, acc) \
	asm volatile ("msac.l %0, %1, #" #shift ", %%" #acc \
	              : : "ad" ((x)), "ad" ((y)));

#define SET_MACSR(x) \
	asm volatile ("mov.l %0, %%macsr" : : "adi" ((x)));

#define TRANSFER_ACC(acca, accb) \
	asm volatile ("mov.l %" #acca ", %" #accb);

#define SET_ACC(x, acc) \
	asm volatile ("mov.l %0, %%" #acc : : "adi" ((x)));

#define GET_ACC(x, acc) \
	asm volatile ("mov.l %%" #acc ", %0\n\t" : "=ad" ((x)));

#define GET_ACC_CLR(x, acc) \
	asm volatile ("movclr.l %%" #acc ", %0\n\t" : "=ad" ((x)));

#define EMAC_SATURATE   0x00000080
#define EMAC_FRACTIONAL 0x00000020
#define EMAC_ROUND      0x00000010


void FLAC__lpc_restore_signal_order8_mac(const FLAC__int32 residual[], unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 data[]);

#endif
#endif
