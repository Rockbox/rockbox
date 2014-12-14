#include "ti/asic.h"
#include "debugger/debugger.h"

int command_stop(struct debugger_state *state, int argc, char **argv) {
	state->asic->stopped = 1;
	return 0;
}
