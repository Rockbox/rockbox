
	.section	.text
	.align		4
	
	.arm
	
	.global		main
        .global         _interrupt_disable
	.global		_interrupt_enable

        .global		usb_write_addr
	.global		usb_sz

	#define		BUFF_ADDR	0x60800000

// -----------------------------------------------------
//  vector table
// -----------------------------------------------------
    ldr pc, =main
    ldr pc, =main
    ldr pc, =main
    ldr pc, =main
    ldr pc, =main
    ldr pc, =main
    ldr pc, =irq_handler
    ldr pc, =main

// -----------------------------------------------------
//  main
// -----------------------------------------------------
main:
	// turn on usb interrupts
	mov		r0, #0x18000000
	add		r0, r0, #0x80000
	ldr		r1, [r0, #0x10c]
	orr		r1, r1, #0x10000
	str		r1, [r0, #0x10c]

	// enable usb-bulk
	add		r0, r0, #0x20000	// R0 = 0x180A0000 (UDC_BASE)

	// enable EP1, write_reg32(UDC_RX1CON, (0x1 << 8) | RxACKINTEN | RxEPEN);
	mov		r1, #0x190	// bits 8,7,4 -> 0x190
	str		r1, [r0, #0x58]
	
	// setup receive buffer (must be aligned on dword boundary)
	ldr		r1,=usb_write_addr	// write_reg32(UDC_DMA1LM_OADDR, (uint32_t)rx_buff);
	ldr		r1, [r1]
	str		r1, [r0, #0x60]		// UDC_DMA1LM_OADDR = usb_write_addr
	
	// write_reg32(UDC_DMA1CTRLO, read_reg32(UDC_DMA1CTRLO) | ENP_DMA_START);
	ldr		r1, [r0, #0x5c]
	orr		r1, r1, #2
	str		r1, [r0, #0x5c]

	// enable bulk_out1 interrupt
	ldr		r1, [r0, #0x14]		// UDC_ENINT
	orr		r1, r1, #0x100		// EN_BOUT1_INTR
	str		r1, [r0, #0x14]

	bl		_interrupt_enable
idle:
	b		idle

// -----------------------------------------------------
// _interrupt_enable - enables interrupts
// -----------------------------------------------------
_interrupt_enable:
        mrs             r0, cpsr
        bic             r0, r0, #0x80
        msr             cpsr_c, r0
        mov             pc, lr

// -----------------------------------------------------
// _interrupt_disable - disables interrupts
// -----------------------------------------------------
_interrupt_disable:
        mrs             r0, cpsr
        orr             r0, r0, #0xc0
        msr             cpsr_c, r0
        mov             pc, lr


	.section .data
usb_write_addr:
	.word	(BUFF_ADDR-4)

usb_sz:
	.word	(BUFF_ADDR-4)
