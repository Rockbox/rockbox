#define _PAGE_ Unlocking a password protected harddisk
#include "head.t"

<p>During development of the Rockbox firmware, on several occations the harddisk has become locked, i.e. password protected. This results in the Archos displaying:
<pre>
Part. Error
Pls Chck HD
</pre>

<p>We are still not 100% sure why it happened. Theories range from
low-power conditions to accidental chip select failure.
It has also happened for normal users,
using the standard Archos-supplied firmware, although it was more frequent for
us developers.

<p>Note: None of us developers have experienced this problem since march 2002.

<p>We do however know how to unlock the disk:

<h2>Windows/DOS unlock</h2>

<p>Note: This requires taking the Archos apart, which will void your warranty!

<ol>
<li>Grab 
<a href="atapwd.zip">atapwd</a>
(written by
<a href="http://www.upsystems.com.ua/support/alexmina/">Alex Mina</a>)
<li>Create a bootable DOS floppy disk, and put atapwd.exe on it
<li>Remove the harddisk from your Archos and plug it into a laptop (or a standard PC, using a 3.5" =&gt; 2.5" IDE adapter)
<li>Boot from the floppy and run atapwd.exe
<li>Select the locked harddrive and press enter for the menu
<li>For Fujitsu disks: Choose "unlock with user password", then "disable with user password". The password is empty, so just press enter at the prompt.
<li>For Toshiba and Hitachi disks, if the above doesn't work: Choose "unlock with master password", then "disable with master password". The password is all spaces.
<li>Your disk is now unlocked. Shut down the computer and remove the disk.
</ol>

<p>Big thanks to Magnus Andersson for discovering the Fujitsu (lack of) user password!

<p>There is also a program for win32,
<a href="http://www.ws64.com/archos/ArchosUnlock.exe">ArchosUnlock.exe</a>,
that creates a linux boot disk with the below mentioned patched isd200 driver.

<h2>Linux unlock</h2>

<p>For those of us using Linux, we have written 
<a href="mail/archive/rockbox-archive-2002-03/att-0010/01-isd200.diff">an isd200 driver patch for unlocking the disk</a>.
This modified driver will automatically unlock the disk when you connect your Archos via USB, so you don't have to do anything special. Apply the patch to a 2.4.18 linux kernel tree.

<h2>Still locked?</h2>

<p>If the above suggestions don't work, here's some background info about the disk lock feature:

<p>The disk lock is a built-in security feature in the disk. It is part of the ATA specification, and thus not specific to any brand or device.

<p>A disk always has two passwords: A User password and a Master password. Most disks support a Master Password Revision Code, which can tell you if the Master password has been changed, or it it still the factory default. The revision code is word 92 in the IDENTIFY response. A value of 0xFFFE means the Master password is unchanged.

<p>A disk can be locked in two modes: High security mode or Maximum security mode. Bit 8 in word 128 of the IDENTIFY response tell you which mode your disk is in: 0 = High, 1 = Maximum.

<p>In High security mode, you can unlock the disk with either the user or master password, using the "SECURITY UNLOCK DEVICE" ATA command. There is an attempt limit, normally set to 5, after which you must power cycle or hard-reset the disk before you can attempt again.

<p>In Maximum security mode, you <b>cannot</b> unlock the disk! The only way to get the disk back to a usable state is to issue the SECURITY ERASE PREPARE command, immediately followed by SECURITY ERASE UNIT. The SECURITY ERASE UNIT command requires the Master password and will completely erase all data on the disk. The operation is rather slow, expect half an hour or more for big disks. (Word 89 in the IDENTIFY response indicates how long the operation will take.)


#include "foot.t"
