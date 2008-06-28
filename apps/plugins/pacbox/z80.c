/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Copyright (c) 1997-2003,2004 Alessandro Scotti
 * http://www.ascotti.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "hardware.h"
#include "z80.h"
#include "z80_internal.h"

// Table with parity, sign and zero flags precomputed for each byte value
unsigned char PSZ_[256] IDATA_ATTR = {
    Zero|Parity, 0, 0, Parity, 0, Parity, Parity, 0, 0, Parity, Parity, 0, Parity, 0, 0, Parity, 
    0, Parity, Parity, 0, Parity, 0, 0, Parity, Parity, 0, 0, Parity, 0, Parity, Parity, 0, 
    0, Parity, Parity, 0, Parity, 0, 0, Parity, Parity, 0, 0, Parity, 0, Parity, Parity, 0, 
    Parity, 0, 0, Parity, 0, Parity, Parity, 0, 0, Parity, Parity, 0, Parity, 0, 0, Parity, 
    0, Parity, Parity, 0, Parity, 0, 0, Parity, Parity, 0, 0, Parity, 0, Parity, Parity, 0, 
    Parity, 0, 0, Parity, 0, Parity, Parity, 0, 0, Parity, Parity, 0, Parity, 0, 0, Parity, 
    Parity, 0, 0, Parity, 0, Parity, Parity, 0, 0, Parity, Parity, 0, Parity, 0, 0, Parity, 
    0, Parity, Parity, 0, Parity, 0, 0, Parity, Parity, 0, 0, Parity, 0, Parity, Parity, 0, 
    Sign, Sign|Parity, Sign|Parity, Sign, Sign|Parity, Sign, Sign, Sign|Parity, Sign|Parity, Sign, Sign, Sign|Parity, Sign, Sign|Parity, Sign|Parity, Sign, 
    Sign|Parity, Sign, Sign, Sign|Parity, Sign, Sign|Parity, Sign|Parity, Sign, Sign, Sign|Parity, Sign|Parity, Sign, Sign|Parity, Sign, Sign, Sign|Parity, 
    Sign|Parity, Sign, Sign, Sign|Parity, Sign, Sign|Parity, Sign|Parity, Sign, Sign, Sign|Parity, Sign|Parity, Sign, Sign|Parity, Sign, Sign, Sign|Parity, 
    Sign, Sign|Parity, Sign|Parity, Sign, Sign|Parity, Sign, Sign, Sign|Parity, Sign|Parity, Sign, Sign, Sign|Parity, Sign, Sign|Parity, Sign|Parity, Sign, 
    Sign|Parity, Sign, Sign, Sign|Parity, Sign, Sign|Parity, Sign|Parity, Sign, Sign, Sign|Parity, Sign|Parity, Sign, Sign|Parity, Sign, Sign, Sign|Parity, 
    Sign, Sign|Parity, Sign|Parity, Sign, Sign|Parity, Sign, Sign, Sign|Parity, Sign|Parity, Sign, Sign, Sign|Parity, Sign, Sign|Parity, Sign|Parity, Sign, 
    Sign, Sign|Parity, Sign|Parity, Sign, Sign|Parity, Sign, Sign, Sign|Parity, Sign|Parity, Sign, Sign, Sign|Parity, Sign, Sign|Parity, Sign|Parity, Sign, 
    Sign|Parity, Sign, Sign, Sign|Parity, Sign, Sign|Parity, Sign|Parity, Sign, Sign, Sign|Parity, Sign|Parity, Sign, Sign|Parity, Sign, Sign, Sign|Parity
};

// Interrupt flags
enum {
    IFF1    = 0x40,     // Interrupts enabled/disabled
    IFF2    = 0x20,     // Copy of IFF1 (used by non-maskable interrupts)
    Halted  = 0x10      // Internal use: signals that the CPU is halted
};

// Implements an opcode
typedef void (OpcodeHandler)(void);

typedef struct {
    OpcodeHandler*   handler;
    unsigned        cycles;
} OpcodeInfo;

// Implements an opcode for instructions that use the form (IX/IY + b)
typedef void (OpcodeHandlerXY)( unsigned );

typedef struct {
    OpcodeHandlerXY* handler;
    unsigned        cycles;
} OpcodeInfoXY;

/** */
void do_opcode_xy( OpcodeInfo * );

/** */
unsigned do_opcode_xycb( unsigned xy );

unsigned    iflags_ IBSS_ATTR;    // Interrupt mode (bits 0 and 1) and flags
unsigned    cycles_ IBSS_ATTR;    // Number of CPU cycles elapsed so far


// Registers
unsigned char   B  IBSS_ATTR;     //@- B register
unsigned char   C  IBSS_ATTR;     //@- C register
unsigned char   D  IBSS_ATTR;     //@- D register
unsigned char   E  IBSS_ATTR;     //@- E register
unsigned char   H  IBSS_ATTR;     //@- H register
unsigned char   L  IBSS_ATTR;     //@- L register
unsigned char   A  IBSS_ATTR;     //@- A register (accumulator)
unsigned char   F  IBSS_ATTR;     //@- Flags register
unsigned char   B1 IBSS_ATTR;     //@- Alternate B register (B')
unsigned char   C1 IBSS_ATTR;     //@- Alternate C register (C')
unsigned char   D1 IBSS_ATTR;     //@- Alternate D register (D')
unsigned char   E1 IBSS_ATTR;     //@- Alternate E register (E')
unsigned char   H1 IBSS_ATTR;     //@- Alternate H register (H')
unsigned char   L1 IBSS_ATTR;     //@- Alternate L register (L')
unsigned char   A1 IBSS_ATTR;     //@- Alternate A register (A')
unsigned char   F1 IBSS_ATTR;     //@- Alternate flags register (F')
unsigned        IX IBSS_ATTR;     //@- Index register X
unsigned        IY IBSS_ATTR;     //@- Index register Y
unsigned        PC IBSS_ATTR;     //@- Program counter
unsigned        SP IBSS_ATTR;     //@- Stack pointer
unsigned char   I  IBSS_ATTR;     //@- Interrupt register
unsigned char   R  IBSS_ATTR;     //@- Refresh register


/** Returns the 16 bit register BC. */
#define BC() (((unsigned)B << 8) | C)
#define DE() (((unsigned)D << 8) | E)
#define HL() (((unsigned)H << 8) | L)

/** 
    Returns the number of Z80 CPU cycles elapsed so far. 

    The cycle count is reset to zero when reset() is called, or
    it can be set to any value with setCycles(). It is updated after
    a CPU instruction is executed, for example by calling step()
    or interrupt().
*/
unsigned getCycles(void) {
    return cycles_;
}

/** Sets the CPU cycle counter to the specified value. */
void setCycles( unsigned value ) {
    cycles_ = value;
}

/** Returns the current interrupt mode. */
unsigned getInterruptMode(void) {
    return iflags_ & 0x03;
}

/** Sets the interrupt mode to the specified value. */
void setInterruptMode( unsigned mode );

/** Returns non-zero if the CPU is halted, otherwise zero. */
int isHalted(void) {
    return iflags_ & Halted;
}

/*
    Sets the interrupt mode to IM0, IM1 or IM2.
*/
void setInterruptMode( unsigned mode )
{
    if( mode <= 2 ) {
        iflags_ = (iflags_ & ~0x03) | mode;
    }
}

/*
    Calls a subroutine at the specified address.
*/
void callSub( unsigned addr )
{
    SP -= 2;
    writeWord( SP, PC ); // Save current program counter in the stack
    PC = addr & 0xFFFF; // Jump to the specified address
}

/*
    Decrements a byte value by one. 
    Note that this is different from subtracting one from the byte value,
    because flags behave differently.
*/
static inline unsigned char decByte( unsigned char b )
{
    F = Subtraction | (F & Carry); // Preserve the carry flag
    if( (b & 0x0F) == 0 ) F |= Halfcarry;
    --b;
    if( b == 0x7F ) F |= Overflow;
    if( b & 0x80 ) F |= Sign;
    if( b == 0 ) F |= Zero;

    return b;
}

/*
    Increments a byte value by one. 
    Note that this is different from adding one to the byte value,
    because flags behave differently.
*/
static inline unsigned char incByte( unsigned char b )
{
    ++b;
    F &= Carry; // Preserve the carry flag
    if( ! (b & 0x0F) ) F |= Halfcarry;
    if( b == 0x80 ) F |= Overflow;
    if( b & 0x80 ) F |= Sign;
    if( b == 0 ) F |= Zero;

    return b;
}

/*
    Reads one byte from port C, updating flags according to the rules of "IN r,(C)".
*/
static inline unsigned char inpReg(void)
{
    unsigned char   r = readPort( C );

    F = (F & Carry) | PSZ_[r];

    return r;
}

/*
    Performs a relative jump to the specified offset.
*/
static inline void relJump( unsigned char o )
{
    int offset = (int)((signed char)o);

    PC = (unsigned)((int)PC + offset) & 0xFFFF;
    cycles_++;
}

/*
    Returns from a subroutine, popping the saved Program Counter from the stack.
*/
static inline void retFromSub(void)
{
    PC = readWord( SP );
    SP += 2;
}

/*
    Rotates left one byte thru the carry flag.
*/
static inline unsigned char rotateLeft( unsigned char op )
{
    unsigned char f = F;

    F = 0;
    if( op & 0x80 ) F |= Carry;
    op <<= 1;
    if( f & Carry ) op |= 0x01;
    F |= PSZ_[op];
    
    return op;
}

/*
    Rotates left one byte copying the most significant bit (bit 7) in the carry flag.
*/
static inline unsigned char rotateLeftCarry( unsigned char op )
{
    F = 0;
    if( op & 0x80 ) F |= Carry;
    op = (op << 1) | (op >> 7);
    F |= PSZ_[op];
    
    return op;
}

/*
    Rotates right one byte thru the carry flag.
*/
static inline unsigned char rotateRight( unsigned char op )
{
    unsigned char f = F;

    F = 0;
    if( op & 0x01 ) F |= Carry;
    op >>= 1;
    if( f & Carry ) op |= 0x80;
    F |= PSZ_[op];
    
    return op;
}

/*
    Rotates right one byte copying the least significant bit (bit 0) in the carry flag.
*/
static inline unsigned char rotateRightCarry( unsigned char op )
{
    F = 0;
    if( op & 0x01 ) F |= Carry;
    op = (op >> 1) | (op << 7);
    F |= PSZ_[op];
    
    return op;
}

/*
    Shifts left one byte.
*/
static inline unsigned char shiftLeft( unsigned char op )
{
    F = 0;
    if( op & 0x80 ) F |= Carry;
    op <<= 1;
    F |= PSZ_[op];
    
    return op;
}

/*
    Shifts right one byte, preserving its sign (most significant bit).
*/
static inline unsigned char shiftRightArith( unsigned char op )
{
    F = 0;
    if( op & 0x01 ) F |= Carry;
    op = (op >> 1) | (op & 0x80);

    F |= PSZ_[op];
    
    return op;
}

/*
    Shifts right one byte.
*/
static inline unsigned char shiftRightLogical( unsigned char op )
{
    F = 0;
    if( op & 0x01 ) F |= Carry;
    op >>= 1;

    F |= PSZ_[op];
    
    return op;
}

/*
    Tests whether the specified bit of op is set.
*/
static inline void testBit( unsigned char bit, unsigned char op )
{
    // Flags for a bit test operation are:
    // S, P: unknown
    // Z: set if bit is zero, reset otherwise
    // N: reset
    // H: set
    // C: unaffected
    // However, it seems that parity is always set like Z, so we emulate that as well.
    F = (F & (Carry | Sign)) | Halfcarry;

    if( (op & (1 << bit)) == 0 ) {
        // Bit is not set, so set the zero flag
        F |= Zero | Parity;
    }
}

/*
    Adds the specified byte op to the accumulator, adding
    carry.
*/
static inline void addByte( unsigned char op, unsigned char cf )
{
    unsigned    x = A + op;

    if( cf ) x++; // Add carry

    F = 0;
    if( !(x & 0xFF) ) F |= Zero;
    if( x & 0x80 ) F |= Sign;
    if( x >= 0x100 ) F |= Carry;

    /*
        Halfcarry is set on carry from the low order four bits.

        To see how to compute it, let's take a look at the following table, which
        shows the binary addition of two binary numbers:

        A   B   A+B
        -----------
        0   0   0
        0   1   1
        1   0   1
        1   1   0

        Note that if only the lowest bit is used, then A+B, A-B and A^B yield the same 
        value. If we know A, B and the sum A+B+C, then C is easily derived:
            C = A+B+C - A - B,  that is
            C = A+B+C ^ A ^ B.

        For the halfcarry, A and B above are the fifth bit of a byte, which corresponds
        to the value 0x10. So:

            Halfcarry = ((accumulator+operand+halfcarry) ^ accumulator ^ operand) & 0x10

        Note that masking off all bits but one is important because we have worked all
        the math by using one bit only.
    */
    if( (A ^ op ^ x) & 0x10 ) F |= Halfcarry;

    /*
        The overflow bit is set when the result is too large to fit into the destination
        register, causing a change in the sign bit.

        For a sum, we can only have overflow when adding two numbers that are both positive
        or both negative. For example 0x5E + 0x4B (94 + 75) yields 0xA9 (169), which fits
        into an 8-bit register only if it is interpreted as an unsigned number. If we 
        consider the result as a signed integer, then 0xA9 corresponds to decimal -87 and
        we have overflow.
        Note that if we add two signed numbers of opposite sign then we cannot overflow
        the destination register, because the absolute value of the result will always fit
        in 7 bits, leaving the most significant bit free for use as a sign bit.

        We can code all the above concisely by noting that:

            ~(A ^ op) & 0x80

        is true if and only if A and op have the same sign. Also:

            (x ^ op) & 0x80

        is true if and only if the sum of A and op has taken a sign opposite to that
        of its operands.

        Thus the expression:

            ~(A ^ op) & (x ^ op) & 0x80

        reads "A has the same sign as op, and the opposite as x", where x is the sum of
        A and op (and an optional carry).
    */
    if( ~(A ^ op) & (x ^ op) & 0x80 ) F |= Overflow;

    A = x;
}

/*
    Subtracts the specified byte op from the accumulator, using carry as
    borrow from a previous operation.
*/
static inline unsigned char subByte( unsigned char op, unsigned char cf )
{
    unsigned char   x = A - op;

    if( cf ) x--;

    F = Subtraction;
    if( x == 0 ) F |= Zero;
    if( x & 0x80 ) F |= Sign;
    if( (x >= A) && (op | cf)) F |= Carry;

    // See addByte() for an explanation of the halfcarry bit
    if( (A ^ op ^ x) & 0x10 ) F |= Halfcarry;

    // See addByte() for an explanation of the overflow bit. The only difference here
    // is that for a subtraction we must check that the two operands have different
    // sign, because in fact A-B is A+(-B). Note however that since subtraction is not
    // symmetric, we have to use (x ^ A) to get the correct result, whereas for the
    // addition (x ^ A) is equivalent to (x ^ op)
    if( (A ^ op) & (x ^ A) & 0x80 ) F |= Overflow;

    return x;
}

static inline unsigned addDispl( unsigned addr, unsigned char displ ) {
    return (unsigned)((int)addr + (int)(signed char)displ);
}

/** Compares the accumulator and the specified operand (CP op) */
static inline void cmpByte( unsigned char op ) {
    subByte( op, 0 );
}

/** Fetches a byte from the program counter location */
static inline unsigned char fetchByte(void) {
    return readByte( PC++ );
}

/** Fetches a 16 bit word from the program counter location */
static inline unsigned fetchWord(void) {
    unsigned x = readWord( PC );
    PC += 2;
    return x;
}

/** Sets the parity, sign and zero flags from the accumulator value */
static inline void setFlagsPSZ(void) {
    F = Halfcarry | PSZ_[A];
}

/** Sets the parity, sign, zero, 3rd and 5th flag bits from the accumulator value */
static inline void setFlags35PSZ(void) {
    F = (F & (Carry | Halfcarry | Subtraction)) | PSZ_[A];
}

/** */
static inline void setFlags35PSZ000(void) {
    F = PSZ_[A];
}

/* Resets the CPU */
void z80_reset()
{
    PC = 0;         // Program counter is zero
    I = 0;          // Interrupt register cleared
    R = 0;          // Memory refresh register cleared
    iflags_ = 0;    // IFF1 and IFF2 cleared, IM0 enabled
    cycles_ = 0;    // Could that be 2 (according to some Zilog docs)?

    // There is no official documentation for the following!
    B = B1 = 0; 
    C = C1 = 0;
    D = D1 = 0; 
    E = E1 = 0;
    H = H1 = 0;
    L = L1 = 0;
    A = A1 = 0;
    F = F1 = 0;
    IX = 0;
    IY = 0;
    SP = 0xF000;
}

unsigned z80_getSizeOfSnapshotBuffer(void)
{
    unsigned result =
        8*2 +   // 8-bit registers
        1 +     // I
        1 +     // R
        2 +     // IX
        2 +     // IY
        2 +     // PC
        2 +     // SP
        4 +     // iflags_
        4;      // cycles_    

    return result;
}

static unsigned saveUint16( unsigned char * buffer, unsigned u )
{
    *buffer++ = (unsigned char) (u >> 8);
    *buffer   = (unsigned char) (u);

    return 2;
}

unsigned z80_takeSnapshot( unsigned char * buffer )
{
    unsigned char * buf = buffer;

    *buf++ = A; *buf++ = A1;
    *buf++ = B; *buf++ = B1;
    *buf++ = C; *buf++ = C1;
    *buf++ = D; *buf++ = D1;
    *buf++ = E; *buf++ = E1;
    *buf++ = H; *buf++ = H1;
    *buf++ = L; *buf++ = L1;
    *buf++ = F; *buf++ = F1;

    *buf++ = I;
    *buf++ = R;

    buf += saveUint16( buf, IX );
    buf += saveUint16( buf, IY );
    buf += saveUint16( buf, PC );
    buf += saveUint16( buf, SP );

    buf += saveUint16( buf, iflags_ >> 16 );
    buf += saveUint16( buf, iflags_ );
    buf += saveUint16( buf, cycles_ >> 16 );
    buf += saveUint16( buf, cycles_ );

    return buffer - buf;
}

static unsigned loadUint16( unsigned char ** buffer )
{
    unsigned char * buf = *buffer;
    unsigned result = *buf++;

    result = (result << 8) | *buf++;

    *buffer = buf;

    return result;
}

unsigned z80_restoreSnapshot( unsigned char * buffer )
{
    unsigned char * buf = buffer;

    A = *buf++; A1 = *buf++;
    B = *buf++; B1 = *buf++;
    C = *buf++; C1 = *buf++;
    D = *buf++; D1 = *buf++;
    E = *buf++; E1 = *buf++;
    H = *buf++; H1 = *buf++;
    L = *buf++; L1 = *buf++;
    F = *buf++; F1 = *buf++;

    I = *buf++;
    R = *buf++;

    IX = loadUint16( &buf );
    IY = loadUint16( &buf );
    PC = loadUint16( &buf );
    SP = loadUint16( &buf );

    iflags_ = loadUint16( &buf );
    iflags_ = (iflags_ << 16) | loadUint16(&buf);
    cycles_ = loadUint16( &buf );
    cycles_ = (cycles_ << 16) | loadUint16(&buf);

    return buf - buffer;
}

OpcodeInfo OpInfoCB_[256] = {
    { &opcode_cb_00,  8 }, // RLC B
    { &opcode_cb_01,  8 }, // RLC C
    { &opcode_cb_02,  8 }, // RLC D
    { &opcode_cb_03,  8 }, // RLC E
    { &opcode_cb_04,  8 }, // RLC H
    { &opcode_cb_05,  8 }, // RLC L
    { &opcode_cb_06, 15 }, // RLC (HL)
    { &opcode_cb_07,  8 }, // RLC A
    { &opcode_cb_08,  8 }, // RRC B
    { &opcode_cb_09,  8 }, // RRC C
    { &opcode_cb_0a,  8 }, // RRC D
    { &opcode_cb_0b,  8 }, // RRC E
    { &opcode_cb_0c,  8 }, // RRC H
    { &opcode_cb_0d,  8 }, // RRC L
    { &opcode_cb_0e, 15 }, // RRC (HL)
    { &opcode_cb_0f,  8 }, // RRC A
    { &opcode_cb_10,  8 }, // RL B
    { &opcode_cb_11,  8 }, // RL C
    { &opcode_cb_12,  8 }, // RL D
    { &opcode_cb_13,  8 }, // RL E
    { &opcode_cb_14,  8 }, // RL H
    { &opcode_cb_15,  8 }, // RL L
    { &opcode_cb_16, 15 }, // RL (HL)
    { &opcode_cb_17,  8 }, // RL A
    { &opcode_cb_18,  8 }, // RR B
    { &opcode_cb_19,  8 }, // RR C
    { &opcode_cb_1a,  8 }, // RR D
    { &opcode_cb_1b,  8 }, // RR E
    { &opcode_cb_1c,  8 }, // RR H
    { &opcode_cb_1d,  8 }, // RR L
    { &opcode_cb_1e, 15 }, // RR (HL)
    { &opcode_cb_1f,  8 }, // RR A
    { &opcode_cb_20,  8 }, // SLA B
    { &opcode_cb_21,  8 }, // SLA C
    { &opcode_cb_22,  8 }, // SLA D
    { &opcode_cb_23,  8 }, // SLA E
    { &opcode_cb_24,  8 }, // SLA H
    { &opcode_cb_25,  8 }, // SLA L
    { &opcode_cb_26, 15 }, // SLA (HL)
    { &opcode_cb_27,  8 }, // SLA A
    { &opcode_cb_28,  8 }, // SRA B
    { &opcode_cb_29,  8 }, // SRA C
    { &opcode_cb_2a,  8 }, // SRA D
    { &opcode_cb_2b,  8 }, // SRA E
    { &opcode_cb_2c,  8 }, // SRA H
    { &opcode_cb_2d,  8 }, // SRA L
    { &opcode_cb_2e, 15 }, // SRA (HL)
    { &opcode_cb_2f,  8 }, // SRA A
    { &opcode_cb_30,  8 }, // SLL B 
    { &opcode_cb_31,  8 }, // SLL C 
    { &opcode_cb_32,  8 }, // SLL D 
    { &opcode_cb_33,  8 }, // SLL E 
    { &opcode_cb_34,  8 }, // SLL H 
    { &opcode_cb_35,  8 }, // SLL L 
    { &opcode_cb_36, 15 }, // SLL (HL)
    { &opcode_cb_37,  8 }, // SLL A
    { &opcode_cb_38,  8 }, // SRL B
    { &opcode_cb_39,  8 }, // SRL C
    { &opcode_cb_3a,  8 }, // SRL D
    { &opcode_cb_3b,  8 }, // SRL E
    { &opcode_cb_3c,  8 }, // SRL H
    { &opcode_cb_3d,  8 }, // SRL L
    { &opcode_cb_3e, 15 }, // SRL (HL)
    { &opcode_cb_3f,  8 }, // SRL A
    { &opcode_cb_40,  8 }, // BIT 0, B
    { &opcode_cb_41,  8 }, // BIT 0, C
    { &opcode_cb_42,  8 }, // BIT 0, D
    { &opcode_cb_43,  8 }, // BIT 0, E
    { &opcode_cb_44,  8 }, // BIT 0, H
    { &opcode_cb_45,  8 }, // BIT 0, L
    { &opcode_cb_46, 12 }, // BIT 0, (HL)
    { &opcode_cb_47,  8 }, // BIT 0, A
    { &opcode_cb_48,  8 }, // BIT 1, B
    { &opcode_cb_49,  8 }, // BIT 1, C
    { &opcode_cb_4a,  8 }, // BIT 1, D
    { &opcode_cb_4b,  8 }, // BIT 1, E
    { &opcode_cb_4c,  8 }, // BIT 1, H
    { &opcode_cb_4d,  8 }, // BIT 1, L
    { &opcode_cb_4e, 12 }, // BIT 1, (HL)
    { &opcode_cb_4f,  8 }, // BIT 1, A
    { &opcode_cb_50,  8 }, // BIT 2, B
    { &opcode_cb_51,  8 }, // BIT 2, C
    { &opcode_cb_52,  8 }, // BIT 2, D
    { &opcode_cb_53,  8 }, // BIT 2, E
    { &opcode_cb_54,  8 }, // BIT 2, H
    { &opcode_cb_55,  8 }, // BIT 2, L
    { &opcode_cb_56, 12 }, // BIT 2, (HL)
    { &opcode_cb_57,  8 }, // BIT 2, A
    { &opcode_cb_58,  8 }, // BIT 3, B
    { &opcode_cb_59,  8 }, // BIT 3, C
    { &opcode_cb_5a,  8 }, // BIT 3, D
    { &opcode_cb_5b,  8 }, // BIT 3, E
    { &opcode_cb_5c,  8 }, // BIT 3, H
    { &opcode_cb_5d,  8 }, // BIT 3, L
    { &opcode_cb_5e, 12 }, // BIT 3, (HL)
    { &opcode_cb_5f,  8 }, // BIT 3, A
    { &opcode_cb_60,  8 }, // BIT 4, B
    { &opcode_cb_61,  8 }, // BIT 4, C
    { &opcode_cb_62,  8 }, // BIT 4, D
    { &opcode_cb_63,  8 }, // BIT 4, E
    { &opcode_cb_64,  8 }, // BIT 4, H
    { &opcode_cb_65,  8 }, // BIT 4, L
    { &opcode_cb_66, 12 }, // BIT 4, (HL)
    { &opcode_cb_67,  8 }, // BIT 4, A
    { &opcode_cb_68,  8 }, // BIT 5, B
    { &opcode_cb_69,  8 }, // BIT 5, C
    { &opcode_cb_6a,  8 }, // BIT 5, D
    { &opcode_cb_6b,  8 }, // BIT 5, E
    { &opcode_cb_6c,  8 }, // BIT 5, H
    { &opcode_cb_6d,  8 }, // BIT 5, L
    { &opcode_cb_6e, 12 }, // BIT 5, (HL)
    { &opcode_cb_6f,  8 }, // BIT 5, A
    { &opcode_cb_70,  8 }, // BIT 6, B
    { &opcode_cb_71,  8 }, // BIT 6, C
    { &opcode_cb_72,  8 }, // BIT 6, D
    { &opcode_cb_73,  8 }, // BIT 6, E
    { &opcode_cb_74,  8 }, // BIT 6, H
    { &opcode_cb_75,  8 }, // BIT 6, L
    { &opcode_cb_76, 12 }, // BIT 6, (HL)
    { &opcode_cb_77,  8 }, // BIT 6, A
    { &opcode_cb_78,  8 }, // BIT 7, B
    { &opcode_cb_79,  8 }, // BIT 7, C
    { &opcode_cb_7a,  8 }, // BIT 7, D
    { &opcode_cb_7b,  8 }, // BIT 7, E
    { &opcode_cb_7c,  8 }, // BIT 7, H
    { &opcode_cb_7d,  8 }, // BIT 7, L
    { &opcode_cb_7e, 12 }, // BIT 7, (HL)
    { &opcode_cb_7f,  8 }, // BIT 7, A
    { &opcode_cb_80,  8 }, // RES 0, B
    { &opcode_cb_81,  8 }, // RES 0, C
    { &opcode_cb_82,  8 }, // RES 0, D
    { &opcode_cb_83,  8 }, // RES 0, E
    { &opcode_cb_84,  8 }, // RES 0, H
    { &opcode_cb_85,  8 }, // RES 0, L
    { &opcode_cb_86, 15 }, // RES 0, (HL)
    { &opcode_cb_87,  8 }, // RES 0, A
    { &opcode_cb_88,  8 }, // RES 1, B
    { &opcode_cb_89,  8 }, // RES 1, C
    { &opcode_cb_8a,  8 }, // RES 1, D
    { &opcode_cb_8b,  8 }, // RES 1, E
    { &opcode_cb_8c,  8 }, // RES 1, H
    { &opcode_cb_8d,  8 }, // RES 1, L
    { &opcode_cb_8e, 15 }, // RES 1, (HL)
    { &opcode_cb_8f,  8 }, // RES 1, A
    { &opcode_cb_90,  8 }, // RES 2, B
    { &opcode_cb_91,  8 }, // RES 2, C
    { &opcode_cb_92,  8 }, // RES 2, D
    { &opcode_cb_93,  8 }, // RES 2, E
    { &opcode_cb_94,  8 }, // RES 2, H
    { &opcode_cb_95,  8 }, // RES 2, L
    { &opcode_cb_96, 15 }, // RES 2, (HL)
    { &opcode_cb_97,  8 }, // RES 2, A
    { &opcode_cb_98,  8 }, // RES 3, B
    { &opcode_cb_99,  8 }, // RES 3, C
    { &opcode_cb_9a,  8 }, // RES 3, D
    { &opcode_cb_9b,  8 }, // RES 3, E
    { &opcode_cb_9c,  8 }, // RES 3, H
    { &opcode_cb_9d,  8 }, // RES 3, L
    { &opcode_cb_9e, 15 }, // RES 3, (HL)
    { &opcode_cb_9f,  8 }, // RES 3, A
    { &opcode_cb_a0,  8 }, // RES 4, B
    { &opcode_cb_a1,  8 }, // RES 4, C
    { &opcode_cb_a2,  8 }, // RES 4, D
    { &opcode_cb_a3,  8 }, // RES 4, E
    { &opcode_cb_a4,  8 }, // RES 4, H
    { &opcode_cb_a5,  8 }, // RES 4, L
    { &opcode_cb_a6, 15 }, // RES 4, (HL)
    { &opcode_cb_a7,  8 }, // RES 4, A
    { &opcode_cb_a8,  8 }, // RES 5, B
    { &opcode_cb_a9,  8 }, // RES 5, C
    { &opcode_cb_aa,  8 }, // RES 5, D
    { &opcode_cb_ab,  8 }, // RES 5, E
    { &opcode_cb_ac,  8 }, // RES 5, H
    { &opcode_cb_ad,  8 }, // RES 5, L
    { &opcode_cb_ae, 15 }, // RES 5, (HL)
    { &opcode_cb_af,  8 }, // RES 5, A
    { &opcode_cb_b0,  8 }, // RES 6, B
    { &opcode_cb_b1,  8 }, // RES 6, C
    { &opcode_cb_b2,  8 }, // RES 6, D
    { &opcode_cb_b3,  8 }, // RES 6, E
    { &opcode_cb_b4,  8 }, // RES 6, H
    { &opcode_cb_b5,  8 }, // RES 6, L
    { &opcode_cb_b6, 15 }, // RES 6, (HL)
    { &opcode_cb_b7,  8 }, // RES 6, A
    { &opcode_cb_b8,  8 }, // RES 7, B
    { &opcode_cb_b9,  8 }, // RES 7, C
    { &opcode_cb_ba,  8 }, // RES 7, D
    { &opcode_cb_bb,  8 }, // RES 7, E
    { &opcode_cb_bc,  8 }, // RES 7, H
    { &opcode_cb_bd,  8 }, // RES 7, L
    { &opcode_cb_be, 15 }, // RES 7, (HL)
    { &opcode_cb_bf,  8 }, // RES 7, A
    { &opcode_cb_c0,  8 }, // SET 0, B
    { &opcode_cb_c1,  8 }, // SET 0, C
    { &opcode_cb_c2,  8 }, // SET 0, D
    { &opcode_cb_c3,  8 }, // SET 0, E
    { &opcode_cb_c4,  8 }, // SET 0, H
    { &opcode_cb_c5,  8 }, // SET 0, L
    { &opcode_cb_c6, 15 }, // SET 0, (HL)
    { &opcode_cb_c7,  8 }, // SET 0, A
    { &opcode_cb_c8,  8 }, // SET 1, B
    { &opcode_cb_c9,  8 }, // SET 1, C
    { &opcode_cb_ca,  8 }, // SET 1, D
    { &opcode_cb_cb,  8 }, // SET 1, E
    { &opcode_cb_cc,  8 }, // SET 1, H
    { &opcode_cb_cd,  8 }, // SET 1, L
    { &opcode_cb_ce, 15 }, // SET 1, (HL)
    { &opcode_cb_cf,  8 }, // SET 1, A
    { &opcode_cb_d0,  8 }, // SET 2, B
    { &opcode_cb_d1,  8 }, // SET 2, C
    { &opcode_cb_d2,  8 }, // SET 2, D
    { &opcode_cb_d3,  8 }, // SET 2, E
    { &opcode_cb_d4,  8 }, // SET 2, H
    { &opcode_cb_d5,  8 }, // SET 2, L
    { &opcode_cb_d6, 15 }, // SET 2, (HL)
    { &opcode_cb_d7,  8 }, // SET 2, A
    { &opcode_cb_d8,  8 }, // SET 3, B
    { &opcode_cb_d9,  8 }, // SET 3, C
    { &opcode_cb_da,  8 }, // SET 3, D
    { &opcode_cb_db,  8 }, // SET 3, E
    { &opcode_cb_dc,  8 }, // SET 3, H
    { &opcode_cb_dd,  8 }, // SET 3, L
    { &opcode_cb_de, 15 }, // SET 3, (HL)
    { &opcode_cb_df,  8 }, // SET 3, A
    { &opcode_cb_e0,  8 }, // SET 4, B
    { &opcode_cb_e1,  8 }, // SET 4, C
    { &opcode_cb_e2,  8 }, // SET 4, D
    { &opcode_cb_e3,  8 }, // SET 4, E
    { &opcode_cb_e4,  8 }, // SET 4, H
    { &opcode_cb_e5,  8 }, // SET 4, L
    { &opcode_cb_e6, 15 }, // SET 4, (HL)
    { &opcode_cb_e7,  8 }, // SET 4, A
    { &opcode_cb_e8,  8 }, // SET 5, B
    { &opcode_cb_e9,  8 }, // SET 5, C
    { &opcode_cb_ea,  8 }, // SET 5, D
    { &opcode_cb_eb,  8 }, // SET 5, E
    { &opcode_cb_ec,  8 }, // SET 5, H
    { &opcode_cb_ed,  8 }, // SET 5, L
    { &opcode_cb_ee, 15 }, // SET 5, (HL)
    { &opcode_cb_ef,  8 }, // SET 5, A
    { &opcode_cb_f0,  8 }, // SET 6, B
    { &opcode_cb_f1,  8 }, // SET 6, C
    { &opcode_cb_f2,  8 }, // SET 6, D
    { &opcode_cb_f3,  8 }, // SET 6, E
    { &opcode_cb_f4,  8 }, // SET 6, H
    { &opcode_cb_f5,  8 }, // SET 6, L
    { &opcode_cb_f6, 15 }, // SET 6, (HL)
    { &opcode_cb_f7,  8 }, // SET 6, A
    { &opcode_cb_f8,  8 }, // SET 7, B
    { &opcode_cb_f9,  8 }, // SET 7, C
    { &opcode_cb_fa,  8 }, // SET 7, D
    { &opcode_cb_fb,  8 }, // SET 7, E
    { &opcode_cb_fc,  8 }, // SET 7, H
    { &opcode_cb_fd,  8 }, // SET 7, L
    { &opcode_cb_fe, 15 }, // SET 7, (HL)
    { &opcode_cb_ff,  8 }  // SET 7, A
};
    
