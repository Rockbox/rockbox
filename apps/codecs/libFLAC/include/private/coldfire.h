#ifndef SIMULATOR
#ifndef _FLAC_COLDFIRE_H
#define _FLAC_COLDFIRE_H

#include <FLAC/ordinals.h>

void FLAC__lpc_restore_signal_mcf5249(const FLAC__int32 residual[], unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 data[]);

#endif
#endif
