#define _PAGE_ Daily builds
#include "head.t"

<h2>Source tarballs</h2>

<!--#exec cmd="./dailysrc.pl" -->

<p>Please also look at our a useful <a href="docs/">documentation</a> for do-it-yourselfers.

<a name="target_builds"></a>
<a name="daily_builds"></a>
<h2>Daily builds for different models</h2>

<p>These are automated daily builds of the CVS code.
They are <i>not</i> official releases and are in fact almost guaranteed to contain bugs!

<p>
<!--#exec cmd="./dailymod.pl" -->

<p><b>Note 1:</b> You must rename the file to "archos.mod" ("ajbrec.ajz" for the recorder) before copying it to the root of your archos.


<h2>CVS compile status</h2>

<p>This table shows which targets are currently compilable from the CVS code, and how many compiler warnings the build generates. "OK" means no warnings.
The batch timestamp is GMT.

<p>
<!--#include virtual="buildstatus.link" -->

<a name="bleeding_edge"></a>
<h2>Bleeding edge builds</h2>

<p>These builds are as "bleeding edge" as you can get. Up-to-date builds of the latest CVS code (the top line from the above table):

<p><table class=dailymod><tr valign=top>
<td>
<a href="auto/build-player/archos.mod">Player</a><br>
<a href="auto/build-playerdebug/archos.mod">Player debug</a><br>
<a href="auto/build-playersim/rockboxui">Player simulator (linux)</a><br>
</td>
<td>
<a href="auto/build-recorder/ajbrec.ajz">Recorder</a><br>
<a href="auto/build-recorderdebug/ajbrec.ajz">Recorder debug</a><br>
<a href="auto/build-recordersim/rockboxui">Recorder simulator (linux)</a><br>
</td>
<td>
<a href="auto/build-fmrecorder/ajbrec.ajz">FM Recorder</a><br>
<a href="auto/build-recorder8mb/ajbrec.ajz">8MB Recorder</a><br>
</td>
</tr></table>

#include "foot.t"
