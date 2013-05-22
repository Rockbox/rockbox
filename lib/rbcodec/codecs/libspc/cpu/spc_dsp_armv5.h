#if !SPC_NOINTERP
/* Want scale optimized for smulw(y) */
#define GAUSS_TABLE_SCALE 4
#endif

#if !SPC_NOECHO

#define SPC_DSP_ECHO_APPLY

enum
{
    FIR_BUF_CNT   = FIR_BUF_HALF * 2,
    FIR_BUF_SIZE  = FIR_BUF_CNT * sizeof ( int32_t ),
    FIR_BUF_ALIGN = FIR_BUF_SIZE,
    FIR_BUF_MASK  = ~((FIR_BUF_ALIGN / 2) | (sizeof ( int32_t ) - 1))
};

/* Echo filter structure embedded in struct Spc_Dsp */
struct echo_filter
{
   /* fir_buf [i + 8] == fir_buf [i], to avoid wrap checking in FIR code */
    int32_t* ptr;
    /* FIR history is interleaved with guard to eliminate wrap checking
     * when convolving.
     * |LR|LR|LR|LR|LR|LR|LR|LR|--|--|--|--|--|--|--|--| */
    /* copy of echo FIR constants as int16_t, loaded as int32 for
     * halfword, packed multiples */
    int16_t coeff [VOICE_COUNT]; 
};
#endif /* SPC_NOECHO */
