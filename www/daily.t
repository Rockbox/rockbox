#define _PAGE_ Daily builds
#include "head.t"

<h2>Source tarballs</h2>

<!--#exec cmd="./dailysrc.pl" -->

<p>Useful links for do-it-yourselfers:
<ul>
<li><a href="cross-gcc.html">Building the SH1 cross compiler</a>
<li><a href="sh-win/">Setting up an SH1 cross compiler for Windows</a>
<li><a href="docs/how_to_compile.html">README from the docs directory</a>,
describing how to compile Rockbox
<lI><a href="manual/">The Rockbox Manual</a>
</ul>

<h2>Target builds</h2>

<p>These are automated daily builds of the CVS code.
They are <i>not</i> official releases and are in fact almost guaranteed to contain bugs!

<p><b>New:</b> We now accept bug reports for daily builds too. Just make sure you mark the report with the exact version you were running.

<p>
<!--#exec cmd="./dailymod.pl" -->

<p><b>Note 1:</b> You must rename the file to "archos.mod" ("ajbrec.ajz" for the recorder) before copying it to the root of your archos.


<h2>CVS compile status</h2>

<p>This table shows which targets are currently compilable from the CVS code, and how many compiler warnings the build generates. "OK" means no warnings.
The batch timestamp is GMT.

<p>
<!--#include virtual="buildstatus.link" -->

<h2>Bleeding edge builds</h2>

<p>These builds are as "bleeding edge" as you can get. Up-to-date builds of the latest CVS code (the top line from the above table):

<p><table class=dailymod><tr valign=top>
<td>
<a href="auto/build-player/archos.mod">Player</a><br>
<a href="auto/build-playerdebug/archos.mod">Player debug</a><br>
<a href="auto/build-playersim/rockboxui">Player simulator (linux)</a><br>
<a href="http://codeforce.d2g.com/rockbox/autocvs">Player simulator (win32)</a><br>
</td>
<td>
<a href="auto/build-recorder/ajbrec.ajz">Recorder</a><br>
<a href="auto/build-recorderdebug/ajbrec.ajz">Recorder debug</a><br>
<a href="auto/build-recordersim/rockboxui">Recorder simulator (linux)</a><br>
<a href="http://codeforce.d2g.com/rockbox/autocvs">Recorder simulator (win32)</a><br>
</td>
</tr></table>

#include "foot.t"
