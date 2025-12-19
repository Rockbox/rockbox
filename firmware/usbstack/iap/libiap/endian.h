#pragma once
#include <stdint.h>

__attribute__((unused)) static uint8_t swap_8(uint8_t num) {
    return num;
}

__attribute__((unused)) static uint16_t swap_16(uint16_t num) {
    return (num >> 8) | (num << 8);
}

__attribute__((unused)) static uint32_t swap_32(uint32_t num) {
    return (num & 0xFF000000) >> 24 |
           (num & 0x00FF0000) >> 8 |
           (num & 0x0000FF00) << 8 |
           (num & 0x000000FF) << 24;
}

__attribute__((unused)) static uint64_t swap_64(uint64_t num) {
    return (num & 0xFF00000000000000) >> 56 |
           (num & 0x00FF000000000000) >> 40 |
           (num & 0x0000FF0000000000) << 24 |
           (num & 0x000000FF00000000) << 8 |
           (num & 0x000000000FF00000) << 8 |
           (num & 0x00000000000FF000) << 24 |
           (num & 0x000000000000FF00) << 40 |
           (num & 0x00000000000000FF) << 56;
}
