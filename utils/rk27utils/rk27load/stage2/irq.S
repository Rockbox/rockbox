	.section .text
	.align 4

	.global irq_handler
	#define         BUFF_ADDR       0x60800000

irq_handler:
        stmfd sp!, {r0-r7, ip, lr}

        // get interrupt number
        mov             r4, #0x18000000
        add             r4, r4, #0x80000
        ldr             r5, [r4, #0x104]
        and             r5, r5, #0x1f
        cmp             r5, #0x10 // UDC interrupt

        bleq udc_irq

        // clear pending interrupt
        mov             r3, #1
        mov             r2, r3, LSL r5
        str             r2, [r4, #0x118]

        ldmfd sp!, {r0-r7, ip, lr}
        subs  pc, lr, #4

udc_irq:
        stmfd   sp!, {r4-r8, lr}

        // handle usb interrupt
        ldr             r4,=0x180A0000
        ldr             r5, [r4, #0x18]         // UDC_INTFLAG

        // ep0 in intr
        tst             r5, #0x04
        beq             bulk_recv_intr

        // write_reg32(UDC_TX0STAT, read_reg32(UDC_TX0STAT) & ~0x7FF);
        ldr             r5, [r4, #0x40]
        mov             r5, r5, lsr #10
        mov             r5, r5, lsl #10         // clear clower 10 bits
        str             r5, [r4, #0x40]

        // write_reg32(UDC_DMA0LM_OADDR, (uint32_t)(state.ctrlep_data));
        mov             r5, #0x60000000
        str             r5, [r4, #0x3c]

        // write_reg32(UDC_DMA0CTLO, read_reg32(UDC_DMA0CTLO) | ENP_DMA_START);
        mov             r5, #1
        str             r5, [r4, #0x38]

        ldmfd   sp!, {r4-r8, pc}

// bulk out interrupt
bulk_recv_intr:
        tst             r5, #0x100
        ldmeqfd sp!, {r4-r8, pc}

        // read UDC_RX1STAT
        ldr             r5, [r4, #0x54]
        mov             r5, r5, lsl #21
        mov             r5, r5, lsr #21         // r5 = length

        ldr             r6,=usb_sz
        ldr             r6, [r6]
        ldr             r7, [r6]                        // r7 = total_code_length expected

        subs            r7, r7, r5
        bne             usb_bulk_out1_recv

        // copy from buff to the begining of the ram
        ldr             r0,=BUFF_ADDR
        ldr		r1,[r0,#-4]     // size

	ldr r1,=0x800000 // buffer size

        add             r1,r1,r0        // end address
        ldr             r2,=0x60000000  // destination
1:
        cmp             r1,r0
        ldrhi           r3,[r0],#4
        strhi           r3,[r2],#4
        bhi             1b

        // execute user code
        ldr             r0,=0x60000000
        bx              r0              // jump to 0x60000000

usb_bulk_out1_recv:
        str             r7, [r6]                        // size = size - received

        ldr             r6,=usb_write_addr
        ldr             r7, [r6]

        add             r7, r7, r5
        str             r7, [r6]                        // usb_write_addr += length

        str             r7, [r4, #0x60]         // DMA1LM_OADDR = usb_write_addr

        mov             r5, #1
        str             r5, [r4, #0x5c]         // DMA1_CTL0 = ENP_DMA_START

        ldmfd   sp!, {r4-r8, pc}
