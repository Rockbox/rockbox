#include "debugger/debugger.h"
#include "debugger/commands.h"

int command_in(struct debugger_state *state, int argc, char **argv) {
	if(argc != 2) {
		state->print(state, "%s `port` - read from port `port`.\n", argv[0]);
		return 0;
	}

	uint8_t port = parse_expression(state, argv[1]) & 0xFF;
	uint8_t val = 0;
	if (state->asic->cpu->devices[port].read_in != NULL) {
		val = state->asic->cpu->devices[port].read_in(state->asic->cpu->devices[port].device);
	}
	state->print(state, "port 0x%02X -> 0x%02X\n", port, val);
	return 0;
}

int command_out(struct debugger_state *state, int argc, char **argv) {
	if(argc != 3) {
		state->print(state, "%s `port` `value` - write `value` into port `port`.\n", argv[0]);
		return 0;
	}

	uint8_t port = parse_expression(state, argv[1]) & 0xFF;
	uint8_t val = parse_expression(state, argv[2]) & 0xFF;
	if (state->asic->cpu->devices[port].write_out != NULL) {
		state->asic->cpu->devices[port].write_out(state->asic->cpu->devices[port].device, val);
	}
	state->print(state, "port 0x%02X <- 0x%02X\n", port, val);
	return 0;
}
