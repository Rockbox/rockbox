#ifdef DYNAREC
#include <system.h>
#include "rockmacros.h"
#include "defs.h"
#include "regs.h"
#include "hw.h"
#include "cpu-gb.h"
#include "lcdc.h"
#include "mem.h"
#include "fastmem.h"
#include "cpuregs.h"
#include "cpucore.h"

void *dynapointer,*branchp[10];
int blockclen;

#define DYNA_DEBUG 1

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
    DYNA_JSR((n)); \
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

void DYNA_INC_l_r(un8 dest,int is_areg) {
    DYNA_ADDQ_l_i_to_r(1,dest,is_areg);
}

void DYNA_DEC_l_r(un8 dest,int is_areg) {
    DYNA_SUBQ_l_i_to_r(1,dest,is_areg);
}


void dynamic_recompile (struct dynarec_block *newblock) {
    int done=0,writepc=1,fd;
    byte op;
    unsigned int oldpc=PC;
    unsigned short temp;
    unsigned int tclen=0,clen;
    char meow[500];
    dynapointer=malloc(512);
    newblock->block=dynapointer;
#ifdef DYNA_DEBUG
    snprintf(meow,499,"/dyna_0x%x_asm.rb",PC);
    fd=open(meow,O_WRONLY|O_CREAT|O_TRUNC);
    if(fd<0) {
	die("couldn't open dyna debug file");
        return;
    }
#endif
    snprintf(meow,499,"Recompiling 0x%x",oldpc);
    rb->splash(HZ*1,1,meow);
    while(!done) {
#ifdef DYNA_DEBUG
      fdprintf(fd,"0x%x: ",PC);
#endif
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

	  case 0x0B: /* DEC BC*
#ifdef DYNA_DEBUG
	      fdprintf(fd,"DEC BC\n");
#endif
	      DYNA_TST_b_r(3); // test C
	      DYNA_DUMMYBRANCH(2,0);
	      DYNA_DEC_l_r(2,0); // dec B
	      DYNA_BCC_c(0x6,2,0); //jump here if not zero
	      DYNA_DEC_l_r(3,0); // dec C
	      break;
          case 0x41: /* LD B,C */
#ifdef DYNA_DEBUG
	      fdprintf(fd,"LD B,C\n");
#endif
              DYNA_MOVE_b_r_to_r(3,2); 
              break;
          case 0x42: /* LD B,D */
#ifdef DYNA_DEBUG
	      fdprintf(fd,"LD B,D\n");
#endif
              DYNA_MOVE_b_r_to_r(4,2);
              break;
          case 0x43: /* LD B,E */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD B,E\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(5,2);
              break;
          case 0x44: /* LD B,H */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD B,H\n"); 
#endif
	      GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,2);
              break;
          case 0x45: /* LD B,L */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD B,L\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(6,2);
              break;
          case 0x47: /* LD B,A */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD B,A\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(1,2);
              break;
          case 0x48: /* LD C,B */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD C,B\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(2,3);
              break;
          case 0x4A: /* LD C,D */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD C,D\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(4,3);
              break;
          case 0x4B: /* LD C,E */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD C,E\n");
#endif
	      DYNA_MOVE_b_r_to_r(5,3);
              break;
          case 0x4C: /* LD C,H */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD C,H\n"); 
#endif
	      GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,3);
              break;
          case 0x4D: /* LD C,L */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD C,L\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(6,3);
              break;
          case 0x4F: /* LD C,A */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD C,A\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(1,3);
              break;
           
          case 0x50: /* LD D,B */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD D,B\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(2,4);
              break;
          case 0x51: /* LD D,C */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD D,C\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(3,4);
              break;
          case 0x53: /* LD D,E */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD D,E\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(5,4);
              break;
          case 0x54: /* LD D,H */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD D,H\n"); 
#endif
	      GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,4);
              break;
          case 0x55: /* LD D,L */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD D,L\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(6,4);
              break;
          case 0x57: /* LD D,A */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD D,A\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(1,4);
              break;
         
          case 0x58: /* LD E,B */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD E,B\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(2,5);
              break;
          case 0x59: /* LD E,C */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD E,C\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(3,5);
              break;
          case 0x5A: /* LD E,D */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD E,D\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(4,5);
              break;
          case 0x5C: /* LD E,H */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD E,H\n"); 
