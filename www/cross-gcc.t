#define _PAGE_ Building the SH1 cross compiler
#include "head.t"

<p>
In this example I will assume that you are running Linux with the bash shell.
We will only build the C compiler along with the assembler, linker and stuff.
Note that the procedure is exactly the same if you are running cygwin on Windows.

<h2>Download the source code</h2>
<p>
You will need the following archives:
<ul>
<li>binutils-2.11.tar.gz (find it at your closest GNU FTP site)
<li>gcc-3.0.4.tar.gz (find it at your closest GNU FTP site)
<li>(optional) gdb-5.1.1.tar.gz (find it at your closest GNU FTP site)
</ul>

<h2>Unpack the archives</h2>
<p>
<pre>
 /home/linus> tar zxf binutils-2.11.tar.gz
 /home/linus> tar zxf gcc-3.0.3.tar.gz
 /home/linus> tar zxf gdb-5.1.1.tar.gz
</pre>

<h2>Create the directory tree</h2>
<p>
<pre>
 /home/linus> mkdir build
 /home/linus> cd build
 /home/linus/build> mkdir binutils
 /home/linus/build> mkdir gcc
 /home/linus/build> mkdir gdb
</pre>

<h2>Choose location</h2>
<p>
Now is the time to decide where you want the tools to be installed. This is
the directory where all binaries, libraries, man pages and stuff end up when
you do "make install".
<p>
In this example I have chosen "/home/linus/sh1" as my installation directory, or <i>prefix</i> as it is called. Feel free to use any prefix, like
/usr/local/sh1 for example.

<h2>Build binutils</h2>
<p>
We will start with building the binutils (the assembler, linker and stuff).
This is pretty straightforward. We will be installing the whole tool chain
in the /home/linus/sh1 directory.
<pre>
 /home/linus> cd build/binutils
 /home/linus/build/binutils> ../../binutils-2.11/configure --target=sh-elf --prefix=/home/linus/sh1
 /home/linus/build/binutils> make
 /home/linus/build/binutils> make install
</pre>

<h2>Build GCC</h2>
<p>
Now you are ready to build GCC. To do this, you must have the newly built
binutils in the PATH.
<pre>
 /home/linus> export PATH=/home/linus/sh1/bin:$PATH
 /home/linus> cd build/gcc
 /home/linus/gcc> ../../gcc-3.0.3/configure --target=sh-elf --prefix=/home/linus/sh1 --enable-languages=c
 /home/linus/build/gcc> make
 /home/linus/build/gcc> make install
</pre>

<h2>Build GDB</h2>
<p>
If you are planning to debug your code with GDB, you have to build it as well.
<pre>
 /home/linus> export PATH=/home/linus/sh1/bin:$PATH
 /home/linus> cd build/gdb
 /home/linus/gdb> ../../gdb-5.1.1/configure --target=sh-elf --prefix=/home/linus/sh1
 /home/linus/build/gdb> make
 /home/linus/build/gdb> make install
</pre>

<h2>Done</h2>
<p>
If someone up there likes you, you now have a working tool chain for SH1.
To compile a file with gcc:
<pre>
 /home/linus> sh-elf-gcc -c main.o main.c
</pre>
Good luck!
<p>
<i>Linus</i>

#include "foot.t"
