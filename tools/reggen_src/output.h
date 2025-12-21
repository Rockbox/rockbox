/*
 * This file is part of RegGen -- register definition generator
 * Copyright (C) 2025 Aidan MacDonald
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef REGGEN_OUTPUT_H
#define REGGEN_OUTPUT_H

#include "array.h"
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

struct output_file
{
    FILE *fstream;

    const char *filename;
    const char *include_guard_token;

    char *linebuf;
    size_t linesize;

    struct array lines;
    size_t num_lines;
};

void output_vaddcol(struct output_file *file, size_t column_nr,
                    const char *fmt, va_list ap_in);
void output_addcol(struct output_file *file, size_t column_nr,
                   const char *fmt, ...);
void output_newline(struct output_file *file);
void output_flush(struct output_file *file);
void output_free(struct output_file *file);

#endif /* REGGEN_OUTPUT_H */
