/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "databox.h"

PLUGIN_HEADER

/* variable button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define DBX_SELECT BUTTON_SELECT
#define DBX_STOP   BUTTON_OFF
#elif CONFIG_KEYPAD == RECORDER_PAD
#define DBX_SELECT BUTTON_PLAY
#define DBX_STOP   BUTTON_OFF
#elif CONFIG_KEYPAD == ONDIO_PAD
#define DBX_SELECT BUTTON_MENU
#define DBX_STOP   BUTTON_OFF
#elif (CONFIG_KEYPAD == IPOD_4G_PAD)
#define DBX_SELECT BUTTON_SELECT
#define DBX_STOP   BUTTON_MENU
#elif CONFIG_KEYPAD == PLAYER_PAD
#define DBX_SELECT BUTTON_PLAY
#define DBX_STOP   BUTTON_STOP
#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define DBX_SELECT BUTTON_SELECT
#define DBX_STOP   BUTTON_PLAY
#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define DBX_SELECT BUTTON_SELECT
#define DBX_STOP   BUTTON_PLAY
#endif

#define MAX_TOKENS 70

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
struct plugin_api* rb;
struct token tokenbuf[MAX_TOKENS];

struct print printing;
struct editor editor;
struct editing editing;

extern int acceptedmask;

void databox_init(void) {
#ifdef HAVE_LCD_BITMAP
    printing.fontfixed = rb->font_get(FONT_SYSFIXED);
    rb->lcd_setfont(FONT_SYSFIXED);
    printing.font_w = printing.fontfixed->maxwidth;
    printing.font_h = printing.fontfixed->height;
#endif
    printing.line=0;
    printing.position=0;
    editor.editingmode = INVALID_MARK;
    editor.token = tokenbuf;
}

#ifdef HAVE_LCD_BITMAP
void print(char *word, int invert) {
    int strlen=rb->strlen(word), newpos=printing.position+strlen+1;
    if(newpos*printing.font_w>LCD_WIDTH) {
        printing.line++;
        printing.position=0;
        newpos=printing.position+strlen+1;
    }
    /* Fixme: the display code needs to keep the current item visible instead of
     * just displaying the first items. */
    if (printing.font_h*printing.line >= LCD_HEIGHT)
        return;
    rb->lcd_putsxy(printing.font_w*printing.position,printing.font_h*printing.line,word);
    if(invert) {
        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        rb->lcd_fillrect(printing.font_w*printing.position,printing.font_h*printing.line,printing.font_w*strlen,printing.font_h);
        rb->lcd_set_drawmode(DRMODE_SOLID);
    }
    rb->lcd_update_rect(printing.font_w*printing.position,printing.font_h*printing.line,printing.font_w*strlen,printing.font_h);
    printing.position=newpos;
}
#else /* HAVE_LCD_CHARCELLS */
#define MARKER_LEFT 0x81
#define MARKER_RIGHT 0x82
void print(char *word, int invert) {
    int strlen = rb->strlen(word);
    int newpos = printing.position + strlen + (invert ? 3 : 1);
    if (newpos > 11) {
        printing.line++;
        printing.position = 0;
        newpos = printing.position + strlen + (invert ? 3 : 1);
    } 
    /* Fixme: the display code needs to keep the current item visible instead of
     * just displaying the first items. */
    if (printing.line >= 2)
        return;
    if (invert) {
        rb->lcd_putc(printing.position, printing.line, MARKER_LEFT);
        rb->lcd_puts(printing.position + 1, printing.line, word);
        rb->lcd_putc(printing.position + strlen + 1, printing.line, MARKER_RIGHT);
    }
    else
        rb->lcd_puts(printing.position, printing.line, word);
    printing.position = newpos;
}
#endif

void displaytstream(struct token *token) {
    int index=0;
    while(token[index].kind!=TOKEN_EOF||index==editor.currentindex) {
        if(editing.selecting&&index==editor.currentindex) {
          print(tokentypetostring(editing.selection_candidates[editing.currentselection]),1);
        }
        else
          print(tokentostring(&token[index]),index==editor.currentindex ? 1 : 0);
        index++;
    }
}

