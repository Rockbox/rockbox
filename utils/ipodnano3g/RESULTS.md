# Nano 3G storage validation results

Results below were recorded on 2026-09-13. Hardware results used a 4 GB Nano
3G with Hynix NAND ID `0xA514D3AD`.

## Host tests

- The independent newest-page oracle agreed with `ftl_read()` for all
  1,982,464 logical sectors in the device image.
- Read/write, wear-level, bad-block injection, allocation eligibility,
  restore and mock-NAND suites completed without failures.
- Power was cut at 300 operation boundaries in each of 48 deterministic
  campaigns. Half modeled a torn page program or half-completed erase. All
  acknowledged writes survived each remount and no page was programmed twice
  or out of order.
- The 600-file write replay retained 66,388 copy-merge reads and 34,816
  delete-merge reads. Batched reads reduced these to 528 and 280 controller
  calls respectively, averaging 125.7 and 124.3 pages per call.
- Driver and emulator register traces were identical in all write scenarios:
  one page, single-plane rows, repeated-bank pages, and one through three
  two-plane rows.
- Driver and emulator register traces were identical for one read, four-bank
  reads, repeated-bank reads and a mixed duplicate-bank split.

## Hardware tests

- A complete NAND dump produced 184 VFL context pages that all passed the
  format's checksums.
- Rockbox mounted and restored the uncommitted state left by Apple disk mode.
- A 77,067,368-byte, 600-file tree written through Rockbox read back with all
  SHA-256 hashes intact through both Rockbox and Apple disk mode.
- The same tree read at 7.550 MB/s through Rockbox USB and 9.745 MB/s through
  Apple disk mode. Rockbox wrote and synced it at 0.731 MB/s; Apple wrote and
  synced it at 1.026 MB/s.
- A setting survived reset and restore, and Apple firmware subsequently read
  the volume successfully.

Bad-block remapping and forced program/erase failure recovery have extensive
host fault-injection coverage, including validation of the persisted remap
after remount. They have not been deliberately triggered on hardware. The
implemented layout and recovery order follow the decoded Apple FTL/VFL
process documented in `decode/APPLE-FTL-WRITE.md`; that correspondence is
evidence for the implementation, not a substitute for destructive hardware
fault testing.

## Later additions

- **NAND check tool update, 2026-09-17.** The rebuilt check image reads a
  4GB unit with no ECC failures or timeouts; two independent collections
  from it agree byte for byte.
- **2-CE Micron A5D5D52C, enabled 2026-09-18.** A contributor's check
  archive matches the table row exactly, including a derived `vflspares`
  of 89. Replayed against the host suite: `test_ftl` agrees with the
  oracle on all 1,982,464 sectors; `test_crash` survives 100 power cuts
  (172 writes, 6 syncs, 0 sectors wrong). No on-device write test.
- **B614D5EC x2, A5D5D589 x2/x4, 3E94D589 x2, 2026-09-19.** Four
  contributors' check archives all match their rows and pass `test_crash`
  clean. `test_ftl` passes three outright; A5D5D589 x2 disagrees with the
  oracle on two logical pages - confirmed against Apple's own disassembly
  (`_FTLRestore`, osos 1.1.3) to be that algorithm's own tie-break between
  two competing logs, not a defect (see the note in `ftltest/test_ftl.c`).
- **BA94D598 x4, 2026-09-20.** A contributor's check archive matches the
  row exactly (Toshiba, mode 1, vflspares 201 by our own formula). Clean
  on both host tests: `test_ftl` agrees with the oracle on all 3,964,928
  sectors; `test_crash` survives 100 power cuts (344 writes, 11 syncs, 0
  sectors wrong). No on-device write test.
- **B614D5EC x4, 2026-09-21.** A contributor's check archive matches the
  row exactly (mode 1, vflspares 201 by our own formula). `test_crash` is
  clean (100 power cuts, 349 writes, 9 syncs, 0 sectors wrong). `test_ftl`
  disagrees with the oracle on two logical pages in one block; traced to a
  second, distinct false positive from the one already documented for
  A5D5D589 x2 - a closed data block addressed purely by position, with one
  stale leftover page - confirmed against the decode notes
  (`_FTLRestore`'s "closed blocks -> map" step) and not a defect (see
  `ftltest/test_ftl.c`).
