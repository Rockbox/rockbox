#include <string.h>

#include "endian.h"
#include "macros.h"
#include "span.h"
#include "unaligned.h"

const void* iap_span_read(struct IAPSpan* span, size_t count) {
    check_ret(span->size >= count, NULL);
    span->ptr += count;
    span->size -= count;
    return span->ptr - count;
}

void* iap_span_alloc(struct IAPSpan* span, size_t count) __attribute__((alias("iap_span_read")));

IAPBool iap_span_append(struct IAPSpan* span, const void* ptr, size_t size) {
    void* dest = iap_span_alloc(span, size);
    check_ret(dest != NULL, iap_false);
    memcpy(dest, ptr, size);
    return iap_true;
}

#define iap_span_template(width)                                                  \
    IAPBool iap_span_peek_##width(struct IAPSpan* span, uint##width##_t* value) { \
        check_ret(span->size >= width / 8, iap_false);                            \
        *value = swap_##width(*(uu##width*)span->ptr);                            \
        return iap_true;                                                          \
    }                                                                             \
    IAPBool iap_span_read_##width(struct IAPSpan* span, uint##width##_t* value) { \
        const uu##width* ptr = iap_span_read(span, width / 8);                    \
        check_ret(ptr != NULL, iap_false);                                        \
        *value = swap_##width(*ptr);                                              \
        return iap_true;                                                          \
    }                                                                             \
    IAPBool iap_span_write_##width(struct IAPSpan* span, uint##width##_t value) { \
        uint##width##_t* ptr = iap_span_alloc(span, width / 8);                   \
        check_ret(ptr != NULL, iap_false);                                        \
        *ptr = swap_##width(value);                                               \
        return iap_true;                                                          \
    }
iap_span_template(8);
iap_span_template(16);
iap_span_template(32);
iap_span_template(64);
#undef iap_span_template
