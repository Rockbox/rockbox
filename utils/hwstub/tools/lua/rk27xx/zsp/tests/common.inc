; Shared definitions for on-device tests.
; DMEM_DSP is the DSP-side address of DMEM; pass it with -D DMEM_DSP=...
; (found by t0b_probe). All offsets are in DSP data-address units.
RES     = DMEM_DSP + 0x100      ; 0x100-unit result block (host clears it to 0xA5A5)
EXTRA   = RES + 0x40            ; per-test results beyond the register dump
SCRATCH = DMEM_DSP + 0x300      ; work area for load/store tests
MAGIC   = 0xD0DE
; Store-address lag (t0d-t0g): without a DSP peripheral reset (RSTCFG bit 5)
; a store writes its data to the address of the store two before it.
; ZSP.setup() now does the reset, so by default 'pst' is a plain store.
; Build with -D STORE_LAG=1 to get the workaround back (two extra stores to
; a7+STORE_DUMMY after each 'pst'); STORE_DUMMY/+1 are scratch slots in RES.
STORE_DUMMY = 58
