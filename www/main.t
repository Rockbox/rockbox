#define _LOGO_ <img src="rockbox400.png" width=400 height=123>
#define _PAGE_ Open Source Jukebox Firmware
#include "head.t"

<p><small><a href="docs/FAQ">faq</a> &middot;
<a href="notes.html">research notes</a> &middot;
<a href="docs/">data sheets</a> &middot;
<a href="schematics/">schematics</a> &middot;
<a href="mods/">hardware mods</a> &middot;
<a href="http://bjorn.haxx.se/rockbox/mail.cgi">mail list archive</a> &middot;
<a href="irc/">irc logs</a> &middot;
<a href="tools.html">tools</a> &middot;
<a href="internals/">photos</a> &middot;
<a href="http://sourceforge.net/projects/rockbox/">sourceforge project</a> &middot;
<a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/">browse cvs</a>
</small>

<h2>Purpose</h2>

<p>The purpose of this project is to write an Open Source replacement
firmware for the Archos Jukebox <i>6000</i>, <i>Studio</i> and <i>Recorder</i> MP3 players.

<p>The main emphasis and first target is the Jukebox 6000.

#if 0
<h2>Warning</h2>
<p>All firmware mods on this page are still <i><b>highly experimental</b></i>.
Try them on your own risk. If you are not 100% sure of what you are doing, keep cool.
#endif

<h2>Activity</h2>

#include "activity.html"

<h2>News</h2>

<p><i>2002-03-28</i>: Lots of new stuff on the web page:
<a href="docs/FAQ">faq</a>,
<a href="irc/">irc logs</a>,
<a href="tools.html">tools</a> and
<a href="internals/">photos</a>.

<p><i>2002-03-25</i>: New section for
<a href="mods/">hardware modifications</a>.
First out is the long awaited
<a href="mods/serialport.html">serial port mod</a>.

<p><i>2002-03-25</i>: New instructions for
<a href="cross-gcc.html">how to build an SH-1 cross-compiler</a>.

<p><i>2002-03-14</i>: New linux patch and instructions for 
<a href="lock.html">unlocking the archos harddisk</a> if you have the "Part. Error" problem.

<p><i>2002-03-08</i>: Uploaded a simple example, showing
<a href="example/">how to build a program for the Archos</a>.

<p><i>2002-03-05</i>: The 
<a href="lock.html">harddisk password lock problem is solved</a>!
Development can now resume at full speed!

<p><i>2002-01-29</i>: If you have feature requests or suggestions,
please submit them to our
<a href="http://sourceforge.net/projects/rockbox/">Sourceforge page</a>.

<p><i>2002-01-19</i>: Cool logo submitted by Thomas Saeys.

<p><i>2002-01-16</i>: The project now has a proper name: Rockbox. 
Logos are welcome! :-)
<br>Also, Felix Arends wrote a quick <a href="sh-win/">tutorial</a>
for how to get sh-gcc running under windows.

<p><i>2002-01-09</i>: Nicolas Sauzede 
<a href="mail/rockbox-archive-2002-01/0096.shtml">found out</a>
how to 
<a href="mail/rockbox-archive-2002-01/0099.shtml">display icons and custom characters</a> on the Jukebox LCD.

<p><i>2002-01-08</i>: The two LCD charsets have been 
<a href="notes.html#charsets">mapped and drawn</a>.

<p><i>2002-01-07</i>:
<a href="mail/rockbox-archive-2002-01/0026.shtml">Jukebox LCD code</a>.
I have written a small test program that scrolls some text on the display.
You need
<a href="mail/rockbox-archive-2002-01/att-0026/01-archos.mod.gz">this file</a>
for units with ROM earlier than 4.50 and
<a href="mail/rockbox-archive-2002-01/att-0050/02-archos.mod.gz">this file</a>
for all others. (The files are gzipped, you need to unzip them before they will work.)

<p><i>2001-12-29</i>: Recorder LCD code. Gary Czvitkovicz knew the Recorder LCD controller since before and wrote some
<a href="mail/rockbox-archive-2001-12/att-0145/01-ajbr_lcd.zip">code</a>
that writes text on the Recorder screen.

<p><i>2001-12-13</i>: First program 
<a href="mail/rockbox-archive-2001-12/0070.shtml">released</a>!
A 550 bytes long 
<a href="mail/rockbox-archive-2001-12/att-0070/01-archos.mod">archos.mod</a>
that performs the amazing magic of flashing the red LED. :-)

<p><i>2001-12-11</i>: Checksum algorithm solved, thanks to Andy Choi. A new "scramble" utility is available.

<p><i>2001-12-09</i>: Working my way through the setup code. The <a href="notes.html">notes</a> are being updated continously.

<p><i>2001-12-08</i>: Analyzed the exception vector table. See <a href="notes.html">the notes</a>. Also, a <a href="mail.cgi">mailing list archive</a> is up.

<p><i>2001-12-07</i>:
 I just wrote this web page to announce descramble.c. 
I've disassembled one firmware version and looked a bit on the code, but no real analysis yet.
Summary: Lots of dreams, very little reality. :-)

<p>I've set up a mailing list: rockbox@cool.haxx.se.
To subscribe, send a message to <a href="mailto:majordomo@cool.haxx.se">majordomo@cool.haxx.se</a> with the words "subscribe rockbox" in the body.


<h2>About the hardware</h2>

<p>I wrote a <a href="internals/bjorn.html">"dissection" page</a> some months ago,
showing the inside of the Archos and listing the main components.
I have also collected a couple of <a href="docs/">data sheets</a>.
Also, don't miss the <a href="notes.html">research notes</a>
from my reverse-engineering the firmware.

<h2>About the software</h2>

<p>The player has one version of the firmware burnt into flash ROM.
The first thing this version does after boot is to look for a file called
"archos.mod" in the root directory of the harddisk. 
If it exists, it is loaded into RAM and started.
This is how firmware upgrades are loaded.

<h2>Dreams</h2>
<p>Ok, forget about reality, what could we do with this?

<ul>
<li>All those simple mp3-play features we sometimes miss:
 <ul>
 <li>No pause between songs
 <li>Mid-song resume
 <li>Mid-playlist resume
 <li>No-scan playlists
 <li>Unlimited playlist size
 <li>Autobuild playlists (such as "all songs in this directory tree")
 <li>Auto-continue play in the next directory
 <li>Current folder and all sub-folder random play
 <li>Full disk random play
 <li>REAL random (if press back it goes to the previous song that was played)
 <li>Multi song queue (folder queue)
</ul>
<li>Faster scroll speed
<li>Archos Recorder support. Most of the hardware is the same, but the display and some other things differ.
<li>All kinds of cool features done from the wire remote control, including controlling your Archos from your car radio (req hw mod)
<li>Ogg Vorbis support [unverified: the MAS is somewhat programmable, but enough?]
<li>Support for megabass switch (req hw mod) [unverified: I just saw the DAC docs shows how to do it switchable. we need a free port pin to be able to switch]
<li>Player control via USB [unverified]
<li>Memory expansion? [doubtful: the current DRAM chip only has 10 address lines. we'd have to pull off one heck of a hw mod to expand that]
</ul>

#include "foot.t"
