#define _PAGE_ Firmware Feature Comparison Chart
#include "head.t"

#define NAME    <tr><td class=feature>
#define ENAME   </td>
#define TD <td class=fneutral>
#define ETD </td>
#define EFEAT </tr>

#define YES  <td class=fgood>Yes ETD
#define PARTLY <td class=fgood>Partly ETD
#define NO   <td class=fbad>No ETD
#define BADYES  <td class=fbad>Yes ETD
#define GOODNO  <td class=fgood>No ETD
#define UNKNOWN TD ? ETD

<p>
 The Rockbox column may specify features only available in CVS and daily
 builds.

<p>
<table class=rockbox>

<tr class=header><th>Feature</th><th>Rockbox</th><th>Archos</th></tr>

NAME ID3v1 and ID3v2 support ENAME
YES
UNKNOWN
EFEAT

NAME Background noise during playback ENAME
GOODNO
BADYES
EFEAT

NAME Mid-track resume ENAME
YES
NO
EFEAT

NAME Mid-playlist resume ENAME
YES
NO
EFEAT

NAME Resumed playlist order ENAME
YES
NO
EFEAT

NAME Battery lifetime ENAME
TD Longer ETD
TD  Long ETD
EFEAT

NAME Battery time indicator ENAME
YES
NO
EFEAT

NAME Customizable font (Recorder) ENAME
YES
NO
EFEAT

NAME Customizable screen info when playing songs ENAME
YES
NO
EFEAT

NAME USB attach/detach without reboot ENAME
YES
NO
EFEAT

NAME Can load another firmware without rebooting ENAME
YES
NO
EFEAT

NAME Playlist load speed, songs/sec ENAME
TD 3000 - 4000 ETD
TD 15 - 20 ETD
EFEAT

NAME Max number of songs in a playlist ENAME
TD 20 000 ETD
TD 999 ETD
EFEAT

NAME Supports bad path prefixes in playlists ENAME
YES
YES
EFEAT

NAME Open source/development process ENAME
YES
NO
EFEAT

NAME Corrects reported bugs ENAME
YES
NO
EFEAT

NAME Automatic Volume Control (Recorder) ENAME
YES
NO
EFEAT

NAME Pitch control (Recorder) ENAME
YES
NO
EFEAT

NAME Text File Reader ENAME
YES
YES
EFEAT

NAME Games (Recorder) ENAME
TD 7 ETD
NO
EFEAT

NAME File Delete & Rename ENAME
YES
YES
EFEAT

NAME Playlist Building ENAME
YES
YES
EFEAT

NAME Recording (Recorder) ENAME
YES
YES
EFEAT

NAME Generates XING VBR header when recording ENAME
YES
YES
EFEAT

NAME High Resolution Volume Control ENAME
YES
NO
EFEAT

NAME Deep discharge option (Recorder) ENAME
YES
NO
EFEAT

NAME Customizable backlight timeout ENAME
YES
YES
EFEAT

NAME Backlight-on when charging option ENAME
YES
NO
EFEAT

NAME Queue function ENAME
YES
YES
EFEAT

NAME Supports the XING header ENAME
YES
YES
EFEAT

NAME Supports the VBRI header ENAME
PARTLY
YES
EFEAT

NAME Max number of files in a dir ENAME
TD 10 000 ETD
TD 999 ETD
EFEAT

NAME Adjustable scroll speed ENAME
YES
NO
EFEAT

NAME Screensaver style demos (Recorder) ENAME
YES
NO
EFEAT

NAME Variable step / accelerating ffwd and rwd ENAME
YES
NO
EFEAT

NAME Visual Progress Bar ENAME
YES
NO
EFEAT

NAME Select/Load configs ENAME
YES
NO
EFEAT

NAME Sleep timer ENAME
YES
NO
EFEAT

NAME Easy User Interface ENAME
YES
NO
EFEAT

NAME Remote Control Controllable ENAME
YES
YES
EFEAT

NAME ISO8859-1 font support (Player) ENAME
YES
NO
EFEAT

NAME Queue songs to play next ENAME
YES
YES
EFEAT

NAME Bookmark positions in songs ENAME
NO
NO
EFEAT

NAME Number of available languages ENAME
TD 19 ETD
TD 3? ETD
EFEAT

NAME Accurate VBR bitrate display ENAME
YES
NO
EFEAT

NAME FM Tuner support (FM Recorder) ENAME
YES
YES
EFEAT

</table>

<p>
 Wrong facts? Mail rockbox@cool.haxx.se now!

#include "foot.t"
