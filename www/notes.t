#define _PAGE_ Jukebox notes
#include "head.t"

<center><table class=rockbox width=70%><tr><td>
<h2>Important:</h2>
<p>This page was written in late 2001/early 2002 during the initial reverse engineering of the hardware. Much of the information has since turned out to be wrong.
<p>View this page as a historical anecdote more than hard facts.
<p align=right>/Björn
</td></tr></table>
</center>

<h2>Exception vectors</h2>

<p>The first 0x200 bytes of the image appears to be the exception vector table.
The vectors are explained on pages 54 and 70-71 in the SH-1 Hardware Manual, 

<p>Here's the vector table for v5.03a:

<table border=1><tr>
<th>Vector</th><th>Address</th><th>Description/interrupt source</th>
<tr><td>  0</td><td>09000200</td><td>Power-on reset PC</td></tr>
<tr><td>  1</td><td>0903f2bc</td><td>Power-on reset SP</td></tr>
<tr><td>  2</td><td>09000200</td><td>Manual reset PC</td></tr>
<tr><td>  3</td><td>0903f2bc</td><td>Manual reset SP</td></tr>
<tr><td> 11</td><td>09000cac</td><td>NMI</td></tr>
<tr><td> 64</td><td>0900c060</td><td>IRQ0</td></tr>
<tr><td> 70</td><td>09004934</td><td>IRQ6</td></tr>
<tr><td> 78</td><td>09004a38</td><td>DMAC3 DEI3</td></tr>
<tr><td> 80</td><td>0900dfd0</td><td>ITU0 IMIA0</td></tr>
<tr><td> 88</td><td>0900df60</td><td>ITU2 IMIA2</td></tr>
<tr><td> 90</td><td>0900df60</td><td>ITU2 OVI2</td></tr>
<tr><td>104</td><td>09004918</td><td>SCI1 ERI1</td></tr>
<tr><td>105</td><td>090049e0</td><td>SCI1 Rxl1</td></tr>
<tr><td>109</td><td>09010270</td><td>A/D ITI</td></tr>
</table>

<p>From the use of address 0x0903f2bc as stack pointer, we can deduce
that the DRAM is located at address 0x09000000.
This is backed by the HW manual p102, which says that DRAM can only be at put on CS1, which is either 0x01000000 (8-bit) or 0x09000000 (16-bit).

<p>The vector table also corresponds with the fact that there is code at address 0x200 of the image file. 0x200 is thus the starting point for all code.

<h2>Port pins</h2>
<p><table><tr valign="top"><td>

<p>Port A pin function configuration summary:
<table border=1>
<tr><th>Pin</th><th>Function</th><th>Input/output</th><th>Initial value</th><th>Used for</th></tr>
<tr><td>PA0</td><td>i/o</td><td>Input</td><td></td><td>DC adapter detect</td></tr>
<tr><td>PA1</td><td>/RAS</td><td>Output</td><td></td><td>DRAM</td></tr>
<tr><td>PA2</td><td>/CS6</td><td>Output</td><td></td><td>IDE</td></tr>
<tr><td>PA3</td><td>/WAIT</td></tr>
<tr><td>PA4</td><td>/WR</td><td>Output</td><td></td><td>DRAM+Flash</td></tr>
<tr><td>PA5</td><td>i/o</td><td>Input</td><td></td><td>Key: ON</td></tr>
<tr><td>PA6</td><td>/RD</td><td>Output</td><td></td><td>IDE</td></tr>
<tr><td>PA7</td><td>i/o</td><td>Output</td><td>0</td></tr>
<tr><td>PA8</td><td>i/o</td><td>Output</td><td>0</td></tr>
<tr><td>PA9</td><td>i/o</td><td>Output</td><td>1</td></tr>
<tr><td>PA10</td><td>i/o</td><td>Output</td></tr>
<tr><td>PA11</td><td>i/o</td><td>Input</td><td></td><td>Key: STOP</td></tr>
<tr><td>PA12</td><td>/IRQ0</td></tr>
<tr><td>PA13</td><td>i/o</td></tr>
<tr><td>PA14</td><td>i/o</td><td>Output</td><td></td><td>Backlight</td></tr>
<tr><td>PA15</td><td>i/o</td><td>Input</td><td></td><td>USB cable detect</td></tr>
</table>

