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
#include "output.h"
#include <stdlib.h>
#include <string.h>

#define MAX_COLUMNS 16

struct output_column
{
    char *buffer;
    size_t length;
    size_t capacity;
};

struct output_line
{
    struct array columns;
};

static struct output_line *output_getline(struct output_file *file, size_t line_nr)
{
    struct output_line *line;

    /* Allocate line buffer if needed */
    while (line_nr >= array_size(&file->lines))
    {
        line = calloc(1, sizeof(*line));
        array_push(&file->lines, line);
    }

    /* Get line buffer */
    return array_get(&file->lines, line_nr);
}

static struct output_column *output_getcol(struct output_file *file, size_t column_nr)
{
    struct output_line *line = output_getline(file, file->num_lines);
    struct output_column *column;

    /* Allocate column buffer if needed */
    while (column_nr >= array_size(&line->columns))
    {
        column = calloc(1, sizeof(*column));
        array_push(&line->columns, column);
    }

    /* Get column buffer */
    return array_get(&line->columns, column_nr);
}

void output_vaddcol(struct output_file *file, size_t column_nr,
                    const char *fmt, va_list ap_in)
{
    struct output_column *column = output_getcol(file, column_nr);

    /* Append text to column buffer */
    for (;;)
    {
        size_t avail_len = column->capacity - column->length;
        va_list ap;
        int rc;

        va_copy(ap, ap_in);
        rc = vsnprintf(column->buffer + column->length, avail_len, fmt, ap);
        va_end(ap);

        /* TODO: report error? */
        if (rc < 0)
            break;

        if ((size_t)rc >= avail_len)
        {
            column->capacity += (size_t)rc - avail_len + 1;
            column->buffer = realloc(column->buffer, column->capacity);
            continue;
        }

        column->length += rc;
        break;
    }
}

void output_addcol(struct output_file *file, size_t column_nr,
                   const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    output_vaddcol(file, column_nr, fmt, ap);
    va_end(ap);
}

void output_newline(struct output_file *file)
{
    file->num_lines++;
}

void output_flush(struct output_file *file)
{
    /* Iterate over columns and compute max width of each column */
    size_t column_width[MAX_COLUMNS] = { 0 };
    for (size_t i = 0; i < file->num_lines; ++i)
    {
        if (i >= array_size(&file->lines))
            break;

        struct output_line *line = array_get(&file->lines, i);
        for (size_t j = 0; j < array_size(&line->columns); ++j)
        {
            struct output_column *column = array_get(&line->columns, j);
            if (column->length > column_width[j])
                column_width[j] = column->length;
        }
    }

    /* Calculate worst-case output buffer length for columnated data */
    size_t need_linesize = 0;
    for (size_t j = 0; j < MAX_COLUMNS; ++j)
    {
        /* Add 1 byte for space/newline */
        need_linesize += column_width[j] + 1;
    }

    /* Add 1 byte for null terminator */
    need_linesize += 1;

    /* Allocate output buffer */
    if (file->linesize < need_linesize)
    {
        file->linesize = need_linesize;
        file->linebuf = realloc(file->linebuf, file->linesize);
    }

    /* Write columns to the output with required amount of padding */
    for (size_t i = 0; i < file->num_lines; ++i)
    {
        struct output_line *line = NULL;
        if (i < array_size(&file->lines))
            line = array_get(&file->lines, i);

        int pos = 0;
        for (size_t j = 0; line && j < array_size(&line->columns); ++j)
        {
            struct output_column *column = array_get(&line->columns, j);
            size_t num_spaces = column_width[j] - column->length;

            /* Fill column data */
            memcpy(file->linebuf + pos, column->buffer, column->length);
            memset(file->linebuf + pos + column->length, ' ', num_spaces);

            /* Update position & clear column data for next run */
            pos += column_width[j];
            column->length = 0;
        }

        /* Strip trailing spaces & add a newline */
        while (pos > 0 && file->linebuf[pos-1] == ' ')
            pos--;

        file->linebuf[pos++] = '\n';

        /* Append to the output */
        if (file->fstream)
            fprintf(file->fstream, "%.*s", pos, file->linebuf);
    }

    /* Clear all used lines */
    file->num_lines = 0;
}

void output_free(struct output_file *file)
{
    struct output_line *line;
    ARRAY_FOREACH(&file->lines, line)
    {
        struct output_column *column;
        ARRAY_FOREACH(&line->columns, column)
        {
            free(column->buffer);
            free(column);
        }

        array_free(&line->columns);
        free(line);
    }

    array_free(&file->lines);
    free(file->linebuf);

    if (file->fstream)
        fclose(file->fstream);
}
