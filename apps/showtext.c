/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <string.h>

#include "file.h"
#include "lcd.h"
#include "kernel.h"
#include "button.h"
#include "sprintf.h"

char *strcat(char *s1,
             const char *s2)
{
  char *s = s1;

  while (*s1)
    s1++;

  while ((*s1++ = *s2++))
    ;
  return s;
}


static int here=0;

char *singleshow(char *word)
{
    static unsigned char words[22];
    int len = strlen(word);

    if(len>=10) {
        if(len < 12 ) {
            lcd_clear_display();
            lcd_puts(0,0, word);
            strcpy(words, "");
            here=0;
            return words;
        }
        /* huuuge word, use two lines! */
        return NULL;
    }

    else if(here +1 + len <= 11) { /* 1 is for space */
        if(words[0])
            strcat(words, " ");
        strcat(words, word);
        here+=1+len;
        return NULL; /* no show right now */
    }
    else {
        lcd_clear_display();
        lcd_puts(0,0, words);
        strcpy(words, word);
        here=len;
        return words;
    }
}

#define SEP(x) (((x) == '\n') || ((x) == '\t') || ((x) == ' '))

void showtext(char *filename)
{
  static char textbuffer[1024];

  int fd;
  int size;
  char *ptr;
  char *end;
  unsigned char backup;
  char num[8];
  int count=0;
  int b;
  char *show;
  int delay = HZ;

  fd = open(filename, O_RDONLY);
  
  if(-1 == fd)
    return;

  do {
      size = read(fd, textbuffer, sizeof(textbuffer));

      ptr = textbuffer;
      while (size > 0) {
          while(ptr && *ptr && SEP(*ptr)) {
              ptr++;
              size--;
              count++;
          }
          end = ptr;
          
          while(end && *end && !SEP(*end)) {
              end++;
              count++;
          }

          backup = *end;
          *end = 0;


          show = singleshow(ptr);

          if(show) {
              snprintf(num, sizeof(num), "%d", count);
              lcd_puts(0,1, num);
          }

          *end = backup;
      
          ptr += (end - ptr);
          size -= (end - ptr);
          
          b = button_get(false);
          if(b) {
              size = -1;
              break;
          }
          if(show)
              sleep(delay);
    }
    

  } while(size>0);

  close(fd);

}