void opcode_cb_00()    // RLC B
{
    B = rotateLeftCarry( B );    
}

void opcode_cb_01()    // RLC C
{
    C = rotateLeftCarry( C );    
}

void opcode_cb_02()    // RLC D
{
    D = rotateLeftCarry( D );    
}

void opcode_cb_03()    // RLC E
{
    E = rotateLeftCarry( E );    
}

void opcode_cb_04()    // RLC H
{
    H = rotateLeftCarry( H );    
}

void opcode_cb_05()    // RLC L
{
    L = rotateLeftCarry( L );
}

void opcode_cb_06()    // RLC (HL)
{
    writeByte( HL(), rotateLeftCarry( readByte( HL() ) ) );
}

void opcode_cb_07()    // RLC A
{
    A = rotateLeftCarry( A );
}

void opcode_cb_08()    // RRC B
{
    B = rotateRightCarry( B );
}

void opcode_cb_09()    // RRC C
{
    C = rotateLeftCarry( C );
}

void opcode_cb_0a()    // RRC D
{
    D = rotateLeftCarry( D );
}

void opcode_cb_0b()    // RRC E
{
    E = rotateLeftCarry( E );
}

void opcode_cb_0c()    // RRC H
{
    H = rotateLeftCarry( H );
}

void opcode_cb_0d()    // RRC L
{
    L = rotateLeftCarry( L );
}

void opcode_cb_0e()    // RRC (HL)
{
    writeByte( HL(), rotateRightCarry( readByte( HL() ) ) );    
}

void opcode_cb_0f()    // RRC A
{
    A = rotateLeftCarry( A );
}

void opcode_cb_10()    // RL B
{
    B = rotateLeft( B );    
}

void opcode_cb_11()    // RL C
{
    C = rotateLeft( C );    
}

void opcode_cb_12()    // RL D
{
    D = rotateLeft( D );    
}

void opcode_cb_13()    // RL E
{
    E = rotateLeft( E );
}

void opcode_cb_14()    // RL H
{
    H = rotateLeft( H );    
}

void opcode_cb_15()    // RL L
{
    L = rotateLeft( L );    
}

void opcode_cb_16()    // RL (HL)
{
    writeByte( HL(), rotateLeft( readByte( HL() ) ) );    
}

void opcode_cb_17()    // RL A
{
    A = rotateLeft( A ); 
}

void opcode_cb_18()    // RR B
{
    B = rotateRight( B ); 
}

void opcode_cb_19()    // RR C
{
    C = rotateRight( C ); 
}

void opcode_cb_1a()    // RR D
{
    D = rotateRight( D ); 
}

void opcode_cb_1b()    // RR E
{
    E = rotateRight( E ); 
}

void opcode_cb_1c()    // RR H
{
    H = rotateRight( H ); 
}

void opcode_cb_1d()    // RR L
{
    L = rotateRight( L ); 
}

void opcode_cb_1e()    // RR (HL)
{
    writeByte( HL(), rotateRight( readByte( HL() ) ) );    
}

void opcode_cb_1f()    // RR A
{
    A = rotateRight( A ); 
}

void opcode_cb_20()    // SLA B
{
    B = shiftLeft( B );
}

void opcode_cb_21()    // SLA C
{
    C = shiftLeft( C );
}

void opcode_cb_22()    // SLA D
{
    D = shiftLeft( D );
}

void opcode_cb_23()    // SLA E
{
    E = shiftLeft( E );
}

void opcode_cb_24()    // SLA H
{
    H = shiftLeft( H );
}

void opcode_cb_25()    // SLA L
{
    L = shiftLeft( L );
}

void opcode_cb_26()    // SLA (HL)
{
    writeByte( HL(), shiftLeft( readByte( HL() ) ) );
}

void opcode_cb_27()    // SLA A
{
    A = shiftLeft( A );
}

void opcode_cb_28()    // SRA B
{
    B = shiftRightArith( B );
}

void opcode_cb_29()    // SRA C
{
    C = shiftRightArith( C );
}

void opcode_cb_2a()    // SRA D
{
    D = shiftRightArith( D );
}

void opcode_cb_2b()    // SRA E
{
    E = shiftRightArith( E );
}

void opcode_cb_2c()    // SRA H
{
    H = shiftRightArith( H );
}

void opcode_cb_2d()    // SRA L
{
    L = shiftRightArith( L );
}

void opcode_cb_2e()    // SRA (HL)
{
    writeByte( HL(), shiftRightArith( readByte( HL() ) ) );
}

void opcode_cb_2f()    // SRA A
{
    A = shiftRightArith( A );
}

void opcode_cb_30()    // SLL B
{
    B = shiftLeft( B ) | 0x01;
}

void opcode_cb_31()    // SLL C
{
    C = shiftLeft( C ) | 0x01;
}

void opcode_cb_32()    // SLL D
{
    D = shiftLeft( D ) | 0x01;
}

void opcode_cb_33()    // SLL E
{
    E = shiftLeft( E ) | 0x01;
}

void opcode_cb_34()    // SLL H
{
    H = shiftLeft( H ) | 0x01;
}

void opcode_cb_35()    // SLL L
{
    L = shiftLeft( L ) | 0x01;
}

void opcode_cb_36()    // SLL (HL)
{
    writeByte( HL(), shiftLeft( readByte( HL() ) ) | 0x01 );
}

void opcode_cb_37()    // SLL A
{
    A = shiftLeft( A ) | 0x01;
}

void opcode_cb_38()    // SRL B
{
    B = shiftRightLogical( B );
}

void opcode_cb_39()    // SRL C
{
    C = shiftRightLogical( C );
}

void opcode_cb_3a()    // SRL D
{
    D = shiftRightLogical( D );
}

void opcode_cb_3b()    // SRL E
{
    E = shiftRightLogical( E );
}

void opcode_cb_3c()    // SRL H
{
    H = shiftRightLogical( H );
}

void opcode_cb_3d()    // SRL L
{
    L = shiftRightLogical( L );
}

void opcode_cb_3e()    // SRL (HL)
{
    writeByte( HL(), shiftRightLogical( readByte( HL() ) ) );
}

void opcode_cb_3f()    // SRL A
{
    A = shiftRightLogical( A );
}

void opcode_cb_40()    // BIT 0, B
{
    testBit( 0, B );
}

void opcode_cb_41()    // BIT 0, C
{
    testBit( 0, C );
}

void opcode_cb_42()    // BIT 0, D
{
    testBit( 0, D );
}

void opcode_cb_43()    // BIT 0, E
{
    testBit( 0, E );
}

void opcode_cb_44()    // BIT 0, H
{
    testBit( 0, H );
}

void opcode_cb_45()    // BIT 0, L
{
    testBit( 0, L );
}

void opcode_cb_46()    // BIT 0, (HL)
{
    testBit( 0, readByte( HL() ) );
}

void opcode_cb_47()    // BIT 0, A
{
    testBit( 0, A );
}

void opcode_cb_48()    // BIT 1, B
{
    testBit( 1, B );
}

void opcode_cb_49()    // BIT 1, C
{
    testBit( 1, C );
}

void opcode_cb_4a()    // BIT 1, D
{
    testBit( 1, D );
}

void opcode_cb_4b()    // BIT 1, E
{
    testBit( 1, E );
}

void opcode_cb_4c()    // BIT 1, H
{
    testBit( 1, H );
}

void opcode_cb_4d()    // BIT 1, L
{
    testBit( 1, L );
}

void opcode_cb_4e()    // BIT 1, (HL)
{
    testBit( 1, readByte( HL() ) );
}

void opcode_cb_4f()    // BIT 1, A
{
    testBit( 1, A );
}

void opcode_cb_50()    // BIT 2, B
{
    testBit( 2, B );
}

void opcode_cb_51()    // BIT 2, C
{
    testBit( 2, C );
}

void opcode_cb_52()    // BIT 2, D
{
    testBit( 2, D );
}

void opcode_cb_53()    // BIT 2, E
{
    testBit( 2, E );
}

void opcode_cb_54()    // BIT 2, H
{
    testBit( 2, H );
}

void opcode_cb_55()    // BIT 2, L
{
    testBit( 2, L );
}

void opcode_cb_56()    // BIT 2, (HL)
{
    testBit( 2, readByte( HL() ) );
}

void opcode_cb_57()    // BIT 2, A
{
    testBit( 2, A );
}

void opcode_cb_58()    // BIT 3, B
{
    testBit( 3, B );
}

void opcode_cb_59()    // BIT 3, C
{
    testBit( 3, C );
}

void opcode_cb_5a()    // BIT 3, D
{
    testBit( 3, D );
}

void opcode_cb_5b()    // BIT 3, E
{
    testBit( 3, E );
}

void opcode_cb_5c()    // BIT 3, H
{
    testBit( 3, H );
}

void opcode_cb_5d()    // BIT 3, L
{
    testBit( 3, L );
}

void opcode_cb_5e()    // BIT 3, (HL)
{
    testBit( 3, readByte( HL() ) );
}

void opcode_cb_5f()    // BIT 3, A
{
    testBit( 3, A );
}

void opcode_cb_60()    // BIT 4, B
{
    testBit( 4, B );
}

void opcode_cb_61()    // BIT 4, C
{
    testBit( 4, C );
}

void opcode_cb_62()    // BIT 4, D
{
    testBit( 4, D );
}

void opcode_cb_63()    // BIT 4, E
{
    testBit( 4, E );
}

void opcode_cb_64()    // BIT 4, H
{
    testBit( 4, H );
}

void opcode_cb_65()    // BIT 4, L
{
    testBit( 4, L );
}

void opcode_cb_66()    // BIT 4, (HL)
{
    testBit( 4, readByte( HL() ) );
}

void opcode_cb_67()    // BIT 4, A
{
    testBit( 4, A );
}

void opcode_cb_68()    // BIT 5, B
{
    testBit( 5, B );
}

void opcode_cb_69()    // BIT 5, C
{
    testBit( 5, C );
}

void opcode_cb_6a()    // BIT 5, D
{
    testBit( 5, D );
}

void opcode_cb_6b()    // BIT 5, E
{
    testBit( 5, E );
}

void opcode_cb_6c()    // BIT 5, H
{
    testBit( 5, H );
}

void opcode_cb_6d()    // BIT 5, L
{
    testBit( 5, L );
}

void opcode_cb_6e()    // BIT 5, (HL)
{
    testBit( 5, readByte( HL() ) );
}

void opcode_cb_6f()    // BIT 5, A
{
    testBit( 5, A );
}

void opcode_cb_70()    // BIT 6, B
{
    testBit( 6, B );
}

void opcode_cb_71()    // BIT 6, C
{
    testBit( 6, C );
}

void opcode_cb_72()    // BIT 6, D
{
    testBit( 6, D );
}

void opcode_cb_73()    // BIT 6, E
{
    testBit( 6, E );
}

void opcode_cb_74()    // BIT 6, H
{
    testBit( 6, H );
}

void opcode_cb_75()    // BIT 6, L
{
    testBit( 6, L );
}

void opcode_cb_76()    // BIT 6, (HL)
{
    testBit( 6, readByte( HL() ) );
}

void opcode_cb_77()    // BIT 6, A
{
    testBit( 6, A );
}

void opcode_cb_78()    // BIT 7, B
{
    testBit( 7, B );
}

void opcode_cb_79()    // BIT 7, C
{
    testBit( 7, C );
}

void opcode_cb_7a()    // BIT 7, D
{
    testBit( 7, D );
}

void opcode_cb_7b()    // BIT 7, E
{
    testBit( 7, E );
}

void opcode_cb_7c()    // BIT 7, H
{
    testBit( 7, H );
}

void opcode_cb_7d()    // BIT 7, L
{
    testBit( 7, L );
}

void opcode_cb_7e()    // BIT 7, (HL)
{
    testBit( 7, readByte( HL() ) );
}

void opcode_cb_7f()    // BIT 7, A
{
    testBit( 7, A );
}

void opcode_cb_80()    // RES 0, B
{
    B &= ~(unsigned char) (1 << 0);
}

void opcode_cb_81()    // RES 0, C
{
    C &= ~(unsigned char) (1 << 0);
}

void opcode_cb_82()    // RES 0, D
{
    D &= ~(unsigned char) (1 << 0);
}

void opcode_cb_83()    // RES 0, E
{
    E &= ~(unsigned char) (1 << 0);
}

void opcode_cb_84()    // RES 0, H
{
    H &= ~(unsigned char) (1 << 0);
}

void opcode_cb_85()    // RES 0, L
{
    L &= ~(unsigned char) (1 << 0);
}

void opcode_cb_86()    // RES 0, (HL)
{
    writeByte( HL(), readByte( HL() ) & (unsigned char) ~(unsigned char) (1 << 0) );
}

void opcode_cb_87()    // RES 0, A
{
    A &= ~(unsigned char) (1 << 0);
}

void opcode_cb_88()    // RES 1, B
{
    B &= ~(unsigned char) (1 << 1);
}

void opcode_cb_89()    // RES 1, C
{
    C &= ~(unsigned char) (1 << 1);
}

void opcode_cb_8a()    // RES 1, D
{
    D &= ~(unsigned char) (1 << 1);
}

void opcode_cb_8b()    // RES 1, E
{
    E &= ~(unsigned char) (1 << 1);
}

void opcode_cb_8c()    // RES 1, H
{
    H &= ~(unsigned char) (1 << 1);
}

void opcode_cb_8d()    // RES 1, L
{
    L &= ~(unsigned char) (1 << 1);
}

void opcode_cb_8e()    // RES 1, (HL)
{
    writeByte( HL(), readByte( HL() ) & (unsigned char) ~(unsigned char) (1 << 1) );
}

void opcode_cb_8f()    // RES 1, A
{
    A &= ~(unsigned char) (1 << 1);
}

void opcode_cb_90()    // RES 2, B
{
    B &= ~(unsigned char) (1 << 2);
}

void opcode_cb_91()    // RES 2, C
{
    C &= ~(unsigned char) (1 << 2);
}

void opcode_cb_92()    // RES 2, D
{
    D &= ~(unsigned char) (1 << 2);
}

void opcode_cb_93()    // RES 2, E
{
    E &= ~(unsigned char) (1 << 2);
}

void opcode_cb_94()    // RES 2, H
{
    H &= ~(unsigned char) (1 << 2);
}

void opcode_cb_95()    // RES 2, L
{
    L &= ~(unsigned char) (1 << 2);
}

void opcode_cb_96()    // RES 2, (HL)
{
    writeByte( HL(), readByte( HL() ) & (unsigned char) ~(unsigned char) (1 << 2) );
}

void opcode_cb_97()    // RES 2, A
{
    A &= ~(unsigned char) (1 << 2);
}

void opcode_cb_98()    // RES 3, B
{
    B &= ~(unsigned char) (1 << 3);
}

void opcode_cb_99()    // RES 3, C
{
    C &= ~(unsigned char) (1 << 3);
}

void opcode_cb_9a()    // RES 3, D
{
    D &= ~(unsigned char) (1 << 3);
}

void opcode_cb_9b()    // RES 3, E
{
    E &= ~(unsigned char) (1 << 3);
}

void opcode_cb_9c()    // RES 3, H
{
    H &= ~(unsigned char) (1 << 3);
}

void opcode_cb_9d()    // RES 3, L
{
    L &= ~(unsigned char) (1 << 3);
}

void opcode_cb_9e()    // RES 3, (HL)
{
    writeByte( HL(), readByte( HL() ) & (unsigned char) ~(unsigned char) (1 << 3) );
}

void opcode_cb_9f()    // RES 3, A
{
    A &= ~(unsigned char) (1 << 3);
}

void opcode_cb_a0()    // RES 4, B
{
    B &= ~(unsigned char) (1 << 4);
}

void opcode_cb_a1()    // RES 4, C
{
    C &= ~(unsigned char) (1 << 4);
}

void opcode_cb_a2()    // RES 4, D
{
    D &= ~(unsigned char) (1 << 4);
}

void opcode_cb_a3()    // RES 4, E
{
    E &= ~(unsigned char) (1 << 4);
}

void opcode_cb_a4()    // RES 4, H
{
    H &= ~(unsigned char) (1 << 4);
}

void opcode_cb_a5()    // RES 4, L
{
    L &= ~(unsigned char) (1 << 4);
}

void opcode_cb_a6()    // RES 4, (HL)
{
    writeByte( HL(), readByte( HL() ) & (unsigned char) ~(unsigned char) (1 << 4) );
}

void opcode_cb_a7()    // RES 4, A
{
    A &= ~(unsigned char) (1 << 4);
}

void opcode_cb_a8()    // RES 5, B
{
    B &= ~(unsigned char) (1 << 5);
}

void opcode_cb_a9()    // RES 5, C
{
    C &= ~(unsigned char) (1 << 5);
}

void opcode_cb_aa()    // RES 5, D
{
    D &= ~(unsigned char) (1 << 5);
}

void opcode_cb_ab()    // RES 5, E
{
    E &= ~(unsigned char) (1 << 5);
}

void opcode_cb_ac()    // RES 5, H
{
    H &= ~(unsigned char) (1 << 5);
}

void opcode_cb_ad()    // RES 5, L
{
    L &= ~(unsigned char) (1 << 5);
}

void opcode_cb_ae()    // RES 5, (HL)
{
    writeByte( HL(), readByte( HL() ) & (unsigned char) ~(unsigned char) (1 << 5) );
}

void opcode_cb_af()    // RES 5, A
{
    A &= ~(unsigned char) (1 << 5);
}

void opcode_cb_b0()    // RES 6, B
{
    B &= ~(unsigned char) (1 << 6);
}

void opcode_cb_b1()    // RES 6, C
{
    C &= ~(unsigned char) (1 << 6);
}

void opcode_cb_b2()    // RES 6, D
{
    D &= ~(unsigned char) (1 << 6);
}

void opcode_cb_b3()    // RES 6, E
{
    E &= ~(unsigned char) (1 << 6);
}

void opcode_cb_b4()    // RES 6, H
{
    H &= ~(unsigned char) (1 << 6);
}

void opcode_cb_b5()    // RES 6, L
{
    L &= ~(unsigned char) (1 << 6);
}

void opcode_cb_b6()    // RES 6, (HL)
{
    writeByte( HL(), readByte( HL() ) & (unsigned char) ~(unsigned char) (1 << 6) );
}

void opcode_cb_b7()    // RES 6, A
{
    A &= ~(unsigned char) (1 << 6);
}

void opcode_cb_b8()    // RES 7, B
{
    B &= ~(unsigned char) (1 << 7);
}

void opcode_cb_b9()    // RES 7, C
{
    C &= ~(unsigned char) (1 << 7);
}

void opcode_cb_ba()    // RES 7, D
{
    D &= ~(unsigned char) (1 << 7);
}

void opcode_cb_bb()    // RES 7, E
{
    E &= ~(unsigned char) (1 << 7);
}

void opcode_cb_bc()    // RES 7, H
{
    H &= ~(unsigned char) (1 << 7);
}

void opcode_cb_bd()    // RES 7, L
{
    L &= ~(unsigned char) (1 << 7);
}

void opcode_cb_be()    // RES 7, (HL)
{
    writeByte( HL(), readByte( HL() ) & (unsigned char) ~(unsigned char) (1 << 7) );
}

void opcode_cb_bf()    // RES 7, A
{
    A &= ~(unsigned char) (1 << 7);
}

void opcode_cb_c0()    // SET 0, B
{
    B |= (unsigned char) (1 << 0);
}

void opcode_cb_c1()    // SET 0, C
{
    C |= (unsigned char) (1 << 0);
}

void opcode_cb_c2()    // SET 0, D
{
    D |= (unsigned char) (1 << 0);
}

void opcode_cb_c3()    // SET 0, E
{
    E |= (unsigned char) (1 << 0);
}

void opcode_cb_c4()    // SET 0, H
{
    H |= (unsigned char) (1 << 0);
}

void opcode_cb_c5()    // SET 0, L
{
    L |= (unsigned char) (1 << 0);
}

void opcode_cb_c6()    // SET 0, (HL)
{
    writeByte( HL(), readByte( HL() ) | (unsigned char) (1 << 0) );
}

void opcode_cb_c7()    // SET 0, A
{
    A |= (unsigned char) (1 << 0);
}

void opcode_cb_c8()    // SET 1, B
{
    B |= (unsigned char) (1 << 1);
}

void opcode_cb_c9()    // SET 1, C
{
    C |= (unsigned char) (1 << 1);
}

void opcode_cb_ca()    // SET 1, D
{
    D |= (unsigned char) (1 << 1);
}

void opcode_cb_cb()    // SET 1, E
{
    E |= (unsigned char) (1 << 1);
}

void opcode_cb_cc()    // SET 1, H
{
    H |= (unsigned char) (1 << 1);
}

void opcode_cb_cd()    // SET 1, L
{
    L |= (unsigned char) (1 << 1);
}

void opcode_cb_ce()    // SET 1, (HL)
{
    writeByte( HL(), readByte( HL() ) | (unsigned char) (1 << 1) );
}

void opcode_cb_cf()    // SET 1, A
{
    A |= (unsigned char) (1 << 1);
}

void opcode_cb_d0()    // SET 2, B
{
    B |= (unsigned char) (1 << 2);
}

void opcode_cb_d1()    // SET 2, C
{
    C |= (unsigned char) (1 << 2);
}

void opcode_cb_d2()    // SET 2, D
{
    D |= (unsigned char) (1 << 2);
}

void opcode_cb_d3()    // SET 2, E
{
    E |= (unsigned char) (1 << 2);
}

void opcode_cb_d4()    // SET 2, H
{
    H |= (unsigned char) (1 << 2);
}

void opcode_cb_d5()    // SET 2, L
{
    L |= (unsigned char) (1 << 2);
}

void opcode_cb_d6()    // SET 2, (HL)
{
    writeByte( HL(), readByte( HL() ) | (unsigned char) (1 << 2) );
}

void opcode_cb_d7()    // SET 2, A
{
    A |= (unsigned char) (1 << 2);
}

void opcode_cb_d8()    // SET 3, B
{
    B |= (unsigned char) (1 << 3);
}

void opcode_cb_d9()    // SET 3, C
{
    C |= (unsigned char) (1 << 3);
}

void opcode_cb_da()    // SET 3, D
{
    D |= (unsigned char) (1 << 3);
}

void opcode_cb_db()    // SET 3, E
{
    E |= (unsigned char) (1 << 3);
}

void opcode_cb_dc()    // SET 3, H
{
    H |= (unsigned char) (1 << 3);
}

void opcode_cb_dd()    // SET 3, L
{
    L |= (unsigned char) (1 << 3);
}

void opcode_cb_de()    // SET 3, (HL)
{
    writeByte( HL(), readByte( HL() ) | (unsigned char) (1 << 3) );
}

void opcode_cb_df()    // SET 3, A
{
    A |= (unsigned char) (1 << 3);
}

void opcode_cb_e0()    // SET 4, B
{
    B |= (unsigned char) (1 << 4);
}

void opcode_cb_e1()    // SET 4, C
{
    C |= (unsigned char) (1 << 4);
}

void opcode_cb_e2()    // SET 4, D
{
    D |= (unsigned char) (1 << 4);
}

void opcode_cb_e3()    // SET 4, E
{
    E |= (unsigned char) (1 << 4);
}

void opcode_cb_e4()    // SET 4, H
{
    H |= (unsigned char) (1 << 4);
}

void opcode_cb_e5()    // SET 4, L
{
    L |= (unsigned char) (1 << 4);
}

void opcode_cb_e6()    // SET 4, (HL)
{
    writeByte( HL(), readByte( HL() ) | (unsigned char) (1 << 4) );
}

void opcode_cb_e7()    // SET 4, A
{
    A |= (unsigned char) (1 << 4);
}

void opcode_cb_e8()    // SET 5, B
{
    B |= (unsigned char) (1 << 5);
}

void opcode_cb_e9()    // SET 5, C
{
    C |= (unsigned char) (1 << 5);
}

void opcode_cb_ea()    // SET 5, D
{
    D |= (unsigned char) (1 << 5);
}

void opcode_cb_eb()    // SET 5, E
{
    E |= (unsigned char) (1 << 5);
}

void opcode_cb_ec()    // SET 5, H
{
    H |= (unsigned char) (1 << 5);
}

void opcode_cb_ed()    // SET 5, L
{
    L |= (unsigned char) (1 << 5);
}

void opcode_cb_ee()    // SET 5, (HL)
{
    writeByte( HL(), readByte( HL() ) | (unsigned char) (1 << 5) );
}

void opcode_cb_ef()    // SET 5, A
{
    A |= (unsigned char) (1 << 5);
}

void opcode_cb_f0()    // SET 6, B
{
    B |= (unsigned char) (1 << 6);
}

void opcode_cb_f1()    // SET 6, C
{
    C |= (unsigned char) (1 << 6);
}

void opcode_cb_f2()    // SET 6, D
{
    D |= (unsigned char) (1 << 6);
}

void opcode_cb_f3()    // SET 6, E
{
    E |= (unsigned char) (1 << 6);
}

void opcode_cb_f4()    // SET 6, H
{
    H |= (unsigned char) (1 << 6);
}

void opcode_cb_f5()    // SET 6, L
{
    L |= (unsigned char) (1 << 6);
}

void opcode_cb_f6()    // SET 6, (HL)
{
    writeByte( HL(), readByte( HL() ) | (unsigned char) (1 << 6) );
}

void opcode_cb_f7()    // SET 6, A
{
    A |= (unsigned char) (1 << 6);
}

void opcode_cb_f8()    // SET 7, B
{
    B |= (unsigned char) (1 << 7);
}

void opcode_cb_f9()    // SET 7, C
{
    C |= (unsigned char) (1 << 7);
}

