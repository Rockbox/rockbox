#define _PAGE_ Repair your JBR V1 battery connectors
#include "head.t"

<p>
 This guide will show you how to repair the battery connectors on your
 Jukebox Recorder V1.

<h2>Symptoms</h2>
A loose battery connector can give all kinds of weird behaviour:

<ul>
 <li>Drastically lowered battery runtime
 <li>It shuts down or reboots when you squeeze the bumpers
 <li>It refuses to start up, saying something like this:
     <pre>
      HD register error
      SC1 (85) 128
      SN1 (170) 128
      SC2 (170) 128
      SN2 (85) 128
     </pre>
</ul>

<h2>Performing the surgery</h2>
First you open up your recorder, this is described
<a href=http://rockbox.haxx.se/mods/disassemble.html>here</a>.
<p>
This picture shows you the two solder joints that most often are broken.

<p>
<a href=solderjoints.jpg><img border=0 src=solderjoints_t.jpg></a>

<p>
 Now you fire up your soldering iron and resolder the joints. Make sure that the PCB really is connected to the metal housing.

<p>
<a href=solderjoints2.jpg><img border=0 src=solderjoints2_t.jpg></a>

<p>
Once you have resoldered all joints, reassemble the archos and start it up.
The reassembly is described
<a href=http://rockbox.haxx.se/mods/reassemble.html>here</a>.

<p>
Good luck!

<p>
<i>Linus</i>

#include "foot.t"
