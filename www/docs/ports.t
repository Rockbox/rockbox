#define _PAGE_ Port pin assignments
#include "head.t"

<h1>CPU Port pin assignments</h1>

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
    <td><b>/IRQ3</b></td><td>MAS Demand IRQ</td>
    <td><b>/IRQ3</b></td><td>MAS Demand IRQ</td>
</tr>
</table>

#include "foot.t"