void opcode_cb_fa()    // SET 7, D
{
    D |= (unsigned char) (1 << 7);
}

void opcode_cb_fb()    // SET 7, E
{
    E |= (unsigned char) (1 << 7);
}

void opcode_cb_fc()    // SET 7, H
{
    H |= (unsigned char) (1 << 7);
}

void opcode_cb_fd()    // SET 7, L
{
    L |= (unsigned char) (1 << 7);
}

void opcode_cb_fe()    // SET 7, (HL)
{
    writeByte( HL(), readByte( HL() ) | (unsigned char) (1 << 7) );
}

void opcode_cb_ff()    // SET 7, A
{
    A |= (unsigned char) (1 << 7);
}

OpcodeInfo OpInfoDD_[256] = {
    { 0, 0 }, // 0x00
    { 0, 0 }, // 0x01
    { 0, 0 }, // 0x02
    { 0, 0 }, // 0x03
    { 0, 0 }, // 0x04
    { 0, 0 }, // 0x05
    { 0, 0 }, // 0x06
    { 0, 0 }, // 0x07
    { 0, 0 }, // 0x08
    { &opcode_dd_09, 15 }, // ADD IX, BC
    { 0, 0 }, // 0x0A
    { 0, 0 }, // 0x0B
    { 0, 0 }, // 0x0C
    { 0, 0 }, // 0x0D
    { 0, 0 }, // 0x0E
    { 0, 0 }, // 0x0F
    { 0, 0 }, // 0x10
    { 0, 0 }, // 0x11
    { 0, 0 }, // 0x12
    { 0, 0 }, // 0x13
    { 0, 0 }, // 0x14
    { 0, 0 }, // 0x15
    { 0, 0 }, // 0x16
    { 0, 0 }, // 0x17
    { 0, 0 }, // 0x18
    { &opcode_dd_19, 15 }, // ADD IX, DE
    { 0, 0 }, // 0x1A
    { 0, 0 }, // 0x1B
    { 0, 0 }, // 0x1C
    { 0, 0 }, // 0x1D
    { 0, 0 }, // 0x1E
    { 0, 0 }, // 0x1F
    { 0, 0 }, // 0x20
    { &opcode_dd_21, 14 }, // LD IX, nn
    { &opcode_dd_22, 20 }, // LD (nn), IX
    { &opcode_dd_23, 10 }, // INC IX
    { &opcode_dd_24,  9 }, // INC IXH
    { &opcode_dd_25,  9 }, // DEC IXH
    { &opcode_dd_26,  9 }, // LD IXH, n
    { 0, 0 }, // 0x27
    { 0, 0 }, // 0x28
    { &opcode_dd_29, 15 }, // ADD IX, IX
    { &opcode_dd_2a, 20 }, // LD IX, (nn)
    { &opcode_dd_2b, 10 }, // DEC IX
    { &opcode_dd_2c,  9 }, // INC IXL
    { &opcode_dd_2d,  9 }, // DEC IXL
    { &opcode_dd_2e,  9 }, // LD IXL, n
    { 0, 0 }, // 0x2F
    { 0, 0 }, // 0x30
    { 0, 0 }, // 0x31
    { 0, 0 }, // 0x32
    { 0, 0 }, // 0x33
    { &opcode_dd_34, 23 }, // INC (IX + d)
    { &opcode_dd_35, 23 }, // DEC (IX + d)
    { &opcode_dd_36, 19 }, // LD (IX + d), n
    { 0, 0 }, // 0x37
    { 0, 0 }, // 0x38
    { &opcode_dd_39, 15 }, // ADD IX, SP
    { 0, 0 }, // 0x3A
    { 0, 0 }, // 0x3B
    { 0, 0 }, // 0x3C
    { 0, 0 }, // 0x3D
    { 0, 0 }, // 0x3E
    { 0, 0 }, // 0x3F
    { 0, 0 }, // 0x40
    { 0, 0 }, // 0x41
    { 0, 0 }, // 0x42
    { 0, 0 }, // 0x43
    { &opcode_dd_44,  9 }, // LD B, IXH
    { &opcode_dd_45,  9 }, // LD B, IXL
    { &opcode_dd_46, 19 }, // LD B, (IX + d)
    { 0, 0 }, // 0x47
    { 0, 0 }, // 0x48
    { 0, 0 }, // 0x49
    { 0, 0 }, // 0x4A
    { 0, 0 }, // 0x4B
    { &opcode_dd_4c,  9 }, // LD C, IXH
    { &opcode_dd_4d,  9 }, // LD C, IXL
    { &opcode_dd_4e, 19 }, // LD C, (IX + d)
    { 0, 0 }, // 0x4F
    { 0, 0 }, // 0x50
    { 0, 0 }, // 0x51
    { 0, 0 }, // 0x52
    { 0, 0 }, // 0x53
    { &opcode_dd_54,  9 }, // LD D, IXH
    { &opcode_dd_55,  9 }, // LD D, IXL
    { &opcode_dd_56, 19 }, // LD D, (IX + d)
    { 0, 0 }, // 0x57
    { 0, 0 }, // 0x58
    { 0, 0 }, // 0x59
    { 0, 0 }, // 0x5A
    { 0, 0 }, // 0x5B
    { &opcode_dd_5c,  9 }, // LD E, IXH
    { &opcode_dd_5d,  9 }, // LD E, IXL
    { &opcode_dd_5e, 19 }, // LD E, (IX + d)
    { 0, 0 }, // 0x5F
    { &opcode_dd_60,  9 }, // LD IXH, B
    { &opcode_dd_61,  9 }, // LD IXH, C
    { &opcode_dd_62,  9 }, // LD IXH, D
    { &opcode_dd_63,  9 }, // LD IXH, E
    { &opcode_dd_64,  9 }, // LD IXH, IXH
    { &opcode_dd_65,  9 }, // LD IXH, IXL
    { &opcode_dd_66,  9 }, // LD H, (IX + d)
    { &opcode_dd_67,  9 }, // LD IXH, A
    { &opcode_dd_68,  9 }, // LD IXL, B
    { &opcode_dd_69,  9 }, // LD IXL, C
    { &opcode_dd_6a,  9 }, // LD IXL, D
    { &opcode_dd_6b,  9 }, // LD IXL, E
    { &opcode_dd_6c,  9 }, // LD IXL, IXH
    { &opcode_dd_6d,  9 }, // LD IXL, IXL
    { &opcode_dd_6e,  9 }, // LD L, (IX + d)
    { &opcode_dd_6f,  9 }, // LD IXL, A
    { &opcode_dd_70, 19 }, // LD (IX + d), B
    { &opcode_dd_71, 19 }, // LD (IX + d), C
    { &opcode_dd_72, 19 }, // LD (IX + d), D
    { &opcode_dd_73, 19 }, // LD (IX + d), E
    { &opcode_dd_74, 19 }, // LD (IX + d), H
    { &opcode_dd_75, 19 }, // LD (IX + d), L
    { 0,19 }, // 0x76
    { &opcode_dd_77, 19 }, // LD (IX + d), A
    { 0, 0 }, // 0x78
    { 0, 0 }, // 0x79
    { 0, 0 }, // 0x7A
    { 0, 0 }, // 0x7B
    { &opcode_dd_7c,  9 }, // LD A, IXH
    { &opcode_dd_7d,  9 }, // LD A, IXL
    { &opcode_dd_7e, 19 }, // LD A, (IX + d)
    { 0, 0 }, // 0x7F
    { 0, 0 }, // 0x80
    { 0, 0 }, // 0x81
    { 0, 0 }, // 0x82
    { 0, 0 }, // 0x83
    { &opcode_dd_84,  9 }, // ADD A, IXH
    { &opcode_dd_85,  9 }, // ADD A, IXL
    { &opcode_dd_86, 19 }, // ADD A, (IX + d)
    { 0, 0 }, // 0x87
    { 0, 0 }, // 0x88
    { 0, 0 }, // 0x89
    { 0, 0 }, // 0x8A
    { 0, 0 }, // 0x8B
    { &opcode_dd_8c,  9 }, // ADC A, IXH
    { &opcode_dd_8d,  9 }, // ADC A, IXL
    { &opcode_dd_8e, 19 }, // ADC A, (IX + d)
    { 0, 0 }, // 0x8F
    { 0, 0 }, // 0x90
    { 0, 0 }, // 0x91
    { 0, 0 }, // 0x92
    { 0, 0 }, // 0x93
    { &opcode_dd_94,  9 }, // SUB IXH
    { &opcode_dd_95,  9 }, // SUB IXL
    { &opcode_dd_96, 19 }, // SUB (IX + d)
    { 0, 0 }, // 0x97
    { 0, 0 }, // 0x98
    { 0, 0 }, // 0x99
    { 0, 0 }, // 0x9A
    { 0, 0 }, // 0x9B
    { &opcode_dd_9c,  9 }, // SBC A, IXH
    { &opcode_dd_9d,  9 }, // SBC A, IXL
    { &opcode_dd_9e, 19 }, // SBC A, (IX + d)
    { 0, 0 }, // 0x9F
    { 0, 0 }, // 0xA0
    { 0, 0 }, // 0xA1
    { 0, 0 }, // 0xA2
    { 0, 0 }, // 0xA3
    { &opcode_dd_a4,  9 }, // AND IXH
    { &opcode_dd_a5,  9 }, // AND IXL
    { &opcode_dd_a6, 19 }, // AND (IX + d)
    { 0, 0 }, // 0xA7
    { 0, 0 }, // 0xA8
    { 0, 0 }, // 0xA9
    { 0, 0 }, // 0xAA
    { 0, 0 }, // 0xAB
    { &opcode_dd_ac,  9 }, // XOR IXH
    { &opcode_dd_ad,  9 }, // XOR IXL
    { &opcode_dd_ae, 19 }, // XOR (IX + d)
    { 0, 0 }, // 0xAF
    { 0, 0 }, // 0xB0
    { 0, 0 }, // 0xB1
    { 0, 0 }, // 0xB2
    { 0, 0 }, // 0xB3
    { &opcode_dd_b4,  9 }, // OR IXH
    { &opcode_dd_b5,  9 }, // OR IXL
    { &opcode_dd_b6, 19 }, // OR (IX + d)
    { 0, 0 }, // 0xB7
    { 0, 0 }, // 0xB8
    { 0, 0 }, // 0xB9
    { 0, 0 }, // 0xBA
    { 0, 0 }, // 0xBB
    { &opcode_dd_bc,  9 }, // CP IXH
    { &opcode_dd_bd,  9 }, // CP IXL
    { &opcode_dd_be, 19 }, // CP (IX + d)
    { 0, 0 }, // 0xBF
    { 0, 0 }, // 0xC0
    { 0, 0 }, // 0xC1
    { 0, 0 }, // 0xC2
    { 0, 0 }, // 0xC3
    { 0, 0 }, // 0xC4
    { 0, 0 }, // 0xC5
    { 0, 0 }, // 0xC6
    { 0, 0 }, // 0xC7
    { 0, 0 }, // 0xC8
    { 0, 0 }, // 0xC9
    { 0, 0 }, // 0xCA
    { &opcode_dd_cb,  0 }, // 
    { 0, 0 }, // 0xCC
    { 0, 0 }, // 0xCD
    { 0, 0 }, // 0xCE
    { 0, 0 }, // 0xCF
    { 0, 0 }, // 0xD0
    { 0, 0 }, // 0xD1
    { 0, 0 }, // 0xD2
    { 0, 0 }, // 0xD3
    { 0, 0 }, // 0xD4
    { 0, 0 }, // 0xD5
    { 0, 0 }, // 0xD6
    { 0, 0 }, // 0xD7
    { 0, 0 }, // 0xD8
    { 0, 0 }, // 0xD9
    { 0, 0 }, // 0xDA
    { 0, 0 }, // 0xDB
    { 0, 0 }, // 0xDC
    { 0, 0 }, // 0xDD
    { 0, 0 }, // 0xDE
    { 0, 0 }, // 0xDF
    { 0, 0 }, // 0xE0
    { &opcode_dd_e1, 14 }, // POP IX
    { 0, 0 }, // 0xE2
    { &opcode_dd_e3, 23 }, // EX (SP), IX
    { 0, 0 }, // 0xE4
    { &opcode_dd_e5, 15 }, // PUSH IX
    { 0, 0 }, // 0xE6
    { 0, 0 }, // 0xE7
    { 0, 0 }, // 0xE8
    { &opcode_dd_e9,  8 }, // JP (IX)
    { 0, 0 }, // 0xEA
    { 0, 0 }, // 0xEB
    { 0, 0 }, // 0xEC
    { 0, 0 }, // 0xED
    { 0, 0 }, // 0xEE
    { 0, 0 }, // 0xEF
    { 0, 0 }, // 0xF0
    { 0, 0 }, // 0xF1
    { 0, 0 }, // 0xF2
    { 0, 0 }, // 0xF3
    { 0, 0 }, // 0xF4
    { 0, 0 }, // 0xF5
    { 0, 0 }, // 0xF6
    { 0, 0 }, // 0xF7
    { 0, 0 }, // 0xF8
    { &opcode_dd_f9, 10 }, // LD SP, IX
    { 0, 0 }, // 0xFA
    { 0, 0 }, // 0xFB
    { 0, 0 }, // 0xFC
    { 0, 0 }, // 0xFD
    { 0, 0 }, // 0xFE
    { 0, 0 }  // 0xFF
};

void opcode_dd_09()    // ADD IX, BC
{
    unsigned rr = BC();

    F &= (Zero | Sign | Parity);
    if( ((IX & 0xFFF)+(rr & 0xFFF)) > 0xFFF ) F |= Halfcarry;
    IX += rr;
    if( IX & 0x10000 ) F |= Carry;
}

void opcode_dd_19()    // ADD IX, DE
{
    unsigned rr = DE();

    F &= (Zero | Sign | Parity);
    if( ((IX & 0xFFF)+(rr & 0xFFF)) > 0xFFF ) F |= Halfcarry;
    IX += rr;
    if( IX & 0x10000 ) F |= Carry;
}

void opcode_dd_21()    // LD IX, nn
{
    IX = fetchWord();
}

void opcode_dd_22()    // LD (nn), IX
{
    writeWord( fetchWord(), IX );
}

void opcode_dd_23()    // INC IX
{
    IX++;
}

void opcode_dd_24()    // INC IXH
{
    IX = (IX & 0xFF) | ((unsigned)incByte( IX >> 8 ) << 8);
}

void opcode_dd_25()    // DEC IXH
{
    IX = (IX & 0xFF) | ((unsigned)decByte( IX >> 8 ) << 8);
}

void opcode_dd_26()    // LD IXH, n
{
    IX = (IX & 0xFF) | ((unsigned)fetchByte() << 8);
}

void opcode_dd_29()    // ADD IX, IX
{
    F &= (Zero | Sign | Parity);
    if( IX & 0x800 ) F |= Halfcarry;
    IX += IX;
    if( IX & 0x10000 ) F |= Carry;
}

void opcode_dd_2a()    // LD IX, (nn)
{
    IX = readWord( fetchWord() );
}

void opcode_dd_2b()    // DEC IX
{
    IX--;
}

void opcode_dd_2c()    // INC IXL
{
    IX = (IX & 0xFF00) | incByte( IX & 0xFF );
}

void opcode_dd_2d()    // DEC IXL
{
    IX = (IX & 0xFF00) | decByte( IX & 0xFF );
}

void opcode_dd_2e()    // LD IXL, n
{
    IX = (IX & 0xFF00) | fetchByte();
}

void opcode_dd_34()    // INC (IX + d)
{
    unsigned    addr = addDispl( IX, fetchByte() );

    writeByte( addr, incByte( readByte( addr ) ) );
}

void opcode_dd_35()    // DEC (IX + d)
{
    unsigned    addr = addDispl( IX, fetchByte() );

    writeByte( addr, decByte( readByte( addr ) ) );
}

void opcode_dd_36()    // LD (IX + d), n
{
    unsigned    addr = addDispl( IX, fetchByte() );

    writeByte( addr, fetchByte() );
}

void opcode_dd_39()    // ADD IX, SP
{
    F &= (Zero | Sign | Parity);
    if( ((IX & 0xFFF)+(SP & 0xFFF)) > 0xFFF ) F |= Halfcarry;
    IX += SP;
    if( IX & 0x10000 ) F |= Carry;
}

void opcode_dd_44()    // LD B, IXH
{
    B = IX >> 8;
}

void opcode_dd_45()    // LD B, IXL
{
    B = IX & 0xFF;
}

void opcode_dd_46()    // LD B, (IX + d)
{
    B = readByte( addDispl(IX,fetchByte()) );
}

void opcode_dd_4c()    // LD C, IXH
{
    C = IX >> 8;
}

void opcode_dd_4d()    // LD C, IXL
{
    C = IX & 0xFF;
}

void opcode_dd_4e()    // LD C, (IX + d)
{
    C = readByte( addDispl(IX,fetchByte()) );
}

void opcode_dd_54()    // LD D, IXH
{
    D = IX >> 8;
}

void opcode_dd_55()    // LD D, IXL
{
    D = IX & 0xFF;
}

void opcode_dd_56()    // LD D, (IX + d)
{
    D = readByte( addDispl(IX,fetchByte()) );
}

void opcode_dd_5c()    // LD E, IXH
{
    E = IX >> 8;
}

void opcode_dd_5d()    // LD E, IXL
{
    E = IX & 0xFF;
}

void opcode_dd_5e()    // LD E, (IX + d)
{
    E = readByte( addDispl(IX,fetchByte()) );
}

void opcode_dd_60()    // LD IXH, B
{
    IX = (IX & 0xFF) | ((unsigned)B << 8);
}

void opcode_dd_61()    // LD IXH, C
{
    IX = (IX & 0xFF) | ((unsigned)C << 8);
}

void opcode_dd_62()    // LD IXH, D
{
    IX = (IX & 0xFF) | ((unsigned)D << 8);
}

void opcode_dd_63()    // LD IXH, E
{
    IX = (IX & 0xFF) | ((unsigned)E << 8);
}

void opcode_dd_64()    // LD IXH, IXH
{
}

void opcode_dd_65()    // LD IXH, IXL
{
    IX = (IX & 0xFF) | ((IX << 8) & 0xFF00);
}

void opcode_dd_66()    // LD H, (IX + d)
{
    H = readByte( addDispl(IX,fetchByte()) );
}

void opcode_dd_67()    // LD IXH, A
{
    IX = (IX & 0xFF) | ((unsigned)A << 8);
}

void opcode_dd_68()    // LD IXL, B
{
    IX = (IX & 0xFF00) | B;
}

void opcode_dd_69()    // LD IXL, C
{
    IX = (IX & 0xFF00) | C;
}

void opcode_dd_6a()    // LD IXL, D
{
    IX = (IX & 0xFF00) | D;
}

void opcode_dd_6b()    // LD IXL, E
{
    IX = (IX & 0xFF00) | E;
}

void opcode_dd_6c()    // LD IXL, IXH
{
    IX = (IX & 0xFF00) | ((IX >> 8) & 0xFF);
}

void opcode_dd_6d()    // LD IXL, IXL
{
}

void opcode_dd_6e()    // LD L, (IX + d)
{
    L = readByte( addDispl(IX,fetchByte()) );
}

void opcode_dd_6f()    // LD IXL, A
{
    IX = (IX & 0xFF00) | A;
}

void opcode_dd_70()    // LD (IX + d), B
{
    writeByte( addDispl(IX,fetchByte()), B );
}

void opcode_dd_71()    // LD (IX + d), C
{
    writeByte( addDispl(IX,fetchByte()), C );
}

void opcode_dd_72()    // LD (IX + d), D
{
    writeByte( addDispl(IX,fetchByte()), D );
}

void opcode_dd_73()    // LD (IX + d), E
{
    writeByte( addDispl(IX,fetchByte()), E );
}

void opcode_dd_74()    // LD (IX + d), H
{
    writeByte( addDispl(IX,fetchByte()), H );
}

void opcode_dd_75()    // LD (IX + d), L
{
    writeByte( addDispl(IX,fetchByte()), L );
}

void opcode_dd_77()    // LD (IX + d), A
{
    writeByte( addDispl(IX,fetchByte()), A );
}

void opcode_dd_7c()    // LD A, IXH
{
    A = IX >> 8;
}

void opcode_dd_7d()    // LD A, IXL
{
    A = IX & 0xFF;
}

void opcode_dd_7e()    // LD A, (IX + d)
{
    A = readByte( addDispl(IX,fetchByte()) );
}

void opcode_dd_84()    // ADD A, IXH
{
    addByte( IX >> 8, 0 );
}

void opcode_dd_85()    // ADD A, IXL
{
    addByte( IX & 0xFF, 0 );
}

void opcode_dd_86()    // ADD A, (IX + d)
{
    addByte( readByte( addDispl(IX,fetchByte()) ), 0 );
}

void opcode_dd_8c()    // ADC A, IXH
{
    addByte( IX >> 8, F & Carry );
}

void opcode_dd_8d()    // ADC A, IXL
{
    addByte( IX & 0xFF, F & Carry );
}

void opcode_dd_8e()    // ADC A, (IX + d)
{
    addByte( readByte( addDispl(IX,fetchByte()) ), F & Carry );
}

void opcode_dd_94()    // SUB IXH
{
    A = subByte( IX >> 8, 0 );
}

void opcode_dd_95()    // SUB IXL
{
    A = subByte( IX & 0xFF, 0 );
}

void opcode_dd_96()    // SUB (IX + d)
{
    A = subByte( readByte( addDispl(IX,fetchByte()) ), 0 );
}

void opcode_dd_9c()    // SBC A, IXH
{
    A = subByte( IX >> 8, F & Carry );
}

void opcode_dd_9d()    // SBC A, IXL
{
    A = subByte( IX & 0xFF, F & Carry );
}

void opcode_dd_9e()    // SBC A, (IX + d)
{
    A = subByte( readByte( addDispl(IX,fetchByte()) ), F & Carry );
}

void opcode_dd_a4()    // AND IXH
{
    A &= IX >> 8;
    setFlags35PSZ000();
    F |= Halfcarry;
}

void opcode_dd_a5()    // AND IXL
{
    A &= IX & 0xFF;
    setFlags35PSZ000();
    F |= Halfcarry;
}

void opcode_dd_a6()    // AND (IX + d)
{
    A &= readByte( addDispl(IX,fetchByte()) );
    setFlags35PSZ000();
    F |= Halfcarry;
}

void opcode_dd_ac()    // XOR IXH
{
    A ^= IX >> 8;
    setFlags35PSZ000();
}

void opcode_dd_ad()    // XOR IXL
{
    A ^= IX & 0xFF;
    setFlags35PSZ000();
}

void opcode_dd_ae()    // XOR (IX + d)
{
    A ^= readByte( addDispl(IX,fetchByte()) );
    setFlags35PSZ000();
}

void opcode_dd_b4()    // OR IXH
{
    A |= IX >> 8;
    setFlags35PSZ000();
}

void opcode_dd_b5()    // OR IXL
{
    A |= IX & 0xFF;
    setFlags35PSZ000();
}

void opcode_dd_b6()    // OR (IX + d)
{
    A |= readByte( addDispl(IX,fetchByte()) );
    setFlags35PSZ000();
}

void opcode_dd_bc()    // CP IXH
{
    cmpByte( IX >> 8 );
}

void opcode_dd_bd()    // CP IXL
{
    cmpByte( IX & 0xFF );
}

void opcode_dd_be()    // CP (IX + d)
{
    cmpByte( readByte( addDispl(IX,fetchByte()) ) );
}

void opcode_dd_cb()    // 
{
    do_opcode_xycb( IX );
}

void opcode_dd_e1()    // POP IX
{
    IX = readWord( SP );
    SP += 2;
}

void opcode_dd_e3()    // EX (SP), IX
{
    unsigned    ix = IX;

    IX = readWord( SP );
    writeWord( SP, ix );
}

void opcode_dd_e5()    // PUSH IX
{
    SP -= 2;
    writeWord( SP, IX );
}

void opcode_dd_e9()    // JP (IX)
{
    PC = IX;
}

void opcode_dd_f9()    // LD SP, IX
{
    SP = IX;
}

OpcodeInfo OpInfoED_[256] = {
    { 0, 0 }, // 0x00
    { 0, 0 }, // 0x01
    { 0, 0 }, // 0x02
    { 0, 0 }, // 0x03
    { 0, 0 }, // 0x04
    { 0, 0 }, // 0x05
    { 0, 0 }, // 0x06
    { 0, 0 }, // 0x07
    { 0, 0 }, // 0x08
    { 0, 0 }, // 0x09
    { 0, 0 }, // 0x0A
    { 0, 0 }, // 0x0B
    { 0, 0 }, // 0x0C
    { 0, 0 }, // 0x0D
    { 0, 0 }, // 0x0E
    { 0, 0 }, // 0x0F
    { 0, 0 }, // 0x10
    { 0, 0 }, // 0x11
    { 0, 0 }, // 0x12
    { 0, 0 }, // 0x13
    { 0, 0 }, // 0x14
    { 0, 0 }, // 0x15
    { 0, 0 }, // 0x16
    { 0, 0 }, // 0x17
    { 0, 0 }, // 0x18
    { 0, 0 }, // 0x19
    { 0, 0 }, // 0x1A
    { 0, 0 }, // 0x1B
    { 0, 0 }, // 0x1C
    { 0, 0 }, // 0x1D
    { 0, 0 }, // 0x1E
    { 0, 0 }, // 0x1F
    { 0, 0 }, // 0x20
    { 0, 0 }, // 0x21
    { 0, 0 }, // 0x22
    { 0, 0 }, // 0x23
    { 0, 0 }, // 0x24
    { 0, 0 }, // 0x25
    { 0, 0 }, // 0x26
    { 0, 0 }, // 0x27
    { 0, 0 }, // 0x28
    { 0, 0 }, // 0x29
    { 0, 0 }, // 0x2A
    { 0, 0 }, // 0x2B
    { 0, 0 }, // 0x2C
    { 0, 0 }, // 0x2D
    { 0, 0 }, // 0x2E
    { 0, 0 }, // 0x2F
    { 0, 0 }, // 0x30
    { 0, 0 }, // 0x31
    { 0, 0 }, // 0x32
    { 0, 0 }, // 0x33
    { 0, 0 }, // 0x34
    { 0, 0 }, // 0x35
    { 0, 0 }, // 0x36
    { 0, 0 }, // 0x37
    { 0, 0 }, // 0x38
    { 0, 0 }, // 0x39
    { 0, 0 }, // 0x3A
    { 0, 0 }, // 0x3B
    { 0, 0 }, // 0x3C
    { 0, 0 }, // 0x3D
    { 0, 0 }, // 0x3E
    { 0, 0 }, // 0x3F
    { &opcode_ed_40, 12 }, // IN B, (C)
    { &opcode_ed_41, 12 }, // OUT (C), B
    { &opcode_ed_42, 15 }, // SBC HL, BC
    { &opcode_ed_43, 20 }, // LD (nn), BC
    { &opcode_ed_44,  8 }, // NEG
    { &opcode_ed_45, 14 }, // RETN
    { &opcode_ed_46,  8 }, // IM 0
    { &opcode_ed_47,  9 }, // LD I, A
    { &opcode_ed_48, 12 }, // IN C, (C)
    { &opcode_ed_49, 12 }, // OUT (C), C
    { &opcode_ed_4a, 15 }, // ADC HL, BC
    { &opcode_ed_4b, 20 }, // LD BC, (nn)
    { &opcode_ed_4c,  8 }, // NEG
    { &opcode_ed_4d, 14 }, // RETI
    { &opcode_ed_4e,  8 }, // IM 0/1
    { &opcode_ed_4f,  9 }, // LD R, A
    { &opcode_ed_50, 12 }, // IN D, (C)
    { &opcode_ed_51, 12 }, // OUT (C), D
    { &opcode_ed_52, 15 }, // SBC HL, DE
    { &opcode_ed_53, 20 }, // LD (nn), DE
    { &opcode_ed_54,  8 }, // NEG
    { &opcode_ed_55, 14 }, // RETN
    { &opcode_ed_56,  8 }, // IM 1
    { &opcode_ed_57,  9 }, // LD A, I
    { &opcode_ed_58, 12 }, // IN E, (C)
    { &opcode_ed_59, 12 }, // OUT (C), E
    { &opcode_ed_5a, 15 }, // ADC HL, DE
    { &opcode_ed_5b, 20 }, // LD DE, (nn)
    { &opcode_ed_5c,  8 }, // NEG
    { &opcode_ed_5d, 14 }, // RETN
    { &opcode_ed_5e,  8 }, // IM 2
    { &opcode_ed_5f,  9 }, // LD A, R
    { &opcode_ed_60, 12 }, // IN H, (C)
    { &opcode_ed_61, 12 }, // OUT (C), H
    { &opcode_ed_62, 15 }, // SBC HL, HL
    { &opcode_ed_63, 20 }, // LD (nn), HL
    { &opcode_ed_64,  8 }, // NEG
    { &opcode_ed_65, 14 }, // RETN
    { &opcode_ed_66,  8 }, // IM 0
    { &opcode_ed_67, 18 }, // RRD
    { &opcode_ed_68, 12 }, // IN L, (C)
    { &opcode_ed_69, 12 }, // OUT (C), L
    { &opcode_ed_6a, 15 }, // ADC HL, HL
    { &opcode_ed_6b, 20 }, // LD HL, (nn)
    { &opcode_ed_6c,  8 }, // NEG
    { &opcode_ed_6d, 14 }, // RETN
    { &opcode_ed_6e,  8 }, // IM 0/1
    { &opcode_ed_6f, 18 }, // RLD
    { &opcode_ed_70, 12 }, // IN (C) / IN F, (C)
    { &opcode_ed_71, 12 }, // OUT (C), 0
    { &opcode_ed_72, 15 }, // SBC HL, SP
    { &opcode_ed_73, 20 }, // LD (nn), SP
    { &opcode_ed_74,  8 }, // NEG
    { &opcode_ed_75, 14 }, // RETN
    { &opcode_ed_76,  8 }, // IM 1
    { 0, 0 }, // 0x77
    { &opcode_ed_78, 12 }, // IN A, (C)
    { &opcode_ed_79, 12 }, // OUT (C), A
    { &opcode_ed_7a, 15 }, // ADC HL, SP
    { &opcode_ed_7b, 20 }, // LD SP, (nn)
    { &opcode_ed_7c,  8 }, // NEG
    { &opcode_ed_7d, 14 }, // RETN
    { &opcode_ed_7e,  8 }, // IM 2
    { 0, 0 }, // 0x7F
    { 0, 0 }, // 0x80
    { 0, 0 }, // 0x81
    { 0, 0 }, // 0x82
    { 0, 0 }, // 0x83
    { 0, 0 }, // 0x84
    { 0, 0 }, // 0x85
    { 0, 0 }, // 0x86
    { 0, 0 }, // 0x87
    { 0, 0 }, // 0x88
    { 0, 0 }, // 0x89
    { 0, 0 }, // 0x8A
    { 0, 0 }, // 0x8B
    { 0, 0 }, // 0x8C
    { 0, 0 }, // 0x8D
    { 0, 0 }, // 0x8E
    { 0, 0 }, // 0x8F
    { 0, 0 }, // 0x90
    { 0, 0 }, // 0x91
    { 0, 0 }, // 0x92
    { 0, 0 }, // 0x93
    { 0, 0 }, // 0x94
    { 0, 0 }, // 0x95
    { 0, 0 }, // 0x96
    { 0, 0 }, // 0x97
    { 0, 0 }, // 0x98
    { 0, 0 }, // 0x99
    { 0, 0 }, // 0x9A
    { 0, 0 }, // 0x9B
    { 0, 0 }, // 0x9C
    { 0, 0 }, // 0x9D
    { 0, 0 }, // 0x9E
    { 0, 0 }, // 0x9F
    { &opcode_ed_a0, 16 }, // LDI
    { &opcode_ed_a1, 16 }, // CPI
    { &opcode_ed_a2, 16 }, // INI
    { &opcode_ed_a3, 16 }, // OUTI
    { 0, 0 }, // 0xA4
    { 0, 0 }, // 0xA5
    { 0, 0 }, // 0xA6
    { 0, 0 }, // 0xA7
    { &opcode_ed_a8, 16 }, // LDD
    { &opcode_ed_a9, 16 }, // CPD
    { &opcode_ed_aa, 16 }, // IND
    { &opcode_ed_ab, 16 }, // OUTD
    { 0, 0 }, // 0xAC
    { 0, 0 }, // 0xAD
    { 0, 0 }, // 0xAE
    { 0, 0 }, // 0xAF
    { &opcode_ed_b0,  0 }, // LDIR
    { &opcode_ed_b1,  0 }, // CPIR
    { &opcode_ed_b2,  0 }, // INIR
    { &opcode_ed_b3,  0 }, // OTIR
    { 0, 0 }, // 0xB4
    { 0, 0 }, // 0xB5
    { 0, 0 }, // 0xB6
    { 0, 0 }, // 0xB7
    { &opcode_ed_b8,  0 }, // LDDR
    { &opcode_ed_b9,  0 }, // CPDR
    { &opcode_ed_ba,  0 }, // INDR
    { &opcode_ed_bb,  0 }, // OTDR
    { 0, 0 }, // 0xBC
    { 0, 0 }, // 0xBD
    { 0, 0 }, // 0xBE
    { 0, 0 }, // 0xBF
    { 0, 0 }, // 0xC0
    { 0, 0 }, // 0xC1
    { 0, 0 }, // 0xC2
    { 0, 0 }, // 0xC3
    { 0, 0 }, // 0xC4
    { 0, 0 }, // 0xC5
    { 0, 0 }, // 0xC6
    { 0, 0 }, // 0xC7
    { 0, 0 }, // 0xC8
    { 0, 0 }, // 0xC9
    { 0, 0 }, // 0xCA
    { 0, 0 }, // 0xCB
    { 0, 0 }, // 0xCC
    { 0, 0 }, // 0xCD
    { 0, 0 }, // 0xCE
    { 0, 0 }, // 0xCF
    { 0, 0 }, // 0xD0
    { 0, 0 }, // 0xD1
    { 0, 0 }, // 0xD2
    { 0, 0 }, // 0xD3
    { 0, 0 }, // 0xD4
    { 0, 0 }, // 0xD5
    { 0, 0 }, // 0xD6
    { 0, 0 }, // 0xD7
    { 0, 0 }, // 0xD8
    { 0, 0 }, // 0xD9
    { 0, 0 }, // 0xDA
    { 0, 0 }, // 0xDB
    { 0, 0 }, // 0xDC
    { 0, 0 }, // 0xDD
    { 0, 0 }, // 0xDE
    { 0, 0 }, // 0xDF
    { 0, 0 }, // 0xE0
    { 0, 0 }, // 0xE1
    { 0, 0 }, // 0xE2
    { 0, 0 }, // 0xE3
    { 0, 0 }, // 0xE4
    { 0, 0 }, // 0xE5
    { 0, 0 }, // 0xE6
    { 0, 0 }, // 0xE7
    { 0, 0 }, // 0xE8
    { 0, 0 }, // 0xE9
    { 0, 0 }, // 0xEA
    { 0, 0 }, // 0xEB
    { 0, 0 }, // 0xEC
    { 0, 0 }, // 0xED
    { 0, 0 }, // 0xEE
    { 0, 0 }, // 0xEF
    { 0, 0 }, // 0xF0
    { 0, 0 }, // 0xF1
    { 0, 0 }, // 0xF2
    { 0, 0 }, // 0xF3
    { 0, 0 }, // 0xF4
    { 0, 0 }, // 0xF5
    { 0, 0 }, // 0xF6
    { 0, 0 }, // 0xF7
    { 0, 0 }, // 0xF8
    { 0, 0 }, // 0xF9
    { 0, 0 }, // 0xFA
    { 0, 0 }, // 0xFB
    { 0, 0 }, // 0xFC
    { 0, 0 }, // 0xFD
    { 0, 0 }, // 0xFE
    { 0, 0 }  // 0xFF
};

