/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by James Espinoza
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include "system.h"
#include "imx31l.h"
#include "avic-imx31.h"
#include "debug.h"

void avic_init(void) 
{
	/*following the steps in the AVIC setup in imx31 man*/
	
	/*Initialize interrupt structures*/
	int i,avicstart;
	/*get start of avic_init section for address calculation*/
	__asm__ ("ldr %0,=_avicstart\n\t"
		     :"=r"(avicstart):);

	for(i=0; i < 64;i++)
	{
		imx31_int[i].name = (char *)&imx31_int_names[i];
		imx31_int[i].int_type=IRQ;
		/*integer i MUST be multiplied by 8 b/c gnu as
		  generates 2 instructions for each vector instruction
		  in vector_init(). Hence the value of 8 byte intervals
		  between each vector start address*/
		imx31_int[i].addr=(avicstart+(i*8));
		imx31_int[i].priority=0;
		imx31_int[i].pInt_Handler=Unhandled_Int;
	}
	
	/*enable all Interrupts*/
	avic_enable_int(ALL,IRQ,0);
	
	/*Setup all interrupt type IRQ*/
	avic_set_int_type(ALL,IRQ);
	
	/*Set NM bit to enable VIC*/
	INTCNTL |= (1 << 18);

	/*Setup Registers Vector0-Vector63 for interrupt handler functions*/
	for(i=0; i < 64;i++)
		writel(imx31_int[i].addr,(VECTOR_BASE_ADDR+(i*8)));
	
	/*disable FIQ for now until the interrupt handlers are more mature...*/
	disable_fiq();
	/*enable_fiq();*/
	
	/*enable IRQ in imx31 INTCNTL reg*/
	INTCNTL &= ~(NIDIS);	
	/*disable FIQ in imx31 INTCNTL reg*/
	INTCNTL |= FIDIS;

	/*enable IRQ in ARM11 core, enable VE bit in CP15 Control reg to enable VIC*/
	__asm__ ("mrs r0,cpsr\t\n"
		  "bic r0,r0,#0x80\t\n"
		  "msr cpsr,r0\t\n"
		  "mrc p15,0,r0,c1,c0,0\n\t"
	          "orr r0,r0,#0x1000000\n\t"
		  "mcr p15,0,r0,c1,c0,0\n\t":::
		  "r0");
}

void avic_enable_int(enum IMX31_INT_LIST ints, enum INT_TYPE intstype,
		     void (*pInt_Handler) (void))
{	
	int i;

	if(ints == ALL) 
	{
		avic_set_int_type(ALL,intstype);
		for(i=0;i<64;i++)
			INTENNUM= (long)i;
		if(!(*pInt_Handler))
			pInt_Handler=Unhandled_Int;
		return;
	} 

	imx31_int[ints].int_type=intstype;
	imx31_int[ints].pInt_Handler=pInt_Handler;
	avic_set_int_type(ints,intstype);
	INTENNUM=(long)ints;
}

void avic_disable_int(enum IMX31_INT_LIST ints)
{
	int i;

	if(ints == ALL)
	{
		for(i=0;i<64;i++)		
			INTDISNUM=(long)i;
		imx31_int[ints].pInt_Handler=Unhandled_Int;
		return;
	}
	
	INTDISNUM=(long)ints;
}

void avic_set_int_type(enum IMX31_INT_LIST ints, enum INT_TYPE intstype)
{
	int i;
	if(ints == ALL)
	{	
		imx31_int[ints].int_type=intstype;
		for(i=0;i<64;i++)
		{
			if(intstype > CCM_DVFS)
				INTTYPEH=(long)(intstype-32);
			else INTTYPEL=(long)intstype;
		}
		return;
	}
	
	imx31_int[ints].int_type=intstype;
	if(intstype > CCM_DVFS)
		INTTYPEH=(long)(intstype-32);
	else INTTYPEL=(long)intstype;
}

void Unhandled_Int(void) 
{
	enum IMX31_INT_LIST ints = 0;
	DEBUGF("Unhandled Interrupt:\n");
	DEBUGF("Name : %s\n",imx31_int[ints].name);
	DEBUGF("Interrupt Type : ");
	if(imx31_int[ints].int_type==IRQ)
		DEBUGF("IRQ\n");
	else DEBUGF("FIQ\n");
	DEBUGF("Handler Address : 0x%x\n",imx31_int[ints].addr);
	DEBUGF("Priority : %d",imx31_int[ints].priority);
}

void vector_init(void)
{
		
		/*64 branch instructions, one for every vector in avic
		A better idea would to calculate the shellcode for each of these 
		instructions...*/
		
		
	    __asm__("ldr pc, %0\n\t"::"g"(imx31_int[RESERVED0].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RESERVED1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RESERVED2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[I2C3].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[I2C2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[MPEG4_ENCODER].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RTIC].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[FIR].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[MMC_SDHC2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[MMC_SDHC1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[I2C1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[SSI2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[SSI1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[CSPI2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[CSPI1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[ATA].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[MBX].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[CSPI3].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[UART3].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[IIM].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[SIM1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[SIM2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RNGA].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EVTMON].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[KPP].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RTC].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[PWN].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EPIT2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EPIT1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[GPT].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[PWR_FAIL].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[CCM_DVFS].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[UART2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[NANDFC].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[SDMA].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[USB_HOST1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[USB_HOST2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[USB_OTG].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RESERVED3].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[MSHC1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[MSHC2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[IPU_ERR].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[IPU].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RESERVED4].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RESERVED5].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[UART1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[UART4].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[UART5].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[ETC_IRQ].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[SCC_SCM].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[SCC_SMN].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[GPIO2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[GPIO1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[CCM_CLK].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[PCMCIA].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[WDOG].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[GPIO3].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[RESERVED6].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EXT_PWMG].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EXT_TEMP].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EXT_SENS1].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EXT_SENS2].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EXT_WDOG].pInt_Handler));
		__asm__("ldr pc, %0\n\t"::"g"(imx31_int[EXT_TV].pInt_Handler));
        
}
