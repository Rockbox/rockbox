#define _PAGE_ Rockbox WPS
#include "head.t"
#include "manual.t"
<p>
The While Playing Screen shows info about what is currently played and can
only be seen while there is actual music playback.
<p>
<img src="rec-wps.png">
<p>
The screen lines contain the following information:
<ol>
<li>Status bar: Battery level, charger status, volume, play mode, repeat mode, shuffle mode and clock
<li>Scrolling path+filename of the current song.
<li>The ID3 track name
<li>The ID3 album name
<li>The ID3 artist namn
<li>Bit rate and sample frequency. VBR files display average bitrate.
<li>Elapsed and total time
<li>A slider bar representing where in the song you are
</ol>
<p>
<table>

<tr><th> Key </th><th> Function </th></tr>
<tr><td> UP</td><td> Increase volume </td></tr>
<tr><td> DOWN </td><td>  Decrease volume </td></tr>
<tr><td> LEFT </td><td>  Move to previous song</td></tr>
<tr><td> RIGHT </td><td> Move to next song </td></tr>
<tr><td> PLAY </td><td> Toggle PAUSE/PLAY </td></tr>
<tr><td> ON </td><td> Switch to the <a href="rec-dir.html">dir browser</a> </td></tr>
<tr><td>OFF</td><td> Stop playback </td></tr>
<tr><td>F1</td><td> Switch to the <a href="rec-menu.html">main menu</a></td></tr>
<tr><td>F2</td></tr>
<tr><td>F3</td></tr>
<tr><td> F1 + DOWN </td><td> Toggle key lock ON/OFF </td></tr>
</table>

#include "foot.t"


