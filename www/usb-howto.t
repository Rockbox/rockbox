                   How To Connect your Archos to Your Linux
                   ========================================

Author:  Daniel Stenberg <daniel@haxx.se>
Version: 0.2
Date:    April 23, 2002

 Archos Recorder

  The Recorder does not need Björn's ISD200 driver, that was written for and
  is required for Linux to communicate with the Archos Player (and others).

  The Recorder supports both USB1.1 and USB2.0, and thus you can use either
  version, depending on what your host supports.

  CONFIGURE YOUR KERNEL

  (I've tried this using both 2.4.17 and 2.4.18)

  o Make sure your kernel is configured with SCSI, USB and USB mass storage
    support.

  USB1.1 ONLY

    o On USB config page, select 'UHCI' as a (m)odule, as only then will the
      "Alternate Driver" appear in the config. Set that one to (m)odule as
      well.  Failing to do this might cause you problems. It sure gave me
      some.

  USB2.0 ONLY

    o Make sure you've patched your kernel with the correct USB2 patches:
      [the following is a single URL, split here to look nicer]
      http://sourceforge.net/tracker/index.php?func=detail&aid=503534& \
      group_id=3581&atid=303581

    o On USB config page, select 'EHCI' as a (m)odule

  o Rebuild kernel, install, bla bla, reboot the new one



  MAKE YOUR KERNEL SEE YOUR ARCHOS

  After having booted your shiny new USB+SCSI kernel, do this:

  o Very important *first* start your Archos Recorder, and get it connected to
    the USB. Not starting your Archos first might lead to spurious errors.

  USB 1.1 ONLY

    o insmod usb-uhci
  
  USB 2.0 ONLY

    o insmod ehci-hcd
 
  o insmod usb-storage

  Now, your Archos Recorder might appear something like this:

  $ cat /proc/scsi/scsi
  Attached devices:
  Host: scsi0 Channel: 00 Id: 00 Lun: 00
    Vendor: FUJITSU  Model: MHN2200AT        Rev: 7256
    Type:   Direct-Access                    ANSI SCSI revision: 02

  And you can also see it as an identified device by checking out the file
  /proc/bus/usb/devices.



  MOUNT THE ARCHOS' FILESYSTEM

  In my system, my kernel tells me a 'sda1' SCSI device appears. Using this
  info, I proceed to mount the filesystem of my Archos on my Linux:

  $ mount -f vfat -oumask=0 /dev/sda1 /mnt/archos

  (/dev/sda1 may of course not be exactly this name on your machine)

  You can also make the mount command easier by appending a line to /etc/fstab
  that looks like:

  /dev/sda1   /mnt/archos   vfat  noauto,umask=0  0 0

  So then the mount command can be made as simple as this instead:

  $ mount /mnt/archos

  The umask stuff makes it possible for all users to write and delete files on
  the archos file system, not only root. The 'noauto' prevents the startup
  sequence to attempt to mount this file system.
