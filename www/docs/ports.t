#define _PAGE_ Port pin assignments
#include "head.t"

<h2>Port A</h2>
<table border=1>
<tr><th>Port pin</th>
    <th colspan=2>Player</th>
    <th colspan=2>Recorder</th>
    <th colspan=2>FM/V2 Recorder</th>
</tr>
<tr><th><b>PA0</b></th>
    <td><b>GP In</b></td><td>DC adapter detect (0=inserted)</td>
    <td><b>/CS4</b></td><td>MAS Parallel Port (for recording)</td>
    <td><b>/CS4</b></td><td>MAS Parallel Port (for recording)</td>
</tr>
<tr><th><b>PA1</b></th>
    <td><b>/RAS Out</b></td><td>DRAM control</td>
    <td><b>/RAS Out</b></td><td>DRAM control</td>
    <td><b>/RAS Out</b></td><td>DRAM control</td>
</tr>
<tr><th><b>PA2</b></th>
    <td><b>/CS6 Out</b></td><td>ATA registers</td>
    <td><b>/CS6 Out</b></td><td>ATA registers</td>
    <td><b>/CS6 Out</b></td><td>ATA registers</td>
</tr>
<tr><th><b>PA3</b></th>
    <td><b>/WAIT In</b></td><td>Bus handshake</td>
    <td><b>/WAIT In</b></td><td>Bus handshake</td>
    <td><b>/WAIT In</b></td><td>Bus handshake</td>
</tr>
<tr><th><b>PA4</b></th>
    <td><b>/WR Out</b></td><td>Bus write signal</td>
    <td><b>/WR Out</b></td><td>Bus write signal</td>
    <td><b>/WR Out</b></td><td>Bus write signal</td>
</tr>
<tr><th><b>PA5</b></th>
    <td><b>GP In</b></td><td>ON key (0=pressed)</td>
    <td><b>GP Out</b></td><td>ATA power control (1=on)</td>
    <td><b>GP Out</b></td><td>ATA/LED power control (1=on)</td>
</tr>
<tr><th><b>PA6</b></th>
    <td><b>/RD Out</b></td><td>Bus read signal</td>
    <td><b>/RD Out</b></td><td>Bus read signal</td>
    <td><b>/RD Out</b></td><td>Bus read signal</td>
</tr>
<tr><th><b>PA7</b></th>
    <td><b>GP Out</b></td><td>ATA buffer control (0=active)</td>
    <td><b>GP Out</b></td><td>ATA buffer control (0=active)</td>
    <td><b>GP Out</b></td><td>ATA buffer control (0=active)</td>
</tr>
<tr><th><b>PA8</b></th>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
    <td><b>GP Out</b></td><td>MAS POR Reset (polarity varies)</td>
    <td><b>GP Out</b></td><td>MAS POR Reset (polarity varies)</td>
</tr>
<tr><th><b>PA9</b></th>
    <td><b>GP Out</b></td><td>ATA Reset (0=reset)</td>
    <td><b>GP Out</b></td><td>ATA Reset (0=reset)</td>
    <td><b>GP Out</b></td><td>ATA Reset (0=reset)</td>
</tr>
<tr><th><b>PA10</b></th>
    <td><b>GP Out</b></td><td>USB Enable (0=enable)</td>
    <td><b>GP Out</b></td><td>USB Enable (polarity varies)</td>
    <td><b>GP Out</b></td><td>USB Enable (polarity varies)</td>
</tr>
<tr><th><b>PA11</b></th>
    <td><b>GP In</b></td><td>STOP key (0=pressed)</td>
    <td><b>GP Out</b></td><td>MAS PR DMA Request (polarity varies)</td>
    <td><b>GP Out</b></td><td>MAS PR DMA Request (polarity varies)</td>
</tr>
<tr><th><b>PA12</b></th>
    <td><b>/IRQ0</b></td><td>ATA INTRQ (not used)</td>
    <td><b>/IRQ0</b></td><td>ATA INTRQ (not used)</td>
    <td><b>/IRQ0</b></td><td>ATA INTRQ (not used)</td>
</tr>
<tr><th><b>PA13</b></th>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
    <td><b>/IRQ1</b></td><td>RTC IRQ</td>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
</tr>
<tr><th><b>PA14</b></th>
    <td><b>GP Out</b></td><td>Backlight (1=on)</td>
    <td><b>GP In</b></td><td>Not used</td>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
</tr>
<tr><th><b>PA15</b></th>
    <td><b>GP In</b></td><td>USB cable detect (0=inserted)</td>
    <td><b>/IRQ3</b></td><td>MAS Demand IRQ, start demand</td>
    <td><b>/IRQ3</b></td><td>MAS Demand IRQ, start demand</td>
</tr>
</table>

<h2>Port B</h2>
<table border=1>
<tr><th>Port pin</th>
    <th colspan=2>Player</th>
    <th colspan=2>Recorder</th>
    <th colspan=2>FM/V2 Recorder</th>
</tr>
<tr><th><b>PB0</b></th>
    <td><b>GP Out</b></td><td>LCD Data Select (1=data)</td>
    <td><b>GP Out</b></td><td>LCD Serial Data</td>
    <td><b>GP Out</b></td><td>LCD Serial Data / FM Radio Data In</td>
</tr>
<tr><th><b>PB1</b></th>
    <td><b>GP Out</b></td><td>LCD Chip Select (0=active)</td>
    <td><b>GP Out</b></td><td>LCD Serial Clock</td>
    <td><b>GP Out</b></td><td>LCD Serial Clock / FM Radio Serial Clock</td>
