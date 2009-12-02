-c
-x 
-stack 0x1000 
-heap  0x100 

/* The rtx500.lib should be included if you want proper initialization, 
 *  currently the program is setup so that it is not necessary to save space.
 *  reset vector should jump to _c_int00 if initialization is needed.
 *
 * -l rts500.lib*/
 
MEMORY
{
PAGE 0:
    DARAM (RWIX): origin =  80h,    length =  7F00h
    VECS  (RIX) : origin =  7F80h,  length =  80h
    
    /* SARAM can be read and written to, but it cannot execute and does not need
     *  to be initialized. */
    SARAM (RW)  : origin =  8000h,  length =  4000h
}

SECTIONS
{
    .text   > DARAM PAGE 0
    .cinit  > DARAM PAGE 0
    .switch > DARAM PAGE 0
    .data   > DARAM PAGE 0

    .bss    > DARAM PAGE 0
    .const  > DARAM PAGE 0
    .sysmem > DARAM PAGE 0
    .stack  > DARAM PAGE 0

    .vectors > VECS PAGE 0

    /* DMA buffers for ABU mode must start on a 2*size boundary. */
    .dma    > SARAM PAGE 0
}
