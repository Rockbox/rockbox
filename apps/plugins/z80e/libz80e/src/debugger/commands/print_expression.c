#include "debugger/commands.h"
#include "debugger/debugger.h"

#include <stdlib.h>
#include <string.h>

int command_print_expression(struct debugger_state *state, int argc, char **argv) {
	if (argc == 1) {
		state->print(state, "%s `expression` - Print an expression\n Don't forget to quote your expression!\n", argv[0]);
		return 0;
	}

	int strl = 0;
	int i;
	for (i = 0; i < argc - 1; i++) {
		strl += strlen(argv[i + 1]) + 1;
	}

	char *data = malloc(strl);
	char *dpointer = data;
	for (i = 0; i < argc - 1; i++) {
		strcpy(dpointer, argv[i + 1]);
		dpointer += strlen(argv[i + 1]);
		*(dpointer++) = ' ';
	}
	*(dpointer - 1) = 0;

	uint16_t expr = parse_expression(state, data);

	state->print(state, "%s = 0x%04X (%u)\n", data, expr, expr);

	free(data);
	return 0;
}
