#define _PAGE_ Setting up an SH-1 compiler for Windows
#include "head.t"
<P>
by <A href="mailto:edx@codeforce.d2g.com">Felix Arends</A>, 1/8/2002
<BR>

<P>
NOTE: THIS COMPILER DOES NOT YET WORK WITH WINDOWS XP!!!

<P>
I have spent a long time figuring out how to compile SH1 code in windows (using 
the sh-elf-gcc compiler) and when I finally compiled the first OS for my 
Jukebox I decided to write a little tutorial explaining the setup process.

<H2>
The GNU-SH Tool Chain for Windows
</H2>
<P>
This is actually all you need to download. It includes the binutils, gcc and 
newlib. Download the GNUSH Tool Chain for ELF format (13 MB).

<P>
The GNUSH website can be found at <a href="http://www.kpit.com/download/downloadgnush.htm">
http://www.kpit.com/download/downloadgnush.htm</a> (source code is also 
available there). The new v0202 version now uses MinGW instead of Cygwin.

<H2>
Setting up&nbsp;the Compiler
</H2>
<P>
Install the GNUSH Tool Chain (nothing you really have to care about during the 
installation process). After that you should add some paths to your PATH system 
environment variable. If you have Windows 95/98/Me you can do that by modifying 
your autoexec.bat:

<P>
Add the following line to your autoexec.bat:

<P>
<TABLE cellSpacing="1" cellPadding="1" width="100%" border="1">
<TBODY>
<TR>
<TD bgcolor="#a0d6e8">
<code>SET PATH=%PATH%;C:\Programs\kpit\GNU-SH v0101 
[ELF]\Sh-elf\bin\;C:\Programs\kpit\GNU-SH v0101 
[ELF]\Sh-elf\lib\gcc-lib\sh-elf\2.9-GNU-SH-v0101\;C:\Programs\kpit\GNU-SH v0101 
[ELF]\Other Utilities</code>
</TD>
</TR>
</TBODY>
</TABLE>

<P>
</CODE>(Note: This is just one single line)

<P>
Replace the beginning of the paths with the path-name you chose to installt the 
tools in. Reboot.

<P>
In Windows 2000 it is a bit different. You can find the PATH-environment 
variable if you right-click the "My Computer" icon on your desktop and choose 
"Properties" in the popup-menu. Go to the "Advanced" tab and click 
"Environment-Variables":

<P align="center">
<IMG src="enviro.jpg"> <IMG src="enviro2.jpg"> <IMG src="enviro3.jpg">

<P>
(Note: There is also a PATH-variable in the "System variables" list, it does 
not matter which one you edit)

<P>
To the value the PATH-variable already has, add:

<P>
<TABLE cellSpacing="1" cellPadding="1" width="100%" border="1">
<TR>
<TD bgcolor="#a0d6e8">
<code>;C:\Programs\kpit\GNU-SH v0101 [ELF]\Sh-elf\bin\;C:\Programs\kpit\GNU-SH 
v0101 [ELF]\Sh-elf\lib\gcc-lib\sh-elf\2.9-GNU-SH-v0101\;C:\Programs\kpit\GNU-SH 
v0101 [ELF]\Other Utilities</code>
</TD>
</TR>
</TABLE>

<P>
Replace the program path with the path you chose for the program. You do not 
have to reboot.

<H2>
An "empty" System
</H2>
<P>
First of all, I'll explain what to do to compile an "empty" system. It just 
initializes and calls the <EM>main</EM> function, but does not do anything 
else. You can add some code to the <EM>main</EM> function and simply recompile. 
It is actually like this: You don't have to care about any of those files, 
because you won't have to change much of them (except the main.cpp of 
course!!).

