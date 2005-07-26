/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 *
 * Copyright (C) 2003-2005 Jörg Hohensohn
 *
 * Alpine CD changer Project
 * This is a feasibility study for Archos emulating an Alpine M-Bus CD changer.
 * 
 * Currently it will do seeks and change tracks, but nothing like disks.
 * The debug version shows a dump of the M-Bus communication on screen.
 *
 * Usage: Start plugin, it will stay in the background and do the emulation.
 * You need to make an adapter with an 8-pin DIN plug for the radio at one end
 * and a 4-ring 3.5 mm plug for the Archos headphone jack at the other.
 * The Archos remote pin connects to the M-Bus, audio as usual.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

/* Only build for (correct) target */
#if !defined(SIMULATOR) && CONFIG_CPU==SH7034 && !defined(HAVE_MMC)

#ifdef HAVE_LCD_CHARCELLS /* player model */
#define LINES    2
#define COLUMNS 11
#else /* recorder models */
#define LINES    8
#define COLUMNS 32 /* can't really tell for proportional font */
#endif

/****************** imports ******************/

#include "sh7034.h"
#include "system.h"

/****************** constants ******************/

/* measured bit time on the M-Bus is 3.075 ms = 325.2 Hz */
#define MBUS_BAUDRATE 3252 /* make it 10 * bittime */
#define MBUS_STEP_FREQ (MBUS_BAUDRATE/2) /* 5 steps per bit */
#define MBUS_BIT_FREQ (MBUS_BAUDRATE/10) /* the true bit frequency again */

#define MBUS_MAX_SIZE      16 /* maximum length of an M-Bus packet, incl. checksum */
#define MBUS_RCV_QUEUESIZE  4 /* how many packets can be queued by receiver */

#define ERI1  (*((volatile unsigned long*)0x090001A0)) /* RX error */
#define RXI1  (*((volatile unsigned long*)0x090001A4)) /* RX */

#define PB10 0x0400

/* receive status */
#define RX_BUSY      0 /* reception in progress */
#define RX_RECEIVED  1 /* valid data available */
#define RX_FRAMING   2 /* frame error */
#define RX_OVERRUN   3 /* receiver overrun */
#define RX_PARITY    4 /* parity error */
#define RX_SYMBOL    5 /* invalid bit timing */
#define RX_OVERFLOW  6 /* buffer full */
#define RX_OVERLAP   7 /* receive interrupt while transmitting */

/* timer operation mode */
#define TM_OFF        0 /* not in use */
#define TM_TRANSMIT   1 /* periodic timer to transmit */
#define TM_RX_TIMEOUT 2 /* single shot for receive timeout */

/* emulation play state */
#define EMU_IDLE      0
#define EMU_PREPARING 1
#define EMU_STOPPED   2
#define EMU_PAUSED    3
#define EMU_PLAYING   4
#define EMU_SPINUP    5
#define EMU_FF        6
#define EMU_FR        7


/****************** prototypes ******************/

void timer_init(unsigned hz, unsigned to); /* setup static timer registers and values */
void timer_set_mode(int mode); /* define for what the timer should be used right now */
void timer4_isr(void); /* IMIA4 ISR */

void transmit_isr(void); /* 2nd level ISR for M-Bus transmission */

void uart_init(unsigned baudrate); /* UART setup */
void uart_rx_isr(void) __attribute__((interrupt_handler)); /* RXI1 ISR */
void uart_err_isr(void) __attribute__((interrupt_handler)); /* ERI1 ISR */
void receive_timeout_isr(void); /* 2nd level ISR for receiver timeout */

void mbus_init(void); /* prepare the M-Bus layer */
int mbus_send(unsigned char* p_msg, int digits); /* packet send */
int mbus_receive(unsigned char* p_msg, unsigned bufsize, int timeout); /* packet receive */

unsigned char calc_checksum(unsigned char* p_msg, int digits); /* make M-Bus checksum */
bool bit_test(unsigned char* buf, unsigned bit); /* test one bit of M-Bus packet */
void bit_set(unsigned char* buf, unsigned bit, bool val); /* set/clear one bit of M-Bus packet */

void print_scroll(char* string); /* implements a scrolling screen */
void dump_packet(char* dest, int dst_size, char* src, int n); /* convert packet to ASCII */

void emu_init(void); /* init changer emulation */
void emu_process_packet(unsigned char* mbus_msg, int msg_size); /* feed a received radio packet */
void emu_tick(void); /* for regular actions of the emulator */

int get_playtime(void); /* return the current track time in seconds */
int get_tracklength(void); /* return the total length of the current track */
void set_track(int selected);
int get_track(void); /* return the track number */
void set_play(void); /* start or resume playback */
void set_pause(void); /* pause playback */
void set_stop(void); /* stop playback */
void set_position(int seconds); /* seek */
void get_playmsg(void); /* update the play message with Rockbox info */
void get_diskmsg(void); /* update the disk status message with Rockbox info */

