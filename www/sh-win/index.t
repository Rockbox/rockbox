#define _PAGE_ Setting up an SH-1 compiler for Windows
#include "head.t"

<P>
	by <A href="mailto:edx@codeforce.d2g.com">Felix Arends</A>, 1/8/2002
	<BR>
</P>
<P>
	I have spent a long time figuring out how to compile SH1 code in windows (using 
	the sh-elf-gcc compiler) and when I finally compiled the first OS for my 
	Jukebox I decided to write a little tutorial explaining the setup process.
	<H2>
		The GNU-SH Tool Chain for Windows
	</H2>
<P></P>
This is actually all you need to download. It includes the binutils, gcc and 
newlib. Download the GNUSH Tool Chain for ELF format (35 MB).
<P>
	The GNUSH website can be found at <a href="http://www.kpit.com/download/downloadgnushv0203.htm">
		http://www.kpit.com/download/downloadgnushv0203.htm</a> (source code is 
	also available there). Get the "GNUSH v0203 Tool Chain for ELF format".
	<H2>
		Setting up&nbsp;the Compiler
	</H2>
<P></P>
<P>Install the GNUSH Tool Chain (nothing you really have to care about during the 
	installation process).</P>
<P>
	<H2>Compiling the latest Rockbox Source
	</H2>
<P>Use CVS to download the latest source code of Rockbox (the firmware and apps 
	modules). In addition, you need to copy a win32 compilation of scramble.exe 
	into the apps dir. The pre-compiled scramble.exe can be downloaded <a href="scramble.exe">here</a>.
	From your start menu, open the "SH-ELF tool chain" batch file inside the GNU-SH 
	v0203 program folder. You should end up seeing a command prompt. Go to the apps 
	directory and type:</P>
<P>
	<TABLE id="Table1" cellSpacing="1" cellPadding="1" width="100%" border="1">
		<TR>
			<TD>Command</TD>
			<TD>Description</TD>
		</TR>
		<TR>
			<TD>make -f win32.mak<BR>
				make -f win32.mak RECORDER = 1</TD>
			<TD vAlign="top">build for recorder target</TD>
		</TR>
		<TR>
			<TD>make -f win32.mak PLAYER</TD>
			<TD>build for player target</TD>
		</TR>
		<TR>
			<TD>make -f win32.mak PLAYER_OLD</TD>
			<TD>build for old player target</TD>
		</TR>
		<TR>
			<TD>make -f win32.mak RECORDER=1 DISABLE_GAMES=1</TD>
			<TD>build for recorder target, disable games</TD>
		</TR>
		<TR>
			<TD vAlign="top">make -f win32.mak RECORDER=1 PROPFONTS=1</TD>
			<TD>build for recorder target, enable propfonts</TD>
		</TR>
		<TR>
			<TD vAlign="top">make -f win32.mak RECORDER=1 PROPFONTS=1&nbsp;DISABLE_GAMES = 
				1&nbsp;</TD>
			<TD>build for recorder target, disable games, use propfonts</TD>
		</TR>
	</TABLE>
</P>
<P>
	I hope this tutorial helped you to compile an Archos firmware with windows. If 
	you have any questions, comments or corrections, please mail to <A href="mailto:edx@go.cc">
		edx@go.cc</A>
</P>
#include "foot.t"
