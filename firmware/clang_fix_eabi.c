#include <stddef.h>

void __aeabi_memcpy8(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
}

void __aeabi_memcpy4(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
}

void __aeabi_memcpy(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
}


void __aeabi_memmove8(void *dest, const void *src, size_t n) {
    memmove(dest, src, n);
}

void __aeabi_memmove4(void *dest, const void *src, size_t n) {
    memmove(dest, src, n);
}

void __aeabi_memmove(void *dest, const void *src, size_t n) {
    memmove(dest, src, n);
}

/*
 * __aeabi_memset has the order of its second and third arguments reversed.
 *  This allows __aeabi_memclr to tail-call __aeabi_memset
 */
 
void __aeabi_memset8(void *dest, size_t n, int c) {
    memset(dest, c, n);
}

void __aeabi_memset4(void *dest, size_t n, int c) {
    memset(dest, c, n);
}

void __aeabi_memset(void *dest, size_t n, int c) {
    memset(dest, c, n);
}


void __aeabi_memclr8(void *dest, size_t n) {
    __aeabi_memset8(dest, n, 0);
}

void __aeabi_memclr4(void *dest, size_t n) {
    __aeabi_memset4(dest, n, 0);
}

void __aeabi_memclr(void *dest, size_t n) {
    __aeabi_memset(dest, n, 0);
}