void sound_neutral(void); /* set to everything flat and 0 dB volume */
void sound_normal(void); /* return to user settings */

void thread(void); /* the thread running it all */
int main(void* parameter); /* main loop */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter); /* entry */


/****************** data types ******************/

/* one entry in the receive queue */
typedef struct
{
    unsigned char buf[MBUS_MAX_SIZE]; /* message buffer */
    unsigned size; /* length of data in the buffer */
    unsigned error; /* error code from reception */
} t_rcv_queue_entry;


/****************** globals ******************/


/* information owned by the timer transmit ISR */
struct 
{
    unsigned char send_buf[MBUS_MAX_SIZE]; /* M-Bus message */
    unsigned send_size; /* current length of data in the buffer */
    unsigned index; /* index for which byte to send */
    unsigned byte; /* which byte to send */
    unsigned bitmask; /* which bit to send */
    unsigned step; /* where in the pulse are we */
    bool bit; /* currently sent bit */
    bool collision; /* set if a collision happened */
    bool busy; /* flag if in transmission */
} gSendIRQ;


/* information owned by the UART receive ISR */
struct 
{
    t_rcv_queue_entry queue[MBUS_RCV_QUEUESIZE]; /* M-Bus message queue */
    unsigned buf_read; /* readout maintained by the user application */
    unsigned buf_write; /* writing maintained by ISR */
    bool overflow; /* indicate queue overflow */
    unsigned byte; /* currently assembled byte */
    unsigned bit; /* which bit to receive */
} gRcvIRQ;


/* information owned by the timer */
struct
{
    unsigned mode; /* off, transmit, receive timout */
    unsigned transmit; /* value for transmit */
    unsigned timeout; /* value for receive timeout */
} gTimer;


/* information owned by the changer emulation */
struct
{
    unsigned char playmsg[15]; /* current play state msg */
    unsigned char changemsg[11]; /* changing message */
    unsigned char diskmsg[12]; /* disk status message */
    long poll_interval; /* call the emu each n ticks */
    int time; /* seconds within the song */
    int set_state; /* the desired state to change into */
} gEmu;


/* communication to the worker thread */
struct 
{
    int id; /* ID of the thread */
    bool foreground; /* set as long as we're owning the UI */
    bool exiting; /* signal to the thread that we want to exit */
    bool ended; /* response from the thread, that is has exited */
} gTread;

static struct plugin_api* rb; /* here is the global API struct pointer */


/****************** implementation ******************/


/* setup static timer registers and values */
void timer_init(unsigned hz, unsigned to)
{
    rb->memset(&gTimer, 0, sizeof(gTimer));
    
    gTimer.transmit = FREQ / hz; /* time for bit transitions */
    gTimer.timeout = FREQ / to; /* time for receive timeout */
}


/* define for what the timer should be used right now */
void timer_set_mode(int mode)
{
    TCNT4 = 0; /* start counting at 0 */
    gTimer.mode = mode; /* store the mode */

    if (mode == TM_RX_TIMEOUT)
    {
        rb->timer_register(1, NULL, gTimer.timeout, 11, timer4_isr);
    }
    else if (mode == TM_TRANSMIT)
    {
        rb->timer_register(1, NULL, gTimer.transmit, 14, timer4_isr);
    }
    else
    {
        rb->timer_unregister();
    }
}


void timer4_isr(void) /* IMIA4 */
{
    switch (gTimer.mode)
    {   /* distribute the interrupt */
    case TM_TRANSMIT:
        transmit_isr();
        break;
    case TM_RX_TIMEOUT:
        receive_timeout_isr();
        rb->timer_unregister(); /* single shot */
        break;
    default:
        timer_set_mode(TM_OFF); /* spurious interrupt */
    } /* switch */
}


/* About Alpine M-Bus
 * ------------------
 *
 * The protocol uses a single wire in half duplex mode.
 * A bit like I2C, this wire is either pulled low or left floating high.
 * Bit time is ~3 ms, a "zero" is coded as ~0.6 ms low, a "one" as ~1.8 ms low.
 * Nice to view in a 0.6 ms grid:
 *
 *    0   0.6  1.2  1.8  2.4  3.0
 *    |    |    |    |    |    |
 *  __      ___________________
 *    \____/                   \    "zero" bit
 *  __                _________
 *    \______________/         \    "one" bit
 *
 * So I send out the data in a timer interrupt spawned to 0.6 ms.
 * In phases where the line is floating high, I can check for collisions.
 * (happens if the other side driving it low, too.)
 *
 * Data is transmitted in multiples of 4 bit, to ease BCD representation.
 */


