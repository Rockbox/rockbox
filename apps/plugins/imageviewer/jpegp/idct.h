extern void idct_sq(short *coef, int *q);		// <-- scaled integer idct WITH de-quantization 
extern void idct_s(int *t, short *y);			// <-- scaled integer idct

extern int zigzag[64];
extern int SCALEM[64];
extern unsigned char *CLIP;
