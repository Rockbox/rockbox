/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Michael Sevakis
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

/* Largely taken from sdmaStruct.h from the Linux BSP provided by Freescale.
 * Copyright 2007-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/* Other information gleaned from RE-ing the BSP and SDMA code */

#ifndef SDMA_STRUCT_H
#define SDMA_STRUCT_H

/**
 * Number of channels
 */
#define CH_NUM  32

/**
 * Ownership
 */
#define CH_OWNSHP_EVT (1 << 0)
#define CH_OWNSHP_MCU (1 << 1)
#if 0
#define CH_OWNSHP_DSP (1 << 2)
#endif

/**
 * Channel contexts management
 */

/* Contexts for each channel begin here within SDMA core */
#define CHANNEL_CONTEXT_BASE_ADDR 0x800
/* Compute SDMA address where context for a channel is stored */
#define CHANNEL_CONTEXT_ADDR(channel) \
    (CHANNEL_CONTEXT_BASE_ADDR+sizeof(struct context_data)/4*(channel))

/**
 * Error bit set in the CCB status field by the SDMA, 
 * in setbd routine, in case of a transfer error
 */
#define DATA_ERROR  (1 << 28) /* BD_RROR set on last buffer descriptor */
#define DATA_FAULT  (1 << 29) /* A source or destination fault occured */

/**
 * Buffer descriptor status values.
 */
#define BD_DONE  0x01 /* Set by host, cleared when SDMA has finished with
                         this BD */
#define BD_WRAP  0x02 /* If set in last descriptor, allows circular buffer
                       * structure. curr_bd_ptr will be reset to base_bd_ptr
                       */
#define BD_CONT  0x04 /* If set, more descriptors follow (multi-buffer) */
#define BD_INTR  0x08 /* Interrupt when transfer complete */
#define BD_RROR  0x10 /* Error during BD processing (set by SDMA) */
#define BD_LAST  0x20 /* Set by SDMA ap_2_bp and bp_2_ap scripts.
                         TODO: determine function */
#define BD_EXTD  0x80 /* Use extended buffer address (indicates BD is 12
                         bytes instead of 8) */

/**
 * Buffer descriptor channel 0 commands.
 */
#define C0_SETCTX 0x07 /* Write context for a channel (ch# = BD command [7:3]) */
#define C0_GETCTX 0x03 /* Read context for a channel (ch# = BD command [7:3]) */
#define C0_SETDM  0x01 /* Write 32-bit words to SDMA memory */
#define C0_GETDM  0x02 /* Read 32-bit words from SDMA memory */
#define C0_SETPM  0x04 /* Write 16-bit halfwords to SDMA memory */
#define C0_GETPM  0x08 /* Read 16-bit halfwords from SDMA memory */

/* Treated the same as those above */
#define C0_ADDR   0x01
#define C0_LOAD   0x02
#define C0_DUMP   0x03

/**
 * Transfer sizes, encoded in the BD command field (when not a C0_ command).
 */
#define TRANSFER_32BIT 0x00
#define TRANSFER_8BIT  0x01
#define TRANSFER_16BIT 0x02
#define TRANSFER_24BIT 0x03

/**
 * Change endianness indicator in the BD command field
 */
#define CHANGE_ENDIANNESS   0x80

/**
 * Size in bytes of struct buffer_descriptor
 */
#define SDMA_BD_SIZE           8
#define SDMA_EXTENDED_BD_SIZE 12
#define BD_NUMBER              4

/**
 * Channel interrupt policy
 */
#define DEFAULT_POLL 0
#define CALLBACK_ISR 1
/**
 * Channel status
 */
#define UNINITIALIZED 0
#define INITIALIZED 1

/**
 * IoCtl particular values
 */
#define SET_BIT_ALL     0xFFFFFFFF
#define BD_NUM_OFFSET   16
#define BD_NUM_MASK     0xFFFF0000

/**
 * Maximum values for IoCtl calls, used in high or middle level calls
 */
#define MAX_BD_NUM         256
#define MAX_BD_SIZE        65536
#define MAX_BLOCKING       2
#define MAX_SYNCH          2   
#define MAX_OWNERSHIP      8
#define MAX_CH_PRIORITY    8
#define MAX_TRUST          2
#define MAX_WML            256


/**
 * Default values for channel descriptor - nobody owns the channel
 */
#define CD_DEFAULT_OWNERSHIP  7

#if 0 /* IPC not used */
/**
 * Data Node descriptor status values.
 */
#define DND_END_OF_FRAME  0x80
#define DND_END_OF_XFER   0x40
#define DND_DONE          0x20
#define DND_UNUSED        0x01

/**
 * IPCV2 descriptor status values.
 */
#define BD_IPCV2_END_OF_FRAME  0x40

