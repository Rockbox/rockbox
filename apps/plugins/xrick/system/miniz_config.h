/*
 * xrick/system/miniz_config.h
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza. All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#ifndef _MINIZ_CONFIG_H
#define _MINIZ_CONFIG_H

/*
 * miniz used only for crc32 calculation
 */
#define MINIZ_NO_STDIO
#define MINIZ_NO_TIME
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#define MINIZ_NO_ZLIB_APIS
#define MINIZ_NO_MALLOC
#ifdef ROCKBOX
#  define MINIZ_NO_ASSERT
#endif

#endif /* ndef _MINIZ_CONFIG_H */

/* eof */
