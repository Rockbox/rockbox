/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Pierre Delore
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
#include "lib/configfile.h"

/* FIXME: Only for LCD_CHARCELL ?? */
#ifdef HAVE_LCD_CHARCELLS

/* Euro converter for the player */
/*
Use:
+ : Digit +1 
- : Digit -1
PLAY : Next digit
STOP : Prev digit
ON : RESET
ON+PLAY : Swap Euro<>Home
MENU : Display the Menu
        Currency -> Allows to choose the currency
        Exit-> Exit the plugin

Notes:
I don't use float.
I use signed long long (64 bits).
A value have 5 digits after the . (123.45 = 12345000)

To do:
- The Irish currency needs 6 digits after the . to have sufficient precision on big number
*/

PLUGIN_HEADER

/* Name and path of the config file*/
static const char cfg_filename[] =  "euroconverter.cfg";
#define CFGFILE_VERSION 0     /* Current config file version */
#define CFGFILE_MINVERSION 0  /* Minimum config file version to accept */

/* typedef for simplifying usage of long long type */
typedef long long int longlong_t;

/*Pattern for the converter*/
static unsigned char pattern_euro[]={0x07, 0x08, 0x1E, 0x10, 0x1E, 0x08, 0x07};    /* € */
static unsigned char pattern_home[]={0x04, 0x0A, 0x11, 0x1F, 0x11, 0x11, 0x1F};    /* Home icon*/

/* 1 euro = ... (remenber 5 digits after the .)*/
static int currency[12]={
                            655957,     /*FRF France*/
                            195583,     /*DEM Germany*/
                            1376030,    /*ATS Austria*/
                            4033990,    /*BEF Belgium*/
                            16638600,   /*ESP Spain*/
                            594573,     /*FIM Finland*/
                            78756,      /*IEP Ireland*/
                            193627000,  /*ITL Italy*/
                            4033990,    /*LUF Luxemburg*/
                            220371,     /*NLG Netherlands*/
                            20048200,   /*PTE Portugal*/
                            34075100,   /*GRD Greece*/
                        };

/* Number of digit of the currency (for the display) */
static int nb_digit[12]={
                            2,   /*FRF France*/
                            2,   /*DEM Germany*/
                            2,   /*ATS Austria*/
                            2,   /*BEF Belgium*/
                            0,   /*ESP Spain*/
                            2,   /*FIM Finland*/
                            2,   /*IEP Ireland*/
                            0,   /*ITL Italy*/
                            2,   /*LUF Luxemburg*/
                            2,   /*NLG Netherlands*/
                            0,   /*PTE Portugal*/
                            0    /*GRD Greece*/
                        };

/* max euro to have home currency */
static longlong_t max_euro[12]={
                                  99999999000LL,   /*FRF France      999 999.99 */
                                  99999999000LL,   /*DEM Germany     999 999.99 */
                                  99999999000LL,   /*ATS Austria     999 999.99 */
                                  99999999000LL,   /*BEF Belgium     999 999.99 */
                                  99999999000LL,   /*ESP Spain       99 999 999 */
                                  99999999000LL,   /*FIM Finland     999 999.99 */
                                  99999999000LL,   /*IEP Ireland     999 999.99 */
                                  51645690000LL,   /*ITL Italy       999 999 999 */
                                  99999999000LL,   /*LUF Luxemburg   999 999.99 */
                                  99999999000LL,   /*NLG Netherlands 999 999.99 */
                                  99999999000LL,   /*PTE Portugal    99 999 999 */
                                  29347028000LL    /*GRD Greece      99 999 999 */
                              };

