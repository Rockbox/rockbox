#define _PAGE_ Manual - Directory Browser
#include "head.t"
#include "manual.t"

<p>
<table>
<tr valign=top><td>
<img src="rec-dir.png" width=120 height=68 alt="Dir browser">
<br><small>Recorder screenshot</small>
</td>
</tr></table>


<p>
In the dir browser, you navigate your way around the harddisk.
There are icons on the left side of each file that shows what file type it is.

<p>
The list of files you see is affected by the
<a href="general.html">"Show files" setting</a>.

<h2>Button bindings</h2>

<table class=buttontable>
<tr><th>Button</th><th>Function</th>
<tr valign=top>
<td nowrap> UP/DOWN (r) <br> LEFT/RIGHT (p)</td>
<td>
     Go to previous/next item in list. If you are on the first/last entry, 
     the cursor will wrap to the last/first entry.
</td></tr>
<tr valign=top>
<td nowrap> ON + UP/DOWN </td>
<td>
     (Recorder only:) Move one page up/down in list.
</td></tr>
<tr valign=top>
<td> LEFT (r) <br> STOP (p)</td>
<td>   Go to the parent directory.
</td></tr>
<tr valign=top>
<td> PLAY </td>
<td>Action depends on the file type the cursor points at:
<dl>
<dt><b>Directory</b>
<dd>The browser enters that directory.

<dt><b>.mp3 file</b>
<dd>You will be taken to the <a href="wps.html">WPS</a>
and start playing the file.

<dt><b>.m3u file</b>
<dd>The playlist will be loaded and started and you will then be taken to the WPS.

<dt><b>.ajz (recorder) or .mod (player) file</b>
<dd>The firmware file will be loaded and executed.

<dt><b>.wps file</b>
<dd>The file will be loaded and used for the wps display.
  Look <a href="/docs/custom_wps_format.html">here</a> for information about
  the .wps file format.

<dt><b>.cfg file</b>
<dd>The file will be loaded and the sound settings will be set accordingly.
  Look <a href="/docs/custom_cfg_format.html">here</a> for information about
  the .cfg file format.

<dt><b>.lng file</b>
<dd>The language file will be loaded and replace the current language.
  Look <a href="/lang/">here</a> for downloadable language files.

<dt><b>.txt file</b>
<dd> The text file will be displayed in the <a href="textreader.html">Text Reader</a>.

<dt><b>.fnt file</b>
<dd> (Recorder only:) The font will be loaded and used in place of the
  default font.
  Look <a href="/fonts/">here</a> for downloadable fonts.

</dl>
</td></tr>
<tr><td>
 ON
</td><td>
 If there is an mp3 playing, this will go back to the WPS.
</td></tr>
<tr><td>
 OFF (r)
</td><td>
    If there is an mp3 playing, this stops playback.
</td></tr>
<tr><td>F1 (r) <br>MENU (p)</td>
<td>Switches to the <a href="menu.html">main menu</a>.</td></tr>
<tr><td>F2 (r)</td><td>(Recorder only) Show browse/play settings screen. A quick press will leave the screen up (press F2 again to exit), while holding it will close the screen when you release it.</td></tr>
<tr><td>F3 (r)</td><td>(Recorder only) Show display settings screen. Quick/hold works as for F2.</td></tr>
</table>

#include "foot.t"
