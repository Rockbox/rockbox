#define _PAGE_ Rockbox WPS
#include "head.t"
#include "manual.t"
<p>
The While Playing Screen shows info about what is currently played and can
only be seen while there is actual music playback.
<p>
<img src="rec-wps.png">
<p>
The screen lines by default contain the following information:
<ol>
<li>Status bar: Battery level, charger status, volume, play mode, repeat mode, shuffle mode and clock
<li>Scrolling path+filename of the current song.
<li>The ID3 track name
<li>The ID3 album name
<li>The ID3 artist namn
<li>Bit rate. VBR files display average bitrate and "(avg)".
<li>Elapsed and total time
<li>A slidebar representing where in the song you are
</ol>

<p>You can configure the WPS contents by creating a .wps file and "playing" it.
The <a href="/docs/custom_wps_format.html">custom wps format</a> supports a wide variety of configurations.

<p>
<table class=buttontable>
<tr><th>Key</th><th>Function</th></tr>
<tr><td> UP</td><td> Increase volume </td></tr>
<tr><td> DOWN </td><td>  Decrease volume </td></tr>

<tr><td> LEFT </td><td> Quick press = Go to beginning of track, or if
pressed while in the first second of a track, go to previous track.<br>
Hold = Rewind in track. </td></tr>

<tr><td> RIGHT </td><td> Quick press = Go to next track. <br>
Hold = Fast-forward in track. </td></tr>
<tr><td> PLAY </td><td> Toggle play/pause.</td></tr>
<tr><td>ON</td><td> Quick press: Go to <a href="rec-dir.html">dir browser</a> <br>
Hold: Show pitch setting screen </td></tr>
<tr><td>OFF</td><td> Stop playback </td></tr>
<tr><td>F1</td><td> Go to <a href="rec-menu.html">main menu</a></td></tr>
<tr><td nowrap>F1 + DOWN</td><td> Key lock on/off </td></tr>
<tr><td nowrap>F1 + PLAY</td><td> Mute on/off </td></tr>
<tr><td nowrap>F1 + ON</td><td> Enter ID3 viewer </td></tr>
<tr><td>F2</td><td>Show browse/play settings screen</td></tr>
<tr><td>F3</td><td>Show display settings screen</td></tr>
</table>

#include "foot.t"
