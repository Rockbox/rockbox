; T0a: DSP runs a tight loop. Nothing to observe except that the host
; survives reset release and that IMEM still verifies afterwards.
        .org 0
loop:   br loop
