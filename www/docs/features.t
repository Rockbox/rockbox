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
#define NA TD N/A ETD

<p>
 The Rockbox column may specify features only available in CVS and daily
 builds.

<p>
<table class=rockbox>

<tr class=header>
<th>Feature</th>
<th>Rockbox</th>
<th>Archos</th>
<th>iRiver h1x0</th>
</tr>

NAME ID3v1 and ID3v2 support ENAME
YES
TD ID3v1 ETD
YES
EFEAT

NAME Background noise during playback ENAME
GOODNO
BADYES
GOODNO
EFEAT

NAME Mid-track resume ENAME
YES
NO
NO
EFEAT

NAME Mid-playlist resume ENAME
YES
NO
UNKNOWN
EFEAT

NAME Resumed playlist order ENAME
YES
NO
UNKNOWN
EFEAT

NAME Battery lifetime ENAME
TD Longer ETD
TD Long ETD
TD Long ETD
EFEAT

NAME Battery time indicator ENAME
YES
NO
NO
EFEAT

NAME Customizable font (Recorder) ENAME
YES
NO
NO
EFEAT

NAME Customizable screen info when playing songs ENAME
YES
NO
NO
EFEAT

NAME USB attach/detach without reboot ENAME
YES
NO
YES
EFEAT

NAME Can load another firmware without rebooting ENAME
YES
NO
NO
EFEAT

NAME Playlist load speed, songs/sec ENAME
TD 3000 - 4000 ETD
TD 15 - 20 ETD
UNKNOWN
EFEAT

NAME Max number of songs in a playlist ENAME
TD 20 000 ETD
TD 999 ETD
UNKNOWN
EFEAT

NAME Supports bad path prefixes in playlists ENAME
YES
YES
UNKNOWN
EFEAT

NAME Open source/development process ENAME
YES
NO
NO
EFEAT

NAME Corrects reported bugs ENAME
YES
NO
NO
EFEAT

NAME Automatic Volume Control (Recorder) ENAME
YES
NO
NO
EFEAT

NAME Pitch control (Recorder) ENAME
YES
NO
NO
EFEAT

NAME Text File Reader ENAME
YES
YES
NO
EFEAT

NAME Games (Recorder) ENAME
TD 8 ETD
NO
NO
EFEAT

NAME Games (Player) ENAME
TD 2 ETD
NO
NA
EFEAT

NAME File Delete & Rename ENAME
YES
YES
NO
EFEAT

NAME Playlist Building ENAME
YES
YES
NO
EFEAT

NAME Recording (Recorder) ENAME
YES
YES
YES
EFEAT

NAME Generates XING VBR header when recording ENAME
YES
YES
UNKNOWN
EFEAT

NAME High Resolution Volume Control ENAME
YES
NO
YES
EFEAT

NAME Deep discharge option (Recorder) ENAME
YES
NO
NO
EFEAT

NAME Customizable backlight timeout ENAME
YES
YES
YES
EFEAT

NAME Backlight-on when charging option ENAME
YES
NO
NO
EFEAT

NAME Queue function ENAME
YES
YES
NO
EFEAT

NAME Supports the XING header ENAME
YES
YES
UNKNOWN
EFEAT

NAME Supports the VBRI header ENAME
PARTLY
YES
UNKNOWN
EFEAT

NAME Max number of files in a dir ENAME
TD 10 000 ETD
TD 999 ETD
UNKNOWN
EFEAT

NAME Adjustable scroll speed ENAME
YES
NO
YES
EFEAT

NAME Screensaver style demos (Recorder) ENAME
YES
NO
NO
EFEAT

NAME Variable step / accelerating ffwd and rwd ENAME
YES
NO
NO
EFEAT

NAME Visual Progress Bar ENAME
YES
NO
YES
EFEAT

NAME Select/Load configs ENAME
YES
NO
NO
EFEAT

NAME Sleep timer ENAME
YES
NO
NO
EFEAT

NAME Easy User Interface ENAME
YES
NO
NO
EFEAT

NAME Remote Control Controllable ENAME
YES
YES
YES
EFEAT

NAME ISO8859-1 font support (Player) ENAME
YES
NO
NA
EFEAT

NAME Queue songs to play next ENAME
YES
YES
UNKNOWN
EFEAT

NAME Bookmark positions in songs ENAME
YES
NO
NO
EFEAT

NAME Number of available languages ENAME
TD 24 ETD
TD 3 ETD
UNKNOWN
EFEAT

NAME Accurate VBR bitrate display ENAME
YES
NO
UNKNOWN
EFEAT

NAME FM Tuner support (FM Recorder) ENAME
YES
YES
YES
EFEAT

NAME FF/FR with sound ENAME
NO
YES
UNKNOWN
EFEAT

NAME Pre-Recording (Recorders) ENAME
YES
YES
UNKNOWN
EFEAT

NAME Video Playback with sound (Recorders) ENAME
YES
NO
NO
EFEAT

NAME Boot Time from Flash (in seconds) ENAME
TD 4 ETD
TD 12 ETD
UNKNOWN
EFEAT

NAME Speaking Menus Support ENAME
YES
NO
NO
EFEAT

</table>

<p>
 Wrong facts? Mail rockbox@cool.haxx.se now!

#include "foot.t"
