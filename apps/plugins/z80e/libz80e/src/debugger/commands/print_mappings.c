#include "debugger/commands.h"
#include "debugger/debugger.h"

#include <string.h>

int command_print_mappings(struct debugger_state *state, int argc, char **argv) {
	if (argc > 2 || (argc == 2 && strcmp(argv[1], "-v") != 0)) {
		state->print(state, "%s [-v] - Print the MMU page mappings.\n The -v flag adds verbosity to the output.\n", argv[0]);
		state->print(state, " The terse output is formatted like this: \"Bank 0: F:00\".\n"
				" 'F' says the mapped page is a flash page, 'R' says the mapped page is a ram page.\n");
		return 0;
	}

	int verbose = argc > 1 && strcmp(argv[1], "-v") == 0;
	int i = 0;

	for (i = 0; i < 4; i++) {
		ti_mmu_bank_state_t mapping = state->asic->mmu->banks[i];
		if (verbose) {
			state->print(state, "Page %d (0x%04X - 0x%04X): mapped to %s page %d (0x%04X - 0x%04X).\n",
				i, i * 0x4000, (i + 1) * 0x4000 - 1, mapping.flash ? "ROM" : "RAM", mapping.page,
				mapping.page * 0x4000, (mapping.page + 1) * 0x4000 - 1);
		} else {
			state->print(state, "Bank %d: %c:%02X\n", i, mapping.flash ? 'F' : 'R', mapping.page);
		}
	}

	return 0;
}
