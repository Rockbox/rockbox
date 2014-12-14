#include "debugger/debugger.h"
#include "core/cpu.h"
#include "core/registers.h"

int command_unhalt(struct debugger_state *state, int argc, char **argv) {
	if (argc != 1) {
		state->print(state, "unhalt - Unhalts the running CPU.\n");
		return 0;
	}

	z80cpu_t *cpu = state->asic->cpu;
	cpu->halted = 0;
	return 0;
}

