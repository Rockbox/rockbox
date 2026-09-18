# Apple Nano 3G FTL write path (osos 1.1.3, Whimory 2.1), decoded 2026-09-13

Disassembly in this directory (`*.s`, from `dis/fn.py`). Whimory source path
string: `Whimory2_1\core\FTL\FTLInter...`. Function names come from the
firmware's own log strings.

| function | address | Nano 2G equivalent (ftl-nano2g.c) |
|---|---|---|
| `FTL_Write` | 0x0803af64 | `ftl_write()` |
| `_PrepareLog` | 0x0806b5ec | `ftl_allocate_log_entry()` |
| `_FindLogForLbn` | 0x08084998 | `ftl_get_log_entry()` |
| `_Merge` | 0x080dd8c4 | `ftl_remove_scattered_block()` |
| `_DoMoveMerge` | 0x0806f788 | `ftl_compact_scattered()` |
| `_DoCopyMerge` | 0x0806f488 | `ftl_commit_sequential()` |
| `_DoSimpleMerge` | 0x0807c1b0 | `ftl_commit_scattered()` |
| `_FTLMarkCxtNotValid` | 0x080a1e50 | clean_flag block in `ftl_write()` |
| `_GetFreeVb` / `_SetFreeVb` | 0x08067098 / 0x0806724c | `ftl_allocate_pool_block()` / `ftl_release_pool_block()` |
| `_AutoWearLevel` | 0x0807bdfc | `ftl_swap_blocks()` |
| `FTL_Flush` | 0x0803a404 | `ftl_sync()` |
| `_StoreFTLCxt` | 0x0806fc1c | `ftl_commit_cxt()` |
| `_FTLRestore` | 0x08069a4c | (none - nano2g refuses an unclean mount) |

Globals: 0x08b5ed00 device info (+0x20 u16 pages per superblock, +0x2e u16
user logical blocks, +0x30 u32 capacity in pages, +0x34 u16 page size);
0x08a34b7c pointers (+4 FTL context, +12 spare buffer array).
FTL context: +4 user usn, +8 freecount, +12 swapcounter, +0x198 map table
pointer, +0x1a4 17 log structs of 20 bytes, +0x318 ctrlpage, +0x31c clean
flag, +0x3c8 refresh-list count.
Log struct: +0 usn (recency), +4 vbn (0xffff none), +6 lbn, +8 offsets
table pointer, +12 pages used, +14 pages current, +16 is-sequential.

## FTL_Write(lpn, count, buf)
- `_FTLMarkCxtNotValid()` at every call (writes the 0x4f mark only if clean).
- Per logical block: `log = _PrepareLog(lbn)`.
  - Log full (pages used == ppb): `_Merge(log)`; if that freed the log's
    block, `_GetFreeVb` a new one.
  - Whole-block write (count >= ppb, offset 0): into the log block if it is
    empty, else into a fresh `_GetFreeVb` block; then `_SetFreeVb` the log
    block and the old data block, map[lbn] = new block, log vbn = 0xffff.
  - Else append min(ppb - offset, count, ppb - used) pages to the log with
    `VFL_WriteMultiplePagesInVb` (0x0805f8f0), updating offsets; sequential
    stays set only while every page lands at its own index.
  - Last page of a block gets type 0x41 when whole-block or still sequential.
  - A full, sequential log is merged immediately (becomes the data block).
- **usn (context +4) increments only when a write starts at page 0 of a
  block**; all pages appended to a log share that usn. (Matches the dump:
  Apple's logs of 8 and 4 pages each carry one usn.)
- Program failure: log marked full and non-sequential (forces a merge), or a
  fresh whole-block target is `_SetFreeVb`'d; then retry. No panic.
- **No commit.** Afterwards: refresh list > 2 entries -> handle refresh;
  else swapcounter >= 300 -> swapcounter -= 20 and up to 4 `_AutoWearLevel`
  (identical to nano2g).

## _PrepareLog(lbn)  (= nano2g ftl_allocate_log_entry)
- Existing log for lbn: log usn = context usn - 1 (recency), return it.
- Else reuse a slot that has a block but no pages.
- Else if freecount == 3: `_Merge(NULL)`.
- Then take a free slot, `_GetFreeVb` its block.

## _Merge(log)  (= nano2g ftl_remove_scattered_block)
- `_FTLMarkCxtNotValid()`.
- log NULL: victim = lowest usn, ties -> more current pages; sequential ->
  `_DoCopyMerge`, else `_DoSimpleMerge`.
- log given: current pages <= ppb/2 -> **`_DoMoveMerge` (compaction)**;
  else sequential -> `_DoCopyMerge`, else `_DoSimpleMerge`.
- swapcounter++ per merge.

## FTL_Flush(mode)
- Clean flag set: return.
- **mode 1: fold every log** (sequential -> `_DoCopyMerge`, else
  `_DoSimpleMerge`, swapcounter++ each), then store.
- **other modes: store without folding - logs stay open, recorded in the
  context and the 17 0x45 offset-table pages.** The measured dump's commit
  (18 free, 2 logs, non-FF 0x45 data) is one of these.
- `_StoreFTLCxt` up to 5 tries, moving ctrlpage to a block end on failure.
- Callers: only the AND call dispatcher (0x080279e4, mode from the OS) and
  the end of `_FTLRestore`. Never from the write path.

## _FTLRestore
Closed blocks (last page 0x41) -> map, highest usn wins; unclosed with page 0
data -> logs; one log per lbn (loser of an offset-table comparison dropped);
more than 17 logs fails (then FTL_Open fails, "erasing block 0" = reformat);
holes filled with erased blocks; free pool = the rest, must total 20 with
logs; whole pool erased; then FTL_Flush.
