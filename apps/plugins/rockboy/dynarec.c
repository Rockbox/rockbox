#ifdef DYNAREC
#include <system.h>
#include "rockmacros.h"
#include "defs.h"
#include "regs.h"
#include "hw.h"
#include "cpu.h"
#include "lcdc.h"
#include "mem.h"
#include "fastmem.h"
#include "cpuregs.h"
#include "cpucore.h"

void *dynapointer,*branchp[10];
int blockclen;

#define DWRITEB(a) *((unsigned char *) dynapointer)=(a); dynapointer+=1
#define DWRITEW(a) *((unsigned short *) dynapointer)=(a); dynapointer+=2
#define DWRITEL(a) *((unsigned long *) dynapointer)=(a); dynapointer+=4
#define FETCH (readb(PC++))

/*
 * dest=0xFF00;
 * dest&=src;
 * dest=dest>>8;
 */

#define GETUPPER(src,dest) \
    DYNA_MOVE_l_i_to_r(0xFF00,(dest)); \
    DYNA_AND_l_r_to_r((src),(dest)); \
    DYNA_ASHIFT_l(0,(dest),0,0)

/*
 * dest&=0xFF;
 * src&=0xFF;
 * src=src<<8;
 * dest|=src;
 */
#define PUTUPPER(src,dest) \
    DYNA_AND_l_i_to_r(0xFF,(dest)); \
    DYNA_AND_l_i_to_r(0xFF,(src)); \
    DYNA_ASHIFT_l(0,(src),1,0); \
    DYNA_OR_l_r_to_r((src),(dest))
/*
 * movea.l &cpu.a, %a3
 * movem.l d1-d7/a0-a1 , (%a3)
 * jsr (n)
 * movea.l &cpu.a, %a3
 * movem.l (%a3), d1-d7/a0-a1
 */
#define CALL_NATIVE(n) \
    DYNA_MOVEA_l_i_to_r(&cpu.a,3); \
    DYNA_MOVEM(3,0x3FE,0); \
    DYNA_JSR(&writehi); \
    DYNA_MOVEA_l_i_to_r(&cpu.a,3); \
    DYNA_MOVEM(3,0x3FE,1);
    
/*
 * SUBQ 2, %a0 // decrease gb sp
 * PEA #(n) // push n
 * PUSH %a0 // push gb sp
 * call_native writew(SP, (n)
 * ADDQ 8, %a7
 */
#define PUSH(n) \
    DYNA_SUBQ_l_i_to_r(2,0,1); \
    DYNA_PEA_w_i((n)); \
    DYNA_PUSH_l_r(0,1); \
    CALL_NATIVE(&writew); \
    DYNA_ADDQ_l_i_to_r(0,7,1);    
    
/*
 * PUSH %a0 // push gb sp
 * call_native readw(SP);
 * addq 4, a7
 * addq 2, a0
 * movea.w %d0, (n)
 */

#define POPA(n) \
    DYNA_PUSH_l_r(0,1); \
    CALL_NATIVE(&readw); \
    DYNA_ADDQ_l_i_to_r(4,7,1); \
    DYNA_ADDQ_l_i_to_r(2,0,1); \
    DYNA_MOVEA_w_r_to_r(0,(n),0);

#define ADD(n) \
    DYNA_MOVEQ_l_i_to_r(0x0,7); \
    DYNA_MOVE_l_r_to_r(1,0,0); \
    DYNA_ADD_l_r_to_r((n),1); \
    DYNA_XOR_l_r_to_r(1,0); \
    DYNA_XOR_l_r_to_r((n),0); \
    DYNA_LSHIFT_l(1,0,1,0); \
    DYNA_AND_l_i_to_r(0x20,0); \
    DYNA_OR_l_r_to_r(0,7); \
    DYNA_TST_b_r(1,0); \
    DYNA_SET_b_r(0,0x7); \
    DYNA_AND_l_i_to_r(0x80,0); \
    DYNA_OR_l_r_to_r(0,7); \
    DYNA_MOVE_l_r_to_r(1,0,0); \
    DYNA_LSHIFT_l(4,0,0,0); \
    DYNA_ANDI_l_i_to_r(0x10,0); \
    DYNA_OR_l_r_to_r(0,7); \
    DYNA_AND_l_i_to_r(0xFF,1);