void opcode_ed_40()    // IN B, (C)
{
    B = inpReg();
}

void opcode_ed_41()    // OUT (C), B
{
    writePort( C, B );
}

void opcode_ed_42()    // SBC HL, BC
{
    unsigned char a;

    a = A;
    A = L; L = subByte( C, F & Carry );
    A = H; H = subByte( B, F & Carry );
    A = a;
    if( HL() == 0 ) F |= Zero; else F &= ~Zero;
}

void opcode_ed_43()    // LD (nn), BC
{
    unsigned addr = fetchWord();

    writeByte( addr, C );
    writeByte( addr+1, B );
}

void opcode_ed_44()    // NEG
{
    unsigned char   a = A;

    A = 0;
    A = subByte( a, 0 );
}

void opcode_ed_45()    // RETN
{
    retFromSub();
    iflags_ &= ~IFF1; 
    if( iflags_ & IFF2 ) iflags_ |= IFF1;
}

void opcode_ed_46()    // IM 0
{
    setInterruptMode( 0 );
}

void opcode_ed_47()    // LD I, A
{
    I = A;
}

void opcode_ed_48()    // IN C, (C)
{
    C = inpReg();
}

void opcode_ed_49()    // OUT (C), C
{
    writePort( C, C );
}

void opcode_ed_4a()    // ADC HL, BC
{
    unsigned char a;

    a = A;
    A = L; addByte( C, F & Carry ); L = A;
    A = H; addByte( B, F & Carry ); H = A;
    A = a;
    if( HL() == 0 ) F |= Zero; else F &= ~Zero;
}

void opcode_ed_4b()    // LD BC, (nn)
{
    unsigned    addr = fetchWord();

    C = readByte( addr );
    B = readByte( addr+1 );
}

void opcode_ed_4c()    // NEG
{
    opcode_ed_44();
}

void opcode_ed_4d()    // RETI
{
    retFromSub();
    //onReturnFromInterrupt();
}

void opcode_ed_4e()    // IM 0/1
{
    setInterruptMode( 0 );
}

void opcode_ed_4f()    // LD R, A
{
    R = A;
}

void opcode_ed_50()    // IN D, (C)
{
    D = inpReg();
}

void opcode_ed_51()    // OUT (C), D
{
    writePort( C, D );
}

void opcode_ed_52()    // SBC HL, DE
{
    unsigned char a;

    a = A;
    A = L; L = subByte( E, F & Carry );
    A = H; H = subByte( D, F & Carry );
    A = a;
    if( HL() == 0 ) F |= Zero; else F &= ~Zero;
}

void opcode_ed_53()    // LD (nn), DE
{
    unsigned addr = fetchWord();

    writeByte( addr, E );
    writeByte( addr+1, D );
}

void opcode_ed_54()    // NEG
{
    opcode_ed_44();
}

void opcode_ed_55()    // RETN
{
    opcode_ed_45();
}

void opcode_ed_56()    // IM 1
{
    setInterruptMode( 1 );
}

void opcode_ed_57()    // LD A, I
{
    A = I;
    setFlags35PSZ();
    F &= ~(Halfcarry | Parity | AddSub);
    if( iflags_ & IFF2 ) F |= Parity;
}

void opcode_ed_58()    // IN E, (C)
{
    E = inpReg();
}

void opcode_ed_59()    // OUT (C), E
{
    writePort( C, E );
}

void opcode_ed_5a()    // ADC HL, DE
{
    unsigned char a;

    a = A;
    A = L; addByte( E, F & Carry ); L = A;
    A = H; addByte( D, F & Carry ); H = A;
    A = a;
    if( HL() == 0 ) F |= Zero; else F &= ~Zero;
}

void opcode_ed_5b()    // LD DE, (nn)
{
    unsigned    addr = fetchWord();

    E = readByte( addr );
    D = readByte( addr+1 );
}

void opcode_ed_5c()    // NEG
{
    opcode_ed_44();
}

void opcode_ed_5d()    // RETN
{
    opcode_ed_45();
}

void opcode_ed_5e()    // IM 2
{
    setInterruptMode( 2 );
}

void opcode_ed_5f()    // LD A, R
{
    A = R;
    setFlags35PSZ();
    F &= ~(Halfcarry | Parity | AddSub);
    if( iflags_ & IFF2 ) F |= Parity;
}

void opcode_ed_60()    // IN H, (C)
{
    H = inpReg();
}

void opcode_ed_61()    // OUT (C), H
{
    writePort( C, H );
}

void opcode_ed_62()    // SBC HL, HL
{
    unsigned char a;

    a = A;
    A = L; L = subByte( L, F & Carry );
    A = H; H = subByte( H, F & Carry );
    A = a;
    if( HL() == 0 ) F |= Zero; else F &= ~Zero;
}

void opcode_ed_63()    // LD (nn), HL
{
    unsigned addr = fetchWord();

    writeByte( addr, L );
    writeByte( addr+1, H );
}

void opcode_ed_64()    // NEG
{
    opcode_ed_44();
}

void opcode_ed_65()    // RETN
{
    opcode_ed_45();
}

void opcode_ed_66()    // IM 0
{
    setInterruptMode( 0 );
}

void opcode_ed_67()    // RRD
{
    unsigned char   x = readByte( HL() );

    writeByte( HL(), (A << 4) | (x >> 4) );
    A = (A & 0xF0) | (x & 0x0F);
    setFlags35PSZ();
    F &= ~(Halfcarry | AddSub);
}

void opcode_ed_68()    // IN L, (C)
{
    L = inpReg();
}

void opcode_ed_69()    // OUT (C), L
{
    writePort( C, L );
}

void opcode_ed_6a()    // ADC HL, HL
{
    unsigned char a;

    a = A;
    A = L; addByte( L, F & Carry ); L = A;
    A = H; addByte( H, F & Carry ); H = A;
    A = a;
    if( HL() == 0 ) F |= Zero; else F &= ~Zero;
}

void opcode_ed_6b()    // LD HL, (nn)
{
    unsigned    addr = fetchWord();

    L = readByte( addr );
    H = readByte( addr+1 );
}

void opcode_ed_6c()    // NEG
{
    opcode_ed_44();
}

void opcode_ed_6d()    // RETN
{
    opcode_ed_45();
}

void opcode_ed_6e()    // IM 0/1
{
    setInterruptMode( 0 );
}

void opcode_ed_6f()    // RLD
{
    unsigned char   x = readByte( HL() );

    writeByte( HL(), (x << 4) | (A & 0x0F) );
    A = (A & 0xF0) | (x >> 4);
    setFlags35PSZ();
    F &= ~(Halfcarry | AddSub);
}

void opcode_ed_70()    // IN (C) / IN F, (C)
{
    inpReg();
}

void opcode_ed_71()    // OUT (C), 0
{
    writePort( C, 0 );
}

void opcode_ed_72()    // SBC HL, SP
{
    unsigned char a;

    a = A;
    A = L; L = subByte( SP & 0xFF, F & Carry );
    A = H; H = subByte( (SP >> 8) & 0xFF, F & Carry );
    A = a;
    if( HL() == 0 ) F |= Zero; else F &= ~Zero;
}

void opcode_ed_73()    // LD (nn), SP
{
    writeWord( fetchWord(), SP );
}

void opcode_ed_74()    // NEG
{
    opcode_ed_44();
}

void opcode_ed_75()    // RETN
{
    opcode_ed_45();
}

void opcode_ed_76()    // IM 1
{
    setInterruptMode( 1 );
}

void opcode_ed_78()    // IN A, (C)
{
    A = inpReg();
}

void opcode_ed_79()    // OUT (C), A
{
    writePort( C, A );
}

void opcode_ed_7a()    // ADC HL, SP
{
    unsigned char a;

    a = A;
    A = L; addByte( SP & 0xFF, F & Carry ); L = A;
    A = H; addByte( (SP >> 8) & 0xFF, F & Carry ); H = A;
    A = a;
    if( HL() == 0 ) F |= Zero; else F &= ~Zero;
}

void opcode_ed_7b()    // LD SP, (nn)
{
    SP = readWord( fetchWord() );
}

void opcode_ed_7c()    // NEG
{
    opcode_ed_44();
}

void opcode_ed_7d()    // RETN
{
    opcode_ed_45();
}

void opcode_ed_7e()    // IM 2
{
    setInterruptMode( 2 );
}

void opcode_ed_a0()    // LDI
{
    writeByte( DE(), readByte( HL() ) );
    if( ++L == 0 ) ++H; // HL++
    if( ++E == 0 ) ++D; // DE++
    if( C-- == 0 ) --B; // BC--
    F &= ~(Halfcarry | Subtraction | Parity);
    if( BC() ) F |= Parity;
}

void opcode_ed_a1()    // CPI
{
    unsigned char f = F;

    cmpByte( readByte( HL() ) );
    if( ++L == 0 ) ++H; // HL++
    if( C-- == 0 ) --B; // BC--
    F = (F & ~(Carry | Parity)) | (f & Carry);
    if( BC() ) F |= Parity;
}

void opcode_ed_a2()    // INI
{
    writeByte( HL(), readPort( C ) );
    if( ++L == 0 ) ++H; // HL++
    B = decByte( B );
}

void opcode_ed_a3()    // OUTI
{
    writePort( C, readByte( HL() ) );
    if( ++L == 0 ) ++H; // HL++
    B = decByte( B );
}

void opcode_ed_a8()    // LDD
{
    writeByte( DE(), readByte( HL() ) );
    if( L-- == 0 ) --H; // HL--
    if( E-- == 0 ) --D; // DE--
    if( C-- == 0 ) --B; // BC--
    F &= ~(Halfcarry | Subtraction | Parity);
    if( BC() ) F |= Parity;
}

void opcode_ed_a9()    // CPD
{
    unsigned char f = F;

    cmpByte( readByte( HL() ) );
    if( L-- == 0 ) --H; // HL--
    if( C-- == 0 ) --B; // BC--
    F = (F & ~(Carry | Parity)) | (f & Carry);
    if( BC() ) F |= Parity;
}

void opcode_ed_aa()    // IND
{
    writeByte( HL(), readPort( C ) );
    if( L-- == 0 ) --H; // HL--
    B = decByte( B );
}

void opcode_ed_ab()    // OUTD
{
    writePort( C, readByte( HL() ) );
    if( L-- == 0 ) --H; // HL--
    B = decByte( B );
}

void opcode_ed_b0()    // LDIR
{
    opcode_ed_a0(); // LDI
    if( F & Parity ) { // After LDI, the Parity flag will be zero when BC=0
        cycles_ += 5;
        PC -= 2; // Decrement PC so that instruction is re-executed at next step (this allows interrupts to occur)
    }
}

void opcode_ed_b1()    // CPIR
{
    opcode_ed_a1(); // CPI
    if( (F & Parity) && !(F & Zero) ) { // Parity clear when BC=0, Zero set when A=(HL)
        cycles_ += 5;
        PC -= 2; // Decrement PC so that instruction is re-executed at next step (this allows interrupts to occur)
    }
}

void opcode_ed_b2()    // INIR
{
    opcode_ed_a2(); // INI
    if( B != 0 ) {
        cycles_ += 5;
        PC -= 2; // Decrement PC so that instruction is re-executed at next step (this allows interrupts to occur)
    }
}

void opcode_ed_b3()    // OTIR
{
    opcode_ed_a3(); // OUTI
    if( B != 0 ) {
        cycles_ += 5;
        PC -= 2; // Decrement PC so that instruction is re-executed at next step (this allows interrupts to occur)
    }
}

void opcode_ed_b8()    // LDDR
{
    opcode_ed_a8(); // LDD
    if( F & Parity ) { // After LDD, the Parity flag will be zero when BC=0
        cycles_ += 5;
        PC -= 2; // Decrement PC so that instruction is re-executed at next step (this allows interrupts to occur)
    }
}

void opcode_ed_b9()    // CPDR
{
    opcode_ed_a9(); // CPD
    if( (F & Parity) && !(F & Zero) ) { // Parity clear when BC=0, Zero set when A=(HL)
        cycles_ += 5;
        PC -= 2; // Decrement PC so that instruction is re-executed at next step (this allows interrupts to occur)
    }
}

void opcode_ed_ba()    // INDR
{
    opcode_ed_aa(); // IND
    if( B != 0 ) {
        cycles_ += 5;
        PC -= 2; // Decrement PC so that instruction is re-executed at next step (this allows interrupts to occur)
    }
}
                            
void opcode_ed_bb()    // OTDR
{
    opcode_ed_ab(); // OUTD
    if( B != 0 ) {
        cycles_ += 5;
        PC -= 2; // Decrement PC so that instruction is re-executed at next step (this allows interrupts to occur)
    }
}

OpcodeInfo OpInfoFD_[256] = {
    { 0, 0 }, // 0x00
    { 0, 0 }, // 0x01
    { 0, 0 }, // 0x02
    { 0, 0 }, // 0x03
    { 0, 0 }, // 0x04
    { 0, 0 }, // 0x05
    { 0, 0 }, // 0x06
    { 0, 0 }, // 0x07
    { 0, 0 }, // 0x08
    { &opcode_fd_09, 15 }, // ADD IY, BC
    { 0, 0 }, // 0x0A
    { 0, 0 }, // 0x0B
    { 0, 0 }, // 0x0C
    { 0, 0 }, // 0x0D
    { 0, 0 }, // 0x0E
    { 0, 0 }, // 0x0F
    { 0, 0 }, // 0x10
    { 0, 0 }, // 0x11
    { 0, 0 }, // 0x12
    { 0, 0 }, // 0x13
    { 0, 0 }, // 0x14
    { 0, 0 }, // 0x15
    { 0, 0 }, // 0x16
    { 0, 0 }, // 0x17
    { 0, 0 }, // 0x18
    { &opcode_fd_19, 15 }, // ADD IY, DE
    { 0, 0 }, // 0x1A
    { 0, 0 }, // 0x1B
    { 0, 0 }, // 0x1C
    { 0, 0 }, // 0x1D
    { 0, 0 }, // 0x1E
    { 0, 0 }, // 0x1F
    { 0, 0 }, // 0x20
    { &opcode_fd_21, 14 }, // LD IY, nn
    { &opcode_fd_22, 20 }, // LD (nn), IY
    { &opcode_fd_23, 10 }, // INC IY
    { &opcode_fd_24,  9 }, // INC IYH
    { &opcode_fd_25,  9 }, // DEC IYH
    { &opcode_fd_26,  9 }, // LD IYH, n
    { 0, 0 }, // 0x27
    { 0, 0 }, // 0x28
    { &opcode_fd_29, 15 }, // ADD IY, IY
    { &opcode_fd_2a, 20 }, // LD IY, (nn)
    { &opcode_fd_2b, 10 }, // DEC IY
    { &opcode_fd_2c,  9 }, // INC IYL
    { &opcode_fd_2d,  9 }, // DEC IYL
    { &opcode_fd_2e,  9 }, // LD IYL, n
    { 0, 0 }, // 0x2F
    { 0, 0 }, // 0x30
    { 0, 0 }, // 0x31
    { 0, 0 }, // 0x32
    { 0, 0 }, // 0x33
    { &opcode_fd_34, 23 }, // INC (IY + d)
    { &opcode_fd_35, 23 }, // DEC (IY + d)
    { &opcode_fd_36, 19 }, // LD (IY + d), n
    { 0, 0 }, // 0x37
    { 0, 0 }, // 0x38
    { &opcode_fd_39, 15 }, // ADD IY, SP
    { 0, 0 }, // 0x3A
    { 0, 0 }, // 0x3B
    { 0, 0 }, // 0x3C
    { 0, 0 }, // 0x3D
    { 0, 0 }, // 0x3E
    { 0, 0 }, // 0x3F
    { 0, 0 }, // 0x40
    { 0, 0 }, // 0x41
    { 0, 0 }, // 0x42
    { 0, 0 }, // 0x43
    { &opcode_fd_44,  9 }, // LD B, IYH
    { &opcode_fd_45,  9 }, // LD B, IYL
    { &opcode_fd_46, 19 }, // LD B, (IY + d)
    { 0, 0 }, // 0x47
    { 0, 0 }, // 0x48
    { 0, 0 }, // 0x49
    { 0, 0 }, // 0x4A
    { 0, 0 }, // 0x4B
    { &opcode_fd_4c,  9 }, // LD C, IYH
    { &opcode_fd_4d,  9 }, // LD C, IYL
    { &opcode_fd_4e, 19 }, // LD C, (IY + d)
    { 0, 0 }, // 0x4F
    { 0, 0 }, // 0x50
    { 0, 0 }, // 0x51
    { 0, 0 }, // 0x52
    { 0, 0 }, // 0x53
    { &opcode_fd_54,  9 }, // LD D, IYH
    { &opcode_fd_55,  9 }, // LD D, IYL
    { &opcode_fd_56, 19 }, // LD D, (IY + d)
    { 0, 0 }, // 0x57
    { 0, 0 }, // 0x58
    { 0, 0 }, // 0x59
    { 0, 0 }, // 0x5A
    { 0, 0 }, // 0x5B
    { &opcode_fd_5c,  9 }, // LD E, IYH
    { &opcode_fd_5d,  9 }, // LD E, IYL
    { &opcode_fd_5e, 19 }, // LD E, (IY + d)
    { 0, 0 }, // 0x5F
    { &opcode_fd_60,  9 }, // LD IYH, B
    { &opcode_fd_61,  9 }, // LD IYH, C
    { &opcode_fd_62,  9 }, // LD IYH, D
    { &opcode_fd_63,  9 }, // LD IYH, E
    { &opcode_fd_64,  9 }, // LD IYH, IYH
    { &opcode_fd_65,  9 }, // LD IYH, IYL
    { &opcode_fd_66,  9 }, // LD H, (IY + d)
    { &opcode_fd_67,  9 }, // LD IYH, A
    { &opcode_fd_68,  9 }, // LD IYL, B
    { &opcode_fd_69,  9 }, // LD IYL, C
    { &opcode_fd_6a,  9 }, // LD IYL, D
    { &opcode_fd_6b,  9 }, // LD IYL, E
    { &opcode_fd_6c,  9 }, // LD IYL, IYH
    { &opcode_fd_6d,  9 }, // LD IYL, IYL
    { &opcode_fd_6e,  9 }, // LD L, (IY + d)
    { &opcode_fd_6f,  9 }, // LD IYL, A
    { &opcode_fd_70, 19 }, // LD (IY + d), B
    { &opcode_fd_71, 19 }, // LD (IY + d), C
    { &opcode_fd_72, 19 }, // LD (IY + d), D
    { &opcode_fd_73, 19 }, // LD (IY + d), E
    { &opcode_fd_74, 19 }, // LD (IY + d), H
    { &opcode_fd_75, 19 }, // LD (IY + d), L
    { 0,19 }, // 0x76
    { &opcode_fd_77, 19 }, // LD (IY + d), A
    { 0, 0 }, // 0x78
    { 0, 0 }, // 0x79
    { 0, 0 }, // 0x7A
    { 0, 0 }, // 0x7B
    { &opcode_fd_7c,  9 }, // LD A, IYH
    { &opcode_fd_7d,  9 }, // LD A, IYL
    { &opcode_fd_7e, 19 }, // LD A, (IY + d)
    { 0, 0 }, // 0x7F
    { 0, 0 }, // 0x80
    { 0, 0 }, // 0x81
    { 0, 0 }, // 0x82
    { 0, 0 }, // 0x83
    { &opcode_fd_84,  9 }, // ADD A, IYH
    { &opcode_fd_85,  9 }, // ADD A, IYL
    { &opcode_fd_86, 19 }, // ADD A, (IY + d)
    { 0, 0 }, // 0x87
    { 0, 0 }, // 0x88
    { 0, 0 }, // 0x89
    { 0, 0 }, // 0x8A
    { 0, 0 }, // 0x8B
    { &opcode_fd_8c,  9 }, // ADC A, IYH
    { &opcode_fd_8d,  9 }, // ADC A, IYL
    { &opcode_fd_8e, 19 }, // ADC A, (IY + d)
    { 0, 0 }, // 0x8F
    { 0, 0 }, // 0x90
    { 0, 0 }, // 0x91
    { 0, 0 }, // 0x92
    { 0, 0 }, // 0x93
    { &opcode_fd_94,  9 }, // SUB IYH
    { &opcode_fd_95,  9 }, // SUB IYL
    { &opcode_fd_96, 19 }, // SUB (IY + d)
    { 0, 0 }, // 0x97
    { 0, 0 }, // 0x98
    { 0, 0 }, // 0x99
    { 0, 0 }, // 0x9A
    { 0, 0 }, // 0x9B
    { &opcode_fd_9c,  9 }, // SBC A, IYH
    { &opcode_fd_9d,  9 }, // SBC A, IYL
    { &opcode_fd_9e, 19 }, // SBC A, (IY + d)
    { 0, 0 }, // 0x9F
    { 0, 0 }, // 0xA0
    { 0, 0 }, // 0xA1
    { 0, 0 }, // 0xA2
    { 0, 0 }, // 0xA3
    { &opcode_fd_a4,  9 }, // AND IYH
    { &opcode_fd_a5,  9 }, // AND IYL
    { &opcode_fd_a6, 19 }, // AND (IY + d)
    { 0, 0 }, // 0xA7
    { 0, 0 }, // 0xA8
    { 0, 0 }, // 0xA9
    { 0, 0 }, // 0xAA
    { 0, 0 }, // 0xAB
    { &opcode_fd_ac,  9 }, // XOR IYH
    { &opcode_fd_ad,  9 }, // XOR IYL
    { &opcode_fd_ae, 19 }, // XOR (IY + d)
    { 0, 0 }, // 0xAF
    { 0, 0 }, // 0xB0
    { 0, 0 }, // 0xB1
    { 0, 0 }, // 0xB2
    { 0, 0 }, // 0xB3
    { &opcode_fd_b4,  9 }, // OR IYH
    { &opcode_fd_b5,  9 }, // OR IYL
    { &opcode_fd_b6, 19 }, // OR (IY + d)
    { 0, 0 }, // 0xB7
    { 0, 0 }, // 0xB8
    { 0, 0 }, // 0xB9
    { 0, 0 }, // 0xBA
    { 0, 0 }, // 0xBB
    { &opcode_fd_bc,  9 }, // CP IYH
    { &opcode_fd_bd,  9 }, // CP IYL
    { &opcode_fd_be, 19 }, // CP (IY + d)
    { 0, 0 }, // 0xBF
    { 0, 0 }, // 0xC0
    { 0, 0 }, // 0xC1
    { 0, 0 }, // 0xC2
    { 0, 0 }, // 0xC3
    { 0, 0 }, // 0xC4
    { 0, 0 }, // 0xC5
    { 0, 0 }, // 0xC6
    { 0, 0 }, // 0xC7
    { 0, 0 }, // 0xC8
    { 0, 0 }, // 0xC9
    { 0, 0 }, // 0xCA
    { &opcode_fd_cb,  0 }, // 
    { 0, 0 }, // 0xCC
    { 0, 0 }, // 0xCD
    { 0, 0 }, // 0xCE
    { 0, 0 }, // 0xCF
    { 0, 0 }, // 0xD0
    { 0, 0 }, // 0xD1
    { 0, 0 }, // 0xD2
    { 0, 0 }, // 0xD3
    { 0, 0 }, // 0xD4
    { 0, 0 }, // 0xD5
    { 0, 0 }, // 0xD6
    { 0, 0 }, // 0xD7
    { 0, 0 }, // 0xD8
    { 0, 0 }, // 0xD9
    { 0, 0 }, // 0xDA
    { 0, 0 }, // 0xDB
    { 0, 0 }, // 0xDC
    { 0, 0 }, // 0xDD
    { 0, 0 }, // 0xDE
    { 0, 0 }, // 0xDF
    { 0, 0 }, // 0xE0
    { &opcode_fd_e1, 14 }, // POP IY
    { 0, 0 }, // 0xE2
    { &opcode_fd_e3, 23 }, // EX (SP), IY
    { 0, 0 }, // 0xE4
    { &opcode_fd_e5, 15 }, // PUSH IY
    { 0, 0 }, // 0xE6
    { 0, 0 }, // 0xE7
    { 0, 0 }, // 0xE8
    { &opcode_fd_e9,  8 }, // JP (IY)
    { 0, 0 }, // 0xEA
    { 0, 0 }, // 0xEB
    { 0, 0 }, // 0xEC
    { 0, 0 }, // 0xED
    { 0, 0 }, // 0xEE
    { 0, 0 }, // 0xEF
    { 0, 0 }, // 0xF0
    { 0, 0 }, // 0xF1
    { 0, 0 }, // 0xF2
    { 0, 0 }, // 0xF3
    { 0, 0 }, // 0xF4
    { 0, 0 }, // 0xF5
    { 0, 0 }, // 0xF6
    { 0, 0 }, // 0xF7
    { 0, 0 }, // 0xF8
    { &opcode_fd_f9, 10 }, // LD SP, IY
    { 0, 0 }, // 0xFA
    { 0, 0 }, // 0xFB
    { 0, 0 }, // 0xFC
    { 0, 0 }, // 0xFD
    { 0, 0 }, // 0xFE
    { 0, 0 }  // 0xFF
};

