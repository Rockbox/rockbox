# jztool -- Ingenic device utility & bootloader installer

The `jztool` utility can install, backup, and restore the bootloader on
Rockbox players based on a supported Ingenic SoC.

## FiiO M3K

To use `jztool` on the FiiO M3K you have to connect the player to your
computer in USB boot mode.

The easiest way to do this is by plugging in the microUSB cable to the M3K
and holding the volume down button while plugging the USB into your computer.
If you entered USB boot mode, the button light will turn on but the LCD will
turn off.

To install or update the Rockbox bootloader on the M3K, use the command
`jztool fiiom3k install`. It is recommended that you take a backup of your
current bootloader so you can restore it in case of any problems.

After any operation finishes, you will have to force a power off of the M3K
by holding down the power button for at least 10 seconds. This must be done
whether the operation succeeds or fails. Just don't power off or unplug the
device in the middle of an operation -- that might make bad things happen.

See `jztool --help` for info.

## TODO list

### Add better documentation and logging

There's only a bare minimum of documentation, and logging is sparse, not
really enough to debug problems.

Some of the error messages could be friendlier too.

### Integration with the Rockbox utility

Adding support to the Rockbox utility should be mostly boilerplate since the
jztool library wraps all the troublesome details.

Getting appropriate privileges to access the USB device is the main issue.
Preferably, the Rockbox utility should not run as root/admin/etc.

- Windows: not sure
- Linux: needs udev rules or root privileges
- Mac: apparently does not need privileges

### Porting to Windows

Windows wants to see a driver installed before we can access the USB device,
the easiest way to do this is by having the user run Zadig, a 3rd party app
which can install the WinUSB driver. WinUSB itself is from Microsoft and
bundled with Windows.

Zadig's homepage: https://zadig.akeo.ie/

### Porting to Mac

According to the libusb wiki, libusb works on Mac without any special setup or
privileges, presumably porting there is easy.
