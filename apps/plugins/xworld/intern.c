#include "intern.h"
#include "awendian.h"
uint8_t scriptPtr_fetchByte(struct Ptr* p) {
    return *p->pc++;
}

uint16_t scriptPtr_fetchWord(struct Ptr* p) {
    uint16_t i = READ_BE_UINT16(p->pc);
    p->pc += 2;
    return i;
}
