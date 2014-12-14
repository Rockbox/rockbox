#include "debugger/commands.h"
#include "debugger/debugger.h"
#include "disassembler/disassemble.h"
#include "log/log.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

enum {
	REGISTER,
	MEMORY,
	EXECUTION,
};

enum {
	READ = (1 << 0),
	WRITE = (1 << 1)
};
int register_from_string(char *string) {
	#define REGISTER(num, len, print) \
		if (strncasecmp(string, print, len) == 0) {\
			return num; \
		}
	REGISTER(A, 1, "A");
	REGISTER(B, 1, "B");
	REGISTER(C, 1, "C");
	REGISTER(D, 1, "D");
	REGISTER(E, 1, "E");
	REGISTER(F, 1, "F");
	REGISTER(H, 1, "H");
	REGISTER(L, 1, "L");
	REGISTER(AF, 2, "AF");
	REGISTER(_AF, 3, "AF'");
	REGISTER(BC, 2, "BC");
	REGISTER(_BC, 3, "BC'");
	REGISTER(DE, 2, "DE");
	REGISTER(_DE, 3, "DE'");
	REGISTER(HL, 2, "HL");
	REGISTER(_HL, 3, "HL'");
	REGISTER(PC, 2, "PC");
	REGISTER(SP, 2, "SP");
	REGISTER(I, 1, "I");
	REGISTER(R, 1, "R");
	REGISTER(IXH, 3, "IXH");
	REGISTER(IXL, 3, "IXL");
	REGISTER(IX, 2, "IX");
	REGISTER(IYH, 3, "IYH");
	REGISTER(IYL, 3, "IYL");
	REGISTER(IY, 2, "IY");

	return -1;
}

typedef struct {
	int look_for;
	debugger_state_t *deb_sta;
	char *exec_string;
} on_state_t;

uint16_t command_on_register_hook(void *state, registers reg, uint16_t value) {
	on_state_t *data = state;
	debugger_exec(data->deb_sta, data->exec_string);
	return value;
}

uint8_t command_on_memory_hook(void *state, uint16_t location, uint8_t value) {
	on_state_t *data = state;
	debugger_exec(data->deb_sta, data->exec_string);
	return value;
}

int command_on(struct debugger_state *state, int argc, char **argv) {
	if (argc < 5) {
		printf("%s `type` `read|write` `value` `command`\n"
			" Run a command on a specific case\n"
			" The type can be memory / register\n"
			" The value can be a register, or an expression\n",
			argv[0]);
		return 0;
	}

	int thing = 0;
	if (*argv[2] == 'r') {
		thing = READ;
	}

	if (*argv[2] == 'w') {
		thing = WRITE;
	}

	if (strncasecmp(argv[2], "rw", 2) == 0) {
		thing = READ | WRITE;
	}

	if (thing == 0) {
		state->print(state, "ERROR: First argument must be read, write, or rw\n");
		return 1;
	}

	on_state_t *sta = malloc(sizeof(on_state_t));
	sta->deb_sta = state->create_new_state(state, argv[4]);
	sta->exec_string = malloc(strlen(argv[4]) + 1);
	strcpy(sta->exec_string, argv[4]);

	if (strncasecmp(argv[1], "register", 8) == 0) {
		sta->look_for = register_from_string(argv[3]);
		if (sta->look_for == -1) {
			state->print(state, "ERROR: Invalid register!\n");
			free(sta);
			return 1;
		}
		if (thing & READ) {
			hook_add_register_read(state->asic->hook, sta->look_for, sta, command_on_register_hook);
		}
		if (thing & WRITE) {
			hook_add_register_write(state->asic->hook, sta->look_for, sta, command_on_register_hook);
		}
	} else if (strncasecmp(argv[1], "memory", 6) == 0) {
		sta->look_for = parse_expression(state, argv[3]);
		if (thing & READ) {
			hook_add_memory_read(state->asic->hook, sta->look_for, sta->look_for, sta, command_on_memory_hook);
		}
		if (thing & WRITE) {
			hook_add_memory_write(state->asic->hook, sta->look_for, sta->look_for, sta, command_on_memory_hook);
		}
	} else {
		free(sta);
		state->print(state, "ERROR: Second argument must be memory or register!\n");
		return 1;
	}

	return 0;
}

