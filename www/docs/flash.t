#define _PAGE_ Rockbox in Flash - FAQ and User Manual
#include "head.t"
<p>
by Jörg Hohensohn aka [IDC]Dragon
<p>
1. What is this about?<br>
2. How is it working?<br>
3. Is it dangerous?<br>
4. Will it work for me?<br>
5. How do I flash the firmware?<br>
6. How do I bring in a current / my personal build of Rockbox?<br>
7. Known issues, limitations<br>
8. Movies and images<br>

<h2>1. What is this about?</h2>
<p>
Flashing in the sense used here and elsewhere in regard to Rockbox means
reprogramming the flash memory of the Archos unit.  Flash memory (sometimes
called "Flash ROM") is a type of nonvolatile memory that can be erased and 
reprogrammed in circuit. It is a variation of electrically erasable 
programmable read-only memory (EEPROM).
<p>
 When you bought Your Archos, it came with the Archos firmware flashed. Now,
you can replace the built-in software with Rockbox.
<p>
Some terminology I'm gonna use in the following:<br>
<b>Firmware</b> means the flash ROM content as a whole.<br>
<b>Image</b> means one operating software started from there.
<p>
By reprogramming the firmware we can bot much faster. Archos has a pathetic
boot loader, versus the boot time for Rrockbox is much faster than the disk
spinup, in fact it has to wait for the disk. Your boot time will be as quick as
a disk spinup. In my case, that's 4 seconds from powerup until resuming
playback.

<h2>2. How is it working?</h2>
<p>
The replaced firmware will host a bootloader and 2 images. I use data
compression to make this possible. The first is the "permanent" backup, not to
be changed any more. The second is the default one to be started, the first is
only used when you hold the F1 key during start. Like supplied here, the first
image is the original Archos firmware, the second is a current build of
Rockbox. This second image is meant to be reprogrammed, it can contain anything
you like, if you prefer, you can program the Archos firmware to there, too.
<p>
I supply two programming tools:
<ul>
<li> The first one is called "firmware_flash.rock" and is used to program the
whole flash with a new content. You can also use it to revert back to the
original firmware you've hopefully backup-ed. In the ideal case, you'll need
this tool only once. You can view this as "formatting" the flash with the
desired image structure.
<li> The second one is called "rockbox_flash.rock" and is used to reprogram only
the second image. It won't touch any other byte, should be safe to fool around
with. If the programmed firmware is inoperational, you can still use the F1
start with the Archos firmware and Rockbox booted from disk to try better.
</ul>
<p>
I will provide more technical details in the future, as well as my non-user
tools. There's an authoring tool which composed the firmware file with the
bootloader and the 2 images, the bootloader project, the plugin sources, and
the tools for the UART boot feature: a monitor program for the box and a PC
tool to drive it. Feel free to review the 
<a href="http://joerg.hohensohn.bei.t-online.de/archos/flash/flash_sourcecode.zip">sources</a> 
 for all of it, but be careful when fooling around with powerful toys!

<h2>3. Is it dangerous?</h2>
<p>
Yes, certainly, like programming a mainboard BIOS, CD/DVD drive firmware,
mobile phone, etc. If the power fails, your chip breaks while programming or
most of all the programming software malfunctions, you'll have a dead box. And
I take no responsibility of any kind, you do that at your own risk. However, I
tried as carefully as possible to bulletproof this code. The new firmware file
is completely read before it starts programming, there are a lot of sanity
checks. If any fails, it will not program. Before releasing this, I have
checked the flow with exactly these files supplied here, starting from the
original firmware in flash. It worked reliably for me, there's no reason why
such low level code should behave different on your box.
<p>
There's one ultimate safety net to bring back boxes with even completely
garbled flash content: the UART boot mod, which in turn requires the serial
mod. It can bring the dead back to life, with that it's possible to reflash
independently from the outside, even if the flash is completely erased. I used
that during development, else Rockbox in flash wouldn't have been possible.
Most of the developing effort went into this tooling. So people skilled to do
these mods don't need to worry. The others may feel unpleasant using the first
tool for reflashing the firmware.
<p>
To comfort you a bit again: The flash tools are stable since quite a while. 
I use them a lot and quite careless meanwhile, even reflashed while playing. 
However, I don't generally recommend that.  ;-)
<p>
About the safety of operation: Since we have dual boot, you're not giving up
the Archos firmware. It's still there when you hold F1 during startup. So even
if Rockbox from flash is not 100% stable for everyone, you can still use the
box, reflash the second image with an updated Rockbox copy, etc.
<p>
The flash chip being used by Archos is specified for 100,000 cycles (in words:
one hundred thousand), so you don't need to worry about that wearing out.

