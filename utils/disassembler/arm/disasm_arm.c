#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define ULONG uint32_t
#define UCHAR uint8_t

#define FRMT     "0x%x"       // "0x%x"
#define SHFTFRMC "%s %s #%d"  // "%s %s %d"
#define SHFTFRMR "%s %s %s"   // "%s %s %s"
//#define FRMT     "0x%x"
//#define SHFTFRMC "%s %s %d"
//#define SHFTFRMR "%s %s %s"

char *cond[16] = { "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le",   "", "nv" };
char *cnd1[16] = { "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le",  " ", "nv" };
char *opcd[16] = {"and","eor","sub","rsb","add","adc","sbc","rsc","tst","teq","cmp","cmn","orr","mov","bic","mvn" };
char  setc[32] = {0,115,0,115,0,115,0,115,0,115,0,115,0,115,0,115,0, 0 ,0, 0 ,0, 0 ,0, 0 ,0,115,0,115,0,115,0,115 };
char *regs[16] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc" };

char *shfts[4] = { "lsl", "lsr", "asr", "ror" };

/*
31-28 27 26 25 24  23 22 21 20 19 18 17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
Cond  0  0  I  ---Opcode--- S |----Rn----- ----Rd----- --------Operand 2-------- Data Processing /PSR Transfer
Cond  0  0  0  0 | 0  0  A  S |----Rd----- ----Rn----- ---Rs---- 1 0 0 1 --Rm--- Multiply
Cond  0  0  0  0 | 1  U  A  S |---RdHi---- ---RdLo---- ---Rn---- 1 0 0 1 --Rm--- Multiply Long
Cond  0  0  0  1 | 0  B  0  0 |----Rn----- ----Rd----- 0  0  0 0 1 0 0 1 --Rm--- Single Data Swap
Cond  0  0  0  1 | 0  0  1  0 |1  1  1  1  1  1  1  1  1  1  1 1 0 0 0 1 --Rn--- Branch and Exchange
Cond  0  0  0  P | U  0  W  L |----Rn----- ----Rd----- 0  0  0 0 1 S H 1 --Rm--- Halfword Data Transfer: register offset
Cond  0  0  0  P | U  1  W  L |----Rn----- ----Rd----- --Offset- 1 S H 1 -Offset Halfword Data Transfer: immediate offset
Cond  0  1  I  P | U  B  W  L |----Rn----- ----Rd----- --------Offset----------- Single Data Transfer
Cond  0  1  1  1 | x  x  x  x |x  x  x  x  x  x  x  x  x  x  x x x x x 1 x x x x Undefined
Cond  1  0  0  P | U  S  W  L |----Rn----- -----------Register List------------- Block Data Transfer
Cond  1  0  1  L | -------------------------Offset------------------------------ Branch
Cond  1  1  0  P | U  N  W  L |----Rn----- ----CRd---- ---CP#--- -----Offset---- Coprocessor Data Transfer
Cond  1  1  1  0 | --CP Opc---|----CRn---- ----CRd---- ---CP#--- -CP-- 0 --CRm-- Coprocessor Data Operation
Cond  1  1  1  0 | CP Opc   L |----CRn---- ----Rd----- ---CP#--- -CP-- 1 --CRm-- Coprocessor Register Transfer
Cond  1  1  1  1 | x  x  x  x |x  x  x  x  x  x  x  x  x  x  x x x x x x x x x x Software Interrupt

0x04200000
0001  0  0  0  0   0  1  1  0       6          e            1    1 1 0 1    8
================================================================================
Cond  0  1  I  P | U  B  W  L |----Rn----- ----Rd----- --------Offset----------- Single Data Transfer


EQ 0 Z set equal
NE 1 Z clear not equal
CS 2 C set unsigned higher or same
CC 3 C clear unsigned lower
MI 4 N set negative
PL 5 N clear positive or zero
VS 6 V set overflow
VC 7 V clear no overflow
HI 8 C set and Z clear unsigned higher
LS 9 C clear or Z set unsigned lower or same
GE A N equals V greater or equal
LT B N not equal to V less than
GT C Z clear AND (N equals V) greater than
LE D Z set OR (N not equal to V) less than or equal
AL E (ignored) always

AND 0 operand1 AND operand2
EOR 1 operand1 EOR operand2
SUB 2 operand1 - operand2
RSB 3 operand2 - operand1
ADD 4 operand1 + operand2
ADC 5 operand1 + operand2 + carry
SBC 6 operand1 - operand2 + carry - 1
RSC 7 operand2 - operand1 + carry - 1
TST 8 AND, but result is not written
TEQ 9 as EOR, but result is not written
CMP A as SUB, but result is not written
CMN B as ADD, but result is not written
ORR C operand1 OR operand2
MOV D operand2 (operand1 is ignored)
BIC E operand1 AND NOT operand2 (Bit clear)
MVN F NOT operand2 (operand1 is ignored)
*/

