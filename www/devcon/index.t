#define _PAGE_ Rockbox Developer Conference 2002
#include "head.t"

<table align="right"><tr><td><a href="show.cgi?img4083.jpg"><img src="img4083t.jpg" alt="photo" border=0 width=200 height=150></a><br><small><i>Comparison of Recorder and Player</i></small></td></tr></table>

<p>Well, almost. :-) Björn, Linus, Daniel and Kjell sat down at Linus' house
friday night (2002-04-19) with our Archoses and had a long and fruitful discussion about software design.
Here are a few things that we discussed:

<h2>Application Programming Interfaces</h2>

<p>We want to try to stick to POSIX where these exist and are practical. The
reason is simply that many people already know these APIs well. Here are a
few which haven't already been defined in the code:

<h3>File operations</h3>
<ul>
<li>open
<li>close
<li>read
<li>write
<li>seek
<li>unlink
<li>rename
</ul>

<table align="right"><tr><td><a href="show.cgi?img4084.jpg"><img src="img4084t.jpg" alt="photo" border=0 width=200 height=150></a>
<br><small><i>Contest: Spot the development box!</i></small></td></tr></table>

<h3>Directory operations</h3>
<ul>
<li>opendir
<li>closedir
<li>readdir
</ul>

<h3>Disk operations</h3>
<ul>
<li>readblock
<li>writeblock
<li>spindown
<li>diskinfo
<li>partitioninfo
</ul>

<p>We also decided that we will use the 'newlib' standard C library,
replacing some functions with smaller variants as we move forward.

<h2>Multitasking</h2>

<p>We spent much time discussing and debating task scheduling, or the lack
thereof. First, we went with the idea that we don't really need "real"
scheduling. Instead, a simple "tree-task" system would be used: A
main-loop, a timer tick and a "bottom half" low-priority interrupt, each
with an event queue.

<p>Pretty soon we realized that we will want to:

<ol style="a">
<li> Use a timer tick to poll disk I/O (assuming we can't get an interrupt)
<li> Perform slow disk operations in both the MP3->DAC feeder and the user
     interface, sometimes at the same time.
<li> Not lock up the user interface during I/O.
</ol>

<table align="right"><tr><td><a href="show.cgi?img4086.jpg"><img src="img4086t.jpg" alt="photo" border=0 width=200 height=150></a>
<br><small><i>A stack of "virgins"!</i></small></td></tr></table>

<p>At the same time, we agreed that we should not walk into the common trap
of engaging in "job splitting". That is, to split up jobs in small chunks
so they don't take so long to finish. The problem with job splitting is
that it makes the code flow very complex.

<p>After much scratching our collective heads over how to make a primitive
"three-task" system be able to do everything we wanted without resorting
to complex job splitting, we finally came to the conclusion that we were
heading down the wrong road:

<p><blockquote>
 <b>We need threading.</b>
</blockquote>

<p>Even though a scheduler adds complexity, it makes the rest of the code so
much more straight-forward that the total net result is less overall
complexity.

<p>To keep it simple, we decided to use a cooperative scheduler. That is, one
in which the threads themselves decide when scheduling is performed. The
big gain from this, apart from making the scheduler itself less complex,
is that we don't have to worry as much about making all code "multithread
safe".

<p>Affording ourselves the luxury of threads, we soon identified four basic
threads:

<ul>
<li>Disk thread, performing all disk operations
<li>UI thread, handling the user interface
<li>MP3 feed thread, making sure the MAS is fed with data at all times
<li>I2C thread, handling the sometimes very relaxed timing of the I2C bus
</ul>

<p>Threads use message passing between them and each have a message queue
associated to it.

<table align="right"><tr><td><a href="show.cgi?img4089.jpg"><img src="img4089t.jpg" alt="photo" border=0 width=200 height=150></a>
<br><small><i>There's much fun to be had with these things!</i></small></td></tr></table>

<p>In addition to the threads, we need a timer interrupt with the ability to
send messages to threads at specific intervals. This will also be used to
scan the keys of the jukebox and handle key repeat detection (when a key
has been pressed for a number of ticks).

<p>None of these things are, of course, written in stone. Feel free to
comment, discuss and argue about them!

<p>We are currently 89 subscribers to this list. If you want to get more
deeply involved in what's going on, I encourage you to:

<ul>
<li>Subscribe to the rockbox-cvs list, to see all code that goes in.
<li>Join the #rockbox channel on irc.openprojects.net. There are always a
couple of us in there.
</ul>

<p>I have written a set of guidelines for contributing code to the project.
Take a look at them in CVS or here:
<a href="http://bjorn.haxx.se/rockbox/firmware/CONTRIBUTING">CONTRIBUTING</a>

<p>/Björn

#include "foot.t"
