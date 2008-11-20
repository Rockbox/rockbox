/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Adam Gashlin (hcs)
 * Copyright (C) 2004-2007 Shay Green (blargg)
 * Copyright (C) 2002 Brad Martin
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
  
/* The CPU portion (shock!) */
#include "codeclib.h"
#include "spc_codec.h"
#include "spc_profiler.h"

#undef check
#define check assert

#define READ( addr )            (SPC_read( this, addr, spc_time_ ))
#define WRITE( addr, value )    (SPC_write( this, addr, value, spc_time_ ))

#define READ_DP( addr )         READ( (addr) + dp )
#define WRITE_DP( addr, value ) WRITE( (addr) + dp, value )

#define READ_PROG16( addr )     GET_LE16( RAM + (addr) )

#define READ_PC( pc )           (*(pc))
#define READ_PC16( pc )         GET_LE16( pc )

#define SET_PC( n )             (pc = RAM + (n))
#define GET_PC()                (pc - RAM)

static unsigned char const cycle_table [0x100] = {
    2,8,4,5,3,4,3,6,2,6,5,4,5,4,6,8, /* 0 */
    2,8,4,5,4,5,5,6,5,5,6,5,2,2,4,6, /* 1 */
    2,8,4,5,3,4,3,6,2,6,5,4,5,4,5,4, /* 2 */
    2,8,4,5,4,5,5,6,5,5,6,5,2,2,3,8, /* 3 */
    2,8,4,5,3,4,3,6,2,6,4,4,5,4,6,6, /* 4 */
    2,8,4,5,4,5,5,6,5,5,4,5,2,2,4,3, /* 5 */
    2,8,4,5,3,4,3,6,2,6,4,4,5,4,5,5, /* 6 */
    2,8,4,5,4,5,5,6,5,5,5,5,2,2,3,6, /* 7 */
    2,8,4,5,3,4,3,6,2,6,5,4,5,2,4,5, /* 8 */
    2,8,4,5,4,5,5,6,5,5,5,5,2,2,12,5,/* 9 */
    3,8,4,5,3,4,3,6,2,6,4,4,5,2,4,4, /* A */
    2,8,4,5,4,5,5,6,5,5,5,5,2,2,3,4, /* B */
    3,8,4,5,4,5,4,7,2,5,6,4,5,2,4,9, /* C */
    2,8,4,5,5,6,6,7,4,5,4,5,2,2,6,3, /* D */
    2,8,4,5,3,4,3,6,2,4,5,3,4,3,4,3, /* E */
    2,8,4,5,4,5,5,6,3,4,5,4,2,2,4,3  /* F */
};

#define MEM_BIT() CPU_mem_bit( this, pc, spc_time_ )
    
static unsigned CPU_mem_bit( THIS, uint8_t const* pc, long const spc_time_ )
    ICODE_ATTR;
    
static unsigned CPU_mem_bit( THIS, uint8_t const* pc, long const spc_time_ )
{
    unsigned addr = READ_PC16( pc );
    unsigned t = READ( addr & 0x1FFF ) >> (addr >> 13);
    return (t << 8) & 0x100;
}

/* status flags */
enum { st_n = 0x80 };
enum { st_v = 0x40 };
enum { st_p = 0x20 };
enum { st_b = 0x10 };
enum { st_h = 0x08 };
enum { st_i = 0x04 };
enum { st_z = 0x02 };
enum { st_c = 0x01 };
    
#define IS_NEG (nz & 0x880)

#define CALC_STATUS( out )\
{\
    out = status & ~(st_n | st_z | st_c);\
    out |= (c >> 8) & st_c;\
    out |= (dp >> 3) & st_p;\
    if ( IS_NEG ) out |= st_n;\
    if ( !(nz & 0xFF) ) out |= st_z;\
}

#define SET_STATUS( in )\
{\
    status = in & ~(st_n | st_z | st_c | st_p);\
    c = in << 8;\
    nz = ((in << 4) & 0x800) | (~in & st_z);\
    dp = (in << 3) & 0x100;\
}


