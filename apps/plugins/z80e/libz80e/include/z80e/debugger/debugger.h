#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdarg.h>
#include <stdio.h>

typedef struct debugger_state debugger_state_t;
typedef struct debugger debugger_t;
typedef int (*debugger_function_t)(debugger_state_t *, int, char **);

#include <z80e/ti/asic.h>
#include <z80e/log/log.h>

typedef struct {
	const char *name;
	debugger_function_t function;
	int priority;
	void *state;
} debugger_command_t;

typedef struct {
	int count;
	int capacity;
	debugger_command_t **commands;
} debugger_list_t;

struct debugger_state {
	int (*print)(debugger_state_t *, const char *, ...);
	int (*vprint)(debugger_state_t *, const char *, va_list);
	void *state;
	void *interface_state;
	asic_t *asic;
	debugger_t *debugger;
	debugger_state_t *(*create_new_state)(debugger_state_t *, const char *command_name);
	void (*close_window)(debugger_state_t *);
	log_t *log;
};

typedef enum {
	DEBUGGER_DISABLED,
	DEBUGGER_ENABLED,
	DEBUGGER_LONG_OPERATION,
	DEBUGGER_LONG_OPERATION_INTERRUPTABLE
} debugger_state;

struct debugger {
	struct {
		int echo : 1;
		int echo_reg : 1;
		int auto_on : 1;
		int knightos : 1;
		int nointonstep : 1;
	} flags;

	debugger_list_t commands;
	asic_t *asic;
	debugger_state state;
};

int debugger_source_rc(debugger_state_t *, const char *rc_name);

debugger_t *init_debugger(asic_t *);
void free_debugger(debugger_t *);

int find_best_command(debugger_t *, const char *, debugger_command_t **);
void register_command(debugger_t *, const char *, debugger_function_t, void *, int);

char **debugger_parse_commandline(const char *, int *);
int debugger_exec(debugger_state_t *, const char *);

#endif
