////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2004 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

This package contains a tiny version of the WavPack 4.0 decoder that might
be used in a "resource limited" CPU environment or form the basis for a
hardware decoding implementation. It is packaged with a demo command-line
program that accepts a WavPack audio file on stdin and outputs a RIFF wav
file to stdout. The program is standard C, and a win32 executable is
included which was compiled under MS Visual C++ 6.0 using this command:

cl /O1 /DWIN32 wvfilter.c wputils.c unpack.c float.c metadata.c words.c bits.c

WavPack data is read with a stream reading callback. No direct seeking is
provided for, but it is possible to start decoding anywhere in a WavPack
stream. In this case, WavPack will be able to provide the sample-accurate
position when it synchs with the data and begins decoding.

For demonstration purposes this uses a single static copy of the
WavpackContext structure, so obviously it cannot be used for more than one
file at a time. Also, this decoder will not handle "correction" files, plays
only the first two channels of multi-channel files, and is limited in
resolution in some large integer or floating point files (but always
provides at least 24 bits of resolution). It also will not accept WavPack
files from before version 4.0.

To make this code viable on the greatest number of hardware platforms, the
following are true:

   speed is about 4x realtime on an AMD K6 300 MHz
      ("high" mode 16/44 stereo; normal mode is about twice that fast)

   no floating-point math required; just 32b * 32b = 32b int multiply

   large data areas are static and less than 4K total
   executable code and tables are less than 32K
   no malloc / free usage

To maintain compatibility on various platforms, the following conventions
are used:

   a "short" must be 16-bits
   a "long" must be 32-bits
   an "int" must be at least 16-bits, but may be larger
   a "char" must default to signed


Questions or comments should be directed to david@wavpack.com
