#define _PAGE_ Rockbox Directory Browser
#include "head.t"
#include "manual.t"
<p>
In the dir browser, you will see the files and directories that are put in the
directory you are currently browsing.
<p>
[screen dump]
<p>
If you have enabled 'mp3/m3u filter' in the global settings, only files ending
with .mp3 or .m3u will be displayed in the browser.
<p>
<table class=buttontable>
<tr><th>Button</th><th>Function</th>
<tr><td>-</td><td>
      Move up in the list. If you already are on the topmost entry, the
      cursor will move to the last entry in the list.
</td></tr>
<tr><td> +</td><td>
        Move down in the list. If you already are on the last entry, 
        the cursor will move to the first entry in the list.
</td></tr>
<tr><td>
 STOP
</td><td>
   Move to the parent directory.
</td></tr>
<tr><td>
 PLAY
</td>
<td>If the cursor is on a directory, the browser will go into that directory.

<p>If the cursor is on an .mp3 file, it'll jump into the
  <a href="rec-wps.html">WPS</a> and start playing this file. It will then
  continue to play the other songs in the same directory.

<p> If the cursor is on an .m3u file (playlist), the list will first be
  loaded and then pop up the WPS showing the first file that gets played from
  the list.

<p> If the cursor is on a .mod file (firmware), the firmware file will
  be loaded and executed.

<p> If the cursor is on a .wps file (WPS config), the file will
  be loaded and used for the wps display.

</td></tr>
<tr><td>
 ON 
</td><td>
    If there is an mp3 playing, this'll switch back the
        <a href="play-wps.html">WPS</a>.
</td></tr>
<tr><td>
 MENU 
</td><td>
  Switches to the <a href="play-menu.html">main menu</a>.
</td></tr>
</table>

#include "foot.t"
