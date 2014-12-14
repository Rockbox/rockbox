#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

typedef enum {
	L_ERROR = 0,
	L_WARN = 1,
	L_INFO = 2,
	L_DEBUG = 3,
} loglevel_t;

typedef void (*log_func)(void *, loglevel_t, const char *, const char *, va_list);

typedef struct {
	void *data;
	log_func log;
	int logging_level;
} log_t;

log_t *init_log(log_func, void *, int);
void log_message(log_t *, loglevel_t, const char *, const char *, ...);
void free_log(log_t *);

#endif
