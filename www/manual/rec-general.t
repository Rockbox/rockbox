#define _PAGE_ Rockbox Sound Settings
#include "head.t"
#include "manual.t"
The general settings menu offers:
<p>
 <img src="rec-generalsettings.png">

<p> <b>Shuffle mode</b> - Select shuffle ON/OFF. This alters how Rockbox will
  select which next song to play.

<p> <b>mp3/m3u filter</b> - When set to ON, this will hide all files in the <a
href="rec-dir.html">dir browser</a> that aren't mp3 or m3u files. Set to OFF
to see all files.

<p> <b>Sort mode</b> - How to sort directories displayed in the <a
href="rec-dir.html">dir browser</a>. Case sensitive ON makes uppercase and
lowercase differences matter. Having it OFF makes them get treated the same.

<p> <b>Backlight timer</b> - How long time the backlight should be switched on
after a keypress. Set to OFF to never light it, set to ON to never shut it off
or set a prefered timeout period.

<p> <b>Contrast</b> - changes the contrast of your LCD display. 

<p> <b>Scroll speed</b> - controls at what speed the general line scrolling
will use. The scroll appears in the <a href="rec-dir.html">dir browser</a>,
the <a href="rec-wps.html">WPS</a> and elsewhere.

<p> <b>While Playing</b> - ID3 tags, file, parse

<p> <b>Deep discharge</b> - Set this to ON if you intend to keep your charger
connected for a long period of time. It lets the batteries go down to 10%
before starting to charge again. Setting this to OFF will cause the charging
restart on 95%.

<p> <b>Time/Date</b> - Set current time and date.

<p> <b>Show Hidden Files</b> - Files with names that start with a dot or use
the 'hidden' attribute are shown in the <a href="rec-dir.html">dir browser</a>
if this is set to ON. If set to OFF, they're not shown.

<p> <b>FF/Rewind</b> - set the FF/rewind speed. Holding down the LEFT or RIGHT
keys in the <a href="rec-wps.html">WPS</a> makes a fast forward/backward. This
setting controls how fast it is.

<p> <b>Resume</b> - ASK, ON or OFF. If set to ASK or ON, Rockbox will save
info about playlists/directories being played, to allow later resuming. If ASK
is selected, a 'continue resumed play' question will appear when starting
Rockbox. If ON is selected, resuming is done unconditionally. If OFF, no
resuming is made.

<p> <b>Disk Spindown</b> - Rockbox has a timer that makes it spin down the
disk driver a certain time of idleness. You modify this timeout here. A very
short timeout makes the interface less responsive and use more batteries for
more frequent spinups, while a long timeout makes the disk spin too long and
use more batteries...

<p>
Buttons:
<p>
<table>
<tr><td>UP</td><td>Move upwards in the list</td></tr>
<tr><td>DOWN</td><td>Move downwards in the list</td></tr>
<tr><td>RIGHT</td><td>Select item in list</td></tr>
<tr><td>F1</td><td>Toggle back to the screen you came from when you arrived here, <a href="rec-wps.html">WPS</a> or <a href="rec-dir.html">dir browser</a></td></tr>
</table>

#include "foot.t"

