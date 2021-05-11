# jztool -- Ingenic device utility & bootloader installer

The `jztool` utility can help install, backup, and restore the bootloader on
Rockbox players based on a supported Ingenic SoC.

## FiiO M3K

First, get a copy of the `bootloader.m3k` file, either by downloading it
from <https://rockbox.org>, or by compiling it yourself (choose 'B'ootloader
build when configuring your build).

The first time you install Rockbox, you need to load the Rockbox bootloader
over USB by entering USB boot mode. The easiest way to do this is by plugging
in the microUSB cable to the M3K and holding the VOL- button while plugging
the USB into your computer. If you entered USB boot mode, the button light
will turn on but the LCD will remain black.

Copy the `bootloader.m3k` next to the `jztool` executable and follow the
instructions below which are appropriate to your OS.

### Running jztool

#### Linux/Mac

Run the following command in a terminal. Note that on Linux, you will need to
have root access to allow libusb to access the USB device.

```sh
# Linux / Mac
# NOTE: root permissions are required on Linux to access the USB device
#       eg. with 'sudo' or 'su -c' depending on your distro.
$ ./jztool fiiom3k load bootloader.m3k
```

#### Windows

To allow `jztool` access to the M3K in USB boot mode, you need to install
the WinUSB driver. The recommended way to install it is using Zadig, which
may be downloaded from its homepage <https://zadig.akeo.ie>. Please note
this is 3rd party software not maintained or supported by Rockbox developers.
(Zadig will require administrator access on the machine you are using.)

When running Zadig you must select the WinUSB driver; the other driver options
will not work properly with `jztool`. You will have to select the correct USB
device in Zadig -- the name and USB IDs of the M3K in USB boot mode are listed
below. NOTE: the device name may show only as "X" and a hollow square in Zadig.
The IDs will not change, so those are the most reliable way to confirm you have
selected the correct device.

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

### Further instructions

After running `jztool` successfully your M3K will display the recovery menu
of the Rockbox bootloader. If you want to permanently install Rockbox to your
M3K, copy `bootloader.m3k` to the root of an SD card, insert it to your device,
then choose "Install/update bootloader" from the menu.

It is _highly_ recommended that you take a backup of your existing bootloader
in case of any trouble -- choose "Backup bootloader" from the recovery menu.
The backup file is called "fiiom3k-boot.bin" and will be saved to the root of
the SD card. If you need to restore it, simply place the file at the root of
your SD card and select "Restore bootloader".

In the future if you want to backup, restore, or update the bootloader, you
can access the Rockbox bootloader's recovery menu by holding VOL+ when booting.

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
