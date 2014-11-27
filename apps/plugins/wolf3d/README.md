Chocolate-Wolfenstein-3D
========================

Based on Wolf4SDL by Moritz "Ripper" Kroll (http://www.chaos-software.de.vu).

Original Wolfenstein 3D by id Software (http://www.idsoftware.com)

Chocolate Wolf3D removes all the crap that was added over the years 
(snow, rain ...) in order to recreate the experience from 1993. 

All other port display the framebuffer as 320x200 without accounting for the CRT 4:3
distortion. Chocolate Wolfenstien 3D has a CRT emulator based on OpenGL:

Direct framebuffer to window (resulting in compressed image):

![alt tag](https://github.com/fabiensanglard/Chocolate-Wolfenstein-3D/blob/master/screenshots/crt_framebuffer.png)

CRT 4:3 aspect ratio emulated to match what gamer saw on their screen in 1993. Image is streched to 320x240 just
like the CRT did :

![alt tag](https://github.com/fabiensanglard/Chocolate-Wolfenstein-3D/blob/master/screenshots/crt_aspect.png)

Tested on Windows, Linux and Mac OS X

Step by Step Mac OS X setup here: http://clubctrl.com/org/prog/wolf.html

Fabien Sanglard 