<P>
<STRONG>main.cpp:
<BR>
</STRONG>
<TABLE cellSpacing="1" cellPadding="1" width="550" border="1">
<TR>
<TD bgcolor="#a0d6e8">
<P>
<code><font color="#0000ff">int</font> __main(<font color="#0000ff">void</font>){}
<BR>
<BR>
<font color="#0000ff">int</font> main(<font color="#0000ff">void</font>)
<BR>
{
<BR>
<font color="#009000">&nbsp;&nbsp;&nbsp; // add code here</font>
<BR>
}
<BR>
<BR>
<FONT color="#0000ff">extern</FONT> <FONT color="#0000ff">const</FONT> <FONT color="#0000ff">
void</FONT> stack(<FONT color="#0000ff">void</FONT>);
<br>
<br>
<FONT color="#0000ff">const</FONT> <FONT color="#0000ff">void</FONT>* vectors[] 
__attribute__ ((section (".vectors"))) =
<br>
{
<br>
&nbsp;&nbsp;&nbsp; main, <FONT color="#009000">/* Power-on reset */</FONT>
<br>
&nbsp;&nbsp;&nbsp; stack, <FONT color="#009000">/* Power-on reset (stack pointer) 
*/</FONT>
<br>
&nbsp;&nbsp;&nbsp; main, <FONT color="#009000">/* Manual reset */</FONT>
<br>
&nbsp;&nbsp;&nbsp; stack <FONT color="#009000">/* Manual reset (stack pointer) */</FONT>
<br>
};
<br>
</code>

</TD>
</TR>
</TABLE>

<P>
We need a start-up assembler code:

<P>
<STRONG>start.asm
<br>
</STRONG>
<TABLE cellSpacing="1" cellPadding="1" width="550" border="1">
<TR>
<TD bgcolor="#a0d6e8">
<code>
<pre>! note: sh-1 has a "delay cycle" after every branch where you can
! execute another instruction "for free".

.file"start.asm"
.section.text.start
.extern_main
.extern _vectors
.extern _stack
.global _start
.align  2

_start:
mov.l1f, r1
mov.l3f, r3
mov.l2f, r15
jmp@r3
ldcr1, vbr
nop

1:.long_vectors
2:.long_stack
3:.long_main
.type_start,@function</pre>
</code>
</TD>
</TR>
</TABLE>

<P>
(I took this code from Björn's LCDv2 source)

<P>
Then we need a linker configuration file:

<P>
<STRONG>linker.cfg</STRONG>
<BR>
<TABLE cellSpacing="1" cellPadding="1" width="550" border="1">
<TR>
<TD bgcolor="#a0d6e8">
<P>
<code>
<pre>ENTRY(_start)
OUTPUT_FORMAT(elf32-sh)
SECTIONS
{
    .vectors 0x09000000 :
    {
        *(.vectors);
        . = ALIGN(0x200);
        *(.text.start)
        *(.text)
        *(.rodata)
    }

    .bss :
    {
       _stack = . + 0x1000;
    }

    .pad 0x0900C800 :
    {
        LONG(0);
    }
  }</pre>
</code>
</TD>
</TR>
</TABLE>

<P>
(This code comes from Börn's LCDv2 as well)

<P>
Last but not least, we need a batch file to link all this and output a usable 
.mod file (you don't really need a batch file if you want to enter all the 
commands over and over angain :])

<P>
<STRONG>make.bat</STRONG>
<BR>
<TABLE cellSpacing="1" cellPadding="1" width="550" border="1">
<TR>
<TD bgcolor="#a0d6e8">
<P>
<code>
<pre>SET INCLUDES=
SET SOURCEFILES=main.c
SET OBJECTS=main.o start.o

sh-elf-as start.asm -o start.o -L -a
sh-elf-gcc -O2 -m1 -o main.o -c -nostdlib %INCLUDES% %SOURCEFILES%
sh-elf-ld -o main.out %OBJECTS% -Tlinker.cfg
padit main.out
scramble main.out archos.mod

PAUSE</pre>
</code>
</TD>
</TR>
</TABLE>

<P>
And that's it! I have prepared all those files in a .zip archive for you so you 
don't have to copy'n'paste that much :). I have also prepared a package with 
the LCDv2 code Björn wrote (ready to compile with Windows).

<P>
<a href="LCDv2Win.zip">empty.zip</a>
<BR>
<a href="LCDv2Win.zip">LCDv2Win.zip</a>

<P>
I hope this tutorial helped you to compile an Archos firmware with windows. If 
you have any questions, comments or corrections, please mail to <A href="mailto:edx@codeforce.d2g.com">
edx@codeforce.d2g.com</A>

#include "foot.t"

