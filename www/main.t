#define _LOGO_ <img src="rockbox400.png" align=center width=400 height=123>
#define _PAGE_ Open Source Jukebox Firmware
#define MAIN_PAGE
#include "head.t"

<h2>Purpose</h2>

<p>The purpose of this project is to write an Open Source replacement
firmware for the Archos Jukebox <i>5000</i>, <i>6000</i>, <i>Studio</i> and <i>Recorder</i> MP3 players.

<h2>News</h2>

<p><i>2002-08-06</i>: Web site moved to new domain: <strong>rockbox.haxx.se</strong>. Bear with us while we chase down the last bad links.

<p><i>2002-08-06</i>: Battery charging added for the Recorders (players have hardware charging).

<p><i>2002-08-02</i>: <a href="tshirt-contest">Rockbox T-Shirt Design Contest</a>

<p><i>2002-07-28</i>: Configuration saving implemented for all models. Experimental saving to disk
is not yet enabled on the players by default.

<p><i>2002-06-30</i>: USB cable detection added.

<p><i>2002-06-27</i>: MP3 playback now works for Recorder 6000 and Recorder 10 too.

<p><i>2002-06-19</i>: Version 1.1 is released. <a href="download/">Download it here</a>.

<p><i><small>(Old news items have moved to a 
<a href="history.html">separate page</a>.)</small></i>

<h2>Recent CVS activity</h2>
<p>
<!--#include file="lastcvs.link" -->

<h2>Open bug reports</h2>
<p>
<!--#include file="bugs.html" -->
<small><b>Note:</b> Don't file bug reports on daily builds. They are work in progress.</small>

<h2>Roadmap</h2>
<p>This is a rough indication of which features we plan/expect/hope to be
included in which versions. Naturally, this is all subject to change without
notice.

<dl>
<dt><b>Version 1.1</b>
<dd>Playlist support, scrolling, recorder support

<dt><b>Version 1.2</b>
<dd>UI improvements

<dt><b>Version 1.3</b>
<dd>Resume, persistent settings, autobuild playlists, UI improvements
</dl>

<h2>About the hardware</h2>

<p>I wrote a <a href="internals/bjorn.html">"dissection" page</a> some months ago,
showing the inside of the Archos and listing the main components.
I have also collected a couple of <a href="docs/">data sheets</a>.
Also, don't miss the <a href="notes.html">research notes</a>
from my reverse-engineering the firmware.

<h2>About the software</h2>

<p>The player has one version of the firmware in ROM.
The first thing this version does after boot is to look for a file called
"archos.mod" on the Player/Studio or "ajbrec.ajz" on the recorder in the
root directory of the harddisk. 
If it exists, it is loaded into RAM and started.
This is how firmware upgrades are loaded. Note that you can revert to the ROM
version by just deleting the firmware file.

#include "foot.t"