</td><td>

<p>Port B pin function configuration summary:
<table border=1>
<tr><th>Pin</th><th>Function</th><th>Input/output</th><th>Initial value</th><th>Used for</th></tr>
<tr><td>PB0</td><td>i/o</td><td>Output</td><td></td><td>LCD</td></tr>
<tr><td>PB1</td><td>i/o</td><td>Output</td><td></td><td>LCD</td></tr>
<tr><td>PB2</td><td>i/o</td><td>Output</td><td></td><td>LCD</td></tr>
<tr><td>PB3</td><td>i/o</td><td>Output</td><td></td><td>LCD</td></tr>
<tr><td>PB4</td><td>i/o</td><td>Input</td></tr>
<tr><td>PB5</td><td>i/o</td><td>Output</td><td>1</td><td>MAS WSEN</td></tr>
<tr><td>PB6</td><td>i/o</td><td>Output</td><td>0</td></tr>
<tr><td>PB7</td><td>i/o</td><td>Output</td><td></td><td>I²C data</td></tr>
<tr><td>PB8</td><td>i/o</td></tr>
<tr><td>PB9</td><td>TxD0</td><td>Output</td><td></td><td>MPEG</td></tr>
<tr><td>PB10</td><td>RxD1</td><td>Input</td></td><td></td><td>Remote</td></tr>
<tr><td>PB11</td><td>TxD1</td><td>Output</td><td></td><td>Remote?</td></tr>
<tr><td>PB12</td><td>SCK0</td><td>Output</td><td></td><td>MPEG</td></tr>
<tr><td>PB13</td><td>i/o</td><td>Output</td><td></td><td>I²C clock</td></tr>
<tr><td>PB14</td><td>/IRQ6</td><td>Input</td><td></td><td>MAS demand</td></tr>
<tr><td>PB15</td><td>i/o</td><td>Input</td><td></td><td>MAS MP3 frame sync</td></tr>
</table>


</td></tr></table>

<p>Port C pin function configuration summary:
<table border=1>
<tr><th>Pin</th><th>Function</th><th>Input/output</th><th>Used for</th></tr>
<tr><td>PC0</td><td>i/o</td><td>Input</td><td>Key: - / PREV</td></tr>
<tr><td>PC1</td><td>i/o</td><td>Input</td><td>Key: MENU</td></tr>
<tr><td>PC2</td><td>i/o</td><td>Input</td><td>Key: + / NEXT</td></tr>
<tr><td>PC3</td><td>i/o</td><td>Input</td><td>Key: PLAY</td></tr>
<tr><td>PC4</td><td>i/o</td><td>Input</td></tr>
<tr><td>PC5</td><td>i/o</td><td>Input</td></tr>
<tr><td>PC6</td><td>i/o</td><td>Input</td></tr>
<tr><td>PC7</td><td>i/o</td><td>Input</td></tr>
</table>


<h2>Labels</h2>
<p>Note: Everything is about v5.03a.

<ul>
<li>0x0200: Start point
<li>0x383d: Text: "Archos Jukebox hard drive is not bootable! Please insert a bootable floppy and press any key to try again" :-)
<li>0xc390: Address of "Update" string shown early on LCD.
<li>0xc8c0: Start of setup code
<li>0xc8c8: DRAM setup
<li>0xc4a0: Serial port 1 setup
<li>0xc40a: Port configuration setup
<li>0xe3bc: Character set conversion table
<li>0xfcd0: ITU setup
<li>0xc52a: Memory area #6 setup
<li>0x114b0: Start of menu strings
</ul>


<h2>Setup</h2>

<p>The startup code at 0x200 (0x09000200) naturally begins with setting up the system.

<h3>Vector Base Register</h3>

<p>The first thing the code does is setting the VBR, Vector Base Register,
and thus move the exception vector table from the internal ROM at address 0
to the DRAM at address 0x09000000:

<pre>
0x00000200: mov.l  @(0x02C,pc),r1  ; 0x0000022C (0x09000000)
0x00000202: ldc	   r1,vbr
</pre>

