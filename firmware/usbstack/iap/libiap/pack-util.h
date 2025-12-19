#include <stdint.h>
#include <string.h>

#include "bool.h"
#include "macros.h"

__attribute__((unused)) static IAPBool pack_u8(uint8_t** data, size_t* size, uint8_t value) {
    check_ret(*size >= 1, iap_false);
    (*data)[0] = value;
    *data += 1;
    *size -= 1;
    return iap_true;
}

__attribute__((unused)) static IAPBool pack_u16(uint8_t** data, size_t* size, uint16_t value) {
    check_ret(*size >= 2, iap_false);
    (*data)[0] = value >> 8;
    (*data)[1] = value;
    *data += 2;
    *size -= 2;
    return iap_true;
}

__attribute__((unused)) static IAPBool pack_u32(uint8_t** data, size_t* size, uint32_t value) {
    check_ret(*size >= 2, iap_false);
    (*data)[0] = value >> 24;
    (*data)[1] = value >> 16;
    (*data)[2] = value >> 8;
    (*data)[3] = value;
    *data += 4;
    *size -= 4;
    return iap_true;
}

__attribute__((unused)) static IAPBool pack_data(uint8_t** data, size_t* size, const void* payload, size_t payload_size) {
    check_ret(*size >= payload_size, iap_false);
    memcpy(*data, payload, payload_size);
    *data += payload_size;
    *size -= payload_size;
    return iap_true;
}