/* max home to have euro currency */
/* 92233720300000 Limitation due to the max capacity of long long (2^63)*/
static longlong_t max_curr[12]={
                                  99999999000LL,   /*FRF France      152449.02 */
                                  99999999000LL,   /*DEM Germany     511291.88 */
                                  99999999000LL,   /*ATS Austria     72672.83 */
                                  99999999000LL,   /*BEF Belgium     24789.35 */
                                  92233720300000LL,/*ESP Spain       5543358.23 */
                                  99999999000LL,   /*FIM Finland     168187.92 */
                                  9999999900LL,    /*IEP Ireland     1269744.51 exact value=1269738.07 */
                                  92233720300000LL,/*ITL Italy       476347.41 */
                                  99999999000LL,   /*LUF Luxemburg   24789.35 */
                                  99999999000LL,   /*NLG Netherlands 453780.21 */
                                  92233720300000LL,/*PTE Portugal    4600598.57 */
                                  92233720300000LL /*GRD Greece      2706777.69 */
                              };

static unsigned char *abbrev_str[12] = {
                                          "...FRF...",  /*France*/
                                          "...DEM...",  /*Germany*/
                                          "...ATS...",  /*Austria*/
                                          "...BEF...",  /*Belgium*/
                                          "...ESP...",  /*Spain*/
                                          "...FIM...",  /*Finland*/
                                          "...IEP...",  /*Ireland*/
                                          "...ITL...",  /*Italy*/
                                          "...LUF...",  /*Luxemburg*/
                                          "...NLG...",  /*Netherlands*/
                                          "...PTE...",  /*Portugal*/
                                          "...GRD..."   /*Greece*/
                                      };


static unsigned long heuro,hhome; /*Handles for the new patterns*/

static char *currency_str[12] = {
   "France",
   "Germany",
   "Austria",
   "Belgium",
   "Spain",
   "Finland",
   "Ireland",
   "Italy",
   "Luxemburg",
   "Netherlands",
   "Portugal",
   "Greece"
};


static int country;     /*Country selected*/
static int cur_pos;     /*Cursor position*/
static longlong_t inc;   

/* Persistent settings */
static struct configdata config[] = {
   { TYPE_ENUM, 0, 12, &country, "country", currency_str, NULL }
};


/* 64bits*64 bits with 5 digits after the . */
static longlong_t mymul(longlong_t a, longlong_t b)
{
    return((a*b)/100000LL);
}


/* 64bits/64 bits with 5 digits after the . */
static longlong_t mydiv(longlong_t a, longlong_t b)
{
    return((a*100000LL)/b);
}


/* 123.45=12345000  split => i=123 f=45000*/
static void split(longlong_t v, longlong_t* i, longlong_t* f)
{
    longlong_t t;

    t=v/100000LL;
    (*i)=t;
    (*f)=(v-(t*100000LL));
}


/* result=10^n */
static longlong_t pow10(int n)
{
    int i;
    longlong_t r;

    r=1;
    for (i=0;i<n;i++)
        r=r*10LL;
    return(r);
}


/* round the i.f at n digit after the . */
static void round(longlong_t* i, longlong_t* f, int n)
{

    longlong_t m;
    int add=0;

    m=(int)pow10(5-n-1);
    if (((*f)/m)%10>=5)
        add=1;
    if (n>0)
    {
        (*f)=((*f)/(int)pow10(5-n))+add;
        if ((*f)==100LL)
        {
            (*i)+=1;
            (*f)=0;
        }
    }
    else
    {
        (*i)+=add;
        (*f)=0;
    }
}