struct break_data {
	uint16_t address;
	asic_t *asic;
	int hook_id;
	int count;
	int log;
};

void break_callback(struct break_data *data, uint16_t address) {
	if(data->address != address) {
		return;
	}

	if (data->log) {
		log_message(data->asic->log, L_DEBUG, "break", "Breakpoint hit at 0x%04X", address);
	}

	data->asic->stopped = 1;

	if (data->count != -1 && !(--data->count)) {
		hook_remove_before_execution(data->asic->hook, data->hook_id);
	}
}

int command_break(struct debugger_state *state, int argc, char **argv) {
	if (argc != 2 && argc != 3) {
		state->print(state, "%s `address` [count] - break at address\n", argv[0]);
		return 0;
	}

	uint16_t address = parse_expression(state, argv[1]);

	int count = -1;
	if (argc == 3) {
		count = parse_expression(state, argv[2]);
	}

	struct break_data *data = malloc(sizeof(struct break_data));
	data->address = address;
	data->asic = state->asic;
	data->count = count;
	data->log = 1;
	data->hook_id = hook_add_before_execution(state->asic->hook, data, (hook_execution_callback)break_callback);
	return 0;
}

typedef struct {
	ti_mmu_t *mmu;
	debugger_state_t *state;
} command_step_over_dism_extra_t;

uint8_t step_over_read_byte(struct disassemble_memory *dmem, uint16_t mem) {
	command_step_over_dism_extra_t *extra = dmem->extra_data;
	return ti_read_byte(extra->mmu, mem);
}

int step_over_disasm_write(struct disassemble_memory *mem, const char *thing, ...) {
	command_step_over_dism_extra_t *extra = mem->extra_data;
	if (extra->state->debugger->flags.echo) {
		va_list list;
		va_start(list, thing);
		return extra->state->vprint(extra->state, thing, list);
	} else {
		return 0;
	}
}

int command_step_over(struct debugger_state *state, int argc, char **argv) {
	if (argc != 1) {
		state->print(state, "%s - set a breakpoint for the instruction after the current one\n", argv[0]);
		return 0;
	}
	command_step_over_dism_extra_t extra = { state->asic->mmu, state };
	struct disassemble_memory mem = { step_over_read_byte, state->asic->cpu->registers.PC, &extra };

	if (state->debugger->flags.echo) {
		state->print(state, "0x%04X: ", state->asic->cpu->registers.PC);
	}
	uint16_t size = parse_instruction(&mem, step_over_disasm_write);
	if (state->debugger->flags.echo) {
		state->print(state, "\n");
	}
	// Note: 0x18, 0xFE is JR $, i.e. an infinite loop, which we step over as a special case
	const uint8_t jumps[] = { 0x18, 0x28, 0x38, 0x30, 0x20 };
	int i;
	for (i = 0; i < sizeof(jumps) / sizeof(uint8_t); ++i) {
		if (cpu_read_byte(state->asic->cpu, state->asic->cpu->registers.PC) == jumps[i] &&
			cpu_read_byte(state->asic->cpu, state->asic->cpu->registers.PC + 1) == 0xFE) {
			state->asic->cpu->registers.PC += 2;
			return 0;
		}
	}
	if (state->debugger->flags.knightos) {
		if (cpu_read_byte(state->asic->cpu, state->asic->cpu->registers.PC) == 0xE7) {
			size += 2;
		}
	}

	struct break_data *data = malloc(sizeof(struct break_data));
	data->address = state->asic->cpu->registers.PC + size;
	data->asic = state->asic;
	data->count = 1;
	data->log = 0;
	data->hook_id = hook_add_before_execution(state->asic->hook, data, (hook_execution_callback) break_callback);

	char *_argv[] = { "run" };
	int orig_echo = state->debugger->flags.echo;
	state->debugger->flags.echo = 0;

	int val = command_run(state, 1, _argv);
	state->debugger->flags.echo = orig_echo;
	return val;
}