/* 2nd level ISR for M-Bus transmission */
void transmit_isr(void)
{
    bool exit = false;

    TSR4 &= ~0x01; /* clear the interrupt */

    switch(gSendIRQ.step++)
    {
    case 0:
        and_b(~0x04, &PBDRH); /* low (read-modify-write access may have changed it while it was input) */
        or_b(0x04, &PBIORH); /* drive low (output) */
        break;
    case 1: /* 0.6 ms */
        if (!gSendIRQ.bit) /* sending "zero"? */
            and_b(~0x04, &PBIORH); /* float (input) */
        break;
    case 2: /* 1.2 ms */
        if (!gSendIRQ.bit && ((PBDR & PB10) == 0))
            gSendIRQ.collision = true;
        break;
    case 3: /* 1.8 ms */
        if (gSendIRQ.bit) /* sending "one"? */
            and_b(~0x04, &PBIORH); /* float (input) */
        else if ((PBDR & PB10) == 0)
            gSendIRQ.collision = true;
        break;
    case 4: /* 2.4 ms */
        if ((PBDR & PB10) == 0)
            gSendIRQ.collision = true;
        
        /* prepare next round */
        gSendIRQ.step = 0;
        gSendIRQ.bitmask >>= 1;
        if (gSendIRQ.bitmask)
        {   /* new bit */
            gSendIRQ.bit = (gSendIRQ.byte & gSendIRQ.bitmask) != 0;
        }
        else
        {   /* new byte */
            if (++gSendIRQ.index < gSendIRQ.send_size)
            {
                gSendIRQ.bitmask = 0x08;
                gSendIRQ.byte = gSendIRQ.send_buf[gSendIRQ.index];
                gSendIRQ.bit = (gSendIRQ.byte & gSendIRQ.bitmask) != 0;
            }
            else
                exit = true; /* done */
        }
        break;
    }

    if (exit || gSendIRQ.collision)
    {   /* stop transmission */
        or_b(0x20, PBCR1_ADDR+1); /* RxD1 again for PB10 */
        timer_set_mode(TM_OFF); /* stop the timer */
        gSendIRQ.busy = false; /* do this last, to avoid race conditions */
    }
}


/* For receiving, I use the "normal" serial RX feature of the CPU,
 * so we can receive within an interrupt, no line polling necessary.
 * Luckily, the M-Bus bit always starts with a falling edge and ends with a high,
 * this matches with the start bit and the stop bit of a serial transmission.
 * The baudrate is set such that the M-Bus bit time (ca. 3ms) matches
 * the serial reception time of one byte, so we receive one byte per
 * M-Bus bit.
 * Start bit, 8 data bits and stop bit (total=10) nicely fall into the 5
 * phases like above:
 *
 *    0      0.6     1.2     1.8     2.4     3.0 ms
 *    |       |       |       |       |       |    time
 *  __         _______________________________
 *    \_______/                               \    "zero" bit
 *  __                         _______________
 *    \_______________________/               \    "one" bit
 *
 *    |   |   |   |   |   |   |   |   |   |   |    serial sampling interval
 *    Start 0   1   2   3   4   5   6   7  Stop    bit (LSB first!)
 *
 * By looking at the bit pattern in the serial byte we can distinguish 
 * the short low from the longer low, tell "zero" and "one" apart.
 * So we receive 0xFE for a "zero", 0xE0 for a "one".
 * It may be necessary to treat the bits next to transitions as don't care,
 * in case the timing is not so accurate.
 * Bits are always sent "back-to-back", so I detect the packet end by timeout.
 */


void uart_init(unsigned baudrate) 
{
    RXI1 = (unsigned long)uart_rx_isr; /* install ISR */
    ERI1 = (unsigned long)uart_err_isr; /* install ISR */

    SCR1 = 0x00; /* disable everything; select async mode with SCK pin as I/O */
    SMR1 = 0x00; /* async, 8N1, NoMultiProc, sysclock/1 */
    BRR1 = ((FREQ/(32*baudrate))-1);
                 
    IPRE = (IPRE & ~0xf000) | 0xc000; /* interrupt on level 12 */

    rb->sleep(1); /* hardware needs to settle for at least one bit interval */

    and_b(~(SCI_RDRF | SCI_ORER | SCI_FER | SCI_PER), &SSR1); /* clear any receiver flag */
    or_b(SCI_RE | SCI_RIE , &SCR1); /* enable the receiver with interrupt */
}


void uart_rx_isr(void) /* RXI1 */
{
    unsigned char data;
    t_rcv_queue_entry* p_entry = &gRcvIRQ.queue[gRcvIRQ.buf_write]; /* short cut */
    
    data = RDR1; /* get data */
    
    and_b(~SCI_RDRF, &SSR1); /* clear data received flag */

    if (gTimer.mode == TM_TRANSMIT)
        p_entry->error = RX_OVERLAP; /* oops, we're also transmitting, stop */
    else
        timer_set_mode(TM_RX_TIMEOUT); /* (re)spawn timeout */

    if (p_entry->error != RX_BUSY)
        return;

    if ((data & ~0x00) == 0xFE) /* 01111111 in line order (reverse) */
    {   /* "zero" received */
        gRcvIRQ.byte <<= 1;
    }
    else if ((data & ~0x00) == 0xE0) /* 00000111 in line order (reverse) */
    {   /* "one" received */
        gRcvIRQ.byte = gRcvIRQ.byte << 1 | 0x01;
    }
    else
    {   /* unrecognized pulse */
        p_entry->error = RX_SYMBOL;
    }

    if (p_entry->error == RX_BUSY)
    {
        if (++gRcvIRQ.bit >= 4)
        {   /* byte completed */
            if (p_entry->size >= sizeof(p_entry->buf))
            {
                p_entry->error = RX_OVERFLOW; /* buffer full */
            }
            else
            {
                p_entry->buf[p_entry->size] = gRcvIRQ.byte;
                gRcvIRQ.byte = 0;
                gRcvIRQ.bit = 0;
                p_entry->size++;
            }
        }
    }
}


