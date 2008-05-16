-c 
-x 
-stack 0x1000 
-heap  0x100 
-l rts500.lib
 
MEMORY
{
PAGE 0:
    DARAM: origin =     80h, length =  7F80h
    SARAM: origin =   8000h, length =  4000h
}

SECTIONS
{
    .text   PAGE 0
    .cinit  PAGE 0
    .switch PAGE 0

    .bss    PAGE 0
    .const  PAGE 0
    .sysmem PAGE 0
    .stack  PAGE 0

    .vectors : PAGE 0 load = 7F80h

    /* DMA buffers for ABU mode must start on a 2*size boundary. */
    .dma : PAGE 0 load = 0x8000
}