</tr>
<tr><th><b>PB2</b></th>
    <td><b>GP Out</b></td><td>LCD Serial Data</td>
    <td><b>GP Out</b></td><td>LCD Data Select (1=data)</td>
    <td><b>GP Out</b></td><td>LCD Data Select (1=data)</td>
</tr>
<tr><th><b>PB3</b></th>
    <td><b>GP Out</b></td><td>LCD Serial Clock</td>
    <td><b>GP Out</b></td><td>LCD Chip Select (0=active)</td>
    <td><b>GP Out</b></td><td>LCD Chip Select (0=active) / FM Radio Chip Enable (1=active)</td>
</tr>
<tr><th><b>PB4</b></th>
    <td><b>GP Out</b></td><td>Hard disk power (1=on) <b>NewPlayer only</b></td>
    <td><b>GP In</b></td><td>OFF key (0=pressed)</td>
    <td><b>GP In</b></td><td>FM Radio Data Out</td>
</tr>
<tr><th><b>PB5</b></th>
    <td><b>GP Out</b></td><td>MAS WSEN (1=enable)</td>
    <td><b>GP Out</b></td><td>Charger control (0=enable)</td>
    <td><b>GP Out</b></td><td>Main power control (0=shut off)</td>
</tr>
<tr><th><b>PB6</b></th>
    <td><b>GP Out</b></td><td>Red LED control (1=on)</td>
    <td><b>GP Out</b></td><td>Red LED control (1=on)</td>
    <td><b>GP Out</b></td><td>Red LED control (1=on)</td>
</tr>
<tr><th><b>PB7</b></th>
    <td><b>GP I/O</b></td><td>I²C Data</td>
    <td><b>GP Out</b></td><td>I²C Data</td>
    <td><b>GP Out</b></td><td>I²C Data</td>
</tr>
<tr><th><b>PB8</b></th>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
    <td><b>GP In</b></td><td>ON key (0=pressed)</td>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
</tr>
<tr><th><b>PB9</b></th>
    <td><b>TxD0</b></td><td>MAS Serial link for MP3 data</td>
    <td><b>TxD0</b></td><td>MAS Serial link for MP3 data</td>
    <td><b>TxD0</b></td><td>MAS Serial link for MP3 data</td>
</tr>
<tr><th><b>PB10</b></th>
    <td><b>RxD1</b></td><td>Remote control serial input</td>
    <td><b>RxD1</b></td><td>Remote control serial input</td>
    <td><b>Unused</b></td><td>(meant for RDS data input, IIRC)</td>
</tr>
<tr><th><b>PB11</b></th>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
    <td><b>&nbsp;</b></td><td>&nbsp;</td>
</tr>
<tr><th><b>PB12</b></th>
    <td><b>SCK0</b></td><td>MAS Serial Clock for MP3 data</td>
    <td><b>SCK0</b></td><td>MAS Serial Clock for MP3 data</td>
    <td><b>SCK0</b></td><td>MAS Serial Clock for MP3 data</td>
</tr>
<tr><th><b>PB13</b></th>
    <td><b>GP Out</b></td><td>I²C Clock</td>
    <td><b>GP Out</b></td><td>I²C Clock</td>
    <td><b>GP Out</b></td><td>I²C Clock</td>
</tr>
<tr><th><b>PB14</b></th>
    <td><b>/IRQ6</b></td><td>MAS Demand IRQ, stop demand</td>
    <td><b>/IRQ6</b></td><td>MAS Demand IRQ, stop demand</td>
    <td><b>/IRQ6</b></td><td>MAS Demand IRQ, stop demand</td>
</tr>
<tr><th><b>PB15</b></th>
    <td><b>GP In</b></td><td>MAS MP3 frame sync</td>
    <td><b>GP In</b></td><td>MAS PRTW input (0=ready)</td>
    <td><b>GP In</b></td><td>MAS PRTW input (0=ready)</td>
</tr>
</table>

<h2>Port C/Analog In</h2>
<table border=1>
<tr><th>Port pin</th>
    <th>Player</th>
    <th>Recorder</th>
    <th>FM/V2 Recorder</th>
</tr>
<tr><th><b>PC0/AN0</b></th>
    <td>LEFT key</td>
    <td>Battery voltage 1 (unusable)</td>
    <td>&nbsp;</td>
</tr>
<tr><th><b>PC1/AN1</b></th>
    <td>MENU key</td>
    <td>Charger regulator voltage</td>
    <td>USB detect</td>
</tr>
<tr><th><b>PC2/AN2</b></th>
    <td>RIGHT key</td>
    <td>USB voltage</td>
    <td>OFF key</td>
</tr>
<tr><th><b>PC3/AN3</b></th>
    <td>PLAY key</td>
    <td>&nbsp;</td>
    <td>ON key</td>
</tr>
<tr><th><b>PC4/AN4</b></th>
    <td>&nbsp;</td>
    <td>F1, F2, F3, UP keys</td>
    <td>F1, F2, F3, UP keys</td>
</tr>
<tr><th><b>PC5/AN5</b></th>
    <td>&nbsp;</td>
    <td>DOWN, PLAY, LEFT, RIGHT keys</td>
    <td>DOWN, PLAY, LEFT, RIGHT keys</td>
</tr>
<tr><th><b>PC6/AN6</b></th>
    <td>Battery voltage</td>
    <td>Battery voltage</td>
    <td>Battery voltage</td>
</tr>
<tr><th><b>PC7/AN7</b></th>
    <td>DC input voltage</td>
    <td>DC input voltage</td>
    <td>Charge current?</td>
</tr>
</table>
#include "foot.t"
