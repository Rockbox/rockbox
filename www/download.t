#define _PAGE_ Download
#define DOWNLOAD_PAGE
#include "head.t"

<h2>Version 1.0</h2>

<p>This first version is more a proof-of-concept than a serious replacement for the stock firmware.
Read the <a href="rockbox-1.0-notes.txt">release notes</a>.

<h3>player-old</h3>
<p>This version is for old Archos Jukebox 6000 models with ROM firmware older than 4.50.

<ul>
<li><a href="rockbox-1.0-player-old.zip">rockbox-1.0-player-old.zip</a>
</ul>

<h3>player</h3>
<p>This version is for Archos Jukebox 6000 with ROM firmware 4.50 or later, and all Studio models.

<ul>
<li><a href="rockbox-1.0-player.zip">rockbox-1.0-player.zip</a>
</ul>

<h3>recorder</h3>
<p>Version 1.0 does not support the Recorder. Future versions will.

<h3>Source code</h3>
<ul>
<li><a href="rockbox-1.0.tar.gz">rockbox-1.0.tar.gz</a>
</ul>

<h3>User interface simulators</h3>
<p><img src="player-sim.png" alt="Player simulator">
<img src="recorder-sim.png" alt="Recorder simulator">

<ul>
<li><a href="player-sim-1.0">player simulator 1.0 for linux</a> (42880 bytes)
(if you don't have libgcc_s.so.1, get it <a href="libgcc_s.so.1">here</a>)
<li><a href="recorder-sim-1.0">recorder simulator 1.0 for linux</a> (51976 bytes)
<li><a href="recorder-sim-1.0.exe">recorder simulator 1.0 for win32</a> (167936 bytes)
</ul>

<p>The simulators browse a directory called 'archos' in the directory they are started from. Create it and copy some mp3 files there. The buttons are simulated on the numeric keypad:
<dl>
<dt><b>Player</b>
<dd>4/6 = prev/next, 8 = play, 2 = stop, enter = menu, + = on

<dt><b>Recorder</b>
<dd>4/6 = left/right, 8/2 = up/down, 5 = play/pause, / * - = menu keys
</dl>

<h2>Installation</h2>
<p>Unzip the archive. Rename your current archos.mod (if any) in the root of your archos, then copy the rockbox archos.mod there.

<p>To remove the Rockbox firmware, just reverse the process: Rename the rockbox archos.mod file and replace it with your original firmware file. If you had no firmware file, just renaming or removing the rockbox file is sufficient.

<p>(Note that some models cannot shut off while the power adapter is plugged in.)

<h2>Bug reports</h2>

<p>Please use our
<a href="http://sourceforge.net/projects/rockbox/">Sourceforge page</a>
for all bug reports and feature requests. If you have a sourceforge account,
please log in first so we have a name to connect to the report.

<p>If you are interested in helping with the development of Rockbox, please join the mailing list.

#include "foot.t"
