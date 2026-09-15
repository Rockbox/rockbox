# Nano 3G NAND check

Apple fitted many different NAND chips to the iPod Nano 3G. Rockbox's Nano
3G NAND support is validated chip by chip: it writes only to chips that have
been tested on hardware, and keeps the storage read-only on the rest. This
check collects what we need to validate your chip. It **changes nothing on your
iPod**: nothing is installed, and nothing is written to its NAND. The image
runs from RAM and is gone when the iPod restarts.

## Which chip is in your iPod

Every Nano 3G is model A1236, whichever NAND it has, so the chip has to be
read from the iPod itself:

1. **Capacity** narrows it down: Settings > About in Apple's firmware
   shows 4GB or 8GB, and each row below is one capacity.
2. **The check's screen** names it exactly, within seconds and without
   changing anything: run steps 1 and 2 below and read the `id` line (the
   chip, as in the table) and the `banks` line (how many chip enables
   answered). The `verdict` line says whether Rockbox knows that
   combination and whether it is validated.

If your chip is already validated, there is nothing to collect: restart
the iPod. Otherwise please carry on with steps 3 and 4.

These are all the chips Apple's firmware (1.1.3) supports. Apple picks the
row by the id and the number of chip enables, so one id can appear twice.
Rockbox's NAND driver knows every row, but apart from the validated chip
it has so far been tested only against simulated NAND on a computer, which
is why the archives matter.

| id | maker | capacity | chip enables | page | status |
|---|---|---|---|---|---|
| `A514D3AD` | Hynix | 4GB | 4 | 2KiB | **validated** |
| `A555D5AD` | Hynix | 8GB | 4 | 2KiB | reported, needs a check |
| `B614D5AD` | Hynix | 8GB | 4 | 4KiB | needed |
| `B614D5EC` | Samsung | 4GB | 2 | 4KiB | reported, needs a check |
| `B614D5EC` | Samsung | 8GB | 4 | 4KiB | needed |
| `2555D5EC` | Samsung | 8GB | 4 | 2KiB | needed |
| `A585D598` | Toshiba | 4GB | 2 | 2KiB | needed |
| `A585D598` | Toshiba | 8GB | 4 | 2KiB | needed |
| `BA94D598` | Toshiba | 4GB | 2 | 4KiB | needed |
| `BA94D598` | Toshiba | 8GB | 4 | 4KiB | needed |
| `A5D5D589` | Intel | 4GB | 2 | 2KiB | reported, needs a check |
| `A5D5D589` | Intel | 8GB | 4 | 2KiB | reported, needs a check |
| `3E94D589` | Intel | 4GB | 2 | 4KiB | reported, needs a check |
| `3ED5D789` | Intel | 8GB | 2 | 4KiB | needed |
| `A5D5D52C` | Micron | 4GB | 2 | 2KiB | needed |
| `A5D5D52C` | Micron | 8GB | 4 | 2KiB | needed |
| `3E94D52C` | Micron | 4GB | 2 | 4KiB | needed |
| `3ED5D72C` | Micron | 8GB | 2 | 4KiB | needed |

"Reported" means someone has seen that chip in a unit, but no check archive
has been collected from it yet. The id is the first four READ ID bytes read
as one little-endian number: `A514D3AD` is the bytes `AD D3 14 A5`, and the
last byte is the maker (`AD` Hynix, `EC` Samsung, `98` Toshiba, `89` Intel,
`2C` Micron). When a chip is validated, the driver's chip table and this
table are updated together.

## What you need

- A computer (Linux, macOS or Windows) and a USB cable.
- `mks5lboot`, to send the image to the iPod: build it with
  `make -C utils/mks5lboot` (libusb-1.0 development files on Linux). On
  Windows, see `utils/mks5lboot/README` for the USB driver it needs.
- Python 3.

## 1. Get the image

`nano3g-check.dfu` in this directory is ready to use. It is the Nano 3G
bootloader built with `-DNAND_CHECK` from the Nano 3G NAND driver work,
which is not in Rockbox yet; its build shows on the iPod's screen as
"Version:" and in the report as `version`.

## 2. Run it

1. Put the iPod in DFU mode
2. Send the image:
   ```
   utils/mks5lboot/mks5lboot --dfusend utils/ipodnano3g/nandcheck/nano3g-check.dfu
   ```
3. Within a few seconds the iPod shows "Nano 3G NAND check" and a short
   summary: `battery`, `id` (the chip), `banks`, `row`, `mode`, `pagesize`,
   `validated`, `ftl`, `verdict`, `model` and `swvr`. Then "Bootloader USB
   mode" once the computer has picked it up. **Photograph the screen.**

## 3. Collect

1. Leave USB connected until `collect` finishes: unplugging ends the
   iPod's USB session, and the check has to be run again. The iPod appears
   as a write-protected disk named "Nano 3G NAND" (4.3 GB on a 4GB unit).
   It has no file system: if Windows offers to format it, or macOS says
   the disk is not readable, choose Cancel or Ignore.
2. Run, with read access to the disk:
   ```
   sudo python3 utils/ipodnano3g/nandcheck/nandcheck.py collect
   ```
   It finds the iPod by the report in its first sector. On Windows, run
   `python utils\ipodnano3g\nandcheck\nandcheck.py collect` from an
   administrator prompt. To name the disk instead, add `/dev/sdX` (Linux),
   `/dev/rdiskN` (macOS) or `\\.\PhysicalDriveN` (Windows). It takes
   about 7 minutes on a 4GB unit (longer on 8GB) and writes
   `nandcheck-<id>-x<n>.tar.gz`, a few MB.
3. Unplug USB, then hold MENU+SELECT to restart. The iPod comes back as it
   was.

## 4. Send

The `.tar.gz` and the photo. The archive holds your chip's structure
records and the unit's model and firmware version - no music, file names or file contents, and not the serial number.
If the verdict says the chip is not in Apple's table, the photo is enough.

## If something goes wrong

- **No disk appears.** Check the screen: the summary should be there and
  the last line should read "Bootloader USB mode". On Linux, `dmesg` shows
  what the computer saw. Send the photo and that output.
- **"no disk carrying the Nano 3G NAND check was found"** with "no
  permission to read ...": run as root or administrator. On Linux you can
  instead give yourself read access to that disk until it is unplugged:
  `sudo setfacl -m u:$USER:r /dev/sdX`.
- **Unplugged too early.** Restart the iPod (MENU+SELECT) and start again
  from step 2.

## What is collected

| file | contents |
|---|---|
| `report.txt` | chip ids (all eight id bytes, every chip enable), chip table row and derived geometry, read-only mount result, verdict, model, hardware and firmware version, battery voltage, backlight register |
| `meta.bin` | every page's 12 spare bytes and read result, bank by bank (the host tests' dump format) |
| `results.bin` | every page's full read result: ECC class, most bits corrected in a chunk, erased flag |
| `pages.bin` | every page that is not user data or erased: FTL and VFL control structures, Apple's bad-block records |
| `summary.txt` | page counts by type, ECC failures, timeouts, erased-flag agreement, corrected-bit histogram |