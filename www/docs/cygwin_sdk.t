#define _PAGE_ Setting up a cygwin Rockbox development environment
#include "head.t"

<p>
 Eric Lassauge has kindly compiled the SH-1 cross GCC tools as Cygwin packages
 and put them up for us to use.

<h2>Step 1: Download the cygwin installer</h2>
<p>
 Go to the cygwin home page, <a href=http://www.cygwin.com>http://www.cygwin.com</a>
 and download setup.exe from there ("Install or update now!").

<h2>Step 2: Install the base development environment</h2>
<p>
 If you are unsure about the questions asked by the installer, just choose
 the default.
<p>
 When you are asked to select the packages to install, select the following,
 except for the Base, which is required:

 <ul>
  <li>Devel - binutils</li>
  <li>Devel - cvs</li>
  <li>Devel - gcc</li>
  <li>Devel - gcc-mingw-core</li>
  <li>Devel - gdb (if you want to debug simulator code)</li>
  <li>Devel - gcc-mingw-runtime</li>
  <li>Devel - make</li>
  <li>Devel - patchutils</li>
  <li>Interpreters - perl</li>
 </ul>

<p>
 If you have CVS commit access, you also want to install SSH:

 <ul>
  <li>Net - openssh</li>
 </ul>

<h2>Step 3: Select Eric's mirror site and install</h2>
<p>
 Start the Setup program again. When the installer prompts you for a mirror
 site URL, enter <b>http://lassauge.free.fr/cygwin</b> in the "User URL" field.

<p>
 Then select your packages and install:

 <ul>
  <li>Devel - sh-binutils</li>
  <li>Devel - sh-gcc</li>
 </ul>


#include "foot.t"
