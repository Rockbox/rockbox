#define _PAGE_ Daily builds
#include "head.t"

<h2>Daily builds for different models</h2>

<p>These are automated daily builds of the code in CVS. They contain all the
latest features. They may also contain bugs and/or undocumented changes... <a href="/docs/devicechart.html">identify your model</a>

<p>
<!--#exec cmd="./dailymod.pl" -->
<h2>Source tarballs</h2>

<!--#exec cmd="./dailysrc.pl" -->

<p>Please also look at the <a href="/twiki/bin/view/Main/DocsIndex">documentation</a> for do-it-yourselfers.

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

<p><b>Note: These are not complete installation archives.</b> Unless you really know what you are doing, you probably want to download the daily build instead.

<p><table class=dailymod><tr valign=top>
<td>
<a href="auto/build-player/archos.mod">Player</a>
(<a href="auto/build-player/rocks.zip">rocks</a>)<br>

<a href="auto/build-playerdebug/archos.mod">Player debug</a>
(<a href="auto/build-playerdebug/rocks.zip">rocks</a>)<br>

</td>
<td>
<a href="auto/build-recorder/ajbrec.ajz">Recorder</a>
(<a href="auto/build-recorder/rocks.zip">rocks</a>)<br>

<a href="auto/build-recorderdebug/ajbrec.ajz">Recorder debug</a>
(<a href="auto/build-recorderdebug/rocks.zip">rocks</a>)<br>

<a href="auto/build-ondiosp/ajbrec.ajz">Ondio SP</a>
<a href="auto/build-ondiofm/ajbrec.ajz">Ondio FM</a>

</td>
<td>
<a href="auto/build-fmrecorder/ajbrec.ajz">FM Recorder</a>
(<a href="auto/build-fmrecorder/rocks.zip">rocks</a>)<br>
<a href="auto/build-recorderv2/ajbrec.ajz">V2 Recorder</a>
(<a href="auto/build-recorderv2/rocks.zip">rocks</a>)<br>
<a href="auto/build-recorder8mb/ajbrec.ajz">8MB Recorder</a>
(<a href="auto/build-recorder8mb/rocks.zip">rocks</a>)<br>
</td>
</tr></table>

<p>
<a href="/twiki/bin/view/Main/UsingCVS">How to use CVS</a>.

#include "foot.t"
