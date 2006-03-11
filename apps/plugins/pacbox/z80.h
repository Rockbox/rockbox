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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef Z80_H_
#define Z80_H_

/**
    Environment for Z80 emulation.

    This class implements all input/output functions for the Z80 emulator class,
    that is it provides functions to access the system RAM, ROM and I/O ports.

    An object of this class corresponds to a system that has no RAM, ROM or ports:
    users of the Z80 emulator should provide the desired behaviour by writing a
    descendant of this class.

    @author Alessandro Scotti
    @version 1.0
*/

    /** Sets the CPU cycle counter to the specified value. */
void setCycles( unsigned value );

void onReturnFromInterrupt(void);


/**
    Z80 software emulator.

    @author Alessandro Scotti
    @version 1.1
*/
    /** CPU flags */
    enum {
        Carry     = 0x01,                       // C
        AddSub    = 0x02, Subtraction = AddSub, // N
        Parity    = 0x04, Overflow = Parity,    // P/V, same bit used for parity and overflow
        Flag3     = 0x08,                       // Aka XF, not used
        Halfcarry = 0x10,                       // H
        Flag5     = 0x20,                       // Aka YF, not used
        Zero      = 0x40,                       // Z
        Sign      = 0x80                        // S
    };

    /**
        Constructor: creates a Z80 object with the specified environment.
    */
//    Z80( Z80Environment & );

    /**
        Copy constructor: creates a copy of the specified Z80 object.
    */
//    Z80( const Z80 & );

//    /** Destructor. */
//    virtual ~Z80() {

    /**
        Resets the CPU to its initial state.

        The stack pointer (SP) is set to F000h, all other registers are cleared.
    */
    void z80_reset(void);

    /**
        Runs the CPU for the specified number of cycles.

        Note that the number of CPU cycles performed by this function may be
        actually a little more than the value specified. If that happens then the
        function returns the number of extra cycles executed.

        @param cycles number of cycles the CPU must execute

        @return the number of extra cycles executed by the last instruction
    */
    unsigned z80_run( unsigned cycles );

    /**
        Executes one instruction.
    */
    void z80_step(void);

    /**
        Invokes an interrupt.

        If interrupts are enabled, the current program counter (PC) is saved on
        the stack and assigned the specified address. When the interrupt handler
        returns, execution resumes from the point where the interrupt occurred.

        The actual interrupt address depends on the current interrupt mode and 
        on the interrupt type. For maskable interrupts, data is as follows:
        - mode 0: data is an opcode that is executed (usually RST xxh);
        - mode 1: data is ignored and a call is made to address 0x38;
        - mode 2: a call is made to the 16 bit address given by (256*I + data).
    */
    void z80_interrupt( unsigned char data );

    /** Forces a non-maskable interrupt. */
    void z80_nmi(void);

    /**
        Copies CPU register from one object to another.

        Note that the environment is not copied, only registers.
    */
//    Z80 & operator = ( const Z80 & );

    /** Returns the size of the buffer needed to take a snapshot of the CPU. */
    unsigned getSizeOfSnapshotBuffer(void);

    /**         
        Takes a snapshot of the CPU.

        A snapshot saves all of the CPU registers and internals. It can be
        restored at any time to bring the CPU back to the exact status it
        had when the snapshot was taken.

        Note: the size of the snapshot buffer must be no less than the size
        returned by the getSizeOfSnapshotBuffer() function.

        @param buffer buffer where the snapshot data is stored

        @return the number of bytes written into the buffer
    */
    unsigned takeSnapshot( unsigned char * buffer );

    /**
        Restores a snapshot taken with takeSnapshot().

        This function uses the data saved in the snapshot buffer to restore the
        CPU status.

        @param buffer buffer where the snapshot data is stored

        @return the number of bytes read from the buffer
    */
    unsigned restoreSnapshot( unsigned char * buffer );

#endif // Z80_H_
