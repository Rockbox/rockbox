/*
Snake!

by Itai Shaked

ok, a little explanation - 
board holds the snake and apple position - 1+ - snake body (the number
represents the age [1 is the snake's head]).
-1 is an apple, and 0 is a clear spot.
dir is the current direction of the snake - 0=up, 1=right, 2=down, 3=left;

(i'll write more later)
*/

#include "sprintf.h"


#include "config.h"
#include "lcd.h"
#include "button.h"
#include "kernel.h"
#include "menu.h"
 

int board[28][16],snakelength;
unsigned int score,hiscore=0;
short dir,frames,apple,level=1,dead=0;

int rand_num (int range) {
    return current_tick % range;
}

void die (void) {
    char pscore[5],hscore[17];
    lcd_clear_display();
    snprintf(pscore,sizeof(pscore),"%d",score);
    lcd_putsxy(3,12,"oops...",0);
    lcd_putsxy(3,22,"Your Score :",0);
    lcd_putsxy(3,32, pscore,0);
    if (score>hiscore) {
        hiscore=score;
        lcd_putsxy(3,42,"New High Score!",0);
    }
    else {
        snprintf(hscore,sizeof(hscore),"High Score %d",hiscore);
        lcd_putsxy(3,42,hscore,0);
    }
    lcd_update();
    sleep(HZ);    
    dead=1;
}

void colission (short x, short y) {
    switch (board[x][y]) {
        case 0:
            break; 
        case -1:
            snakelength+=2;
            score+=level;
            apple=0;
            break;
        default:
            die();
            break;
    }
    if (x==28 || x<0 || y==16 || y<0) 
        die();
}

void move_head (short x, short y) {
    switch (dir) {
        case 0:
            y-=1;
            break;
        case 1:
            x+=1; 
            break;
        case 2:
            y+=1;
            break;
        case 3:
            x-=1;
            break;
    }
    colission (x,y);
    if (dead) return;
    board[x][y]=1;
    lcd_fillrect(x*4,y*4,4,4);
}

void frame (void) {
    short x,y,head=0;
    for (x=0; x<28; x++) {
        for (y=0; y<16; y++) {
            switch (board[x][y]) {
                case 1:
                    if (!head) {
                        move_head(x,y);
                        if (dead) return;
                        board[x][y]++;
                        head=1;
                    }
                    break;
                case 0:
                    break;
                case -1:
                    break;
                default:
                    if (board[x][y]==snakelength) {
                        board[x][y]=0;
                        lcd_clearrect(x*4,y*4,4,4);
                    }
                    else 
                        board[x][y]++;
            }
        }
    }
    lcd_update();
}

void redraw (void) {
    short x,y;
    lcd_clear_display();
    for (x=0; x<28; x++) {
        for (y=0; y<16; y++) {
            switch (board[x][y]) {
                case -1:
                    lcd_fillrect((x*4)+1,y*4,2,4);
                    lcd_fillrect(x*4,(y*4)+1,4,2);
                    break;
                case 0:
                    break;
                default:
                    lcd_fillrect(x*4,y*4,4,4);
                    break;
            }
        }
    }
    lcd_update();
}

void game_pause (void) {
    lcd_clear_display();
    lcd_putsxy(3,12,"Game Paused",0);
    lcd_putsxy(3,22,"[play] to resume",0);
    lcd_putsxy(3,32,"[off] to quit",0);
    lcd_update();
    while (1) {
        switch (button_get(true)) {
            case BUTTON_OFF:
                dead=1;
                return;
            case BUTTON_PLAY:
                redraw();
                sleep(HZ/2);
                return;
        }
    }
}


void game (void) {
    short x,y;
    while (1) {
        frame();
        if (dead) return;
	frames++;
        if (frames==10) {
            frames=0;
            if (!apple) {
                do {
                    x=rand_num(28);
                    y=rand_num(16);
                } while (board[x][y]);
                apple=1;
                board[x][y]=-1;
                lcd_fillrect((x*4)+1,y*4,2,4);
                lcd_fillrect(x*4,(y*4)+1,4,2);
            }
        }

        sleep(HZ/level);

        switch (button_get(false)) {
             case BUTTON_UP:
                 if (dir!=2) dir=0;
                 break;
             case BUTTON_RIGHT:
                 if (dir!=3) dir=1;
                 break;
             case BUTTON_DOWN:
                 if (dir!=0) dir=2;
                 break;
             case BUTTON_LEFT:
                 if (dir!=1) dir=3;
                 break;
             case BUTTON_OFF:
                 dead=1;
                 return;
             case BUTTON_PLAY:
                 game_pause();
                 break;
        }      
    }
}

void game_init(void) {
    short x,y;
    char plevel[10],phscore[20];

    for (x=0; x<28; x++) {
        for (y=0; y<16; y++) {
            board[x][y]=0;
        }
    }
    dead=0;
    apple=0;
    snakelength=4;
    score=0;
    board[11][7]=1;   


    lcd_clear_display();
    snprintf(plevel,sizeof(plevel),"Level - %d",level);
    snprintf(phscore,sizeof(phscore),"High Score - %d",hiscore);
    lcd_putsxy(3,2, plevel,0);
    lcd_putsxy(3,12, "(1 - slow, 9 - fast)",0);
    lcd_putsxy(3,22, "[off] to quit",0);
    lcd_putsxy(3,32, "[play] to start/pause",0);
    lcd_putsxy(3,42, phscore,0);
    lcd_update();

    while (1) {    
        switch (button_get(true)) {
            case BUTTON_RIGHT:
            case BUTTON_UP:
                if (level<9) 
                    level++;
                break;
            case BUTTON_LEFT:
            case BUTTON_DOWN:
                if (level>1)
                    level--;
                break;
            case BUTTON_OFF:
                dead=1;
                return;
                break;
            case BUTTON_PLAY:
                return;
                break;
        }
        snprintf(plevel,sizeof(plevel),"Level - %d",level);
        lcd_putsxy(3,2, plevel,0);
        lcd_update();
    }
     
}

Menu snake(void) {
    game_init(); 
    lcd_clear_display();
    game();    
    return MENU_OK;
}