void opcode_fd_09()    // ADD IY, BC
{
    unsigned rr = BC();

    F &= (Zero | Sign | Parity);
    if( ((IY & 0xFFF)+(rr & 0xFFF)) > 0xFFF ) F |= Halfcarry;
    IY += rr;
    if( IY & 0x10000 ) F |= Carry;
}

void opcode_fd_19()    // ADD IY, DE
{
    unsigned rr = DE();

    F &= (Zero | Sign | Parity);
    if( ((IY & 0xFFF)+(rr & 0xFFF)) > 0xFFF ) F |= Halfcarry;
    IY += rr;
    if( IY & 0x10000 ) F |= Carry;
}

void opcode_fd_21()    // LD IY, nn
{
    IY = fetchWord();
}

void opcode_fd_22()    // LD (nn), IY
{
    writeWord( fetchWord(), IY );
}

void opcode_fd_23()    // INC IY
{
    IY++;
}

void opcode_fd_24()    // INC IYH
{
    IY = (IY & 0xFF) | ((unsigned)incByte( IY >> 8 ) << 8);
}

void opcode_fd_25()    // DEC IYH
{
    IY = (IY & 0xFF) | ((unsigned)decByte( IY >> 8 ) << 8);
}

void opcode_fd_26()    // LD IYH, n
{
    IY = (IY & 0xFF) | ((unsigned)fetchByte() << 8);
}

void opcode_fd_29()    // ADD IY, IY
{
    F &= (Zero | Sign | Parity);
    if( IY & 0x800 ) F |= Halfcarry;
    IY += IY;
    if( IY & 0x10000 ) F |= Carry;
}

void opcode_fd_2a()    // LD IY, (nn)
{
    IY = readWord( fetchWord() );
}

void opcode_fd_2b()    // DEC IY
{
    IY--;
}

void opcode_fd_2c()    // INC IYL
{
    IY = (IY & 0xFF00) | incByte( IY & 0xFF );
}

void opcode_fd_2d()    // DEC IYL
{
    IY = (IY & 0xFF00) | decByte( IY & 0xFF );
}

void opcode_fd_2e()    // LD IYL, n
{
    IY = (IY & 0xFF00) | fetchByte();
}

void opcode_fd_34()    // INC (IY + d)
{
    unsigned    addr = addDispl( IY, fetchByte() );

    writeByte( addr, incByte( readByte( addr ) ) );
}

void opcode_fd_35()    // DEC (IY + d)
{
    unsigned    addr = addDispl( IY, fetchByte() );

    writeByte( addr, decByte( readByte( addr ) ) );
}

void opcode_fd_36()    // LD (IY + d), n
{
    unsigned    addr = addDispl( IY, fetchByte() );

    writeByte( addr, fetchByte() );
}

void opcode_fd_39()    // ADD IY, SP
{
    F &= (Zero | Sign | Parity);
    if( ((IY & 0xFFF)+(SP & 0xFFF)) > 0xFFF ) F |= Halfcarry;
    IY += SP;
    if( IY & 0x10000 ) F |= Carry;
}

void opcode_fd_44()    // LD B, IYH
{
    B = IY >> 8;
}

void opcode_fd_45()    // LD B, IYL
{
    B = IY & 0xFF;
}

void opcode_fd_46()    // LD B, (IY + d)
{
    B = readByte( addDispl(IY,fetchByte()) );
}

void opcode_fd_4c()    // LD C, IYH
{
    C = IY >> 8;
}

void opcode_fd_4d()    // LD C, IYL
{
    C = IY & 0xFF;
}

void opcode_fd_4e()    // LD C, (IY + d)
{
    C = readByte( addDispl(IY,fetchByte()) );
}

void opcode_fd_54()    // LD D, IYH
{
    D = IY >> 8;
}

void opcode_fd_55()    // LD D, IYL
{
    D = IY & 0xFF;
}

void opcode_fd_56()    // LD D, (IY + d)
{
    D = readByte( addDispl(IY,fetchByte()) );
}

void opcode_fd_5c()    // LD E, IYH
{
    E = IY >> 8;
}

void opcode_fd_5d()    // LD E, IYL
{
    E = IY & 0xFF;
}

void opcode_fd_5e()    // LD E, (IY + d)
{
    E = readByte( addDispl(IY,fetchByte()) );
}

void opcode_fd_60()    // LD IYH, B
{
    IY = (IY & 0xFF) | ((unsigned)B << 8);
}

void opcode_fd_61()    // LD IYH, C
{
    IY = (IY & 0xFF) | ((unsigned)C << 8);
}

void opcode_fd_62()    // LD IYH, D
{
    IY = (IY & 0xFF) | ((unsigned)D << 8);
}

void opcode_fd_63()    // LD IYH, E
{
    IY = (IY & 0xFF) | ((unsigned)E << 8);
}

void opcode_fd_64()    // LD IYH, IYH
{
}

void opcode_fd_65()    // LD IYH, IYL
{
    IY = (IY & 0xFF) | ((IY << 8) & 0xFF00);
}

void opcode_fd_66()    // LD H, (IY + d)
{
    H = readByte( addDispl(IY,fetchByte()) );
}

void opcode_fd_67()    // LD IYH, A
{
    IY = (IY & 0xFF) | ((unsigned)A << 8);
}

void opcode_fd_68()    // LD IYL, B
{
    IY = (IY & 0xFF00) | B;
}

void opcode_fd_69()    // LD IYL, C
{
    IY = (IY & 0xFF00) | C;
}

void opcode_fd_6a()    // LD IYL, D
{
    IY = (IY & 0xFF00) | D;
}

void opcode_fd_6b()    // LD IYL, E
{
    IY = (IY & 0xFF00) | E;
}

void opcode_fd_6c()    // LD IYL, IYH
{
    IY = (IY & 0xFF00) | ((IY >> 8) & 0xFF);
}

void opcode_fd_6d()    // LD IYL, IYL
{
}

void opcode_fd_6e()    // LD L, (IY + d)
{
    L = readByte( addDispl(IY,fetchByte()) );
}

void opcode_fd_6f()    // LD IYL, A
{
    IY = (IY & 0xFF00) | A;
}

void opcode_fd_70()    // LD (IY + d), B
{
    writeByte( addDispl(IY,fetchByte()), B );
}

void opcode_fd_71()    // LD (IY + d), C
{
    writeByte( addDispl(IY,fetchByte()), C );
}

void opcode_fd_72()    // LD (IY + d), D
{
    writeByte( addDispl(IY,fetchByte()), D );
}

void opcode_fd_73()    // LD (IY + d), E
{
    writeByte( addDispl(IY,fetchByte()), E );
}

void opcode_fd_74()    // LD (IY + d), H
{
    writeByte( addDispl(IY,fetchByte()), H );
}

void opcode_fd_75()    // LD (IY + d), L
{
    writeByte( addDispl(IY,fetchByte()), L );
}

void opcode_fd_77()    // LD (IY + d), A
{
    writeByte( addDispl(IY,fetchByte()), A );
}

void opcode_fd_7c()    // LD A, IYH
{
    A = IY >> 8;
}

void opcode_fd_7d()    // LD A, IYL
{
    A = IY & 0xFF;
}

void opcode_fd_7e()    // LD A, (IY + d)
{
    A = readByte( addDispl(IY,fetchByte()) );
}

void opcode_fd_84()    // ADD A, IYH
{
    addByte( IY >> 8, 0 );
}

void opcode_fd_85()    // ADD A, IYL
{
    addByte( IY & 0xFF, 0 );
}

void opcode_fd_86()    // ADD A, (IY + d)
{
    addByte( readByte( addDispl(IY,fetchByte()) ), 0 );
}

void opcode_fd_8c()    // ADC A, IYH
{
    addByte( IY >> 8, F & Carry );
}

void opcode_fd_8d()    // ADC A, IYL
{
    addByte( IY & 0xFF, F & Carry );
}

void opcode_fd_8e()    // ADC A, (IY + d)
{
    addByte( readByte( addDispl(IY,fetchByte()) ), F & Carry );
}

void opcode_fd_94()    // SUB IYH
{
    A = subByte( IY >> 8, 0 );
}

void opcode_fd_95()    // SUB IYL
{
    A = subByte( IY & 0xFF, 0 );
}

void opcode_fd_96()    // SUB (IY + d)
{
    A = subByte( readByte( addDispl(IY,fetchByte()) ), 0 );
}

void opcode_fd_9c()    // SBC A, IYH
{
    A = subByte( IY >> 8, F & Carry );
}

void opcode_fd_9d()    // SBC A, IYL
{
    A = subByte( IY & 0xFF, F & Carry );
}

void opcode_fd_9e()    // SBC A, (IY + d)
{
    A = subByte( readByte( addDispl(IY,fetchByte()) ), F & Carry );
}

void opcode_fd_a4()    // AND IYH
{
    A &= IY >> 8;
    setFlags35PSZ000();
    F |= Halfcarry;
}

void opcode_fd_a5()    // AND IYL
{
    A &= IY & 0xFF;
    setFlags35PSZ000();
    F |= Halfcarry;
}

void opcode_fd_a6()    // AND (IY + d)
{
    A &= readByte( addDispl(IY,fetchByte()) );
    setFlags35PSZ000();
    F |= Halfcarry;
}

void opcode_fd_ac()    // XOR IYH
{
    A ^= IY >> 8;
    setFlags35PSZ000();
}

void opcode_fd_ad()    // XOR IYL
{
    A ^= IY & 0xFF;
    setFlags35PSZ000();
}

void opcode_fd_ae()    // XOR (IY + d)
{
    A ^= readByte( addDispl(IY,fetchByte()) );
    setFlags35PSZ000();
}

void opcode_fd_b4()    // OR IYH
{
    A |= IY >> 8;
    setFlags35PSZ000();
}

void opcode_fd_b5()    // OR IYL
{
    A |= IY & 0xFF;
    setFlags35PSZ000();
}

void opcode_fd_b6()    // OR (IY + d)
{
    A |= readByte( addDispl(IY,fetchByte()) );
    setFlags35PSZ000();
}

void opcode_fd_bc()    // CP IYH
{
    cmpByte( IY >> 8 );
}

void opcode_fd_bd()    // CP IYL
{
    cmpByte( IY & 0xFF );
}

void opcode_fd_be()    // CP (IY + d)
{
    cmpByte( readByte( addDispl(IY,fetchByte()) ) );
}

void opcode_fd_cb()    // 
{
    do_opcode_xycb( IY );
}

void opcode_fd_e1()    // POP IY
{
    IY = readWord( SP );
    SP += 2;
}

void opcode_fd_e3()    // EX (SP), IY
{
    unsigned    iy = IY;

    IY = readWord( SP );
    writeWord( SP, iy );
}

void opcode_fd_e5()    // PUSH IY
{
    SP -= 2;
    writeWord( SP, IY );
}

void opcode_fd_e9()    // JP (IY)
{
    PC = IY;
}

void opcode_fd_f9()    // LD SP, IY
{
    SP = IY;
}

OpcodeInfoXY OpInfoXYCB_[256] = {
    { &opcode_xycb_00, 20 }, // LD B, RLC (IX + d)
    { &opcode_xycb_01, 20 }, // LD C, RLC (IX + d)
    { &opcode_xycb_02, 20 }, // LD D, RLC (IX + d)
    { &opcode_xycb_03, 20 }, // LD E, RLC (IX + d)
    { &opcode_xycb_04, 20 }, // LD H, RLC (IX + d)
    { &opcode_xycb_05, 20 }, // LD L, RLC (IX + d)
    { &opcode_xycb_06, 20 }, // RLC (IX + d)
    { &opcode_xycb_07, 20 }, // LD A, RLC (IX + d)
    { &opcode_xycb_08, 20 }, // LD B, RRC (IX + d)
    { &opcode_xycb_09, 20 }, // LD C, RRC (IX + d)
    { &opcode_xycb_0a, 20 }, // LD D, RRC (IX + d)
    { &opcode_xycb_0b, 20 }, // LD E, RRC (IX + d)
    { &opcode_xycb_0c, 20 }, // LD H, RRC (IX + d)
    { &opcode_xycb_0d, 20 }, // LD L, RRC (IX + d)
    { &opcode_xycb_0e, 20 }, // RRC (IX + d)
    { &opcode_xycb_0f, 20 }, // LD A, RRC (IX + d)
    { &opcode_xycb_10, 20 }, // LD B, RL (IX + d)
    { &opcode_xycb_11, 20 }, // LD C, RL (IX + d)
    { &opcode_xycb_12, 20 }, // LD D, RL (IX + d)
    { &opcode_xycb_13, 20 }, // LD E, RL (IX + d)
    { &opcode_xycb_14, 20 }, // LD H, RL (IX + d)
    { &opcode_xycb_15, 20 }, // LD L, RL (IX + d)
    { &opcode_xycb_16, 20 }, // RL (IX + d)
    { &opcode_xycb_17, 20 }, // LD A, RL (IX + d)
    { &opcode_xycb_18, 20 }, // LD B, RR (IX + d)
    { &opcode_xycb_19, 20 }, // LD C, RR (IX + d)
    { &opcode_xycb_1a, 20 }, // LD D, RR (IX + d)
    { &opcode_xycb_1b, 20 }, // LD E, RR (IX + d)
    { &opcode_xycb_1c, 20 }, // LD H, RR (IX + d)
    { &opcode_xycb_1d, 20 }, // LD L, RR (IX + d)
    { &opcode_xycb_1e, 20 }, // RR (IX + d)
    { &opcode_xycb_1f, 20 }, // LD A, RR (IX + d)
    { &opcode_xycb_20, 20 }, // LD B, SLA (IX + d)
    { &opcode_xycb_21, 20 }, // LD C, SLA (IX + d)
    { &opcode_xycb_22, 20 }, // LD D, SLA (IX + d)
    { &opcode_xycb_23, 20 }, // LD E, SLA (IX + d)
    { &opcode_xycb_24, 20 }, // LD H, SLA (IX + d)
    { &opcode_xycb_25, 20 }, // LD L, SLA (IX + d)
    { &opcode_xycb_26, 20 }, // SLA (IX + d)
    { &opcode_xycb_27, 20 }, // LD A, SLA (IX + d)
    { &opcode_xycb_28, 20 }, // LD B, SRA (IX + d)
    { &opcode_xycb_29, 20 }, // LD C, SRA (IX + d)
    { &opcode_xycb_2a, 20 }, // LD D, SRA (IX + d)
    { &opcode_xycb_2b, 20 }, // LD E, SRA (IX + d)
    { &opcode_xycb_2c, 20 }, // LD H, SRA (IX + d)
    { &opcode_xycb_2d, 20 }, // LD L, SRA (IX + d)
    { &opcode_xycb_2e, 20 }, // SRA (IX + d)
    { &opcode_xycb_2f, 20 }, // LD A, SRA (IX + d)
    { &opcode_xycb_30, 20 }, // LD B, SLL (IX + d)
    { &opcode_xycb_31, 20 }, // LD C, SLL (IX + d)
    { &opcode_xycb_32, 20 }, // LD D, SLL (IX + d)
    { &opcode_xycb_33, 20 }, // LD E, SLL (IX + d)
    { &opcode_xycb_34, 20 }, // LD H, SLL (IX + d)
    { &opcode_xycb_35, 20 }, // LD L, SLL (IX + d)
    { &opcode_xycb_36, 20 }, // SLL (IX + d)
    { &opcode_xycb_37, 20 }, // LD A, SLL (IX + d)
    { &opcode_xycb_38, 20 }, // LD B, SRL (IX + d)
    { &opcode_xycb_39, 20 }, // LD C, SRL (IX + d)
    { &opcode_xycb_3a, 20 }, // LD D, SRL (IX + d)
    { &opcode_xycb_3b, 20 }, // LD E, SRL (IX + d)
    { &opcode_xycb_3c, 20 }, // LD H, SRL (IX + d)
    { &opcode_xycb_3d, 20 }, // LD L, SRL (IX + d)
    { &opcode_xycb_3e, 20 }, // SRL (IX + d)
    { &opcode_xycb_3f, 20 }, // LD A, SRL (IX + d)
    { &opcode_xycb_40, 20 }, // BIT 0, (IX + d)
    { &opcode_xycb_41, 20 }, // BIT 0, (IX + d)
    { &opcode_xycb_42, 20 }, // BIT 0, (IX + d)
    { &opcode_xycb_43, 20 }, // BIT 0, (IX + d)
    { &opcode_xycb_44, 20 }, // BIT 0, (IX + d)
    { &opcode_xycb_45, 20 }, // BIT 0, (IX + d)
    { &opcode_xycb_46, 20 }, // BIT 0, (IX + d)
    { &opcode_xycb_47, 20 }, // BIT 0, (IX + d)
    { &opcode_xycb_48, 20 }, // BIT 1, (IX + d)
    { &opcode_xycb_49, 20 }, // BIT 1, (IX + d)
    { &opcode_xycb_4a, 20 }, // BIT 1, (IX + d)
    { &opcode_xycb_4b, 20 }, // BIT 1, (IX + d)
    { &opcode_xycb_4c, 20 }, // BIT 1, (IX + d)
    { &opcode_xycb_4d, 20 }, // BIT 1, (IX + d)
    { &opcode_xycb_4e, 20 }, // BIT 1, (IX + d)
    { &opcode_xycb_4f, 20 }, // BIT 1, (IX + d)
    { &opcode_xycb_50, 20 }, // BIT 2, (IX + d)
    { &opcode_xycb_51, 20 }, // BIT 2, (IX + d)
    { &opcode_xycb_52, 20 }, // BIT 2, (IX + d)
    { &opcode_xycb_53, 20 }, // BIT 2, (IX + d)
    { &opcode_xycb_54, 20 }, // BIT 2, (IX + d)
    { &opcode_xycb_55, 20 }, // BIT 2, (IX + d)
    { &opcode_xycb_56, 20 }, // BIT 2, (IX + d)
    { &opcode_xycb_57, 20 }, // BIT 2, (IX + d)
    { &opcode_xycb_58, 20 }, // BIT 3, (IX + d)
    { &opcode_xycb_59, 20 }, // BIT 3, (IX + d)
    { &opcode_xycb_5a, 20 }, // BIT 3, (IX + d)
    { &opcode_xycb_5b, 20 }, // BIT 3, (IX + d)
    { &opcode_xycb_5c, 20 }, // BIT 3, (IX + d)
    { &opcode_xycb_5d, 20 }, // BIT 3, (IX + d)
    { &opcode_xycb_5e, 20 }, // BIT 3, (IX + d)
    { &opcode_xycb_5f, 20 }, // BIT 3, (IX + d)
    { &opcode_xycb_60, 20 }, // BIT 4, (IX + d)
    { &opcode_xycb_61, 20 }, // BIT 4, (IX + d)
    { &opcode_xycb_62, 20 }, // BIT 4, (IX + d)
    { &opcode_xycb_63, 20 }, // BIT 4, (IX + d)
    { &opcode_xycb_64, 20 }, // BIT 4, (IX + d)
    { &opcode_xycb_65, 20 }, // BIT 4, (IX + d)
    { &opcode_xycb_66, 20 }, // BIT 4, (IX + d)
    { &opcode_xycb_67, 20 }, // BIT 4, (IX + d)
    { &opcode_xycb_68, 20 }, // BIT 5, (IX + d)
    { &opcode_xycb_69, 20 }, // BIT 5, (IX + d)
    { &opcode_xycb_6a, 20 }, // BIT 5, (IX + d)
    { &opcode_xycb_6b, 20 }, // BIT 5, (IX + d)
    { &opcode_xycb_6c, 20 }, // BIT 5, (IX + d)
    { &opcode_xycb_6d, 20 }, // BIT 5, (IX + d)
    { &opcode_xycb_6e, 20 }, // BIT 5, (IX + d)
    { &opcode_xycb_6f, 20 }, // BIT 5, (IX + d)
    { &opcode_xycb_70, 20 }, // BIT 6, (IX + d)
    { &opcode_xycb_71, 20 }, // BIT 6, (IX + d)
    { &opcode_xycb_72, 20 }, // BIT 6, (IX + d)
    { &opcode_xycb_73, 20 }, // BIT 6, (IX + d)
    { &opcode_xycb_74, 20 }, // BIT 6, (IX + d)
    { &opcode_xycb_75, 20 }, // BIT 6, (IX + d)
    { &opcode_xycb_76, 20 }, // BIT 6, (IX + d)
    { &opcode_xycb_77, 20 }, // BIT 6, (IX + d)
    { &opcode_xycb_78, 20 }, // BIT 7, (IX + d)
    { &opcode_xycb_79, 20 }, // BIT 7, (IX + d)
    { &opcode_xycb_7a, 20 }, // BIT 7, (IX + d)
    { &opcode_xycb_7b, 20 }, // BIT 7, (IX + d)
    { &opcode_xycb_7c, 20 }, // BIT 7, (IX + d)
    { &opcode_xycb_7d, 20 }, // BIT 7, (IX + d)
    { &opcode_xycb_7e, 20 }, // BIT 7, (IX + d)
    { &opcode_xycb_7f, 20 }, // BIT 7, (IX + d)
    { &opcode_xycb_80, 20 }, // LD B, RES 0, (IX + d)
    { &opcode_xycb_81, 20 }, // LD C, RES 0, (IX + d)
    { &opcode_xycb_82, 20 }, // LD D, RES 0, (IX + d)
    { &opcode_xycb_83, 20 }, // LD E, RES 0, (IX + d)
    { &opcode_xycb_84, 20 }, // LD H, RES 0, (IX + d)
    { &opcode_xycb_85, 20 }, // LD L, RES 0, (IX + d)
    { &opcode_xycb_86, 20 }, // RES 0, (IX + d)
    { &opcode_xycb_87, 20 }, // LD A, RES 0, (IX + d)
    { &opcode_xycb_88, 20 }, // LD B, RES 1, (IX + d)
    { &opcode_xycb_89, 20 }, // LD C, RES 1, (IX + d)
    { &opcode_xycb_8a, 20 }, // LD D, RES 1, (IX + d)
    { &opcode_xycb_8b, 20 }, // LD E, RES 1, (IX + d)
    { &opcode_xycb_8c, 20 }, // LD H, RES 1, (IX + d)
    { &opcode_xycb_8d, 20 }, // LD L, RES 1, (IX + d)
    { &opcode_xycb_8e, 20 }, // RES 1, (IX + d)
    { &opcode_xycb_8f, 20 }, // LD A, RES 1, (IX + d)
    { &opcode_xycb_90, 20 }, // LD B, RES 2, (IX + d)
    { &opcode_xycb_91, 20 }, // LD C, RES 2, (IX + d)
    { &opcode_xycb_92, 20 }, // LD D, RES 2, (IX + d)
    { &opcode_xycb_93, 20 }, // LD E, RES 2, (IX + d)
    { &opcode_xycb_94, 20 }, // LD H, RES 2, (IX + d)
    { &opcode_xycb_95, 20 }, // LD L, RES 2, (IX + d)
    { &opcode_xycb_96, 20 }, // RES 2, (IX + d)
    { &opcode_xycb_97, 20 }, // LD A, RES 2, (IX + d)
    { &opcode_xycb_98, 20 }, // LD B, RES 3, (IX + d)
    { &opcode_xycb_99, 20 }, // LD C, RES 3, (IX + d)
    { &opcode_xycb_9a, 20 }, // LD D, RES 3, (IX + d)
    { &opcode_xycb_9b, 20 }, // LD E, RES 3, (IX + d)
    { &opcode_xycb_9c, 20 }, // LD H, RES 3, (IX + d)
    { &opcode_xycb_9d, 20 }, // LD L, RES 3, (IX + d)
    { &opcode_xycb_9e, 20 }, // RES 3, (IX + d)
    { &opcode_xycb_9f, 20 }, // LD A, RES 3, (IX + d)
    { &opcode_xycb_a0, 20 }, // LD B, RES 4, (IX + d)
    { &opcode_xycb_a1, 20 }, // LD C, RES 4, (IX + d)
    { &opcode_xycb_a2, 20 }, // LD D, RES 4, (IX + d)
    { &opcode_xycb_a3, 20 }, // LD E, RES 4, (IX + d)
    { &opcode_xycb_a4, 20 }, // LD H, RES 4, (IX + d)
    { &opcode_xycb_a5, 20 }, // LD L, RES 4, (IX + d)
    { &opcode_xycb_a6, 20 }, // RES 4, (IX + d)
    { &opcode_xycb_a7, 20 }, // LD A, RES 4, (IX + d)
    { &opcode_xycb_a8, 20 }, // LD B, RES 5, (IX + d)
    { &opcode_xycb_a9, 20 }, // LD C, RES 5, (IX + d)
    { &opcode_xycb_aa, 20 }, // LD D, RES 5, (IX + d)
    { &opcode_xycb_ab, 20 }, // LD E, RES 5, (IX + d)
    { &opcode_xycb_ac, 20 }, // LD H, RES 5, (IX + d)
    { &opcode_xycb_ad, 20 }, // LD L, RES 5, (IX + d)
    { &opcode_xycb_ae, 20 }, // RES 5, (IX + d)
    { &opcode_xycb_af, 20 }, // LD A, RES 5, (IX + d)
    { &opcode_xycb_b0, 20 }, // LD B, RES 6, (IX + d)
    { &opcode_xycb_b1, 20 }, // LD C, RES 6, (IX + d)
    { &opcode_xycb_b2, 20 }, // LD D, RES 6, (IX + d)
    { &opcode_xycb_b3, 20 }, // LD E, RES 6, (IX + d)
    { &opcode_xycb_b4, 20 }, // LD H, RES 6, (IX + d)
    { &opcode_xycb_b5, 20 }, // LD L, RES 6, (IX + d)
    { &opcode_xycb_b6, 20 }, // RES 6, (IX + d)
    { &opcode_xycb_b7, 20 }, // LD A, RES 6, (IX + d)
    { &opcode_xycb_b8, 20 }, // LD B, RES 7, (IX + d)
    { &opcode_xycb_b9, 20 }, // LD C, RES 7, (IX + d)
    { &opcode_xycb_ba, 20 }, // LD D, RES 7, (IX + d)
    { &opcode_xycb_bb, 20 }, // LD E, RES 7, (IX + d)
    { &opcode_xycb_bc, 20 }, // LD H, RES 7, (IX + d)
    { &opcode_xycb_bd, 20 }, // LD L, RES 7, (IX + d)
    { &opcode_xycb_be, 20 }, // RES 7, (IX + d)
    { &opcode_xycb_bf, 20 }, // LD A, RES 7, (IX + d)
    { &opcode_xycb_c0, 20 }, // LD B, SET 0, (IX + d)
    { &opcode_xycb_c1, 20 }, // LD C, SET 0, (IX + d)
    { &opcode_xycb_c2, 20 }, // LD D, SET 0, (IX + d)
    { &opcode_xycb_c3, 20 }, // LD E, SET 0, (IX + d)
    { &opcode_xycb_c4, 20 }, // LD H, SET 0, (IX + d)
    { &opcode_xycb_c5, 20 }, // LD L, SET 0, (IX + d)
    { &opcode_xycb_c6, 20 }, // SET 0, (IX + d)
    { &opcode_xycb_c7, 20 }, // LD A, SET 0, (IX + d)
    { &opcode_xycb_c8, 20 }, // LD B, SET 1, (IX + d)
    { &opcode_xycb_c9, 20 }, // LD C, SET 1, (IX + d)
    { &opcode_xycb_ca, 20 }, // LD D, SET 1, (IX + d)
    { &opcode_xycb_cb, 20 }, // LD E, SET 1, (IX + d)
    { &opcode_xycb_cc, 20 }, // LD H, SET 1, (IX + d)
    { &opcode_xycb_cd, 20 }, // LD L, SET 1, (IX + d)
    { &opcode_xycb_ce, 20 }, // SET 1, (IX + d)
    { &opcode_xycb_cf, 20 }, // LD A, SET 1, (IX + d)
    { &opcode_xycb_d0, 20 }, // LD B, SET 2, (IX + d)
    { &opcode_xycb_d1, 20 }, // LD C, SET 2, (IX + d)
    { &opcode_xycb_d2, 20 }, // LD D, SET 2, (IX + d)
    { &opcode_xycb_d3, 20 }, // LD E, SET 2, (IX + d)
    { &opcode_xycb_d4, 20 }, // LD H, SET 2, (IX + d)
    { &opcode_xycb_d5, 20 }, // LD L, SET 2, (IX + d)
    { &opcode_xycb_d6, 20 }, // SET 2, (IX + d)
    { &opcode_xycb_d7, 20 }, // LD A, SET 2, (IX + d)
    { &opcode_xycb_d8, 20 }, // LD B, SET 3, (IX + d)
    { &opcode_xycb_d9, 20 }, // LD C, SET 3, (IX + d)
    { &opcode_xycb_da, 20 }, // LD D, SET 3, (IX + d)
    { &opcode_xycb_db, 20 }, // LD E, SET 3, (IX + d)
    { &opcode_xycb_dc, 20 }, // LD H, SET 3, (IX + d)
    { &opcode_xycb_dd, 20 }, // LD L, SET 3, (IX + d)
    { &opcode_xycb_de, 20 }, // SET 3, (IX + d)
    { &opcode_xycb_df, 20 }, // LD A, SET 3, (IX + d)
    { &opcode_xycb_e0, 20 }, // LD B, SET 4, (IX + d)
    { &opcode_xycb_e1, 20 }, // LD C, SET 4, (IX + d)
    { &opcode_xycb_e2, 20 }, // LD D, SET 4, (IX + d)
    { &opcode_xycb_e3, 20 }, // LD E, SET 4, (IX + d)
    { &opcode_xycb_e4, 20 }, // LD H, SET 4, (IX + d)
    { &opcode_xycb_e5, 20 }, // LD L, SET 4, (IX + d)
    { &opcode_xycb_e6, 20 }, // SET 4, (IX + d)
    { &opcode_xycb_e7, 20 }, // LD A, SET 4, (IX + d)
    { &opcode_xycb_e8, 20 }, // LD B, SET 5, (IX + d)
    { &opcode_xycb_e9, 20 }, // LD C, SET 5, (IX + d)
    { &opcode_xycb_ea, 20 }, // LD D, SET 5, (IX + d)
    { &opcode_xycb_eb, 20 }, // LD E, SET 5, (IX + d)
    { &opcode_xycb_ec, 20 }, // LD H, SET 5, (IX + d)
    { &opcode_xycb_ed, 20 }, // LD L, SET 5, (IX + d)
    { &opcode_xycb_ee, 20 }, // SET 5, (IX + d)
    { &opcode_xycb_ef, 20 }, // LD A, SET 5, (IX + d)
    { &opcode_xycb_f0, 20 }, // LD B, SET 6, (IX + d)
    { &opcode_xycb_f1, 20 }, // LD C, SET 6, (IX + d)
    { &opcode_xycb_f2, 20 }, // LD D, SET 6, (IX + d)
    { &opcode_xycb_f3, 20 }, // LD E, SET 6, (IX + d)
    { &opcode_xycb_f4, 20 }, // LD H, SET 6, (IX + d)
    { &opcode_xycb_f5, 20 }, // LD L, SET 6, (IX + d)
    { &opcode_xycb_f6, 20 }, // SET 6, (IX + d)
    { &opcode_xycb_f7, 20 }, // LD A, SET 6, (IX + d)
    { &opcode_xycb_f8, 20 }, // LD B, SET 7, (IX + d)
    { &opcode_xycb_f9, 20 }, // LD C, SET 7, (IX + d)
    { &opcode_xycb_fa, 20 }, // LD D, SET 7, (IX + d)
    { &opcode_xycb_fb, 20 }, // LD E, SET 7, (IX + d)
    { &opcode_xycb_fc, 20 }, // LD H, SET 7, (IX + d)
    { &opcode_xycb_fd, 20 }, // LD L, SET 7, (IX + d)
    { &opcode_xycb_fe, 20 }, // SET 7, (IX + d)
    { &opcode_xycb_ff, 20 }  // LD A, SET 7, (IX + d)
};

