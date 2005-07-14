/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Kjell Ericson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

struct coordinate {
  int x;
  int y;
};
struct rectangle {
  int x;
  int y;
  int width;
  int height;
};

void drawdots(int color, struct coordinate *coord, int count);
void drawdot(int color, int x, int y);
void drawrect(int color, int x1, int y1, int x2, int y2);
void drawrectangles(int color, struct rectangle *rects, int count);


void dots(int *colors, struct coordinate *points, int count);


