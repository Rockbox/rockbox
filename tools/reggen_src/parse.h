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
#ifndef REGGEN_PARSE_H
#define REGGEN_PARSE_H

#include <stdio.h>

#define PARSE_OK    1
#define PARSE_EMPTY 0
#define PARSE_ERR   (-1)

struct context;

int parse(struct context *ctx, const char *filename, FILE *input);

#endif /* REGGEN_PARSE_H */
