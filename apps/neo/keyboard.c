
/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 by an Open Neo author (FILL IN)
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <string.h>

#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "version.h"
#include "sprintf.h"
#include "lcd-charset.h"
#include "lang.h"
#include "debug.h"

#define KEYBOARD_MAX_LENGTH 255

static unsigned char* kbd_screens[3] = {
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
  "abcdefghijklmnopqrstuvwxyz",
  " !\"#$%&'()*+,-./0123456789;<=>?@[]^_`{|}"
};

static unsigned char* kbd_screens_names[3] = {
  "Capitals",
  "Small",
  "Others"
};

static void kbd_show_legend( int nb )
{
  char buf[24];
  snprintf(buf, sizeof(buf), "[%s]", kbd_screens_names[nb] );
  lcd_puts( 0, 1, buf );  
  lcd_puts( 0, 2, kbd_screens[nb] );
  lcd_puts( 0, 3, &kbd_screens[nb][20] );
}

/* returns text len 
   Max = KEYBOARD_MAX_LENGTH characters
*/
int kbd_input( char* text, int buflen )
{
  char* pstart;
  char* pcursor;
  char* pold;
  int bufferlen;
  char cursorpos = 0;
  int ret = 0;
  char buffer[KEYBOARD_MAX_LENGTH+1];
  bool done = false;
  int key;
  int screen = 0;
  int screenidx = -1;
  unsigned char * pcurscreen = kbd_screens[0];
  bool ctl;

  bufferlen = strlen(text);
  
  if( bufferlen > KEYBOARD_MAX_LENGTH )
    bufferlen = KEYBOARD_MAX_LENGTH;
  
  strncpy( buffer, text, bufferlen );
  buffer[bufferlen] = 0;

  lcd_clear_display();

  /* Initial setup */
  lcd_puts( 0, 0, buffer );
  kbd_show_legend( screen );
  lcd_cursor( cursorpos, 0 );
  lcd_write(true,LCD_BLINKCUR);
  
  pstart = pcursor = buffer;

  while(!done) {
      /* We want all the keys except the releases and the repeats */
      key = button_get(true);
    
      if( key & BUTTON_IR)
          ctl = key & (NEO_IR_BUTTON_PLAY|NEO_IR_BUTTON_STOP|NEO_IR_BUTTON_BROWSE);
      else
          ctl = key & (BUTTON_PLAY|BUTTON_STOP|BUTTON_MENU);

      if( ctl ) {
          /* These key do not change the first line */
          switch( key ) {
              case BUTTON_MENU:
              case BUTTON_IR|NEO_IR_BUTTON_BROWSE:
	
                  /* Toggle legend screen */
                  screen++;
                  if( screen == 3 )
                      screen = 0;

                  pcurscreen = kbd_screens[screen];

                  screenidx = -1;
                  kbd_show_legend( screen );
                  
                  /* Restore cursor */
                  lcd_cursor( cursorpos, 0 );
                  break;
                  
              case BUTTON_PLAY:
              case BUTTON_IR|NEO_IR_BUTTON_PLAY:
                  
                  if( bufferlen ) {
                      strncpy(text, buffer, bufferlen);
                      text[bufferlen] = 0;
                      ret = bufferlen;
                  }
                  /* fallthrough */
                  
              case BUTTON_STOP:
              case BUTTON_IR|NEO_IR_BUTTON_STOP:
                  
                  /* Remove blinking cursor */
                  lcd_write(true,LCD_OFFCUR);
                  done = true;
          }
      }
      else {
      
          switch( key ) {
	
              case BUTTON_PROGRAM:
              case BUTTON_PROGRAM|BUTTON_REPEAT:
              case BUTTON_IR|NEO_IR_BUTTON_PROGRAM:
	
                  /* Delete char at pcursor */
                  /* Check if we are at the last char */

                  if( *(pcursor+1) != 0 ) {
                      /* move rest of the string to the left in buffer */
                      pold = pcursor;
                      while( *pcursor ){
                          *pcursor = *(pcursor+1);
                          pcursor++;
                      }
	  
                      /* Restore position */
                      pcursor = pold;
                  }
                  else {
                      *pcursor = 0;
                      pcursor--;
                      cursorpos--;
                  }
	  
                  bufferlen--;
                  break;
                  
              case BUTTON_IR|NEO_IR_BUTTON_EQ:
              case BUTTON_SELECT|BUTTON_LEFT:
                  
                  /* Insert left */

                  if(bufferlen >=  KEYBOARD_MAX_LENGTH )
                      break;
                  
                  pold = pcursor;
	
                  /* Goto end */
                  while( *pcursor )
                      pcursor++;

                  /* Move string content to the right */
                  while( pcursor >= pold ){
                      *(pcursor+1) = *pcursor;
                      pcursor--;
                  }

                  pcursor = pold;
                  *pcursor = ' ';
                  
                  bufferlen++;
                  break;
	
              case BUTTON_IR|NEO_IR_BUTTON_MUTE:
              case BUTTON_SELECT|BUTTON_RIGHT:
	
                  /* Insert Right */
	
                  if(bufferlen >=  KEYBOARD_MAX_LENGTH )
                      break;
	
                  pold = pcursor;
	
                  /* Goto end */
                  while( *pcursor )
                      pcursor++;
	
                  /* Move string content to the right */
                  while( pcursor > pold ){
                      *(pcursor+1) = *pcursor;
                      pcursor--;
                  }
	
                  pcursor = pold;
                  *(pcursor+1) = ' ';
	
                  bufferlen++;
	
                  button_add( BUTTON_RIGHT );
                  break;

              case BUTTON_LEFT:
              case BUTTON_REPEAT|BUTTON_LEFT:
              case BUTTON_IR|NEO_IR_BUTTON_REWIND:
              case BUTTON_IR|NEO_IR_BUTTON_REWIND|BUTTON_REPEAT:

                  /* Move cursor left.  Shift text right if all the way to the
                     left */
                  
                  /* Check for start of string */
                  if( pcursor > buffer ) {
                      
                      screenidx = -1;
                      cursorpos--;
                      pcursor--;
                      
                      /* Check if were going off the screen */
                      if( cursorpos == -1 ) {
                          cursorpos = 0;
                          
                          /* Shift text right if we are */
                          pstart--;
                      }
                  }
                  break;
                  
              case BUTTON_RIGHT:
              case BUTTON_REPEAT|BUTTON_RIGHT:
              case BUTTON_IR|NEO_IR_BUTTON_FFORWARD:
              case BUTTON_IR|NEO_IR_BUTTON_FFORWARD|BUTTON_REPEAT:
                  
                  /* Move cursor right.  Shift text left if all the way to
                     the right */
    
                  /* Check for end of string */
                  if( *(pcursor+1) != 0 ) {
                      screenidx = -1;
                      cursorpos++;
                      pcursor++;
	
                      /* Check if were going of the screen */
                      if( cursorpos == 20 ) {
                          cursorpos = 19;
	    
                          /* Shift text left if we are */
                          pstart++;
                      }
                  }
                  break;
	
              case BUTTON_UP:
              case BUTTON_UP|BUTTON_REPEAT:
              case BUTTON_IR|NEO_IR_BUTTON_VOLUP:
              case BUTTON_IR|NEO_IR_BUTTON_VOLUP|BUTTON_REPEAT:
                  screenidx += 2;
                  /* fallthrough */
              case BUTTON_DOWN:
              case BUTTON_DOWN|BUTTON_REPEAT:
              case BUTTON_IR|NEO_IR_BUTTON_VOLDN:
              case BUTTON_IR|NEO_IR_BUTTON_VOLDN|BUTTON_REPEAT:
                  screenidx--;

                  if( screenidx < 0 )
                      screenidx = strlen(pcurscreen)-1;
                  
                  if( pcurscreen[screenidx] == 0 )
                      screenidx = 0;	   

                  /* Changes the character over the cursor */
                  *pcursor = pcurscreen[screenidx];
          }
      
          lcd_puts( 0, 0, pstart);
          lcd_cursor( cursorpos, 0 );
      }
  }
  return ret;
}