void buildchoices(int mask) {
    int i;
    for(i=0;i<20;i++)
        editing.selection_candidates[i]=-1;
    i=0;
    if(editing.selecting&& 
            editing.old_token.kind!=TOKEN_EOF &&
            editing.old_token.kind!=TOKEN_INVALID) {
        editing.selection_candidates[i++]=TOKEN_EDIT;
    }
    if((mask&ACCEPT_EOF)&&editor.valid)
        editing.selection_candidates[i++]=TOKEN_EOF;
    if(mask&ACCEPT_NOT)
        editing.selection_candidates[i++]=TOKEN_NOT;
    if(mask&ACCEPT_BOOLOP) {
        editing.selection_candidates[i++]=TOKEN_AND;
        editing.selection_candidates[i++]=TOKEN_OR;
    }
    if(mask&ACCEPT_NUMOP) {
        editing.selection_candidates[i++]=TOKEN_GT;
        editing.selection_candidates[i++]=TOKEN_GTE;
        editing.selection_candidates[i++]=TOKEN_LT;
        editing.selection_candidates[i++]=TOKEN_LTE;
        editing.selection_candidates[i++]=TOKEN_NE;
        editing.selection_candidates[i++]=TOKEN_EQ;
    }
    if(mask&ACCEPT_STROP) {
        editing.selection_candidates[i++]=TOKEN_CONTAINS;
        editing.selection_candidates[i++]=TOKEN_EQUALS;
        editing.selection_candidates[i++]=TOKEN_STARTSWITH;
        editing.selection_candidates[i++]=TOKEN_ENDSWITH;
    }
    if(mask&ACCEPT_LPAREN) {
        editing.selection_candidates[i++]=TOKEN_LPAREN;
    }
    if(mask&ACCEPT_RPAREN) {
        editing.selection_candidates[i++]=TOKEN_RPAREN;
    }
    if(mask&ACCEPT_NUMARG) {
        editing.selection_candidates[i++]=TOKEN_NUM;
        editing.selection_candidates[i++]=TOKEN_YEAR;
        editing.selection_candidates[i++]=TOKEN_RATING;
        editing.selection_candidates[i++]=TOKEN_PLAYCOUNT;
        editing.selection_candidates[i++]=TOKEN_AUTORATING;
        editing.selection_candidates[i++]=TOKEN_TRACKNUM;
        editing.selection_candidates[i++]=TOKEN_PLAYTIME;
        editing.selection_candidates[i++]=TOKEN_SAMPLERATE;
        editing.selection_candidates[i++]=TOKEN_BITRATE;
    }
    if(mask&ACCEPT_STRARG) {
        editing.selection_candidates[i++]=TOKEN_STRING;
        editing.selection_candidates[i++]=TOKEN_TITLE;
        editing.selection_candidates[i++]=TOKEN_ARTIST;
        editing.selection_candidates[i++]=TOKEN_ALBUM;
        editing.selection_candidates[i++]=TOKEN_GENRE;
        editing.selection_candidates[i++]=TOKEN_FILENAME;
    }
    editing.selectionmax=i;
}

/* returns tokencount or 0 if error */
int readtstream(char *filename,struct token *token,int max) {
    int tokencount=0;
    int filelen,i;
    int fd;
    rb->memset(token,0,max*sizeof(struct token));
    fd=rb->open(filename,O_RDONLY);
    if(fd>=0) {
        filelen=rb->filesize(fd);
        if(filelen>0) {
            if(filelen % sizeof(struct token)) {
                rb->splash(HZ*2,true,"Filesize not a multiple of sizeof(struct token)");
                rb->close(fd);
                return 0;
            }
            tokencount=(filelen/sizeof(struct token))-1;
            for(i=0;i<tokencount&&i<max;i++) {
              rb->read(fd,&token[i],sizeof(struct token));
              token[i].intvalue=BE32(token[i].intvalue);
            }
        }
        rb->close(fd);
    }
    return tokencount;
}

