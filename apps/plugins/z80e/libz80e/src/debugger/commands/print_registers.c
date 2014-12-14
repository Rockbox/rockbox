#include "debugger/debugger.h"
#include "core/cpu.h"
#include "core/registers.h"

int command_print_registers(struct debugger_state *state, int argc, char **argv) {
	if (argc != 1) {
		state->print(state, "print_registers - Print the contents of the emulator's registers\n"
				"This command will print the contents of the registers of the emulator\n"
				" at the time of running.\n");
		return 0;
	}

	z80cpu_t *cpu = state->asic->cpu;

	z80registers_t r = cpu->registers;
	state->print(state, "   AF: 0x%04X   BC: 0x%04X   DE: 0x%04X  HL: 0x%04X\n", r.AF, r.BC, r.DE, r.HL);
	state->print(state, "  'AF: 0x%04X  'BC: 0x%04X  'DE: 0x%04X 'HL: 0x%04X\n", r._AF, r._BC, r._DE, r._HL);
	state->print(state, "   PC: 0x%04X   SP: 0x%04X   IX: 0x%04X  IY: 0x%04X\n", r.PC, r.SP, r.IX, r.IY);
	state->print(state, "   IFF1: %d      IFF2: %d      IM %d\n", cpu->IFF1, cpu->IFF2, cpu->int_mode);
	state->print(state, "Flags: ");
	if (r.flags.S) state->print(state, "S ");
	if (r.flags.Z) state->print(state, "Z ");
	if (r.flags.H) state->print(state, "H ");
	if (r.flags.PV) state->print(state, "P/V ");
	if (r.flags.N) state->print(state, "N ");
	if (r.flags.C) state->print(state, "C ");
	if (r.F == 0) state->print(state, "None set");
	state->print(state, "\n");
	if (cpu->halted) state->print(state, "CPU halted\n");

	return 0;
}