#endif
	      GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,5); 
              break;
          case 0x5D: /* LD E,L */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD E,L\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(6,5);
              break;
          case 0x5F: /* LD E,A */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD E,A\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(1,5); 
              break;
           
          case 0x60: /* LD H,B */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD H,B\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(2,0);
              PUTUPPER(0,6);
              break;
          case 0x61: /* LD H,C */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD H,C\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(3,0);
              PUTUPPER(0,6);
              break;
          case 0x62: /* LD H,D */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD H,D\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(4,0);
              PUTUPPER(0,6);
              break;
          case 0x63: /* LD H,E */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD H,E\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(5,0);
              PUTUPPER(0,6);
              break;
          case 0x65: /* LD H,L */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD H,L\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(6,0);
              PUTUPPER(0,6);
              break;
          case 0x67: /* LD H,A */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD H,A\n"); 
#endif	      
              DYNA_MOVE_b_r_to_r(1,0);
              PUTUPPER(0,6);
              break;
          case 0x68: /* LD L,B */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD L,B\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(2,6);
              break;
          case 0x69: /* LD L,C */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD L,C\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(3,6);
              break;
          case 0x6A: /* LD L,D */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD L,D\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(4,6);
              break;
          case 0x6B: /* LD L,E */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD L,E\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(5,6);
              break;
          case 0x6C: /* LD L,H */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD L,H\n"); 
#endif
	      GETUPPER(6,0);
              DYNA_MOVE_b_r_to_r(0,6);
              break;
           
          case 0x78: /* LD A,B */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD A,B\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(2,1);
              break;
          case 0x79: /* LD A,C */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD A,C\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(3,1);
              break;
          case 0x7A: /* LD A,D */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD A,D\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(4,1);
              break;
          case 0x7B: /* LD A,E */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD A,E\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(5,1);
              break;
          case 0x7C: /* LD A,H */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD A,H\n"); 
#endif
	      GETUPPER(5,0);
              DYNA_MOVE_b_r_to_r(0,1);
              break;
	  case 0x7D: /* LD A,L */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD A,L\n"); 
#endif
	      DYNA_MOVE_b_r_to_r(5,1);
              break;
          case 0x01: /* LD BC,imm */
              {   /* warning (do we have endianness right?) */
#ifdef DYNA_DEBUG 
		  fdprintf(fd,"LD BC,#0x%x\n",readw(xPC)); 
#endif		      
		  temp=readw(xPC);
                  DYNA_MOVEQ_l_i_to_r((temp&0xFF00)>>8,2);
                  DYNA_MOVEQ_l_i_to_r(temp&0xFF,3);
                  PC+=2;
              }
              break;
          case 0x11: /* LD DE,imm */
              { 
#ifdef DYNA_DEBUG
		 fdprintf(fd,"LD DE,#0x%x\n",readw(xPC));
#endif
		 temp=readw(xPC);
                 DYNA_MOVEQ_l_i_to_r((temp&0xFF00)>>8,4);
                 DYNA_MOVEQ_l_i_to_r(temp&0xFF,5);
                 PC += 2;
              }  
              break;
          case 0x21: /* LD HL,imm */
              {
#ifdef DYNA_DEBUG
                 fdprintf(fd,"LD HL,#0x%x\n",readw(xPC));
#endif
                 DYNA_MOVE_l_i_to_r(readw(xPC),6);
                 PC += 2;
              }
              break;
	  case 0x22: /* LDI (HL), A */
#ifdef DYNA_DEBUG
	      fdprintf(fd,"LDI (HL),A\n");
#endif
/*              DYNA_PUSH_l_r(1,0);
              DYNA_PUSH_l_r(6,0);
              CALL_NATIVE(&writeb);
              DYNA_ADDQ_l_i_to_r(0,7,1);
	      DYNA_INC_l_r(6,0);*/
	      break;
          case 0x31: /* LD SP,imm */
#ifdef DYNA_DEBUG
	      fdprintf(fd,"LD SP,#0x%x\n",readw(xPC));
#endif
              DYNA_MOVEA_l_i_to_r(readw(xPC),0);
              PC += 2; 
              break;

          case 0x06: /* LD B,imm */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD B,#0x%x\n",readb(xPC)); 
#endif
              DYNA_MOVEQ_l_i_to_r(FETCH,2);
              break;
          case 0x0E: /* LD C,imm */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD C,#0x%x\n",readb(xPC)); 
