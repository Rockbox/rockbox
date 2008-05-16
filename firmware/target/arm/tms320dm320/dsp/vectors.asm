;* Copyright (c) 2007, C.P.R. Baaij
;* All rights reserved.
;*
;* Redistribution and use in source and binary forms, with or without
;* modification, are permitted provided that the following conditions are met:
;*     * Redistributions of source code must retain the above copyright
;*       notice, this list of conditions and the following disclaimer.
;*     * Redistributions in binary form must reproduce the above copyright
;*       notice, this list of conditions and the following disclaimer in the
;*       documentation and/or other materials provided with the distribution.
;*
;* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY  EXPRESS OR 
;* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
;* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
;* IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
;* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
;* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
;* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
;* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
;* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

;*-----------------------------------------------------------------------------*
;*  Interrupt Vectors                                                          *
;*-----------------------------------------------------------------------------*
            .mmregs
            
            ; External Functions
            .global _handle_int0
            .global _c_int00
            .global _handle_dma0
            .global _handle_dmac

            .sect   ".vectors"
; Reset Interrupt
RS_V:       BD    _c_int00
            NOP
            NOP

; Non-Maskable Interrupt
NMI_V:      RETE
            NOP
            NOP
            NOP
            
; Software Interrupts
SINt17_V:   .space  4*16
SINt18_V:   .space  4*16
SINt19_V:   .space  4*16
SINt20_V:   .space  4*16
SINt21_V:   .space  4*16
SINt22_V:   .space  4*16
SINt23_V:   .space  4*16
SINt24_V:   .space  4*16
SINt25_V:   .space  4*16
SINt26_V:   .space  4*16
SINt27_V:   .space  4*16
SINt28_V:   .space  4*16
SINt29_V:   .space  4*16
SINt30_V:   .space  4*16
; INT0 - ARM Interrupting DSP via HPIB
INT0_V:     BD      _handle_int0
            NOP
            NOP
; INT1 - Interrupt is generated based on the settings of DSP_SYNC_STATE and 
; DSP_SYNC_MASK register of the coprocessor subsystem or when DSPINT1 bit in 
; CP_INTC is set.
INT1_V:     RETE
            NOP
            NOP
            NOP
; INT2 - Interrupt is generated when DSPINT2 bit in CP_INTC register of the 
; coprocessor subsystem is set.
INT2_V:     RETE
            NOP
            NOP
            NOP
; Timer Interrupt
TINT_V:     RETE
            NOP
            NOP
            NOP
; McBSP0 receive interrupt
BRINT0_V:   RETE
            NOP
            NOP
            NOP
; McBSP0 transmit interrupt
BXINT0_V:   RETE
            NOP
            NOP
            NOP
; DMA Channel-0 interrupt
DMAC0_V:    BD _handle_dma0
            NOP
            NOP
; DMA Channel-1 interrupt
DMAC1_V:    RETE
            NOP
            NOP
            NOP
; INT3 - Interrupt is generated when DSPINT3 bit in CP_INTC register of the 
; coprocessor subsystem is set or on write of any value to BRKPT_TRG
INT3_V:     RETE
            NOP
            NOP
            NOP
; HPIB HINT to DSP
HINT_V:     RETE
            NOP
            NOP
            NOP
; BRINT1/DMAC2 McBSP1 receive interrupt
BRINT1_V:   RETE
            NOP
            NOP
            NOP
; BXINT1/DMAC3 McBSP1 transmit interrupt
BXINT1_V:   RETE
            NOP
            NOP
            NOP
; DMA Channel-4 interrupt
DMAC4_V:    RETE
            NOP
            NOP
            NOP
; DMA Channel-5 interrupt
DMAC5_V:    RETE
            NOP
            NOP
            NOP
; HPIB DMAC interrupt
HPIB_DMA_V: BD _handle_dmac
            NOP
            NOP

; EHIF interrupt to DSP
EHIV_V:     RETE
            NOP
            NOP
            NOP
            .end
