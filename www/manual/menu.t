#define _PAGE_ Manual - Main Menu
#include "head.t"
#include "manual.t"
<p>
<img src="rec-menu.png" width=120 height=68 alt="Main menu">
<br><small>Recorder screenshot</small>

<p>The main menu offers:
<ul>
<li> <b><a href="sound.html">Sound Settings</a></b> - Vol, bass, treble etc.
<li> <b><a href="general.html">General Settings</a></b> - Scroll, display, filters etc.

<li> <b>Games</b> - (Recorder only) Pick a game to play! Tetris, Sokoban and <a href=wormlet.html>Wormlet</a> are available.

<li> <b>Demos</b> - (Recorder only) Some silly little toys. <i>Bounce</i> will show a bouncing text (try the buttons), <i>Snow</i> will simulate falling snow and <i>Oscillograph</i> will give you a nice animated graph of the currently playing music (try the buttons here too).

<li> <b>Info</b> - Shows MP3 ram buffer size and battery voltage level info.

<li> <b>Version</b> - Software version and credits display.

<li> <b>Debug (keep out)</b> - Various informational displays for development purposes.
</ul>

#include "menu-buttons.t"

#include "foot.t"
