#define _PAGE_ Firmware Feature Comparison Chart
#include "head.t"

#define NAME    <tr><td>
#define ENAME   </td>
#define TD <td>
#define ETD </td>
#define EFEAT </tr>

#define YES  TD <font color="#00ff00">Yes</font> ETD
#define NO   TD <font color="#ff0000">No</font> ETD
#define UNKNOWN TD ? ETD

<table>

NAME <b>ID3v1 and ID3v2</b> support ENAME
YES
UNKNOWN
EFEAT

NAME <b>Mid-track and mid-playlist resume</b> ENAME
YES
NO
EFEAT

NAME <b>Games</b> ENAME
TD Tetris, Sokoban, Snake ETD
NO
EFEAT

NAME When resuming playback of a shuffled playlist, <b>PREV still works</b>. ENAME
YES
NO
EFEAT

NAME <b>Battery lifetime</b> ENAME
TD Longer ETD
TD  Long ETD
EFEAT

NAME <b>Customizable font</b> (Recorder) ENAME
YES
NO
EFEAT

NAME <b>Customizable screen info</b> when playing songs ENAME
YES
NO
EFEAT

NAME <b>USB attach/detach without reboot</b> ENAME
YES
NO
EFEAT

NAME <b>Ability to run another firmware</b> without rebooting ENAME
YES
NO
EFEAT

NAME <b>Fast playlist loading</b> ENAME
YES
NO
EFEAT

NAME <b>Open source/development process</b> ENAME
YES
NO
EFEAT

NAME <b>Corrects reported bugs</b> ENAME
YES
NO
EFEAT

NAME <b>Text File Reader</b> ENAME
YES
NO
EFEAT

NAME <b>File Management</b> ENAME
NO
YES
EFEAT

NAME <b>Playlist Building</b> ENAME
NO
YES
EFEAT

NAME <b>Recording</b> (Recorder) ENAME
TD Developing ETD
YES
EFEAT

</table>

<p>
 Wrong facts? Mail rockbox@cool.haxx.se now!

#include "foot.t"
