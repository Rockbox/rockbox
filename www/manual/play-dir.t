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
Buttons:
<table class=listtable>
<tr><td>-</td><td>
      Move upwards in the list, if it already is on the topmost entry, you'll
      see the cursor moved to the last entry in the list.
</td></tr>
<tr><td> +</td><td>
        Move downwards in the list, if it already is on the last entry, you'll
        see the cursor moved to the first entry in the list.
</td></tr>
<tr><td>
 STOP
</td><td>
   Move "up" one directory level to the parent directory.
</td></tr>
<tr><td>
 PLAY
</td><td>
   If the cursor is now on a line of a directory, the browser will switch
        into browsing that directory. If the cursor is on a line with a file,
        special actions are peformed depending on the file type. If the file
        is an mp3 file, it'll jump into the <a href="play-wps.html">WPS</a>
        and start playing this file.
        If the file is an m3u file (playlist), the list will first be loaded
        and then pop up the WPS showing the first file that gets played from
        the list. If you have 'shuffle' selected in the global settings, the
        playlist will be shuffled directly after having been loaded, otherwise
        it'll be played in a straight top-to-bottom fashion.
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
