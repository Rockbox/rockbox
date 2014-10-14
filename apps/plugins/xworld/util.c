/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "plugin.h"
#include "lib/pluginlib_exit.h"
#include <stdarg.h>
#include "util.h"
#define DEBUG_FILE "/.rockbox/xworld/log"
uint16_t g_debugMask;
void debug(uint16_t cm, const char *msg, ...) {
	char buf[1024];
        static int fd=-1;
        if(fd<0)
            fd=rb->open(DEBUG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (cm & g_debugMask) {
		va_list va;
		va_start(va, msg);
		rb->vsnprintf(buf, 1024, msg, va);
		va_end(va);
		rb->logf("%s\n", buf);
                rb->write(fd, buf, rb->strlen(buf));
                rb->write(fd, "\n", 1);
	}
}

void error(const char *msg, ...) {
	char buf[1024];
	va_list va;
	va_start(va, msg);
	rb->vsnprintf(buf, 1024, msg, va);
	va_end(va);
	rb->splashf(HZ*2, "ERROR: %s!\n", buf);
	exit(-1);
}

void warning(const char *msg, ...) {
	char buf[1024];
	va_list va;
	va_start(va, msg);
	rb->vsnprintf(buf, 1024, msg, va);
	va_end(va);
	rb->splashf(HZ*2, "WARNING: %s!\n", buf);
}

void string_lower(char *p) {
	for (; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') {
			*p += 'a' - 'A';
		}
	}
}

void string_upper(char *p) {
	for (; *p; ++p) {
		if (*p >= 'a' && *p <= 'z') {
			*p += 'A' - 'a';
		}
	}
}
