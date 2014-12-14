#include "ti/memory.h"
#include "core/cpu.h"
#include "debugger/commands.h"
#include "debugger/debugger.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

uint16_t parse_operand(debugger_state_t *state, const char *start, const char **end) {
	if (*start >= '0' && *start <= '9') {
		return strtol(start, (char **)end, 0);
	} else {
#define REGISTER(num, len, print) \
if (strncasecmp(start, print, len) == 0) {\
	*end += len; \
	return state->asic->cpu->registers. num; \
}
		REGISTER(IXH, 3, "IXH");
		REGISTER(IXL, 3, "IXL");
		REGISTER(IYH, 3, "IYH");
		REGISTER(IYL, 3, "IYL");
		REGISTER(_BC, 3, "BC'");
		REGISTER(_DE, 3, "DE'");
		REGISTER(_HL, 3, "HL'");
		REGISTER(_AF, 3, "AF'");
		REGISTER(IX, 2, "IX");
		REGISTER(IY, 2, "IY");
		REGISTER(AF, 2, "AF");
		REGISTER(BC, 2, "BC");
		REGISTER(DE, 2, "DE");
		REGISTER(HL, 2, "HL");
		REGISTER(PC, 2, "PC");
		REGISTER(SP, 2, "SP");
		REGISTER(A, 1, "A");
		REGISTER(B, 1, "B");
		REGISTER(C, 1, "C");
		REGISTER(D, 1, "D");
		REGISTER(E, 1, "E");
		REGISTER(F, 1, "F");
		REGISTER(H, 1, "H");
		REGISTER(L, 1, "L");
		REGISTER(I, 1, "I");
		REGISTER(R, 1, "R");

		state->print(state, "ERROR: Unknown register/number!\n");
		while(!strchr("+-*/(){} 	\n", *start)) {
			start++;
		}
		*end = 0;
		return 0;
	}
}

int precedence(char op) {
	switch (op) {
	case '+':
	case '-':
		return 1;
	case '*':
	case '/':
	case '%':
		return 2;
	case '(':
		return -2;
	case ')':
		return -3;
	case '{':
		return -4;
	case '}':
		return -5;
	default:
		return -1;
	}
}

void print_stack(uint16_t *a, int b, char *c, int d) {
	printf("Stack: ");
	while (b != 0) {
		b--;
		printf("%04X (%u) ", a[b], a[b]);
	}

	printf("\nOperator: ");

	while(d != 0) {
		printf("%c ", c[--d]);
	}

	printf("\n");
}

uint16_t run_operator(char operator, uint16_t arg2, uint16_t arg1) {
	switch(operator) {
	case '+':
		return arg1 + arg2;
	case '-':
		return arg1 - arg2;
	case '*':
		return arg1 * arg2;
	case '/':
		return arg1 / arg2;
	case '%':
		return arg1 % arg2;
	default:
		return 0;
	}
}

uint16_t parse_expression(debugger_state_t *state, const char *string) {
	uint16_t value_stack[20];
	int value_stack_pos = 0;

	char operator_stack[20];
	int operator_stack_pos = 0;

	while (isspace(*string)) {
		string++;
	}

	while (*string != 0) {
		if (strchr("+-*/%(){}", *string)) {
			int op = precedence(*string);

			if (op == -2 || op == -4) { // left parenthesis
				operator_stack[operator_stack_pos++] = *string;
			} else if (op == -3) { // right parenthesis
				while (operator_stack_pos > 0) {
					char ch = operator_stack[operator_stack_pos - 1];
					int prec = precedence(ch);
					if (prec != -2 && value_stack_pos < 2) {
						state->print(state, "ERROR: Missing values!\n");
					} else if (prec != -2 && value_stack_pos > 1) {
						uint16_t first = value_stack[value_stack_pos - 1];
						uint16_t second = value_stack[value_stack_pos - 2];
						value_stack_pos -= 2; // popped values
						value_stack[value_stack_pos++] = run_operator(ch, first, second);
						operator_stack_pos--;
					} else {
						operator_stack_pos--;
						break;
					}
				}
			} else if (op == -5) { // right dereference
				while (operator_stack_pos > 0) {
					char ch = operator_stack[operator_stack_pos - 1];
					int prec = precedence(ch);
					if (prec != -4 && value_stack_pos < 2) {
						state->print(state, "ERROR: Missing values!\n");
					} else if (prec != -2 && value_stack_pos > 1) {
						uint16_t first = value_stack[value_stack_pos - 1];
						uint16_t second = value_stack[value_stack_pos - 2];
						value_stack_pos -= 2; // popped values
						value_stack[value_stack_pos++] = run_operator(ch, first, second);
						operator_stack_pos--;
					} else {
						operator_stack_pos--;
						break;
					}
				}

				if (value_stack_pos < 1) {
					state->print(state, "ERROR: Dereferencing failed!\n");
				} else {
					uint16_t memory = value_stack[value_stack_pos - 1];
					value_stack[value_stack_pos - 1] = state->asic->cpu->read_byte(state->asic->cpu->memory, memory);
				}
			} else {
				while (operator_stack_pos > 0) {
					char ch = operator_stack[operator_stack_pos - 1];
					int prec = precedence(ch);
					if (prec > op && value_stack_pos < 2) {
						state->print(state, "ERROR: Missing values!\n");
					} else if (prec > op) {
						uint16_t first = value_stack[value_stack_pos - 1];
						uint16_t second = value_stack[value_stack_pos - 2];
						value_stack_pos -= 2; // popped values
						value_stack[value_stack_pos++] = run_operator(ch, first, second);
						operator_stack_pos--;
					} else {
						break;
					}
				}
				operator_stack[operator_stack_pos++] = *string;
			}
			string++;
		} else {
			value_stack[value_stack_pos++] = parse_operand(state, string, &string);
			if (string == 0) {
				return 0;
			}
		}

		while (isspace(*string)) {
			string++;
		}
	}

	//	print_stack(value_stack, value_stack_pos, operator_stack, operator_stack_pos);

	while (operator_stack_pos > 0) {
		char ch = operator_stack[operator_stack_pos - 1];
		int prec = precedence(ch);

		if(prec == -2) {
			state->print(state, "ERROR: Mismatched parentheses!\n");
			return 0;
		} else if(prec == -4) {
			state->print(state, "ERROR: Mismatched dereference!\n");
			return 0;
		} else if(value_stack_pos < 2) {
			state->print(state, "ERROR: Missing values!\n");
			return 0;
		} else {
			uint16_t first = value_stack[value_stack_pos - 1];
			uint16_t second = value_stack[value_stack_pos - 2];
			value_stack_pos -= 2; // popped values
			value_stack[value_stack_pos++] = run_operator(ch, first, second);
			operator_stack_pos--;
		}
	}

	//	print_stack(value_stack, value_stack_pos, operator_stack, operator_stack_pos);

	if (value_stack_pos > 0) {
		return value_stack[value_stack_pos - 1];
	} else {
		return 0;
	}
}
