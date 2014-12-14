#include <z80e/ti/asic.h>

typedef struct {
	asic_t *asic;
    uint8_t chip_size;
} flash_state_t;

void init_flash_ports(asic_t *asic);
void free_flash_ports(asic_t *asic);
