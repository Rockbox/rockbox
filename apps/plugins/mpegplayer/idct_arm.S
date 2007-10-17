/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id $
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
    .global     mpeg2_idct_copy_arm
    .type       mpeg2_idct_copy_arm, %function
    .global     mpeg2_idct_add_arm
    .type       mpeg2_idct_add_arm, %function


/* Custom calling convention:
 * r0 contains block pointer and is non-volatile
 * all non-volatile c context saved and restored on its behalf
 */
.idct:
    add    r12, r0, #128
1:
    ldrsh  r1, [r0, #0]           /* d0 */
    ldrsh  r2, [r0, #2]           /* d1 */
    ldrsh  r3, [r0, #4]           /* d2 */
    ldrsh  r4, [r0, #6]           /* d3 */
    ldrsh  r5, [r0, #8]           /* d0 */
    ldrsh  r6, [r0, #10]          /* d1 */
    ldrsh  r7, [r0, #12]          /* d2 */
    ldrsh  r8, [r0, #14]          /* d3 */
    orrs   r9, r2, r3            
    orreqs r9, r4, r5            
    orreqs r9, r6, r7            
    cmpeq  r8, #0                
    bne    2f                    
    mov    r1, r1, asl #15       
    bic    r1, r1, #0x8000       
    orr    r1, r1, r1, lsr #16   
    str    r1, [r0], #4          
    str    r1, [r0], #4          
    str    r1, [r0], #4          
    str    r1, [r0], #4          
    cmp    r0, r12               
    blo    1b                    
    b      3f                    
2:
    mov    r1, r1, asl #11        /* r1 = d0 = (block[0] << 11) + 2048 */
    add    r1, r1, #2048         
    add    r1, r1, r3, asl #11    /* r1 = t0 = d0 + (block[2] << 11) */
    sub    r3, r1, r3, asl #12    /* r3 = t1 = d0 - (block[2] << 11) */
 
    add    r9, r2, r4             /* r9 = tmp = (d1+d3)*(1108/4) */
    add    r10, r9, r9, asl #2
    add    r10, r10, r9, asl #4  
    add    r9, r10, r9, asl #8   
 
    add    r10, r2, r2, asl #4    /* r2 = t2 = tmp + (d1*(1568/32)*8) */
    add    r2, r10, r2, asl #5   
    add    r2, r9, r2, asl #3    
 
    add    r10, r4, r4, asl #2    /* r4 = t3 = tmp - (d3*(3784/8)*2) */
    rsb    r10, r10, r4, asl #6  
    add    r4, r4, r10, asl #3   
    sub    r4, r9, r4, asl #1    
    /* t2 & t3 are 1/4 final value here */
    add    r1, r1, r2, asl #2     /* r1 = a0 = t0 + t2 */
    sub    r2, r1, r2, asl #3     /* r2 = a3 = t0 - t2 */
    add    r3, r3, r4, asl #2     /* r3 = a1 = t1 + t3 */
    sub    r4, r3, r4, asl #3     /* r4 = a2 = t1 - t3 */
 
    add    r9, r8, r5             /* r9 = tmp = 565*(d3 + d0) */
    add    r10, r9, r9, asl #4   
    add    r10, r10, r10, asl #5 
    add    r9, r10, r9, asl #2   
 
    add    r10, r5, r5, asl #4    /* r5 = t0 = tmp + (((2276/4)*d0)*4) */
    add    r10, r10, r10, asl #5 
    add    r5, r10, r5, asl #3   
    add    r5, r9, r5, asl #2    
 
    add    r10, r8, r8, asl #2    /* r8 = t1 = tmp - (((3406/2)*d3)*2) */
    add    r10, r10, r10, asl #4 
    add    r10, r10, r8, asl #7  
    rsb    r8, r8, r10, asl #3   
    sub    r8, r9, r8, asl #1    
 
    add    r9, r6, r7             /* r9 = tmp = (2408/8)*(d1 + d2) */
    add    r10, r9, r9, asl #3   
    add    r10, r10, r10, asl #5 
    add    r9, r10, r9, asl #2   
 
    add    r10, r7, r7, asl #3    /* r7 = t2 = (tmp*8) - 799*d2 */
    add    r10, r10, r7, asl #4  
    rsb    r7, r7, r10, asl #5   
    rsb    r7, r7, r9, asl #3    
 
    sub    r10, r6, r6, asl #4    /* r6 = t3 = (tmp*8) - 4017*d1 */
    sub    r10, r10, r6, asl #6  
    add    r10, r10, r6, asl #12 
    add    r6, r10, r6           
    rsb    r6, r6, r9, asl #3    
    /* t0 = r5, t1 = r8, t2 = r7, t3 = r6*/
    add    r9, r5, r7             /* r9 = b0 = t0 + t2 */
    add    r10, r8, r6            /* r10 = b3 = t1 + t3 */
    sub    r5, r5, r7             /* t0 -= t2 */
    sub    r8, r8, r6             /* t1 -= t3 */
    add    r6, r5, r8             /* r6 = t0 + t1 */
    sub    r7, r5, r8             /* r7 = t0 - t1 */
 
    add    r11, r6, r6, asr #2    /* r6 = b1 = r6*(181/128) */
    add    r11, r11, r11, asr #5 
    add    r6, r11, r6, asr #3   
    add    r11, r7, r7, asr #2    /* r7 = b2 = r7*(181/128) */
    add    r11, r11, r11, asr #5 
    add    r7, r11, r7, asr #3   
    /* r1 = a0, r3 = a1,   r4 = a2,   r2 = a3 */
    /* r9 = b0, r6 = b1*2, r7 = b2*2, r10 = b3 */
    add    r5, r1, r9             /* block[0] = (a0 + b0) >> 12 */
    mov    r5, r5, asr #12       
    strh   r5, [r0], #2          
    add    r8, r3, r6, asr #1     /* block[1] = (a1 + b1) >> 12 */
    mov    r8, r8, asr #12       
    strh   r8, [r0], #2          
    add    r5, r4, r7, asr #1     /* block[2] = (a2 + b2) >> 12 */
    mov    r5, r5, asr #12       
    strh   r5, [r0], #2          
    add    r8, r2, r10            /* block[3] = (a3 + b3) >> 12 */
    mov    r8, r8, asr #12       
    strh   r8, [r0], #2          
    sub    r5, r2, r10            /* block[4] = (a3 - b3) >> 12 */
    mov    r5, r5, asr #12       
    strh   r5, [r0], #2          
    sub    r8, r4, r7, asr #1     /* block[5] = (a2 - b2) >> 12 */
    mov    r8, r8, asr #12       
    strh   r8, [r0], #2          
    sub    r5, r3, r6, asr #1     /* block[6] = (a1 - b1) >> 12 */
    mov    r5, r5, asr #12       
    strh   r5, [r0], #2          
    sub    r8, r1, r9             /* block[7] = (a0 - b0) >> 12 */
    mov    r8, r8, asr #12       
    strh   r8, [r0], #2          
    cmp    r0, r12               
    blo    1b                    
3:
    sub    r0, r0, #128          
    add    r12, r0, #16          
4:
    ldrsh  r1, [r0, #0*8]         /* d0 */
    ldrsh  r2, [r0, #2*8]         /* d1 */
    ldrsh  r3, [r0, #4*8]         /* d2 */
    ldrsh  r4, [r0, #6*8]         /* d3 */
    ldrsh  r5, [r0, #8*8]         /* d0 */
    ldrsh  r6, [r0, #10*8]        /* d1 */
    ldrsh  r7, [r0, #12*8]        /* d2 */
    ldrsh  r8, [r0, #14*8]        /* d3 */
 
    mov    r1, r1, asl #11        /* r1 = d0 = (block[0] << 11) + 2048 */
    add    r1, r1, #65536        
    add    r1, r1, r3, asl #11    /* r1 = t0 = d0 + d2:(block[2] << 11) */
    sub    r3, r1, r3, asl #12    /* r3 = t1 = d0 - d2:(block[2] << 11) */
 
    add    r9, r2, r4             /* r9 = tmp = (d1+d3)*(1108/4) */
    add    r10, r9, r9, asl #2   
    add    r10, r10, r9, asl #4  
    add    r9, r10, r9, asl #8   
 
    add    r11, r2, r2, asl #4    /* r2 = t2 = tmp + (d1*(1568/32)*8) */
    add    r2, r11, r2, asl #5   
    add    r2, r9, r2, asl #3    
 
    add    r10, r4, r4, asl #2    /* r4 = t3 = tmp - (d3*(3784/8)*2) */
    rsb    r10, r10, r4, asl #6  
    add    r4, r4, r10, asl #3   
    sub    r4, r9, r4, asl #1    
    /* t2 & t3 are 1/4 final value here */
    add    r1, r1, r2, asl #2     /* r1 = a0 = t0 + t2 */
    sub    r2, r1, r2, asl #3     /* r2 = a3 = t0 - t2 */
    add    r3, r3, r4, asl #2     /* r3 = a1 = t1 + t3 */
    sub    r4, r3, r4, asl #3     /* r4 = a2 = t1 - t3 */
 
    add    r9, r8, r5             /* r9 = tmp = 565*(d3 + d0) */
    add    r10, r9, r9, asl #4   
    add    r10, r10, r10, asl #5 
    add    r9, r10, r9, asl #2   
 
    add    r10, r5, r5, asl #4    /* r5 = t0 = tmp + (((2276/4)*d0)*4) */
    add    r10, r10, r10, asl #5 
    add    r5, r10, r5, asl #3   
    add    r5, r9, r5, asl #2    
 
    add    r10, r8, r8, asl #2    /* r8 = t1 = tmp - (((3406/2)*d3)*2) */
    add    r10, r10, r10, asl #4 
    add    r10, r10, r8, asl #7  
    rsb    r8, r8, r10, asl #3   
    sub    r8, r9, r8, asl #1    
 
    add    r9, r6, r7             /* r9 = tmp = (2408/8)*(d1 + d2) */
    add    r10, r9, r9, asl #3   
    add    r10, r10, r10, asl #5 
    add    r9, r10, r9, asl #2   
 
    add    r10, r7, r7, asl #3    /* r7 = t2 = (tmp*8) - 799*d2 */
    add    r10, r10, r7, asl #4  
    rsb    r7, r7, r10, asl #5   
    rsb    r7, r7, r9, asl #3    
 
    sub    r10, r6, r6, asl #4    /* r6 = t3 = (tmp*8) - 4017*d1 */
    sub    r10, r10, r6, asl #6  
    add    r10, r10, r6, asl #12 
    add    r6, r10, r6           
    rsb    r6, r6, r9, asl #3    
                                    /* t0=r5, t1=r8, t2=r7, t3=r6*/
    add    r9, r5, r7             /* r9 = b0 = t0 + t2 */
    add    r10, r8, r6            /* r10 = b3 = t1 + t3 */
    sub    r5, r5, r7             /* t0 -= t2 */
    sub    r8, r8, r6             /* t1 -= t3 */
    add    r6, r5, r8             /* r6 = t0 + t1 */
    sub    r7, r5, r8             /* r7 = t0 - t1 */
 
    add    r11, r6, r6, asr #2    /* r6 = b1 = r5*(181/128) */
    add    r11, r11, r11, asr #5 
    add    r6, r11, r6, asr #3   
    add    r11, r7, r7, asr #2    /* r7 = b2 = r6*(181/128) */
    add    r11, r11, r11, asr #5 
    add    r7, r11, r7, asr #3   
    /* r1 = a0, r3 = a1,   r4 = a2,    r2 = a3 */
    /* r9 = b0, r6 = b1*2, r7 = b2*2, r10 = b3 */
    add    r5, r1, r9             /* block[0] = (a0 + b0) >> 17 */
    mov    r5, r5, asr #17       
    strh   r5, [r0, #0*8]        
    add    r8, r3, r6, asr #1     /* block[1] = (a1 + b1) >> 17 */
    mov    r8, r8, asr #17       
    strh   r8, [r0, #2*8]        
    add    r5, r4, r7, asr #1     /* block[2] = (a2 + b2) >> 17 */
    mov    r5, r5, asr #17       
    strh   r5, [r0, #4*8]        
    add    r8, r2, r10            /* block[3] = (a3 + b3) >> 17 */
    mov    r8, r8, asr #17       
    strh   r8, [r0, #6*8]        
    sub    r5, r2, r10            /* block[4] = (a3 - b3) >> 17 */
    mov    r5, r5, asr #17       
    strh   r5, [r0, #8*8]        
    sub    r8, r4, r7, asr #1     /* block[5] = (a2 - b2) >> 17 */
    mov    r8, r8, asr #17       
    strh   r8, [r0, #10*8]       
    sub    r5, r3, r6, asr #1     /* block[6] = (a1 - b1) >> 17 */
    mov    r5, r5, asr #17       
    strh   r5, [r0, #12*8]       
    sub    r8, r1, r9             /* block[7] = (a0 - b0) >> 17 */
    mov    r8, r8, asr #17       
    strh   r8, [r0, #14*8]       
    add    r0, r0, #2            
    cmp    r0, r12               
    blo    4b                    
    sub    r0, r0, #16           
    bx     lr
 
mpeg2_idct_copy_arm:
    stmfd  sp!, { r1-r2, r4-r12, lr }
    bl     .idct
    ldmfd  sp!, { r1-r2 }        
    mov    r11, #0               
    add    r12, r0, #128         
1:
    ldrsh  r3, [r0, #0]          
    ldrsh  r4, [r0, #2]          
    ldrsh  r5, [r0, #4]          
    ldrsh  r6, [r0, #6]          
    ldrsh  r7, [r0, #8]          
    ldrsh  r8, [r0, #10]         
    ldrsh  r9, [r0, #12]         
    ldrsh  r10, [r0, #14]        
    cmp    r3, #255              
    mvnhi  r3, r3, asr #31       
    strb   r3, [r1, #0]          
    str    r11, [r0], #4         
    cmp    r4, #255              
    mvnhi  r4, r4, asr #31       
    strb   r4, [r1, #1]          
    cmp    r5, #255              
    mvnhi  r5, r5, asr #31       
    strb   r5, [r1, #2]          
    str    r11, [r0], #4         
    cmp    r6, #255              
    mvnhi  r6, r6, asr #31       
    strb   r6, [r1, #3]          
    cmp    r7, #255              
    mvnhi  r7, r7, asr #31       
    strb   r7, [r1, #4]          
    str    r11, [r0], #4         
    cmp    r8, #255              
    mvnhi  r8, r8, asr #31       
    strb   r8, [r1, #5]          
    cmp    r9, #255              
    mvnhi  r9, r9, asr #31       
    strb   r9, [r1, #6]          
    str    r11, [r0], #4         
    cmp    r10, #255             
    mvnhi  r10, r10, asr #31     
    strb   r10, [r1, #7]         
    add    r1, r1, r2            
    cmp    r0, r12               
    blo    1b                    
    ldmfd  sp!, { r4-r12, pc }
 
mpeg2_idct_add_arm:
    cmp    r0, #129
    mov    r0, r1                
    ldreqsh r1, [r0, #0]         
    bne    1f                    
    and    r1, r1, #0x70         
    cmp    r1, #0x40             
    bne    3f                    
1:
    stmfd  sp!, { r2-r12, lr }
    bl     .idct                  
    ldmfd  sp!, { r1-r2 }        
    mov    r11, #0               
    add    r12, r0, #128         
2:
    ldrb   r3, [r1, #0]          
    ldrb   r4, [r1, #1]          
    ldrb   r5, [r1, #2]          
    ldrb   r6, [r1, #3]          
    ldrsh  r7, [r0, #0]          
    ldrsh  r8, [r0, #2]          
    ldrsh  r9, [r0, #4]          
    ldrsh  r10, [r0, #6]         
    add    r7, r7, r3            
    ldrb   r3, [r1, #4]          
    cmp    r7, #255              
    mvnhi  r7, r7, asr #31       
    strb   r7, [r1, #0]          
    ldrsh  r7, [r0, #8]          
    add    r8, r8, r4            
    ldrb   r4, [r1, #5]          
    cmp    r8, #255              
    mvnhi  r8, r8, asr #31       
    strb   r8, [r1, #1]          
    ldrsh  r8, [r0, #10]         
    add    r9, r9, r5            
    ldrb   r5, [r1, #6]          
    cmp    r9, #255              
    mvnhi  r9, r9, asr #31       
    strb   r9, [r1, #2]          
    ldrsh  r9, [r0, #12]         
    add    r10, r10, r6          
    ldrb   r6, [r1, #7]          
    cmp    r10, #255             
    mvnhi  r10, r10, asr #31     
    strb   r10, [r1, #3]         
    ldrsh  r10, [r0, #14]        
    str    r11, [r0], #4         
    add    r7, r7, r3            
    cmp    r7, #255              
    mvnhi  r7, r7, asr #31       
    strb   r7, [r1, #4]          
    str    r11, [r0], #4         
    add    r8, r8, r4            
    cmp    r8, #255              
    mvnhi  r8, r8, asr #31       
    strb   r8, [r1, #5]          
    str    r11, [r0], #4         
    add    r9, r9, r5            
    cmp    r9, #255              
    mvnhi  r9, r9, asr #31       
    strb   r9, [r1, #6]          
    add    r10, r10, r6          
    cmp    r10, #255             
    mvnhi  r10, r10, asr #31     
    strb   r10, [r1, #7]         
    str    r11, [r0], #4         
    add    r1, r1, r2            
    cmp    r0, r12               
    blo    2b                    
    ldmfd  sp!, { r4-r12, pc }   
3:
    stmfd  sp!, { r4-r11 }       
    ldrsh  r1, [r0, #0]           /* r1 = block[0] */
    mov    r11, #0               
    strh   r11, [r0, #0]          /* block[0] = 0 */
    strh   r11, [r0, #126]        /* block[63] = 0 */
    add    r1, r1, #64            /* r1 = DC << 7 */
    add    r0, r2, r3, asl #3    
4:
    ldrb   r4, [r2, #0]          
    ldrb   r5, [r2, #1]          
    ldrb   r6, [r2, #2]          
    ldrb   r7, [r2, #3]          
    ldrb   r8, [r2, #4]          
    ldrb   r9, [r2, #5]          
    ldrb   r10, [r2, #6]         
    ldrb   r11, [r2, #7]         
    add    r4, r4, r1, asr #7    
    cmp    r4, #255              
    mvnhi  r4, r4, asr #31       
    strb   r4, [r2, #0]          
    add    r5, r5, r1, asr #7    
    cmp    r5, #255              
    mvnhi  r5, r5, asr #31       
    strb   r5, [r2, #1]          
    add    r6, r6, r1, asr #7    
    cmp    r6, #255              
    mvnhi  r6, r6, asr #31       
    strb   r6, [r2, #2]          
    add    r7, r7, r1, asr #7    
    cmp    r7, #255              
    mvnhi  r7, r7, asr #31       
    strb   r7, [r2, #3]          
    add    r8, r8, r1, asr #7    
    cmp    r8, #255              
    mvnhi  r8, r8, asr #31       
    strb   r8, [r2, #4]          
    add    r9, r9, r1, asr #7    
    cmp    r9, #255              
    mvnhi  r9, r9, asr #31       
    strb   r9, [r2, #5]          
    add    r10, r10, r1, asr #7  
    cmp    r10, #255             
    mvnhi  r10, r10, asr #31     
    strb   r10, [r2, #6]         
    add    r11, r11, r1, asr #7  
    cmp    r11, #255             
    mvnhi  r11, r11, asr #31     
    strb   r11, [r2, #7]         
    add    r2, r2, r3            
    cmp    r2, r0                
    blo    4b                    
    ldmfd  sp!, { r4-r11 }       
    bx     lr
