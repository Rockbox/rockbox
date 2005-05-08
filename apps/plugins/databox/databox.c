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

/* welcome to the example rockbox plugin */

/* here is a global api struct pointer. while not strictly necessary,
   it's nice not to have to pass the api pointer in all function calls
   in the plugin */
struct plugin_api* rb;
struct token tokenbuf[200];

struct print printing;
struct editor editor;
struct editing editing;

extern int acceptedmask;

void databox_init(void) {
    printing.fontfixed = rb->font_get(FONT_SYSFIXED);
    rb->lcd_setfont(FONT_SYSFIXED);
    printing.font_w = printing.fontfixed->maxwidth;
    printing.font_h = printing.fontfixed->height;
    printing.line=0;
    printing.position=0;
    editor.editingmode = INVALID_MARK;
    editor.token = tokenbuf;
}

void print(char *word, int invert) {
    int strlen=rb->strlen(word), newpos=printing.position+strlen+1;
    if(newpos*printing.font_w>LCD_WIDTH) {
            printing.line++;
            printing.position=0;
            newpos=printing.position+strlen+1;
    }
    rb->lcd_putsxy(printing.font_w*printing.position,printing.font_h*printing.line,word);
    if(invert)
        rb->lcd_invertrect(printing.font_w*printing.position,printing.font_h*printing.line,printing.font_w*strlen,printing.font_h);
    rb->lcd_update_rect(printing.font_w*printing.position,printing.font_h*printing.line,printing.font_w*strlen,printing.font_h);
    printing.position=newpos;
}

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

int hcl_button_get(void) {
    int oldbuttonstate,newbuttonstate,pressed=0;
    oldbuttonstate = rb->button_status();
    do {
        newbuttonstate = rb->button_status();
        pressed = newbuttonstate & ~oldbuttonstate;
        oldbuttonstate = newbuttonstate;
        rb->yield();
    }
    while(!pressed);
    rb->button_clear_queue();
    return pressed;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button,done=0;
    char filename[100],buf[100];
    /* this macro should be called as the first thing you do in the plugin.
       it test that the api version and model the plugin was compiled for
       matches the machine it is running on */
    TEST_PLUGIN_API(api);

    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;

    /* if you are using a global api pointer, don't forget to copy it!
       otherwise you will get lovely "I04: IllInstr" errors... :-) */
    rb = api;

    /* now go ahead and have fun! */
    rb->splash(HZ*2, true, "Databox! Enter filename ^.^");
    databox_init();
    if(rb->kbd_input(filename, 100)) {
        rb->splash(HZ*2, true, "Something went wrong with the filename.. exiting..");
        return PLUGIN_ERROR;
    }
    /* add / in front if omitted */
    if(filename[0]!='/') {
        rb->strncpy(buf+1,filename,99);
        buf[0]='/';
        rb->strcpy(filename,buf);
    }
    /* add extension if omitted */
    if(rb->strncasecmp(filename+rb->strlen(filename)-4,".rsp",4)) {
        rb->strcat(filename,".rsp");
    }
    rb->lcd_clear_display();
    rb->lcd_update();
    editor.currentindex=editor.tokencount=readtstream(filename,editor.token,200);
    editing.currentselection=0;
    editing.selecting=editor.currentindex==0 ? 1 : 0;
    do {
        rb->lcd_clear_display();
        rb->lcd_update();
        printing.line=0;
        printing.position=0;
        displaytstream(editor.token);
        editor.valid=check_tokenstream(editor.token,editor.editingmode);
        check_accepted(editor.token,editor.currentindex);
        rb->lcd_update();
        button=hcl_button_get();
        if(editing.selecting) {
            // button handling, up, down, select,stop
            // up/right = move currentselection one up
            // down/left = move currentselection one down
            // select = build token in place.
            // stop = cancel editing
            if(button&BUTTON_LEFT
#if CONFIG_KEYPAD == IRIVER_H100_PAD
                ||button&BUTTON_DOWN
#endif
                ) {
                editing.currentselection=(editing.currentselection+
                    1) %editing.selectionmax;
            }
            if(button&BUTTON_RIGHT
#if CONFIG_KEYPAD == IRIVER_H100_PAD
                ||button&BUTTON_UP
#endif
                ) {
                editing.currentselection=(editing.currentselection +
                    editing.selectionmax-1) % editing.selectionmax;             
            }
            if(button&BUTTON_SELECT) {
                buildtoken(editing.selection_candidates[editing.currentselection],&editor.token[editor.currentindex]);
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
                    rb->memcpy(&editing.old_token,&editor.token[editor.currentindex],sizeof(struct token));
                }
            }
        }
        else {
            // button handling, left, right, select, stop
            // left/down = move currentindex down
            // right/up = move currentindex up
            // select = enter selecting mode.
            // stop = quit editor.
            if(button&BUTTON_LEFT
#if CONFIG_KEYPAD == IRIVER_H100_PAD
                ||button&BUTTON_DOWN
#endif
                ) {
                editor.currentindex=(editor.currentindex + 
                    editor.tokencount) % (editor.tokencount+1);
            }
            if(button&BUTTON_RIGHT
#if CONFIG_KEYPAD == IRIVER_H100_PAD
                ||button&BUTTON_UP
#endif
                ) {
                editor.currentindex=(editor.currentindex+1) % (editor.tokencount+1);
            }
            if(button&BUTTON_SELECT) {
                editing.selecting=1;
                editing.currentselection=0;
                buildchoices(acceptedmask);
                rb->memcpy(&editing.old_token,&editor.token[editor.currentindex],sizeof(struct token));
            }
        }
    } while (!done);
    if(writetstream(filename,editor.token)) {
        rb->splash(HZ*2,true,"Wrote file succesfully ^.^");
        return PLUGIN_OK;
    }
    else {
        rb->splash(HZ*2,true,"Error while writing rsp :(");
        return PLUGIN_ERROR;
    }
}
