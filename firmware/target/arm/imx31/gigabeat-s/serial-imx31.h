/*Remove Comments from UART_INT to enable the UART interrupts,*/
/*otherwise iterrupts will be disabled. For now we will test */
/*UART state by polling the registers, and if necessary update this */
/*method by using the interrupts instead*/

//#define UART_INT

/*Interrupt Control Register*/
#define INTCNTRL (*(volatile int *) 0x68000000)

#define NIDIS 			(1 << 22) /*Normal Interrupt Disable*/
#define FIDIS			(1 << 21) /*Fast Interrupt Disable*/

/*UART1 DEFINES*/
#define UART1_BASE_ADDR 0x43F90000
#define URXD1 (*(volatile int*)UART1_BASE)
#define UTXD1 (*(volatile int*)(UART1_BASE+0x40))
#define UCR1_1 (*(volatile int *)(UART1_BASE+0x80))
#define UCR2_1 (*(volatile int* )(UART1_BASE+0x84))
#define UCR3_1 (*(volatile int* )(UART1_BASE+0x88))
#define UCR4_1 (*(volatile int* )(UART1_BASE+0x8C))
#define UFCR1 (*(volatile int *)(UART1_BASE+ 0x90))
#define USR1_1 (*(volatile int *)(UART1_BASE+0x94))
#define USR2_1 (*(volatile int *)(UART1_BASE+0x98))
#define UTS1    (*(volatile int *)(UART1_BASE+ 0xB4))

#define UCR1_ENABLE_TRDY (1 << 13)/*Enable Tx Rdy Interrupt*/
#define UCR1_ENABLE_RRDY (1 << 9)/*Enable Rx Rdy Interrupt*/
#define UCR1_ENABLE_TXMTY (1 << 6)/*Enable Tx Empty Interrupt*/
#define UCR1_ENABLE_UART (1<< 0) /* Enable UART*/
#define UCR2_WS_8		      (1 << 5) /*Word length 8 bits*/
#define UCR2_EN_TX	      (1 << 2) /*Enable Tx*/
#define UCR2_EN_RX	      (1 << 1)/*Enable Rx*/
#define UCR2_IRTS			(1 << 14)/*Ignore RTS Pin*/
#define UCR4_EN_TC	      (1 << 3)/*Enable Tx Complete Interrupt*/
#define UFCR1_TXTL_32 	      (1 << 15) /*TxFIFO buffer max. 32 characters*/
#define UFCR1_RXTL_32          (1 << 5) /*TxFIFO buffer max. 32 characters*/
#define USR1_TRDY		      (1 << 13)/*Tx RDY Interrupt Flag*/
#define USR1_RRDY		      (1 << 9)  /*Rx RDY Interrupt Flag*/
#define USR2_TXFE		      (1 << 14) /*Tx Empty Interrupt Flag */
#define USR2_TXDC		      (1 << 3)  /*Tx Complete Interrupt Flag*/
#define USR2_RDR		      (1 << 0)/*Rx data ready*/
#define UTS1_TXEMPTY           (1 << 6)/*TxFIFO empty*/
#define UTS1_RXEMPTY	       (1 << 5)/*RxFIFO empty*/
#define UTS1_TXFULL	       (1 << 4)/*TxFIFO full*/
#define UTS1_RXFULL	       (1 << 3)/*RxFIFO full*/

static struct ARM_REGS {
	int r0;
	int r1;
	int r2;
	int r3;
	int r4;
	int r5;
	int r6;
	int r7;
	int r8;
	int r9;
	int r10;
	int r11;
	int r12;
	int sp;
	int lr;
	int pc;
	int cpsr;
} regs;

void Init_UART(void);
int Tx_Rdy(void);
void Tx_Writec(const char c);
void dprintf(const char * str, ... );
inline int dprint(const char * str);
inline void dumpregs();

