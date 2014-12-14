#include "log/log.h"

#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

const char *loglevel_to_string(loglevel_t level) {
	switch (level) {
	case L_DEBUG:
		return "DEBUG";
	case L_INFO:
		return "INFO";
	case L_WARN:
		return "WARN";
	case L_ERROR:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

log_t *init_log(log_func func, void *data, int log_level) {
	log_t *log = calloc(sizeof(log_t), 1);
	log->log = func;
	log->data = data;
	log->logging_level = log_level;
	return log;
}

void log_message(log_t *log, loglevel_t level, const char *part, const char *format, ...) {
	if (log == 0) {
		return;
	}

	if (level > log->logging_level) {
		return;
	}

	va_list format_list;
	va_start(format_list, format);
	log->log(log->data, level, part, format, format_list);
}

void free_log(log_t *log) {
	free(log);
}