int writetstream(char *filename,struct token *token) {
    int fd,i;
    fd=rb->open(filename,O_WRONLY|O_CREAT|O_TRUNC);
    if(fd<0)
        return 0;
    i=0;
    while(token[i].kind!=TOKEN_EOF) {
            token[i].intvalue=BE32(token[i].intvalue);
            rb->write(fd,&token[i++],sizeof(struct token));
    }
    token[i].intvalue=BE32(token[i].intvalue);
    rb->write(fd,&token[i++],sizeof(struct token));
    rb->close(fd);
    return i;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button,done=0,abort=0;
    char filename[100],buf[100];
    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;

    /* if you are using a global api pointer, don't forget to copy it!
       otherwise you will get lovely "I04: IllInstr" errors... :-) */
    rb = api;

    /* now go ahead and have fun! */
    rb->splash(HZ*2, true, "Databox! Enter filename ^.^");
    databox_init();
    filename[0] = '\0';
    if(rb->kbd_input(filename, sizeof filename)) {
        rb->splash(HZ*2, true, "Cancelled...");
        return PLUGIN_OK;
    }
    /* add / in front if omitted */
    if(filename[0]!='/') {
        rb->strncpy(buf+1,filename,sizeof(filename)-1);
        buf[0]='/';
        rb->strcpy(filename,buf);
    }
    /* add extension if omitted */
    if(rb->strncasecmp(filename+rb->strlen(filename)-4,".rsp",4)) {
        rb->strcat(filename,".rsp");
    }
    editor.currentindex=editor.tokencount
                       =readtstream(filename,editor.token,MAX_TOKENS);
    editing.currentselection=0;
    editing.selecting=0;
    if(editor.currentindex==0) {
        editor.valid=check_tokenstream(editor.token,editor.editingmode);
        check_accepted(editor.token,editor.currentindex);
        editing.selecting=1;
        buildchoices(acceptedmask);
        rb->memset(&editing.old_token,0,sizeof(struct token));
    }
    do {
#ifdef HAVE_LCD_BITMAP
        rb->lcd_setfont(FONT_SYSFIXED);
#endif
        rb->lcd_clear_display();
        printing.line=0;
        printing.position=0;
        displaytstream(editor.token);
        editor.valid=check_tokenstream(editor.token,editor.editingmode);
        check_accepted(editor.token,editor.currentindex);
#ifdef HAVE_LCD_BITMAP
        rb->lcd_update();
#endif
        button = rb->button_get(true);
        switch (button) {
          case BUTTON_LEFT:
#ifdef BUTTON_DOWN
          case BUTTON_DOWN:
#endif
            if (editing.selecting)
                editing.currentselection = (editing.currentselection +
                        editing.selectionmax-1) % editing.selectionmax;
            else
                editor.currentindex = (editor.currentindex + editor.tokencount) 
                                    % (editor.tokencount+1);
            break;

          case BUTTON_RIGHT:
#ifdef BUTTON_UP
          case BUTTON_UP:
#endif
            if (editing.selecting)
                editing.currentselection = (editing.currentselection+1)
                                         % editing.selectionmax;
            else
                editor.currentindex = (editor.currentindex+1) 
                                    % (editor.tokencount+1);
            break;

          case DBX_SELECT:
            if(editing.selecting) {
                buildtoken(editing.selection_candidates[editing.currentselection],
                           &editor.token[editor.currentindex]);
                editing.selecting=0;
                if(editor.token[editor.currentindex].kind==TOKEN_EOF)
                    done=1;
                else if(editor.currentindex==editor.tokencount) {
                    editor.tokencount++;
                    editor.currentindex++;
                    editor.valid=check_tokenstream(editor.token,editor.editingmode);
                    check_accepted(editor.token,editor.currentindex);
                    editing.selecting=1;
                    editing.currentselection=0;
                    buildchoices(acceptedmask);
                    rb->memcpy(&editing.old_token,&editor.token[editor.currentindex],
                               sizeof(struct token));
                }
            }
            else {
                editing.selecting=1;
                editing.currentselection=0;
                buildchoices(acceptedmask);
                rb->memcpy(&editing.old_token,&editor.token[editor.currentindex],
                           sizeof(struct token));
            }
            break;

          case DBX_STOP:
            if(editing.selecting) {
                rb->memcpy(&editor.token[editor.currentindex],&editing.old_token,
                           sizeof(struct token));
                editing.selecting=0;
            }
            else
                abort=1;
            break;

          default:
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED) {
#ifdef HAVE_LCD_BITMAP
                rb->lcd_setfont(FONT_UI);
#endif                
                return PLUGIN_USB_CONNECTED;
            }
            break;
        }
    } while (!done&&!abort);
#ifdef HAVE_LCD_BITMAP
    rb->lcd_setfont(FONT_UI);
#endif
    if(abort)
        return PLUGIN_OK;

    if(editor.valid&&editor.tokencount>0) {
        if(writetstream(filename,editor.token)) {
            rb->splash(HZ*2,true,"Wrote file succesfully ^.^");
            return PLUGIN_OK;
        }
        else {
            rb->splash(HZ*2,true,"Error while writing file :(");
            return PLUGIN_ERROR;
        }
    }
    else {
        rb->splash(HZ*2,true,"Search query invalid, not saving.");
        return PLUGIN_OK;
    }
}