unsigned do_opcode_xycb( unsigned xy )
{
    xy = addDispl( xy, fetchByte() );

    unsigned    op = fetchByte();

    cycles_ += OpInfoXYCB_[ op ].cycles;

    OpInfoXYCB_[ op ].handler( xy );

    return xy;
}

void opcode_xycb_00( unsigned xy ) // LD B, RLC (IX + d)
{
    B = rotateLeftCarry( readByte(xy) );
    writeByte( xy, B );
}

void opcode_xycb_01( unsigned xy ) // LD C, RLC (IX + d)
{
    C = rotateLeftCarry( readByte(xy) );
    writeByte( xy, C );
}

void opcode_xycb_02( unsigned xy ) // LD D, RLC (IX + d)
{
    D = rotateLeftCarry( readByte(xy) );
    writeByte( xy, D );
}

void opcode_xycb_03( unsigned xy ) // LD E, RLC (IX + d)
{
    E = rotateLeftCarry( readByte(xy) );
    writeByte( xy, E );
}

void opcode_xycb_04( unsigned xy ) // LD H, RLC (IX + d)
{
    H = rotateLeftCarry( readByte(xy) );
    writeByte( xy, H );
}

void opcode_xycb_05( unsigned xy ) // LD L, RLC (IX + d)
{
    L = rotateLeftCarry( readByte(xy) );
    writeByte( xy, L );
}

void opcode_xycb_06( unsigned xy ) // RLC (IX + d)
{
    writeByte( xy, rotateLeftCarry( readByte(xy) ) );
}

void opcode_xycb_07( unsigned xy ) // LD A, RLC (IX + d)
{
    A = rotateLeftCarry( readByte(xy) );
    writeByte( xy, A );
}

void opcode_xycb_08( unsigned xy ) // LD B, RRC (IX + d)
{
    B = rotateRightCarry( readByte(xy) );
    writeByte( xy, B );
}

void opcode_xycb_09( unsigned xy ) // LD C, RRC (IX + d)
{
    C = rotateRightCarry( readByte(xy) );
    writeByte( xy, C );
}

void opcode_xycb_0a( unsigned xy ) // LD D, RRC (IX + d)
{
    D = rotateRightCarry( readByte(xy) );
    writeByte( xy, D );
}

void opcode_xycb_0b( unsigned xy ) // LD E, RRC (IX + d)
{
    E = rotateRightCarry( readByte(xy) );
    writeByte( xy, E );
}

void opcode_xycb_0c( unsigned xy ) // LD H, RRC (IX + d)
{
    H = rotateRightCarry( readByte(xy) );
    writeByte( xy, H );
}

void opcode_xycb_0d( unsigned xy ) // LD L, RRC (IX + d)
{
    L = rotateRightCarry( readByte(xy) );
    writeByte( xy, L );
}

void opcode_xycb_0e( unsigned xy ) // RRC (IX + d)
{
    writeByte( xy, rotateRightCarry( readByte(xy) ) );
}

void opcode_xycb_0f( unsigned xy ) // LD A, RRC (IX + d)
{
    A = rotateRightCarry( readByte(xy) );
    writeByte( xy, A );
}

void opcode_xycb_10( unsigned xy ) // LD B, RL (IX + d)
{
    B = rotateLeft( readByte(xy) );
    writeByte( xy, B );
}

void opcode_xycb_11( unsigned xy ) // LD C, RL (IX + d)
{
    C = rotateLeft( readByte(xy) );
    writeByte( xy, C );
}

void opcode_xycb_12( unsigned xy ) // LD D, RL (IX + d)
{
    D = rotateLeft( readByte(xy) );
    writeByte( xy, D );
}

void opcode_xycb_13( unsigned xy ) // LD E, RL (IX + d)
{
    E = rotateLeft( readByte(xy) );
    writeByte( xy, E );
}

void opcode_xycb_14( unsigned xy ) // LD H, RL (IX + d)
{
    H = rotateLeft( readByte(xy) );
    writeByte( xy, H );
}

void opcode_xycb_15( unsigned xy ) // LD L, RL (IX + d)
{
    L = rotateLeft( readByte(xy) );
    writeByte( xy, L );
}

void opcode_xycb_16( unsigned xy ) // RL (IX + d)
{
    writeByte( xy, rotateLeft( readByte(xy) ) );
}

void opcode_xycb_17( unsigned xy ) // LD A, RL (IX + d)
{
    A = rotateLeft( readByte(xy) );
    writeByte( xy, A );
}

void opcode_xycb_18( unsigned xy ) // LD B, RR (IX + d)
{
    B = rotateRight( readByte(xy) );
    writeByte( xy, B );
}

void opcode_xycb_19( unsigned xy ) // LD C, RR (IX + d)
{
    C = rotateRight( readByte(xy) );
    writeByte( xy, C );
}

void opcode_xycb_1a( unsigned xy ) // LD D, RR (IX + d)
{
    D = rotateRight( readByte(xy) );
    writeByte( xy, D );
}

void opcode_xycb_1b( unsigned xy ) // LD E, RR (IX + d)
{
    E = rotateRight( readByte(xy) );
    writeByte( xy, E );
}

void opcode_xycb_1c( unsigned xy ) // LD H, RR (IX + d)
{
    H = rotateRight( readByte(xy) );
    writeByte( xy, H );
}

void opcode_xycb_1d( unsigned xy ) // LD L, RR (IX + d)
{
    L = rotateRight( readByte(xy) );
    writeByte( xy, L );
}

void opcode_xycb_1e( unsigned xy ) // RR (IX + d)
{
    writeByte( xy, rotateRight( readByte(xy) ) );
}

void opcode_xycb_1f( unsigned xy ) // LD A, RR (IX + d)
{
    A = rotateRight( readByte(xy) );
    writeByte( xy, A );
}

void opcode_xycb_20( unsigned xy ) // LD B, SLA (IX + d)
{
    B = shiftLeft( readByte(xy) );
    writeByte( xy, B );
}

void opcode_xycb_21( unsigned xy ) // LD C, SLA (IX + d)
{
    C = shiftLeft( readByte(xy) );
    writeByte( xy, C );
}

void opcode_xycb_22( unsigned xy ) // LD D, SLA (IX + d)
{
    D = shiftLeft( readByte(xy) );
    writeByte( xy, D );
}

void opcode_xycb_23( unsigned xy ) // LD E, SLA (IX + d)
{
    E = shiftLeft( readByte(xy) );
    writeByte( xy, E );
}

void opcode_xycb_24( unsigned xy ) // LD H, SLA (IX + d)
{
    H = shiftLeft( readByte(xy) );
    writeByte( xy, H );
}

void opcode_xycb_25( unsigned xy ) // LD L, SLA (IX + d)
{
    L = shiftLeft( readByte(xy) );
    writeByte( xy, L );
}

void opcode_xycb_26( unsigned xy ) // SLA (IX + d)
{
    writeByte( xy, shiftLeft( readByte(xy) ) );
}

void opcode_xycb_27( unsigned xy ) // LD A, SLA (IX + d)
{
    A = shiftLeft( readByte(xy) );
    writeByte( xy, A );
}

void opcode_xycb_28( unsigned xy ) // LD B, SRA (IX + d)
{
    B = shiftRightArith( readByte(xy) );
    writeByte( xy, B );
}

void opcode_xycb_29( unsigned xy ) // LD C, SRA (IX + d)
{
    C = shiftRightArith( readByte(xy) );
    writeByte( xy, C );
}

void opcode_xycb_2a( unsigned xy ) // LD D, SRA (IX + d)
{
    D = shiftRightArith( readByte(xy) );
    writeByte( xy, D );
}

void opcode_xycb_2b( unsigned xy ) // LD E, SRA (IX + d)
{
    E = shiftRightArith( readByte(xy) );
    writeByte( xy, E );
}

void opcode_xycb_2c( unsigned xy ) // LD H, SRA (IX + d)
{
    H = shiftRightArith( readByte(xy) );
    writeByte( xy, H );
}

void opcode_xycb_2d( unsigned xy ) // LD L, SRA (IX + d)
{
    L = shiftRightArith( readByte(xy) );
    writeByte( xy, L );
}

void opcode_xycb_2e( unsigned xy ) // SRA (IX + d)
{
    writeByte( xy, shiftRightArith( readByte(xy) ) );
}

void opcode_xycb_2f( unsigned xy ) // LD A, SRA (IX + d)
{
    A = shiftRightArith( readByte(xy) );
    writeByte( xy, A );
}

void opcode_xycb_30( unsigned xy ) // LD B, SLL (IX + d)
{
    B = shiftLeft( readByte(xy) ) | 0x01;
    writeByte( xy, B );
}

void opcode_xycb_31( unsigned xy ) // LD C, SLL (IX + d)
{
    C = shiftLeft( readByte(xy) ) | 0x01;
    writeByte( xy, C );
}

void opcode_xycb_32( unsigned xy ) // LD D, SLL (IX + d)
{
    D = shiftLeft( readByte(xy) ) | 0x01;
    writeByte( xy, D );
}

void opcode_xycb_33( unsigned xy ) // LD E, SLL (IX + d)
{
    E = shiftLeft( readByte(xy) ) | 0x01;
    writeByte( xy, E );
}

void opcode_xycb_34( unsigned xy ) // LD H, SLL (IX + d)
{
    H = shiftLeft( readByte(xy) ) | 0x01;
    writeByte( xy, H );
}

void opcode_xycb_35( unsigned xy ) // LD L, SLL (IX + d)
{
    L = shiftLeft( readByte(xy) ) | 0x01;
    writeByte( xy, L );
}

void opcode_xycb_36( unsigned xy ) // SLL (IX + d)
{
    writeByte( xy, shiftLeft( readByte(xy) ) | 0x01 );
}

void opcode_xycb_37( unsigned xy ) // LD A, SLL (IX + d)
{
    A = shiftLeft( readByte(xy) ) | 0x01;
    writeByte( xy, A );
}

void opcode_xycb_38( unsigned xy ) // LD B, SRL (IX + d)
{
    B = shiftRightLogical( readByte(xy) );
    writeByte( xy, B );
}

void opcode_xycb_39( unsigned xy ) // LD C, SRL (IX + d)
{
    C = shiftRightLogical( readByte(xy) );
    writeByte( xy, C );
}

void opcode_xycb_3a( unsigned xy ) // LD D, SRL (IX + d)
{
    D = shiftRightLogical( readByte(xy) );
    writeByte( xy, D );
}

void opcode_xycb_3b( unsigned xy ) // LD E, SRL (IX + d)
{
    E = shiftRightLogical( readByte(xy) );
    writeByte( xy, E );
}

void opcode_xycb_3c( unsigned xy ) // LD H, SRL (IX + d)
{
    H = shiftRightLogical( readByte(xy) );
    writeByte( xy, H );
}

void opcode_xycb_3d( unsigned xy ) // LD L, SRL (IX + d)
{
    L = shiftRightLogical( readByte(xy) );
    writeByte( xy, L );
}

void opcode_xycb_3e( unsigned xy ) // SRL (IX + d)
{
    writeByte( xy, shiftRightLogical( readByte(xy) ) );
}

void opcode_xycb_3f( unsigned xy ) // LD A, SRL (IX + d)
{
    A = shiftRightLogical( readByte(xy) );
    writeByte( xy, A );
}

void opcode_xycb_40( unsigned xy ) // BIT 0, (IX + d)
{
    testBit( 0, readByte( xy ) );
}

void opcode_xycb_41( unsigned xy ) // BIT 0, (IX + d)
{
    testBit( 0, readByte( xy ) );
}

void opcode_xycb_42( unsigned xy ) // BIT 0, (IX + d)
{
    testBit( 0, readByte( xy ) );
}

void opcode_xycb_43( unsigned xy ) // BIT 0, (IX + d)
{
    testBit( 0, readByte( xy ) );
}

void opcode_xycb_44( unsigned xy ) // BIT 0, (IX + d)
{
    testBit( 0, readByte( xy ) );
}

void opcode_xycb_45( unsigned xy ) // BIT 0, (IX + d)
{
    testBit( 0, readByte( xy ) );
}

void opcode_xycb_46( unsigned xy ) // BIT 0, (IX + d)
{
    testBit( 0, readByte( xy ) );
}

void opcode_xycb_47( unsigned xy ) // BIT 0, (IX + d)
{
    testBit( 0, readByte( xy ) );
}

void opcode_xycb_48( unsigned xy ) // BIT 1, (IX + d)
{
    testBit( 1, readByte( xy ) );
}

void opcode_xycb_49( unsigned xy ) // BIT 1, (IX + d)
{
    testBit( 1, readByte( xy ) );
}

void opcode_xycb_4a( unsigned xy ) // BIT 1, (IX + d)
{
    testBit( 1, readByte( xy ) );
}

void opcode_xycb_4b( unsigned xy ) // BIT 1, (IX + d)
{
    testBit( 1, readByte( xy ) );
}

void opcode_xycb_4c( unsigned xy ) // BIT 1, (IX + d)
{
    testBit( 1, readByte( xy ) );
}

void opcode_xycb_4d( unsigned xy ) // BIT 1, (IX + d)
{
    testBit( 1, readByte( xy ) );
}

void opcode_xycb_4e( unsigned xy ) // BIT 1, (IX + d)
{
    testBit( 1, readByte( xy ) );
}

void opcode_xycb_4f( unsigned xy ) // BIT 1, (IX + d)
{
    testBit( 1, readByte( xy ) );
}

void opcode_xycb_50( unsigned xy ) // BIT 2, (IX + d)
{
    testBit( 2, readByte( xy ) );
}

void opcode_xycb_51( unsigned xy ) // BIT 2, (IX + d)
{
    testBit( 2, readByte( xy ) );
}

void opcode_xycb_52( unsigned xy ) // BIT 2, (IX + d)
{
    testBit( 2, readByte( xy ) );
}

void opcode_xycb_53( unsigned xy ) // BIT 2, (IX + d)
{
    testBit( 2, readByte( xy ) );
}

void opcode_xycb_54( unsigned xy ) // BIT 2, (IX + d)
{
    testBit( 2, readByte( xy ) );
}

void opcode_xycb_55( unsigned xy ) // BIT 2, (IX + d)
{
    testBit( 2, readByte( xy ) );
}

void opcode_xycb_56( unsigned xy ) // BIT 2, (IX + d)
{
    testBit( 2, readByte( xy ) );
}

void opcode_xycb_57( unsigned xy ) // BIT 2, (IX + d)
{
    testBit( 2, readByte( xy ) );
}

void opcode_xycb_58( unsigned xy ) // BIT 3, (IX + d)
{
    testBit( 3, readByte( xy ) );
}

void opcode_xycb_59( unsigned xy ) // BIT 3, (IX + d)
{
    testBit( 3, readByte( xy ) );
}

void opcode_xycb_5a( unsigned xy ) // BIT 3, (IX + d)
{
    testBit( 3, readByte( xy ) );
}

void opcode_xycb_5b( unsigned xy ) // BIT 3, (IX + d)
{
    testBit( 3, readByte( xy ) );
}

void opcode_xycb_5c( unsigned xy ) // BIT 3, (IX + d)
{
    testBit( 3, readByte( xy ) );
}

void opcode_xycb_5d( unsigned xy ) // BIT 3, (IX + d)
{
    testBit( 3, readByte( xy ) );
}

void opcode_xycb_5e( unsigned xy ) // BIT 3, (IX + d)
{
    testBit( 3, readByte( xy ) );
}

void opcode_xycb_5f( unsigned xy ) // BIT 3, (IX + d)
{
    testBit( 3, readByte( xy ) );
}

void opcode_xycb_60( unsigned xy ) // BIT 4, (IX + d)
{
    testBit( 4, readByte( xy ) );
}

void opcode_xycb_61( unsigned xy ) // BIT 4, (IX + d)
{
    testBit( 4, readByte( xy ) );
}

void opcode_xycb_62( unsigned xy ) // BIT 4, (IX + d)
{
    testBit( 4, readByte( xy ) );
}

void opcode_xycb_63( unsigned xy ) // BIT 4, (IX + d)
{
    testBit( 4, readByte( xy ) );
}

void opcode_xycb_64( unsigned xy ) // BIT 4, (IX + d)
{
    testBit( 4, readByte( xy ) );
}

void opcode_xycb_65( unsigned xy ) // BIT 4, (IX + d)
{
    testBit( 4, readByte( xy ) );
}

void opcode_xycb_66( unsigned xy ) // BIT 4, (IX + d)
{
    testBit( 4, readByte( xy ) );
}

void opcode_xycb_67( unsigned xy ) // BIT 4, (IX + d)
{
    testBit( 4, readByte( xy ) );
}

void opcode_xycb_68( unsigned xy ) // BIT 5, (IX + d)
{
    testBit( 5, readByte( xy ) );
}

void opcode_xycb_69( unsigned xy ) // BIT 5, (IX + d)
{
    testBit( 5, readByte( xy ) );
}

void opcode_xycb_6a( unsigned xy ) // BIT 5, (IX + d)
{
    testBit( 5, readByte( xy ) );
}

void opcode_xycb_6b( unsigned xy ) // BIT 5, (IX + d)
{
    testBit( 5, readByte( xy ) );
}

void opcode_xycb_6c( unsigned xy ) // BIT 5, (IX + d)
{
    testBit( 5, readByte( xy ) );
}

void opcode_xycb_6d( unsigned xy ) // BIT 5, (IX + d)
{
    testBit( 5, readByte( xy ) );
}

void opcode_xycb_6e( unsigned xy ) // BIT 5, (IX + d)
{
    testBit( 5, readByte( xy ) );
}

void opcode_xycb_6f( unsigned xy ) // BIT 5, (IX + d)
{
    testBit( 5, readByte( xy ) );
}

void opcode_xycb_70( unsigned xy ) // BIT 6, (IX + d)
{
    testBit( 6, readByte( xy ) );
}

void opcode_xycb_71( unsigned xy ) // BIT 6, (IX + d)
{
    testBit( 6, readByte( xy ) );
}

void opcode_xycb_72( unsigned xy ) // BIT 6, (IX + d)
{
    testBit( 6, readByte( xy ) );
}

void opcode_xycb_73( unsigned xy ) // BIT 6, (IX + d)
{
    testBit( 6, readByte( xy ) );
}

void opcode_xycb_74( unsigned xy ) // BIT 6, (IX + d)
{
    testBit( 6, readByte( xy ) );
}

void opcode_xycb_75( unsigned xy ) // BIT 6, (IX + d)
{
    testBit( 6, readByte( xy ) );
}

void opcode_xycb_76( unsigned xy ) // BIT 6, (IX + d)
{
    testBit( 6, readByte( xy ) );
}

void opcode_xycb_77( unsigned xy ) // BIT 6, (IX + d)
{
    testBit( 6, readByte( xy ) );
}

void opcode_xycb_78( unsigned xy ) // BIT 7, (IX + d)
{
    testBit( 7, readByte( xy ) );
}

void opcode_xycb_79( unsigned xy ) // BIT 7, (IX + d)
{
    testBit( 7, readByte( xy ) );
}

void opcode_xycb_7a( unsigned xy ) // BIT 7, (IX + d)
{
    testBit( 7, readByte( xy ) );
}

void opcode_xycb_7b( unsigned xy ) // BIT 7, (IX + d)
{
    testBit( 7, readByte( xy ) );
}

void opcode_xycb_7c( unsigned xy ) // BIT 7, (IX + d)
{
    testBit( 7, readByte( xy ) );
}

void opcode_xycb_7d( unsigned xy ) // BIT 7, (IX + d)
{
    testBit( 7, readByte( xy ) );
}

void opcode_xycb_7e( unsigned xy ) // BIT 7, (IX + d)
{
    testBit( 7, readByte( xy ) );
}

void opcode_xycb_7f( unsigned xy ) // BIT 7, (IX + d)
{
    testBit( 7, readByte( xy ) );
}

void opcode_xycb_80( unsigned xy ) // LD B, RES 0, (IX + d)
{
    B = readByte(xy) & (unsigned char) ~(1 << 0);
    writeByte( xy, B );
}

void opcode_xycb_81( unsigned xy ) // LD C, RES 0, (IX + d)
{
    C = readByte(xy) & (unsigned char) ~(1 << 0);
    writeByte( xy, C );
}

void opcode_xycb_82( unsigned xy ) // LD D, RES 0, (IX + d)
{
    D = readByte(xy) & (unsigned char) ~(1 << 0);
    writeByte( xy, D );
}

void opcode_xycb_83( unsigned xy ) // LD E, RES 0, (IX + d)
{
    E = readByte(xy) & (unsigned char) ~(1 << 0);
    writeByte( xy, E );
}

void opcode_xycb_84( unsigned xy ) // LD H, RES 0, (IX + d)
{
    H = readByte(xy) & (unsigned char) ~(1 << 0);
    writeByte( xy, H );
}

void opcode_xycb_85( unsigned xy ) // LD L, RES 0, (IX + d)
{
    L = readByte(xy) & (unsigned char) ~(1 << 0);
    writeByte( xy, L );
}

void opcode_xycb_86( unsigned xy ) // RES 0, (IX + d)
{
    writeByte( xy, readByte(xy) & (unsigned char) ~(1 << 0) );
}

void opcode_xycb_87( unsigned xy ) // LD A, RES 0, (IX + d)
{
    A = readByte(xy) & (unsigned char) ~(1 << 0);
    writeByte( xy, A );
}

void opcode_xycb_88( unsigned xy ) // LD B, RES 1, (IX + d)
{
    B = readByte(xy) & (unsigned char) ~(1 << 1);
    writeByte( xy, B );
}

void opcode_xycb_89( unsigned xy ) // LD C, RES 1, (IX + d)
{
    C = readByte(xy) & (unsigned char) ~(1 << 1);
    writeByte( xy, C );
}

void opcode_xycb_8a( unsigned xy ) // LD D, RES 1, (IX + d)
{
    D = readByte(xy) & (unsigned char) ~(1 << 1);
    writeByte( xy, D );
}

void opcode_xycb_8b( unsigned xy ) // LD E, RES 1, (IX + d)
{
    E = readByte(xy) & (unsigned char) ~(1 << 1);
    writeByte( xy, E );
}

void opcode_xycb_8c( unsigned xy ) // LD H, RES 1, (IX + d)
{
    H = readByte(xy) & (unsigned char) ~(1 << 1);
    writeByte( xy, H );
}

void opcode_xycb_8d( unsigned xy ) // LD L, RES 1, (IX + d)
{
    L = readByte(xy) & (unsigned char) ~(1 << 1);
    writeByte( xy, L );
}

void opcode_xycb_8e( unsigned xy ) // RES 1, (IX + d)
{
    writeByte( xy, readByte(xy) & (unsigned char) ~(1 << 1) );
}

void opcode_xycb_8f( unsigned xy ) // LD A, RES 1, (IX + d)
{
    A = readByte(xy) & (unsigned char) ~(1 << 1);
    writeByte( xy, A );
}

void opcode_xycb_90( unsigned xy ) // LD B, RES 2, (IX + d)
{
    B = readByte(xy) & (unsigned char) ~(1 << 2);
    writeByte( xy, B );
}

void opcode_xycb_91( unsigned xy ) // LD C, RES 2, (IX + d)
{
    C = readByte(xy) & (unsigned char) ~(1 << 2);
    writeByte( xy, C );
}

void opcode_xycb_92( unsigned xy ) // LD D, RES 2, (IX + d)
{
    D = readByte(xy) & (unsigned char) ~(1 << 2);
    writeByte( xy, D );
}

void opcode_xycb_93( unsigned xy ) // LD E, RES 2, (IX + d)
{
    E = readByte(xy) & (unsigned char) ~(1 << 2);
    writeByte( xy, E );
}

void opcode_xycb_94( unsigned xy ) // LD H, RES 2, (IX + d)
{
    H = readByte(xy) & (unsigned char) ~(1 << 2);
    writeByte( xy, H );
}

void opcode_xycb_95( unsigned xy ) // LD L, RES 2, (IX + d)
{
    L = readByte(xy) & (unsigned char) ~(1 << 2);
    writeByte( xy, L );
}

void opcode_xycb_96( unsigned xy ) // RES 2, (IX + d)
{
    writeByte( xy, readByte(xy) & (unsigned char) ~(1 << 2) );
}

void opcode_xycb_97( unsigned xy ) // LD A, RES 2, (IX + d)
{
    A = readByte(xy) & (unsigned char) ~(1 << 2);
    writeByte( xy, A );
}

void opcode_xycb_98( unsigned xy ) // LD B, RES 3, (IX + d)
{
    B = readByte(xy) & (unsigned char) ~(1 << 3);
    writeByte( xy, B );
}

void opcode_xycb_99( unsigned xy ) // LD C, RES 3, (IX + d)
{
    C = readByte(xy) & (unsigned char) ~(1 << 3);
    writeByte( xy, C );
}

void opcode_xycb_9a( unsigned xy ) // LD D, RES 3, (IX + d)
{
    D = readByte(xy) & (unsigned char) ~(1 << 3);
    writeByte( xy, D );
}

void opcode_xycb_9b( unsigned xy ) // LD E, RES 3, (IX + d)
{
    E = readByte(xy) & (unsigned char) ~(1 << 3);
    writeByte( xy, E );
}

void opcode_xycb_9c( unsigned xy ) // LD H, RES 3, (IX + d)
{
    H = readByte(xy) & (unsigned char) ~(1 << 3);
    writeByte( xy, H );
}

void opcode_xycb_9d( unsigned xy ) // LD L, RES 3, (IX + d)
{
    L = readByte(xy) & (unsigned char) ~(1 << 3);
    writeByte( xy, L );
}

void opcode_xycb_9e( unsigned xy ) // RES 3, (IX + d)
{
    writeByte( xy, readByte(xy) & (unsigned char) ~(1 << 3) );
}

void opcode_xycb_9f( unsigned xy ) // LD A, RES 3, (IX + d)
{
    A = readByte(xy) & (unsigned char) ~(1 << 3);
    writeByte( xy, A );
}

void opcode_xycb_a0( unsigned xy ) // LD B, RES 4, (IX + d)
{
    B = readByte(xy) & (unsigned char) ~(1 << 4);
    writeByte( xy, B );
}

void opcode_xycb_a1( unsigned xy ) // LD C, RES 4, (IX + d)
{
    C = readByte(xy) & (unsigned char) ~(1 << 4);
    writeByte( xy, C );
}

void opcode_xycb_a2( unsigned xy ) // LD D, RES 4, (IX + d)
{
    D = readByte(xy) & (unsigned char) ~(1 << 4);
    writeByte( xy, D );
}

void opcode_xycb_a3( unsigned xy ) // LD E, RES 4, (IX + d)
{
    E = readByte(xy) & (unsigned char) ~(1 << 4);
    writeByte( xy, E );
}

void opcode_xycb_a4( unsigned xy ) // LD H, RES 4, (IX + d)
{
    H = readByte(xy) & (unsigned char) ~(1 << 4);
    writeByte( xy, H );
}

void opcode_xycb_a5( unsigned xy ) // LD L, RES 4, (IX + d)
{
    L = readByte(xy) & (unsigned char) ~(1 << 4);
    writeByte( xy, L );
}

void opcode_xycb_a6( unsigned xy ) // RES 4, (IX + d)
{
    writeByte( xy, readByte(xy) & (unsigned char) ~(1 << 4) );
}

void opcode_xycb_a7( unsigned xy ) // LD A, RES 4, (IX + d)
{
    A = readByte(xy) & (unsigned char) ~(1 << 4);
    writeByte( xy, A );
}

void opcode_xycb_a8( unsigned xy ) // LD B, RES 5, (IX + d)
{
    B = readByte(xy) & (unsigned char) ~(1 << 5);
    writeByte( xy, B );
}

void opcode_xycb_a9( unsigned xy ) // LD C, RES 5, (IX + d)
{
    C = readByte(xy) & (unsigned char) ~(1 << 5);
    writeByte( xy, C );
}

void opcode_xycb_aa( unsigned xy ) // LD D, RES 5, (IX + d)
{
    D = readByte(xy) & (unsigned char) ~(1 << 5);
    writeByte( xy, D );
}

void opcode_xycb_ab( unsigned xy ) // LD E, RES 5, (IX + d)
{
    E = readByte(xy) & (unsigned char) ~(1 << 5);
    writeByte( xy, E );
}

void opcode_xycb_ac( unsigned xy ) // LD H, RES 5, (IX + d)
{
    H = readByte(xy) & (unsigned char) ~(1 << 5);
    writeByte( xy, H );
}

