# Apple Nano 3G FTL read path (osos 1.1.3, Whimory 2.1), decoded 2026-09-13

Claims use **[measured]**, **[decoded]**, **[inferred]** and **[unknown]** as
defined by the project notes. Addresses are ARM addresses in
`n3g-dump/osos-1.1.3.bin`; the FMSS program is at IRAM address 0x22009030.

| function | address | identification |
|---|---:|---|
| `FTL_Read` | 0x0803ace4 | **[decoded]** AND dispatcher call and arguments |
| `_FTLRead` | 0x080e3514 | **[decoded]** internal mapping/batched read |
| `VFL_Read` | 0x0805e8e8 | **[decoded]** single-page read and retry |
| `VFL_ReadMultiplePagesInVb` | 0x0805ecc8 | **[decoded]** sequential virtual-block read |
| `VFL_ReadScatteredPagesInVb` | 0x0805f0b8 | **[decoded]** VPN-list read |
| `m1fmssReadScatteredPages` | 0x082d3c78 | **[decoded]** FIL list builder |
| `m1fmssReadSequentialPages` | 0x082d3de0 | **[decoded]** FIL row-list builder |
| FMSS read program | 0x22009030 | **[decoded]** common program used by both FIL calls |

## FTL_Read and _FTLRead

`FTL_Read(lpn, count, buffer)` updates read statistics, calls
`_FTLRead(lpn, count, buffer)`, logs a nonzero result, services the refresh
list when it contains more than two entries (or a secondary counter passes
its threshold), and returns `_FTLRead`'s result. The AND call dispatcher at
0x080279bc loads the same three arguments before calling it. **[decoded]**

`_FTLRead` rejects a null buffer, a zero/out-of-range request, and a buffer
whose address is not four-byte aligned. It splits the request at logical
block boundaries. For each logical block it looks up its log entry. **[decoded]**

- With no log entry, it issues one `VFL_ReadMultiplePagesInVb(data_vbn,
  page_offset, count, buffer, spares, &refresh)` for the whole portion of the
  logical block. **[decoded]**
- With a log entry, it resolves every requested LPN to either the log block
  page or the data block page, builds a VPN list, and issues one
  `VFL_ReadScatteredPagesInVb(vpn_list, count, buffer, spares, &refresh)`.
  **[decoded]**
- It then checks every returned 12-byte spare. A written page's stored LPN
  must equal the requested LPN. An ECC mark other than 0xff is reported as an
  error and the logical block is added to the refresh list. **[decoded]**
- If a multi-page VFL call fails, it retries each page separately through
  `VFL_Read`, preserving the page's resolved virtual address. A failing page
  or an unacceptable ECC mark makes the overall call fail, but the loop can
  continue far enough to report more detail. **[decoded]**

The merge/copy callers use the same fallback: `_DoSimpleMerge` and the block
copy at 0x080e4030 read `pages_per_superblock / 8` pages with `_FTLRead`; on
failure they retry each page with `_FTLRead(..., 1, ...)`, and put 0x55 in
byte 10 of that page's new 12-byte spare if the retry also fails. The 0x55 is
written with the destination block during the following write run; it is not
written back into the source page. **[decoded]**

The Nano 2G implementation selects a bank-wide fast path only when aligned
and when no log is present. Nano 3G Apple firmware instead supplies the whole
logical-block portion to one of the two VFL multi-page interfaces, including
mixed data/log runs through the scattered interface. **[decoded]**

## VFL splitting and error handling

`VFL_ReadMultiplePagesInVb(vbn, start, count, buffer, spares, &refresh)` maps
the virtual block independently on every bank, then splits the run according
to the four-bank row width: **[decoded]**

1. A head ending at the next row boundary is sent to the scattered FIL call,
   unless it is a complete row.
2. While more than one complete row remains, the largest multiple of four
   leaving the final row/tail is sent to the sequential FIL call.
3. The remaining one through four pages are sent to the scattered FIL call.

Thus a row-aligned run of four pages is deliberately a scattered FIL call;
eight pages become four sequential plus four scattered. This is the same
head / whole rows while more than one row remains / tail policy used by
Apple's VFL write path. Reads have no two-plane eligibility test. **[decoded]**

For each subcall the VFL advances data by 2048 bytes/page, spare data by 12
bytes/page, and the virtual-page position by the number completed. It returns
1 only when every FIL subcall returns either 0 or 1; other FIL status values
produce VFL failure (0). The maximum corrected-bit count returned by the FIL
is compared with the device threshold byte at device-info +0x0f; reaching or
exceeding it sets `*refresh = 1`. **[decoded]**

`VFL_ReadScatteredPagesInVb` maps every input VPN into parallel bank and
physical-page arrays, calls the scattered FIL once for the full list, applies
the same corrected-bit threshold, and normalizes FIL status 0 or 1 to VFL
success 1. **[decoded]**

`VFL_Read` maps one VPN and calls the mode's single-page FIL read. On the
relevant failures it resets the NAND bank and retries once. Its later path can
schedule a block remap and update per-block read accounting. **[decoded]**

