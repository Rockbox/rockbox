#define _PAGE_ Download
#define DOWNLOAD_PAGE
#include "head.t"

<h2>Changes for version 1.3</h2>

<p>Please read the <a href="rockbox-1.3-notes.txt">release notes</a>. (<a href="old.html">Older releases</a>)

<p>
<table><tr valign="top"><td>
<h3>player-old</h3>
<p>This version is for old Archos Jukebox 6000 models with ROM firmware older than 4.50.

<ul>
<li><a href="rockbox-1.3-player-old.mod">rockbox-1.3-player-old.mod</a>
</ul>

</td><td>

<h3>player</h3>
<p>This version is for Archos Jukebox 5000, 6000 with ROM firmware 4.50 or later, and all Studio models.

<ul>
<li><a href="rockbox-1.3-player.mod">rockbox-1.3-player.mod</a>
</ul>

</td><td>

<h3>recorder</h3>
<p>This version is for all Archos Jukebox Recorder models.

<ul>
<li><a href="rockbox-1.3-recorder.ajz">rockbox-1.3-recorder.ajz</a>
</ul>

</td></tr></table>

<h3>Source code</h3>
<ul>
<li><a href="rockbox-1.3.tar.gz">rockbox-1.3.tar.gz</a>
</ul>

<h3>User interface simulators</h3>

<p>
<ul>
<li><a href="rockbox-1.3-player-sim">player simulator 1.3 for linux-x86</a>
<li><a href="rockbox-1.3-recorder-sim">recorder simulator 1.3 for linux-x86</a>
<li><a href="rockbox-1.2-player-sim.exe">player simulator 1.2 for win32</a>
<li><a href="rockbox-1.2-recorder-sim.exe">recorder simulator 1.2 for win32</a>
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

<p>Please use our <a href="/bugs.html">bug page</a>
for all bug reports and feature requests.

<p>If you are interested in helping with the development of Rockbox, please join the mailing list.

#include "foot.t"
