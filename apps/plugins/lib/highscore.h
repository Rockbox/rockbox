/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Linus Nielsen Feltzing
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
#ifndef HIGHSCORE_H
#define HIGHSCORE_H

/* see rockblox.c for the example of usage. */

struct highscore
{
    char name[32];
    int score;
    int level; 
};

/* Saves the scores to a file
 *   - filename: name of the file to write the data to
 *   - scores  : scores to store
 *   - num_scores: number of the elements in the array 'scores'
 * Returns 0 on success or a negative value if an error occures
 */
int highscore_save(char *filename, struct highscore *scores, int num_scores);

/* Reads the scores from a file. The file must be a text file, each line
 * represents a score entry.
 *
 *   - filename: name of the file to read the data from
 *   - scores  : where to put the read data
 *   - num_scores: max number of the scores to read (array capacity)
 *
 * Returns 0 on success or a negative value if an error occures
 */
int highscore_load(char *filename, struct highscore *scores, int num_scores);

/* Inserts score and level into array of struct highscore in the
 * descending order of scores, i.e. higher scores are at lower array
 * indexes.
 *
 *   - score : the new score value to insert
 *   - level : the game level at which the score was reached
 *   - name  : the name of the new entry (whatever it means)
 *   - scores: the array of scores to insert the new value into
 *   - num_scores: number of elements in 'scores'
 *
 * Returns the 0-based position of the newly inserted score if it was
 * inserted. Returns a negative value if the score was not inserted
 * (i.e. it was less than the lowest score in the array).
 */
int highscore_update(int score, int level, const char *name,
                     struct highscore *scores, int num_scores);

/* Checks whether the new score would be inserted into the score table.
 * This function can be used to find out whether a score with the given
 * value would be inserted into the score table. If yes, the program
 * can collect the name of the entry from the user (if it's done that
 * way) and then really update the score table with 'highscore_update'.
 *
 *   - score : the score value to check
 *   - scores: the array of existing scores
 *   - num_scores: number of elements in 'scores'
 *
 * Returns true iff the given score would be inserted into the score
 * table by highscore_update.
 */
bool highscore_would_update(int score, struct highscore *scores,
                            int num_scores);

/* Displays a nice highscore table. In general the font is FONT_UI, but if
 * the highscore table doesn't fit on the the display size it uses 
 * FONT_SYSFIXED.
 * 
 *  - position : highlight position line
 *  - scores   : the array of existing scores
 *  - num_scores: number of elements in 'scores'
 *  - show_level: whether to display the level column or not
 */
void highscore_show(int position, struct highscore *scores, int num_scores,
                    bool show_level);
#endif
