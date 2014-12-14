#include "core/cpu.h"

#include "debugger/commands.h"
#include "debugger/debugger.h"
#include "disassembler/disassemble.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

struct mmu_disassemble_memory {
	struct disassemble_memory mem;
	z80cpu_t *cpu;
	struct debugger_state *state;
};

int disassemble_print(struct disassemble_memory *s, const char *format, ...) {
	struct mmu_disassemble_memory *m = (struct mmu_disassemble_memory *)s;

	char buffer[50];

	va_list list;
	va_start(list, format);

	vsnprintf(buffer, 50, format, list);

	return m->state->print(m->state, "%s", buffer);
}

uint8_t disassemble_read(struct disassemble_memory *s, uint16_t pointer) {
	struct mmu_disassemble_memory *m = (struct mmu_disassemble_memory *)s;
	return m->cpu->read_byte(m->cpu->memory, pointer);
}

int command_disassemble(struct debugger_state *state, int argc, char **argv) {
	if (argc > 3) {
		state->print(state, "%s `start` `count` - Print the disassembled commands\n"
				" Prints `count` disassembled commands starting in memory from `start`.\n", argv[0]);
		return 0;
	}

	z80cpu_t *cpu = state->asic->cpu;

	uint16_t start = state->asic->cpu->registers.PC;
	uint16_t count = 10;

	if (argc > 1) {
		start = parse_expression(state, argv[1]);
	}
	if (argc > 2) {
		count = parse_expression(state, argv[2]);
	}

	uint16_t i = 0;

	struct mmu_disassemble_memory str = { { disassemble_read, start }, cpu, state };

	for (i = 0; i < count; i++) {
		state->print(state, "0x%04X: ", str.mem.current);
		parse_instruction(&(str.mem), disassemble_print);
		state->print(state, "\n");
	}

	return 0;
}
