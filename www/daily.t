#define _PAGE_ Daily builds
#include "head.t"

<h2>Daily builds for different models</h2>

<p>These are automated daily builds of the code in CVS. They contain all the latest features. They may also contain bugs and/or undocumented changes... :-)
The top line is the latest.

<p>
<!--#exec cmd="./dailymod.pl" -->
<p>
<b>mod</b> - The file you should name "archos.mod" before copying it to the root of your archos.<br>
<b>ajz</b> - The file you should name "ajbrec.ajz" before copying it to the root of your archos.<br>
<b>rocks</b> - All plugins for this particular release of Rockbox.<br>
<b>full</b> - Full zip archive, with rockbox, plugins, languages, docs, fonts, ucl etc.<br>
<b>ucl</b> - File to use when <a href="/docs/flash.html">flashing Rockbox</a>

<h2>Source tarballs</h2>

<!--#exec cmd="./dailysrc.pl" -->

<p>Please also look at the <a href="docs/">documentation</a> for do-it-yourselfers.

<a name="target_builds"></a>
<a name="daily_builds"></a>

<h2>CVS compile status</h2>

<p>This table shows which targets are currently compilable from the CVS code, and how many compiler warnings the build generates. "OK" means no warnings.
The batch timestamp is GMT.

<p>
<!--#include virtual="buildstatus.link" -->

<a name="bleeding_edge"></a>
<h2>Bleeding edge builds</h2>

<p>These builds are as "bleeding edge" as you can get. They are updated every 20 minutes. (See status on the first line in the above table).

<p><table class=dailymod><tr valign=top>
<td>
<a href="auto/build-player/archos.mod">Player</a>
(<a href="auto/build-player/rocks.zip">rocks</a>)<br>

<a href="auto/build-playerdebug/archos.mod">Player debug</a>
(<a href="auto/build-playerdebug/rocks.zip">rocks</a>)<br>

<a href="auto/build-playersim/rockboxui">Player simulator (linux)</a>
(<a href="auto/build-playersim/rocks.zip">rocks</a>)<br>
<a href="auto/build-playersimwin32/uisw32.exe">Player simulator (win32)</a>
(<a href="auto/build-playersimwin32/rocks.zip">rocks</a>)<br>
</td>
<td>
<a href="auto/build-recorder/ajbrec.ajz">Recorder</a>
(<a href="auto/build-recorder/rocks.zip">rocks</a>)<br>

<a href="auto/build-recorderdebug/ajbrec.ajz">Recorder debug</a>
(<a href="auto/build-recorderdebug/rocks.zip">rocks</a>)<br>

<a href="auto/build-recordersim/rockboxui">Recorder simulator (linux)</a>
(<a href="auto/build-recordersim/rocks.zip">rocks</a>)<br>
<a href="auto/build-recordersimwin32/uisw32.exe">Recorder simulator (win32)</a>
(<a href="auto/build-recordersimwin32/rocks.zip">rocks</a>)<br>
</td>
<td>
<a href="auto/build-fmrecorder/ajbrec.ajz">FM Recorder</a>
(<a href="auto/build-fmrecorder/rocks.zip">rocks</a>)<br>
<a href="auto/build-recorder8mb/ajbrec.ajz">8MB Recorder</a>
(<a href="auto/build-recorder8mb/rocks.zip">rocks</a>)<br>
</td>
</tr></table>

#include "foot.t"
