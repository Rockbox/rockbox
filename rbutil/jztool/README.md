# jztool -- Ingenic device utility & bootloader installer

The `jztool` utility can help install, backup, and restore the bootloader on
Rockbox players based on a supported Ingenic SoC (currently only the X1000).

## Running jztool

### Getting a bootloader

To use `jztool` you need to compile or download a bootloader for your player.
It's recommended to use only official released bootloaders, since bootloaders
compiled from Git are not tested and might be buggy.

You can download released bootloaders from <https://download.rockbox.org/>.

The bootloader file is named after the target: for example, the FiiO M3K
bootloader is called `bootloader.m3k`. The FiiO M3K is used as an example
here, but the instructions apply to all X1000-based players.

Use `jztool --help` to find out the model name of your player.

### Entering USB boot mode

USB boot mode is a low-level mode provided by the CPU which allows a computer
to load firmware onto the device. You need to put your player into this mode
manually before using `jztool` (unfortunately, it can't be done automatically.)

To connect the player in USB boot mode, follow these steps:

1. Ensure the player is fully powered off.
2. Plug one end of the USB cable into your player.
3. Hold down your player's USB boot key (see below).
4. Plug the other end of the USB cable into your computer.
5. Let go of the USB boot key.

The USB boot key depends on your player:

- FiiO M3K: Volume Down
- Shanling Q1: Play
- Eros Q: Menu

### Linux/Mac

Run the following command in a terminal. Note that on Linux, you will need to
have root access to allow libusb to access the USB device.

```sh
# Linux / Mac
# NOTE: root permissions are required on Linux to access the USB device
#       eg. with 'sudo' or 'su -c' depending on your distro.
$ ./jztool fiiom3k load bootloader.m3k
```

### Windows

To allow `jztool` access to your player in USB boot mode, you need to install
the WinUSB driver. The recommended way to install it is using Zadig, which
may be downloaded from its homepage <https://zadig.akeo.ie>. Please note
this is 3rd party software not maintained or supported by Rockbox developers.
(Zadig will require administrator access on the machine you are using.)

When running Zadig you must select the WinUSB driver; the other driver options
will not work properly with `jztool`. You will have to select the correct USB
device in Zadig. All X1000-based players use the same USB ID while in USB boot
mode, listed below. NOTE: the device name may show only as "X" and a hollow
square in Zadig. The IDs will not change, so those are the most reliable way
to confirm you have selected the correct device.

```
Name:   Ingenic Semiconductor Co.,Ltd X1000
USB ID: A108 1000
```

Assuming you installed the WinUSB driver successfully, open a command prompt
in the folder containing `jztool`. Administrator access is not required for
this step.

Type the following command to load the Rockbox bootloader:

```sh
# Windows
$ jztool.exe fiiom3k load bootloader.m3k
```

## Using the recovery menu

If `jztool` runs successfully your player will display the Rockbox bootloader's
recovery menu. If you want to permanently install Rockbox to your device, copy
the bootloader file you downloaded to the root of your SD card, insert the SD
card to your player, and choose "Install/update bootloader" from the menu.

It is _highly_ recommended that you take a backup of your existing bootloader
in case of any trouble -- choose "Backup bootloader" from the recovery menu.
The backup file is called `PLAYER-boot.bin`, where `PLAYER` is the model name.
(Example: `fiiom3k-boot.bin`.)

You can restore the backup later by putting it on the root of your SD card and
selecting "Restor bootloader" in the recovery menu.

After installing the Rockbox bootloader, you can access the recovery menu by
holding a key while booting:

- FiiO M3K: Volume Up
- Shanling Q1: Next (button on the lower left)
- Eros Q: Volume Up

### Known issues

- When using the bootloader's USB mode, you may get stuck on "Waiting for USB"
  even though the cable is already plugged in. If this occurs, unplug the USB
  cable and plug it back in to trigger the connection.


## TODO list

### Add better documentation and logging

There's only a bare minimum of documentation, and logging is sparse, not
really enough to debug problems.

Some of the error messages could be friendlier too.

### Integration with the Rockbox utility

Adding support to the Rockbox utility should be mostly boilerplate since the
jztool library wraps all the troublesome details.

Permissions are an issue on Linux because by default only root can access
"raw" USB devices. If we want to package rbutil for distro we can install
a udev rule to allow access to the specific USB IDs we need, eg. allowing
users in the "wheel" group to access the device.

On Windows and Mac, no special permissions are needed to access USB devices
assuming the drivers are set up. (Zadig does require administrator access
to run, but that's external to the Rockbox utility.)
