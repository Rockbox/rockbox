#define _PAGE_ RVF Conversions and Similar
#include "head.t"

<h2>Introduction</h2>
This is a simple tutorial (or, at least, as simply put as possible) on how
to convert your video files to RVF (Rockbox Video File), to be played on
the Archos Recorder / FM Recorder / V2 line.

<p> See also Fabian Merki's <a
 href="http://merkisoft.ch/rockbox/">msi-rvf-gallery</a>, a Java program for
 building RVF movies out of individual JPEGs.

<h3>How To Convert AVI to RVF</h3>
<p>
 <b><big>This Process Is For Windows Users Only</big></b>
<ol>

<li> Convert your movie file to an AVI file, uncompressed, and with the size:
   112x64. There are quite a few programs out there that will do this for you,
   so I will leave this step up to you. One such program is "BPS Video
   Converter" available online. Use google if you need.

<li> Download the tools required here:
   <a
href="http://joerg.hohensohn.bei.t-online.de/archos/doom/source.zip">http://joerg.hohensohn.bei.t-online.de/archos/doom/source.zip</a>
   Unzip to a PATH, such as C:\RVF, that is easily remembered.

<li> Press START on your taskbar, choose RUN and type in the box (minus
   quotes): "command" You should now be looking at a command prompt. If you
   don't know basic DOS commands, here is what you need to know: Use 'cd'
   change dir (format: cd <dir>) IE: "cd .." to go UP one, "cd ROCKBOX" to
   enter a path "ROCKBOX"

<li> Navigate to your PATH in DOS prompt, using "cd" as illustrated above.

<li> Run the file, avitoyuv, which was in the packet you earlier downloaded, using the format:
<pre>
	avitoyuv [input.avi] [output.yuv]
</pre>
   For example, if your AVI movie is called "filename" then you'd put in the following:
<pre>
	avitoyuv filename.avi filename.yuv
</pre>

   OPTIONAL: You can name the output file differently, whatever you specify it
   will be called.  INFO: This can take long to convert.

<li> Now run the file, halftone, which was in the packet you earlier downloaded, using the format:
<pre>
	halftone [input.yuv] [output.rvf]
</pre>
   For example, if your YUV output from step 5 is called "filename" then you'd put in the following:
<pre>
	halftone filename.yuv filename.rvf
</pre>

 OPTIONAL: You can name the output differently, again. INFO: When this is
 done, a long list will appear on your DOS screen and you will be back at the
 command prompt again.

<li> Copy the .rvf output to your jukebox, load up a recent daily build and
   plugins, and kick back and watch the movie!

<p>
Video tools, player: Jörg Hohensohn
<p>
Tutorial: Zakk Roberts
#include "foot.t"
