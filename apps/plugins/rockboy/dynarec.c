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
 * SUBQ 2, %a0                 decrease gb sp
 * PEA #(n)                    push n
 * PUSH %a0                    push gb sp
 * call_native writew(SP, (n))
 * ADDQ 8, %a7
 */
#define PUSH(n) \
    DYNA_SUBQ_l_i_to_r(2,0,1); \
    DYNA_PEA_w_i((n)); \
    DYNA_PUSH_l_r(0,1); \
    CALL_NATIVE(&writew); \
    DYNA_ADDQ_l_i_to_r(0,7,1);    
    
/*
 * PUSH %a0                push gb sp
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


void DYNA_MOVE_b_r_to_r(un8 src,un8 dest)
{
    DWRITEW(0x1000|(src&0x7)|(dest&0x7)<<9);
}

void DYNA_ASHIFT_l(un8 src, un8 dest, int left, int src_is_reg)
{
    unsigned short opcode;
    opcode=(0xE080)|((src&0x7)<<9)|(dest&0x7);
    if(left)
        opcode|=0x100;
    if(src_is_reg)
        opcode|=0x20;
    DWRITEW(opcode);
}

void DYNA_LSHIFT_l(un8 src, un8 dest, int left, int src_is_reg)
{
    unsigned short opcode=0xE088|((src&0x7)<<9)|(dest&0x7);
    if(left)
        opcode|=0x100;
    if(src_is_reg)
        opcode|=0x20;
    DWRITEW(opcode);
}

void DYNA_MOVE_l_i_to_r(un32 imm, un8 dest)
{
    DWRITEW(0x203C|(dest&0x7)<<9);
    DWRITEL(imm); /* endianness? */
}

void DYNA_MOVE_l_i_to_m(un32 imm, un8 dest_a)
{
    DWRITEW(0x20FC|((dest_a&0x7)<<9));
    DWRITEL(imm);
}

void DYNA_MOVE_l_r_to_m(un8 src,un8 dest_a)
{
    DWRITEW(0x2080|(dest_a&0x7)<<9|(src&0x7));
}

void DYNA_MOVE_l_r_to_r(un8 src, un8 dest, int src_is_areg)
{
    unsigned short opcode=0x2000|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        opcode|=0x8;
    DWRITEW(opcode);
}

void DYNA_MOVE_w_r_to_r(un8 src, un8 dest, int src_is_areg)
{
    unsigned short opcode=0x3000|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        opcode|=0x8;
    DWRITEW(opcode);
}

void DYNA_MOVEQ_l_i_to_r(un8 imm, un8 dest)
{
    DWRITEW(0x7000|((dest&0x7)<<9)|imm);
}

void DYNA_PEA_w_i(un16 imm)
{
    DWRITEW(0x4878);
    DWRITEW(imm);
}