The refresh list and its handler mutate FTL state and can ultimately cause
NAND writes. Rockbox currently has no equivalent. Porting that mechanism is
outside the read batching itself and requires the user's explicit scope
decision before implementation. **[decoded]**

## FIL parameter block

Both FIL functions run program 0x22009030 and write the same sequencer
parameter block. **[decoded]**

| offset | meaning |
|---:|---|
| `SEQ+0xc04` | program address, 0x22009030 |
| `SEQ+0xd04` | bank count (4 on Nano 3G) |
| `SEQ+0xd0c` | pointer to u32 physical-page list |
| `SEQ+0xd10` | pointer to u32 logical-bank list |
| `SEQ+0xd14` | pointer to bank-to-chip-enable table |
| `SEQ+0xd18` | number of pages |
| `SEQ+0xd1c` | pointer to packed 12-byte spare outputs |
| `SEQ+0xd20` | pointer to the page/chunk DMA-address list |
| `SEQ+0xd24` | pointer to one u32 ECC/result word per page |
| `SEQ+0xd28` | 2KiB units per page (sectors-per-page / 4 = 1 here) |
| `SEQ+0xd48` | base `FMCTRL0` bits from the mode descriptor |
| `SEQ+0xd4c` | additional `FMCTRL0` bits from the mode descriptor |

The scattered FIL accepts parallel bank/page inputs and expands a contiguous
data buffer into four 512-byte DMA addresses per page. The sequential FIL
accepts one starting physical page per bank and expands it row by row into
the same page and bank lists; its buffer layout is likewise expanded into
four chunk addresses per page. Both call the same completion routine and
then classify the aggregate result at `SEQ+0xc30`. **[decoded]**

The result classifier treats bit 30 as a hard FIL failure. Bit 29 identifies
the erased/empty-page condition used by a single-page caller. The low byte is
a bitmap of corrected-bit counts seen among the page's four chunks; the
highest set bit becomes the returned maximum corrected-bit count. **[decoded]**

## FMSS opcodes 0x11, 0x18 and 0x19

All occurrences in 0x22009030 pin down the previously unknown operations:
**[decoded]**

```
0x11: mem32[rB] = rA
0x18: rA = seq32[rB]
0x19: seq32[rB] = rA
```

Opcode 0x11 stores the three spare words and the per-page result into ARM
memory. Opcodes 0x18/0x19 indirectly access sequencer-local registers/RAM.
The program uses 0x19 to build the unique chip-enable list at `SEQ+0xd60`,
then 0x18 to walk it. A later 0x18 loads the next chip enable through the
pointer at `SEQ+0xd50`. The disassembler and emulator now implement all
three operations. **[decoded]**

## Program 0x22009030

The program first scans the supplied bank list, converts each bank through
the chip-enable table, and records each distinct chip enable in sequencer
memory at `SEQ+0xd60`. It then sends NAND READ (0x00), the row address and
READ START (0x30) for the first pending page on every distinct chip enable
before waiting for any one chip's ready status. This overlaps NAND page-load
time across banks. **[decoded]**

For each page it selects the corresponding chip enable, waits for status
ready with command 0x70, returns to data output with command 0x00, and runs
four 512-byte data/ECC pipelines. Each chunk stages controller data, starts
the ECC transfer, records an uncorrectable indication as result bit 30 or a
corrected-bit-count indication in the result's low-byte bitmap, and DMA-writes
the corrected 512 bytes to its supplied address. **[decoded]**

After the chunks it decodes and stores `FMSYND5..7` as the 12-byte spare,
stores the page result word, and ORs that word into the aggregate result.
When another page remains on that chip enable, it starts that NAND page load
before proceeding, maintaining one outstanding load per bank while pages on
other banks are transferred. Finally it clears the DMA/chip-select bits and
ends; the sequencer exposes the aggregate result at `SEQ+0xc30`. **[decoded]**

The emulator now completes one-page and multi-bank runs. Its clean model
produces result word zero; setting `FMUNK810`'s corrected count field to 3
produces low-byte result bit 2 (`0x4`) for each page, matching the classifier's
reported correction count of 3. These are emulator observations, not device
measurements. **[measured]**

## Porting consequences

- One Rockbox `nand_read_pages` call should accept an ordered physical-page,
  bank, data-buffer and spare/result list and reproduce 0x22009030. A
  single-page wrapper may use it only after register equivalence and hardware
  validation. **[inferred]**
- FTL host reads should split only at logical-block boundaries. A block
  portion without a log uses the multiple-VB path; one with a log builds a
  scattered physical list. **[decoded]**
- Merge reads should retain Apple's 128-page grouping and page-by-page retry,
  including the destination spare's 0x55 ECC mark after a repeated failure.
  Any on-flash behavior will be called out before device testing. **[decoded]**
- The existing BootROM-derived `nand_read_page` remains the hardware-proven
  fallback until the sequencer implementation has passed on the device.
  **[inferred]**