#define IPCV2_MAX_NODES        50

/**
 * User Type Section
 */

/**
 * Command/Mode/Count of buffer descriptors
 */ 
struct mode_count_ipcv2
{
    unsigned long count    : 16; /* size of the buffer pointed by this BD */
    unsigned long reserved :  8; /* reserved */
    unsigned long status   :  8; /* L, E, D bits stored here */
};

/**
 * Data Node descriptor - IPCv2
 * (differentiated between evolutions of SDMA)
 */
struct data_node_descriptor
{
    struct mode_count_ipcv2 mode; /* command, status and count */
    void* buffer_addr;            /* address of the buffer described */
};

struct mode_count_ipcv1_v2
{
    unsigned long count      : 16; /* size of the buffer pointed by this BD */
    unsigned long status     :  8; /* E,R,I,C,W,D status bits stored here */
    unsigned long reserved   :  7;  
    unsigned long endianness :  1;
};

/**
 * Buffer descriptor 
 * (differentiated between evolutions of SDMA)
 */
struct buffer_descriptor_ipcv1_v2
{
    struct mode_count_ipcv1_v2 mode; /* command, status and count */
    void *buffer_addr;               /* address of the buffer described */
    void *ext_buffer_addr;           /* extended buffer address */
};
#endif /* No IPC */

/**
 * Mode/Count of data node descriptors - IPCv2
 */ 
struct mode_count
{
    unsigned long count   : 16; /* size of the buffer pointed by this BD */
    unsigned long status  :  8; /* E,R,I,C,W,D status bits stored here:
                                 * SDMA r/w */
    unsigned long command :  8; /* channel 0 command or transfer size */
};


/**
 * Buffer descriptor - describes each buffer in a DMA transfer.
 * (differentiated between evolutions of SDMA)
 */
/* (mode.status & BD_EXTD) == 0 (8 bytes) */
struct buffer_descriptor
{
    volatile struct mode_count mode; /* command, status and count: SDMA r/w */
    void *buf_addr;     /* address of the buffer described: SDMA r */
};

/* (mode.status & BD_EXTD) != 0 (12 bytes) */
struct buffer_descriptor_extd
{
    struct buffer_descriptor bd;
    void *buf_addr_ext; /* extended buffer address described (r6): SDMA r */
};

#if 0 /* A different one is defined for Rockbox use - this has too much.
       * See below. */
struct channel_control_block;
struct channel_descriptor;
/**
 * Channel Descriptor
 */
struct channel_descriptor
{
    unsigned char  channel_number;       /* stores the channel number */
    unsigned char  buffer_desc_count;    /* number of BD's allocated
                                            for this channel */
    unsigned short buffer_per_desc_size; /* size (in bytes) of buffer
                                            descriptors' data buffers */
    unsigned long  blocking        :  3; /* polling/ callback method
                                            selection */
    unsigned long  callback_synch  :  1; /* (iapi) blocking / non blocking
                                            feature selection */
    unsigned long  ownership       :  3; /* ownership of the channel
                                            (host+dedicated+event) */
    unsigned long  priority        :  3; /* reflects the SDMA channel
                                            priority register */
    unsigned long  trust           :  1; /* trusted buffers or kernel
                                            allocated */
    unsigned long  use_data_size   :  1; /* (iapi) indicates if the dataSize
                                            field is meaningfull */
    unsigned long  data_size       :  2; /* (iapi->BD) data transfer
                                            size - 8,16,24 or 32 bits*/
    unsigned long  force_close     :  1; /* If TRUE, close channel even
                                            with BD owned by SDMA*/
    unsigned long  script_id       :  7; /* number of the script */
    unsigned long  watermark_level : 10; /* (greg) Watermark level for the
                                            peripheral access */
    unsigned long  event_mask1;          /* (greg) First Event mask */
    unsigned long  event_mask2;          /* (greg) Second  Event mask */
    unsigned long  shp_addr;             /* (greg) Address of the peripheral
                                             or its fifo when needed */
    void (* callback)(struct channel_descriptor *); /* pointer to the
                                             callback function (or NULL) */
    struct channel_control_block *ccb_ptr; /* pointer to the channel control
                                              block associated to this
                                              channel */
};
#endif

/* Only what we need, members sorted by size, no code-bloating bitfields */
struct channel_descriptor
{
    unsigned int bd_count;                 /* number of BD's allocated
                                              for this channel */
    struct channel_control_block *ccb_ptr; /* pointer to the channel control
                                              block associated to this
                                              channel */
    void (* callback)(void);               /* pointer to the
                                              callback function (or NULL) */
    unsigned long shp_addr;                /* (greg) Address of the peripheral
                                              or its fifo when needed */
    unsigned short wml;                    /* (greg) Watermark level for the
                                              peripheral access */
    unsigned char per_type;                /* Peripheral type */
    unsigned char tran_type;               /* Transfer type */
    unsigned char event_id1;               /* DMA request ID */
    unsigned char event_id2;               /* DMA request ID 2 */
    unsigned char is_setup;                /* Channel setup has been done */
};

