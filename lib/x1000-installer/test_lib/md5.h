#ifndef _MD5_H
#define _MD5_H

#include <stdint.h>

typedef struct
{
    uint32_t total[2];
    uint32_t state[4];
    uint8_t buffer[64];
}
md5_context;

void md5_starts( md5_context *ctx );
void md5_update( md5_context *ctx, uint8_t *input, uint32_t length );
void md5_finish( md5_context *ctx, uint8_t digest[16] );

#endif /* md5.h */
