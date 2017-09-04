This file explains how you can compile Wolf4SDL using Bloodshed's Dev-C++.
Keep in mind, that Dev-C++ is a dead project since 2005. The recommended way
to compile Wolf4SDL on Windows is using Visual Studio 2005 C++ or the free
Visual C++ 2005 Express. But for dial-up users Dev-C++ is probably still a
good option.

Needed files:
 - "Dev-C++ 5.0 Beta 9.2 (4.9.9.2)" with Mingw/GCC 3.4.2 (about 9 MB)
   http://www.bloodshed.net/dev/devcpp.html
 - SDL-1.2.13-1chaos.DevPak (544 kB)
   http://www.chaos-software.de.vu -> Downloads
 - SDL_mixer-1.2.6-2mol.DevPak (347 kB)
   http://sourceforge.net/project/showfiles.php?group_id=94270&package_id=151751

Installation:
 - Install Dev-C++ to C:\Dev-Cpp
 - Open Wolf4SDL.dev
 - Go to "Tools" -> "Package Manager"
 - Click on the "Install" button in the toolbar
 - Select "SDL-1.2.13-1chaos.DevPak" (where ever you saved it)
 - Some "Next" buttons and a "Finish" button later...
 - Click on the "Install" button in the toolbar
 - Select "SDL_mixer-1.2.6-2mol.DevPak" (where ever you saved it)
 - Some "Next" buttons and a "Finish" button later...
 - Close the Package Manager

Data file setup:
 - Copy the data files (e.g. *.WL6) you want to use to the Wolf4SDL
   source code folder
 - If you want to use the data files of the full Activision version of
   Wolfenstein 3D v1.4, you can just skip to the next section
 - Otherwise open "version.h" and comment/uncomment the definitions
   according to the description given in this file

Compiling Wolf4SDL:
 - Compile via "Execute" -> "Compile"
 - No errors should be displayed
 - Run Wolf4SDL via "Execute" -> "Run"

Troubleshooting:
 - If you get an error message "undefined reference to `__cpu_features_init'",
   make sure, there is no c:\mingw folder. Otherwise Dev-C++ will mix different
   versions of MinGW libraries...