/* stack */
#define PUSH( v )       (*--sp = (uint8_t) (v))
#define PUSH16( v )     (sp -= 2, SET_LE16( sp, v ))
#define POP()           (*sp++)
#define SET_SP( v )     (sp = RAM + 0x101 + (v))
#define GET_SP()        (sp - 0x101 - RAM)

long CPU_run( THIS, long start_time )
{
#if 0
    ENTER_TIMER(cpu);
#endif
    
    register long spc_time_ = start_time;
    
#ifdef CPU_ARM
    uint8_t* const ram_ = ram.ram;
    #undef RAM
    #define RAM ram_
#endif
    
    int a = this->r.a;
    int x = this->r.x;
    int y = this->r.y;
    
    uint8_t const* pc;
    SET_PC( this->r.pc );
    
    uint8_t* sp;
    SET_SP( this->r.sp );
    
    int status;
    int c;
    int nz;
    unsigned dp;
    {
        int temp = this->r.status;
        SET_STATUS( temp );
    }

    goto loop;
    
    /* main loop */
cbranch_taken_loop:
    pc += *(int8_t const*) pc;
    spc_time_ += 2;
inc_pc_loop:
    pc++;
loop:
    check( (unsigned) GET_PC() < 0x10000 );
    check( (unsigned) GET_SP() < 0x100 );
    check( (unsigned) a < 0x100 );
    check( (unsigned) x < 0x100 );
    check( (unsigned) y < 0x100 );
    
    unsigned opcode = *pc;
    int cycles = this->cycle_table [opcode];
    unsigned data = *++pc;
    if ( (spc_time_ += cycles) > 0 )
        goto out_of_time;
    switch ( opcode )
    {
    
/* Common instructions */

#define BRANCH( cond )\
{\
    pc++;\
    int offset = (int8_t) data;\
    if ( cond ) {\
        pc += offset;\
        spc_time_ += 2;\
    }\
    goto loop;\
}
    
    case 0xF0: /* BEQ (most common) */
        BRANCH( !(uint8_t) nz )
    
    case 0xD0: /* BNE */
        BRANCH( (uint8_t) nz )
    
    case 0x3F: /* CALL */
        PUSH16( GET_PC() + 2 );
        SET_PC( READ_PC16( pc ) );
        goto loop;
    
    case 0x6F: /* RET */
        SET_PC( POP() );
        pc += POP() * 0x100;
        goto loop;

#define CASE( n )   case n:

/* Define common address modes based on opcode for immediate mode. Execution
   ends with data set to the address of the operand. */
#define ADDR_MODES( op )\
    CASE( op - 0x02 ) /* (X) */\
        data = x + dp;\
        pc--;\
        goto end_##op;\
    CASE( op + 0x0F ) /* (dp)+Y */\
        data = READ_PROG16( data + dp ) + y;\
        goto end_##op;\
    CASE( op - 0x01 ) /* (dp+X) */\
        data = READ_PROG16( ((uint8_t) (data + x)) + dp );\
        goto end_##op;\
    CASE( op + 0x0E ) /* abs+Y */\
        data += y;\
        goto abs_##op;\
    CASE( op + 0x0D ) /* abs+X */\
        data += x;\
    CASE( op - 0x03 ) /* abs */\
    abs_##op:\
        data += 0x100 * READ_PC( ++pc );\
        goto end_##op;\
    CASE( op + 0x0C ) /* dp+X */\
        data = (uint8_t) (data + x);\
    CASE( op - 0x04 ) /* dp */\
        data += dp;\
    end_##op:

/* 1. 8-bit Data Transmission Commands. Group I */

    ADDR_MODES( 0xE8 ) /* MOV A,addr */
    /*case 0xE4:*/ /* MOV a,dp (most common) */
    mov_a_addr:
        a = nz = READ( data );
        goto inc_pc_loop;
    case 0xBF: /* MOV A,(X)+ */
        data = x + dp;
        x = (uint8_t) (x + 1);
        pc--;
        goto mov_a_addr;
    
    case 0xE8: /* MOV A,imm */
        a = data;
        nz = data;
        goto inc_pc_loop;
    
    case 0xF9: /* MOV X,dp+Y */
        data = (uint8_t) (data + y);
    case 0xF8: /* MOV X,dp */
        data += dp;
        goto mov_x_addr;
    case 0xE9: /* MOV X,abs */
        data = READ_PC16( pc );
        pc++;
    mov_x_addr:
        data = READ( data );
    case 0xCD: /* MOV X,imm */
        x = data;
        nz = data;
        goto inc_pc_loop;
    
    case 0xFB: /* MOV Y,dp+X */
        data = (uint8_t) (data + x);
    case 0xEB: /* MOV Y,dp */
        data += dp;
        goto mov_y_addr;
    case 0xEC: /* MOV Y,abs */
        data = READ_PC16( pc );
        pc++;
    mov_y_addr:
        data = READ( data );
    case 0x8D: /* MOV Y,imm */
        y = data;
        nz = data;
        goto inc_pc_loop;

/* 2. 8-BIT DATA TRANSMISSION COMMANDS, GROUP 2 */

    ADDR_MODES( 0xC8 ) /* MOV addr,A */
        WRITE( data, a );
        goto inc_pc_loop;
    
    {
        int temp;
    case 0xCC: /* MOV abs,Y */
        temp = y;
        goto mov_abs_temp;
    case 0xC9: /* MOV abs,X */
        temp = x;
    mov_abs_temp:
        WRITE( READ_PC16( pc ), temp );
        pc += 2;
        goto loop;
    }
    
    case 0xD9: /* MOV dp+Y,X */
        data = (uint8_t) (data + y);
    case 0xD8: /* MOV dp,X */
        WRITE( data + dp, x );
        goto inc_pc_loop;
    
    case 0xDB: /* MOV dp+X,Y */
        data = (uint8_t) (data + x);
    case 0xCB: /* MOV dp,Y */
        WRITE( data + dp, y );
        goto inc_pc_loop;

    case 0xFA: /* MOV dp,dp */
        data = READ( data + dp );
    case 0x8F: /* MOV dp,#imm */
        WRITE_DP( READ_PC( ++pc ), data );
        goto inc_pc_loop;
    
/* 3. 8-BIT DATA TRANSMISSIN COMMANDS, GROUP 3. */
    
    case 0x7D: /* MOV A,X */
        a = x;
        nz = x;
        goto loop;
    
    case 0xDD: /* MOV A,Y */
        a = y;
        nz = y;
        goto loop;
    
    case 0x5D: /* MOV X,A */
        x = a;
        nz = a;
        goto loop;
    
    case 0xFD: /* MOV Y,A */
        y = a;
        nz = a;
        goto loop;
    
    case 0x9D: /* MOV X,SP */
        x = nz = GET_SP();
        goto loop;
    
    case 0xBD: /* MOV SP,X */
        SET_SP( x );
        goto loop;
    
    /*case 0xC6:*/ /* MOV (X),A (handled by MOV addr,A in group 2) */
    
    case 0xAF: /* MOV (X)+,A */
        WRITE_DP( x, a );
        x++;
        goto loop;
    
/* 5. 8-BIT LOGIC OPERATION COMMANDS */
    
#define LOGICAL_OP( op, func )\
    ADDR_MODES( op ) /* addr */\
        data = READ( data );\
    case op: /* imm */\
        nz = a func##= data;\
        goto inc_pc_loop;\
    {   unsigned addr;\
    case op + 0x11: /* X,Y */\
        data = READ_DP( y );\
        addr = x + dp;\
        pc--;\
        goto addr_##op;\
    case op + 0x01: /* dp,dp */\
        data = READ_DP( data );\
    case op + 0x10: /*dp,imm*/\
        addr = READ_PC( ++pc ) + dp;\
    addr_##op:\
        nz = data func READ( addr );\
        WRITE( addr, nz );\
        goto inc_pc_loop;\
    }
    
    LOGICAL_OP( 0x28, & ); /* AND */
    
    LOGICAL_OP( 0x08, | ); /* OR */
    
    LOGICAL_OP( 0x48, ^ ); /* EOR */
    
/* 4. 8-BIT ARITHMETIC OPERATION COMMANDS */

    ADDR_MODES( 0x68 ) /* CMP addr */
        data = READ( data );
    case 0x68: /* CMP imm */
        nz = a - data;
        c = ~nz;
        nz &= 0xFF;
        goto inc_pc_loop;
    
    case 0x79: /* CMP (X),(Y) */
        data = READ_DP( x );
        nz = data - READ_DP( y );
        c = ~nz;
        nz &= 0xFF;
        goto loop;
    
    case 0x69: /* CMP (dp),(dp) */
        data = READ_DP( data );
    case 0x78: /* CMP dp,imm */
        nz = READ_DP( READ_PC( ++pc ) ) - data;
        c = ~nz;
        nz &= 0xFF;
        goto inc_pc_loop;
    
    case 0x3E: /* CMP X,dp */
        data += dp;
        goto cmp_x_addr;
    case 0x1E: /* CMP X,abs */
        data = READ_PC16( pc );
        pc++;
    cmp_x_addr:
        data = READ( data );
    case 0xC8: /* CMP X,imm */
        nz = x - data;
        c = ~nz;
        nz &= 0xFF;
        goto inc_pc_loop;
    
    case 0x7E: /* CMP Y,dp */
        data += dp;
        goto cmp_y_addr;
    case 0x5E: /* CMP Y,abs */
        data = READ_PC16( pc );
        pc++;
    cmp_y_addr:
        data = READ( data );
    case 0xAD: /* CMP Y,imm */
        nz = y - data;
        c = ~nz;
        nz &= 0xFF;
        goto inc_pc_loop;
    
    {
        int addr;
    case 0xB9: /* SBC (x),(y) */
    case 0x99: /* ADC (x),(y) */
        pc--; /* compensate for inc later */
        data = READ_DP( x );
        addr = y + dp;
        goto adc_addr;
    case 0xA9: /* SBC dp,dp */
    case 0x89: /* ADC dp,dp */
        data = READ_DP( data );
    case 0xB8: /* SBC dp,imm */
    case 0x98: /* ADC dp,imm */
        addr = READ_PC( ++pc ) + dp;
    adc_addr:
        nz = READ( addr );
        goto adc_data;
        
/* catch ADC and SBC together, then decode later based on operand */
#undef CASE
#define CASE( n ) case n: case (n) + 0x20:
    ADDR_MODES( 0x88 ) /* ADC/SBC addr */
        data = READ( data );
    case 0xA8: /* SBC imm */
    case 0x88: /* ADC imm */
        addr = -1; /* A */
        nz = a;
    adc_data: {
        if ( opcode & 0x20 )
            data ^= 0xFF; /* SBC */
        int carry = (c >> 8) & 1;
        int ov = (nz ^ 0x80) + carry + (int8_t) data; /* sign-extend */
        int hc = (nz & 15) + carry;
        c = nz += data + carry;
        hc = (nz & 15) - hc;
        status = (status & ~(st_v | st_h)) | ((ov >> 2) & st_v) |
            ((hc >> 1) & st_h);
        if ( addr < 0 ) {
            a = (uint8_t) nz;
            goto inc_pc_loop;
        }
        WRITE( addr, (uint8_t) nz );
        goto inc_pc_loop;
    }
    
    }
    
/* 6. ADDITION & SUBTRACTION COMMANDS */

#define INC_DEC_REG( reg, n )\
        nz = reg + n;\
        reg = (uint8_t) nz;\
        goto loop;

    case 0xBC: INC_DEC_REG( a, 1 )  /* INC A */
    case 0x3D: INC_DEC_REG( x, 1 )  /* INC X */
    case 0xFC: INC_DEC_REG( y, 1 )  /* INC Y */
    
    case 0x9C: INC_DEC_REG( a, -1 ) /* DEC A */
    case 0x1D: INC_DEC_REG( x, -1 ) /* DEC X */
    case 0xDC: INC_DEC_REG( y, -1 ) /* DEC Y */

    case 0x9B: /* DEC dp+X */
    case 0xBB: /* INC dp+X */
        data = (uint8_t) (data + x);
    case 0x8B: /* DEC dp */
    case 0xAB: /* INC dp */
        data += dp;
        goto inc_abs;
    case 0x8C: /* DEC abs */
    case 0xAC: /* INC abs */
        data = READ_PC16( pc );
        pc++;
    inc_abs:
        nz = ((opcode >> 4) & 2) - 1;
        nz += READ( data );
        WRITE( data, (uint8_t) nz );
        goto inc_pc_loop;
    
/* 7. SHIFT, ROTATION COMMANDS */

    case 0x5C: /* LSR A */
        c = 0;
    case 0x7C:{/* ROR A */
        nz = ((c >> 1) & 0x80) | (a >> 1);
        c = a << 8;
        a = nz;
        goto loop;
    }
    
    case 0x1C: /* ASL A */
        c = 0;
    case 0x3C:{/* ROL A */
        int temp = (c >> 8) & 1;
        c = a << 1;
        nz = c | temp;
        a = (uint8_t) nz;
        goto loop;
    }
    
    case 0x0B: /* ASL dp */
        c = 0;
        data += dp;
        goto rol_mem;
    case 0x1B: /* ASL dp+X */
        c = 0;
    case 0x3B: /* ROL dp+X */
        data = (uint8_t) (data + x);
    case 0x2B: /* ROL dp */
        data += dp;
        goto rol_mem;
    case 0x0C: /* ASL abs */
        c = 0;
    case 0x2C: /* ROL abs */
        data = READ_PC16( pc );
        pc++;
    rol_mem:
        nz = (c >> 8) & 1;
        nz |= (c = READ( data ) << 1);
        WRITE( data, (uint8_t) nz );
        goto inc_pc_loop;
    
    case 0x4B: /* LSR dp */
        c = 0;
        data += dp;
        goto ror_mem;
    case 0x5B: /* LSR dp+X */
        c = 0;
    case 0x7B: /* ROR dp+X */
        data = (uint8_t) (data + x);
    case 0x6B: /* ROR dp */
        data += dp;
        goto ror_mem;
    case 0x4C: /* LSR abs */
        c = 0;
    case 0x6C: /* ROR abs */
        data = READ_PC16( pc );
        pc++;
    ror_mem: {
        int temp = READ( data );
        nz = ((c >> 1) & 0x80) | (temp >> 1);
        c = temp << 8;
        WRITE( data, nz );
        goto inc_pc_loop;
    }

    case 0x9F: /* XCN */
        nz = a = (a >> 4) | (uint8_t) (a << 4);
        goto loop;

/* 8. 16-BIT TRANSMISION COMMANDS */

    case 0xBA: /* MOVW YA,dp */
        a = READ_DP( data );
        nz = (a & 0x7F) | (a >> 1);
        y = READ_DP( (uint8_t) (data + 1) );
        nz |= y;
        goto inc_pc_loop;
    
    case 0xDA: /* MOVW dp,YA */
        WRITE_DP( data, a );
        WRITE_DP( (uint8_t) (data + 1), y );
        goto inc_pc_loop;
    
/* 9. 16-BIT OPERATION COMMANDS */

    case 0x3A: /* INCW dp */
    case 0x1A:{/* DECW dp */
        data += dp;
        
        /* low byte */
        int temp = READ( data );
        temp += ((opcode >> 4) & 2) - 1; /* +1 for INCW, -1 for DECW */
        nz = ((temp >> 1) | temp) & 0x7F;
        WRITE( data, (uint8_t) temp );
        
        /* high byte */
        data = ((uint8_t) (data + 1)) + dp;
        temp >>= 8;
        temp = (uint8_t) (temp + READ( data ));
        nz |= temp;
        WRITE( data, temp );
        
        goto inc_pc_loop;
    }
        
    case 0x9A: /* SUBW YA,dp */
    case 0x7A: /* ADDW YA,dp */
    {
        /* read 16-bit addend */
        int temp = READ_DP( data );
        int sign = READ_DP( (uint8_t) (data + 1) );
        temp += 0x100 * sign;
        status &= ~(st_v | st_h);
        
        /* to do: fix half-carry for SUBW (it's probably wrong) */
        
        /* for SUBW, negate and truncate to 16 bits */
        if ( opcode & 0x80 ) {
            temp = (temp ^ 0xFFFF) + 1;
            sign = temp >> 8;
        }
        
        /* add low byte (A) */
        temp += a;
        a = (uint8_t) temp;
        nz = (temp | (temp >> 1)) & 0x7F;
        
        /* add high byte (Y) */
        temp >>= 8;
        c = y + temp;
        nz = (nz | c) & 0xFF;
        
        /* half-carry (temporary avoids CodeWarrior optimizer bug) */
        unsigned hc = (c & 15) - (y & 15);
        status |= (hc >> 4) & st_h;
        
        /* overflow if sign of YA changed when previous sign
           and addend sign were same */
        status |= (((c ^ y) & ~(y ^ sign)) >> 1) & st_v;
        
        y = (uint8_t) c;
        
        goto inc_pc_loop;
    }
    
    case 0x5A: { /* CMPW YA,dp */
        int temp = a - READ_DP( data );
        nz = ((temp >> 1) | temp) & 0x7F;
        temp = y + (temp >> 8);
        temp -= READ_DP( (uint8_t) (data + 1) );
        nz |= temp;
        c = ~temp;
        nz &= 0xFF;
        goto inc_pc_loop;
    }
    
/* 10. MULTIPLICATION & DIVISON COMMANDS */

    case 0xCF: { /* MUL YA */
        unsigned temp = y * a;
        a = (uint8_t) temp;
        nz = ((temp >> 1) | temp) & 0x7F;
        y = temp >> 8;
        nz |= y;
        goto loop;
    }
    
    case 0x9E: /* DIV YA,X */
    {
        /* behavior based on SPC CPU tests */
        
        status &= ~(st_h | st_v);
        
        if ( (y & 15) >= (x & 15) )
            status |= st_h;
        
        if ( y >= x )
            status |= st_v;
        
        unsigned ya = y * 0x100 + a;
        if ( y < x * 2 )
        {
            a = ya / x;
            y = ya - a * x;
        }
        else
        {
            a = 255 - (ya - x * 0x200) / (256 - x);
            y = x   + (ya - x * 0x200) % (256 - x);
        }
        
        nz = (uint8_t) a;
        a = (uint8_t) a;
        
        goto loop;
    }
    
/* 11. DECIMAL COMPENSATION COMMANDS */
    
    /* seem unused */
    /* case 0xDF: */ /* DAA */
    /* case 0xBE: */ /* DAS */
    
/* 12. BRANCHING COMMANDS */

    case 0x2F: /* BRA rel */
        pc += (int8_t) data;
        goto inc_pc_loop;
    
    case 0x30: /* BMI */
        BRANCH( IS_NEG )
    
    case 0x10: /* BPL */
        BRANCH( !IS_NEG )
    
    case 0xB0: /* BCS */
        BRANCH( c & 0x100 )
    
    case 0x90: /* BCC */
        BRANCH( !(c & 0x100) )
    
    case 0x70: /* BVS */
        BRANCH( status & st_v )
    
    case 0x50: /* BVC */
        BRANCH( !(status & st_v) )
    
    case 0x03: /* BBS dp.bit,rel */
    case 0x23:
    case 0x43:
    case 0x63:
    case 0x83:
    case 0xA3:
    case 0xC3:
    case 0xE3:
        pc++;
        if ( (READ_DP( data ) >> (opcode >> 5)) & 1 )
            goto cbranch_taken_loop;
        goto inc_pc_loop;
    
    case 0x13: /* BBC dp.bit,rel */
    case 0x33:
    case 0x53:
    case 0x73:
    case 0x93:
    case 0xB3:
    case 0xD3:
    case 0xF3:
        pc++;
        if ( !((READ_DP( data ) >> (opcode >> 5)) & 1) )
            goto cbranch_taken_loop;
        goto inc_pc_loop;
    
    case 0xDE: /* CBNE dp+X,rel */
        data = (uint8_t) (data + x);
        /* fall through */
    case 0x2E: /* CBNE dp,rel */
        pc++;
        if ( READ_DP( data ) != a )
            goto cbranch_taken_loop;
        goto inc_pc_loop;
    
    case 0xFE: /* DBNZ Y,rel */
        y = (uint8_t) (y - 1);
        BRANCH( y )
    
    case 0x6E: { /* DBNZ dp,rel */
        pc++;
        unsigned temp = READ_DP( data ) - 1;
        WRITE_DP( (uint8_t) data, (uint8_t) temp );
        if ( temp )
            goto cbranch_taken_loop;
        goto inc_pc_loop;
    }
    
    case 0x1F: /* JMP (abs+X) */
        SET_PC( READ_PC16( pc ) + x );
        /* fall through */
    case 0x5F: /* JMP abs */
        SET_PC( READ_PC16( pc ) );
        goto loop;
    
/* 13. SUB-ROUTINE CALL RETURN COMMANDS */
    
    case 0x0F:{/* BRK */
        check( 0 ); /* untested */
        PUSH16( GET_PC() + 1 );
        SET_PC( READ_PROG16( 0xFFDE ) ); /* vector address verified */
        int temp;
        CALC_STATUS( temp );
        PUSH( temp );
        status = (status | st_b) & ~st_i;
        goto loop;
    }
    
    case 0x4F: /* PCALL offset */
        PUSH16( GET_PC() + 1 );
        SET_PC( 0xFF00 + data );
        goto loop;
    
    case 0x01: /* TCALL n */
    case 0x11:
    case 0x21:
    case 0x31:
    case 0x41:
    case 0x51:
    case 0x61:
    case 0x71:
    case 0x81:
    case 0x91:
    case 0xA1:
    case 0xB1:
    case 0xC1:
    case 0xD1:
    case 0xE1:
    case 0xF1:
        PUSH16( GET_PC() );
        SET_PC( READ_PROG16( 0xFFDE - (opcode >> 3) ) );
        goto loop;
    
/* 14. STACK OPERATION COMMANDS */

    {
        int temp;
    case 0x7F: /* RET1 */
        temp = POP();
        SET_PC( POP() );
        pc += POP() << 8;
        goto set_status;
    case 0x8E: /* POP PSW */
        temp = POP();
    set_status:
        SET_STATUS( temp );
        goto loop;
    }
    
    case 0x0D: { /* PUSH PSW */
        int temp;
        CALC_STATUS( temp );
        PUSH( temp );
        goto loop;
    }

    case 0x2D: /* PUSH A */
        PUSH( a );
        goto loop;
    
    case 0x4D: /* PUSH X */
        PUSH( x );
        goto loop;
    
    case 0x6D: /* PUSH Y */
        PUSH( y );
        goto loop;
    
    case 0xAE: /* POP A */
        a = POP();
        goto loop;
    
    case 0xCE: /* POP X */
        x = POP();
        goto loop;
    
    case 0xEE: /* POP Y */
        y = POP();
        goto loop;
    
/* 15. BIT OPERATION COMMANDS */

    case 0x02: /* SET1 */
    case 0x22:
    case 0x42:
    case 0x62:
    case 0x82:
    case 0xA2:
    case 0xC2:
    case 0xE2:
    case 0x12: /* CLR1 */
    case 0x32:
    case 0x52:
    case 0x72:
    case 0x92:
    case 0xB2:
    case 0xD2:
    case 0xF2: {
        data += dp;
        int bit = 1 << (opcode >> 5);
        int mask = ~bit;
        if ( opcode & 0x10 )
            bit = 0;
        WRITE( data, (READ( data ) & mask) | bit );
        goto inc_pc_loop;
    }
        
    case 0x0E: /* TSET1 abs */
    case 0x4E:{/* TCLR1 abs */
        data = READ_PC16( pc );
        pc += 2;
        unsigned temp = READ( data );
        nz = temp & a;
        temp &= ~a;
        if ( !(opcode & 0x40) )
            temp |= a;
        WRITE( data, temp );
        goto loop;
    }
    
    case 0x4A: /* AND1 C,mem.bit */
        c &= MEM_BIT();
        pc += 2;
        goto loop;
    
    case 0x6A: /* AND1 C,/mem.bit */
        check( 0 ); /* untested */
        c &= ~MEM_BIT();
        pc += 2;
        goto loop;
    
    case 0x0A: /* OR1 C,mem.bit */
        check( 0 ); /* untested */
        c |= MEM_BIT();
        pc += 2;
        goto loop;
    
    case 0x2A: /* OR1 C,/mem.bit */
        check( 0 ); /* untested */
        c |= ~MEM_BIT();
        pc += 2;
        goto loop;
    
    case 0x8A: /* EOR1 C,mem.bit */
        c ^= MEM_BIT();
        pc += 2;
        goto loop;
    
    case 0xEA: { /* NOT1 mem.bit */
        data = READ_PC16( pc );
        pc += 2;
        unsigned temp = READ( data & 0x1FFF );
        temp ^= 1 << (data >> 13);
        WRITE( data & 0x1FFF, temp );
        goto loop;
    }
    
    case 0xCA: { /* MOV1 mem.bit,C */
        data = READ_PC16( pc );
        pc += 2;
        unsigned temp = READ( data & 0x1FFF );
        unsigned bit = data >> 13;
        temp = (temp & ~(1 << bit)) | (((c >> 8) & 1) << bit);
        WRITE( data & 0x1FFF, temp );
        goto loop;
    }
    
    case 0xAA: /* MOV1 C,mem.bit */
        c = MEM_BIT();
        pc += 2;
        goto loop;
    
/* 16. PROGRAM STATUS FLAG OPERATION COMMANDS */

    case 0x60: /* CLRC */
        c = 0;
        goto loop;
        
    case 0x80: /* SETC */
        c = ~0;
        goto loop;
    
    case 0xED: /* NOTC */
        c ^= 0x100;
        goto loop;
        
    case 0xE0: /* CLRV */
        status &= ~(st_v | st_h);
        goto loop;
    
    case 0x20: /* CLRP */
        dp = 0;
        goto loop;
    
    case 0x40: /* SETP */
        dp = 0x100;
        goto loop;
    
    case 0xA0: /* EI */
        check( 0 ); /* untested */
        status |= st_i;
        goto loop;
    
    case 0xC0: /* DI */
        check( 0 ); /* untested */
        status &= ~st_i;
        goto loop;
    
/* 17. OTHER COMMANDS */

    case 0x00: /* NOP */
        goto loop;
    
    /*case 0xEF:*/ /* SLEEP */
    /*case 0xFF:*/ /* STOP */
    case 0xFF:
        c |= 1; /* force switch table to have 256 entries,
                   hopefully helping optimizer */
    } /* switch */
    
    /* unhandled instructions fall out of switch so emulator can catch them */
    
out_of_time:
    /* undo partial execution of opcode */
    spc_time_ -= this->cycle_table [*--pc]; 
    {
        int temp;
        CALC_STATUS( temp );
        this->r.status = (uint8_t) temp;
    }
    
    this->r.pc = GET_PC();
    this->r.sp = (uint8_t) GET_SP();
    this->r.a  = (uint8_t) a;
    this->r.x  = (uint8_t) x;
    this->r.y  = (uint8_t) y;
 
#if 0
    EXIT_TIMER(cpu);
#endif
    return spc_time_;
}

void CPU_Init( THIS )
{
    ci->memcpy( this->cycle_table, cycle_table, sizeof cycle_table );
}

