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

#include "foot.t"