<h3>Stack</h3>

<p>The next instruction loads r15 with the contents of 0x228, which is 0x0903f2bc. This is the stack pointer, which is used all over the code.

<pre>
0x00000204: mov.l  @(0x024,pc),r15 ; 0x00000228 (0x0903F2BC)
</pre>

<p>After that the code jumps to the hardware setup at 0xc8c0.
<pre>
0x00000206: mov.l  @(0x01C,pc),r0   ; 0x00000220 (0x0900C8C0)
0x00000208: jsr    @r0
</pre>

<h3>DRAM controller</h3>

<p>First up is DRAM setup, at 0xc8c8. It sets the memory controller registers:

<pre>
0x0000C8C8: mov.l  @(0x068,pc),r2  ; 0x0000C930 (0x05FFFFA8)
0x0000C8CA: mov.w  @(0x05A,pc),r1  ; 0x0000C924 (0x1E00)
0x0000C8CC: mov.l  @(0x068,pc),r7  ; 0x0000C934 (0x0F0001C0)
0x0000C8CE: mov.w  r1,@r2          ; 0x1e00 -> DCR
0x0000C8D0: mov.l  @(0x068,pc),r2  ; 0x0000C938 (0x05FFFFAC)
0x0000C8D2: mov.w  @(0x054,pc),r1  ; 0x0000C926 (0x5AB0)
0x0000C8D4: mov.w  r1,@r2          ; 0x5ab0 -> RCR
0x0000C8D6: mov.l  @(0x068,pc),r2  ; 0x0000C93C (0x05FFFFB2)
0x0000C8D8: mov.w  @(0x050,pc),r1  ; 0x0000C928 (0x9605)
0x0000C8DA: mov.w  r1,@r2          ; 0x9505 -> RTCOR
0x0000C8DC: mov.l  @(0x064,pc),r2  ; 0x0000C940 (0x05FFFFAE)
0x0000C8DE: mov.w  @(0x04C,pc),r1  ; 0x0000C92A (0xA518)
0x0000C8E0: mov.w  r1,@r2          ; 0xa518 -> RTCSR
</pre>

<h3>Serial port 0</h3>

<p>Code starting at 0x483c.

<p>As C code:

<table border><tr><td bgcolor="#a0d6e8">
<pre>
void setup_sci0(void)
{
    /* set PB12 to output */
    PBIOR |= 0x1000;
&nbsp;    
    /* Disable serial port */
    SCR0 = 0x00;
&nbsp;
    /* Syncronous, 8N1, no prescale */
    SMR0 = 0x80;
&nbsp;
    /* Set baudrate 1Mbit/s */
    BRR0 = 0x03;
&nbsp;
    /* use SCK as serial clock output */
    SCR0 = 0x01;
&nbsp;
    /* Clear FER and PER */
    SSR0 &= 0xe7;
&nbsp;
    /* Set interrupt D priority to 0 */
    IPRD &= 0x0ff0;
&nbsp;
    /* set IRQ6 and IRQ7 to edge detect */
    ICR |= 0x03;
&nbsp;
    /* set PB15 and PB14 to inputs */
    PBIOR &= 0x7fff;
    PBIOR &= 0xbfff;
&nbsp;
    /* set IRQ6 prio 8 and IRQ7 prio 0 */
    IPRB = ( IPRB & 0xff00 ) | 0x80;
&nbsp;
    /* Enable Tx (only!) */
    SCR0 = 0x20;
}
</pre>
</td></tr></table>


<h3>Serial port 1</h3>

<p>Code starting at 0x47a0.

<p>As C code:

<table border><tr><td bgcolor="#a0d6e8">
<pre>
&#35;define SYSCLOCK 12000000
&#35;define PRIORITY 8
&nbsp;
void setup_sci1(int baudrate)
{
    /* Disable serial port */
    SCR1 = 0;
&nbsp;    
    /* Set PB11 to Tx and PB10 to Rx */
    PBCR1 = (PBCR1 & 0xff0f) | 0xa0;
&nbsp;
    /* Asynchronous, 8N1, no prescaler */
    SMR1 = 0;
&nbsp;
    /* Set baudrate */
    BRR1 = SYSCLOCK / (baudrate * 32) - 1;
&nbsp;
    /* Clear FER and PER */
    SSR1 &= 0xe7;
&nbsp;
    /* Set interrupt priority to 8 */
    IPRE = (IPRE & 0x0fff) | (PRIORITY << 12);
&nbsp;
    /* Enable Rx, Tx and Rx interrupt */
    SCR1 = 0x70;
}
</pre>
</td></tr></table>

<h3>Pin configuration</h3>

<p>Starting at 0xc40a:

<p><tt>CASCR = 0xafff</tt>: Column Address Strobe Pin Control Register. Set bits CASH MD1 and CASL MD1.

<h4>Port A</h4>
<br><tt>PACR1 = 0x0102</tt>: Set pin functions
<br><tt>PACR2 = 0xbb98</tt>: Set pin functions
<br><tt>PAIOR &= 0xfffe</tt>: PA0 is input
<br><tt>PAIOR &= 0xffdf</tt>: PA5 is input
<br><tt>PADR &= 0xff7f</tt>: Set pin PA7 low
<br><tt>PAIOR |= 0x80</tt>: PA7 is output
<br><tt>PAIOR |= 0x100</tt>: PA8 is output
<br><tt>PADR |= 0x200</tt>: Set pin PA9 high
<br><tt>PAIOR |= 0x200</tt>: PA9 is output
<br><tt>PAIOR |= 0x400</tt>: PA10 is output
<br><tt>PAIOR &= 0xf7ff</tt>: PA11 is input
<br><tt>PAIOR &= 0xbfff</tt>: PA14 is input
<br><tt>PAIOR = 0x7fff</tt>: PA15 is input
<br><tt>PADR &= 0xfeff</tt>: Set pin PA8 low

<h4>Port B</h4>
<br><tt>PBCR1 = 0x12a8</tt>: Set pin functions
<br><tt>PBCR2 = 0x0000</tt>: Set pin functions
<br><tt>PBDR &= 0xffef</tt>: Set pin PB4 low
<br><tt>PBIOR &= 0xffef</tt>: PB4 is input
<br><tt>PBIOR |= 0x20</tt>: PB5 is output
<br><tt>PBIOR |= 0x40</tt>: PA6 is output
<br><tt>PBDR &= 0xffbf</tt>: Set pin PB6 low
<br><tt>PBDR |= 0x20</tt>: Set pin PB5 high

<h3>ITU (Integrated Timer Pulse Unit)</h3>

<p>Starting at 0xfcd0:

<p><tt>TSNC &= 0xfe</tt>: The timer counter for channel 0 (TCNT0) operates independently of other channels
<br><tt>TMDR &= 0xfe</tt>: Channel 0 operates in normal (not PWM) mode
<br><tt>GRA0 = 0x1d4c</tt>:
<br><tt>TCR0 &= 0x67; TCR0 |= 0x23</tt>: TCNT is cleared by general register A (GRA) compare match or input capture. Counter clock = f/8
<br><tt>TIOR0 = 0x88</tt>: Compare disabled
<br><tt>TIER0 = 0xf9</tt>: Enable interrupt requests by IMFA (IMIA)
<br><tt>IPRC &= 0xff0f; IPRC |= 0x30</tt>: Set ITU0 interrupt priority level 3.
<br><tt>TSTR |= 0x01</tt>: Start TCNT0

<h3>Memory area #6 ?</h3>

<p>From 0xc52a:

<p><tt>PADR |= 0x0200</tt>: Set PA13 high
<br><tt>WCR1 = 0x40ff</tt>: Enable /WAIT support for memory area 6. Hmmm, what's on CS6?
<br><tt>WCR1 &= 0xfdfd</tt>: Turn off RW5 (was off already) and WW1 (enable short address output cycle).
<br><tt>WCR3 &= 0xe7ff</tt>: Turn off A6LW1 and A6LW0; 1 wait state for CS6.
<br><tt>ICR |= 0x80</tt>: Interrupt is requested on falling edge of IRQ0 input