#endif
	      DYNA_MOVEQ_l_i_to_r(FETCH,3);
              break;
          case 0x16: /* LD D,imm */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD D,#0x%x\n",readb(xPC)); 
#endif
	      DYNA_MOVEQ_l_i_to_r(FETCH,4);
              break;
          case 0x1E: /* LD E,imm */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD E,#0x%x\n",readb(xPC)); 
#endif
	      DYNA_MOVEQ_l_i_to_r(FETCH,5);
              break;
          case 0x26: /* LD H,imm */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD H,#0x%x\n",readb(xPC)); 
#endif
	      DYNA_AND_l_i_to_r(0xFF,6);
              DYNA_OR_l_i_to_r(FETCH<<8,6);
              break;
          case 0x2E: /* LD L,imm */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD L,#0x%x\n",readb(xPC)); 
#endif
	      DYNA_AND_l_i_to_r(0xFF00,6);
              DYNA_OR_l_i_to_r(FETCH,6);
              break;
          case 0x3E: /* LD A,imm */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD A,#0x%x\n",readb(xPC)); 
#endif
	      DYNA_MOVEQ_l_i_to_r(FETCH,1);
              break;

          case 0xF9: /* LD SP,HL */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD SP,HL\n"); 
#endif
	      DYNA_MOVEA_w_r_to_r(6,0,0);
              break;     
          case 0xF3: /* DI */
#ifdef DYNA_DEBUG
	      fdprintf(fd,"DI\n");
#endif
              DYNA_CLR_l_m(&cpu.ime);
              DYNA_CLR_l_m(&cpu.ima);
              DYNA_CLR_l_m(&cpu.halt);
              /* cpu.halt = cpu.ima = cpu.ime = 0; */
              break;
          case 0xFB: /* EI */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"EI\n"); 
#endif
              DYNA_MOVEQ_l_i_to_r(1,0);
              DYNA_MOVEA_l_i_to_r(&cpu.ima,3);
              DYNA_MOVE_l_r_to_m(0,3);
              /*cpu.ima=1; */
              break;
           
          case 0xE0: /* LDH (imm),A */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"LD (0x%x),A\n",readb(xPC)); 
#endif
	      DYNA_PUSH_l_r(1,0);
              DYNA_PEA_w_i(FETCH);
              CALL_NATIVE(&writehi);
              DYNA_ADDQ_l_i_to_r(0,7,1);
              /*writehi(FETCH, A); */
              break;

          case 0xC3: /* JP (imm) PC = readw(PC) */
#ifdef DYNA_DEBUG 
	      fdprintf(fd,"JP (0x%x)\n",readw(xPC)); 
#endif
              PC=readw(PC);
              done=1;
              break;
          case 0xCD: /* CALL (imm) PUSH(PC+2) PC=readw(PC); */
#ifdef DYNA_DEBUG
	      fdprintf(fd,"CALL (0x%x)\n",readw(xPC));
#endif
              PUSH(PC+2);
              PC=readw(PC);
              done=1;
              break;
        
          case 0x97: /* SUB A (halfcarry ?) */
#ifdef DYNA_DEBUG
	      fdprintf(fd,"SUB A\n");
#endif
              DYNA_CLR_l_r(1);
              DYNA_MOVEQ_l_i_to_r(0xC0,7);
              break;
          case 0xF0: /* LDH A,(imm) */
#ifdef DYNA_DEBUG
	      fdprintf(fd,"LDH A,(0x%x)\n",readb(xPC));
#endif
              DYNA_PEA_w_i(FETCH);
              CALL_NATIVE(&readhi);
              DYNA_ADDQ_l_i_to_r(4,7,1);
              DYNA_MOVE_b_r_to_r(0,1);
              /*A = readhi(FETCH)*/
              break;
          case 0x87:  // ADD A,A
#ifdef DYNA_DEBUG
	      fdprintf(fd,"ADD A,A\n");
#endif
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
#ifdef DYNA_DEBUG 
	    fdprintf(fd,"RET NC\n"); 
#endif
            DYNA_BTST_l_r(5,7); /* btst #5,d7 */
            DYNA_DUMMYBRANCH(2,0);
            POPA(1); /* POP %a1 */
            DYNA_MOVEA_l_i_to_r(&blockclen,3);
            DYNA_MOVE_l_i_to_m(tclen,3);
            DYNA_RET();
            DYNA_BCC_c(0x6,2,0); /* jump here if bit is not zero */
            tclen-=3;
            break;
	case 0xC9: /* RET */