void multiply_stg(char *stg, ULONG val)
{
  if((val&0xc00000) == 0) // simple mul
  {
    if(val & 0x100000) // set condition flags
      if(val & 0x200000) sprintf(stg+strlen(stg), "mla%ss	", cond[val>>28]);
      else               sprintf(stg+strlen(stg), "mul%ss	", cond[val>>28]);
    else
      if(val & 0x200000) sprintf(stg+strlen(stg), "mla%s	", cnd1[val>>28]);
      else               sprintf(stg+strlen(stg), "mul%s	", cnd1[val>>28]);

    if(val & 0x200000) // accumulate
      sprintf(stg+strlen(stg), "%s, %s, %s, %s", regs[(val>>16)&15], regs[(val>>0)&15], regs[(val>>8)&15], regs[(val>>12)&15]);
    else
      sprintf(stg+strlen(stg), "%s, %s, %s",     regs[(val>>16)&15], regs[(val>>0)&15], regs[(val>>8)&15]);
  }
  else
  {
    if(val & 0x100000) // set condition flags
      if(val & 0x200000) // accumulate
        if(val & 0x400000) sprintf(stg+strlen(stg), "smlal%ss	", cond[val>>28]);
        else               sprintf(stg+strlen(stg), "umlal%ss	", cond[val>>28]);
      else
        if(val & 0x400000) sprintf(stg+strlen(stg), "smull%ss	", cond[val>>28]);
        else               sprintf(stg+strlen(stg), "umull%ss	", cond[val>>28]);
    else
      if(val & 0x200000)
        if(val & 0x400000) sprintf(stg+strlen(stg), "smlal%s	", cond[val>>28]);
        else               sprintf(stg+strlen(stg), "umlal%s	", cond[val>>28]);
      else
        if(val & 0x400000) sprintf(stg+strlen(stg), "smull%s	", cond[val>>28]);
        else               sprintf(stg+strlen(stg), "umull%s	", cond[val>>28]);

    sprintf(stg+strlen(stg), "%s, %s, %s, %s", regs[(val>>12)&15], regs[(val>>16)&15], regs[(val>>0)&15], regs[(val>>8)&15]);
  }
}

