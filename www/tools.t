#define _PAGE_ Tools
#include "head.t"

<h2>Descrambler / Scrambler</h2>

<p>The archos.mod file is scrambled, but luckily not using encryption.

<p>Each data byte is inverted and ROLed 1 bit.
The data is then spread over four memory segments. The two least significant bits of the address is used as segment number and the rest as offset in the segment. So, basically:

<ul>
<li>segment number = address % 4
<li>segment offset = address / 4
<li>segment length = imgsize / 4
</ul>

<p>A 6-byte header is added to the beginning of the scrambled image:
<ul>
<li>32 bit length (big-endian)
<li>16 bit checksum
</ul>

<p>I've written a small utility to descramble the firmware files:
<ul>
<li><a href="descramble.c">descramble.c</a> - 1835 bytes - The source code (pure ANSI C, should work everywhere). GPL licensed.
<li><a href="descramble">descramble</a> - 4280 bytes - Dynamically linked i386 linux executable
<li><a href="descramble.static.bz2">descramble.static.bz2</a> - 176015 bytes - bzip2 compressed statically linked i386 linux executable
<li><a href="descramble.exe">descramble.exe</a> - 45056 bytes - win32 executable
</ul>

<p>...and one to scramble files:
<ul>
<li><a href="scramble.c">scramble.c</a> - 2242 bytes - The source code (pure ANSI C, should work everywhere). GPL licensed.
<li><a href="scramble">scramble</a> - 4376 bytes - Dynamically linked i386 linux executable
<li><a href="scramble.static.bz2">scramble.static.bz2</a> - 176117 bytes - bzip2 compressed statically linked i386 linux executable
<li><a href="scramble.exe">scramble.exe</a> - 93385 bytes - win32 executable
</ul>

<h2>Disassembler</h2>

<p>I found a nice public domain SH-1/SH-2 disassembler written by Bart Trzynadlowski, called <a href="http://saturndev.emuvibes.com/Files/sh2d020.zip">sh2d</a>:
<p><b>Update:</b> I've added address lookup and register name translation to the disassembler (2001-12-09)
<ul>
<li><a href="sh2d.c">sh2d.c</a> - 28 kB - Source code
<li><a href="sh2d">sh2d</a> - 15 kB - Dynamically linked i386 linux executable
<li><a href="sh2d.static.bz2">sh2d.static.bz2</a> - 170 kB - bzip2 compressed statically linked i386 linux executable
<li><a href="sh2d.exe">sh2d.exe</a> - 40 kB - win32 executable (original version; no lookup)
</ul>

<h2>Compiler</h2>

<p>GCC supports the SH processor. Just 
<a href="cross-gcc.html">cook yourself a cross-compiler</a>
(sh-elf-gcc) and voila, instant SH-1 code.

<p>There are also 
<a href="http://www.sh-linux.org/rpm/RPMS/i386/RedHat7.1/">
pre-cooked RH7.1 RPMs</a> available from sh-linux.org

<p>Felix Arends has written a page about
<a href="sh-win/">setting up an SH-1 compiler for Windows</a>.

#include "foot.t"