void opcode_xycb_ad( unsigned xy ) // LD L, RES 5, (IX + d)
{
    L = readByte(xy) & (unsigned char) ~(1 << 5);
    writeByte( xy, L );
}

void opcode_xycb_ae( unsigned xy ) // RES 5, (IX + d)
{
    writeByte( xy, readByte(xy) & (unsigned char) ~(1 << 5) );
}

void opcode_xycb_af( unsigned xy ) // LD A, RES 5, (IX + d)
{
    A = readByte(xy) & (unsigned char) ~(1 << 5);
    writeByte( xy, A );
}

void opcode_xycb_b0( unsigned xy ) // LD B, RES 6, (IX + d)
{
    B = readByte(xy) & (unsigned char) ~(1 << 6);
    writeByte( xy, B );
}

void opcode_xycb_b1( unsigned xy ) // LD C, RES 6, (IX + d)
{
    C = readByte(xy) & (unsigned char) ~(1 << 6);
    writeByte( xy, C );
}

void opcode_xycb_b2( unsigned xy ) // LD D, RES 6, (IX + d)
{
    D = readByte(xy) & (unsigned char) ~(1 << 6);
    writeByte( xy, D );
}

void opcode_xycb_b3( unsigned xy ) // LD E, RES 6, (IX + d)
{
    E = readByte(xy) & (unsigned char) ~(1 << 6);
    writeByte( xy, E );
}

void opcode_xycb_b4( unsigned xy ) // LD H, RES 6, (IX + d)
{
    H = readByte(xy) & (unsigned char) ~(1 << 6);
    writeByte( xy, H );
}

void opcode_xycb_b5( unsigned xy ) // LD L, RES 6, (IX + d)
{
    L = readByte(xy) & (unsigned char) ~(1 << 6);
    writeByte( xy, L );
}

void opcode_xycb_b6( unsigned xy ) // RES 6, (IX + d)
{
    writeByte( xy, readByte(xy) & (unsigned char) ~(1 << 6) );
}

void opcode_xycb_b7( unsigned xy ) // LD A, RES 6, (IX + d)
{
    A = readByte(xy) & (unsigned char) ~(1 << 6);
    writeByte( xy, A );
}

void opcode_xycb_b8( unsigned xy ) // LD B, RES 7, (IX + d)
{
    B = readByte(xy) & (unsigned char) ~(1 << 7);
    writeByte( xy, B );
}

void opcode_xycb_b9( unsigned xy ) // LD C, RES 7, (IX + d)
{
    C = readByte(xy) & (unsigned char) ~(1 << 7);
    writeByte( xy, C );
}

void opcode_xycb_ba( unsigned xy ) // LD D, RES 7, (IX + d)
{
    D = readByte(xy) & (unsigned char) ~(1 << 7);
    writeByte( xy, D );
}

void opcode_xycb_bb( unsigned xy ) // LD E, RES 7, (IX + d)
{
    E = readByte(xy) & (unsigned char) ~(1 << 7);
    writeByte( xy, E );
}

void opcode_xycb_bc( unsigned xy ) // LD H, RES 7, (IX + d)
{
    H = readByte(xy) & (unsigned char) ~(1 << 7);
    writeByte( xy, H );
}

void opcode_xycb_bd( unsigned xy ) // LD L, RES 7, (IX + d)
{
    L = readByte(xy) & (unsigned char) ~(1 << 7);
    writeByte( xy, L );
}

void opcode_xycb_be( unsigned xy ) // RES 7, (IX + d)
{
    writeByte( xy, readByte(xy) & (unsigned char) ~(1 << 7) );
}

void opcode_xycb_bf( unsigned xy ) // LD A, RES 7, (IX + d)
{
    A = readByte(xy) & (unsigned char) ~(1 << 7);
    writeByte( xy, A );
}

void opcode_xycb_c0( unsigned xy ) // LD B, SET 0, (IX + d)
{
    B = readByte(xy) | (unsigned char) (1 << 0);
    writeByte( xy, B );
}

void opcode_xycb_c1( unsigned xy ) // LD C, SET 0, (IX + d)
{
    C = readByte(xy) | (unsigned char) (1 << 0);
    writeByte( xy, C );
}

void opcode_xycb_c2( unsigned xy ) // LD D, SET 0, (IX + d)
{
    D = readByte(xy) | (unsigned char) (1 << 0);
    writeByte( xy, D );
}

void opcode_xycb_c3( unsigned xy ) // LD E, SET 0, (IX + d)
{
    E = readByte(xy) | (unsigned char) (1 << 0);
    writeByte( xy, E );
}

void opcode_xycb_c4( unsigned xy ) // LD H, SET 0, (IX + d)
{
    H = readByte(xy) | (unsigned char) (1 << 0);
    writeByte( xy, H );
}

void opcode_xycb_c5( unsigned xy ) // LD L, SET 0, (IX + d)
{
    L = readByte(xy) | (unsigned char) (1 << 0);
    writeByte( xy, L );
}

void opcode_xycb_c6( unsigned xy ) // SET 0, (IX + d)
{
    writeByte( xy, readByte(xy) | (unsigned char) (1 << 0) );
}

void opcode_xycb_c7( unsigned xy ) // LD A, SET 0, (IX + d)
{
    A = readByte(xy) | (unsigned char) (1 << 0);
    writeByte( xy, A );
}

void opcode_xycb_c8( unsigned xy ) // LD B, SET 1, (IX + d)
{
    B = readByte(xy) | (unsigned char) (1 << 1);
    writeByte( xy, B );
}

void opcode_xycb_c9( unsigned xy ) // LD C, SET 1, (IX + d)
{
    C = readByte(xy) | (unsigned char) (1 << 1);
    writeByte( xy, C );
}

void opcode_xycb_ca( unsigned xy ) // LD D, SET 1, (IX + d)
{
    D = readByte(xy) | (unsigned char) (1 << 1);
    writeByte( xy, D );
}

void opcode_xycb_cb( unsigned xy ) // LD E, SET 1, (IX + d)
{
    E = readByte(xy) | (unsigned char) (1 << 1);
    writeByte( xy, E );
}

void opcode_xycb_cc( unsigned xy ) // LD H, SET 1, (IX + d)
{
    H = readByte(xy) | (unsigned char) (1 << 1);
    writeByte( xy, H );
}

void opcode_xycb_cd( unsigned xy ) // LD L, SET 1, (IX + d)
{
    L = readByte(xy) | (unsigned char) (1 << 1);
    writeByte( xy, L );
}

void opcode_xycb_ce( unsigned xy ) // SET 1, (IX + d)
{
    writeByte( xy, readByte(xy) | (unsigned char) (1 << 1) );
}

void opcode_xycb_cf( unsigned xy ) // LD A, SET 1, (IX + d)
{
    A = readByte(xy) | (unsigned char) (1 << 1);
    writeByte( xy, A );
}

void opcode_xycb_d0( unsigned xy ) // LD B, SET 2, (IX + d)
{
    B = readByte(xy) | (unsigned char) (1 << 2);
    writeByte( xy, B );
}

void opcode_xycb_d1( unsigned xy ) // LD C, SET 2, (IX + d)
{
    C = readByte(xy) | (unsigned char) (1 << 2);
    writeByte( xy, C );
}

void opcode_xycb_d2( unsigned xy ) // LD D, SET 2, (IX + d)
{
    D = readByte(xy) | (unsigned char) (1 << 2);
    writeByte( xy, D );
}

void opcode_xycb_d3( unsigned xy ) // LD E, SET 2, (IX + d)
{
    E = readByte(xy) | (unsigned char) (1 << 2);
    writeByte( xy, E );
}

void opcode_xycb_d4( unsigned xy ) // LD H, SET 2, (IX + d)
{
    H = readByte(xy) | (unsigned char) (1 << 2);
    writeByte( xy, H );
}

void opcode_xycb_d5( unsigned xy ) // LD L, SET 2, (IX + d)
{
    L = readByte(xy) | (unsigned char) (1 << 2);
    writeByte( xy, L );
}

void opcode_xycb_d6( unsigned xy ) // SET 2, (IX + d)
{
    writeByte( xy, readByte(xy) | (unsigned char) (1 << 2) );
}

void opcode_xycb_d7( unsigned xy ) // LD A, SET 2, (IX + d)
{
    A = readByte(xy) | (unsigned char) (1 << 2);
    writeByte( xy, A );
}

void opcode_xycb_d8( unsigned xy ) // LD B, SET 3, (IX + d)
{
    B = readByte(xy) | (unsigned char) (1 << 3);
    writeByte( xy, B );
}

void opcode_xycb_d9( unsigned xy ) // LD C, SET 3, (IX + d)
{
    C = readByte(xy) | (unsigned char) (1 << 3);
    writeByte( xy, C );
}

void opcode_xycb_da( unsigned xy ) // LD D, SET 3, (IX + d)
{
    D = readByte(xy) | (unsigned char) (1 << 3);
    writeByte( xy, D );
}

void opcode_xycb_db( unsigned xy ) // LD E, SET 3, (IX + d)
{
    E = readByte(xy) | (unsigned char) (1 << 3);
    writeByte( xy, E );
}

void opcode_xycb_dc( unsigned xy ) // LD H, SET 3, (IX + d)
{
    H = readByte(xy) | (unsigned char) (1 << 3);
    writeByte( xy, H );
}

void opcode_xycb_dd( unsigned xy ) // LD L, SET 3, (IX + d)
{
    L = readByte(xy) | (unsigned char) (1 << 3);
    writeByte( xy, L );
}

void opcode_xycb_de( unsigned xy ) // SET 3, (IX + d)
{
    writeByte( xy, readByte(xy) | (unsigned char) (1 << 3) );
}

void opcode_xycb_df( unsigned xy ) // LD A, SET 3, (IX + d)
{
    A = readByte(xy) | (unsigned char) (1 << 3);
    writeByte( xy, A );
}

void opcode_xycb_e0( unsigned xy ) // LD B, SET 4, (IX + d)
{
    B = readByte(xy) | (unsigned char) (1 << 4);
    writeByte( xy, B );
}

void opcode_xycb_e1( unsigned xy ) // LD C, SET 4, (IX + d)
{
    C = readByte(xy) | (unsigned char) (1 << 4);
    writeByte( xy, C );
}

void opcode_xycb_e2( unsigned xy ) // LD D, SET 4, (IX + d)
{
    D = readByte(xy) | (unsigned char) (1 << 4);
    writeByte( xy, D );
}

void opcode_xycb_e3( unsigned xy ) // LD E, SET 4, (IX + d)
{
    E = readByte(xy) | (unsigned char) (1 << 4);
    writeByte( xy, E );
}

void opcode_xycb_e4( unsigned xy ) // LD H, SET 4, (IX + d)
{
    H = readByte(xy) | (unsigned char) (1 << 4);
    writeByte( xy, H );
}

void opcode_xycb_e5( unsigned xy ) // LD L, SET 4, (IX + d)
{
    L = readByte(xy) | (unsigned char) (1 << 4);
    writeByte( xy, L );
}

void opcode_xycb_e6( unsigned xy ) // SET 4, (IX + d)
{
    writeByte( xy, readByte(xy) | (unsigned char) (1 << 4) );
}

void opcode_xycb_e7( unsigned xy ) // LD A, SET 4, (IX + d)
{
    A = readByte(xy) | (unsigned char) (1 << 4);
    writeByte( xy, A );
}

void opcode_xycb_e8( unsigned xy ) // LD B, SET 5, (IX + d)
{
    B = readByte(xy) | (unsigned char) (1 << 5);
    writeByte( xy, B );
}

void opcode_xycb_e9( unsigned xy ) // LD C, SET 5, (IX + d)
{
    C = readByte(xy) | (unsigned char) (1 << 5);
    writeByte( xy, C );
}

void opcode_xycb_ea( unsigned xy ) // LD D, SET 5, (IX + d)
{
    D = readByte(xy) | (unsigned char) (1 << 5);
    writeByte( xy, D );
}

void opcode_xycb_eb( unsigned xy ) // LD E, SET 5, (IX + d)
{
    E = readByte(xy) | (unsigned char) (1 << 5);
    writeByte( xy, E );
}

void opcode_xycb_ec( unsigned xy ) // LD H, SET 5, (IX + d)
{
    H = readByte(xy) | (unsigned char) (1 << 5);
    writeByte( xy, H );
}

void opcode_xycb_ed( unsigned xy ) // LD L, SET 5, (IX + d)
{
    L = readByte(xy) | (unsigned char) (1 << 5);
    writeByte( xy, L );
}

void opcode_xycb_ee( unsigned xy ) // SET 5, (IX + d)
{
    writeByte( xy, readByte(xy) | (unsigned char) (1 << 5) );
}

void opcode_xycb_ef( unsigned xy ) // LD A, SET 5, (IX + d)
{
    A = readByte(xy) | (unsigned char) (1 << 5);
    writeByte( xy, A );
}

void opcode_xycb_f0( unsigned xy ) // LD B, SET 6, (IX + d)
{
    B = readByte(xy) | (unsigned char) (1 << 6);
    writeByte( xy, B );
}

void opcode_xycb_f1( unsigned xy ) // LD C, SET 6, (IX + d)
{
    C = readByte(xy) | (unsigned char) (1 << 6);
    writeByte( xy, C );
}

void opcode_xycb_f2( unsigned xy ) // LD D, SET 6, (IX + d)
{
    D = readByte(xy) | (unsigned char) (1 << 6);
    writeByte( xy, D );
}

void opcode_xycb_f3( unsigned xy ) // LD E, SET 6, (IX + d)
{
    E = readByte(xy) | (unsigned char) (1 << 6);
    writeByte( xy, E );
}

void opcode_xycb_f4( unsigned xy ) // LD H, SET 6, (IX + d)
{
    H = readByte(xy) | (unsigned char) (1 << 6);
    writeByte( xy, H );
}

void opcode_xycb_f5( unsigned xy ) // LD L, SET 6, (IX + d)
{
    L = readByte(xy) | (unsigned char) (1 << 6);
    writeByte( xy, L );
}

void opcode_xycb_f6( unsigned xy ) // SET 6, (IX + d)
{
    writeByte( xy, readByte(xy) | (unsigned char) (1 << 6) );
}

void opcode_xycb_f7( unsigned xy ) // LD A, SET 6, (IX + d)
{
    A = readByte(xy) | (unsigned char) (1 << 6);
    writeByte( xy, A );
}

void opcode_xycb_f8( unsigned xy ) // LD B, SET 7, (IX + d)
{
    B = readByte(xy) | (unsigned char) (1 << 7);
    writeByte( xy, B );
}

void opcode_xycb_f9( unsigned xy ) // LD C, SET 7, (IX + d)
{
    C = readByte(xy) | (unsigned char) (1 << 7);
    writeByte( xy, C );
}

void opcode_xycb_fa( unsigned xy ) // LD D, SET 7, (IX + d)
{
    D = readByte(xy) | (unsigned char) (1 << 7);
    writeByte( xy, D );
}

void opcode_xycb_fb( unsigned xy ) // LD E, SET 7, (IX + d)
{
    E = readByte(xy) | (unsigned char) (1 << 7);
    writeByte( xy, E );
}

void opcode_xycb_fc( unsigned xy ) // LD H, SET 7, (IX + d)
{
    H = readByte(xy) | (unsigned char) (1 << 7);
    writeByte( xy, H );
}

void opcode_xycb_fd( unsigned xy ) // LD L, SET 7, (IX + d)
{
    L = readByte(xy) | (unsigned char) (1 << 7);
    writeByte( xy, L );
}

void opcode_xycb_fe( unsigned xy ) // SET 7, (IX + d)
{
    writeByte( xy, readByte(xy) | (unsigned char) (1 << 7) );
}

void opcode_xycb_ff( unsigned xy ) // LD A, SET 7, (IX + d)
{
    A = readByte(xy) | (unsigned char) (1 << 7);
    writeByte( xy, A );
}

OpcodeInfo OpInfo_[256] = {
    { &opcode_00,  4 }, // NOP
    { &opcode_01, 10 }, // LD   BC,nn
    { &opcode_02,  7 }, // LD   (BC),A
    { &opcode_03,  6 }, // INC  BC
    { &opcode_04,  4 }, // INC  B
    { &opcode_05,  4 }, // DEC  B
    { &opcode_06,  7 }, // LD   B,n
    { &opcode_07,  4 }, // RLCA
    { &opcode_08,  4 }, // EX   AF,AF'
    { &opcode_09, 11 }, // ADD  HL,BC
    { &opcode_0a,  7 }, // LD   A,(BC)
    { &opcode_0b,  6 }, // DEC  BC
    { &opcode_0c,  4 }, // INC  C
    { &opcode_0d,  4 }, // DEC  C
    { &opcode_0e,  7 }, // LD   C,n
    { &opcode_0f,  4 }, // RRCA
    { &opcode_10,  8 }, // DJNZ d
    { &opcode_11, 10 }, // LD   DE,nn
    { &opcode_12,  7 }, // LD   (DE),A
    { &opcode_13,  6 }, // INC  DE
    { &opcode_14,  4 }, // INC  D
    { &opcode_15,  4 }, // DEC  D
    { &opcode_16,  7 }, // LD   D,n
    { &opcode_17,  4 }, // RLA
    { &opcode_18, 12 }, // JR   d
    { &opcode_19, 11 }, // ADD  HL,DE
    { &opcode_1a,  7 }, // LD   A,(DE)
    { &opcode_1b,  6 }, // DEC  DE
    { &opcode_1c,  4 }, // INC  E
    { &opcode_1d,  4 }, // DEC  E
    { &opcode_1e,  7 }, // LD   E,n
    { &opcode_1f,  4 }, // RRA
    { &opcode_20,  7 }, // JR   NZ,d
    { &opcode_21, 10 }, // LD   HL,nn
    { &opcode_22, 16 }, // LD   (nn),HL
    { &opcode_23,  6 }, // INC  HL
    { &opcode_24,  4 }, // INC  H
    { &opcode_25,  4 }, // DEC  H
    { &opcode_26,  7 }, // LD   H,n
    { &opcode_27,  4 }, // DAA
    { &opcode_28,  7 }, // JR   Z,d
    { &opcode_29, 11 }, // ADD  HL,HL
    { &opcode_2a, 16 }, // LD   HL,(nn)
    { &opcode_2b,  6 }, // DEC  HL
    { &opcode_2c,  4 }, // INC  L
    { &opcode_2d,  4 }, // DEC  L
    { &opcode_2e,  7 }, // LD   L,n
    { &opcode_2f,  4 }, // CPL
    { &opcode_30,  7 }, // JR   NC,d
    { &opcode_31, 10 }, // LD   SP,nn
    { &opcode_32, 13 }, // LD   (nn),A
    { &opcode_33,  6 }, // INC  SP
    { &opcode_34, 11 }, // INC  (HL)
    { &opcode_35, 11 }, // DEC  (HL)
    { &opcode_36, 10 }, // LD   (HL),n
    { &opcode_37,  4 }, // SCF
    { &opcode_38,  7 }, // JR   C,d
    { &opcode_39, 11 }, // ADD  HL,SP
    { &opcode_3a, 13 }, // LD   A,(nn)
    { &opcode_3b,  6 }, // DEC  SP
    { &opcode_3c,  4 }, // INC  A
    { &opcode_3d,  4 }, // DEC  A
    { &opcode_3e,  7 }, // LD   A,n
    { &opcode_3f,  4 }, // CCF
    { &opcode_40,  4 }, // LD   B,B
    { &opcode_41,  4 }, // LD   B,C
    { &opcode_42,  4 }, // LD   B,D
    { &opcode_43,  4 }, // LD   B,E
    { &opcode_44,  4 }, // LD   B,H
    { &opcode_45,  4 }, // LD   B,L
    { &opcode_46,  7 }, // LD   B,(HL)
    { &opcode_47,  4 }, // LD   B,A
    { &opcode_48,  4 }, // LD   C,B
    { &opcode_49,  4 }, // LD   C,C
    { &opcode_4a,  4 }, // LD   C,D
    { &opcode_4b,  4 }, // LD   C,E
    { &opcode_4c,  4 }, // LD   C,H
    { &opcode_4d,  4 }, // LD   C,L
    { &opcode_4e,  7 }, // LD   C,(HL)
    { &opcode_4f,  4 }, // LD   C,A
    { &opcode_50,  4 }, // LD   D,B
    { &opcode_51,  4 }, // LD   D,C
    { &opcode_52,  4 }, // LD   D,D
    { &opcode_53,  4 }, // LD   D,E
    { &opcode_54,  4 }, // LD   D,H
    { &opcode_55,  4 }, // LD   D,L
    { &opcode_56,  7 }, // LD   D,(HL)
    { &opcode_57,  4 }, // LD   D,A
    { &opcode_58,  4 }, // LD   E,B
    { &opcode_59,  4 }, // LD   E,C
    { &opcode_5a,  4 }, // LD   E,D
    { &opcode_5b,  4 }, // LD   E,E
    { &opcode_5c,  4 }, // LD   E,H
    { &opcode_5d,  4 }, // LD   E,L
    { &opcode_5e,  7 }, // LD   E,(HL)
    { &opcode_5f,  4 }, // LD   E,A
    { &opcode_60,  4 }, // LD   H,B
    { &opcode_61,  4 }, // LD   H,C
    { &opcode_62,  4 }, // LD   H,D
    { &opcode_63,  4 }, // LD   H,E
    { &opcode_64,  4 }, // LD   H,H
    { &opcode_65,  4 }, // LD   H,L
    { &opcode_66,  7 }, // LD   H,(HL)
    { &opcode_67,  4 }, // LD   H,A
    { &opcode_68,  4 }, // LD   L,B
    { &opcode_69,  4 }, // LD   L,C
    { &opcode_6a,  4 }, // LD   L,D
    { &opcode_6b,  4 }, // LD   L,E
    { &opcode_6c,  4 }, // LD   L,H
    { &opcode_6d,  4 }, // LD   L,L
    { &opcode_6e,  7 }, // LD   L,(HL)
    { &opcode_6f,  4 }, // LD   L,A
    { &opcode_70,  7 }, // LD   (HL),B
    { &opcode_71,  7 }, // LD   (HL),C
    { &opcode_72,  7 }, // LD   (HL),D
    { &opcode_73,  7 }, // LD   (HL),E
    { &opcode_74,  7 }, // LD   (HL),H
    { &opcode_75,  7 }, // LD   (HL),L
    { &opcode_76,  4 }, // HALT
    { &opcode_77,  7 }, // LD   (HL),A
    { &opcode_78,  4 }, // LD   A,B
    { &opcode_79,  4 }, // LD   A,C
    { &opcode_7a,  4 }, // LD   A,D
    { &opcode_7b,  4 }, // LD   A,E
    { &opcode_7c,  4 }, // LD   A,H
    { &opcode_7d,  4 }, // LD   A,L
    { &opcode_7e,  7 }, // LD   A,(HL)
    { &opcode_7f,  4 }, // LD   A,A
    { &opcode_80,  4 }, // ADD  A,B
    { &opcode_81,  4 }, // ADD  A,C
    { &opcode_82,  4 }, // ADD  A,D
    { &opcode_83,  4 }, // ADD  A,E
    { &opcode_84,  4 }, // ADD  A,H
    { &opcode_85,  4 }, // ADD  A,L
    { &opcode_86,  7 }, // ADD  A,(HL)
    { &opcode_87,  4 }, // ADD  A,A
    { &opcode_88,  4 }, // ADC  A,B
    { &opcode_89,  4 }, // ADC  A,C
    { &opcode_8a,  4 }, // ADC  A,D
    { &opcode_8b,  4 }, // ADC  A,E
    { &opcode_8c,  4 }, // ADC  A,H
    { &opcode_8d,  4 }, // ADC  A,L
    { &opcode_8e,  7 }, // ADC  A,(HL)
    { &opcode_8f,  4 }, // ADC  A,A
    { &opcode_90,  4 }, // SUB  B
    { &opcode_91,  4 }, // SUB  C
    { &opcode_92,  4 }, // SUB  D
    { &opcode_93,  4 }, // SUB  E
    { &opcode_94,  4 }, // SUB  H
    { &opcode_95,  4 }, // SUB  L
    { &opcode_96,  7 }, // SUB  (HL)
    { &opcode_97,  4 }, // SUB  A
    { &opcode_98,  4 }, // SBC  A,B
    { &opcode_99,  4 }, // SBC  A,C
    { &opcode_9a,  4 }, // SBC  A,D
    { &opcode_9b,  4 }, // SBC  A,E
    { &opcode_9c,  4 }, // SBC  A,H
    { &opcode_9d,  4 }, // SBC  A,L
    { &opcode_9e,  7 }, // SBC  A,(HL)
    { &opcode_9f,  4 }, // SBC  A,A
    { &opcode_a0,  4 }, // AND  B
    { &opcode_a1,  4 }, // AND  C
    { &opcode_a2,  4 }, // AND  D
    { &opcode_a3,  4 }, // AND  E
    { &opcode_a4,  4 }, // AND  H
    { &opcode_a5,  4 }, // AND  L
    { &opcode_a6,  7 }, // AND  (HL)
    { &opcode_a7,  4 }, // AND  A
    { &opcode_a8,  4 }, // XOR  B
    { &opcode_a9,  4 }, // XOR  C
    { &opcode_aa,  4 }, // XOR  D
    { &opcode_ab,  4 }, // XOR  E
    { &opcode_ac,  4 }, // XOR  H
    { &opcode_ad,  4 }, // XOR  L
    { &opcode_ae,  7 }, // XOR  (HL)
    { &opcode_af,  4 }, // XOR  A
    { &opcode_b0,  4 }, // OR   B
    { &opcode_b1,  4 }, // OR   C
    { &opcode_b2,  4 }, // OR   D
    { &opcode_b3,  4 }, // OR   E
    { &opcode_b4,  4 }, // OR   H
    { &opcode_b5,  4 }, // OR   L
    { &opcode_b6,  7 }, // OR   (HL)
    { &opcode_b7,  4 }, // OR   A
    { &opcode_b8,  4 }, // CP   B
    { &opcode_b9,  4 }, // CP   C
    { &opcode_ba,  4 }, // CP   D
    { &opcode_bb,  4 }, // CP   E
    { &opcode_bc,  4 }, // CP   H
    { &opcode_bd,  4 }, // CP   L
    { &opcode_be,  7 }, // CP   (HL)
    { &opcode_bf,  4 }, // CP   A
    { &opcode_c0,  5 }, // RET  NZ
    { &opcode_c1, 10 }, // POP  BC
    { &opcode_c2, 10 }, // JP   NZ,nn
    { &opcode_c3, 10 }, // JP   nn
    { &opcode_c4, 10 }, // CALL NZ,nn
    { &opcode_c5, 11 }, // PUSH BC
    { &opcode_c6,  7 }, // ADD  A,n
    { &opcode_c7, 11 }, // RST  0
    { &opcode_c8,  5 }, // RET  Z
    { &opcode_c9, 10 }, // RET
    { &opcode_ca, 10 }, // JP   Z,nn
    { &opcode_cb,  0 }, // [Prefix]
    { &opcode_cc, 10 }, // CALL Z,nn
    { &opcode_cd, 17 }, // CALL nn
    { &opcode_ce,  7 }, // ADC  A,n
    { &opcode_cf, 11 }, // RST  8
    { &opcode_d0,  5 }, // RET  NC
    { &opcode_d1, 10 }, // POP  DE
    { &opcode_d2, 10 }, // JP   NC,nn
    { &opcode_d3, 11 }, // OUT  (n),A
    { &opcode_d4, 10 }, // CALL NC,nn
    { &opcode_d5, 11 }, // PUSH DE
    { &opcode_d6,  7 }, // SUB  n
    { &opcode_d7, 11 }, // RST  10H
    { &opcode_d8,  5 }, // RET  C
    { &opcode_d9,  4 }, // EXX
    { &opcode_da, 10 }, // JP   C,nn
    { &opcode_db, 11 }, // IN   A,(n)
    { &opcode_dc, 10 }, // CALL C,nn
    { &opcode_dd,  0 }, // [IX Prefix]
    { &opcode_de,  7 }, // SBC  A,n
    { &opcode_df, 11 }, // RST  18H
    { &opcode_e0,  5 }, // RET  PO
    { &opcode_e1, 10 }, // POP  HL
    { &opcode_e2, 10 }, // JP   PO,nn
    { &opcode_e3, 19 }, // EX   (SP),HL
    { &opcode_e4, 10 }, // CALL PO,nn
    { &opcode_e5, 11 }, // PUSH HL
    { &opcode_e6,  7 }, // AND  n
    { &opcode_e7, 11 }, // RST  20H
    { &opcode_e8,  5 }, // RET  PE
    { &opcode_e9,  4 }, // JP   (HL)
    { &opcode_ea, 10 }, // JP   PE,nn
    { &opcode_eb,  4 }, // EX   DE,HL
    { &opcode_ec, 10 }, // CALL PE,nn
    { &opcode_ed,  0 }, // [Prefix]
    { &opcode_ee,  7 }, // XOR  n
    { &opcode_ef, 11 }, // RST  28H
    { &opcode_f0,  5 }, // RET  P
    { &opcode_f1, 10 }, // POP  AF
    { &opcode_f2, 10 }, // JP   P,nn
    { &opcode_f3,  4 }, // DI
    { &opcode_f4, 10 }, // CALL P,nn
    { &opcode_f5, 11 }, // PUSH AF
    { &opcode_f6,  7 }, // OR   n
    { &opcode_f7, 11 }, // RST  30H
    { &opcode_f8,  5 }, // RET  M
    { &opcode_f9,  6 }, // LD   SP,HL
    { &opcode_fa, 10 }, // JP   M,nn
    { &opcode_fb,  4 }, // EI
    { &opcode_fc, 10 }, // CALL M,nn
    { &opcode_fd,  0 }, // [IY Prefix]
    { &opcode_fe,  7 }, // CP   n
    { &opcode_ff, 11 }  // RST  38H
};                          

void opcode_00()    // NOP
{
}

void opcode_01()    // LD   BC,nn
{
    C = fetchByte();
    B = fetchByte();
}

void opcode_02()    // LD   (BC),A
{
    writeByte( BC(), A );
}

void opcode_03()    // INC  BC
{
    if( ++C == 0 ) ++B;
}

void opcode_04()    // INC  B
{
    B = incByte( B );
}

void opcode_05()    // DEC  B
{
    B = decByte( B );
}

void opcode_06()    // LD   B,n
{
    B = fetchByte();
}

