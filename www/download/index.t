#define _PAGE_ Download
#define DOWNLOAD_PAGE
#include "head.t"

<h2>Version 1.1</h2>

<p>Recorder support, playlists and scrolling are the main new features of this version. Read the <a href="rockbox-1.1-notes.txt">release notes</a>.

<p>
<table><tr valign="top"><td>
<h3>player-old</h3>
<p>This version is for old Archos Jukebox 6000 models with ROM firmware older than 4.50.

<ul>
<li><a href="rockbox-1.1-player-old.mod">rockbox-1.1-player-old.mod</a>
</ul>

</td><td>

<h3>player</h3>
<p>This version is for Archos Jukebox 5000, 6000 with ROM firmware 4.50 or later, and all Studio models.

<ul>
<li><a href="rockbox-1.1-player.mod">rockbox-1.1-player.mod</a>
</ul>

<p><b>Bug</b>: Bass and treble adjustment does not work.

</td><td>

<h3>recorder</h3>
<p>This version is for all Archos Jukebox Recorder models.

<ul>
<li><a href="rockbox-1.1-recorder.ajz">rockbox-1.1-recorder.ajz</a>
</ul>

<p><b>Bug</b>: MP3 playback only works on Recorder 20, not 6 or 10. PLAY key is not detected, use RIGHT.

</td></tr></table>


<h3>Source code</h3>
<ul>
<li><a href="rockbox-1.1.tar.gz">rockbox-1.1.tar.gz</a>
</ul>

<h3>Screen shots (from simulator)</h3>

<p>
<table><tr>
<td><img src="player-sim.png" alt="Player simulator">
<br>Player boot screen</td>

<td><img src="recorder-sim.png" alt="Recorder simulator">
<br>Recorder boot screen</td>

<td><img src="browser.png" alt="File browser"><br>Recorder file browser</td>

<td><img src="id3.png" alt="ID3 display"><br>Recorder ID3 display</td>
</tr><tr>
<td><img src="tetris.png" alt="Tetris"><br>Tetris (recorder only)</td>
<td><img src="sokoban.png" alt="Sokoban"><br>Sokoban (recorder only)</td>
<td><img src="bounce.png" alt="Bounce"><br>Bouncing text (recorder only)</td>
</tr></table>


<h3>User interface simulators</h3>

<p>
<ul>
<li><a href="player-sim-1.1">player simulator 1.1 for linux-x86</a> (48300 bytes)
(if you don't have libgcc_s.so.1, get it <a href="libgcc_s.so.1">here</a>)
<li><a href="recorder-sim-1.1">recorder simulator 1.1 for linux-x86</a> (98184 bytes)
<li><a href="player-sim-1.1.exe">player simulator 1.1 for win32</a> (126976 bytes)
<li><a href="recorder-sim-1.1.exe">recorder simulator 1.1 for win32</a> (172032 bytes)
</ul>

<p>The simulators browse a directory called 'archos' in the directory they are started from. Create it and copy some mp3 files there. The buttons are simulated on the numeric keypad:
<dl>
<dt><b>Player</b>
<dd>4/6 = prev/next, 8 = play, 2 = stop, enter = menu, + = on

<dt><b>Recorder</b>
<dd>4/6 = left/right, 8/2 = up/down, 5 = play/pause, / * - = menu keys, +/enter = on/off
</dl>

<h2>Installation</h2>
<p>Rename your current archos.mod (if any) in the root of your archos, then copy the rockbox archos.mod there. Make sure it's called "archos.mod" (5000,6000,Studio) or "ajbrec.ajz" (Recorder).

<p>To remove the Rockbox firmware, just reverse the process: Rename the rockbox archos.mod file and replace it with your original firmware file. If you had no firmware file, just renaming or removing the rockbox file is sufficient.

<p>(Note that some models cannot shut off while the power adapter is plugged in.)

<h2>Bug reports</h2>

<p>Please use our
<a href="http://sourceforge.net/tracker/?group_id=44306&atid=439118">Sourceforge page</a>
for all bug reports and feature requests. If you have a sourceforge account,
please log in first so we have a name to connect to the report.

<p>If you are interested in helping with the development of Rockbox, please join the mailing list.

#include "foot.t"
