/**********************************************************************************
*  
*   Scaled Integer 1-D IDCT based on the LLM-method that 
*   reduces the number of multiplications from 11 to to 6. 
*   Here further reduced to 3 using some dyadic decomposition. 
*  
*   The real scaling vector: 
*     v[0] = v[4] = 1.0;
*     v[2] = beta;
*     v[6] = alpha;
*     v[5] = v[3] = theta*M_SQRT2;
*     v[1] = v[7] = theta;
*  
*   The integer scaling matrix is derived as SCALEM = [v vt] << 12.
*  
*
*   Copyright (c) 2017 A. Tarpai 
*   
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*   
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*   
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*/


int SCALEM[64] = { // theta,12
 4096, 2276, 5352, 3218, 4096, 3218, 2217, 2276,
 2276, 1264, 2973, 1788, 2276, 1788, 1232, 1264,
 5352, 2973, 6992, 4205, 5352, 4205, 2896, 2973,
 3218, 1788, 4205, 2529, 3218, 2529, 1742, 1788,
 4096, 2276, 5352, 3218, 4096, 3218, 2217, 2276,
 3218, 1788, 4205, 2529, 3218, 2529, 1742, 1788,
 2217, 1232, 2896, 1742, 2217, 1742, 1200, 1232,
 2276, 1264, 2973, 1788, 2276, 1788, 1232, 1264,
};

static unsigned char clip_table[3*256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};
unsigned char *CLIP = clip_table + 256;


#define XADD(a,b,c,d) p=a+b, n=a-b, a=p+d, b=n+c, c=n-c, d=p-d

static void idct1(int *F, int *f)
{
	int p, n;

	XADD(F[1],F[7],F[5],F[3]);

	p= F[5]*45, n= F[3]*45;                //XROT(F[5],F[3],90,542,362);
	F[5]= ( n+p + (p<<2) + F[5] ) >> 7;	   // *181 = 45*4+1
	F[3]= ( n-p + (n<<2) + F[3] ) >> 7;    // *181
	
	p=F[1]<<8, n=F[7]<<8;                  //XROT(F[1],F[7],256,639,127);
	F[1]= ( n+p + (F[1]<<7) - F[1] ) >> 8; // *127
	F[7]= ( n-p + (F[7]<<7) - F[7] ) >> 8; // *127

	p= F[6];
	F[6]+= F[2];
	F[2]= ((F[2]-p) * 181 >> 7) - F[6];

	XADD(F[0],F[4],F[2],F[6]);	

	f[0*8]= F[0]+F[1];
	f[1*8]= F[4]+F[5];
	f[2*8]= F[2]+F[3];
	f[3*8]= F[6]+F[7];
	f[4*8]= F[6]-F[7];
	f[5*8]= F[2]-F[3];
	f[6*8]= F[4]-F[5];
	f[7*8]= F[0]-F[1];
}


/////////////// SCALED INTEGER IDCT AND MODIFIED LLM METHOD ///////////////////////////
//
//  Input: de-quantized coefficient block

extern void idct_s(int *t, short *y)
{
	int i, R[64], C[64];
	R[0]= ( t[0] + 4 ) * SCALEM[0];
	for (i=1; i<64; i++) R[i] = t[i] * SCALEM[i];

	for (i=0; i<8; i++) idct1(R+i*8, C+i);
	for (i=0; i<8; i++) idct1(C+i*8, R+i);
	
	for (i=0; i<64; i++) y[i] = CLIP[ R[i] >> 15 ];
}


/////////////// SCALED IDCT WITH DEQUANTIZATION IN ONE STEP /////////////////////////
//
//  Input: raw (un-zigzagged) coefficient block and the scaled quantization table

int zigzag[64] = {
	0,1,8,16,9,2,3,10,
	17,24,32,25,18,11,4,5,
	12,19,26,33,40,48,41,34,
	27,20,13,6,7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63,
};

extern void idct_sq(short *coef, int *sq)
{
	int i, R[64], C[64];
	R[0]= coef[0] * sq[0] + ((1024+4)<<12);					// DC dequant + scale + level-shift + rounding bias
	for (i=1; i<64; i++) R[zigzag[i]] = coef[i] * sq[i];	// AC dequant + scale (with zigzag)

	for (i=0; i<8; i++) idct1(R+i*8, C+i);
	for (i=0; i<8; i++) idct1(C+i*8, R+i);
	
	for (i=0; i<64; i++) coef[i] = CLIP[ R[i] >> 15 ];
}