void uart_err_isr(void) /* ERI1 */
{
    t_rcv_queue_entry* p_entry = &gRcvIRQ.queue[gRcvIRQ.buf_write]; /* short cut */

    if (p_entry->error == RX_BUSY)
    {   /* terminate reception in case of error */
        if (SSR1 & SCI_FER) 
            p_entry->error = RX_FRAMING;
        else if (SSR1 & SCI_ORER)
            p_entry->error = RX_OVERRUN;
        else if (SSR1 & SCI_PER)
            p_entry->error = RX_PARITY;
    }

    /* clear any receiver flag */
    and_b(~(SCI_RDRF | SCI_ORER | SCI_FER | SCI_PER), &SSR1);
}


/* 2nd level ISR for receiver timeout, this finalizes reception */
void receive_timeout_isr(void)
{
    t_rcv_queue_entry* p_entry = &gRcvIRQ.queue[gRcvIRQ.buf_write]; /* short cut */

    timer_set_mode(TM_OFF); /* single shot */

    if (p_entry->error == RX_BUSY) /* everthing OK so far? */
        p_entry->error = RX_RECEIVED; /* end with valid data */

    /* move to next queue entry */
    gRcvIRQ.buf_write++;
    if (gRcvIRQ.buf_write >= MBUS_RCV_QUEUESIZE)
        gRcvIRQ.buf_write = 0;
    p_entry = &gRcvIRQ.queue[gRcvIRQ.buf_write];

    if (gRcvIRQ.buf_write == gRcvIRQ.buf_read)
    {   /* queue overflow */
        gRcvIRQ.overflow = true;
        /* what can I do? Continueing overwrites the oldest. */
    }

    gRcvIRQ.byte = 0;
    gRcvIRQ.bit = 0;
    p_entry->size = 0;
    p_entry->error = RX_BUSY; /* enable receive on new entry */
}


/* generate the checksum */
unsigned char calc_checksum(unsigned char* p_msg, int digits)
{
	int chk = 0;
	int i;
	
	for (i=0; i<digits; i++)
	{
		chk ^= p_msg[i];
	}
	chk = (chk+1) % 16;
	
	return chk;
}


/****************** high-level M-Bus functions ******************/

void mbus_init(void)
{
    /* init the send object */
    rb->memset(&gSendIRQ, 0, sizeof(gSendIRQ));
    timer_init(MBUS_STEP_FREQ, (MBUS_BIT_FREQ*10)/15); /* setup frequency and timeout (1.5 bit) */

    /* init receiver */
    rb->memset(&gRcvIRQ, 0, sizeof(gRcvIRQ));
    uart_init(MBUS_BAUDRATE);
}


/* send out a number of BCD digits (one per byte) with M-Bus protocol */
int mbus_send(unsigned char* p_msg, int digits)
{
    /* wait for previous transmit/receive to end */
    while(gTimer.mode != TM_OFF) /* wait for "free line" */
        rb->sleep(1);
    
    /* fill in our part */
    rb->memcpy(gSendIRQ.send_buf, p_msg, digits);

    /* add checksum */
    gSendIRQ.send_buf[digits] = calc_checksum(p_msg, digits);
    digits++;

    /* debug dump, to be removed */
    if (gTread.foreground)
    {
        char buf[MBUS_MAX_SIZE+1];
        dump_packet(buf, sizeof(buf), gSendIRQ.send_buf, digits);
        /*print_scroll(buf); */
    }

    gSendIRQ.send_size = digits;

    /* prepare everything so the ISR can start right away */
    gSendIRQ.index = 0;
    gSendIRQ.byte = gSendIRQ.send_buf[0];
    gSendIRQ.bitmask = 0x08;
    gSendIRQ.step = 0;
    gSendIRQ.bit = (gSendIRQ.byte & gSendIRQ.bitmask) != 0;
    gSendIRQ.collision = false;
    gSendIRQ.busy = true;

    /* last chance to wait for a new detected receive to end */
    while(gTimer.mode != TM_OFF) /* wait for "free line" */
        rb->sleep(1);
    
    and_b(~0x30, PBCR1_ADDR+1); /* GPIO for PB10 */
    timer_set_mode(TM_TRANSMIT); /* run */

    /* make the call blocking until sent out */
    rb->sleep(digits*4*HZ/MBUS_BIT_FREQ); /* should take this long */

    while(gSendIRQ.busy) /* poll in case it lasts longer */
        rb->sleep(1); /* (should not happen) */

    /* debug output, to be removed */
    if (gTread.foreground)
    {
        if (gSendIRQ.collision)
            print_scroll("collision");
    }

    return gSendIRQ.collision;
}


