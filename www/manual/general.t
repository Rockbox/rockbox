#define _PAGE_ Manual - General Settings
#include "head.t"
#include "manual.t"

<h2>Playback</h2>
 <ul>
 <li><b>Shuffle</b> - Select shuffle ON/OFF. This alters how Rockbox will
  select which next song to play.

 <li><b>Repeat</b> - Repeat modes are Off/One/All. 
 "Off" means no repeat. "One" means repeat one track over and over. 
 "All" means repeat playlist/directory.

 <li><b>Play selected first</b> - This setting controls what happens when you
 press PLAY on a file in a directory and shuffle mode is on.
 If this setting is Yes, the file you selected will be played first.
 If this setting is No, a random file in the directory will be played first.

 <li><b>Resume</b> - Sets whether Rockbox will resume playing at the point where you shut off. Options are: Ask/Yes/No.
 "Ask" means it will ask at boot time.
 "Yes" means it will unconditionally try to resume.
 "No" means it will not resume.

 <li><b>FF/RW Min Step</b> - The smallest step you want to fast forward or rewind in a track.

 <li><b>FF/RW Accel</b> - How fast you want search (ffwd/rew) to accellerate when you hold down the button. "Off" means no accelleration. "2x/1s" means double the search speed once every second the button is held. "2x/5s" means double the search speed once every 5 seconds the button is held.
 </ul>

<h2>File View</h2>
 <ul>
 <li><b>Sort mode</b> - How directories are sorted.
 Case sensitivity ON makes uppercase and lowercase differences matter.
 Having it OFF makes them get treated the same.

 <li><b>Show files</b> - Controls which files are displayed in the dir browser:
  <ul>
  <li><b>Music</b>: Only directories, .mp3, .mp2, .mpa and .m3u files are shown. Extensions are stripped. Files and directories starting with . or has the "hidden" flag set are hidden.

  <li><b>Supported</b>: All directories and files Rockbox can load (including .fnt, .wps, .cfg, .txt, .ajz/.mod) are shown. Extensions are shown. Files and directories starting with . or has the "hidden" flag set are hidden.

  <li><b>All</b>: All files and directories are shown. Extensions are shown. No files or dirs are hidden.
  </ul>

 <li><b>Follow Playlist</b> - Do you want the dir browser to follow your playlist? If Follow Playlist is set to "Yes", you will find yourself in the same directory as the currently playing file if you go to the Dir Browser from the WPS. If set to "No", you will stay in the same directory as you last were in.
 </ul>

<h2>Display</h2>
<ul>
<li><b>Scroll speed</b> - Controls the speed of scrolling text.

<li><b>Backlight timer</b> - How long time the backlight shines after a keypress. Set to OFF to never light it, set to ON to never shut it off or set a prefered timeout period.

<li><b>Backlight on when plugged</b> - Do you want the backlight to be constantly on while the charger cable is connected?

<li><b>Contrast</b> - Changes the contrast of your LCD display. 

<li><b>Peak meter</b> (Recorder only)
 <ul>
 <li><b>Peak release</b>: How fast should the peak meter shrink after a peak?
 <li><b>Peak hold</b>: How long should the peak meter hold before shrinking?
 <li><b>Clip hold</b>: How long should the clipping indicator be visible after clipping was detected?
 </ul>
</ul>

<h2>System</h2>
<ul>
<li><b>Disk Spindown</b> - Rockbox has a timer that makes it spin down the
harddisk after being idle for a certain time. You can modify this timeout here.

<li><b>Deep discharge</b> (Recorder only) -
 Set this to ON if you intend to keep your charger
connected for a long period of time. It lets the batteries go down to 10%
before starting to charge again. Setting this to OFF will cause the charging
to restart on 95%.

<li><b>Set Time/Date</b> (Recorder only) - Set current time and date.

<li><b>Idle poweroff</b> - After how long period of idle time should the unit power off?

<li><b>Reset settings</b> - Reset all settings to default values. Some settings may need a reboot for the reset to take effect.

</ul>

#include "settings-buttons.t"

#include "foot.t"