#define SUBTRACT(n) \
    DYNA_MOVEQ_l_i_to_r(0x40,7); \
    DYNA_MOVE_l_r_to_r(1,0,0); \
    DYNA_SUB_l_r_to_r((n),1,0); \
    DYNA_XOR_l_r_to_r(1,0); \
    DYNA_XOR_l_r_to_r((n),0); \
    DYNA_LSHIFT_l(1,0,1,0); \
    DYNA_AND_l_i_to_r(0x20,0); \
    DYNA_OR_l_r_to_r(0,7); \
    DYNA_TST_b_r(1,0); \
    DYNA_SET_b_r(0,0x7); \
    DYNA_AND_l_i_to_r(0x80,0); \
    DYNA_OR_l_r_to_r(0,7); \
    DYNA_MOVE_l_r_to_r(1,0,0); \
    DYNA_LSHIFT_l(4,0,0,0); \
    DYNA_AND_l_i_to_r(0x10,0); \
    DYNA_OR_l_r_to_r(0,7);

#define CMP(n) \
    DYNA_MOVEA_l_r_to_r(1,3,0); \
    SUBTRACT((n)); \
    DYNA_MOVE_l_r_to_r(3,1,1);

#define SUB(n) \
    SUBTRACT((n)); \
    DYNA_ANDI_l_i_to_r(0xFF,1);


void DYNA_MOVE_b_r_to_r(un8 src,un8 dest) {
    DWRITEW(0x1000|(src&0x7)|(dest&0x7)<<9);
}

void DYNA_ASHIFT_l(un8 src, un8 dest, int left, int src_is_reg) {
    unsigned short opcode;
    opcode=(0xE080)|((src&0x7)<<9)|(dest&0x7);
    if(left)
      opcode|=0x100;
    if(src_is_reg)
      opcode|=0x20;
    DWRITEW(opcode);
}

void DYNA_LSHIFT_l(un8 src, un8 dest, int left, int src_is_reg) {
    unsigned short opcode=0xE088|((src&0x7)<<9)|(dest&0x7);
    if(left)
        opcode|=0x100;
    if(src_is_reg)
        opcode|=0x20;
    DWRITEW(opcode);
}

void DYNA_MOVE_l_i_to_r(un32 imm, un8 dest) {
    DWRITEW(0x203C|(dest&0x7)<<9);
    DWRITEL(imm); // endianness?
}

void DYNA_MOVE_l_i_to_m(un32 imm, un8 dest_a) {
    DWRITEW(0x20FC|((dest_a&0x7)<<9));
    DWRITEL(imm);
}

void DYNA_MOVE_l_r_to_m(un8 src,un8 dest_a) {
    DWRITEW(0x2080|(dest_a&0x7)<<9|(src&0x7));
}

void DYNA_MOVE_l_r_to_r(un8 src, un8 dest, int src_is_areg) {
    unsigned short opcode=0x2000|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        opcode|=0x8;
    DWRITEW(opcode);
}

void DYNA_MOVE_w_r_to_r(un8 src, un8 dest, int src_is_areg) {
    unsigned short opcode=0x3000|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        opcode|=0x8;
    DWRITEW(opcode);
}

void DYNA_MOVEQ_l_i_to_r(un8 imm, un8 dest) {
    DWRITEW(0x7000|((dest&0x7)<<9)|imm);
}

void DYNA_PEA_w_i(un16 imm) {
    DWRITEW(0x4878);
    DWRITEW(imm);
}

