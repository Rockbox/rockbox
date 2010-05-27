/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: tag_table.h 26292 2010-05-25 22:24:08Z bieber $
 *
 * Copyright (C) 2010 Robert Bieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef TAG_TABLE_H
#define TAG_TABLE_H

#ifdef __cplusplus
extern "C"
{
namespace wps
{
#endif


/*
 * Struct for tag parsing information
 * name   - The name of the tag, i.e. V for %V
 * params - A string specifying all of the tags parameters, each
 *          character representing a single parameter.  Valid
 *          characters for parameters are:
 *             I - Required integer
 *             i - Nullable integer
 *             S - Required string
 *             s - Nullable string
 *             F - Required file name
 *             f - Nullable file name
 *             C - Required WPS code
 *          Any nullable parameter may be replaced in the WPS file
 *          with a '-'.  To specify that parameters may be left off
 *          altogether, place a '|' in the parameter string.  For
 *          instance, with the parameter string...
 *             Ii|Ss
 *          one integer must be specified, one integer can be
 *          specified or set to default with '-', and the user can
 *          stop providing parameters at any time after that.
 *          To specify multiple instances of the same type, put a 
 *          number before the character.  For instance, the string...
 *             2s
 *          will specify two strings.  An asterisk (*) at the beginning of the
 *          string will specify that either all or none of the optional
 *
 */
struct tag_info
{

    char* name;
    char* params;

};

/* 
 * Finds a tag by name and returns its parameter list, or an empty
 * string if the tag is not found in the table
 */
char* find_tag(char* name);

/*
 * Determines whether a character is legal to escape or not.  If 
 * lookup is not found in the legal escape characters string, returns
 * false, otherwise returns true
 */
int find_escape_character(char lookup);

#endif /* TAG_TABLE_H */