/* returns the size of message copy, 0 if timed out, negative on error */
int mbus_receive(unsigned char* p_msg, unsigned bufsize, int timeout)
{
    int retval = 0;

    do
    {
        if (gRcvIRQ.buf_read != gRcvIRQ.buf_write)
        {   /* something in the queue */
            t_rcv_queue_entry* p_entry = &gRcvIRQ.queue[gRcvIRQ.buf_read]; /* short cut */

            if (p_entry->error == RX_RECEIVED)
            {   /* seems valid */
                rb->memcpy(p_msg, p_entry->buf, MIN(p_entry->size, bufsize));
                retval = p_entry->size; /* return message size */
            }
            else
            {   /* an error occured */
                retval = - p_entry->error; /* return negative number */
            }
            
            /* next queue readout position */
            gRcvIRQ.buf_read++;
            if (gRcvIRQ.buf_read >= MBUS_RCV_QUEUESIZE)
                gRcvIRQ.buf_read = 0;

            return retval; /* exit */
        }
        
        if (timeout != 0 || gTimer.mode != TM_OFF) /* also carry on if reception in progress */
        {
            if (timeout != -1 && timeout != 0) /* if not infinite or expired */
                timeout--;
    
            rb->sleep(1); /* wait a while */
        }

    } while (timeout != 0 || gTimer.mode != TM_OFF);

    return 0; /* timeout */
}


/****************** MMI helper fuctions ******************/


void print_scroll(char* string)
{
    static char screen[LINES][COLUMNS+1]; /* visible strings */
    static unsigned pos = 0; /* next print position */
    static unsigned screentop = 0; /* for scrolling */

    if (!gTread.foreground)
        return; /* just to protect careless callers */

    if (pos >= LINES)
    {   /* need to scroll first */
        int i;
        rb->lcd_clear_display();
        screentop++;
        for (i=0; i<LINES-1; i++)
            rb->lcd_puts(0, i, screen[(i+screentop) % LINES]);

        pos = LINES-1;
    }
    
    /* no strncpy avail. */
    rb->snprintf(screen[(pos+screentop) % LINES], sizeof(screen[0]), "%s", string);

    rb->lcd_puts(0, pos, screen[(pos+screentop) % LINES]);
#ifndef HAVE_LCD_CHARCELLS
    rb->lcd_update();
#endif
    pos++;
}


void dump_packet(char* dest, int dst_size, char* src, int n)
{
    int i;
    int len = MIN(dst_size-1, n);

    for (i=0; i<len; i++)
    {   /* convert to hex digits */
        dest[i] = src[i] < 10 ? '0' + src[i] : 'A' + src[i] - 10;
    }
    dest[i] = '\0'; /* zero terminate string */
}


/****************** CD changer emulation ******************/

bool bit_test(unsigned char* buf, unsigned bit)
{
    return (buf[bit/4] & (0x01 << bit%4)) != 0;
}


void bit_set(unsigned char* buf, unsigned bit, bool val)
{
    if (val)
        buf[bit/4] |= (0x01 << bit%4);
    else
        buf[bit/4] &= ~(0x01 << bit%4);
}


void emu_init(void)
{
    rb->memset(&gEmu, 0, sizeof(gEmu));

    gEmu.poll_interval = HZ;

    /* init the play message to 990000000000000 */
    gEmu.playmsg[0] = gEmu.playmsg[1] = 0x9;

    /* init the changing message to 9B900000001 */
    gEmu.changemsg[0] = gEmu.changemsg[2] = 0x9;
    gEmu.changemsg[1] = 0xB;
    gEmu.changemsg[10] = 0x1;

    /* init the disk status message to 9C1019999990 */
    rb->memset(&gEmu.diskmsg, 0x9, sizeof(gEmu.diskmsg));
    gEmu.diskmsg[1] = 0xC;
    gEmu.diskmsg[2] = gEmu.diskmsg[4] = 0x1;
    gEmu.diskmsg[3] = gEmu.diskmsg[11] = 0x0;
}

