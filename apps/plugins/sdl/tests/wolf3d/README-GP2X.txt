Wolf4SDL by Moritz "Ripper" Kroll (http://www.chaos-software.de.vu)
Original Wolfenstein 3D by id Software (http://www.idsoftware.com)
GP2X support by Pickle

Source and Windows Binary: http://www.stud.uni-karlsruhe.de/~uvaue/chaos/downloads.html
GP2X Binary: http://archive.gp2x.de/cgi-bin/cfiles.cgi?0,0,0,0,20,2479

SUMMARY:
See main README.txt


GP2X CONTROLS:
Directional:    these are mapped to the arrow keys.
A :             mapped to space, which opens doors
B :             mapped to left shift, which enables running. Also mapped
                to key n, for the NO response in the menu.
X :             mapped to left control, which enables shooting.
Y :             mapped to the number keys, to select weapons. It cycles
                through each weapon in order. Also mapped to key y, for
                the YES responses in the menu.
** NOTE: In "enter text" mode each button sends its letter,
         for example a=a, y=y

Select:         mapped to the escape key
Start:          mapped to the enter key
Select+Start:   mapped to pause

Shoulder Left:  this is mapped in a way to strafe left
Shoulder Right: this is mapped in a way to strafe right
** NOTE: If you press both the left and right shoulder buttons the statusbar
         will be shown in the fullscreen mode described above.

Volume Buttons: raise and lower the volume.

Either Volume Button + Select: show fps
Either Volume Button + Start:  take a screenshot


** NOTE: The directional stick is given precedence over the strafe keys.
         For example if you hold the shoulder right to strafe right and you
         then move the stick right you will stop strafing and turn. If you
         then release the stick you will resume strafing the right.
         (I've tested this and it seems to work fairly well)


INSTALL:
Pick your Wolf4SDL binary and copy the files at the root of the zip to any
folder together with the data files of the according game (e.g. *.WL6 for
Wolfenstein 3D or *.SOD for Spear of Destiny).
The binaries do not restart the GP2X menu application.
If you use GMenu2x, select the wrapper option for your icon.
If you use the GPH menu, you will have to create your own script to restart it.


Compiling from source code:
I used the Code::Blocks dev kit. (http://archive.gp2x.de/cgi-bin/cfiles.cgi?0,0,0,0,14,2295)
You can use the template example. Add all of the source files to the project.
Under build options (pick your GP2X compilier) and under "Compilier Settings"
-> "Defines" add GP2X. Just press the build button.
The Makefile should also work for linux type environments, although I have
not tried it this way. If you use it, the GP2X define should be added to the
Makefile using CFLAGS += -DGP2X.

I also added the compiler flags
"-fmerge-all-constants -ffast-math -funswitch-loops"
which give a good performance increase.
For Code::Blocks put this line in "Compiler Settings" - "Other Options".

PERFORMANCE:
The game runs good at 200 Mhz.
