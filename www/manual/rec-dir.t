#define _PAGE_ Rockbox Directory Browser
#include "head.t"
#include "manual.t"
<p>
In the dir browser, you will see the files and directories that are put in the
directory you are currently browsing. You will see tiny symbols on the left
side of each entry that shows what kind of entry it is.

<p>
<table>
<tr valign=top><td>
<img src="rec-dir.png" width=120 height=70 alt="Dir browser">

</td>
<td>

<table>

<tr>
 <td> <img src="rec-folder.png" width=33 height=42 alt="folder icon"></td>
 <td>folder</td>
</tr>
<tr>
 <td> <img src="rec-m3u.png" width=35 height=45 alt="m3u icon"></td>
 <td>playlist</td>
</tr>
<tr>
 <td> <img src="rec-mp3.png" width=32 height=40 alt="mp3 icon"></td>
 <td>mp3 file </td>
</tr>
</table>

</td></tr></table>

<p>
If you have enabled 'mp3/m3u filter' in the global settings, only files ending
with .mp3 or .m3u will be displayed in the browser.

<p>

<table class=buttontable>
<tr><th>Button</th><th>Function</th>
<tr valign=top>
<td> UP </td>
<td>
     Move upwards in the list, if it already is on the topmost entry, you'll
        see the cursor moved to the last entry in the list.
</td></tr>
<tr valign=top>
<td>DOWN</td>
<td>
   Move downwards in the list, if it already is on the last entry, you'll
        see the cursor moved to the first entry in the list.
</td></tr>
<tr valign=top>
<td> LEFT</td>
<td>   Move "up" one directory level to the parent directory.
</td></tr>
<tr valign=top>
<td> PLAY or RIGHT</td>
<td>If the cursor is on a directory, the browser will go into that directory.

<p>If the cursor is on an .mp3 file, it'll jump into the
  <a href="rec-wps.html">WPS</a> and start playing this file. It will then
  continue to play the other songs in the same directory.

<p> If the cursor is on an .m3u file (playlist), the list will first be
  loaded and then pop up the WPS showing the first file that gets played from
  the list.

<p> If the cursor is on an .ajz file (firmware), the firmware file will
  be loaded and executed.

<p> If the cursor is on a .wps file (WPS config), the file will
  be loaded and used for the wps display.

<p> If the cursor is on a .eq file (Equalizer configuration), the file will
  be loaded and the sound settings will be set accordingly.

</td></tr>
<tr><td>
 ON
</td><td>
 If there is an mp3 playing, this will go back to the WPS.
</td></tr>
<tr><td>
 OFF
</td><td>
    If there is an mp3 playing, this stops playback.
</td></tr>
<tr><td>F1</td><td>Switches to the <a href="rec-menu.html">main menu</a>.</td></tr>
<tr><td>F2</td><td>Show browse/play settings screen</td></tr>
<tr><td>F3</td><td>Show display settings screen</td></tr>
</table>

#include "foot.t"
