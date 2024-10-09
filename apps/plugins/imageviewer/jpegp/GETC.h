/* 
  "GETC" Interface for byte input (i.e. started as a wrapper for getc())

	For image- and video decoders I needed a transparent mechanism for 
	opening and reading the input data: 

		- either from streams (eg. coded video)
		- or preloaded into memory (eg. small GIF/PNG images)

	RAINBOW is only dependent on the GETC Interface for byte input. 

	File stream input is used during development and testing (FILEGETC.C). 
	In the OS image Rainbow can be linked with MEMGETC.C for memory input. 
	No other changes necessary in RAINBOW. 

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


// For decoders

extern int GETC(void);

// Multibyte helpers
extern int GETWbi(void);	// read word (16-bit) big-endian
extern int GETWli(void);	// little-endian
extern int GETDbi(void);	// read double word (32-bit) big-endian
extern int GETDli(void);	// little-endian

// positioning
extern void SEEK(int);	// move relative to current
extern void POS(int);	// move absolute position (TIFF)
extern int TELL(void);		// read actual position 


// For RAINBOW clients to implement outside of Rainbow Library
extern void *OPEN(char*);
extern void CLOSE(void);