<h2>4. Will it work for me?</h2>
<p>
You need three things:
<ul>
<li> The first is a Recorder or FM model. Be sure you're using the correct
package, Recorder and FM are different! The technology works for
the Player models, too. Players can also be flashed, but Rockbox does not
run cold-started on those, yet.

<li> Second, you need an in-circuit programmable flash. Chances are about 85%
that you have, but Archos also used an older flash chip which can't do the
trick.  You can find out via Rockbox debug menu, entry Hardware Info. If the
flash info gives you question marks, you're out of luck. The only chance then
is to solder in the right chip (SST39VF020), at best with the firmware already
in. If the chip is blank, you'll need the UART boot mod as well.

</ul>

<h2>5. How do I flash the firmware?</h2>
<p>
Short explaination: copy the firmware_*.bin files for your model from my distribution 
to the root directory of your box, then run the "firmware_flash.rock" plugin.
Long version:
<p>

I'm using the new plugin feature to run the flasher code. There's not really a
wrong path to take, however here's a suggested step by step procedure:
<ul>
<li> download the correct package for you model, 
<a href="http://joerg.hohensohn.bei.t-online.de/archos/flash/flash_rec.zip">Recorder</a> 
 or 
<a href="http://joerg.hohensohn.bei.t-online.de/archos/flash/flash_fm.zip">FM</a>
, copy some files of it to your box:
<ol>
  <li> "ajbrec.ajz" into the root directory (the version of Rockbox we're going to use and have in the
firmware file)<br>
  <li> firmware_rec.bin or firmware_fm.bin into the root directory (the complete firmware for your model,
with my bootloader and the two images). There now is also a _norom variant, copy both, 
the plugin will decide which one is required for your box.
  <li> the .rockbox subdirectory with all the plugins for Rockbox<br>
 </ol>
<li> Restart the box so that the new ajbrec.ajz gets started.

<li> Enter the debug menu and select the hardware info screen. Check you flash
IDs (bottom line), and please make a note about your hardware 
mask value (second line). The latter is just for my curiosity, not needed for the
flow. If the flash info shows question marks, you can stop here, sorry.

<li> Backup the current firmware, using the first option of the debug menu
(Dump ROM contents). This creates 2 files in the root directory, which you may
not immediately see in the Rockbox browser. The 256kB-sized
"internal_rom_2000000-203FFFF.bin" one is your present firmware. Back both up 
to your PC.

<li> (optional) While you're in this Rockbox version, I recommend to give it a
test and play around with it, this version is identical to the one about to be
programmed. Make sure that especially USB access and Rolo works. When done,
restart again to have a fresh start and to be back in this Rockbox version.

<li> Use the F2 settings to configure seeing all files within the browser.

<li> Connect the charger and make sure your batteries are also in good
shape. I'm just being paranoid here, it's not that flashing needs more power.

<li> Run the "firmware_flash.rock" plugin. It again tells you about your flash
and the file it's gonna program. After F1 it checks the file. Your hardware
mask value will be kept, it won't overwrite it. Hitting F2 gives you a big
warning.  If I still didn't manage to scare you off, you can hit F3 to
actually program and verify. The programming takes just a few seconds.
If the sanity check fails, you have the wrong kind of boot ROM and are
out of luck by now, sorry.

<li> In the unlikely event that the programming should give you any error,
don't switch off the box! Otherwise you'll have seen it working for the last
time.  While Rockbox is still in DRAM and operational, we could upgrade the
plugin via USB and try again. If you switch it off, it's gone.

<li> Unplug the charger, restart the box and hopefully be in Rockbox straight
away! You may delete "firmware_flash.rock" then, to avoid your little
brother playing with that. Pressing On+Play can do it, or your PC. You
can also delete the ".bin" files.

<li> Try starting again, this time holding F1 while pressing On. It should
boot the Archos firmware, which then loads rockbox from disk. In fact, even
the Archos firmware comes up quicker, because their loader is replaced by
mine.

</ul>

When for any reason you'd like to revert to the original firmware, you can do
like above, but copy and rename your backup to be "firmware_rec.bin" on the
box this time. Keep the Rockbox copy and the plugins of this package for that
job, because that's the one it was tested with.

<h2>6. How do I bring in a current / my personal build of Rockbox?</h2>
<p> Short explaination: very easy, just play a .ucl file like "rockbox.ucl" 
from the download or build. Long version:

<p>
The second image is the working copy, the "rockbox_flash.rock" plugin from this
package reprograms it. The plugins needs to be consistant with the Rockbox
plugin API version, otherwise it will detect mismatch and won't run.

