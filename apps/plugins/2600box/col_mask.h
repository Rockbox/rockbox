#ifndef COL_MASK_H
#define COL_MASK_H

#define PL0_MASK  0x01
#define PL1_MASK  0x02
#define ML0_MASK  0x04
#define ML1_MASK  0x08
#define BL_MASK  0x10
#define PF_MASK  0x20
#define PF_MASK32  0x20202020

#define M0P0_MASK 0x01
#define M0P1_MASK 0x02

#define M1P1_MASK 0x04
#define M1P0_MASK 0x08

#define P0BL_MASK  0x10
#define P0PF_MASK  0x20

#define P1BL_MASK  0x40
#define P1PF_MASK  0x80

#define M0BL_MASK  0x100
#define M0PF_MASK  0x200

#define M1BL_MASK  0x400
#define M1PF_MASK  0x800

#define BLPF_MASK  0x1000

#define M0M1_MASK  0x2000
#define P0P1_MASK  0x4000

#define CXM0P_MASK   (M0P1_MASK | M0P0_MASK) 
#define CXM1P_MASK   (M1P0_MASK | M1P1_MASK) 
#define CXP0FB_MASK   (P0PF_MASK | P0BL_MASK) 
#define CXP1FB_MASK   (P1PF_MASK | P1BL_MASK) 
#define CXM0FB_MASK   (M0PF_MASK | M0BL_MASK) 
#define CXM1FB_MASK   (M1PF_MASK | M1BL_MASK) 
#define CXBLPF_MASK   BLPF_MASK 
#define CXPPMM_MASK   (P0P1_MASK | M0M1_MASK)

#endif

