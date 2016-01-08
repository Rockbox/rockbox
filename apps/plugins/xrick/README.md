xrick
=====

xrick is a clone of [Rick Dangerous](http://en.wikipedia.org/wiki/Rick_Dangerous),
known to run on Linux, Windows, BeOs, Amiga, QNX, all sorts of gaming consoles...

License agreement & legal bable
-------------------------------

* Copyright (C) 1998-2002 BigOrno (bigorno@bigorno.net) (http://www.bigorno.net/xrick/)
* Copyright (C) 2008-2014 Pierluigi Vicinanza (pierluigi DOT vicinanza AT gmail.com)

I (BigOrno) have written the initial [xrick](http://www.bigorno.net/xrick/) code.
However, graphics and maps and sounds are by the authors of the original Rick Dangerous
game, and "Rick Dangerous" remains a trademark of its owner(s) -- maybe
Core Design (who wrote the game) or FireBird (who published it).
As of today, I have not been successful at contacting Core Design.

This makes it a bit difficult to formally release the whole code,
including data for graphics and maps and sounds, under the terms of
licences such as the GNU General Public Licence. So the code is
released "in the spirit" of the GNU GPL. Whatever that means.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.

Building
--------

**Requirements:**

* [CMake](http://www.cmake.org/)
* [SDL](https://www.libsdl.org/download-1.2.php) version 1.2.x
* [zlib](http://www.zlib.net/)

1. *Create a build directory*

  ```
  $ cd xrick-x.x.x
  $ mkdir build
  $ cd build
  ```

2. *Generate your Makefile*

  `$ cmake ../source/xrick/projects/cmake`

3. *Build*

  `$ make`

4. *Install (optional)*

  `$ make install`

Platform specific notes can be found in README.platforms.

Usage
-----

`xrick --help` will tell you all about command-line options.

Controls
--------

- left, right, up (jump) or down (crawl): arrow keys or Z, X, O and K.
- fire: SPACE, end: E, pause: P, exit: ESC.
- use left, right, up, down + fire to poke something with your stick,
  lay a stick of dynamite, or fire a bullet.
- toggle fullscreen: F1 ; zoom in/out: F2, F3.
- mute: F4 ; volume up/down: F5, F6.
- cheat modes, "trainer": F7 ; "never die": F8 ; "expose": F9.

More details at http://www.bigorno.net/xrick/

Release History
---------------

Please see the file called CHANGELOG.md.

Contacts
--------

Report problems or ask questions to:

* _BigOrno_ (bigorno@bigorno.net)
* _Pierluigi Vicinanza_ (pierluigi DOT vicinanza AT gmail.com)