/* feed a radio command into the emulator */
void emu_process_packet(unsigned char* mbus_msg, int msg_size)
{
    bool playmsg_dirty = false;
    bool diskmsg_dirty = false;

    if (msg_size == 2 && mbus_msg[0] == 1 && mbus_msg[1] == 8)
    {   /* 18: ping */
        mbus_send("\x09\x08", 2); /* 98: ping OK */
    }
    else if (msg_size == 5 && mbus_msg[0] == 1 && mbus_msg[1] == 1 && mbus_msg[2] == 1)
    {   /* set play state */
        if (bit_test(mbus_msg, 16))
        {
            if (gEmu.set_state == EMU_FF || gEmu.set_state == EMU_FR) /* was seeking? */
            {   /* seek to final position */
                set_position(gEmu.time);
            }
            else if (gEmu.set_state != EMU_PLAYING || gEmu.set_state != EMU_PAUSED)
            {   /* was not playing yet, better send disk message */
                diskmsg_dirty = true;
            }
            set_play();
            gEmu.set_state = EMU_PLAYING;
            playmsg_dirty = true;
        }
        
        if (bit_test(mbus_msg, 17))
        {
            gEmu.set_state = EMU_PAUSED;
            playmsg_dirty = true;
            set_pause();
        }
        
        if (bit_test(mbus_msg, 14))
        {
            gEmu.set_state = EMU_STOPPED;
            playmsg_dirty = true;
            set_stop();
        }

        if (bit_test(mbus_msg, 18))
        {
            gEmu.set_state = EMU_FF;
            playmsg_dirty = true;
            set_pause();
        }

        if (bit_test(mbus_msg, 19))
        {
            gEmu.set_state = EMU_FR;
            playmsg_dirty = true;
            set_pause();
        }

        if (bit_test(mbus_msg, 12)) /* scan stop */
        {
            bit_set(gEmu.playmsg, 51, false);
            playmsg_dirty = true;
        }

        if (gEmu.set_state == EMU_FF || gEmu.set_state == EMU_FR)
            gEmu.poll_interval = HZ/4; /* faster refresh */
        else
            gEmu.poll_interval = HZ;
    }
    else if (msg_size == 8 && mbus_msg[0] == 1 && mbus_msg[1] == 1 && mbus_msg[2] == 4)
    {   /* set program mode */
        gEmu.playmsg[11] = mbus_msg[3]; /* copy repeat, random, intro */
        gEmu.playmsg[12] = mbus_msg[4]; /* ToDo */
        playmsg_dirty = true;
    }
    else if (msg_size ==8 && mbus_msg[0] == 1 && mbus_msg[1] == 1 && mbus_msg[2] == 3)
    {   /* changing */
        gEmu.time = 0; /* reset playtime */
        playmsg_dirty = true;
        if (mbus_msg[3] == 0)
        {   /* changing track */
            if (mbus_msg[4] == 0xA && mbus_msg[5] == 0x3)
            {    /* next random */
                gEmu.playmsg[3] = rb->rand() % 10; /* ToDo */
                gEmu.playmsg[4] = rb->rand() % 10;
            }
            else if  (mbus_msg[4] == 0xB && mbus_msg[5] == 0x3)
            {   /* previous random */
                gEmu.playmsg[3] = rb->rand() % 10; /* ToDo */
                gEmu.playmsg[4] = rb->rand() % 10;
            }
            else
            {   /* normal track select */
                set_track(mbus_msg[4]*10 + mbus_msg[5]);
            }
        }
        else
        {   /* changing disk */
            diskmsg_dirty = true;
            gEmu.changemsg[3] = mbus_msg[3]; /* copy disk */
            gEmu.diskmsg[2] = mbus_msg[3];
            gEmu.changemsg[7] = gEmu.playmsg[11]; /* copy flags from status */
            gEmu.changemsg[8] = gEmu.playmsg[12];
            /*gEmu.playmsg[3] = 0; */  /* reset to track 1 */
            /*gEmu.playmsg[4] = 1; */
            mbus_send(gEmu.changemsg, sizeof(gEmu.changemsg));
        }
    }
    else
    {   /* if in doubt, send Ack */
        mbus_send("\x09\x0F\x00\x00\x00\x00\x00", 7);
    }

    if (playmsg_dirty)
    {
        rb->yield(); /* give the audio thread a chance to process */
        get_playmsg(); /* force update */
        mbus_send(gEmu.playmsg, sizeof(gEmu.playmsg));
    }

    if (diskmsg_dirty)
    {
        get_diskmsg(); /* force update */
        mbus_send(gEmu.diskmsg, sizeof(gEmu.diskmsg));
    }
}


/* called each second in case the emulator has something to do */
void emu_tick(void)
{
    get_playmsg(); /* force update */
    if (bit_test(gEmu.playmsg, 56)) /* play bit */
    {
        unsigned remain; /* helper as we walk down the digits */
        
        switch(gEmu.set_state)
        {
        case EMU_FF:
            gEmu.time += 10;
        case EMU_FR:
            gEmu.time -= 5;

            if (gEmu.time < 0)
                gEmu.time = 0;
            else if (gEmu.time > get_tracklength())
                gEmu.time = get_tracklength();
        
            /* convert value to MM:SS */
            remain = (unsigned)gEmu.time;
            gEmu.playmsg[7] = remain / (10*60);
            remain -= gEmu.playmsg[7] * (10*60);
            gEmu.playmsg[8] = remain / 60;
            remain -= gEmu.playmsg[8] * 60;
            gEmu.playmsg[9] = remain / 10;
            remain -= gEmu.playmsg[9] * 10;
            gEmu.playmsg[10] = remain;
        }

        mbus_send(gEmu.playmsg, sizeof(gEmu.playmsg));
    }
}


