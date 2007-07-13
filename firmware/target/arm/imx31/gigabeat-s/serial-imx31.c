#include "console.h"
#include <stdarg.h>
#include <stdio.h>

void Init_UART(void) 
{
#ifdef UART_INT /*enable UART Interrupts */
	UCR1_1 |= (UCR1_ENABLE_TRDY | UCR1_ENABLE_RRDY | UCR1_ENABLE_TXMTY);
	UCR4_1 |= (UCR4_EN_TC);
#else		      /*disable UART Interrupts*/
	UCR1_1 &= ~(UCR1_ENABLE_TRDY | UCR1_ENABLE_RRDY | UCR1_ENABLE_TXMTY);
	UCR4_1 &= ~(UCR4_EN_TC);
#endif
	UCR1_1 |= UCR1_ENABLE_UART;
	UCR2_1 |= (UCR2_EN_TX | UCR2_EN_RX | UCR2_WS_8 | UCR2_IRTS);
	UFCR1 |= (UFCR1_TXTL_32 | UFCR1_RXTL_32);
}

int Tx_Rdy(void)
{
	if((UTS1 & UTS1_TXEMPTY))
		return 1;
	else return 0;
}

int Rx_Rdy(void) 
{
	if(!(UTS1 & UTS1_RXEMPTY))
		return 1;
	else return 0;
}

void Tx_Writec(char c)
{
	UTXD1=(int) c;
}

void dprintf(const char * str, ... )
/*void dprintf(const char * str) */
{
	/*doesn't work as of yet...*/
	
	char dprintfbuff[256];
	unsigned char * ptr;
		
	va_list ap;
	va_start(ap, str);
	
	ptr = dprintfbuff;
	vsnprintf(ptr,sizeof(dprintfbuff),str,ap);
	va_end(ap);
	
	dprint(ptr);

}

volatile int dprint(const char * str)
{
	/*Tx*/
	for(;;) {
	
		if(Tx_Rdy()) {
			
			
			if(*str == '\0')
				return 1;
			
			if(*str == '\n')
				Tx_Writec('\r');

			Tx_Writec(*str);
			str++;
			
		}
	}
}

 inline void dumpregs() 
{
	asm volatile ("mov %0,r0\n\t"
				  "mov %1,r1\n\t"
				  "mov %2,r2\n\t"
				  "mov %3,r3":
				  "=r"(regs.r0),"=r"(regs.r1),
				  "=r"(regs.r2),"=r"(regs.r3):);

    asm volatile ("mov %0,r4\n\t"
				  "mov %1,r5\n\t"
				  "mov %2,r6\n\t"
				  "mov %3,r7":
				  "=r"(regs.r4),"=r"(regs.r5),
				  "=r"(regs.r6),"=r"(regs.r7):);

    asm volatile ("mov %0,r8\n\t"
				  "mov %1,r9\n\t"
				  "mov %2,r10\n\t"
				  "mov %3,r12":
				  "=r"(regs.r8),"=r"(regs.r9),
				  "=r"(regs.r10),"=r"(regs.r11):);
  
	asm volatile ("mov %0,r12\n\t"
				   "mov %1,sp\n\t"
				   "mov %2,lr\n\t"
				   "mov %3,pc\n"
				   "sub %3,%3,#8":
				   "=r"(regs.r12),"=r"(regs.sp),
				   "=r"(regs.lr),"=r"(regs.pc):);
		
    dprintf("Register Dump :\n");
	dprintf("R0=0x%x\tR1=0x%x\tR2=0x%x\tR3=0x%x\n",regs.r0,regs.r1,regs.r2,regs.r3);
    dprintf("R4=0x%x\tR5=0x%x\tR6=0x%x\tR7=0x%x\n",regs.r4,regs.r5,regs.r6,regs.r7);
	dprintf("R8=0x%x\tR9=0x%x\tR10=0x%x\tR11=0x%x\n",regs.r8,regs.r9,regs.r10,regs.r11);
	dprintf("R12=0x%x\tSP=0x%x\tLR=0x%x\tPC=0x%x\n",regs.r12,regs.sp,regs.lr,regs.pc);
	//dprintf("CPSR=0x%x\t\n",regs.cpsr);
 }



