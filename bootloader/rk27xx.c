#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "backlight.h"
#include "backlight-target.h"
#include "font.h"
#include "common.h"
#include "version.h"

// 441 Hz samples table, 44100 Hz and 441 Hz -> 100 samples 
const int16_t samples[] = {
         0,   2057,   4106,   6139,   8148,  10125,  12062,  13951,  15785,  17557,
     19259,  20886,  22430,  23886,  25247,  26509,  27666,  28713,  29648,  30465,
     31163,  31737,  32186,  32508,  32702,  32767,  32702,  32508,  32186,  31737,
     31163,  30465,  29648,  28713,  27666,  26509,  25247,  23886,  22430,  20886,
     19259,  17557,  15785,  13951,  12062,  10125,   8148,   6139,   4106,   2057,
         0,  -2057,  -4106,  -6139,  -8148, -10125, -12062, -13951, -15785, -17557,
    -19259, -20886, -22430, -23886, -25247, -26509, -27666, -28713, -29648, -30465,
    -31163, -31737, -32186, -32508, -32702, -32767, -32702, -32508, -32186, -31737,
    -31163, -30465, -29648, -28713, -27666, -26509, -25247, -23886, -22430, -20886,
    -19259, -17557, -15785, -13951, -12062, -10125,  -8148,  -6139,  -4106,  -2057 };

extern int show_logo( void );

void INT_HDMA(void)
{
#if 0
//    static uint32_t i;
//    printf("hdma int: %d", i++);

    HDMA_ISRC0 = (uint32_t)&samples;
    HDMA_IDST0 = (uint32_t)&I2S_TXR;
    HDMA_ICNT0 = (sizeof(samples)/4) - 1;
    HDMA_CON0 = (1<<22)|  // slice mode
                (1<<21)|  // channel enable
                (1<<18)|  // interrupt mode
                (5<<13)|  // transfer mode inc8
                (6<<9) |  // hdreq from i2s tx
                (0<<7) |  // source address increment
                (1<<5) |  // destination address fixed
                (2<<3) |  // data size word
                (1<<0);   // enable hardware triggered dma

    HDMA_ISR = (1<<13) | // mask ch1 page overflow
               (1<<11) | // mask ch1 page count down
               (1<<9);   // mask ch1 interrupts
#endif
return;
}

static int codec_write(uint8_t reg, uint8_t data)
{
    uint8_t tmp = data;
    return i2c_write(0x27<<1, reg<<1, 1, &tmp);
}

void main(void)
{
    int i;

    _backlight_init();

    system_init();
    kernel_init();
    enable_irq();

    lcd_init_device();
    _backlight_on();
    font_init();
    lcd_setfont(FONT_SYSFIXED);

    show_logo();
    sleep(HZ*2);

printf("show logo passed");
    // I2S init
    SCU_CLKCFG &= ~((1<<17) | (1<<16)); // enable i2s, i2c pclk
//SCU_CLKCFG |= ((1<<17) | (1<<16));
    I2S_OPR = (1<<17) |    // reset Tx
              (1<<16) |    // reset Rx
              (1<<6)  |    // disable HDMA Req1
              (1<<5);      // disable HDMA Req2

    I2S_TXCTL = (1<<16) |  // LRCK/SCLK = 64
                (4<<8)  |  // MCLK/SCK = 4
                (1<<4)  |  // 16bit samples
                (0<<3)  |  // stereo mode
                (0<<1);    // I2S
 
    I2S_RXCTL = (1<<16) |  // LRCK/SCLK = 64
                (4<<8)  |  // MCLK/SCK = 4
                (1<<4)  |  // 16bit samples
                (0<<3)  |  // stereo mode
                (0<<1);    // I2S

    I2S_FIFOSTS = (1<<18) |  // Tx int trigger half full
                  (1<<16);   // Rx int trigger half full

    // I2S start
    I2S_OPR = (1<<17) | (1<<16);
    sleep(HZ/100);

    I2S_OPR = (0<<6) | // req channel 1 enable                               
              (1<<5) | // req channel 2 disable
              (0<<4) | // HDMA req channel 1 Tx
              (0<<2) | // normal I2S operation (no loopback)
              (1<<1);  // Tx start

printf("I2S config passed");

    HDMA_ISRC0 = (uint32_t)&samples;
    HDMA_IDST0 = (uint32_t)&I2S_TXR;
    HDMA_ICNT0 = (sizeof(samples)/4) - 1;
    HDMA_ISCNT0 = 7;
    HDMA_IPNCNTD0 = 1;
    HDMA_CON0 = (1<<22)|  // slice mode
                (1<<21)|  // channel enable
                (1<<18)|  // interrupt mode
                (5<<13)|  // transfer mode inc8
                (6<<9) |  // hdreq from i2s tx
                (0<<7) |  // source address increment
                (1<<5) |  // destination address fixed
                (2<<3) |  // data size word
                (1<<0);   // enable hardware triggered dma

    HDMA_ISR = (1<<13) | // mask ch1 page overflow
               (1<<11) | // mask ch1 page count down
               (1<<9);   // mask ch1 interrupts

    INTC_IMR |= (1<<12);
    INTC_IECR |= (1<<12);

printf("HDMA config passed");

    i2c_init();

printf("I2C config passed");

    // codec init
    codec_write(0x00, (1<<3)|(1<<2)|(1<<1)|(1<<0)); // AICR
    codec_write(0x01, (1<<7)|(1<<5)|(1<<3));        // CR1
    codec_write(0x02, (1<<2));                      // CR2
    codec_write(0x03, 0);                           // CCR1
    codec_write(0x04, (2<<4)|(2<<0));               // CCR2
    codec_write(0x07, (3<<5)|(3<<0));               // CCR


    codec_write(0x0f, 0x1f|(2<<6));                 // CGR6
    codec_write(0x14, (1<<1));                      // TR1
    codec_write(0x05, (1<<6)|(1<<5)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0)); // PMR1
    sleep(HZ/100);

    codec_write(0x06, (1<<3)|(1<<2)|(1<<0));               // PMR2


    codec_write(0x05, (1<<6)|(1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0));        // PMR1
    codec_write(0x05, (1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0));               // PMR1


    // DACout mode
    codec_write(0x01, (1<<7)|(1<<3)|(1<<5)|(1<<4));  // CR1
    codec_write(0x05, (1<<4)|(1<<3)|(1<<2)|(1<<1)|(1<<0)); //PMR1
//    codec_write(0x06, (1<<3)|(1<<2));                // PMR2

printf("codec init passed");

    codec_write(0x01, (1<<7)|(1<<3));         // CR1

    codec_write(0x0a, 0); // 0dB digital gain
    codec_write(0x11, 15|(2<<6)); // 

    while(1)
    {
        printf("HDMA_CCNT0: 0x%0x FIFOSTS: 0x%0x", HDMA_CCNT0, I2S_FIFOSTS);
    }
}