/****************** communication with Rockbox playback ******************/


/* update the play message with Rockbox info */
void get_playmsg(void)
{
    int track, time;

    if (gEmu.set_state != EMU_FF && gEmu.set_state != EMU_FR)
    {
        switch(rb->audio_status())
        {
        case AUDIO_STATUS_PLAY:
            print_scroll("AudioStat Play");
            if (gEmu.set_state == EMU_FF || gEmu.set_state == EMU_FR)
                gEmu.playmsg[2] = gEmu.set_state; /* set FF/FR */
            else
                gEmu.playmsg[2] = EMU_PLAYING; /* set normal play */

            bit_set(gEmu.playmsg, 56, true); /* set play */
            bit_set(gEmu.playmsg, 57, false); /* clear pause */
            bit_set(gEmu.playmsg, 59, false); /* clear stop */
            break;

        case AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE:
            print_scroll("AudioStat Pause");
            gEmu.playmsg[2] = EMU_PAUSED;
            bit_set(gEmu.playmsg, 56, false); /* clear play */
            bit_set(gEmu.playmsg, 57, true); /* set pause */
            bit_set(gEmu.playmsg, 59, false); /* clear stop */
            break;

        default:
            print_scroll("AudioStat 0");
            gEmu.playmsg[2] = EMU_STOPPED;
            bit_set(gEmu.playmsg, 56, false); /* clear play */
            bit_set(gEmu.playmsg, 57, false); /* clear pause */
            bit_set(gEmu.playmsg, 59, true); /* set stop */
            break;
        }

        /* convert value to MM:SS */
        time = get_playtime();
        gEmu.time = time; /* copy it */
        gEmu.playmsg[7] = time / (10*60);
        time -= gEmu.playmsg[7] * (10*60);
        gEmu.playmsg[8] = time / 60;
        time -= gEmu.playmsg[8] * 60;
        gEmu.playmsg[9] = time / 10;
        time -= gEmu.playmsg[9] * 10;
        gEmu.playmsg[10] = time;
    }
    else /* FF/FR */
    {
        gEmu.playmsg[2] = gEmu.set_state; /* in FF/FR, report that instead */
    }

    track = get_track();
    gEmu.playmsg[3] = track / 10;
    gEmu.playmsg[4] = track % 10;
}

/* update the disk status message with Rockbox info */
void get_diskmsg(void)
{
    int tracks = rb->playlist_amount();
    if (tracks > 99)
        tracks = 99;
    gEmu.diskmsg[5] = tracks / 10;
    gEmu.diskmsg[6] = tracks % 10;
}

/* return the current track time in seconds */
int get_playtime(void)
{
    struct mp3entry* p_mp3entry;

    p_mp3entry = rb->audio_current_track();
    if (p_mp3entry == NULL)
        return 0;

    return p_mp3entry->elapsed / 1000;
}

/* return the total length of the current track */
int get_tracklength(void)
{
    struct mp3entry* p_mp3entry;

    p_mp3entry = rb->audio_current_track();
    if (p_mp3entry == NULL)
        return 0;

    return p_mp3entry->length / 1000;
}

/* change to a new track */
void set_track(int selected)
{
    if (selected > get_track())
    {
        print_scroll("audio_next");
        rb->audio_next();
    }
    else if (selected < get_track())
    {
        print_scroll("audio_prev");
        rb->audio_prev();
    }
}

/* return the track number */
int get_track(void)
{
    struct mp3entry* p_mp3entry;

    p_mp3entry = rb->audio_current_track();
    if (p_mp3entry == NULL)
        return 0;

    return p_mp3entry->index + 1; /* track numbers start with 1 */
}

/* start or resume playback */
void set_play(void)
{
    if (rb->audio_status() == AUDIO_STATUS_PLAY)
        return;

    if (rb->audio_status() == (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE))
    {
        print_scroll("audio_resume");
        rb->audio_resume();
    }
    else
    {
        print_scroll("audio_play(0)");
        rb->audio_play(0);
    }
}

/* pause playback */
void set_pause(void)
{
    if (rb->audio_status() == AUDIO_STATUS_PLAY)
    {
        print_scroll("audio_pause");
        rb->audio_pause();
    }
}

/* stop playback */
void set_stop(void)
{
    if (rb->audio_status() & AUDIO_STATUS_PLAY)
    {
        print_scroll("audio_stop");
        rb->audio_stop();
    }
}

/* seek */
void set_position(int seconds)
{
    if (rb->audio_status() & AUDIO_STATUS_PLAY)
    {
        print_scroll("audio_ff_rewind");
        rb->audio_ff_rewind(seconds * 1000);
    }
}

/****************** main thread + helper ******************/

/* set to everything flat and 0 dB volume */
void sound_neutral(void)
{    /* neutral sound settings */
    rb->sound_set(SOUND_BASS, 0);
    rb->sound_set(SOUND_TREBLE, 0);
    rb->sound_set(SOUND_BALANCE, 0);
    rb->sound_set(SOUND_VOLUME, 92); /* 0 dB */
    rb->sound_set(SOUND_LOUDNESS, 0);
    rb->sound_set(SOUND_SUPERBASS, 0);
    rb->sound_set(SOUND_AVC, 0);
}

