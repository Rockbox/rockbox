/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#ifndef _DEMAC_CONFIG_H
#define _DEMAC_CONFIG_H

/* Build-time choices for libdemac.
 * Note that this file is included by both .c and .S files. */

#ifdef ROCKBOX

#include "config.h"

#ifndef __ASSEMBLER__
#include "../lib/codeclib.h"
#include <codecs.h>
#endif

#define APE_OUTPUT_DEPTH 29

/* On PP5002 code should go into IRAM. Otherwise put the insane
 * filter buffer into IRAM as long as there is no better use. */
#if CONFIG_CPU == PP5002
#define ICODE_SECTION_DEMAC_ARM   .icode
#define ICODE_ATTR_DEMAC          ICODE_ATTR
#define IBSS_ATTR_DEMAC_INSANEBUF
#else
#define ICODE_SECTION_DEMAC_ARM   .text
#define ICODE_ATTR_DEMAC
#define IBSS_ATTR_DEMAC_INSANEBUF IBSS_ATTR
#endif

#else /* !ROCKBOX */

#define APE_OUTPUT_DEPTH (ape_ctx->bps)

#define IBSS_ATTR
#define IBSS_ATTR_DEMAC_INSANEBUF
#define ICONST_ATTR
#define ICODE_ATTR
#define ICODE_ATTR_DEMAC

#endif /* !ROCKBOX */

/* Defaults */

#ifndef UDIV32
#define UDIV32(a, b) (a / b)
#endif

#ifndef FILTER_HISTORY_SIZE
#define FILTER_HISTORY_SIZE 512
#endif

#ifndef PREDICTOR_HISTORY_SIZE
#define PREDICTOR_HISTORY_SIZE 512
#endif

#endif /* _DEMAC_CONFIG_H */
