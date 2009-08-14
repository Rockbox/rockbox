#ifndef RM_BYTESTREAM_H
#define RM_BYTESTREAM_H

#include <inttypes.h>

static inline void advance_buffer(uint8_t **buf, int val)
{
    *buf += val;
}

static inline uint8_t rm_get_uint8(uint8_t *buf)
{
    return (uint8_t)buf[0];
}

static inline uint16_t rm_get_uint16be(uint8_t *buf)
{
    return (uint16_t)((buf[0] << 8)|buf[1]);
}

static inline uint32_t rm_get_uint32be(uint8_t *buf)
{
    return (uint32_t)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

static inline uint16_t rm_get_uint16le(uint8_t *buf)
{
    return (uint16_t)((buf[1] << 8)|buf[0]);
}

static inline uint32_t rm_get_uint32le(uint8_t *buf)
{
    return (uint32_t)((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
}

#endif /* RM_BYTESTREAM_H */

