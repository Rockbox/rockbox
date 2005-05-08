/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Michiel van der Kolk 
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
extern int acceptedmask;

#define INVALID_STRIP 1
#define INVALID_MARK  2
#define INVALID_EXPERT 3

void check_accepted(struct token *tstream, int count);
void parse_stream(struct token *tstream, int count, int inv_mode);
int check_tokenstream(struct token *tstream, int inv_mode);
void parser_accept_rparen(void);
void parser_accept(int bitmask);
void parse_checktoken(void);
void parseCompareNum(void);
void parseCompareString(void);
void parseExpr(void);
void parseMExpr(void);
