#ifndef SIMULATOR
#include <private/coldfire.h>

void FLAC__lpc_restore_signal_order8_mac(const FLAC__int32 residual[], unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 data[]) __attribute__ ((section (".icode")));
void FLAC__lpc_restore_signal_order8_mac(const FLAC__int32 residual[], unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 data[])
{
	register const FLAC__int32 *qlp0 = &qlp_coeff[(order-1)];
	register FLAC__int32 sum;
	register const FLAC__int32 *history;
	
	SET_MACSR(0);
	history = &data[(-order)];
	SET_ACC(0, acc0);
	
	switch (order) {
	case 8:
		for( ; data_len != 0; --data_len) {
			asm volatile(
				"mov.l (%1), %%d0\n\t"
				"mov.l (%2), %%d1\n\t"
				"mac.l %%d0, %%d1, 4(%2), %%d1, %%acc0\n\t"
				"mov.l -4(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 8(%2), %%d1, %%acc0\n\t"
				"mov.l -8(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 12(%2), %%d1, %%acc0\n\t"
				"mov.l -12(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 16(%2), %%d1, %%acc0\n\t"
				"mov.l -16(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 20(%2), %%d1, %%acc0\n\t"
				"mov.l -20(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 24(%2), %%d1, %%acc0\n\t"
				"mov.l -24(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 28(%2), %%d1, %%acc0\n\t"
				"mov.l -28(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, %%acc0\n\t"
				"movclr.l %%acc0, %0"
				: "=ad" (sum) : "a" (qlp0), "a" (history) : "d0", "d1");
			++history;
			*(data++) = *(residual++) + (sum >> lp_quantization);
		}
		return;
	case 7:
		for( ; data_len != 0; --data_len) {
			asm volatile(
				"mov.l (%1), %%d0\n\t"
				"mov.l (%2), %%d1\n\t"
				"mac.l %%d0, %%d1, 4(%2), %%d1, %%acc0\n\t"
				"mov.l -4(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 8(%2), %%d1, %%acc0\n\t"
				"mov.l -8(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 12(%2), %%d1, %%acc0\n\t"
				"mov.l -12(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 16(%2), %%d1, %%acc0\n\t"
				"mov.l -16(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 20(%2), %%d1, %%acc0\n\t"
				"mov.l -20(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 24(%2), %%d1, %%acc0\n\t"
				"mov.l -24(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, %%acc0\n\t"
				"movclr.l %%acc0, %0"
				: "=ad" (sum) : "a" (qlp0), "a" (history) : "d0", "d1");
			++history;
			*(data++) = *(residual++) + (sum >> lp_quantization);
		} 
		return;
	case 6:
		for( ; data_len != 0; --data_len) {
			asm volatile(
				"mov.l (%1), %%d0\n\t"
				"mov.l (%2), %%d1\n\t"
				"mac.l %%d0, %%d1, 4(%2), %%d1, %%acc0\n\t"
				"mov.l -4(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 8(%2), %%d1, %%acc0\n\t"
				"mov.l -8(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 12(%2), %%d1, %%acc0\n\t"
				"mov.l -12(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 16(%2), %%d1, %%acc0\n\t"
				"mov.l -16(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 20(%2), %%d1, %%acc0\n\t"
				"mov.l -20(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, %%acc0\n\t"
				"movclr.l %%acc0, %0"
				: "=ad" (sum) : "a" (qlp0), "a" (history) : "d0", "d1");
			++history;
			*(data++) = *(residual++) + (sum >> lp_quantization);
		} 
		return;
	case 5:
		for( ; data_len != 0; --data_len) {
			asm volatile(
				"mov.l (%1), %%d0\n\t"
				"mov.l (%2), %%d1\n\t"
				"mac.l %%d0, %%d1, 4(%2), %%d1, %%acc0\n\t"
				"mov.l -4(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 8(%2), %%d1, %%acc0\n\t"
				"mov.l -8(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 12(%2), %%d1, %%acc0\n\t"
				"mov.l -12(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 16(%2), %%d1, %%acc0\n\t"
				"mov.l -16(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, %%acc0\n\t"
				"movclr.l %%acc0, %0"
				: "=ad" (sum) : "a" (qlp0), "a" (history) : "d0", "d1");
			++history;
			*(data++) = *(residual++) + (sum >> lp_quantization);
		} 
		return;
	case 4:
		for( ; data_len != 0; --data_len) {
			asm volatile(
				"mov.l (%1), %%d0\n\t"
				"mov.l (%2), %%d1\n\t"
				"mac.l %%d0, %%d1, 4(%2), %%d1, %%acc0\n\t"
				"mov.l -4(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 8(%2), %%d1, %%acc0\n\t"
				"mov.l -8(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 12(%2), %%d1, %%acc0\n\t"
				"mov.l -12(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, %%acc0\n\t"
				"movclr.l %%acc0, %0"
				: "=ad" (sum) : "a" (qlp0), "a" (history) : "d0", "d1");
			++history;
			*(data++) = *(residual++) + (sum >> lp_quantization);
		} 
		return;
	case 3:
		for( ; data_len != 0; --data_len) {
			asm volatile(
				"mov.l (%1), %%d0\n\t"
				"mov.l (%2), %%d1\n\t"
				"mac.l %%d0, %%d1, 4(%2), %%d1, %%acc0\n\t"
				"mov.l -4(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, 8(%2), %%d1, %%acc0\n\t"
				"mov.l -8(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, %%acc0\n\t"
				"movclr.l %%acc0, %0"
				: "=ad" (sum) : "a" (qlp0), "a" (history) : "d0", "d1");
			++history;
			*(data++) = *(residual++) + (sum >> lp_quantization);
		}
		return;
	case 2:
		for( ; data_len != 0; --data_len) {
			asm volatile(
				"mov.l (%1), %%d0\n\t"
				"mov.l (%2), %%d1\n\t"
				"mac.l %%d0, %%d1, 4(%2), %%d1, %%acc0\n\t"
				"mov.l -4(%1), %%d0\n\t"
				"mac.l %%d0, %%d1, %%acc0\n\t"
				"movclr.l %%acc0, %0"
				: "=ad" (sum) : "a" (qlp0), "a" (history) : "d0", "d1");
			++history;
			*(data++) = *(residual++) + (sum >> lp_quantization);
		}
		return;
	case 1:
		// won't gain anything by using mac here. 
		for( ; data_len != 0; --data_len) {
			sum = (qlp0[0] * (*(history++)));
			*(data++) = *(residual++) + (sum >> lp_quantization);
		} 
		return;
	}
}

#endif
