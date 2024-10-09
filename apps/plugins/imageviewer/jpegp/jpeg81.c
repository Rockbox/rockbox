/************************************************************************
jpeg81.c
  
  An ITU T.81 JPEG coefficient-decoder - without de-quantization and IDCT. 
  Used for conformance testing of ITU T.83 data (a little verbose output). 
  Allocates full image coefficient buffer.

  Supports: 
  - JPEG Interchange Format (JIF)
  - DCT- and Lossless operation
  - Huffman- and arithmetic coding
  - Sequential- and progressive mode
  - max. 4 components
  - all sub-sampling
  - 8/12 - bit samples for DCT
  - 2-16 - bit for Lossless

  It does not support the Hierarchial mode. 
  TODO: more error checking.


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

#include "GETC.h"
#include "rb_glue.h"
#include "jpeg81.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
///////////////////////////////////////// LOSSLESS /////////////////////////////////////////

static int P1(struct COMP *C, TSAMP *samp)	// Px = Ra
{
	return samp[-1];
}

static int P2(struct COMP *C, TSAMP *samp)	// Px = Rb
{
	return samp[-C->du_width];
}

static int P3(struct COMP *C, TSAMP *samp)	// Px = Rc
{
	return samp[-C->du_width-1];
}

static int P4(struct COMP *C, TSAMP *samp)	// Px = Ra + Rb - Rc
{
	return samp[-1] + samp[-C->du_width] - samp[-C->du_width-1];
}

static int P5(struct COMP *C, TSAMP *samp)	// Px = Ra + ((Rb - Rc)/2)
{
	return samp[-1] + ( (samp[-C->du_width] - samp[-C->du_width-1]) >> 1 );
}

static int P6(struct COMP *C, TSAMP *samp)	// Px = Rb + ((Ra - Rc)/2)
{
	return samp[-C->du_width] + ( (samp[-1] - samp[-C->du_width-1]) >> 1 );
}

static int P7(struct COMP *C, TSAMP *samp)	// Px = (Ra + Rb)/2
{
	return (samp[-1] + samp[-C->du_width]) >> 1;
}

static int (*PN[])(struct COMP *C, TSAMP *samp) = {
	0, P1, P2, P3, P4, P5, P6, P7 
};

static int Predx(struct JPEGD *j, struct COMP *C, int x, int y, TSAMP *samp)
{
	int Px;
		if (!x) {
			if (y) Px= samp[-C->du_width];	// Px = Rb (P2)
			else Px= 1<<(j->P-1);	// 'P0'
		}
		else {
			if (y) Px= PN[j->Ss](C, samp); // (PN)
			else Px= samp[-1];	// Px = Ra (P1)
		}
	return Px;
}


//////////////////////////////////// HUFFMAN ////////////////////////////////////////////////

static int Byte_in_huff(struct JPEGD *j)
{
	j->ScanByte = GETC();
	if ( 0xFF == j->ScanByte )
	{
		int marker = GETC();
		if ( marker )			// DEBUG: ERR in Huffman: 
		{
			printf("%08X: FF%02x\n", TELL()-2, marker);
			printf("STREAM ERROR: marker found in Huffman ECS\n");
		}
		//else skip zero-stuffing
	}
	return j->ScanByte;
}

static int GetBit(struct JPEGD *j)
{
	if ( j->ScanBit ) return (j->ScanByte >> --j->ScanBit) & 1;
	j->ScanBit=7;
	return j->Byte_in(j) >> 7;		// arith/huff
}

static int Getnbit(struct JPEGD *j, int n)		// n>0
{
	int v= GetBit(j);
	while (--n) v= 2*v + GetBit(j);
	return v;
}

static int ReadDiff(struct JPEGD *j, int s)	// JPEG magnitude stuff. One way to do this..
{
	int x= Getnbit(j, s);
	if ( 0 == (x >> (s-1)) ) x= (-1<<s) - ~x;	// 0xxxxxx means neg 		//x= ~x & ((1<<s)-1);
	return x << j->Al;							// point transform included PRED??? seems ok. 
}

static int ReadHuffmanCode(struct JPEGD *j, int *pb)	// index into the sym-table
{
	int v= GetBit(j);
	while ( v >= *pb ) v= 2*v + GetBit(j) - *pb++;
	return v;
}	

static void dc_succ_huff(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	*coef |= GetBit(j) << j->Al;
}

static void dc_decode_huff(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	int s= sc->DCS[ReadHuffmanCode(j, sc->DCB)];
	if (s) sc->DC+= ReadDiff(j, s);
	*coef= sc->DC;
}

static void ac_decode_huff(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	int k= j->Ss;
	if (0==sc->EOBRUN) {
		for (; ;k++) {
			int s= sc->ACS[ReadHuffmanCode(j, sc->ACB)];
			int r= s>>4;
			if ( s&=15 ) s= ReadDiff(j, s);
			else {
				if (r < 15) {	// EOBn? 
					if (r) sc->EOBRUN= Getnbit(j, r) + ~(-1<<r);	// ((1<<r)-1);	-1 included
					return;				
				}//else ZRL
			}
			k+=r; 
			coef[k]= s;
			if (k==j->Se) return;
		}
	}
	else sc->EOBRUN--; 
}

static int ac_refine(struct JPEGD *j, TCOEF *coef)
{
	if (*coef) if (GetBit(j)) *coef+= (*coef > 0)? j->Al2 : -j->Al2;
	return *coef;
}

static void ac_succ_huff(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	int k= j->Ss;
	if (0==sc->EOBRUN) {
		for (; ;k++) {
			int s= sc->ACS[ReadHuffmanCode(j, sc->ACB)];
			int r= s>>4;
			if ( s&=15 ) s= GetBit(j)? j->Al2 : -j->Al2;  
			else {
				if (r < 15) {	// EOBn? 
					if (r) sc->EOBRUN= Getnbit(j, r) + ~(-1<<r); //=((1<<r)-1), -1 included
					break;				
				}//else ZRL
			}
			for (; ;k++) if (!ac_refine(j, coef+k)) if (!r--) break;
			coef[k]= s;
			if (k==j->Se) return;
		}
	}
	else sc->EOBRUN--; 
	for (; k<=j->Se; k++) ac_refine(j, coef+k); // Refine EOBRUN
}

static void du_sequential_huff(struct JPEGD *j, struct COMP *sc, TCOEF *coef) 
{
	int s, k;
	dc_decode_huff(j, sc, coef);
	for (k=1; (s=sc->ACS[ReadHuffmanCode(j, sc->ACB)]); k++) { // EOB?
		k+= s>>4;
		if (s==0xf0) continue; // ZRL
		coef[k]= ReadDiff(j, s&15);
		if (k==63) return;
	}
}

static void decode_lossless_huff(struct JPEGD *j, struct COMP *sc, int x, int y, TSAMP *samp)	//	TODO: Pt
{
	int DIFF= sc->DCS[ReadHuffmanCode(j, sc->DCB)];
	*samp= Predx(j, sc, x, y, samp);
	if (DIFF) *samp+= (DIFF==16)? 32768 : ReadDiff(j, DIFF);
}


static void Reset_decoder_huff(struct JPEGD *j)
{
	int c;
	for (c=0; c < j->Ns; c++) j->ScanComponents[c]->DC=0;	// DC/EOBRUN
	j->ScanBit=0;	// trigger next byte read (skipping stuffing '1'-s)
}

static void Reset_decoder_huff_lossless(struct JPEGD *j)
{
	j->LineNo=0;
	j->ScanBit=0;	// trigger next byte read
}




/////////////////////////////////// ARITHMETIC //////////////////////////////////////////////

static int Byte_in_arith(struct JPEGD *j)
{
	j->ScanByte = GETC();
	if ( 0xFF == j->ScanByte )
	{
		if ( GETC() ) {		// Marker detection
			SEEK(-2);		// Arith: "zero byte fed to decoder"
			j->ScanBit=~0;	// Seems like 8 was not enough.. TODO
			j->ScanByte=0;
		}
		//else skip zero-stuffing
	}
	return j->ScanByte;
}

static void ResetStat(struct CABACSTATE *st, int n)		// Initialize statistics areas
{
	while (--n >= 0) st[n].valMPS= st[n].StateIdx=0; 
}

static void InitDecoder(struct JPEGD *j)
{
	j->ScanBit=0;	// trigger next byte read
	j->A=0;
	j->C= Byte_in_arith(j) << 8;
	j->C|= Byte_in_arith(j);
}

static void Reset_decoder_arith(struct JPEGD *j)
{
	int c;
	for (c=0; c < j->Ns; c++) 
	{
		struct COMP *sc= j->ScanComponents[c];
		if (j->Se) ResetStat(sc->ACST+1, 245);	// AC in Scan (+1 adjusted)
		if (!j->Ss)	// DC in Scan
		{	
			ResetStat(sc->DCST, 49);
			sc->DC= 0;
			sc->DIFF= 0;		// extra in Arith
		}
	}
	InitDecoder(j);
}

static void Reset_decoder_arith_lossless(struct JPEGD *j)
{
	int c;
	for (c=0; c < j->Ns; c++) ResetStat(j->ScanComponents[c]->LLST, 158);
	j->LineNo=0;	// Special in lossless	
	InitDecoder(j);
}


static struct _STATE {
	int Qe_Value, Next_Index_LPS, Next_Index_MPS, Switch_MPS;
} STATE[] = {{0x5A1D,1,1,1},{0x2586,14,2,0},{0x1114,16,3,0},{0x080B,18,4,0},{0x03D8,20,5,0},{0x01DA,23,6,0},{0x00E5,25,7,0},{0x006F,28,8,0},{0x0036,30,9,0},{0x001A,33,10,0},{0x000D,35,11,0},{0x0006,9,12,0},{0x0003,10,13,0},{0x0001,12,13,0},{0x5A7F,15,15,1},{0x3F25,36,16,0},{0x2CF2,38,17,0},{0x207C,39,18,0},{0x17B9,40,19,0},{0x1182,42,20,0},{0x0CEF,43,21,0},{0x09A1,45,22,0},{0x072F,46,23,0},{0x055C,48,24,0},{0x0406,49,25,0},{0x0303,51,26,0},{0x0240,52,27,0},{0x01B1,54,28,0},{0x0144,56,29,0},{0x00F5,57,30,0},{0x00B7,59,31,0},{0x008A,60,32,0},{0x0068,62,33,0},{0x004E,63,34,0},{0x003B,32,35,0},{0x002C,33,9,0},{0x5AE1,37,37,1},{0x484C,64,38,0},{0x3A0D,65,39,0},{0x2EF1,67,40,0},{0x261F,68,41,0},{0x1F33,69,42,0},{0x19A8,70,43,0},{0x1518,72,44,0},{0x1177,73,45,0},{0x0E74,74,46,0},{0x0BFB,75,47,0},{0x09F8,77,48,0},{0x0861,78,49,0},{0x0706,79,50,0},{0x05CD,48,51,0},{0x04DE,50,52,0},{0x040F,50,53,0},{0x0363,51,54,0},{0x02D4,52,55,0},{0x025C,53,56,0},{0x01F8,54,57,0},{0x01A4,55,58,0},{0x0160,56,59,0},{0x0125,57,60,0},{0x00F6,58,61,0},{0x00CB,59,62,0},{0x00AB,61,63,0},{0x008F,61,32,0},{0x5B12,65,65,1},{0x4D04,80,66,0},{0x412C,81,67,0},{0x37D8,82,68,0},{0x2FE8,83,69,0},{0x293C,84,70,0},{0x2379,86,71,0},{0x1EDF,87,72,0},{0x1AA9,87,73,0},{0x174E,72,74,0},{0x1424,72,75,0},{0x119C,74,76,0},{0x0F6B,74,77,0},{0x0D51,75,78,0},{0x0BB6,77,79,0},{0x0A40,77,48,0},{0x5832,80,81,1},{0x4D1C,88,82,0},{0x438E,89,83,0},{0x3BDD,90,84,0},{0x34EE,91,85,0},{0x2EAE,92,86,0},{0x299A,93,87,0},{0x2516,86,71,0},{0x5570,88,89,1},{0x4CA9,95,90,0},{0x44D9,96,91,0},{0x3E22,97,92,0},{0x3824,99,93,0},{0x32B4,99,94,0},{0x2E17,93,86,0},{0x56A8,95,96,1},{0x4F46,101,97,0},{0x47E5,102,98,0},{0x41CF,103,99,0},{0x3C3D,104,100,0},{0x375E,99,93,0},{0x5231,105,102,0},{0x4C0F,106,103,0},{0x4639,107,104,0},{0x415E,103,99,0},{0x5627,105,106,1},{0x50E7,108,107,0},{0x4B85,109,103,0},{0x5597,110,109,0},{0x504F,111,107,0},{0x5A10,110,111,1},{0x5522,112,109,0},{0x59EB,112,111,1}};

static void Renorm(struct JPEGD *j)
{
	do j->C= 2*j->C | GetBit(j); 
	while ( (short)(j->A*=2) >= 0);
}

static int tr_MPS(struct JPEGD *j, struct CABACSTATE *st)
{
	Renorm(j);
	st->StateIdx= STATE[ st->StateIdx ].Next_Index_MPS;
	return st->valMPS;
}

static int tr_LPS(struct JPEGD *j, struct CABACSTATE *st)
{
	int D= st->valMPS^1;
	st->valMPS ^= STATE[ st->StateIdx ].Switch_MPS;
	st->StateIdx= STATE[ st->StateIdx ].Next_Index_LPS;
	Renorm(j);
	return D;
}

static int DecodeBin(struct JPEGD *j, struct CABACSTATE *st)
{
	unsigned short A= j->A - STATE[ st->StateIdx ].Qe_Value;
	if ( j->C < A ) 
	{
		j->A = A;
		if ((short)A<0) return st->valMPS;
		return ( A < STATE[ st->StateIdx ].Qe_Value )? tr_LPS(j, st) : tr_MPS(j, st);
	}
	j->C -= A;
	j->A= STATE[ st->StateIdx ].Qe_Value;
	return ( A < j->A )? tr_MPS(j, st) : tr_LPS(j, st);
}

static int DecodeFIX(struct JPEGD *j)
{
	unsigned short A= j->A - 0x5A1D;
	if ( j->C < A ) {
		if ((short)(j->A = A)<0) return 0;
		Renorm(j);
		return ( A < 0x5A1D ) ? 1 : 0;
	}
	else {
		j->C = 2*( j->C - A ) | GetBit(j);
		j->A= 0x5A1D*2;
		return ( A < 0x5A1D ) ? 0 : 1;
	}
}

static int DC_Context(struct COMP *sc, int D)	// DC + LOSSLESS
{
	if (D < 0) {
		if ( D < -sc->U ) return 16;	// large neg
		if ( D < -sc->L ) return 8;	
	}
	else {
		if ( D > sc->U ) return 12;		// large pos
		if ( D > sc->L ) return 4;	
	}
	return 0;
}

static int Decode_V(struct JPEGD *j, struct CABACSTATE *ST, int SNSP, int X1, int X2)	// DC/AC
{
	int Sz, M;
	if ( !DecodeBin(j, ST+SNSP) ) return 1;
	if ( !DecodeBin(j, ST+X1) ) return 2;
	M=2, ST+=X2;
	while ( DecodeBin(j, ST) ) M <<= 1, ST++;
	Sz= M;
	ST += 14;
	while (M>>=1) if (DecodeBin(j, ST)) Sz |= M;
	return Sz+1;
}

static int Decode_DIFF(struct JPEGD *j, struct CABACSTATE *ST, int S0, int X1)	// DC + LOSSLESS
{
	if ( DecodeBin(j, ST+S0) )	// (S0)
	{
		int sign= DecodeBin(j, ST + S0 + 1);	// (SS)
		int DIFF= Decode_V(j, ST, S0 + 2 + sign, X1, X1+1);
		return (sign)? -DIFF : DIFF;
	}
	return 0;
}

static void dc_decode_arith(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	int S0= DC_Context(sc, sc->DIFF);
	sc->DIFF= Decode_DIFF(j, sc->DCST, S0, 20);
	sc->DC+= sc->DIFF << j->Al;
	*coef= sc->DC;
}

static void ac_band(struct JPEGD *j, struct COMP *sc, TCOEF *coef, int k)		// NB: we re-arrange contexts a little (so indexing simply by k)
{
	while ( !DecodeBin(j, sc->ACST+k) )			//	EOB?
	{
		int V, sign;
		while ( !DecodeBin(j, sc->ACST+k+63) ) k++;	// S0
		sign= DecodeFIX(j);
		V= Decode_V(j, sc->ACST, k+126, k+126, (k>sc->Kx)? (217+1) : (189+1) );
		if (sign) V = -V;
		coef[k]= V << j->Al;
		if (k==j->Se) return;
		k++;
	}
}

static void du_sequential_arith(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	dc_decode_arith(j, sc, coef);
	ac_band(j, sc, coef, 1);
}

static void ac_decode_arith(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	ac_band(j, sc, coef, j->Ss);
}

static void dc_succ_arith(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	*coef |= DecodeFIX(j) << j->Al;
}

static void ac_succ_arith(struct JPEGD *j, struct COMP *sc, TCOEF *coef)
{
	int k= j->Ss;
	int EOBx= j->Se;		// actually index of last non-zero coeff already decoded in band

	while ( !coef[EOBx] && EOBx >= k ) EOBx--;	

	for (; k <= EOBx; k++)
	{
		if ( coef[k]) 
		{	 
			if (DecodeBin(j, sc->ACST+k+126)) coef[k] += (coef[k] > 0)? j->Al2 : -j->Al2;	// SC: correction bit?
		}
		else 
		{	
			if (DecodeBin(j, sc->ACST+k+63)) coef[k]= DecodeFIX(j)? -j->Al2 : j->Al2;	// New coeff?
		}
	}

	for (; k <= j->Se && !DecodeBin(j, sc->ACST+k); k++)	// SE: EOB?
	{
		while (!DecodeBin(j, sc->ACST+k+63)) k++;
		coef[k]= DecodeFIX(j)? -j->Al2 : j->Al2;
	}
}

static void decode_lossless_arith(struct JPEGD *j, struct COMP *sc, int x, int y, TSAMP *samp)		// TODO: Pt
{
		int Da= x? 5 * DC_Context(sc, sc->diffLeft[y]) : 0;
		int Db= y? DC_Context(sc, sc->diffAbove[x]) : 0;
		int DIFF= Decode_DIFF(j, sc->LLST, Da + Db, (Db>8)? 129 : 100);
		sc->diffAbove[x]= DIFF;
		sc->diffLeft[y]= DIFF;
		*samp= Predx(j, sc, x, y, samp) + DIFF;
}


///////////////////////////  SCAN DECODING ///////////////////////////////////////////////////////////////////////////////

//static int LocateMarker(struct JPEGD *j)

static int NextMarker(struct JPEGD *j)	// at expected location - this is not LocateMarker()
{
	int Marker;
	printf("%08X: ", TELL());
	while ( 0xFF == (Marker=GETC()) ) printf("FF");	// marker stuffing FF
	printf("%02X ", Marker);
	return Marker;	
}

static void Ri(struct JPEGD *j, int n)
{
	if (j->Ri) 	// Restart enabled?
	{
		if ( 0 == n % j->Ri ) 
		{	
			int Marker= NextMarker(j);
			if ( (Marker & 0xf8) == 0xD0 )	// RSTn expected
			{
				printf("RST%d\n", Marker&7);
				printf("%08X: ECS\n", TELL());
			}
			else { printf("STREAM ERROR: expected RSTn missing from ECS\n"); }
			j->Reset_decoder(j);
		}
	}			
}

static void DecodeInterleaved_DCT(struct JPEGD *j)
{
	int n=0, c, x, y;

	for(;;)
	{
		int mcuy = n / j->mcu_width;
		int mcux = n % j->mcu_width;

		for (c=0; c < j->Ns; c++) 
		{
			struct COMP *C= j->ScanComponents[c];
			DU *du= C->du +  mcuy * C->du_width * C->Vi + mcux * C->Hi;
			for (y=0; y<C->Vi; y++) for (x=0; x<C->Hi; x++) j->DecodeDataUnit(j, C, du[ C->du_width * y + x ]);	// Huff/arith
		}

		if (++n==j->mcu_total) return;	// We count MCU-s. No RST after the last 
		Ri(j, n);
	}
}

static void DecodeSingle_DCT(struct JPEGD *j)
{
	int n=0;
	struct COMP *C= j->ScanComponents[0];

	for (;;) 
	{
		j->DecodeDataUnit(j, C, C->du[ (n / C->du_w) * C->du_width +  n % C->du_w ]);	// Huff/arith
		if ( ++n == C->du_size ) return;	// We count DU-s. No RST after the last 
		Ri(j, n);
	}
}


static void DecodeSingle_LL(struct JPEGD *j)
{
	int n=0, x;
	struct COMP *C= j->ScanComponents[0];
	TSAMP *samp= C->samp;
	j->LineNo=0;
	for (;;) 
	{
		for (x=0; x < C->du_w; x++) j->decode_lossless(j, C, x, j->LineNo, samp+x);		// Huff/arith
		if ( (n+=C->du_w) == C->du_size ) return;	// we count by lines
		j->LineNo=1;
		Ri(j, n);				// Lossless restart interval is one mcu-line
		samp+=C->du_width;
	}
}


static void DecodeInterleaved_LL(struct JPEGD *j)
{
	int n=0, c;
	int x, mcux;
	int y, mcuy;
	j->LineNo=0;
	for (mcuy=0; ; mcuy++) 
	{
		for (mcux=0; mcux < j->mcu_width; mcux++) 
		{
			for (c=0; c < j->Ns; c++) 
			{
				struct COMP *C= j->ScanComponents[c];
				int xs= mcux * C->Hi;
				TSAMP *samp= C->samp + C->du_width * (mcuy * C->Vi) + xs;

				for (y=0; y < C->Vi; y++, samp+=C->du_width)
				{
					for (x=0; x < C->Hi; x++) j->decode_lossless(j, C, xs+x, j->LineNo+y, samp + x);	// Huff/arith
				}
			}
		}
		if ( (n+=j->mcu_width) == j->mcu_total ) return;	// we count by lines
		j->LineNo=1;
		Ri(j, n);		// Lossless restart interval is one mcu-line
	}
}



////////////////////////////// PARSING //////////////////////////////////////////////////////////////////////////////

static char *SOFSTRING[] = {	// debug
	// non-differential, Huffman coding 
	"Baseline DCT", 
	"Huffman Extended sequential DCT",
	"Huffman Progressive DCT",
	"Huffman Lossless sequential",
	// differential, Huffman coding (Hierarchial)
	"",			// DHT
	"-Differential sequential DCT",
	"-Differential progressive DCT",
	"-Differential lossless (sequential)",
	// non-differential, arithmetic coding
	"",			// JPG 
	"Arithmetic Extended sequential DCT", 
	"Arithmetic Progressive DCT", 
	"Arithmetic Lossless sequential", 
	// differential, arithmetic coding (Hierarchial)
	"",			// DAC
	"-Arithmetic Differential sequential DCT",
	"-Arithmetic Differential progressive DCT",
	"-Arithmetic Differential lossless sequential",

};

static int div_up(int a, int b)		//  ~ciel([a/b])
{
	return (a + b - 1) / b;
}

static int set_dim(struct JPEGD *j, int d)		// d= 1 (LL) or 8 (DCT)
{
	int i, TotalDU=0;
	
	j->Hmax= j->Vmax= 0;

		for (i=0; i<j->Nf; i++)		// read component data, set Hmax/Vmax
		{
			struct COMP *C= j->Components + i;

			C->Ci= GETC();	//Cid
			C->Vi= GETC();	//HV
			C->Hi= C->Vi>>4;
			C->Vi&= 15;
			C->Qi= GETC();	

			if ( C->Hi > j->Hmax ) j->Hmax = C->Hi;
			if ( C->Vi > j->Vmax ) j->Vmax = C->Vi;

			printf("    Ci=%3d HV=%dx%d Qi=%d\n", C->Ci, C->Hi, C->Vi, C->Qi);				
		}
		printf("\n");	

		// Full Image MCU cover: 
		j->mcu_width= div_up(j->X, j->Hmax*d);
		j->mcu_height= div_up(j->Y, j->Vmax*d);
		j->mcu_total= j->mcu_width * j->mcu_height;

		// now set parameters based on Hmax/Vmax
		for (i=0; i<j->Nf; i++)			
		{
			struct COMP *C= j->Components + i;

			int xi= div_up(j->X*C->Hi, j->Hmax);	// as Standard: sample rectangle (from image X,Y and Sampling factors)
			int yi= div_up(j->Y*C->Vi, j->Vmax);	// used to compute single scan 'coverage'

			// Single scan DU-cover (LL: d=1)
			C->du_w= div_up(xi, d);
			C->du_h= div_up(yi, d);
			C->du_size= C->du_w * C->du_h;

			// Interleaved scan DU-cover
			C->du_width= j->mcu_width*C->Hi; 
			C->du_total= C->du_width * j->mcu_height*C->Vi;

			TotalDU+= C->du_total;

					//printf("  %d\n", i);				
					//printf("    Sample: x=%d+%d (1:%d) y=%d+%d (1:%d)\n", C->xi, C->dux*8-C->xi, j->Hmax/C->Hi, C->yi, C->duy*8-C->yi, j->Vmax/C->Vi);				
					//printf("    8x8 Data Unit: %d (X=%d Y=%d)\n", C->duN, C->dux, C->duy); 
					//printf("    MCU Data Unit: %d (X=%d Y=%d)\n", C->du_total, C->du_width, C->duyI); 
					// LL
					//printf("    Sample: x=%d (1:%d) y=%d (1:%d)\n", C->du_xi, j->Hmax/C->Hi, C->du_yi, j->Vmax/C->Vi);				
		}

	return TotalDU;
}


extern enum JPEGENUM JPEGDecode(struct JPEGD *j)
{
	int marker, i;

	j->jpeg_mem= 0;
	marker = NextMarker(j);
	if ( marker != 0xD8 ) return JPEGENUMERR_MISSING_SOI;
	printf("SOI\n");

	// The 'Decoder_setup' procedure
	j->Ri=0;
	// Arithmetic capable Decoder this is:
	// default DC bounds: L = 0 and U = 1, Kx=5
	j->U[0]= j->U[1]= j->U[2]= j->U[3]= 2;		// 1<<U
	j->L[0]= j->L[1]= j->L[2]= j->L[3]= 0;
	j->Kx[0]= j->Kx[1]= j->Kx[2]= j->Kx[3]= 5;

	// Set first to zero for scaled quantization 
	// Also for QT entry redefinition (not implemented, see remarks)
	j->QT[0][0]=j->QT[1][0]=j->QT[2][0]=j->QT[3][0]=0;

	for (;;)
	{
		marker = NextMarker(j);

		if ( marker == 0xCC )	// DAC
		{
			int La= GETWbi();
			printf("DAC\n");
			printf("  Arithmetic Conditioning\n  parameters:\n");
			for (La-=2; La; La-=2) 
			{
				int CB= GETC();
				int Tc= CB>>4;
				int Tb= CB&15;
				int Cs= GETC();
				if (Tc)		// AC
				{
					printf("  AC%d Kx=%d\n", Tb, Cs);
					j->Kx[Tb]= Cs;
				}
				else 
				{
					int L= Cs&15;
					int U= Cs>>4;
					printf("  DC%d L=%d U=%d\n", Tb, Cs&15, Cs>>4);
					j->U[Tb]= 1<<U;
					j->L[Tb]= L? 1 << (L-1) : 0;
				}
			}
		}
		else if ( marker == 0xC8 )	// T.851??
		{
			return JPEGENUMERR_MARKERC8;	
		}
		else if ( marker == 0xC4 )	// DHT
		{
			int N;
			int Lh= GETWbi();
			printf("DHT\n");
			for (Lh-=2; Lh; Lh -= 17 + N) 
			{
				int CH= GETC();
				int Tc= CH>>4;
				int Th= CH&15;
				int *B= j->HTB[Tc][Th];
				unsigned char *S= j->HTS[Tc][Th];
				printf("  %s%d\n", Tc?"AC":"DC", Th);
				printf("    N: ");
				for (i=N=0; i<16; i++) {
					int n= GETC();
					N+= n;
					printf("%d ", n);
					B[i]= N;	// running total				
				}
				printf("\n");
				printf("    S: %d symbol bytes\n", N);
				for (i=0; i<N; i++) S[i]= GETC();
			}
		}
		else if ( (marker & 0xf0) == 0xC0 ) // START OF FRAME SOFn C0..CF (C4, CC, C8 checked before)
		{
			GETWbi();//Lf
			j->P= GETC();//P
			j->Y= GETWbi();//Y: Number of lines
			j->X= GETWbi();//X: Number of samples per line
			j->Nf= GETC();//Nf

			printf("SOF%d (%s)\n", marker&15, SOFSTRING[marker&15]);
			printf("  P=%d Y=%d X=%d\n", j->P, j->Y, j->X);
			printf("  Nf=%d\n", j->Nf);

			if (*SOFSTRING[marker&15] == '-') return JPEGENUMERR_UNKNOWN_SOF;
			if (j->Nf>4) return JPEGENUMERR_COMP4;
			if (!j->Y) return JPEGENUMERR_ZEROY;		// I have no idea about this DNL stuff
			j->SOF= marker;

			if ( (j->SOF&3)==3 ) // LOSSLESS-mode
			{
				int TotalDU= set_dim(j, 1);		// for malloc: in samples as coeff;

				if (j->SOF > 0xC8) {	// arithmetic:

					j->Reset_decoder= Reset_decoder_arith_lossless;
					j->decode_lossless= decode_lossless_arith;
					j->Byte_in= Byte_in_arith;
					
					// Arithmetic: need a line to store DIFF, where???
					for (i=0; i<j->Nf; i++) 
					{
						struct COMP *C= j->Components + i;
						C->du_total += C->du_width;
						TotalDU+= C->du_width;
					}
				}
				else {	// Huffman
					j->Reset_decoder= Reset_decoder_huff_lossless;
					j->decode_lossless= decode_lossless_huff;
					j->Byte_in= Byte_in_huff;
				}

				// malloc sample storage 
				{
					TSAMP *samp;
					int mallocTotalCoef= sizeof(TSAMP) * TotalDU;
					j->jpeg_mem= calloc(mallocTotalCoef, 1);
					if ( 0 == j->jpeg_mem ) return JPEGENUMERR_MALLOC;
					samp= j->jpeg_mem;
					for (i=0; i<j->Nf; i++) 
					{
						struct COMP *C= j->Components + i;
						C->samp= samp;
						samp+= C->du_total;
						C->diffAbove= samp - C->du_width;	// 1 line DIFF-BUFFER for Arith
					}
				}
			}
			else // DCT-mode
			{
				int TotalDU= set_dim(j, 8);		// for malloc in DU;

				printf("  %d MCU (%d x %d)\n", j->mcu_total, j->mcu_width, j->mcu_height);				

				// malloc DU-s
				{
					DU *du;
					int mallocTotalCoef= sizeof(DU) * TotalDU;
					j->jpeg_mem= calloc(mallocTotalCoef, 1);
					if ( 0 == j->jpeg_mem ) return JPEGENUMERR_MALLOC;
					du= j->jpeg_mem;
					for (i=0; i<j->Nf; i++) 
					{
						struct COMP *C= j->Components + i;
						C->du= du;
						du+= C->du_total;
					}
				}

				printf("  Malloc for %d Data Units (%lu bytes)\n\n", TotalDU, sizeof(DU)*TotalDU);

				if (j->SOF > 0xC8) {	// DCT Arithmetic
					j->Reset_decoder= Reset_decoder_arith;
					j->Byte_in= Byte_in_arith;
				}
				else {	// DCT Huffman
					j->Reset_decoder= Reset_decoder_huff;
					j->Byte_in= Byte_in_huff;
				}
			}
		}
		/*else if ( (marker & 0xf8) == 0xD0 )		// RSTn D0..D7
		{
			printf("RST%d\n", marker&7);
			printf("%08X: ....\n", TELL());
		}*/
		else if ( marker == 0xD9 ) // EOI
		{
			printf("EOI\n");
			return JPEGENUM_OK;	
		}
		else if ( marker == 0xDA )	// SOS
		{
			int ci;
			GETWbi();	//Ls
			printf("SOS\n");
			j->Ns= GETC();//Ns
			printf("  Ns: %d (%s scan)\n", j->Ns, (j->Ns>1)?"Interleaved":"Single");

			for (ci=0; ci<j->Ns; ci++) 
			{
				struct COMP *sc;
				int Cs= GETC();		// Cs -> Cid (Scan component selector)
				int T= GETC();
				int Td= T>>4;
				int Ta= T&15;
				printf("    Cs=%d Td=%d Ta=%d\n", Cs, Td, Ta);

				{// safe search
					for ( i=0; i<4 && j->Components[i].Ci != Cs; i++ ) ;
					if ( 4 == i ) return JPEGENUMERR_COMPNOTFOUND;
					j->ScanComponents[ci]= sc= j->Components+i;
				}

				if (j->SOF > 0xC8) 	// arithmetic
				{
					sc->U= j->U[Td];
					sc->L= j->L[Td];
					sc->Kx= j->Kx[Ta];
					
					if ((j->SOF&3)==3) sc->LLST= j->ACST[Td];	// LOSSLESS: ACST re-used to save storage (lossles stat. area little less than AC, but more than DC)
					else {
						sc->ACST= j->ACST[Ta]-1;	// DCT. Modified for speed: use 'k' to index the 63 increments for S0, SN,SP...
						sc->DCST= j->DCST[Td];
					}
				}
				else {	// Huffman
					sc->ACB= j->HTB[1][Ta];
					sc->ACS= j->HTS[1][Ta];
					sc->DCB= j->HTB[0][Td];
					sc->DCS= j->HTS[0][Td];
				}
			}

			j->Ss= GETC();//Ss (DCT) or Px (LL)
			j->Se= GETC();//Se
			j->Al= GETC();//AhAl
			j->Ah= j->Al>>4;
			j->Al&= 15;
			j->Al2= 1<<j->Al;//pre-computed

			printf("  %s: %d\n", ((j->SOF&3)==3)?"Px":"Ss", j->Ss);
			printf("  Se: %d\n", j->Se);
			printf("  Ah: %d\n", j->Ah);
			printf("  %s: %d\n", ((j->SOF&3)==3)?"Pt":"Al", j->Al);

			printf("%08X: ECS\n", TELL());	// Entropy-Coded Segment 

			j->Reset_decoder(j);	// arithmetic/huffman/lossless

			if ((j->SOF&3)==3) // LOSSLESS
			{
        		if (j->Ns>1) 
				{
					DecodeInterleaved_LL(j);
				}
				else 
				{
					DecodeSingle_LL(j);
				}
			}
			else {	// DCT-type

				if (j->SOF > 0xC8) 	// arithmetic:
				{
					j->DecodeDataUnit= j->Ss? (j->Ah? ac_succ_arith : ac_decode_arith) : (j->Se? du_sequential_arith : (j->Ah? dc_succ_arith : dc_decode_arith));
				}
				else {	
					j->DecodeDataUnit= j->Ss? (j->Ah? ac_succ_huff : ac_decode_huff) : (j->Se? du_sequential_huff : (j->Ah? dc_succ_huff : dc_decode_huff));
				}
				
        		if (j->Ns>1) 
				{
					DecodeInterleaved_DCT(j);
				}
				else 
				{  
					DecodeSingle_DCT(j);
				}
			}
		}
		else if ( marker == 0xDB )	// DQT
		{
			// Just read in (this is a coeff-decoder)
			int Pq;
			int Lq= GETWbi();
			printf("DQT\n");

			for (Lq-=2; Lq; Lq -= 65 + 64*Pq) 
			{
				int (*get)(void);
				int T= GETC(); 
				int Tq= T&3;
				int *qt= j->QT[Tq];
				Pq= T>>4;
				printf("  Tq=%d Pq=%d (%d-bit)\n", Tq, Pq, Pq?16:8);
				get= Pq? GETWbi : GETC;
				if (*qt) return JPEGENUMERR_QTREDEF; // re-defined quant table? Can be, not implemented.
				for (i=0; i<64; i++) qt[i]= get();
			}
		}
		else if ( marker == 0xDC ) // DNL
		{
			printf("DNL\n");
			return JPEGENUMERR_MARKERDNL;	
		}
		else if ( marker == 0xDD ) // DRI
		{
			GETWbi();//Lr
			j->Ri= GETWbi();
			printf("DRI\n");
			printf("  Ri: %d\n", j->Ri);
		}
		else if ( (marker & 0xf0) == 0xE0 ) // APPn E0..EF
		{
			int La= GETWbi();
			SEEK(La-2);
			printf("APP%d\n", marker&15);
		}
		else if ( marker == 0xFE ) // COM
		{
			SEEK(GETWbi()-2);
			printf("COM\n");
		}
		else 
		{
			printf("???\n");
		}
	}
}

#pragma GCC diagnostic pop