#define _PAGE_ Rockbox Directory Browser
#include "head.t"
#include "manual.t"
<p>
In the dir browser, you will see the files and directories that are put in the
directory you are currently browsing. You will see tiny symbols on the left
side of each entry that shows what kind of entry it is.
<p>
[screen dump]
<p>
[ ] = mp3 file
<p>
[ ] = m3u file
<p>
If you have enabled 'mp3/m3u filter' in the global settings, only files ending
with .mp3 or .m3u will be displayed in the browser.
<p>
Buttons:
<p>
<table>
<tr>
<td> UP </td>
<td>
     Move upwards in the list, if it already is on the topmost entry, you'll
        see the cursor moved to the last entry in the list.
</td></tr>
<tr>
<td>DOWN</td>
<td>
   Move downwards in the list, if it already is on the last entry, you'll
        see the cursor moved to the first entry in the list.
</td></tr>
<tr>
<td> LEFT</td>
<td>   Move "up" one directory level to the parent directory.
</td></tr>
<tr>
<td> RIGHT</td>
<td>  If the cursor is now on a line of a directory, the browser will switch
        into browsing that directory. If the cursor is on a line with a file,
        special actions are peformed depending on the file type. If the file
        is an mp3 file, it'll jump into the <a href="rec-wps.html">WPS</a> and
        start playing this file.
        If the file is an m3u file (playlist), the list will first be loaded
        and then pop up the WPS showing the first file that gets played from
        the list. If you have 'shuffle' selected in the global settings, the
        playlist will be shuffled directly after having been loaded, otherwise
        it'll be played in a straight top-to-bottom fashion.
</td></tr>
<tr><td>
 PLAY
</td><td>
   The same as RIGHT
</td></tr>
<tr><td>
 ON
</td><td>
 If there is an mp3 playing, this'll switch back the WPS.
</td></tr>
<tr><td>
 OFF
</td><td>
    If there is an mp3 playing, this stops playback.
</td></tr>
<tr><td>
 F1
</td><td>
 Switches to the <a href="rec-menu.html">main menu</a>.
</td></tr>
<tr><td>
 F2
</td></tr>
<tr><td>
 F3
</td></tr>
</table>

#include "foot.t"
