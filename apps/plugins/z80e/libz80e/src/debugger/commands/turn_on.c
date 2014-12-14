#include "debugger/commands.h"
#include "debugger/debugger.h"
#include "ti/hardware/interrupts.h"

int command_turn_on(debugger_state_t *state, int argc, char **argv) {
	if (argc != 1) {
		state->print(state, "%s - Interrupt the CPU and raise the 'on button' interrupt\n", argv[0]);
		return 0;
	}

	ti_interrupts_interrupt(state->asic->interrupts, INTERRUPT_ON_KEY);
	return 0;
}
