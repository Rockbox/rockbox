#define _PAGE_ RVF Conversions and Similar
#include "head.t"

<h2>Introduction</h2>
This is a simple tutorial (or, at least, as simply put as possible) on how
to convert your video files to RVF (Rockbox Video File), to be played on
the Archos Recorder / FM Recorder / V2 line.

<p> Other option is to get the GUI Video Conversion Tool from John Wunder, which
  can be downloaded from <a href="http://home.ripway.com/2004-2/66978/RockVideoRelease.zip">
  http://home.ripway.com/2004-2/66978/RockVideoRelease.zip</a>. (Windows users only)

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
href="http://joerg.hohensohn.bei.t-online.de/archos/video/">http://joerg.hohensohn.bei.t-online.de/archos/video/</a>
   Unzip to a PATH, such as C:\RVF, that is easily remembered.

<li> Press START on your taskbar, choose RUN and type in the box (minus
   quotes): "command" You should now be looking at a command prompt. If you
   don't know basic DOS commands, here is what you need to know:
<br>
   Use 'cd' to change dir (format: cd [dir]) IE: "cd .." to go UP one, "cd
   ROCKBOX" to enter a path "ROCKBOX"

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

<li> Next step is adding sound to your video file. Run the tool avi2wav using the format:
<pre>
        avi2wav [input.avi] [output.wav]
</pre>
   For example, if your original file is called "filename" then you'd put in the following:
<pre>
        avi2wav filename.avi filename.wav
</pre>

 OPTIONAL: You can name the output differently.
 
<li> The extracted audio file must be in mp3 format, so you have to convert the WAV file into MP3.
     One option to make this is using the LAME codec. You can download the win32 binary from <a
     href="http://mitiok.cjb.net">http://mitiok.cjb.net</a>.
     One format used with LAME (good quality/size) is:
<pre>
         lame --preset standard [input.wav] [output.mp3]
</pre>
   For example, if your audio file is called "filename" then you'd put in the following:
<pre>
         lame --preset standard filename.wav filename.mp3
</pre>

 OPTIONAL: You can name the output differently. Also, you can use other wav to mp3 tool, or even use
 other options in the lame command. NOTE: The --preset standard will give you a VBR file, so if you want a 
 CBR file, just change the preset to --preset cbr [kbps], where [kbps] is the Constant Bit Rate desired.
 
<li> Now we have to merge the sound with the video, so run the rvf_mux tool found
     in the packet you've downloaded, using the format:
<pre>
        rvf_mux [option] [videoinput.rvf] [audioinput.mp3] [output.rvf]
</pre>
   For example, if your video file from step 6 is called "filename.rvf" and the audio file from step 7
   is called "filename.wav" then you'd put in the following:
<pre>
        rvf_mux filename.rvf filename.mp3 filename_av.rvf
</pre>

 NOTE: You can use any name for the output file, but it's recomended that the name is not the same name
 used in the input video file.
 You can change the frames per second of Rockbox playback using the -play_fps [fps] option. The default
 value is 67.0 fps.

<li> Copy the .rvf output to your jukebox, load up a recent daily build and
   plugins, and kick back and watch the movie!
</ol>

<p>
Video tools, player: Jörg Hohensohn
<p>
Tutorial: Zakk Roberts
#include "foot.t"
