#define _PAGE_ Daily builds
#include "head.t"

<h2>Source tarballs</h2>

<!--#exec cmd="./dailysrc.pl" -->

<p>Useful links for do-it-yourselfers:
<ul>
<li><a href="cross-gcc.html">Building the SH1 cross compiler</a>
<li><a href="sh-win/">Setting up an SH1 cross compiler for Windows</a>
<li><a href="firmware/README">README from the firmware directory</a>,
describing how to compile Rockbox
</ul>

<h2>Target builds</h2>

<p>These are automated daily builds of the CVS code.
They are <i>not</i> official releases, they are in fact almost guaranteed to not work properly! These builds are discussed in IRC only, <b>do not file bug reports for them.</b>

<p>There are three versions of each build:

<!--#exec cmd="./dailymod.pl" -->

<p><b>Note 1:</b> You must rename the file to "archos.mod" (archos.ajz for the recorder) before copying it to the root of your archos.

<p><b>Note 2:</b> The Recorder version does not work yet, due to unfinished drivers. It builds, but does not run.

<h2>Build status</h2>

<p>This table shows which targets are currently buildable with the CVS code.
The timestamp is GMT.

<p>
<!--#include virtual="buildstatus.link" -->
#include "foot.t"
