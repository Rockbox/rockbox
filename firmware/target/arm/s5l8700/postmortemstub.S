.section .text.post_mortem_stub, "ax", %progbits
.align 4
.global post_mortem_stub
.type post_mortem_stub, %function
post_mortem_stub:
MSR CPSR_c, #0xD3          @ Supervisor mode, no IRQs, no FIQs
MRC p15, 0, R0,c1,c0
BIC R0, R0, #5
MCR p15, 0, R0,c1,c0       @ Disable the Protection Unit and DCache
MOV R13, #0
pms_flushcache_loop:
  MCR p15, 0, R13,c7,c14,2
  ADD R0, R13, #0x10
  MCR p15, 0, R0,c7,c14,2
  ADD R0, R0, #0x10
  MCR p15, 0, R0,c7,c14,2
  ADD R0, R0, #0x10
  MCR p15, 0, R0,c7,c14,2
  ADDS R13, R13, #0x04000000
BNE pms_flushcache_loop
MCR p15, 0, R13,c7,c10,4

LDR R7, pms_00080200
ORR R8, R7, #0x8000
ADR R9, pms_recvbuf
LDR R10, pms_20080040
MOV R11, #0x38800000
MOV R12, #1

MOV R2, #0x3C400000
ADD R1, R2, #0x00100000    @ Enable USB clocks
LDR R0, [R1,#0x28]
BIC R0, R0, #0x4000
STR R0, [R1,#0x28]
LDR R0, [R1,#0x40]
BIC R0, R0, #0x800
STR R0, [R1,#0x40]
LDR R0, pms_20803180       @ Clocking config
STR R0, [R1]
MOV R0, #0x280
STR R0, [R1,#0x3C]
MRC p15, 0, R0,c1,c0
ORR R0, R0, #0xc0000000
MCR p15, 0, R0,c1,c0       @ Asynchronous mode

STR R13, [R11,#0xE00]        @ PHY clock enable

MOV R1, #0x800
ORR R0, R2, #2
STR R0, [R11,#0x804]        @ USB2 Gadget: Soft disconnect

STR R13, [R2]              @ USB2 PHY: Power on
STR R12, [R2,#0x08]         @ USB2 PHY: Assert Software Reset
MOV R0, #0x10000
pms_wait:
SUBS R0, R0, #1
BNE pms_wait
STR R13, [R2,#0x08]         @ USB2 PHY: Deassert Software Reset
STR R13, [R2,#0x04]         @ USB2 PHY: Clock is 48MHz

STR R12, [R11,#0x10]        @ USB2 Gadget: Assert Core Software Reset
pms_waitcorereset:
LDR R0, [R11,#0x10]         @ USB2 Gadget: Wait for Core to reset
TST R0, #1
BNE pms_waitcorereset
TST R0, #0x80000000        @ USB2 Gadget: Wait for AHB IDLE
BEQ pms_waitcorereset

MOV R0, #0x200
STR R0, [R11,#0x24]         @ USB2 Gadget: RX FIFO size: 512 bytes
ORR R0, R0, #0x2000000
STR R0, [R11,#0x28]         @ USB2 Gadget: Non-periodic TX FIFO size: 512 bytes
MOV R0, #0x26
STR R0, [R11,#0x08]         @ USB2 Gadget: DMA Enable, Burst Length: 4, Mask Interrupts
MOV R0, #0x1400
ADD R0, R0, #8
STR R0, [R11,#0x0C]         @ USB2 Gadget: PHY IF is 16bit, Turnaround 5
STR R1, [R11,#0x804]        @ USB2 Gadget: Soft reconnect

ADR R14, pms_ctrlbuf
ORR R5, R8, #0x84000000
@ fallthrough

pms_mainloop:
  LDR R3, [R11,#0x14]         @ Global USB interrupts
  TST R3, #0x00001000        @ BUS reset
  BEQ pms_noreset
    MOV R0, #0x500
    STR R0, [R11,#0x804]
    MOV R0, #4
    STR R0, [R11,#0x800]        @ USB2 Gadget: Device Address 0, STALL on non-zero length status stage
    MOV R0, #0x8000
    STR R0, [R11,#0x900]        @ USB2 Gadget: Endpoint 0 IN Control: ACTIVE
    STR R10, [R11,#0xB10]        @ USB2 Gadget: Endpoint 0 OUT Transfer Size: 64 Bytes, 1 Packet, 1 Setup Packet
    STR R14, [R11,#0xB14]        @ USB2 Gadget: Endpoint 0 OUT DMA Address: pms_ctrlbuf
    ORR R6, R0, #0x84000000
    STR R6, [R11,#0xB00]        @ USB2 Gadget: Endpoint 0 OUT Control: ENABLE CLEARNAK
    STR R8, [R11,#0x960]        @ USB2 Gadget: Endpoint 3 IN Control: ACTIVE BULK, 512 byte packets
    STR R8, [R11,#0xB80]        @ USB2 Gadget: Endpoint 4 OUT Control: ACTIVE BULK, 512 byte packets
    STR R7, [R11,#0xB90]        @ USB2 Gadget: Endpoint 4 OUT Transfer Size: 512 Bytes, 1 Packet
    STR R9, [R11,#0xB94]        @ USB2 Gadget: Endpoint 4 OUT DMA Address: pms_recvbuf
    ORR R4, R5, #0x10000000
    STR R4, [R11,#0xB80]        @ USB2 Gadget: Endpoint 4 OUT Control: ENABLE CLEARNAK DATA0
  pms_noreset:
  LDR R0, [R11,#0x908]        @ Just ACK all IN events...
  STR R0, [R11,#0x908]
  LDR R0, [R11,#0x968]
  STR R0, [R11,#0x968]
  LDR R2, [R11,#0xB08]
  MOVS R2, R2                @ Event on OUT EP0
  BEQ pms_noep0out
    TST R2, #8                 @ SETUP phase done
    BEQ pms_controldone
      LDRB R0, [R14,#1] @ Get request type
      CMP R0, #0
        BEQ pms_GET_STATUS
      CMP R0, #1
        BEQ pms_CLEAR_FEATURE
      CMP R0, #3
        BEQ pms_SET_FEATURE
      CMP R0, #5
        BEQ pms_SET_ADDRESS
      CMP R0, #6
        BEQ pms_GET_DESCRIPTOR
      CMP R0, #8
        BEQ pms_GET_CONFIGURATION
      CMP R0, #9
        BEQ pms_SET_CONFIGURATION
      pms_ctrlstall:
      LDR R0, [R11,#0x900]
      ORR R0, R0, #0x00200000
      STR R0, [R11,#0x900]        @ Stall IN EP0
      LDR R0, [R11,#0xB00]
      ORR R0, R0, #0x00200000
      STR R0, [R11,#0xB00]        @ Stall OUT EP0
    pms_controldone:
    STR R10, [R11,#0xB10]        @ OUT EP0: 64 Bytes, 1 Packet, 1 Setup Packet
    STR R14, [R11,#0xB14]        @ OUT EP0: DMA address
    STR R6, [R11,#0xB00]        @ OUT EP0: Enable ClearNAK
  pms_noep0out:
  STR R2, [R11,#0xB08]        @ ACK it, whatever it was...
  LDR R2, [R11,#0xB88]
  MOVS R2, R2                @ Event on OUT EP4
  BEQ pms_noep1out
    TST R2, #1                 @ XFER complete
    BEQ pms_datadone
      LDR R0, pms_000001FF
      LDR R1, pms_recvbuf+4
      ADD R0, R0, R1
      MOV R0, R0,LSR#9
      ORR R1, R1, R0,LSL#19      @ Number of packets
      LDR R0, pms_recvbuf
      STR R1, [R11,#0x970]        @ EP3 IN: Number of packets, size
      STR R0, [R11,#0x974]        @ EP3 IN: DMA address
      STR R5, [R11,#0x960]        @ EP3 IN: Enable ClearNAK
    pms_datadone:
    STR R7, [R11,#0xB90]        @ OUT EP4: 512 Bytes, 1 Packet
    STR R9, [R11,#0xB94]        @ Out EP4: DMA address
    STR R5, [R11,#0xB80]        @ Out EP4: Enable ClearNAK
  pms_noep1out:
  STR R2, [R11,#0xB88]        @ ACK it, whatever it was...
  STR R3, [R11,#0x14]         @ ACK global ints
B pms_mainloop

pms_CLEAR_FEATURE:
  LDRB R0, [R14]
  CMP R0, #2
  LDREQ R0, [R14,#2]
  BICEQ R0, R0, #0x00800000
  CMPEQ R0, #0x00010000
@ fallthrough

pms_SET_CONFIGURATION:
  ORREQ R0, R8, #0x10000000
  STREQ R0, [R11,#0x960]      @ EP3 IN: Set DATA0 PID
  STREQ R4, [R11,#0xB80]      @ EP4 OUT: Set DATA0 PID
B pms_SET_FEATURE              @ zero-length ACK

pms_GET_CONFIGURATION:
  MOV R1, #1
  STR R1, [R14]
@ fallthrough

pms_ctrlsend:
  ORR R0, R1, #0x00080000    @ 1 Packet
  STR R0, [R11,#0x910]        @ EP0 IN: 1 Packet, Size as in R1
  STR R14, [R11,#0x914]        @ EP0 IN: DMA address
  ORR R0, R6, #0x1800
  STR R0, [R11,#0x900]        @ EP0 IN: Enable ClearNAK
  ADR R14, pms_ctrlbuf
B pms_controldone

pms_GET_DESCRIPTOR:
  LDRB R0, [R14,#3]           @ Descriptor type
  CMP R0, #1
    ADREQ R14, pms_devicedescriptor
    BEQ pms_senddescriptor
  CMP R0, #2
    ADREQ R14, pms_configurationdescriptor
    MOVEQ R1, #0x20
    BEQ pms_senddescriptorcustomsize
  CMP R0, #3
  BNE pms_ctrlstall
  LDRB R0, [R14,#2]           @ String descriptor index
  CMP R0, #0
    LDREQ R0, pms_langstringdescriptor
    STREQ R0, [R14]
    BEQ pms_senddescriptor
  CMP R0, #1
  CMPNE R0, #2
    ADREQ R14, pms_devnamestringdescriptor
  BNE pms_ctrlstall
@ fallthrough

pms_senddescriptor:
  LDRB R1, [R14]              @ Descriptor length
@ fallthrough

pms_senddescriptorcustomsize:
  LDRH R0, pms_ctrlbuf+6        @ Requested length
  CMP R0, R1
  MOVLO R1, R0
B pms_ctrlsend

pms_SET_ADDRESS:
  LDRH R1, [R14,#2]        @ new address
  LDR R0, [R11,#0x800]
  BIC R0, R0, #0x000007F0
  ORR R0, R0, R1,LSL#4
  STR R0, [R11,#0x800]        @ set new address
@ fallthrough

pms_SET_FEATURE:
  MOV R1, #0                 @ zero-length ACK
B pms_ctrlsend

pms_20803180:
.word 0x20803180

.ltorg

.align 4

pms_configurationdescriptor:
.word 0x00200209
.word 0xC0000101
.word 0x00040932
.word 0xFFFF0200
.word 0x050700FF
.word 0x02000204
.word 0x83050701
.word 0x01020002

pms_devicedescriptor:
.word 0x02000112
.word 0x40FFFFFF
.word 0xA112FFFF
.word 0x02010001
.word 0x00010100

pms_00080200:
.word 0x00080200

pms_20080040:
.word 0x20080040

pms_000001FF:
.word 0x000001FF

pms_devnamestringdescriptor:
.word 0x0052030C
.word 0x00500042
.word 0x0053004D

pms_langstringdescriptor:
.word 0x04090304

pms_ctrlbuf:
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0

pms_recvbuf:
.word 0
.word 0

pms_GET_STATUS:
  LDRB R0, [R14]
  CMP R0, #0x80
  STREQ R12, [R14]
  STRNE R13, [R14]
  MOV R1, #2
B pms_ctrlsend

.size post_mortem_stub, .-post_mortem_stub
