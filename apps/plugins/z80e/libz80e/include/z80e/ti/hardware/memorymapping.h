#include <z80e/ti/asic.h>

typedef struct {
	asic_t *asic;
	int map_mode;

	uint8_t bank_a_page;
	int bank_a_flash;

	uint8_t bank_b_page;
	int bank_b_flash;

	uint8_t ram_bank_page;
} memory_mapping_state_t;

void reload_mapping(memory_mapping_state_t *);
void init_mapping_ports(asic_t *asic);
void free_mapping_ports(asic_t *asic);
