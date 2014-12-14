#ifndef STATUS_H
#define STATUS_H
#include <z80e/core/cpu.h>
#include <z80e/ti/ti.h>
#include <z80e/ti/asic.h>

z80iodevice_t init_status(asic_t *asic);
void free_status(z80iodevice_t status);

#endif