/* Display the imput and the result
    pos: false : first line 
       : true  : second line
*/
static void display(longlong_t euro, longlong_t home, bool pos)
{
    longlong_t i,f;
    unsigned char str[20];
    unsigned char s1[20];
    unsigned char s2[20];

    if (pos)
    {   /*Edit the second line*/
        rb->strcpy(s1," %6d.%02d");
        if (nb_digit[country]==2)
            rb->strcpy(s2,"\xee\x84\x90%06d.%02d");
        else
            rb->strcpy(s2,"\xee\x84\x90%09d");
    }
    else
    {
        rb->strcpy(s1,"\xee\x84\x90%06d.%02d");
        if (nb_digit[country]==2)
            rb->strcpy(s2," %6d.%02d");
        else
            rb->strcpy(s2," %9d");
    }

    rb->lcd_remove_cursor();
    /*First line*/
    rb->lcd_putc(0,0,heuro);
    split(euro,&i,&f);
    if (pos)
        round(&i,&f,2);
    rb->snprintf(str,sizeof(str),s1,(int)i,(int)f);

    if (!pos)
    {
        rb->lcd_puts(1,0,str);
        rb->lcd_put_cursor(10-cur_pos,0,0x5F);
    }
    else
        rb->lcd_puts_scroll(1,0,str);

    /*Second line*/
    rb->lcd_putc(0,1,hhome);
    split(home,&i,&f);
    if (!pos)
        round(&i,&f,nb_digit[country]);
    rb->snprintf(str,sizeof(str),s2,(int)i,(int)f);
    if (pos)
    {
        rb->lcd_puts(1,1,str);
        rb->lcd_put_cursor(10-cur_pos,1,0x5F);
    }
    else
        rb->lcd_puts_scroll(1,1,str);
        
    rb->lcd_update();
}


/* Show country Abbreviation */
static void show_abbrev(void)
{
    rb->splash(HZ*3/4,abbrev_str[country]);
}


/* Save the config to disk */
static void save_config(void)
{
    configfile_save(cfg_filename, config, 1, CFGFILE_VERSION);
}


/* Load the config from disk */
static void load_config(void)
{
    configfile_load(cfg_filename, config, 1, CFGFILE_MINVERSION);
}


/*Currency choice*/
static void currency_menu(void)
{
    int c=country;

    rb->lcd_clear_display();
    while (true)
    {
        rb->lcd_puts(0,0,"Currency:");
        rb->lcd_puts(0,1,currency_str[c]);
        rb->lcd_update();
        switch (rb->button_get(true))
        {
            case BUTTON_RIGHT|BUTTON_REL:
                c++;
                if (c>11)
                    c=0;
                break;
            case BUTTON_LEFT|BUTTON_REL:
                c--;
                if (c<0)
                    c=11;
                break;
            case BUTTON_PLAY|BUTTON_REL:
                country=c;
                save_config();
                return;
                break;
            case BUTTON_STOP|BUTTON_REL:
                return;
        }
    }
}


/* Display the choice menu. */
static int euro_menu(void)
{
    int c=0;


    while (true)
    {
        rb->lcd_clear_display();
        rb->lcd_puts(0,0," Currency");
        rb->lcd_puts(0,1," Exit");
        rb->lcd_putc(0,c,0xe110);
        rb->lcd_update();

        switch (rb->button_get(true))
        {
            case BUTTON_RIGHT|BUTTON_REL:
                c=1;
                break;
            case BUTTON_LEFT|BUTTON_REL:
                c=0;
                break;
            case BUTTON_PLAY|BUTTON_REL:
                if (c==0)
                    currency_menu();
                else
                    return 1;
                break;
            case BUTTON_STOP|BUTTON_REL:
                return 0;
        }
    }
}


/* Call when the program end */
static void euro_exit(void *parameter)
{
    (void)parameter;
    
    //Restore the old pattern (i don't find another way to do this. An idea?)
    rb->lcd_unlock_pattern(heuro);
    rb->lcd_unlock_pattern(hhome);

    //Clear the screen
    rb->lcd_clear_display();
    rb->lcd_update();
}