void opcode_07()    // RLCA
{
    A = (A << 1) | (A >> 7);
    F = F & ~(AddSub | Halfcarry | Carry);
    if( A & 0x01 ) F |= Carry;
}

void opcode_08()    // EX   AF,AF'
{
    unsigned char x;

    x = A; A = A1; A1 = x;
    x = F; F = F1; F1 = x;
}

void opcode_09()    // ADD  HL,BC
{
    unsigned hl = HL();
    unsigned rp = BC();
    unsigned x  = hl + rp;

    F &= Sign | Zero | Parity;
    if( x > 0xFFFF ) F |= Carry;
    if( ((hl & 0xFFF) + (rp & 0xFFF)) > 0xFFF ) F |= Halfcarry;

    L = x & 0xFF;
    H = (x >> 8) & 0xFF;
}

void opcode_0a()    // LD   A,(BC)
{
    A = readByte( BC() );
}

void opcode_0b()    // DEC  BC
{
    if( C-- == 0 ) --B;
}

void opcode_0c()    // INC  C
{
    C = incByte( C );
}

void opcode_0d()    // DEC  C
{
    C = decByte( C );
}

void opcode_0e()    // LD   C,n
{
    C = fetchByte();
}

void opcode_0f()    // RRCA
{
    A = (A >> 1) | (A << 7);
    F = F & ~(AddSub | Halfcarry | Carry);
    if( A & 0x80 ) F |= Carry;
}

void opcode_10()    // DJNZ d
{
    unsigned char o = fetchByte();
    
    if( --B != 0 ) relJump( o ); 
}

void opcode_11()    // LD   DE,nn
{
    E = fetchByte();
    D = fetchByte();
}

void opcode_12()    // LD   (DE),A
{
    writeByte( DE(), A );
}

void opcode_13()    // INC  DE
{
    if( ++E == 0 ) ++D;
}

void opcode_14()    // INC  D
{
    D = incByte( D );
}

void opcode_15()    // DEC  D
{
    D = decByte( D );
}

void opcode_16()    // LD   D,n
{
    D = fetchByte();
}

void opcode_17()    // RLA
{
    unsigned char a = A;

    A <<= 1;
    if( F & Carry ) A |= 0x01;
    F = F & ~(AddSub | Halfcarry | Carry);
    if( a & 0x80 ) F |= Carry;
}

void opcode_18()    // JR   d
{
    relJump( fetchByte() );
}

void opcode_19()    // ADD  HL,DE
{
    unsigned hl = HL();
    unsigned rp = DE();
    unsigned x  = hl + rp;

    F &= Sign | Zero | Parity;
    if( x > 0xFFFF ) F |= Carry;
    if( ((hl & 0xFFF) + (rp & 0xFFF)) > 0xFFF ) F |= Halfcarry;

    L = x & 0xFF;
    H = (x >> 8) & 0xFF;
}

void opcode_1a()    // LD   A,(DE)
{
    A = readByte( DE() );
}

void opcode_1b()    // DEC  DE
{
    if( E-- == 0 ) --D;
}

void opcode_1c()    // INC  E
{
    E = incByte( E );
}

void opcode_1d()    // DEC  E
{
    E = decByte( E );
}

void opcode_1e()    // LD   E,n
{
    E = fetchByte();
}

void opcode_1f()    // RRA
{
    unsigned char a = A;

    A >>= 1;
    if( F & Carry ) A |= 0x80;
    F = F & ~(AddSub | Halfcarry | Carry);
    if( a & 0x01 ) F |= Carry;
}

void opcode_20()    // JR   NZ,d
{
    unsigned char o = fetchByte();
    
    if( ! (F & Zero) ) relJump( o );
}

void opcode_21()    // LD   HL,nn
{
    L = fetchByte();
    H = fetchByte();
}

void opcode_22()    // LD   (nn),HL
{
    unsigned x = fetchWord();

    writeByte( x  , L );
    writeByte( x+1, H );
}

void opcode_23()    // INC  HL
{
    if( ++L == 0 ) ++H;
}

void opcode_24()    // INC  H
{
    H = incByte( H );
}

void opcode_25()    // DEC  H
{
    H = decByte( H );
}

void opcode_26()    // LD   H,n
{
    H = fetchByte();
}

/*
    DAA is computed using the following table to get a diff value
    that is added to or subtracted (according to the N flag) from A:

        C Upper H Lower Diff
        -+-----+-+-----+----
        1   *   0  0-9   60
        1   *   1  0-9   66
        1   *   *  A-F   66
        0  0-9  0  0-9   00
        0  0-9  1  0-9   06
        0  0-8  *  A-F   06
        0  A-F  0  0-9   60
        0  9-F  *  A-F   66
        0  A-F  1  0-9   66

    The carry and halfcarry flags are then updated using similar tables.

    These tables were found by Stefano Donati of Ramsoft and are
    published in the "Undocumented Z80 Documented" paper by Sean Young,
    the following is an algorithmical implementation with no lookups.
*/
void opcode_27()    // DAA
{
    unsigned char diff;
    unsigned char hf = F & Halfcarry;
    unsigned char cf = F & Carry;
    unsigned char lower = A & 0x0F;

    if( cf ) {
        diff = (lower >= 0x0A) || hf ? 0x66 : 0x60;
    }
    else {
        diff = (A >= 0x9A) ? 0x60 : 0x00;

        if( hf || (lower >= 0x0A) ) diff += 0x06;
    }

    if( A >= 0x9A ) cf = Carry;

    if( F & Subtraction ) {
        A -= diff;
        F = PSZ_[A] | Subtraction | cf;
        if( hf && (lower <= 0x05) ) F |= Halfcarry;
    }
    else {
        A += diff;
        F = PSZ_[A] | cf;
        if( lower >= 0x0A ) F |= Halfcarry;
    }
}

void opcode_28()    // JR   Z,d
{
    unsigned char   o = fetchByte();
    
    if( F & Zero ) relJump( o );
}

void opcode_29()    // ADD  HL,HL
{
    unsigned hl = HL();
    unsigned rp = hl;
    unsigned x  = hl + rp;

    F &= Sign | Zero | Parity;
    if( x > 0xFFFF ) F |= Carry;
    if( ((hl & 0xFFF) + (rp & 0xFFF)) > 0xFFF ) F |= Halfcarry;

    L = x & 0xFF;
    H = (x >> 8) & 0xFF;
}

void opcode_2a()    // LD   HL,(nn)
{
    unsigned x = fetchWord();

    L = readByte( x );
    H = readByte( x+1 );
}

void opcode_2b()    // DEC  HL
{
    if( L-- == 0 ) --H;
}

void opcode_2c()    // INC  L
{
    L = incByte( L );
}

void opcode_2d()    // DEC  L
{
    L = decByte( L );
}

void opcode_2e()    // LD   L,n
{
    L = fetchByte();
}

void opcode_2f()    // CPL
{
    A ^= 0xFF;
    F |= AddSub | Halfcarry;
}

void opcode_30()    // JR   NC,d
{
    unsigned char o = fetchByte();
    
    if( ! (F & Carry) ) relJump( o );
}

void opcode_31()    // LD   SP,nn
{
    SP = fetchWord();
}

void opcode_32()    // LD   (nn),A
{
    writeByte( fetchWord(), A );
}

void opcode_33()    // INC  SP
{
    SP = (SP + 1) & 0xFFFF;
}

void opcode_34()    // INC  (HL)
{
    writeByte( HL(), incByte( readByte( HL() ) ) );
}

void opcode_35()    // DEC  (HL)
{
    writeByte( HL(), decByte( readByte( HL() ) ) );
}

void opcode_36()    // LD   (HL),n
{
    writeByte( HL(), fetchByte() );
}

void opcode_37()    // SCF
{
    F = (F & (Parity | Sign | Zero)) | Carry;
}

void opcode_38()    // JR   C,d
{
    unsigned char o = fetchByte();
    
    if( F & Carry ) relJump( o );
}

void opcode_39()    // ADD  HL,SP
{
    unsigned hl = HL();
    unsigned rp = SP;
    unsigned x  = hl + rp;

    F &= Sign | Zero | Parity;
    if( x > 0xFFFF ) F |= Carry;
    if( ((hl & 0xFFF) + (rp & 0xFFF)) > 0xFFF ) F |= Halfcarry;

    L = x & 0xFF;
    H = (x >> 8) & 0xFF;
}

void opcode_3a()    // LD   A,(nn)
{
    A = readByte( fetchWord() );
}

void opcode_3b()    // DEC  SP
{
    SP = (SP - 1) & 0xFFFF;
}

void opcode_3c()    // INC  A
{
    A = incByte( A );
}

void opcode_3d()    // DEC  A
{
    A = decByte( A );
}

void opcode_3e()    // LD   A,n
{
    A = fetchByte();
}

void opcode_3f()    // CCF
{
    if( F & Carry ) {
        F = (F & (Parity | Sign | Zero)) | Halfcarry; // Halfcarry holds previous carry
    }
    else {
        F = (F & (Parity | Sign | Zero)) | Carry;
    }
}

void opcode_40()    // LD   B,B
{
}

void opcode_41()    // LD   B,C
{
    B = C;
}

void opcode_42()    // LD   B,D
{
    B = D;
}

void opcode_43()    // LD   B,E
{
    B = E;
}

void opcode_44()    // LD   B,H
{
    B = H;
}

void opcode_45()    // LD   B,L
{
    B = L;
}

void opcode_46()    // LD   B,(HL)
{
    B = readByte( HL() );
}

void opcode_47()    // LD   B,A
{
    B = A;
}

void opcode_48()    // LD   C,B
{
    C = B;
}

void opcode_49()    // LD   C,C
{
}

void opcode_4a()    // LD   C,D
{
    C = D;
}

void opcode_4b()    // LD   C,E
{
    C = E;
}

void opcode_4c()    // LD   C,H
{
    C = H;
}

void opcode_4d()    // LD   C,L
{
    C = L;
}

void opcode_4e()    // LD   C,(HL)
{
    C = readByte( HL() );
}

void opcode_4f()    // LD   C,A
{
    C = A;
}

void opcode_50()    // LD   D,B
{
    D = B;
}

void opcode_51()    // LD   D,C
{
    D = C;
}

void opcode_52()    // LD   D,D
{
}

void opcode_53()    // LD   D,E
{
    D = E;
}

void opcode_54()    // LD   D,H
{
    D = H;
}

void opcode_55()    // LD   D,L
{
    D = L;
}

void opcode_56()    // LD   D,(HL)
{
    D = readByte( HL() );
}

void opcode_57()    // LD   D,A
{
    D = A;
}

void opcode_58()    // LD   E,B
{
    E = B;
}

void opcode_59()    // LD   E,C
{
    E = C;
}

void opcode_5a()    // LD   E,D
{
    E = D;
}

void opcode_5b()    // LD   E,E
{
}

void opcode_5c()    // LD   E,H
{
    E = H;
}

void opcode_5d()    // LD   E,L
{
    E = L;
}

void opcode_5e()    // LD   E,(HL)
{
    E = readByte( HL() );
}

void opcode_5f()    // LD   E,A
{
    E = A;
}

void opcode_60()    // LD   H,B
{
    H = B;
}

void opcode_61()    // LD   H,C
{
    H = C;
}

void opcode_62()    // LD   H,D
{
    H = D;
}

void opcode_63()    // LD   H,E
{
    H = E;
}

void opcode_64()    // LD   H,H
{
}

void opcode_65()    // LD   H,L
{
    H = L;
}

void opcode_66()    // LD   H,(HL)
{
    H = readByte( HL() );
}

void opcode_67()    // LD   H,A
{
    H = A;
}

void opcode_68()    // LD   L,B
{
    L = B;
}

void opcode_69()    // LD   L,C
{
    L = C;
}

void opcode_6a()    // LD   L,D
{
    L = D;
}

void opcode_6b()    // LD   L,E
{
    L = E;
}

void opcode_6c()    // LD   L,H
{
    L = H;
}

void opcode_6d()    // LD   L,L
{
}

void opcode_6e()    // LD   L,(HL)
{
    L = readByte( HL() );
}

void opcode_6f()    // LD   L,A
{
    L = A;
}

void opcode_70()    // LD   (HL),B
{
    writeByte( HL(), B );
}

void opcode_71()    // LD   (HL),C
{
    writeByte( HL(), C );
}

void opcode_72()    // LD   (HL),D
{
    writeByte( HL(), D );
}

void opcode_73()    // LD   (HL),E
{
    writeByte( HL(), E );
}

void opcode_74()    // LD   (HL),H
{
    writeByte( HL(), H );
}

void opcode_75()    // LD   (HL),L
{
    writeByte( HL(), L );
}

void opcode_76()    // HALT
{
    iflags_ |= Halted;
}

void opcode_77()    // LD   (HL),A
{
    writeByte( HL(), A );
}

void opcode_78()    // LD   A,B
{
    A = B;
}

void opcode_79()    // LD   A,C
{
    A = C;
}

void opcode_7a()    // LD   A,D
{
    A = D;
}

void opcode_7b()    // LD   A,E
{
    A = E;
}

void opcode_7c()    // LD   A,H
{
    A = H;
}

void opcode_7d()    // LD   A,L
{
    A = L;
}

void opcode_7e()    // LD   A,(HL)
{
    A = readByte( HL() );
}

void opcode_7f()    // LD   A,A
{
}

void opcode_80()    // ADD  A,B
{
    addByte( B, 0 );
}

void opcode_81()    // ADD  A,C
{
    addByte( C, 0 );
}

void opcode_82()    // ADD  A,D
{
    addByte( D, 0 );
}

void opcode_83()    // ADD  A,E
{
    addByte( E, 0 );
}

void opcode_84()    // ADD  A,H
{
    addByte( H, 0 );
}

void opcode_85()    // ADD  A,L
{
    addByte( L, 0 );
}

void opcode_86()    // ADD  A,(HL)
{
    addByte( readByte( HL() ), 0 );
}

void opcode_87()    // ADD  A,A
{
    addByte( A, 0 );
}

void opcode_88()    // ADC  A,B
{
    addByte( B, F & Carry );
}

void opcode_89()    // ADC  A,C
{
    addByte( C, F & Carry );
}

void opcode_8a()    // ADC  A,D
{
    addByte( D, F & Carry );
}

void opcode_8b()    // ADC  A,E
{
    addByte( E, F & Carry );
}

void opcode_8c()    // ADC  A,H
{
    addByte( H, F & Carry );
}

void opcode_8d()    // ADC  A,L
{
    addByte( L, F & Carry );
}

void opcode_8e()    // ADC  A,(HL)
{
    addByte( readByte( HL() ), F & Carry );
}

void opcode_8f()    // ADC  A,A
{
    addByte( A, F & Carry );
}

void opcode_90()    // SUB  B
{
    A = subByte( B, 0 );
}

void opcode_91()    // SUB  C
{
    A = subByte( C, 0 );
}

void opcode_92()    // SUB  D
{
    A = subByte( D, 0 );
}

void opcode_93()    // SUB  E
{
    A = subByte( E, 0 );
}

void opcode_94()    // SUB  H
{
    A = subByte( H, 0 );
}

void opcode_95()    // SUB  L
{
    A = subByte( L, 0 );
}

void opcode_96()    // SUB  (HL)
{
    A = subByte( readByte( HL() ), 0 );
}

void opcode_97()    // SUB  A
{
    A = subByte( A, 0 );
}

void opcode_98()    // SBC  A,B
{
    A = subByte( B, F & Carry );
}

void opcode_99()    // SBC  A,C
{
    A = subByte( C, F & Carry );
}

void opcode_9a()    // SBC  A,D
{
    A = subByte( D, F & Carry );
}

void opcode_9b()    // SBC  A,E
{
    A = subByte( E, F & Carry );
}

void opcode_9c()    // SBC  A,H
{
    A = subByte( H, F & Carry );
}

void opcode_9d()    // SBC  A,L
{
    A = subByte( L, F & Carry );
}

void opcode_9e()    // SBC  A,(HL)
{
    A = subByte( readByte( HL() ), F & Carry );
}

void opcode_9f()    // SBC  A,A
{
    A = subByte( A, F & Carry );
}

void opcode_a0()    // AND  B
{
    A &= B;
    setFlagsPSZ();
}

void opcode_a1()    // AND  C
{
    A &= C;
    setFlagsPSZ();
}

void opcode_a2()    // AND  D
{
    A &= D;
    setFlagsPSZ();
}

void opcode_a3()    // AND  E
{
    A &= E;
    setFlagsPSZ();
}

void opcode_a4()    // AND  H
{
    A &= H;
    setFlagsPSZ();
}

void opcode_a5()    // AND  L
{
    A &= L;
    setFlagsPSZ();
}

void opcode_a6()    // AND  (HL)
{
    A &= readByte( HL() );
    setFlagsPSZ();
}

void opcode_a7()    // AND  A
{
    setFlagsPSZ();
}

void opcode_a8()    // XOR  B
{
    A ^= B;
    setFlags35PSZ000();
}

void opcode_a9()    // XOR  C
{
    A ^= C;
    setFlags35PSZ000();
}

void opcode_aa()    // XOR  D
{
    A ^= D;
    setFlags35PSZ000();
}

void opcode_ab()    // XOR  E
{
    A ^= E;
    setFlags35PSZ000();
}

void opcode_ac()    // XOR  H
{
    A ^= H;
    setFlags35PSZ000();
}

void opcode_ad()    // XOR  L
{
    A ^= L;
    setFlags35PSZ000();
}

void opcode_ae()    // XOR  (HL)
{
    A ^= readByte( HL() );
    setFlags35PSZ000();
}

void opcode_af()    // XOR  A
{
    A = 0;
    setFlags35PSZ000();
}

void opcode_b0()    // OR   B
{
    A |= B;
    setFlags35PSZ000();
}

void opcode_b1()    // OR   C
{
    A |= C;
    setFlags35PSZ000();
}

void opcode_b2()    // OR   D
{
    A |= D;
    setFlags35PSZ000();
}

void opcode_b3()    // OR   E
{
    A |= E;
    setFlags35PSZ000();
}

void opcode_b4()    // OR   H
{
    A |= H;
    setFlags35PSZ000();
}

void opcode_b5()    // OR   L
{
    A |= L;
    setFlags35PSZ000();
}

void opcode_b6()    // OR   (HL)
{
    A |= readByte( HL() );
    setFlags35PSZ000();
}

void opcode_b7()    // OR   A
{
    setFlags35PSZ000();
}

void opcode_b8()    // CP   B
{
    cmpByte( B );
}

void opcode_b9()    // CP   C
{
    cmpByte( C );
}

void opcode_ba()    // CP   D
{
    cmpByte( D );
}

void opcode_bb()    // CP   E
{
    cmpByte( E );
}

void opcode_bc()    // CP   H
{
    cmpByte( H );
}

void opcode_bd()    // CP   L
{
    cmpByte( L );
}

void opcode_be()    // CP   (HL)
{
    cmpByte( readByte( HL() ) );
}

void opcode_bf()    // CP   A
{
    cmpByte( A );
}

void opcode_c0()    // RET  NZ
{
    if( ! (F & Zero) ) {
        retFromSub();
        cycles_ += 2;
    }
}

void opcode_c1()    // POP  BC
{
    C = readByte( SP++ );
    B = readByte( SP++ );
}

void opcode_c2()    // JP   NZ,nn
{
    if( ! (F & Zero) )
        PC = fetchWord();
    else
        PC += 2;
}

void opcode_c3()    // JP   nn
{
     PC = readWord( PC );
}

void opcode_c4()    // CALL NZ,nn
{
    if( ! (F & Zero) ) {
        callSub( fetchWord() );
        cycles_ += 2;
    }
    else {
        PC += 2;
    }
}

void opcode_c5()    // PUSH BC
{
    writeByte( --SP, B );
    writeByte( --SP, C );
}

void opcode_c6()    // ADD  A,n
{
    addByte( fetchByte(), 0 );
}

void opcode_c7()    // RST  0
{
    callSub( 0x00 );
}

void opcode_c8()    // RET  Z
{
    if( F & Zero ) {
        retFromSub();
        cycles_ += 2;
    }
}

void opcode_c9()    // RET
{
     retFromSub();
}

void opcode_ca()    // JP   Z,nn
{
    if( F & Zero )
        PC = fetchWord();
    else
        PC += 2;
}

void opcode_cb()    // [Prefix]
{
    unsigned op = fetchByte();

    cycles_ += OpInfoCB_[ op ].cycles;
    OpInfoCB_[ op ].handler();
}

void opcode_cc()    // CALL Z,nn
{
    if( F & Zero ) {
        callSub( fetchWord() );
        cycles_ += 2;
    }
    else {
        PC += 2;
    }
}

void opcode_cd()    // CALL nn
{
    callSub( fetchWord() );
}

void opcode_ce()    // ADC  A,n
{
    addByte( fetchByte(), F & Carry );
}

void opcode_cf()    // RST  8
{
    callSub( 0x08 );
}

void opcode_d0()    // RET  NC
{
    if( ! (F & Carry) ) {
        retFromSub();
        cycles_ += 2;
    }
}

void opcode_d1()    // POP  DE
{
    E = readByte( SP++ );
    D = readByte( SP++ );
}

void opcode_d2()    // JP   NC,nn
{
    if( ! (F & Carry) )
        PC = fetchWord();
    else
        PC += 2;
}

void opcode_d3()    // OUT  (n),A
{
    writePort( fetchByte(), A );
}

void opcode_d4()    // CALL NC,nn
{
    if( ! (F & Carry) ) {
        callSub( fetchWord() );
        cycles_ += 2;
    }
    else {
        PC += 2;
    }
}

void opcode_d5()    // PUSH DE
{
    writeByte( --SP, D );
    writeByte( --SP, E );
}

void opcode_d6()    // SUB  n
{
    A = subByte( fetchByte(), 0 );
}

void opcode_d7()    // RST  10H
{
    callSub( 0x10 );
}

void opcode_d8()    // RET  C
{
    if( F & Carry ) {
        retFromSub();
        cycles_ += 2;
    }
}

void opcode_d9()    // EXX
{
    unsigned char x;

    x = B; B = B1; B1 = x;
    x = C; C = C1; C1 = x;
    x = D; D = D1; D1 = x;
    x = E; E = E1; E1 = x;
    x = H; H = H1; H1 = x;
    x = L; L = L1; L1 = x;
}

void opcode_da()    // JP   C,nn
{
    if( F & Carry )
        PC = fetchWord();
    else
        PC += 2;
}

void opcode_db()    // IN   A,(n)
{
    A = readPort( fetchByte() );
}

void opcode_dc()    // CALL C,nn
{
    if( F & Carry ) {
        callSub( fetchWord() );
        cycles_ += 2;
    }
    else {
        PC += 2;
    }
}

void opcode_dd()    // [IX Prefix]
{
    do_opcode_xy( OpInfoDD_ );
    IX &= 0xFFFF;
}

void opcode_de()    // SBC  A,n
{
    A = subByte( fetchByte(), F & Carry );
}

void opcode_df()    // RST  18H
{
    callSub( 0x18 );
}

void opcode_e0()    // RET  PO
{
    if( ! (F & Parity) ) {
        retFromSub();
        cycles_ += 2;
    }
}

void opcode_e1()    // POP  HL
{
    L = readByte( SP++ );
    H = readByte( SP++ );
}

void opcode_e2()    // JP   PO,nn
{
    if( ! (F & Parity) )
        PC = fetchWord();
    else
        PC += 2;
}

void opcode_e3()    // EX   (SP),HL
{
    unsigned char x;

    x = readByte( SP   ); writeByte( SP,   L ); L = x;
    x = readByte( SP+1 ); writeByte( SP+1, H ); H = x;
}

void opcode_e4()    // CALL PO,nn
{
    if( ! (F & Parity) ) {
        callSub( fetchWord() );
        cycles_ += 2;
    }
    else {
        PC += 2;
    }
}

void opcode_e5()    // PUSH HL
{
    writeByte( --SP, H );
    writeByte( --SP, L );
}

void opcode_e6()    // AND  n
{
    A &= fetchByte();
    setFlagsPSZ();
}

void opcode_e7()    // RST  20H
{
    callSub( 0x20 );
}

void opcode_e8()    // RET  PE
{
    if( F & Parity ) {
        retFromSub();
        cycles_ += 2;
    }
}

void opcode_e9()    // JP   (HL)
{
    PC = HL();
}

void opcode_ea()    // JP   PE,nn
{
    if( F & Parity )
        PC = fetchWord();
    else
        PC += 2;
}

void opcode_eb()    // EX   DE,HL
{
    unsigned char x;

    x = D; D = H; H = x;
    x = E; E = L; L = x;
}

void opcode_ec()    // CALL PE,nn
{
    if( F & Parity ) {
        callSub( fetchWord() );
        cycles_ += 2;
    }
    else {
        PC += 2;
    }
}

void opcode_ed()    // [Prefix]
{
    unsigned op = fetchByte();

    if( OpInfoED_[ op ].handler ) {
        OpInfoED_[ op ].handler();
        cycles_ += OpInfoED_[ op ].cycles;
    }
    else {
        cycles_ += OpInfo_[ 0 ].cycles; // NOP
    }
}

void opcode_ee()    // XOR  n
{
    A ^= fetchByte();
    setFlags35PSZ000();
}

void opcode_ef()    // RST  28H
{
    callSub( 0x28 );
}

void opcode_f0()    // RET  P
{
    if( ! (F & Sign) ) {
        retFromSub();
        cycles_ += 2;
    }
}

void opcode_f1()    // POP  AF
{
    F = readByte( SP++ );
    A = readByte( SP++ );
}

void opcode_f2()    // JP   P,nn
{
    if( ! (F & Sign) )
        PC = fetchWord();
    else
        PC += 2;
}

void opcode_f3()    // DI
{
    iflags_ &= ~(IFF1 | IFF2);
}

void opcode_f4()    // CALL P,nn
{
    if( ! (F & Sign) ) {
        callSub( fetchWord() );
        cycles_ += 2;
    }
    else {
        PC += 2;
    }
}

void opcode_f5()    // PUSH AF
{
    writeByte( --SP, A );
    writeByte( --SP, F );
}

void opcode_f6()    // OR   n
{
    A |= fetchByte();
    setFlags35PSZ000();
}

void opcode_f7()    // RST  30H
{
    callSub( 0x30 );
}

void opcode_f8()    // RET  M
{
    if( F & Sign ) {
        retFromSub();
        cycles_ += 2;
    }
}

void opcode_f9()    // LD   SP,HL
{
    SP = HL();
}

void opcode_fa()    // JP   M,nn
{
    if( F & Sign )
        PC = fetchWord();
    else
        PC += 2;
}

void opcode_fb()    // EI
{
    iflags_ |= IFF1 | IFF2;
}

void opcode_fc()    // CALL M,nn
{
    if( F & Sign ) {
        callSub( fetchWord() );
        cycles_ += 2;
    }
    else {
        PC += 2;
    }
}

void opcode_fd()    // [IY Prefix]
{
    do_opcode_xy( OpInfoFD_ );
    IY &= 0xFFFF;
}


void opcode_fe()    // CP   n
{
    subByte( fetchByte(), 0 );
}

void opcode_ff()    // RST  38H
{
    callSub( 0x38 );
}


/* Executes one instruction */
void step(void)
{
    // Update memory refresh register (not strictly needed but...)
    R = (R+1) & 0x7F; 

    if( iflags_ & Halted ) {
        // CPU is halted, do a NOP instruction
        cycles_ += OpInfo_[0].cycles; // NOP
    }
    else {
        // Get the opcode to execute
        unsigned op = fetchByte();

        // Update the cycles counter with the number of cycles for this opcode
        cycles_ += OpInfo_[ op ].cycles;

        // Execute the opcode handler
        OpInfo_[ op ].handler();

        // Update registers
        PC &= 0xFFFF; // Clip program counter
        SP &= 0xFFFF; // Clip stack pointer
    }
}

/*
    Runs the CPU for the specified number of cycles.

    Note: the memory refresh register is not updated!
*/
unsigned z80_run( unsigned runCycles )
{
    unsigned target_cycles = cycles_ + runCycles;

    // Execute instructions until the specified number of
    // cycles has elapsed
    while( cycles_ < target_cycles ) {
        if( iflags_ & Halted ) {
            // CPU is halted, do NOPs for the rest of cycles
            // (this may be off by a few cycles)
            cycles_ = target_cycles;
        }
        else {
            // Get the opcode to execute
            unsigned op = fetchByte();

            // Update the cycles counter with the number of cycles for this opcode
            cycles_ += OpInfo_[ op ].cycles; 

            // Execute the opcode handler
            OpInfo_[ op ].handler();
        }
    }

    // Update registers
    PC &= 0xFFFF; // Clip program counter
    SP &= 0xFFFF; // Clip stack pointer

    // Return the number of extra cycles executed
    return cycles_ - target_cycles;
}

/* Interrupt */
void z80_interrupt( unsigned char data )
{
    // Execute interrupt only if interrupts are enabled
    if( iflags_ & IFF1 ) {
        // Disable maskable interrupts and restart the CPU if halted
        iflags_ &= ~(IFF1 | IFF2 | Halted); 

        switch( getInterruptMode() ) {
        case 0:
            OpInfo_[ data ].handler();
            cycles_ += 11;
            break;
        case 1:
            callSub( 0x38 );
            cycles_ += 11;
            break;
        case 2:
            callSub( readWord( ((unsigned)I) << 8 | (data & 0xFE) ) );
            cycles_ += 19;
            break;
        }
    }
}

/* Non-maskable interrupt */
void nmi(void)
{
    // Disable maskable interrupts but preserve IFF2 (that is a copy of IFF1),
    // also restart the CPU if halted
    iflags_ &= ~(IFF1 | Halted);

    callSub( 0x66 );

    cycles_ += 11;
}

void do_opcode_xy( OpcodeInfo * info )
{
    unsigned op = fetchByte();

    if( (op == 0xDD) || (op == 0xFD) ) {
        // Exit now, to avoid possible infinite loops
        PC--;
        cycles_ += OpInfo_[ 0 ].cycles; // NOP
    }
    else if( op == 0xED ) {
        // IX or IY prefix is ignored for this opcode
        opcode_ed();
    }
    else {
        // Handle IX or IY prefix if possible
        if( info[ op ].handler ) {
            // Extended opcode is valid
            cycles_ += info[ op ].cycles;
            info[ op ].handler();
        }
        else {
            // Extended opcode not valid, fall back to standard opcode
            cycles_ += OpInfo_[ op ].cycles;
            OpInfo_[ op ].handler();
        }
    }
}

