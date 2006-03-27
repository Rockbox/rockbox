#define _PAGE_ Daily builds
#include "head.t"

<h2>Daily Builds for Different Models</h2>

<p>These are automated daily builds of the code in CVS. They contain all the
latest features. They may also contain bugs and/or undocumented changes... <a
href="/twiki/bin/view/Main/DeviceChart">identify your model</a>

<h2>Daily Build</h2>
<a name="target_builds"></a>
<a name="daily_builds"></a>
<!--#exec cmd="./dailymod.pl" -->
<h2>Daily Changelog</h2>
<!--#exec cmd="./dailysrc.pl" -->

<h2>CVS Compile Status</h2>

#if 1
The CVS builds are now <a href="cvs.shtml">here</a> in its <a href="cvs.shtml">own page</a>.
#else
<p>CVS code build status: number of compiler warnings the build generates. 0
(zero) means no warnings. The timestamp is GMT. <a
href="http://www.rockbox.org/twiki/bin/view/Main/TargetStatus">Target
Status</a>

<p>
<!--#include virtual="buildstatus.link" -->

<a name="bleeding_edge"></a>
<h2>Bleeding edge builds</h2>

<p>These builds are as "bleeding edge" as you can get. They are updated on
every source change. (See status on the first line in the above table).

<p>These are complete installation archives.

<p>

<a href="auto/build-player/rockbox.zip">Player</a>
<a href="auto/build-recorder/rockbox.zip">Recorder</a>
<a href="auto/build-ondiosp/rockbox.zip">Ondio SP</a>
<a href="auto/build-ondiofm/rockbox.zip">Ondio FM</a>
<a href="auto/build-fmrecorder/rockbox.zip">FM Recorder</a>
<a href="auto/build-recorderv2/rockbox.zip">V2 Recorder</a>
<a href="auto/build-recorder8mb/rockbox.zip">8MB Recorder</a>
<a href="auto/build-h100/rockbox.zip">iriver h100</a>
<a href="auto/build-h120/rockbox.zip">iriver h120</a>
<a href="auto/build-h300/rockbox.zip">iriver h300</a>
<a href="auto/build-ipodcolor/rockbox.zip">iPod Color</a>
<a href="auto/build-ipodnano/rockbox.zip">iPod Nano</a>
<a href="auto/build-ipod4gray/rockbox.zip">iPod 4G Gray</a>
<a href="auto/build-ipodvideo/rockbox.zip">iPod Video</a>
<a href="auto/build-source/rockbox-bleeding.tar.gz">source</a>

<p>
<a href="/twiki/bin/view/Main/UsingCVS">How to use CVS</a>.
#endif

#include "foot.t"
