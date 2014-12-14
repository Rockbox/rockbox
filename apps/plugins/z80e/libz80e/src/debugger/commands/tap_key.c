#include <strings.h>
#include "debugger/commands.h"
#include "debugger/debugger.h"
#include "ti/hardware/keyboard.h"
#include "debugger/keys.h"

int command_tap_key(debugger_state_t *state, int argc, char **argv) {
	if (argc != 2) {
		state->print(state, "%s - Depress the specified key code, in hex or by name.\n", argv[0]);
		return 0;
	}
	uint8_t key;

	int i, found = 0;
	for (i = 0; i < sizeof(key_strings) / sizeof(key_string_t); ++i) {
		if (strcasecmp(key_strings[i].key, argv[1]) == 0) {
			key = key_strings[i].value;
			found = 1;
			break;
		}
	}
	if (!found) {
		key = parse_expression(state, argv[1]);
	}

	depress_key(state->asic->cpu->devices[0x01].device, key);
	char *_argv[] = { "run", "1000" };
	command_run(state, 2, _argv);
	release_key(state->asic->cpu->devices[0x01].device, key);
	char *__argv[] = { "run" };
	return command_run(state, 1, __argv);
}
