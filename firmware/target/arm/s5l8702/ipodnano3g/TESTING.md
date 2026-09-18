# iPod Nano 3G testing notes

Evidence for the driver, FTL, storage and hardware support in this
directory.

## NAND driver, FTL and storage

Since the earlier host and hardware pass: `test_disk` passes at every
buffer alignment - it's what found and confirmed the fix for a DMA
alignment bug (a misaligned buffer returned wrong data and wrote past
itself). The storage thread holds the FTL for at most 492 ms during a
merge, measured with a timing build - within the 3 s PCM buffer, so no
audio starvation.

The 8GB Hynix A555D5AD row's evidence is Robin Tschirschnitz's (delacor):
measured and run on their own unit, not this one.

## NAND check image

Tested on the 4GB unit from DFU: host sees "Nano 3G NAND", 2113537 x
2048-byte sectors, write protect on. `nandcheck.py collect` reads it in
seven minutes on Linux and Windows, no ECC failures or timeouts, and the
archive passes the host FTL suite.

## Power, RTC, backlight, battery, audio

Tested on the 4GB unit: 44.1 kHz WAV plays with elapsed time running;
battery reads 4077 mV, charging true/false correctly with a cell below
full; a time set in Rockbox survives a reboot; backlight and brightness
work; "pause on headphone unplug" pauses on disconnect.

Not tested: sample rates other than 44.1 kHz, and the charge-complete
status bits (never seen set).