void halfword_stg(char *stg, ULONG val)
{
  ULONG off = ((val>>4) & 0xf0) + (val & 0x0f);

  if(val & 0x100000) sprintf(stg+strlen(stg), "ldr%s", cond[val>>28]);
  else               sprintf(stg+strlen(stg), "str%s", cond[val>>28]);

  switch((val>>5) & 3) // SWP, HW, SB, SH
  {
  case 0: sprintf(stg+strlen(stg), "error: SWP"); break;
  case 1: sprintf(stg+strlen(stg), "h	");   break;
  case 2: sprintf(stg+strlen(stg), "sb	"); break;
  case 3: sprintf(stg+strlen(stg), "sh	"); break;
  }

  if(val & 0x400000) // immidiate offset
    if(val & 0x1000000) // pre index
      if(val & 0x200000) // write back
        if(val & 0x800000) sprintf(stg+strlen(stg), "%s, [%s, "FRMT"]!",  regs[(val>>12)&15], regs[(val>>16)&15], off);
        else               sprintf(stg+strlen(stg), "%s, [%s, -"FRMT"]!", regs[(val>>12)&15], regs[(val>>16)&15], off);
      else
        if(val & 0x800000) sprintf(stg+strlen(stg), "%s, [%s, "FRMT"]",   regs[(val>>12)&15], regs[(val>>16)&15], off);
        else               sprintf(stg+strlen(stg), "%s, [%s, -"FRMT"]",  regs[(val>>12)&15], regs[(val>>16)&15], off);
    else
      if(val & 0x200000) // write back
                           sprintf(stg+strlen(stg), "error 'write back' on post indexed");
      else
        if(val & 0x800000) sprintf(stg+strlen(stg), "%s, [%s], "FRMT,  regs[(val>>12)&15], regs[(val>>16)&15], off);
        else               sprintf(stg+strlen(stg), "%s, [%s], -"FRMT, regs[(val>>12)&15], regs[(val>>16)&15], off);
  else
    if(val & 0x1000000) // pre index
      if(val & 0x200000) // write back
        if(val & 0x800000) sprintf(stg+strlen(stg), "%s, [%s, %s]!",  regs[(val>>12)&15], regs[(val>>16)&15], regs[val&15]);
        else               sprintf(stg+strlen(stg), "%s, [%s, -%s]!", regs[(val>>12)&15], regs[(val>>16)&15], regs[val&15]);
      else
        if(val & 0x800000) sprintf(stg+strlen(stg), "%s, [%s, %s]",   regs[(val>>12)&15], regs[(val>>16)&15], regs[val&15]);
        else               sprintf(stg+strlen(stg), "%s, [%s, -%s]",  regs[(val>>12)&15], regs[(val>>16)&15], regs[val&15]);
    else
      if(val & 0x200000) // write back
                           sprintf(stg+strlen(stg), "error 'write back' on post indexed");
      else
        if(val & 0x800000) sprintf(stg+strlen(stg), "%s, [%s], %s",  regs[(val>>12)&15], regs[(val>>16)&15], regs[val&15]);
        else               sprintf(stg+strlen(stg), "%s, [%s], -%s", regs[(val>>12)&15], regs[(val>>16)&15], regs[val&15]);
}

void branch_stg(char *stg, ULONG val, ULONG pos)
{
  ULONG off = pos + (((int32_t)val << 8) >> 6) + 8;

  if((val & 0x0ffffff0) == 0x012fff10) // bx instruction
  { sprintf(stg+strlen(stg), "bx%s	%s", cond[val>>28], regs[val&15]); }
  else
  {
    if(((val>>24)&15) == 10) sprintf(stg+strlen(stg), "b%s	", cond[val>>28]);
    else                     sprintf(stg+strlen(stg), "bl%s	", cond[val>>28]);

    sprintf(stg+strlen(stg), "0x%x", off);
  }
}

