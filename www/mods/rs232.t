#define _PAGE_ RS232 converter
#include "head.t"

<h2>What is this?</h2>
<p>
This is an RS232 converter for interfacing a PC with an Archos Jukebox (or any other device with a 3.3V serial port). It converts the PC serial port signals to 3.3V levels. The design is very straightforward, using a standard MAX3232 transceiver.

<h2>How to power it the easy way</h2>
<p>
It takes its power from the DTR signal in the serial port, so the communication software in the PC must set the DTR signal ACTIVE for this to work. It might still not work on some laptops, that may have a weak driver for the DTR signal.


<h2>How to power it the safe way</h2>
<p>
If your PC is a laptop or otherwise might risk having too little power in the serial port, it is also possible to take power from the USB port or another 5-10V source. Just remove the D1 diode and connect the power to the +5V and GND pads.

<h2>How to make one</h2>

<p><img src="rs232_board_bottom.png">
<br>Circuit board bottom layout
(<a href="rs232_board_bottom.pdf">PDF</a>)

<p><img src="rs232_board_top.png">
<br>Circuit board top layout
(<a href="rs232_board_top.pdf">PDF</a>)

<p><img src="rs232_board_place.png">
<br>Circuit board component placement
(<a href="rs232_board_place.pdf">PDF</a>)

<p><a href="rs232_schematic_big.png"><img src="rs232_schematic.png"></a>
<br>Schematic
(<a href="rs232_schematic_big.png">big PNG</a> 33kb)
(<a href="rs232_schematic.pdf">PDF</a>)

<h2>Bill of materials</h2>
<pre>
Part  Value      Device        Package  Description
C1    0.1uF      C-EUC0805     C0805    Capacitor
C2    0.1uF      C-EUC0805     C0805    Capacitor
C3    0.1uF      C-EUC0805     C0805    Capacitor
C4    0.1uF      C-EUC0805     C0805    Capacitor
C5    1uF        CPOL-EUCT3216 CT3216   Polarized Capacitor
D1    BAS32      DIODE-SOD80C  SOD80C   Diode
IC1   LP2980IM5  LP2980IM5     SOT23-5  3.3V Voltage regulator
IC2   MAX3232CWE MAX3232CWE    SO16     RS232 Transceiver
P1    DSUB9      DSUB9         DSUB9    9-pin board mounted D-SUB, 90 deg. angle
</pre>

<p>To make your life complete, here is Linus'
<a href="rs232_eagle.zip">complete schematics</a> made in Eagle 4.08r2.

<p>Contents:
<pre>
Archive:  Rs232.zip
  Length     Date   Time    Name
 --------    ----   ----    ----
    14175  03-26-02 11:13   eagle.epf
     7588  03-06-02 16:28   gerber.cam
    10271  03-06-02 15:32   linus.lbr
      848  03-26-02 13:50   README
      570  03-26-02 13:18   RS232 Converter.bom
    13443  03-26-02 10:57   RS232 Converter.brd
   187210  03-06-02 16:18   RS232 Converter.sch
 --------                   -------
   234105                   7 files
</pre>

#include "foot.t"