#ifdef DYNA_DEBUG 
	    fdprintf(fd,"RET\n"); 
#endif
	    POPA(1);
	    writepc=0;
	    done=1;
	    break;
	case 0x20: /* JR NZ */
#ifdef DYNA_DEBUG 
	    fdprintf(fd,"JR NZ\n"); 
#endif
	    DYNA_BTST_l_r(8,7); /* btst #8,d7 */
	    DYNA_DUMMYBRANCH(2,0);
	    DYNA_MOVEA_l_i_to_r(&blockclen,3);
	    DYNA_MOVE_l_i_to_m(tclen,3);
	    DYNA_MOVEA_l_i_to_r(PC+1+(signed char)readb(PC),1);
	    DYNA_RET();
	    DYNA_BCC_c(0x6,2,0); /* jump here if bit is not zero */
	    tclen--;
	    PC++;
	    break;
	case 0xC2: /* JP NZ */
#ifdef DYNA_DEBUG 
	    fdprintf(fd,"JP NZ\n"); 
#endif
	    DYNA_BTST_l_r(8,7); /* btst #8,d7 */
	    DYNA_DUMMYBRANCH(2,0);
	    DYNA_MOVEA_l_i_to_r(&blockclen,3);
	    DYNA_MOVE_l_i_to_m(tclen,3);
	    DYNA_MOVEA_l_i_to_r(readw(PC),1);
	    DYNA_RET();
	    DYNA_BCC_c(0x6,2,0); /* jump here if bit is not zero */
	    tclen--;
	    PC+=2;
	    break;
/*	case 0xFA: /* LD A, (imm) 
#ifdef DYNA_DEBUG 
	    fdprintf(fd,"LD A,(0x%x)\n",readw(xPC)); 
#endif
	    DYNA_PEA_w_i(readw(xPC));
	    PC+=2; \
	    CALL_NATIVE(&readb); \
	    DYNA_ADDQ_l_i_to_r(4,7,1); \
	    DYNA_MOVE_l_r_to_r(0,1,0);
	    break; */
		
	case 0xFE: /* CMP #<imm> TODO: can be (much) more efficient.*/
#ifdef DYNA_DEBUG
	    fdprintf(fd,"CMP #0x%x\n",readb(xPC));
#endif
	    DYNA_MOVEA_l_r_to_r(2,3,0); /* movea.l %d2, %a3 */
            DYNA_MOVEQ_l_i_to_r(FETCH,2); /* moveq.l #<FETCH>,%d2 */
            CMP(2);
            DYNA_MOVE_l_r_to_r(3,2,1); /* move.l %a3, %d2 */
            break;

	case 0xB1: /* OR C */
#ifdef DYNA_DEBUG 
	    fdprintf(fd,"OR C\n"); 
#endif
	    DYNA_OR_l_r_to_r(3,1); // or %d3,%d1
            DYNA_MOVEQ_l_i_to_r(0,7); 
            DYNA_TST_b_r(1,0);
	    DYNA_DUMMYBRANCH(2,0);
	    DYNA_MOVEQ_l_i_to_r(0x80,7);
	    DYNA_BCC_c(0x6,2,0);
            break;
        default:
           snprintf(meow,499,"unimplemented opcode %d / 0x%x",op,op);
           die(meow);
           return;
           break;
      }  
    }
#ifdef DYNA_DEBUG 
    fdprintf(fd,"(End of Block)\n"); 
    close(fd);
#endif    
    DYNA_MOVEA_l_i_to_r(&blockclen,3);
    DYNA_MOVE_l_i_to_m(tclen,3);
    if(writepc)
      DYNA_MOVEA_l_i_to_r(PC,1);
    DYNA_RET();
    PC=oldpc;
    setmallocpos(dynapointer);
    newblock->length=dynapointer-newblock->block;
    invalidate_icache();
    snprintf(meow,499,"/dyna_0x%x_code.rb",PC);
    fd=open(meow,O_WRONLY|O_CREAT|O_TRUNC);
    if(fd>=0) {
        write(fd,newblock->block,newblock->length);
	close(fd);
    }
}
#endif