<p> It requires an exotic input, a UCL-compressed image, because
that's my internal format. UCL is a nice open-source compression library I
found and use. The decompression is very fast and less than a page of
C-code. The efficiency is even better than Zip with maximum compression, cooks
it down to about 58% of the original size. For details on UCL, see: <a
href="http://www.oberhumer.com/opensource/ucl/">www.oberhumer.com/opensource/ucl/</a>

<p> Linux users will have to download it from there and compile it, for Win32
and Cygwin I can do that, so the executables are in 
<a href="http://joerg.hohensohn.bei.t-online.de/archos/flash">the packages</a>.
The sample program from that download is called "uclpack". We'll use that to
compress "rockbox.bin" which is the result of the compilation. This is 
a part of the build process meanwhile. If you compile Rockbox yourself,
you should copy uclpack to a directory which is in the path, I recommend 
placing it int the same dir as SH compiler.

<p>
Don't flash any "old" builds which don't have the latest coldstart ability I
brought into cvs these days. They won't boot. These instructions refer to
builds from cvs state 2003-07-10 on.
<p>
Here are the steps:
<ul>
<li> If you start from a .ajz file, you'll need to descramble it first into
"rockbox.bin", by using "descramble ajbrec.ajz rockbox.bin". IMPORTANT: For an
FM, the command is different, use "descramble -fm ajbrec.ajz rockbox.bin"!
Otherwise the image won't be functional. Compress the image using uclpack, 
algorithm 2e (the most efficient, and the only one supported by the 
bootloader), with maximum compression, by typing 
"uclpack --2e --best rockbox.bin rockbox.ucl". You can make a batch file for
this and the above step, if you like.

<li> Normally, you'll simply download or compile rockbox.ucl. Copy it together 
with ajbrec.ajz and all the rocks to the appropriate places, replacing the old.

<li> Just "play" the .ucl file, this will kick off the "rockbox_flash.rock" 
plugin. It's a bit similar to the other one, but I made it different 
to make the user aware. It will check the file, 
available size, etc. With F2 it's being programmed, no need for warning this 
time. If it goes wrong, you'll still have the permanent image. 

<li> It may happen 
that you get an "Incompatible Version" error, if the plugin interface has 
changed meanwhile. You're running an "old" copy of Rockbox, but are trying
to execute a newer plugin, the one you just downloaded. The easiest solution
is to rolo into this new version, by playing the ajbrec.ajz file. Then
you are consistant and can play rockbox.ucl.

<li> When done, you can restart the box and hopefully your new Rockbox image.

</ul>

A more luxurious version of the plugin could do the descrambling and
compression by itself, but that's hard to do because a plugin is very limited
with memory (32kB for code and data). Currently I'm doing one flash sector
(4096 bytes) at a time. Don't know how slow the compression algorithm would be
on the box, that's the strenuous part.
<p>
If you like or have to, you can also flash the Archos image as the second one,
e.g. in case Rockbox from flash doesn't work for you. This way you keep the 
dual bootloader and you can easily try different later. I prepared 
<a href="http://joerg.hohensohn.bei.t-online.de/archos/flash">UCLs</a>
for the latest Recorder and FM firmware.

<h2>7. Known issues, limitations</h2>
<p>
Latest Rockbox now has a charging screen, but it is in an early stage. You'll 
get it when the unit is off and you plug in the charger. The Rockbox charging
algorithm is first measuring the battery voltage for about 40 seconds, after
that it only starts charging when the capacity is below 85%. 
You can use the Archos charging (which always tops off) by holding F1 
while plugging in. Some FM users reported charging problems even with F1,
they had to revert to the original flash content.

<p>
If the plugin API is changed, new builds may render the plugins 
incompatible. When updating, make sure you grab those too, and rolo into
the new version before flashing it.

<p>
There are two variants of how the boxes starts, therefore the normal and the _norom
firmware files. The vast majority of the Player/Recorder/FM all have the same boot 
ROM content, differenciation comes later by flash content. Rockbox identifies this
boot ROM with a CRC value of 0x222F in the hardware info screen. Some recorders 
have the boot ROM disabled (it might be unprogrammed) and start directly from a
flash mirror at address zero. They need the new _norom firmware, it has a slightly
different bootloader. Without a boot ROM there is no UART boot safety net. To 
compensate for that as much as possible I have included my MiniMon monitor, 
it starts with F3+On. Using that the box can be reprogrammed via serial if the 
first ~2000 bytes of the flash are OK.

<h2>8. Movies and images</h2>
<p>
 Jörg's AVI movie (1.5MB) <a href="flash/rockbox_flash_boot.avi">rockbox_flash_boot.avi</a> 
showing his unit booting Rockbox from flash.
<p>
 Roland's screendump from the movie:<br>
<img src="flash/rockbox-flash.jpg" width="352" height="288">

#include "foot.t"