void DYNA_PUSH_l_r(un8 reg,int src_is_areg)
{
    unsigned short value= 0x2F00|(reg&0x7);
    if(src_is_areg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_MOVEA_l_i_to_r(un32 imm, un8 dest)
{
    DWRITEW(0x207C|(dest&0x7)<<9);
    DWRITEL(imm);
}

void DYNA_MOVEA_w_r_to_r(un8 src, un8 dest, int src_is_areg)
{
    unsigned short value=0x3040|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_MOVEA_l_r_to_r(un8 src, un8 dest, int src_is_areg)
{
    unsigned short value=0x2040|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        value|=0x8;
    DWRITEW(value);
}


void DYNA_RET(void)
{
    DWRITEW(0x4E75);
}

void DYNA_AND_l_i_to_r(un32 imm, un8 dest)
{
    DWRITEW(0x0280|(dest&0x7));
    DWRITEL(imm);
}

void DYNA_AND_l_r_to_r(un8 src,un8 dest)
{
    DWRITEW(0xC080|((dest&0x7)<<9)|(src&0x7));
}

void DYNA_OR_l_r_to_r(un8 src,un8 dest)
{
    DWRITEW(0x8080|((dest&0x7)<<9)|(src&0x7));
}

void DYNA_OR_l_i_to_r(un32 imm,un8 dest)
{
    DWRITEW(0x0080|(dest&0x7));
    DWRITEL(imm);
}

void DYNA_CLR_l_m(un32 addr)
{
    DWRITEW(0x42B9);
    DWRITEL(addr);
}

void DYNA_CLR_l_r(un8 reg)
{
    DWRITEW(0x4280|(reg&0x7));
}

void DYNA_CLR_w_r(un8 reg)
{
    DWRITEW(0x42C0|(reg&0x7));
}


void DYNA_CLR_b_r(un8 reg)
{
    DWRITEW(0x4200|(reg&0x7));
}


void DYNA_ADDQ_l_i_to_r(un8 imm, un8 reg, int dest_is_a)
{
    unsigned short value=0x5080|(imm&0x7)<<9|(reg&0x7);
    if(dest_is_a)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_ADD_l_r_to_r(un8 src, un8 dest, int src_is_areg)
{
    unsigned short value=0xD080|((dest&0x7)<<9)|(src&0x7);
    if(src_is_areg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_JSR(void *addr)
{
    DWRITEW(0x4EB9);
    DWRITEL(addr);
}

void DYNA_MOVEM(un8 areg,un16 mask, int frommem)
{
    unsigned short value=0x48D0|(areg&0x7);
    if(frommem)
        value|=0x400;
    DWRITEW(value);
    DWRITEW(mask);
}

void DYNA_SUBQ_l_i_to_r(un8 imm, un8 reg, int addr_reg)
{
    unsigned short value=0x5180|(reg&0x7)|((imm&0x7)<<9);
    if(addr_reg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_SUB_l_r_to_r(un8 src, un8 dest, int is_areg)
{
    unsigned short value=0x9080|((dest&0x7)<<9)|(src&0x7);
    if(is_areg)
        value|=0x8;
    DWRITEW(value);
}

void DYNA_EXT_l(un8 reg)
{
    DWRITEW(0x48C0|(reg&0x7));
}

void DYNA_BCC_c(un8 cond, int size,int i)
{
    un32 displace=dynapointer-branchp[i];
    if(!branchp[i])
    {
        die("Dynarec error! BCC trying to write branch without dummy");
        return;
    }
    if((size==2&&displace>0x7f) || (size==4 && displace>0x7FFF))
        die("Dynarec error! BCC invalid displacement");
    else if(displace>0&&displace<0x7F) 
        *( (unsigned short *) branchp[i])=0x6000|((cond&0xF)<<8)|(displace&0xFF);
    else if(displace>0x7F&&displace<0x7FFF)
    {
        *( (unsigned short *) branchp[i])=0x6000|((cond&0xF)<<8);
        branchp[i]+=2;
        *( (unsigned short *) branchp[i])=displace;
    }
    else
        die("Dynarec error! BCC invalid displacement");
        
    branchp[i]=0;
}

void DYNA_DUMMYBRANCH(int size,int i)
{
    branchp[i]=dynapointer;
    dynapointer+=size;
}

void DYNA_TST_l_r(un8 reg,int is_areg)
{
    unsigned short opcode=0x4A80|(reg&0x7);
    if(is_areg)
        opcode|=0x8;
    DWRITEW(opcode);
}

void DYNA_TST_b_r(un8 reg,int is_areg)
{
    unsigned short opcode=0x4A00|(reg&0x7);
    if(is_areg)
        opcode|=0x8;
    DWRITEW(opcode);
}


void DYNA_BTST_l_r(un8 bit, un8 reg)
{
    DWRITEW(0x0800|(reg&0x7));
    DWRITEW(bit);
}

void DYNA_NEG_l(un8 reg)
{
    DWRITEW(0x4480|(reg&0x7));
}

void DYNA_XOR_l_r_to_r(un8 src, un8 dest)
{
    DWRITEW(0xB180|((dest&0x7)<<9)|(src&0x7));
}

void DYNA_SET_b_r(un8 src, un8 cond)
{
    DWRITEW(0x50C0|((cond)&0xF)<<8|(src&0x7));
}

void DYNA_INC_l_r(un8 dest,int is_areg)
{
    DYNA_ADDQ_l_i_to_r(1,dest,is_areg);
}

void DYNA_DEC_l_r(un8 dest,int is_areg)
{
    DYNA_SUBQ_l_i_to_r(1,dest,is_areg);
}


void dynamic_recompile (struct dynarec_block *newblock)
{
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
    if(fd<0)
    {
        die("couldn't open dyna debug file");
        return;
    }
#endif
    snprintf(meow,499,"Recompiling 0x%x",oldpc);
    rb->splash(HZ*1,meow);
    while(!done)
    {
#ifdef DYNA_DEBUG
        fdprintf(fd,"0x%x: ",PC);
#endif
        op=FETCH;
        clen = cycles_table[op];
        tclen+=clen;
        switch(op) {
            case 0x00: /* NOP */
                break;
            case 0x01: /* LD BC,imm */
                /* warning (do we have endianness right?) */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD BC,#0x%x\n",readw(xPC)); 
#endif              
                temp=readw(xPC);
                DYNA_MOVEQ_l_i_to_r((temp&0xFF00)>>8,2);
                DYNA_MOVEQ_l_i_to_r(temp&0xFF,3);
                PC+=2;
                break;
            case 0x02: /* LD (BC),A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (BC),A\n");
#endif
                break; /* FIXME: Implement */
            case 0x03: /* INC BC */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC BC\n");
#endif
                break; /* FIXME: Implement */
            case 0x04: /* INC B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC B\n");
#endif
                DYNA_INC_l_r(2,0); /* Is this right? */
                break;
            case 0x05: /* DEC B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC B\n");
#endif
                DYNA_DEC_l_r(2,0); /* Is this right? */
                break;
            case 0x06: /* LD B,imm */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD B,#0x%x\n",readb(xPC)); 
#endif
                DYNA_MOVEQ_l_i_to_r(FETCH,2);
                break;
            case 0x07: /* RLCA */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RLCA\n");
#endif
                break; /* FIXME: Implement */
            case 0x08: /* LD imm,SP */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD imm,SP\n");
#endif
                break; /* FIXME: Implement */
            case 0x09: /* ADD HL,BC */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD HL,BC\n");
#endif
                break; /* FIXME: Implement */
            case 0x0A: /* LD A,(BC) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD A,(BC)\n");
#endif
                break; /* FIXME: Implement */
            case 0x0B: /* DEC BC */
#ifdef DYNA_DEBUG
                fdprintf(fd,"DEC BC\n");
#endif
                DYNA_TST_b_r(3,0); /* test C */
                DYNA_DUMMYBRANCH(2,0);
                DYNA_DEC_l_r(2,0); /* dec B */
                DYNA_BCC_c(0x6,2,0); /* jump here if not zero */
                DYNA_DEC_l_r(3,0); /* dec C */
                break;
            case 0x0C: /* INC C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC C\n");
#endif
                DYNA_INC_l_r(3,0); /* Is this right? */
                break;
            case 0x0D: /* DEC C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC C\n");
#endif
                DYNA_DEC_l_r(3,0);  /* Is this right? */
                break;
            case 0x0E: /* LD C,imm */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD C,#0x%x\n",readb(xPC)); 
#endif
                DYNA_MOVEQ_l_i_to_r(FETCH,3);
                break;
            case 0x0F: /* RRCA */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RRCA\n");
#endif
                break; /* FIXME: Implement */
            case 0x10: /* STOP */
#ifdef DYNA_DEBUG
                fdprintf(fd, "STOP\n");
#endif
                break; /* FIXME: Implement */
            case 0x11: /* LD DE,imm */
#ifdef DYNA_DEBUG
                fdprintf(fd,"LD DE,#0x%x\n",readw(xPC));
#endif
                temp=readw(xPC);
                DYNA_MOVEQ_l_i_to_r((temp&0xFF00)>>8,4);
                DYNA_MOVEQ_l_i_to_r(temp&0xFF,5);
                PC += 2;
                break;
            case 0x12: /* LD (DE),A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (DE),A\n");
#endif
                break; /* FIXME: Implement */
            case 0x13: /* INC DE */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC DE\n");
#endif
                break; /* FIXME: Implement */
            case 0x14: /* INC D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC D\n");
#endif
                DYNA_INC_l_r(4,0); /* Is this right? */
                break;
            case 0x15: /* DEC D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC D\n");
#endif
                DYNA_DEC_l_r(4,0);  /* Is this right? */
                break;
            case 0x16: /* LD D,imm */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD D,#0x%x\n",readb(xPC)); 
#endif
                DYNA_MOVEQ_l_i_to_r(FETCH,4);
                break;
            case 0x17: /* RLA */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RLA\n");
#endif
                break; /* FIXME: Implement */
            case 0x18: /* JR */
#ifdef DYNA_DEBUG
                fdprintf(fd, "JR\n");
#endif
                break; /* FIXME: Implement */
            case 0x19: /* ADD HL,DE */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD HL,DE\n");
#endif
                break; /* FIXME: Implement */
            case 0x1A: /* LD A,(DE) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD A,(DE)\n");
#endif
                break; /* FIXME: Implement */
            case 0x1B: /* DEC DE */
#ifdef DYNA_DEBUG
                fdprintf(fd,"DEC DE\n");
#endif
                DYNA_TST_b_r(5,0); /* test E */
                DYNA_DUMMYBRANCH(4,0);
                DYNA_DEC_l_r(4,0); /* dec D */
                DYNA_BCC_c(0x6,4,0); /* jump here if not zero */
                DYNA_DEC_l_r(5,0); /* dec E */
                break;
            case 0x1C: /* INC E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC E\n");
#endif
                DYNA_INC_l_r(5,0); /* Is this right? */
                break;
            case 0x1D: /* DEC E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC E\n");
#endif
                DYNA_DEC_l_r(5,0);  /* Is this right? */
                break;
            case 0x1E: /* LD E,imm */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD E,#0x%x\n",readb(xPC)); 
#endif
                DYNA_MOVEQ_l_i_to_r(FETCH,5);
                break;
            case 0x1F: /* RRA */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RRA\n");
#endif
                break; /* FIXME: Implement */
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
            case 0x21: /* LD HL,imm */
#ifdef DYNA_DEBUG
                fdprintf(fd,"LD HL,#0x%x\n",readw(xPC));
#endif
                DYNA_MOVE_l_i_to_r(readw(xPC),6);
                PC += 2;
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
            case 0x23: /* INC HL */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC HL\n");
#endif
                break; /* FIXME: Implement */
            case 0x24: /* INC H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC H\n");
#endif
                break; /* FIXME: Implement */
            case 0x25: /* DEC H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC H\n");
#endif
                break; /* FIXME: Implement */
            case 0x26: /* LD H,imm */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD H,#0x%x\n",readb(xPC)); 
#endif
                DYNA_AND_l_i_to_r(0xFF,6);
                DYNA_OR_l_i_to_r(FETCH<<8,6);
                break;
            case 0x27: /* DAA */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DAA\n");
#endif
                break; /* FIXME: Implement */
            case 0x28: /* JR Z */
#ifdef DYNA_DEBUG
                fdprintf(fd, "JR Z\n");
#endif
                break; /* FIXME: Implement */
            case 0x29: /* ADD HL, HL */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD HL,HL\n");
#endif
                break; /* FIXME: Implement */
            case 0x2A: /* LD A,(HLI) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD A,(HLI)\n");
#endif
                break; /* FIXME: Implement */
            case 0x2B: /* DEC HL */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC HL\n");
#endif
                break; /* FIXME: Implement */
            case 0x2C: /* INC L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC L\n");
#endif
                break; /* FIXME: Implement */
            case 0x2D: /* DEC L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC L\n");
#endif
                break; /* FIXME: Implement */
            case 0x2E: /* LD L,imm */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD L,#0x%x\n",readb(xPC)); 
#endif
                DYNA_AND_l_i_to_r(0xFF00,6);
                DYNA_OR_l_i_to_r(FETCH,6);
                break;
            case 0x2F: /* CPL */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CPL\n");
#endif
                break; /* FIXME: Implement */
            case 0x30: /* JR NC */
#ifdef DYNA_DEBUG
                fdprintf(fd, "JR NC\n");
#endif
                break; /* FIXME: Implement */
            case 0x31: /* LD SP,imm */
#ifdef DYNA_DEBUG
                fdprintf(fd,"LD SP,#0x%x\n",readw(xPC));
#endif
                DYNA_MOVEA_l_i_to_r(readw(xPC),0);
                PC += 2; 
                break;
            case 0x32: /* LD (HLD),A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HLD),A\n");
#endif
                break; /* FIXME: Implement */
            case 0x33: /* INC SP */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC SP\n");
#endif
                break; /* FIXME: Implement */
            case 0x34: /* INC (HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC (HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0x35: /* DEC (HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC (HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0x36: /* LD (HD),imm */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HD),imm\n");
#endif
                break; /* FIXME: Implement */
            case 0x37: /* SCF */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SCF\n");
#endif
                break; /* FIXME: Implement */
            case 0x38: /* JR C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "JR C\n");
#endif
                break; /* FIXME: Implement */
            case 0x39: /* ADD HL,SP */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD HL,SP\n");
#endif
                break; /* FIXME: Implement */
            case 0x3A: /* LD A,(HLD) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD A,(HLD)\n");
#endif
                break; /* FIXME: Implement */
            case 0x3B: /* DEC SP */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC SP\n");
#endif
                break; /* FIXME: Implement */
            case 0x3C: /* INC A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "INC A");
#endif
                DYNA_INC_l_r(1,0); /* Is this right? */
                break;
            case 0x3D: /* DEC A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "DEC A");
#endif
                DYNA_DEC_l_r(1,0);  /* Is this right? */
                break;
            case 0x3E: /* LD A,imm */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD A,#0x%x\n",readb(xPC)); 
#endif
                DYNA_MOVEQ_l_i_to_r(FETCH,1);
                break;
            case 0x3F: /* CCF */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CCF\n");
#endif
                break; /* FIXME: Implement */
            case 0x40: /* LD B,B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD B,B\n");
#endif
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
            case 0x46: /* LD B,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD B,(HL)\n");
#endif
                break; /* FIXME: Implement */
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
            case 0x49: /* LD C,C */
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
            case 0x4E: /* LD C,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD C,(HL)\n");
#endif
                break; /* FIXME: Implement */
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
            case 0x52: /* LD D,D */
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
            case 0x56: /* LD D,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD D,(HL)\n");
#endif
                break; /* FIXME: Implement */
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
            case 0x5B: /* LD E,E */
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
            case 0x5E: /* LD E,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD E,(HL)\n");
#endif
                break; /* FIXME: Implement */
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
            case 0x64: /* LD H,H */
                break;
            case 0x65: /* LD H,L */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD H,L\n"); 
#endif
                DYNA_MOVE_b_r_to_r(6,0);
                PUTUPPER(0,6);
                break;
            case 0x66: /* LD H,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD H,(HL)\n");
#endif
                break; /* FIXME: Implement */
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
            case 0x6D: /* LD L,L */
                break;
            case 0x6E: /* LD L,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD L,(HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0x6F: /* LD L,A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD L,A\n");
#endif
                break; /* FIXME: Implement */
            case 0x70: /* LD (HL),B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HL),B\n");
#endif
                break; /* FIXME: Implement */
            case 0x71: /* LD (HL),C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HL),C\n");
#endif
                break; /* FIXME: Implement */
            case 0x72: /* LD (HL),D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HL),D\n");
#endif
                break; /* FIXME: Implement */
            case 0x73: /* LD (HL),E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HL),E\n");
#endif
                break; /* FIXME: Implement */
            case 0x74: /* LD (HL),H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HL),H\n");
#endif
                break; /* FIXME: Implement */
            case 0x75: /* LD (HL),L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HL),L\n");
#endif
                break; /* FIXME: Implement */
            case 0x76: /* HALT */
#ifdef DYNA_DEBUG
                fdprintf(fd, "HALT\n");
#endif
                break; /* FIXME: Implement */
            case 0x77: /* LD (HL),A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD (HL),A\n");
#endif
                break; /* FIXME: Implement */
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
            case 0x7E: /* LD A,HL */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD A,HL\n");
#endif
                break; /* FIXME: Implement */
            case 0x7F: /* LD A,A */
                break;
            case 0x80: /* ADD A,B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD A,B\n");
#endif
                break; /* FIXME: Implement */
            case 0x81: /* ADD A,C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD A,C\n");
#endif
                break; /* FIXME: Implement */
            case 0x82: /* ADD A,D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD A,D\n");
#endif
                break; /* FIXME: Implement */
            case 0x83: /* ADD A,E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD A,E\n");
#endif
                break; /* FIXME: Implement */
            case 0x84: /* ADD A,H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD A,H\n");
#endif
                break; /* FIXME: Implement */
            case 0x85: /* ADD A,L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD A,L\n");
#endif
                break; /* FIXME: Implement */
            case 0x86: /* ADD A,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD A,(HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0x87: /* ADD A,A */
#ifdef DYNA_DEBUG
                fdprintf(fd,"ADD A,A\n");
#endif
                /* code taken from gcc -O3 output by compiling;
                 *     c=(2*b)&0xFF;
                 *     a=(c) ? 0 : 0x80 |            zero flag 
                 *           (0x20 & (c) << 1) |     halfcarry
                 *           ((2*b)&0x100)>>4;       carry
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
            case 0x88: /* ADC A,B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A,B\n");
#endif
                break; /* FIXME: Implement */
            case 0x89: /* ADC A,C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A,C\n");
#endif
                break; /* FIXME: Implement */
            case 0x8A: /* ADC A,D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A,D\n");
#endif
                break; /* FIXME: Implement */
            case 0x8B: /* ADC A,E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A,E\n");
#endif
                break; /* FIXME: Implement */
            case 0x8C: /* ADC A,H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A,H\n");
#endif
                break; /* FIXME: Implement */
            case 0x8D: /* ADC A,L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A,L\n");
#endif
                break; /* FIXME: Implement */
            case 0x8E: /* ADC A,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A,(HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0x8F: /* ADC A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A\n");
#endif
                break; /* FIXME: Implement */
            case 0x90: /* SUB B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SUB B\n");
#endif
                break; /* FIXME: Implement */
            case 0x91: /* SUB C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SUB C\n");
#endif
                break; /* FIXME: Implement */
            case 0x92: /* SUB D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SUB D\n");
#endif
                break; /* FIXME: Implement */
            case 0x93: /* SUB E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SUB E\n");
#endif
                break; /* FIXME: Implement */
            case 0x94: /* SUB H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SUB H\n");
#endif
                break; /* FIXME: Implement */
            case 0x95: /* SUB L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SUB L\n");
#endif
                break; /* FIXME: Implement */
            case 0x96: /* SUB (HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SUB (HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0x97: /* SUB A (halfcarry ?) */
#ifdef DYNA_DEBUG
                fdprintf(fd,"SUB A\n");
#endif
                DYNA_CLR_l_r(1);
                DYNA_MOVEQ_l_i_to_r(0xC0,7);
                break;
            case 0x98: /* SBC A,B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,B\n");
#endif
                break; /* FIXME: Implement */
            case 0x99: /* SBC A,C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,C\n");
#endif
                break; /* FIXME: Implement */
            case 0x9A: /* SBC A,D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,D\n");
#endif
                break; /* FIXME: Implement */
            case 0x9B: /* SBC A,E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,E\n");
#endif
                break; /* FIXME: Implement */
            case 0x9C: /* SBC A,H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,H\n");
#endif
                break; /* FIXME: Implement */
            case 0x9D: /* SBC A,L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,L\n");
#endif
                break; /* FIXME: Implement */
            case 0x9E: /* SBC A,(HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,(HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0x9F: /* SBC A,A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,A\n");
#endif
                break; /* FIXME: Implement */
            case 0xA0: /* AND B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND B\n");
#endif
                break; /* FIXME: Implement */
            case 0xA1: /* AND C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND C\n");
#endif
                break; /* FIXME: Implement */
            case 0xA2: /* AND D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND D\n");
#endif
                break; /* FIXME: Implement */
            case 0xA3: /* AND E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND E\n");
#endif
                break; /* FIXME: Implement */
            case 0xA4: /* AND H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND H\n");
#endif
                break; /* FIXME: Implement */
            case 0xA5: /* AND L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND L\n");
#endif
                break; /* FIXME: Implement */
            case 0xA6: /* AND (HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND (HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0xA7: /* AND A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND A\n");
#endif
                break; /* FIXME: Implement */
            case 0xA8: /* XOR B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR B\n");
#endif
                break; /* FIXME: Implement */
            case 0xA9: /* XOR C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR C\n");
#endif
                break; /* FIXME: Implement */
            case 0xAA: /* XOR D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR D\n");
#endif
                break; /* FIXME: Implement */
            case 0xAB: /* XOR E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR E\n");
#endif
                break; /* FIXME: Implement */
            case 0xAC: /* XOR H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR H\n");
#endif
                break; /* FIXME: Implement */
            case 0xAD: /* XOR L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR L\n");
#endif
                break; /* FIXME: Implement */
            case 0xAE: /* XOR (HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR (HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0xAF: /* XOR A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR A\n");
#endif
                break; /* FIXME: Implement */
            case 0xB0: /* OR B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR B\n");
#endif
                break; /* FIXME: Implement */
            case 0xB1: /* OR C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR C\n");
#endif
                break; /* FIXME: Implement */
            case 0xB2: /* OR D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR D\n");
#endif
                break; /* FIXME: Implement */
            case 0xB3: /* OR E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR E\n");
#endif
                break; /* FIXME: Implement */
            case 0xB4: /* OR H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR H\n");
#endif
                break; /* FIXME: Implement */
            case 0xB5: /* OR L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR L\n");
#endif
                break; /* FIXME: Implement */
            case 0xB6: /* OR (HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR (HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0xB7: /* OR A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR A\n");
#endif
                break; /* FIXME: Implement */
            case 0xB8: /* CP B */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CP B\n");
#endif
                break; /* FIXME: Implement */
            case 0xB9: /* CP C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CP C\n");
#endif
                break; /* FIXME: Implement */
            case 0xBA: /* CP D */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CP D\n");
#endif
                break; /* FIXME: Implement */
            case 0xBB: /* CP E */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CP E\n");
#endif
                break; /* FIXME: Implement */
            case 0xBC: /* CP H */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CP H\n");
#endif
                break; /* FIXME: Implement */
            case 0xBD: /* CP L */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CP L\n");
#endif
                break; /* FIXME: Implement */
            case 0xBE: /* CP (HL) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CP (HL)\n");
#endif
                break; /* FIXME: Implement */
            case 0xBF: /* CP A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CP A\n");
#endif
                break; /* FIXME: Implement */
            case 0xC0: /* RET NZ */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RET NZ\n");
#endif
                break; /* FIXME: Implement */
            case 0xC1: /* POP BC */
#ifdef DYNA_DEBUG
                fdprintf(fd, "POP BC\n");
#endif
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
            case 0xC3: /* JP (imm) PC = readw(PC) */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"JP (0x%x)\n",readw(xPC)); 
#endif
                PC=readw(PC);
                done=1;
                break;
            case 0xC4: /* CALL NZ */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CALL NZ\n");
#endif
                break; /* FIXME: Implement */
            case 0xC5: /* PUSH BC */
#ifdef DYNA_DEBUG
                fdprintf(fd, "PUSH BC\n");
#endif
                break; /* FIXME: Implement */
            case 0xC6: /* ADD A,imm */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD A,imm\n");
#endif
                break; /* FIXME: Implement */
            case 0xC7: /* RST 0h */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RST 0h\n");
#endif
                break; /* FIXME: Implement */
            case 0xC8: /* RET Z */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RET Z\n");
#endif
                break; /* FIXME: Implement */
            case 0xC9: /* RET */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"RET\n"); 
#endif
                POPA(1);
                writepc=0;
                done=1;
                break;
            case 0xCA: /* JP Z */
                break; /* FIXME: Implement */
            case 0xCB: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xCC: /* CALL Z */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CALL Z\n");
#endif
                break; /* FIXME: Implement */
            case 0xCD: /* CALL (imm) PUSH(PC+2) PC=readw(PC); */
#ifdef DYNA_DEBUG
                fdprintf(fd,"CALL (0x%x)\n",readw(xPC));
#endif
                PUSH(PC+2);
                PC=readw(PC);
                done=1;
                break;
            case 0xCE: /* ADC A,imm */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADC A,imm\n");
#endif
                break; /* FIXME: Implement */
            case 0xCF: /* RST 8h */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RST 8h\n");
#endif
                break; /* FIXME: Implement */
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
            case 0xD1: /* POP DE */
#ifdef DYNA_DEBUG
                fdprintf(fd, "POP DE\n");
#endif
                break; /* FIXME: Implement */
            case 0xD2: /* JP NC */
#ifdef DYNA_DEBUG
                fdprintf(fd, "JP NC\n");
#endif
                break; /* FIXME: Implement */
            case 0xD3: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xD4: /* CALL NC */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CALL NC\n");
#endif
                break; /* FIXME: Implement */
            case 0xD5: /* PUSH DE */
#ifdef DYNA_DEBUG
                fdprintf(fd, "PUSH DE\n");
#endif
                break; /* FIXME: Implement */
            case 0xD6: /* SUB imm */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SUB imm\n");
#endif
                break; /* FIXME: Implement */
            case 0xD7: /* RST 10h */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RST 10h\n");
#endif
                break; /* FIXME: Implement */
            case 0xD8: /* RET C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RET C\n");
#endif
                break; /* FIXME: Implement */
            case 0xD9: /* RETI */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RETI\n");
#endif
                break; /* FIXME: Implement */
            case 0xDA: /* JP C */
#ifdef DYNA_DEBUG
                fdprintf(fd, "JP C\n");
#endif
                break; /* FIXME: Implement */
            case 0xDB: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xDC: /* CALL NC */
#ifdef DYNA_DEBUG
                fdprintf(fd, "CALL NC\n");
#endif
                break; /* FIXME: Implement */
            case 0xDD: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xDE: /* SBC A,imm */
#ifdef DYNA_DEBUG
                fdprintf(fd, "SBC A,imm\n");
#endif
                break; /* FIXME: Implement */
            case 0xDF: /* RST 18h */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RST 18h\n");
#endif
                break; /* FIXME: Implement */
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
            case 0xE1: /* POP HL */
#ifdef DYNA_DEBUG
                fdprintf(fd, "POP HL\n");
#endif
                break; /* FIXME: Implement */
            case 0xE2: /* LDH (imm),A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LDH (imm),A\n");
#endif
                break; /* FIXME: Implement */
            case 0xE3: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xE4: /* NULL */
                break;
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
            case 0xE5: /* PUSH HL */
#ifdef DYNA_DEBUG
                fdprintf(fd, "PUSH HL\n");
#endif
                break; /* FIXME: Implement */
            case 0xE6: /* AND imm */
#ifdef DYNA_DEBUG
                fdprintf(fd, "AND imm\n");
#endif
                break; /* FIXME: Implement */
            case 0xE7: /* RST 20h */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RST 20h\n");
#endif
                break; /* FIXME: Implement */
            case 0xE8: /* ADD SP */
#ifdef DYNA_DEBUG
                fdprintf(fd, "ADD SP\n");
#endif
                break; /* FIXME: Implement */
            case 0xE9: /* JP HL */
#ifdef DYNA_DEBUG
                fdprintf(fd, "JP HL\n");
#endif
                break; /* FIXME: Implement */
            case 0xEA: /* LD A */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD A\n");
#endif
                break; /* FIXME: Implement */
            case 0xEB: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xEC: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xED: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xEE: /* XOR imm */
#ifdef DYNA_DEBUG
                fdprintf(fd, "XOR imm\n");
#endif
                break; /* FIXME: Implement */
            case 0xEF: /* RST 28h */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RST 28h\n");
#endif
                break; /* FIXME: Implement */
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
            case 0xF1: /* POP AF */
#ifdef DYNA_DEBUG
                fdprintf(fd, "POP AF\n");
#endif
                break; /* FIXME: Implement */
            case 0xF2: /* LDH A,(imm) */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LDH A,(imm)\n");
#endif
                break; /* FIXME: Implement */
            case 0xF3: /* DI */
#ifdef DYNA_DEBUG
                fdprintf(fd,"DI\n");
#endif
                DYNA_CLR_l_m(&cpu.ime);
                DYNA_CLR_l_m(&cpu.ima);
                DYNA_CLR_l_m(&cpu.halt);
                /* cpu.halt = cpu.ima = cpu.ime = 0; */
                break;
            case 0xF4: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xF5: /* PUSH AF */
#ifdef DYNA_DEBUG
                fdprintf(fd, "PUSH AF\n");
#endif
                break; /* FIXME: Implement */
            case 0xF6: /* OR imm */
#ifdef DYNA_DEBUG
                fdprintf(fd, "OR imm\n");
#endif
                break; /* FIXME: Implement */
            case 0xF7: /* RST 30h */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RST 30h\n");
#endif
                break; /* FIXME: Implement */
            case 0xF8: /* LD HL,SP */
#ifdef DYNA_DEBUG
                fdprintf(fd, "LD HL,SP\n");
#endif
                break; /* FIXME: Implement */
            case 0xF9: /* LD SP,HL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD SP,HL\n"); 
#endif
                DYNA_MOVEA_w_r_to_r(6,0,0);
                break;
            case 0xFA: /* LD A, (imm) */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"LD A,(0x%x)\n",readw(xPC)); 
#endif
                /*DYNA_PEA_w_i(readw(xPC));
                PC+=2; \
                CALL_NATIVE(&readb); \
                DYNA_ADDQ_l_i_to_r(4,7,1); \
                DYNA_MOVE_l_r_to_r(0,1,0);*/
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
            case 0xFC: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xFD: /* NULL */
#ifdef DYNA_DEBUG 
                fdprintf(fd,"NULL\n"); 
#endif
                break;
            case 0xFE: /* CMP #<imm> TODO: can be (much) more efficient.*/
#ifdef DYNA_DEBUG
                fdprintf(fd,"CMP #0x%x\n",readb(xPC));
#endif
                DYNA_MOVEA_l_r_to_r(2,3,0); /* movea.l %d2, %a3 */
                DYNA_MOVEQ_l_i_to_r(FETCH,2); /* moveq.l #<FETCH>,%d2 */
                CMP(2);
                DYNA_MOVE_l_r_to_r(3,2,1); /* move.l %a3, %d2 */
                break;
            case 0xFF: /* RST 38h */
#ifdef DYNA_DEBUG
                fdprintf(fd, "RST 38h\n");
#endif
                break; /* FIXME: Implement */
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
    IF_COP(rb->cpucache_invalidate());
    snprintf(meow,499,"/dyna_0x%x_code.rb",PC);
    fd=open(meow,O_WRONLY|O_CREAT|O_TRUNC);
    if(fd>=0)
    {
        write(fd,newblock->block,newblock->length);
        close(fd);
    }
}