<h2>Remote control</h2>
<p>Tjerk Schuringa reports:
"Finally got that extra bit going on my bitpattern generator. So far I fed only
simple characters to my jukebox, and this is the result:

<pre>
START D0 1 2 3 4 5 6 7 STOP FUNCTION
0      0 0 0 0 0 1 1 1    1 VOL- (the one I got already)
       0 0 0 0 1 0 1 1      VOL+ (figures)
       0 0 0 1 0 0 1 1      +
       0 0 1 0 0 0 1 1      -
       0 1 0 0 0 0 1 1      STOP
       1 0 0 0 0 0 1 1      PLAY
</pre>

<p>I also found that "repeat" functions (keep a button depressed) needs to be
faster than 0.5 s. If it is around 1 second or more it is interpreted as a
seperate keypress. So far I did not get the "fast forward" function because the
fastest I can get is 0.5 s.

<p>Very important: the baudrate is indeed 9600 baud! These pulses are fed to the
second ring on the headphone jack, and (if I understood correctly) go to RxD1
of the SH1."

<h2>LCD display</h2>

<p>The Recorder uses a Shing Yih Technology G112064-30 graphic LCD display with 112x64 pixels. The controller is a Solomon SSD1815Z.

<p>It's not yet known what display/controller the Jukebox has, but I'd be surprised if it doesn't use a similar controller.

<p>Starting at 0xE050, the code flicks PB2 and PB3 a great deal and then some with PB1 and PB0. Which gives us the following connections:

<table border><tr><th>CPU pin</th><th>LCD pin</th></tr>
<tr><td>PB0</td><td>DC</td></tr>
<tr><td>PB1</td><td>CS1</td></tr>
<tr><td>PB2</td><td>SCK</td></tr>
<tr><td>PB3</td><td>SDA</td></tr>
</table>

<p>The Recorder apparently has the connections this way (according to Gary Czvitkovicz):
<table border><tr><th>CPU pin</th><th>LCD pin</th></tr>
<tr><td>PB0</td><td>SDA</td></tr>
<tr><td>PB1</td><td>SCK</td></tr>
<tr><td>PB2</td><td>DC</td></tr>
<tr><td>PB3</td><td>CS1</td></tr>
</table>

<a name="charsets"><p>The player charsets:

<p><table border=0><tr>
<td><img src="codes_old.png" width=272 height=272><br>
<small>Old LCD charset (before v4.50)</small></td>
<td><img src="codes_new.png" width=272 height=272><br>
<small>New LCD charset (after v4.50)</small></td></tr></table>

<p>And the Recorder charset looks like this:
<br>
<img src="codes_rec.png">

<h3>Code</h3>

<p>This C snippet write a byte to the Jukebox LCD controller.
The 'data' flag inticates if the byte is a command byte or a data byte.

<table border><tr><td bgcolor="#a0d6e8">
<pre>
&#35;define DC  1
&#35;define CS1 2
&#35;define SDA 4
&#35;define SCK 8
&nbsp;
void lcd_write(int byte, int data)
{
   int i;
   char on,off;
&nbsp;
   PBDR &= ~CS1; /* enable lcd chip select */
&nbsp;
   if ( data ) {
      on=~(SDA|SCK);
      off=SCK|DC;
   }
   else {
      on=~(SDA|SCK|DC);
      off=SCK;
   }
   /* clock out each bit, MSB first */
   for (i=0x80;i;i>>=1)
   {
      PBDR &= on;
      if (i & byte)
         PBDR |= SDA;
      PBDR |= off;
   }
&nbsp;
   PBDR |= CS1; /* disable lcd chip select */
}
</pre>
</td></tr></table>

<h2>Firmware size</h2>

<p>Joachim Schiffer found out that firmware files have to be at least 51200
bytes to be loaded by newer firmware ROMs. 
So my "first program" only works on players with older firmware in ROM
(my has 3.18). Joachim posted a
<a href="mail/archive/rockbox-archive-2001-12/att-0087/01-AJBREC.ajz">padded version</a> that works everywhere.

<p>Tests have shown that firmware sizes above 200K won't load.

#include "foot.t"