void opcode_stg(char *stg, ULONG val, ULONG off)
{
  ULONG des, op1;
  char  op2[80];
  char  *st = stg + strlen(stg);

  if(((val & 0x0ffffff0) == 0x012fff10) && (val & 16))
  { branch_stg(stg, val, off); return; }
  else if(((val & 0x0f000000) == 0x00000000) && ((val & 0xf0) == 0x90))
  { multiply_stg(stg, val); return; }
  else if(((val & 0x0f000000) <= 0x01000000) && ((val & 0x90) == 0x90) && ((val & 0xf0) > 0x90) && ((val & 0x01200000) != 0x00200000))
  { halfword_stg(stg, val); return; }

  sprintf(stg+strlen(stg), "%s%s%s	", opcd[(val>>21) & 15], cond[val>>28], setc[(val>>20) & 31]?"s":" ");

  des = (val>>12) & 15;
  op1 = (val>>16) & 15;

  if(val & 0x2000000) // immidiate
	{
    off = (ULONG)((uint64_t)(val&0xff) << (32 - 2 * ((val >> 8) & 15))) | ((val&0xff) >> 2 * ((val >> 8) & 15));
    sprintf(op2, FRMT" ", off);
	}
  else
  {
    if(val & 16) // shift type
        sprintf(op2, SHFTFRMR, regs[val&15], shfts[(val>>5)&3], regs[(val>>8)&15]);
    else
      if((val>>7) & 31)
        sprintf(op2, SHFTFRMC, regs[val&15], shfts[(val>>5)&3], (val>>7) & 31);
      else
        sprintf(op2, "%s  ", regs[val&15]);
  }

  switch((val>>21) & 15)
  {
  case  0:  case 1:  case 2:  case 3:  case 4:  case 5:  case 6:  case 7:  case 12:
  case 14: sprintf(stg+strlen(stg), "%s,	%s,	%s", regs[des], regs[op1], op2); break;

  case  8:  case 9:  case 10:
  case 11: if(val & 0x100000) // set status
             sprintf(stg+strlen(stg), "%s,	%s", regs[op1], op2); // standard TEQ,TST,CMP,CMN
           else
           { //special MRS/MSR opcodes
             if((((val>>23) & 31) == 2) && ((val & 0x3f0fff) == 0x0f0000))
             { sprintf(st, "mrs%s	%s, %s", cnd1[val>>28], regs[des], val&0x400000?"SPSR_xx":"CPSR"); }
             else
             if((((val>>23) & 31) == 2) && ((val & 0x30fff0) == 0x20f000))
             { sprintf(st, "msr%s	%s, %s", cnd1[val>>28], val&0x400000?"SPSR_xx":"CPSR", regs[val&15]); }
             else
             if((((val>>23) & 31) == 6) && ((val & 0x30f000) == 0x20f000))
             { sprintf(st, "msr%s	%s, %s", cnd1[val>>28], val&0x400000?"SPSR_xx":"CPSR_cf", op2); }
             else
             if((((val>>23) & 31) == 2) && ((val & 0x300ff0) == 0x000090))
             { sprintf(st, "swp%s%s	%s, %s, [%s]", val&0x400000?"b":"", cnd1[val>>28], regs[(val>>12)&15], regs[val&15], regs[(val>>16)&15]); }
             else
             { sprintf(stg+strlen(stg), "??????????????"); }
           } break;
  case 13:
  case 15: sprintf(stg+strlen(stg), "%s, %s", regs[des], op2); break;
  }
}

void opcode_cop(char *stg, ULONG val, ULONG off)
{
  char* op;
  int   opcode1 = (val >> 21) & 0x7;
  int   CRn = (val >> 16) & 0xf;
  int   Rd = (val >> 12) & 0xf;
  int   cp_num = (val >> 8) & 0xf;
  int   opcode2 = (val >> 5) & 0x7;
  int   CRm = val & 0xf;


// ee073f5e        mcr     15, 0, r3, cr7, cr14, {2}

  if (val & (1<<4)) {
    if (val & (1<<20)) {
      op = "mrc";
    } else {
      op = "mcr";
    }
    opcode1 = (val >> 21) & 0x7;
    CRn = (val >> 16) & 0xf;
    Rd = (val >> 12) & 0xf;
    cp_num = (val >> 8) & 0xf;
    opcode2 = (val >> 5) & 0x7;
    CRm = val & 0xf;

    sprintf(stg+strlen(stg), "%s%s	%d, %d, r%d, cr%d, cr%d, {%d}", op, cnd1[val>>28], cp_num, opcode1, Rd, CRn, CRm, opcode2);
  } else {
    op = "cdp";

    opcode1 = (val >> 20) & 0xf;
    CRn = (val >> 16) & 0xf;
    Rd = (val >> 12) & 0xf;
    cp_num = (val >> 8) & 0xf;
    opcode2 = (val >> 5) & 0x7;
    CRm = val & 0xf;

    sprintf(stg+strlen(stg), "%s%s	%d, %d, cr%d, cr%d, cr%d, {%d}", op, cnd1[val>>28], cp_num, opcode1, Rd, CRn, CRm, opcode2);
  }

}