/* return to user settings */
void sound_normal(void)
{   /* restore sound settings */
    rb->sound_set(SOUND_BASS, rb->global_settings->bass);
    rb->sound_set(SOUND_TREBLE, rb->global_settings->treble);
    rb->sound_set(SOUND_BALANCE, rb->global_settings->balance);
    rb->sound_set(SOUND_VOLUME, rb->global_settings->volume);
    rb->sound_set(SOUND_LOUDNESS, rb->global_settings->loudness);
    rb->sound_set(SOUND_SUPERBASS, rb->global_settings->superbass);
    rb->sound_set(SOUND_AVC, rb->global_settings->avc);
}

/* the thread running it all */
void thread(void)
{
    int msg_size;
    unsigned char mbus_msg[MBUS_MAX_SIZE];
    char buf[32];
    bool connected = false;
    long last_tick = *rb->current_tick; /* for 1 sec tick */

    do
    {
        msg_size = mbus_receive(mbus_msg, sizeof(mbus_msg), 1);
        if (msg_size > 0)
        {   /* received something */
            if(gTread.foreground)
            {
                dump_packet(buf, sizeof(buf), mbus_msg, msg_size);
                /*print_scroll(buf); */
            }
            if (msg_size > 2 && mbus_msg[0] == 1 
                && mbus_msg[msg_size-1] == calc_checksum(mbus_msg, msg_size-1))
            {   /* sanity and checksum OK */
                if (!connected)
                {   /* with the first received packet: */
                    sound_neutral(); /* set to flat and 0dB volume */
                    connected = true;
                }
                emu_process_packet(mbus_msg, msg_size-1); /* pass without chksum */
            }
            else if(gTread.foreground)
            {   /* not OK */
                print_scroll("bad packet");
            }
        }
        else if (msg_size < 0 && gTread.foreground)
        {   /* error */
            rb->snprintf(buf, sizeof(buf), "rcv error %d", msg_size);
            print_scroll(buf);
        }

        if (*rb->current_tick - last_tick >= gEmu.poll_interval)
        {   /* call the emulation regulary */
            emu_tick();
            last_tick += gEmu.poll_interval;
        }

    } while (!gTread.exiting);

    gTread.ended = true; /* acknowledge the exit */
    rb->remove_thread(gTread.id); /* commit suicide */
    rb->yield(); /* pass control to other threads, we won't return */
}

/* callback to end the TSR plugin, called before a new one gets loaded */
void exit_tsr(void)
{
    gTread.exiting = true; /* tell the thread to end */
    while (!gTread.ended) /* wait until it did */
        rb->yield();

    uart_init(BAUDRATE); /* return to standard baudrate */
    IPRE = (IPRE & ~0xF000); /* UART interrupt off */
    timer_set_mode(TM_OFF); /* timer interrupt off */

    sound_normal(); /* restore sound settings */
}

/****************** main ******************/


int main(void* parameter)
{
    (void)parameter;
#ifdef DEBUG
    int button;
#endif
    int stacksize;
    void* stack;

    mbus_init(); /* init the M-Bus layer */
    emu_init(); /* init emulator */

    rb->splash(HZ/5, true, "Alpine CDC"); /* be quick on autostart */

#ifdef DEBUG
    print_scroll("Alpine M-Bus Test");
    print_scroll("any key to TSR");
#endif

    /* init the worker thread */
    stack = rb->plugin_get_buffer(&stacksize); /* use the rest as stack */
    stack = (void*)(((unsigned int)stack + 100) & ~3); /* a bit away, 32 bit align */
    stacksize = (stacksize - 100) & ~3;
    if (stacksize < DEFAULT_STACK_SIZE)
    {
        rb->splash(HZ*2, true, "Out of memory");
        return -1;
    }

    rb->memset(&gTread, 0, sizeof(gTread));
    gTread.foreground = true;
    gTread.id = rb->create_thread(thread, stack, stacksize, "CDC");

#ifdef DEBUG
    do
    {
        button = rb->button_get(true);
    } while (button & BUTTON_REL);
#endif

    gTread.foreground = false; /* we're in the background now */
    rb->plugin_tsr(exit_tsr); /* stay resident */
    
#ifdef DEBUG
    return rb->default_event_handler(button);
#else
    return 0;
#endif
}


/***************** Plugin Entry Point *****************/


enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    /* this macro should be called as the first thing you do in the plugin.
       it test that the api version and model the plugin was compiled for
       matches the machine it is running on */
    TEST_PLUGIN_API(api);
    rb = api; /* copy to global api pointer */

    /* now go ahead and have fun! */
    return (main(parameter)==0) ? PLUGIN_OK : PLUGIN_ERROR;
}

#endif /* #ifndef SIMULATOR, etc. */
