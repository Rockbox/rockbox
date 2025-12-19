#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bool.h"

struct IAPSpan {
    uint8_t* ptr;
    size_t   size;
};

const void* iap_span_read(struct IAPSpan* span, size_t count);
void*       iap_span_alloc(struct IAPSpan* span, size_t count);
IAPBool     iap_span_append(struct IAPSpan* span, const void* ptr, size_t size);

#define iap_span_template(width)                                                 \
    IAPBool iap_span_peek_##width(struct IAPSpan* span, uint##width##_t* value); \
    IAPBool iap_span_read_##width(struct IAPSpan* span, uint##width##_t* value); \
    IAPBool iap_span_write_##width(struct IAPSpan* span, uint##width##_t value)
iap_span_template(8);
iap_span_template(16);
iap_span_template(32);
iap_span_template(64);
#undef iap_span_template

#ifdef __cplusplus
}
#endif