void single_data(char *stg, ULONG val)
{
  char  op2[80];

  if(((val & 0x0e000000) == 0x06000000) && (val & 16))
  { sprintf(stg+strlen(stg), "undef%s", cond[val>>28]);
    return;
  }

  if(val & 0x400000)
    if(val & 0x100000) sprintf(stg+strlen(stg), "ldr%sb	", cond[val>>28]);
    else               sprintf(stg+strlen(stg), "str%sb	", cond[val>>28]);
  else
    if(val & 0x100000) sprintf(stg+strlen(stg), "ldr%s	", cnd1[val>>28]);
    else               sprintf(stg+strlen(stg), "str%s	", cnd1[val>>28]);

  if(val & 0x2000000) {// reg offset
    if(val & 16) // shift type
      sprintf(op2, "error: reg defined shift");
    else 
      if((val>>7) & 31)
        sprintf(op2, SHFTFRMC, regs[val&15], shfts[(val>>5)&3], (val>>7) & 31);
      else
        sprintf(op2, "%s", regs[val&15]);
  }

  if(val & 0x2000000) // reg offset
    if(val & 0x1000000) // pre index
      if(val & 0x800000) // up offset (+)
        if(val & 0x200000) // write back
          sprintf(stg+strlen(stg), "%s, [%s, %s]!",  regs[(val>>12)&15], regs[(val>>16)&15], op2);
        else
          sprintf(stg+strlen(stg), "%s, [%s, %s]",   regs[(val>>12)&15], regs[(val>>16)&15], op2);
      else
        if(val & 0x200000) // write back
          sprintf(stg+strlen(stg), "%s, [%s, -%s]!", regs[(val>>12)&15], regs[(val>>16)&15], op2);
        else
          sprintf(stg+strlen(stg), "%s, [%s, -%s]",  regs[(val>>12)&15], regs[(val>>16)&15], op2);
    else
      if(val & 0x200000) // write back
          sprintf(stg+strlen(stg), "error 'write back' set");
      else
        if(val & 0x800000) // up offset (+)
          sprintf(stg+strlen(stg), "%s, [%s], %s",   regs[(val>>12)&15], regs[(val>>16)&15], op2);
        else
          sprintf(stg+strlen(stg), "%s, [%s], -%s",  regs[(val>>12)&15], regs[(val>>16)&15], op2);
  else
    if(val & 0x1000000) // pre index
      if(val & 0x800000) // up offset (+)
        if(val & 0x200000) // write back
          if(val & 0xfff)                  sprintf(stg+strlen(stg), "%s, [%s, "FRMT"]!", regs[(val>>12)&15], regs[(val>>16)&15], val & 0xfff);
          else                             sprintf(stg+strlen(stg), "%s, [%s]!",         regs[(val>>12)&15], regs[(val>>16)&15]);
        else
          if(val & 0xfff)                  sprintf(stg+strlen(stg), "%s, [%s, "FRMT"]",  regs[(val>>12)&15], regs[(val>>16)&15], val & 0xfff);
          else                             sprintf(stg+strlen(stg), "%s, [%s]",          regs[(val>>12)&15], regs[(val>>16)&15]);
      else
        if(val & 0x200000) // write back
          if(val & 0xfff)                  sprintf(stg+strlen(stg), "%s, [%s, -"FRMT"]!", regs[(val>>12)&15], regs[(val>>16)&15], val & 0xfff);
          else                             sprintf(stg+strlen(stg), "%s, [%s]!",          regs[(val>>12)&15], regs[(val>>16)&15]);
        else
          if(val & 0xfff)                  sprintf(stg+strlen(stg), "%s, [%s, -"FRMT"]", regs[(val>>12)&15], regs[(val>>16)&15], val & 0xfff);
          else                             sprintf(stg+strlen(stg), "%s, [%s]",          regs[(val>>12)&15], regs[(val>>16)&15]);
    else
      if(val & 0x200000) // write back    
                                           sprintf(stg+strlen(stg), "error 'write back' set");
      else
        if(val & 0x800000) // up offset (+)
          if(val & 0xfff)                  sprintf(stg+strlen(stg), "%s, [%s], "FRMT, regs[(val>>12)&15], regs[(val>>16)&15], val & 0xfff);
          else                             sprintf(stg+strlen(stg), "%s, [%s]",       regs[(val>>12)&15], regs[(val>>16)&15]);
        else
          if(val & 0xfff)                  sprintf(stg+strlen(stg), "%s, [%s], -"FRMT, regs[(val>>12)&15], regs[(val>>16)&15], val & 0xfff);
          else                             sprintf(stg+strlen(stg), "%s, [%s]",        regs[(val>>12)&15], regs[(val>>16)&15]);
}

