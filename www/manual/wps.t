#define _PAGE_ Manual - While Playing Screen
#include "head.t"
#include "manual.t"
<p>
<img src="rec-wps.png">
<br><small>Recorder screenshot</small>

<p>
The While Playing Screen shows info about what is currently played and can
only be seen while there is actual music playback.

<h2>Screen contents</h2>

<p>
The screen lines by default contain the following information:

<h3>Recorder</h3>
<ol>
<li>Status bar: Battery level, charger status, volume, play mode, repeat mode, shuffle mode and clock
<li>Scrolling path+filename of the current song.
<li>The ID3 track name
<li>The ID3 album name
<li>The ID3 artist namn
<li>Bit rate. VBR files display average bitrate and "(avg)".
<li>Elapsed and total time
<li>A slidebar progress meter representing where in the song you are
<li>Peak meter
</ol>

<p>Notes:
<li>The number of lines shown depends on the size of the font used.
<li>The peak meter is normally only visible if you turn off the status bar.

<h3>Player</h3>
<ol>
<li> Playlist index/Playlist size: Artist - Title
<li> Current-time Progress-indicator Left
</ol>

<h2>Configuration</h2>

<p>You can configure the WPS contents by creating a .wps file and "playing" it.
The <a href="/docs/custom_wps_format.html">custom wps format</a> supports a wide variety of configurations.

<h2>Button bindings</h2>

<p>
<table class=buttontable>
<tr><th>Key</th><th>Function</th></tr>
<tr><td nowrap> UP (r) <br>MENU + RIGHT (p)</td>
<td> Increase volume </td></tr>
<tr><td> DOWN (r) <br>MENU + LEFT (p)</td>
<td>  Decrease volume </td></tr>

<tr><td> LEFT </td><td> Quick press = Go to beginning of track, or if
pressed while in the first seconds of a track, go to previous track.<br>
Hold = Rewind in track. </td></tr>

<tr><td> RIGHT </td><td> Quick press = Go to next track. <br>
Hold = Fast-forward in track. </td></tr>
<tr><td> PLAY </td><td> Toggle play/pause.</td></tr>
<tr><td>ON</td>
<td> Quick press = Go to <a href="dir.html">dir browser</a> <br>
(Recorder only:) Hold = Show pitch setting screen </td></tr>
<tr><td>OFF (r) <br>STOP (p)</td><td> Stop playback </td></tr>
<tr><td>F1 (r) <br>MENU (p)</td>
<td> Go to <a href="menu.html">main menu</a></td></tr>
<tr><td nowrap>F1 + DOWN (r) <br>MENU + DOWN (p)</td>
<td> Key lock on/off </td></tr>
<tr><td nowrap>F1 + PLAY (r) <br>MENU + PLAY (p)</td>
<td> Mute on/off </td></tr>
<tr><td nowrap>F1 + ON (r) <br>MENU + ON (p)</td>
<td> Enter ID3 viewer </td></tr>
<tr><td>F2 (r)</td><td>(Recorder only:) Show browse/play settings screen</td></tr>
<tr><td>F3 (r)</td><td>(Recorder only:) Show display settings screen</td></tr>
</table>

#include "foot.t"