/**
 * Channel Status
 */
struct channel_status
{
    unsigned long unused          : 28;
    unsigned long error           :  1; /* Last BD triggered an error:
                                           SDMA r/w */
    unsigned long opened_init     :  1; /* Channel is initialized:
                                           SDMA r/w */
    unsigned long state_direction :  1; /* SDMA is reading/writing (as seen
                                           from channel owner core) */
    unsigned long execute         :  1; /* Channel is being processed
                                           (started) or not */
};

/**
 * Channel control Block
 * SDMA ROM code expects these are 16-bytes each in an array
 * (MC0PTR + 16*CCR)
 */
struct channel_control_block
{
    /* current buffer descriptor processed: SDMA r/w */
    struct buffer_descriptor * volatile curr_bd_ptr;
    /* first element of buffer descriptor array: SDMA r */
    struct buffer_descriptor  *base_bd_ptr;
    /* pointer to the channel descriptor: SDMA ignored */
    struct channel_descriptor *channel_desc;
    /* open/close ; started/stopped ; read/write: SDMA r/w */ 
    volatile struct channel_status status;
};

/**
 * Channel context structure.
 */

/* Channel state bits on SDMA core side */
struct state_registers
{
    /* Offset 0 */
    unsigned long pc      : 14; /* program counter */  
    unsigned long unused0 :  1;
    unsigned long t       :  1; /* test bit: status of arithmetic & test
                                   instruction */
    unsigned long rpc     : 14; /* return program counter */
    unsigned long unused1 :  1;
    unsigned long sf      :  1; /* source fault while loading data */
    /* Offset 1 */
    unsigned long spc     : 14; /* loop start program counter */
    unsigned long unused2 :  1;
    unsigned long df      :  1; /* destination falut while storing data */
    unsigned long epc     : 14; /* loop end program counter */
    unsigned long lm      :  2; /* loop mode */
};

/* Context data saved for every channel on the SDMA core. This is 32 words
 * long which is specified in the SDMA initialization on the AP side. The
 * SDMA scripts utilize the scratch space. */
struct context_data
{
    struct state_registers channel_state; /* channel state bits */
    union
    {
        unsigned long r[8];            /* general registers (r0-r7) */
        struct                         /* script usage of said when
                                          setting contexts */
        {
            unsigned long event_mask2; /* 08h */
            unsigned long event_mask1; /* 0Ch */
            unsigned long r2;          /* 10h */
            unsigned long r3;          /* 14h */
            unsigned long r4;          /* 18h */
            unsigned long r5;          /* 1Ch */
            unsigned long shp_addr;    /* 20h */
            unsigned long wml;         /* 24h */
        };
    };
    unsigned long mda; /* burst dma destination address register */
    unsigned long msa; /* burst dma source address register */
    unsigned long ms;  /* burst dma status register */
    unsigned long md;  /* burst dma data register */
    unsigned long pda; /* peripheral dma destination address register */
    unsigned long psa; /* peripheral dma source address register */
    unsigned long ps;  /* peripheral dma status register */
    unsigned long pd;  /* peripheral dma data register */
    unsigned long ca;  /* CRC polynomial register */
    unsigned long cs;  /* CRC accumulator register */
    unsigned long dda; /* dedicated core destination address register */
    unsigned long dsa; /* dedicated core source address register */
    unsigned long ds;  /* dedicated core status register */
    unsigned long dd;  /* dedicated core data register */
    unsigned long scratch[8]; /* channel context scratch RAM */
};

/**
 * This structure holds the necessary data for the assignment in the 
 * dynamic channel-script association
 */
struct script_data
{
    unsigned long load_address; /* start address of the script */	
    unsigned long wml;		    /* parameters for the channel descriptor */
    unsigned long shp_addr;     /* shared peripheral base address */
    unsigned long event_mask1;  /* first event mask */
    unsigned long event_mask2;  /* second event mask */
};

/**
 * This structure holds the the useful bits of the CONFIG register
 */
struct configs_data
{
  unsigned long dspdma : 1; /* indicates if the DSPDMA is used */
  unsigned long rtdobs : 1; /* indicates if Real-Time Debug pins are
                               enabled */
  unsigned long acr    : 1; /* indicates if AHB freq /core freq = 2 or 1 */
  unsigned long csm    : 2; /* indicates which context switch mode is
                               selected */
};

#endif /* SDMA_STRUCT_H */
