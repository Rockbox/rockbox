#define _PAGE_ First Look at Rockbox
#include "head.t"

<h2>First Time Guide to Rockbox Development</h2>
<p>
 Welcome to our humble project.
<p>
 In order to get your hands dirty as quickly and smoothly as possible, here
follows our suggest approach!

<h2>Join the Rockbox Community</h2>
<p>
 Mail: We have a very active <a href="/mail/">developers mailing list</a> no
serious Rockbox freak can live without.
<p>
 IRC: There's always a bunch of friendly and helpful people around in the
 <a href="/irc/">IRC channel</a>.

<h2>Setup Your Environment</h2> <p>
 You need a cross-compiler and linker to build the code. Pick one of these:
<ul>
<li>
 Linux (or any other unix-like OS: <a href="/cross-gcc.html">Building the cross compiler</a>. This
describes how to build and install gcc for sh1.
<li>
 Windows: (the recommended way) <a href="http://rockbox.my-vserver.de/win32-sdk.html">Setup a cygwin Rockbox development environment</a> (uses approx 7.5MB)
</ul>

<h2>Get The Source</h2>
<p>
 Get a fresh source to build Rockbox from. We usually recommend you get the
sources fresh from the CVS repo (<a href="/cvs.html">How to use CVS</a>), but
you can also get a <a href="/daily.shtml">daily tarball</a> or even the <a
href="/download/">latest released source package</a>.

<h2>Build Rockbox</h2>
<p>
 Build rockbox using your aquired sources! If you're using Linux or the
suggested cygwin approach, read <a href="how_to_compile.html">How to compile
Rockbox</a>.
<p>
 Also note that we have put a whole lot of effort in writing simulators so
that you can build, run and try code on your host PC before you build and
download your target version. This of course requires a working compiler for
your native system.

<h2>Change Rockbox</h2>
<p>
 Before you change any code, make sure to read the <a href="contributing.html">contributing</a> information if you want to have any hope of having your changes accepted.
<p>
 Now, you fixed any bugs? You added any features? Then <a href="patch.html">make a
patch</a> and head over to the <a
href="http://sourceforge.net/tracker/?group_id=44306&atid=439120">patch-tracker</a>
and submit it. Of course, you can also check the <a href="/bugs.shtml">open
bugreports</a> and jump in and fix one of them (or possibly <a
href="http://sourceforge.net/tracker/?func=add&group_id=44306&atid=439118">submit
a new bug report</a>.

<p>
 Regularly checking the open <a href="/requests.shtml">feature-requests</a>
gives a picture of what people want to see happen and what is left to add...

<p>
 You'll be better off with a sourceforge account for most bugreport and
feature-request work.

#include "foot.t"