/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    bool end, pos;
    longlong_t e,h,old_e,old_h;
    int button;

    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;

    /*Get the pattern handle*/
    heuro=rb->lcd_get_locked_pattern();
    hhome=rb->lcd_get_locked_pattern();
    rb->lcd_define_pattern(heuro, pattern_euro);
    rb->lcd_define_pattern(hhome, pattern_home);

    h=0;
    e=0;
    end=false;
    pos=false;
    country=0;
    cur_pos=3;
    inc=100000;

    load_config();

    /*Empty the event queue*/
    rb->button_clear_queue();

    display(e,h,false);
    show_abbrev();
    display(e,h,false);

    /*Main loop*/
    while(end!=true)
    {
        button = rb->button_get(true);
        switch (button)
        {
            case BUTTON_MENU|BUTTON_REL:
                switch (euro_menu())
                {
                    case 1:
                        end=true;
                        break;
                }
                if (!pos)
                {
                    if (e>max_euro[country])
                        e=0;
                    cur_pos=3;
                }
                else
                {
                    if (h>max_curr[country])
                        h=0;
                    if (nb_digit[country]==2)
                        cur_pos=3;
                    else
                        cur_pos=0;
                }

                display(e,h,pos);
                break;

            case BUTTON_ON | BUTTON_PLAY:
                pos=!pos;

            case BUTTON_ON | BUTTON_REL:
                e=0;
                h=0;
                if (!pos)
                {
                    cur_pos=3;
                    inc=100000;
                }
                else
                {
                    inc=100000;
                    if (nb_digit[country]==2)
                        cur_pos=3;
                    else
                        cur_pos=0;
                }
                show_abbrev();
                break;

            case BUTTON_STOP|BUTTON_REL:
                cur_pos--;
                if (!pos)
                {
                    if (cur_pos<0)
                        cur_pos=0;
                    if (cur_pos==2)
                        cur_pos=1;
                    if (cur_pos>2)
                        inc=pow10(3+cur_pos-1);
                    else
                        inc=pow10(3+cur_pos);
                }
                else
                {
                    if (cur_pos<0)
                        cur_pos=0;
                    if (nb_digit[country]==2)
                    {
                        if (cur_pos==2)
                            cur_pos=1;
                        if (cur_pos>2)
                            inc=pow10(3+cur_pos-1);
                        else
                            inc=pow10(3+cur_pos);
                    }
                    else
                        inc=pow10(5+cur_pos);

                }
                break;

            case BUTTON_PLAY|BUTTON_REL:
                cur_pos++;
                if (!pos)
                {
                    if (cur_pos>8)
                        cur_pos=8;
                    if (cur_pos==2)
                        cur_pos=3;
                    if (cur_pos>2)
                        inc=pow10(3+cur_pos-1);
                    else
                        inc=pow10(3+cur_pos);
                }
                else
                {
                    if (cur_pos>8)
                        cur_pos=8;
                    if (nb_digit[country]==2)
                    {
                        if (cur_pos==2)
                            cur_pos=3;
                        if (cur_pos>2)
                            inc=pow10(3+cur_pos-1);
                        else
                            inc=pow10(3+cur_pos);
                    }
                    else
                        inc=pow10(5+cur_pos);
                }
                break;

            case BUTTON_LEFT|BUTTON_REL:
            case BUTTON_LEFT|BUTTON_REPEAT:
                if (!pos)
                {
                    e-=inc;
                    if (e<0)
                        e=0;
                }
                else
                {
                    h-=inc;
                    if (h<0)
                        h=0;
                }
                break;

            case BUTTON_RIGHT|BUTTON_REL:
            case BUTTON_RIGHT|BUTTON_REPEAT:
                old_e=e;
                old_h=h;
                if (!pos)
                {
                    e+=inc;
                    if (e>max_euro[country])
                        e=old_e;
                }
                else
                {
                    h+=inc;
                    if (h>max_curr[country])
                        h=old_h;
                }
                break;

            default:
                if (rb->default_event_handler_ex(button, euro_exit, NULL)
                     == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        /*Display*/
        if (!pos)   /*Euro>home*/
            h=mymul(e,currency[country]);
        else    /*Home>euro*/
            e=mydiv(h,currency[country]);
        display(e,h,pos);
    }
    euro_exit(NULL);
    return PLUGIN_OK;
}

#endif