void block_data(char *stg, ULONG val)
{
  char lst[80];
  int  i;

  strcpy(lst, "{");
  for(i=0; i<16; i++)
    if(val & (1<<i))
      sprintf(lst+strlen(lst), "%s, ", regs[i]);
  if(strlen(lst)>2)
    strcpy(lst+strlen(lst)-2, "}");
  else
    strcpy(lst+strlen(lst), "}");

  if(val & 0x400000) // load psr or force user mode
  strcpy(lst+strlen(lst), "^");


  if(val & 0x100000) // load
    if(val & 0x1000000) // pre offset
      if(val & 0x800000) sprintf(stg+strlen(stg), "ldm%sib	", cond[val>>28]);
      else               sprintf(stg+strlen(stg), "ldm%sdb	", cond[val>>28]);
    else
      if(val & 0x800000) sprintf(stg+strlen(stg), "ldm%sia	", cond[val>>28]);
      else               sprintf(stg+strlen(stg), "ldm%sda	", cond[val>>28]);
  else
    if(val & 0x1000000)
      if(val & 0x800000) sprintf(stg+strlen(stg), "stm%sib	", cond[val>>28]);
      else               sprintf(stg+strlen(stg), "stm%sdb	", cond[val>>28]);
    else
      if(val & 0x800000) sprintf(stg+strlen(stg), "stm%sia	", cond[val>>28]);
      else               sprintf(stg+strlen(stg), "stm%sda	", cond[val>>28]);

  switch((val>>21)&3)
  {
  case 0: sprintf(stg+strlen(stg), "%s, %s", regs[(val>>16)&15], lst); break;
  case 1: sprintf(stg+strlen(stg), "%s!, %s", regs[(val>>16)&15], lst); break;
  case 2: sprintf(stg+strlen(stg), "%s, %s", regs[(val>>16)&15], lst); break;
  case 3: sprintf(stg+strlen(stg), "%s!, %s", regs[(val>>16)&15], lst); break;
  }
}

void dis_asm(ULONG off, ULONG val, char *stg)
{
  sprintf(stg, "%6x:	%08x	", off, val);

  switch((val >> 24) & 15)
  {
    case  0:
    case  1:
    case  2:
    case  3: opcode_stg(stg, val, off); break;
    case  4:
    case  5:
    case  6: 
    case  7: single_data(stg, val); break;
    case  8: 
    case  9: block_data(stg, val); break;
    case 10:
    case 11: branch_stg(stg, val, off); break;
    case 12:
    case 13: sprintf(stg+strlen(stg), "cop%s", cnd1[val>>28]); break;
    case 14: opcode_cop(stg, val, off); break;
    case 15: sprintf(stg+strlen(stg), "swi%s", cnd1[val>>28]); break;
  }
}
