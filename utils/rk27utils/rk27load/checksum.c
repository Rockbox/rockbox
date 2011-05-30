#include <stdint.h>
#include "checksum.h"

uint16_t checksum(void *buff, uint32_t size)
{
        uint32_t r2 = 0xffff;
        uint32_t r3 = 0;
        uint32_t i, j;

        for (i=0; i<size; i++) {
                r3 = 0x80;
                for (j=0; j<8; j++) {
                        if ((r2 & 0x8000) != 0) {
                                r2 <<= 17;
                                r2 >>= 16;
                                r2 ^= 0x1000;
                                r2 ^= 0x21;
                        }
                        else {
                                r2 <<= 17;
                                r2 >>= 16;
                        }

                        if ((((uint8_t *)buff)[i] & r3) != 0) {
                                r2 ^= 0x1000;
                                r2 ^= 0x21;
                        }

                        r3 >>= 1;
                }
        }

        return r2 & 0xffff;
}