void DYNA_PUSH_l_r(un8 reg,int src_is_areg) {
    unsigned short value= 0x2F00|(reg&0x7);
    if(src_is_areg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_MOVEA_l_i_to_r(un32 imm, un8 dest) {
    DWRITEW(0x207C|(dest&0x7)<<9);
    DWRITEL(imm);
}

void DYNA_MOVEA_w_r_to_r(un8 src, un8 dest, int src_is_areg) {
    unsigned short value=0x3040|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_MOVEA_l_r_to_r(un8 src, un8 dest, int src_is_areg) {
    unsigned short value=0x2040|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        value|=0x8;
    DWRITEW(value);
}


void DYNA_RET() {
    DWRITEW(0x4E75);
}

void DYNA_AND_l_i_to_r(un32 imm, un8 dest) {
    DWRITEW(0x0280|(dest&0x7));
    DWRITEL(imm);
}

void DYNA_AND_l_r_to_r(un8 src,un8 dest) {
    DWRITEW(0xC080|((dest&0x7)<<9)|(src&0x7));
}

void DYNA_OR_l_r_to_r(un8 src,un8 dest) {
    DWRITEW(0x8080|((dest&0x7)<<9)|(src&0x7));
}

void DYNA_OR_l_i_to_r(un32 imm,un8 dest) {
    DWRITEW(0x0080|(dest&0x7));
    DWRITEL(imm);
}

void DYNA_CLR_l_m(un32 addr) {
    DWRITEW(0x42B9);
    DWRITEL(addr);
}

void DYNA_CLR_l_r(un8 reg) {
    DWRITEW(0x4280|(reg&0x7));
}

void DYNA_CLR_w_r(un8 reg) {
    DWRITEW(0x42C0|(reg&0x7));
}


void DYNA_CLR_b_r(un8 reg) {
    DWRITEW(0x4200|(reg&0x7));
}


void DYNA_ADDQ_l_i_to_r(un8 imm, un8 reg, int dest_is_a) {
    unsigned short value=0x5080|(imm&0x7)<<9|(reg&0x7);
    if(dest_is_a)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_ADD_l_r_to_r(un8 src, un8 dest, int src_is_areg) {
    unsigned short value=0xD080|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_JSR(void *addr) {
    DWRITEW(0x4EB9);
    DWRITEL(addr);
}

void DYNA_MOVEM(un8 areg,un16 mask, int frommem) {
    unsigned short value=0x48D0|(areg&0x7);
    if(frommem)
        value|=0x400;
    DWRITEW(value);
    DWRITEW(mask);
}

void DYNA_SUBQ_l_i_to_r(un8 imm, un8 reg, int addr_reg) {
    unsigned short value=0x5180|(reg&0x7)|((imm&0x7)<<9);
    if(addr_reg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_SUB_l_r_to_r(un8 src, un8 dest, int is_areg) {
    unsigned short value=0x9080|((dest&0x7)<<9)|(src&0x7);
    if(is_areg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_EXT_l(un8 reg) {
    DWRITEW(0x48C0|(reg&0x7));
}

void DYNA_BCC_c(un8 cond, int size,int i) {
    un32 displace=dynapointer-branchp[i];
    if(!branchp[i]) {
        die("Dynarec error! BCC trying to write branch without dummy");
        return;
    }
    if((size==2&&displace>0x7f) || (size==4 && displace>0x7FFF))
        die("Dynarec error! BCC invalid displacement");
    else if(displace>0&&displace<0x7F) 
        *( (unsigned short *) branchp[i])=0x6000|((cond&0xF)<<8)|(displace&0xFF);
    else if(displace>0x7F&&displace<0x7FFF) {
        *( (unsigned short *) branchp[i])=0x6000|((cond&0xF)<<8);
        branchp[i]+=2;
        *( (unsigned short *) branchp[i])=displace;
    }
    else
        die("Dynarec error! BCC invalid displacement");
        
    branchp[i]=0;
}

void DYNA_DUMMYBRANCH(int size,int i) {
    branchp[i]=dynapointer;
    dynapointer+=size;
}

void DYNA_TST_l_r(un8 reg,int is_areg) {
    unsigned short opcode=0x4A80|(reg&0x7);
    if(is_areg)
        opcode|=0x8;
    DWRITEW(opcode);
}

void DYNA_TST_b_r(un8 reg,int is_areg) {
    unsigned short opcode=0x4A00|(reg&0x7);
    if(is_areg)
        opcode|=0x8;
    DWRITEW(opcode);
}


void DYNA_BTST_l_r(un8 bit, un8 reg) {
    DWRITEW(0x0800|(reg&0x7));
    DWRITEW(bit);
}

void DYNA_NEG_l(un8 reg) {
    DWRITEW(0x4480|(reg&0x7));
}

void DYNA_XOR_l_r_to_r(un8 src, un8 dest) {
    DWRITEW(0xB180|((dest&0x7)<<9)|(src&0x7));
}

void DYNA_SET_b_r(un8 src, un8 cond) {
    DWRITEW(0x50C0|((cond)&0xF)<<8|(src&0x7));
}

void dynamic_recompile (struct dynarec_block *newblock) {
    int done=0;
    byte op;
    unsigned int oldpc=PC;
    unsigned short temp;
    unsigned int tclen=0,clen;
    char meow[500];
    dynapointer=malloc(512);
    newblock->block=dynapointer;

    snprintf(meow,499,"Recompiling 0x%x",oldpc);
    rb->splash(HZ*3,1,meow);
    while(!done) {
      op=FETCH;
      clen = cycles_table[op];
      tclen+=clen;       
      switch(op) {
          case 0x00: /* NOP */
          case 0x40: /* LD B,B */
          case 0x49: /* LD C,C */
          case 0x52: /* LD D,D */
          case 0x5B: /* LD E,E */
          case 0x64: /* LD H,H */
          case 0x6D: /* LD L,L */
          case 0x7F: /* LD A,A */
              break;
          case 0x41: /* LD B,C */
              DYNA_MOVE_b_r_to_r(3,2); 
              break;
          case 0x42: /* LD B,D */
              DYNA_MOVE_b_r_to_r(4,2);
              break;
          case 0x43: /* LD B,E */
              DYNA_MOVE_b_r_to_r(5,2);
              break;
          case 0x44: /* LD B,H */
              GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,2);
              break;
          case 0x45: /* LD B,L */
              DYNA_MOVE_b_r_to_r(6,2);
              break;
          case 0x47: /* LD B,A */
              DYNA_MOVE_b_r_to_r(1,2);
              break;
          case 0x48: /* LD C,B */
              DYNA_MOVE_b_r_to_r(2,3);
              break;
          case 0x4A: /* LD C,D */
              DYNA_MOVE_b_r_to_r(4,3);
              break;
          case 0x4B: /* LD C,E */
              DYNA_MOVE_b_r_to_r(5,3);
              break;
          case 0x4C: /* LD C,H */
              GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,3);
              break;
          case 0x4D: /* LD C,L */
              DYNA_MOVE_b_r_to_r(6,3);
              break;
          case 0x4F: /* LD C,A */
              DYNA_MOVE_b_r_to_r(1,3);
              break;
           
          case 0x50: /* LD D,B */
              DYNA_MOVE_b_r_to_r(2,4);
              break;
          case 0x51: /* LD D,C */
              DYNA_MOVE_b_r_to_r(3,4);
              break;
          case 0x53: /* LD D,E */
              DYNA_MOVE_b_r_to_r(5,4);
              break;
          case 0x54: /* LD D,H */
              GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,4);
              break;
          case 0x55: /* LD D,L */
              DYNA_MOVE_b_r_to_r(6,4);
              break;
          case 0x57: /* LD D,A */
              DYNA_MOVE_b_r_to_r(1,4);
              break;
         
          case 0x58: /* LD E,B */
              DYNA_MOVE_b_r_to_r(2,5);
              break;
          case 0x59: /* LD E,C */
              DYNA_MOVE_b_r_to_r(3,5);
              break;
          case 0x5A: /* LD E,D */
              DYNA_MOVE_b_r_to_r(4,5);
              break;
          case 0x5C: /* LD E,H */
              GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,5); 
              break;
          case 0x5D: /* LD E,L */
              DYNA_MOVE_b_r_to_r(6,5);
              break;
          case 0x5F: /* LD E,A */
              DYNA_MOVE_b_r_to_r(1,5); 
              break;
           
          case 0x60: /* LD H,B */
              DYNA_MOVE_b_r_to_r(2,0);
              PUTUPPER(0,6);
              break;
          case 0x61: /* LD H,C */
              DYNA_MOVE_b_r_to_r(3,0);
              PUTUPPER(0,6);
              break;
          case 0x62: /* LD H,D */
              DYNA_MOVE_b_r_to_r(4,0);
              PUTUPPER(0,6);
              break;
          case 0x63: /* LD H,E */
              DYNA_MOVE_b_r_to_r(5,0);
              PUTUPPER(0,6);
              break;
          case 0x65: /* LD H,L */
              DYNA_MOVE_b_r_to_r(6,0);
              PUTUPPER(0,6);
              break;
          case 0x67: /* LD H,A */
              DYNA_MOVE_b_r_to_r(1,0);
              PUTUPPER(0,6);
              break;
          case 0x68: /* LD L,B */
              DYNA_MOVE_b_r_to_r(2,6);
              break;
          case 0x69: /* LD L,C */
              DYNA_MOVE_b_r_to_r(3,6);
              break;
          case 0x6A: /* LD L,D */
              DYNA_MOVE_b_r_to_r(4,6);
              break;
          case 0x6B: /* LD L,E */
              DYNA_MOVE_b_r_to_r(5,6);
              break;
          case 0x6C: /* LD L,H */
              GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,6);
              break;
           
          case 0x78: /* LD A,B */
              DYNA_MOVE_b_r_to_r(2,1);
              break;
          case 0x79: /* LD A,C */
              DYNA_MOVE_b_r_to_r(3,1);
              break;
          case 0x7A: /* LD A,D */
              DYNA_MOVE_b_r_to_r(4,1);
              break;
          case 0x7B: /* LD A,E */
              DYNA_MOVE_b_r_to_r(5,1);
              break;
          case 0x7C: /* LD A,H */
              GETUPPER(5,0);
              DYNA_MOVE_b_r_to_r(0,1);
              break;
          case 0x7D: /* LD A,L */
              DYNA_MOVE_b_r_to_r(5,1);
              break;
          case 0x01: /* LD BC,imm */
              {   /* warning (do we have endianness right?) */
                  temp=readw(xPC);
                  DYNA_MOVEQ_l_i_to_r((temp&0xFF00)>>8,2);
                  DYNA_MOVEQ_l_i_to_r(temp&0xFF,3);
                  PC+=2;
              }
              break;
          case 0x11: /* LD DE,imm */
              { temp=readw(xPC);
                 DYNA_MOVEQ_l_i_to_r((temp&0xFF00)>>8,4);
                 DYNA_MOVEQ_l_i_to_r(temp&0xFF,5);
                 PC += 2;
              }  
              break;
          case 0x21: /* LD HL,imm */
              {
                 DYNA_MOVE_l_i_to_r(readw(xPC),6);
                 PC += 2;
              }
              break;
          case 0x31: /* LD SP,imm */
              DYNA_MOVEA_l_i_to_r(readw(xPC),0);
              PC += 2; 
              break;

          case 0x06: /* LD B,imm */
              DYNA_MOVEQ_l_i_to_r(FETCH,2);
              break;
          case 0x0E: /* LD C,imm */
              DYNA_MOVEQ_l_i_to_r(FETCH,3);
              break;
          case 0x16: /* LD D,imm */
              DYNA_MOVEQ_l_i_to_r(FETCH,4);
              break;
          case 0x1E: /* LD E,imm */
              DYNA_MOVEQ_l_i_to_r(FETCH,5);
              break;
          case 0x26: /* LD H,imm */
              DYNA_AND_l_i_to_r(0xFF,6);
              DYNA_OR_l_i_to_r(FETCH<<8,6);
              break;
          case 0x2E: /* LD L,imm */
              DYNA_AND_l_i_to_r(0xFF00,6);
              DYNA_OR_l_i_to_r(FETCH,6);
              break;
          case 0x3E: /* LD A,imm */
              DYNA_MOVEQ_l_i_to_r(FETCH,1);
              break;

          case 0xF9: /* LD SP,HL */
              DYNA_MOVEA_w_r_to_r(6,0,0);
              break;     
          case 0xF3: /* DI */
              DYNA_CLR_l_m(&cpu.ime);
              DYNA_CLR_l_m(&cpu.ima);
              DYNA_CLR_l_m(&cpu.halt);
              /* cpu.halt = cpu.ima = cpu.ime = 0; */
              break;
          case 0xFB: /* EI */
              DYNA_MOVEQ_l_i_to_r(1,0);
              DYNA_MOVEA_l_i_to_r(&cpu.ima,3);
              DYNA_MOVE_l_r_to_m(0,3);
              /*cpu.ima=1; */
              break;
           
          case 0xE0: /* LDH (imm),A */
              DYNA_PUSH_l_r(1,0);
              DYNA_PEA_w_i(FETCH);
              CALL_NATIVE(&writehi);
              DYNA_ADDQ_l_i_to_r(0,7,1);
              /*writehi(FETCH, A); */
              break;

          case 0xC3: /* JP (imm) PC = readw(PC) */
              PC=readw(PC);
              done=1;
              break;
          case 0xCD: /* CALL (imm) PUSH(PC+2) PC=readw(PC); */
              PUSH(PC+2);
              PC=readw(PC);
              done=1;
              break;
        
          case 0x97: /* SUB A (halfcarry ?) */
              DYNA_CLR_l_r(1);
              DYNA_MOVEQ_l_i_to_r(0xC0,7);
              break;
          case 0xF0:
              DYNA_PEA_w_i(FETCH);
              CALL_NATIVE(&readhi);
              DYNA_ADDQ_l_i_to_r(4,7,1);
              DYNA_MOVE_b_r_to_r(0,1);
              /*A = readhi(FETCH)*/
              break;
          case 0x87:  // ADD A,A
            /*         code taken from gcc -O3 output by compiling;
             *         c=(2*b)&0xFF;
             *         a=(c) ? 0 : 0x80 | // zero flag
             *            (0x20 & (c) << 1) | // halfcarry
             *            ((2*b)&0x100)>>4;    // carry
	     */
            DYNA_MOVE_l_r_to_r(1,0,0); /* move.l d1, d0 */
            DYNA_ADD_l_r_to_r(0,0,0); /* add.l d0, d0 */
            DYNA_AND_l_i_to_r(510,0); /* and.l #510, d0 */
            DYNA_MOVEA_l_r_to_r(0,3,0); /* movea.l d0,a3 */
            DYNA_CLR_b_r(7); /* clr.b d7 */ 
            DYNA_TST_b_r(0,0); /* tst.b d0 */
            DYNA_DUMMYBRANCH(2,0); 
            DYNA_MOVE_l_r_to_r(1,0,0); /* move.l d1,d0 */
            DYNA_LSHIFT_l(3,0,0,0); /* lsr.l #3, d0 */
            DYNA_MOVEQ_l_i_to_r(16,1); /* moveq #16,d1 */
            DYNA_AND_l_r_to_r(1,0); /* and.l d1 d0 */
            DYNA_MOVEQ_l_i_to_r(0x80,7); /* moveq #0x80,d7 */
            DYNA_OR_l_r_to_r(0,7); /* or.l d0, d7 */
            DYNA_BCC_c(0x6,2,0); /* branch not equal, here */
            DYNA_MOVE_l_r_to_r(3,1,1); /* move.l a3,d1 */
            DYNA_AND_l_i_to_r(0xFE,1); /* and.l #0xFE,d1 */
            DYNA_AND_l_i_to_r(0xB0,7); /* and.l #0xB0,d7  */
            break;
        case 0xD0: /* RET NC */
            DYNA_BTST_l_r(5,7); /* btst #5,d7 */
            DYNA_DUMMYBRANCH(2,0);
            POPA(1); /* POP %a1 */
            DYNA_MOVEA_l_i_to_r(&blockclen,3);
            DYNA_MOVE_l_i_to_m(tclen,3);
            DYNA_RET();
            DYNA_BCC_c(0x6,2,0); /* jump here if not zero (not zero = C) */
            tclen-=3;
            break;
	case 0xFE: /* CMP #<imm> TODO: can be (much) more efficient.*/
            DYNA_MOVEA_l_r_to_r(2,3,0); /* movea.l %d2, %a3 */
            DYNA_MOVEQ_l_i_to_r(FETCH,2); /* moveq.l #<FETCH>,%d2 */
            CMP(2);
            DYNA_MOVE_l_r_to_r(3,2,1); /* move.l %a3, %d2 */
            break;
            
        default:
           snprintf(meow,499,"unimplemented opcode %d / 0x%x",op,op);
           die(meow);
           return;
           break;
      }  
    }
    snprintf(meow,499,"end of block, pc:0x%x",PC);
    rb->splash(HZ*2,true,meow);
    DYNA_MOVEA_l_i_to_r(&blockclen,3);
    DYNA_MOVE_l_i_to_m(tclen,3);
    DYNA_MOVEA_l_i_to_r(PC,1);
    DYNA_RET();
    PC=oldpc;
    setmallocpos(dynapointer);
    newblock->length=dynapointer-newblock->block;
    invalidate_icache();
}
#endif
