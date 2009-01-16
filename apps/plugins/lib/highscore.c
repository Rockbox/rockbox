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
#include "plugin.h"
#include "highscore.h"

int highscore_save(char *filename, struct highscore *scores, int num_scores)
{
    int i;
    int fd;
    int rc;
    char buf[80];

    fd = rb->open(filename, O_WRONLY|O_CREAT);
    if(fd < 0)
        return -1;
    
    for(i = 0;i < num_scores;i++)
    {
        rb->snprintf(buf, sizeof(buf)-1, "%s:%d:%d\n",
                     scores[i].name, scores[i].score, scores[i].level);
        rc = rb->write(fd, buf, rb->strlen(buf));
        if(rc < 0)
        {
            rb->close(fd);
            return -2;
        }
    }
    rb->close(fd);
    return 0;
}

int highscore_load(char *filename, struct highscore *scores, int num_scores)
{
    int i;
    int fd;
    char buf[80];
    char *name, *score, *level;
    char *ptr;

    fd = rb->open(filename, O_RDONLY);

    rb->memset(scores, 0, sizeof(struct highscore)*(num_scores+1));
    
    if(fd < 0)
        return -1;

    i = -1;
    while(rb->read_line(fd, buf, sizeof(buf)-1) && i < num_scores)
    {
        i++;
        
        DEBUGF("%s\n", buf);
        name = buf;
        ptr = rb->strchr(buf, ':');
        if ( !ptr )
            continue;
        *ptr = 0;
        ptr++;
        
        rb->strncpy(scores[i].name, name, sizeof(scores[i].name));
        
        DEBUGF("%s\n", scores[i].name);
        score = ptr;
        
        ptr = rb->strchr(ptr, ':');
        if ( !ptr )
            continue;
        *ptr = 0;
        ptr++;
        
        scores[i].score = rb->atoi(score);
        
        level = ptr;
        scores[i].level = rb->atoi(level);
    }    
    return 0;
}

int highscore_update(int score, int level, struct highscore *scores, int num_scores)
{
	int i, j;
	int new = 0;
	
	/* look through the scores and see if this one is in the top ones */
	for(i = num_scores-1;i >= 0; i--)
    {
		if ((score > scores[i].score))
		{
			/* Move the rest down one... */
			if (i > 0)
			{
				for (j=1; j<=i; j++)
				{
					rb->memcpy((void *)&scores[j-1], (void *)&scores[j], sizeof(struct highscore));
				}
			}
			scores[i].score = score;
			scores[i].level = level;
			/* Need to sort out entering a name... maybe old three letter arcade style */
			new = 1;
			break;
		}
	}
	return new;
}
