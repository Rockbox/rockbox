/* === Rockbox Resistor code/value calculator ===
[insert relevant/useful information here]
TODO:
[ ] Own numeric keypad
*/
 
#include "plugin.h"
#include "lib/display_text.h"
#include "lib/pluginlib_actions.h"
#include "lib/picture.h"
#include "lib/helper.h"

/* Defining player-specific constants */

#if defined(HAVE_LCD_COLOR)
#define    RESISTOR_BMP_X           ((LCD_WIDTH - BMPWIDTH_resistor) / 2)

#if LCD_WIDTH >= 320 && LCD_HEIGHT >= 240 /* iPod video or larger */
#define    RESISTOR_BMP_Y           3

#elif LCD_WIDTH >= 240 && LCD_HEIGHT >= 320 /* Onda, mostly */
#define    RESISTOR_BMP_Y           3

#elif LCD_WIDTH >= 220 && LCD_HEIGHT >= 176 /* Fuze or larger */
#define    RESISTOR_BMP_Y           15

#elif LCD_WIDTH >= 176 && LCD_HEIGHT >= 220 /* e200 or larger */
#define    RESISTOR_BMP_Y          11

#elif LCD_WIDTH >= 176 && LCD_HEIGHT >= 132 /* ipod nano or larger */
#define RESISTOR_BMP_Y             7

#elif LCD_WIDTH >= 160 && LCD_HEIGHT >= 128 /* H10 or larger */
#define RESISTOR_BMP_Y             3

#elif LCD_WIDTH >= 128 && LCD_HEIGHT >= 128 /* GoGear */
#define RESISTOR_BMP_Y             3

#else /* Small screens */
#define RESISTOR_BMP_Y             0

#endif

#else /* HAVE_LCD_COLOR */

#define    USE_TEXT_ONLY
#endif /* HAVE_LCD_COLOR */

#define total_resistance_str_x     1
#define tolerance_str_x            1
#define resistance_val_x           1
#define r_to_c_out_str_x           1

#define INITIAL_TEXT_Y 0

#ifndef USE_TEXT_ONLY
/* (below is for color targets */

#include "pluginbitmaps/resistor.h"

#define band_width                 (BMPWIDTH_resistor/15)
#define band_height                (BMPHEIGHT_resistor*9/10)

#define first_band_x               (BMPWIDTH_resistor/4 + RESISTOR_BMP_X - band_width/2)
#define second_band_x              (3*BMPWIDTH_resistor/8 + RESISTOR_BMP_X - band_width/2)
#define third_band_x               (BMPWIDTH_resistor/2 + RESISTOR_BMP_X - band_width/2)
#define fourth_band_x              (3*BMPWIDTH_resistor/4 + RESISTOR_BMP_X - band_width/2)
#define universal_y                ((BMPHEIGHT_resistor)/2 - band_height/2)

#endif /* USE_TEXT_ONLY */

enum color {
    RES_BLACK,
    RES_BROWN,
    RES_RED,
    RES_ORANGE,
    RES_YELLOW,
    RES_GREEN,
    RES_BLUE,
    RES_VIOLET,
    RES_GREY,
    RES_WHITE,
    RES_GOLD,
    RES_SILVER,
    RES_NONE,
    RES_INVALID = -1,
};

static int common_values[] = { 0, 1, 10, 15, 22, 27, 33, 39, 47, 51, 68, 82 };
static int power_ratings[] = { 125, 250, 500, 1000, 2000, 3000, 5000, 10000, 50000 };
/* All in mW */
    
#ifndef LCD_RGBPACK
/* Warning: dirty kludge */
#define LCD_RGBPACK(x,y,z) 0
#endif

static struct band_data
{
    enum color color;
    char *name;
    int color_value;
    int resistance_value;
    int multiplier;
    char *unit;
    int tolerance;
} band_data[] =
{
    { RES_BLACK,  "Black",  LCD_RGBPACK(0, 0, 0),       0,   100,   "Ohms",-1 },
    { RES_BROWN,  "Brown",  LCD_RGBPACK(118, 78, 0),    1,  1000,   "Ohms", 1 },
    { RES_RED,    "Red",    LCD_RGBPACK(255, 0, 0),     2, 10000,   "Ohms", 2 },
    { RES_ORANGE, "Orange", LCD_RGBPACK(255, 199, 76),  3,   100,  "KOhms",-1 },
    { RES_YELLOW, "Yellow", LCD_RGBPACK(255, 255, 0),   4,  1000,  "KOhms",-1 },
    { RES_GREEN,  "Green",  LCD_RGBPACK(0, 128, 0),     5, 10000,  "KOhms",-1 },
    { RES_BLUE,   "Blue",   LCD_RGBPACK(0, 0, 255),     6,   100,  "MOhms",-1 },
    { RES_VIOLET, "Violet", LCD_RGBPACK(153, 51, 255),  7,  1000,  "MOhms",-1 },
    { RES_GREY,   "Grey",   LCD_RGBPACK(192, 192, 192), 8, 10000,  "MOhms",-1 },
    { RES_WHITE,  "White",  LCD_RGBPACK(255, 255, 255), 9,   100,  "GOhms",-1 },
    { RES_GOLD,   "Gold",   LCD_RGBPACK(146, 146, 0),  -1,    10,  "Ohms",  5 },
    { RES_SILVER, "Silver", LCD_RGBPACK(213, 213, 213),-1,     1,  "Ohms", 10 },
    { RES_NONE,   "[None]",  -1                       ,-1,    -1,       0, 20 }
};

static char *unit_abbrev;

static struct viewport screen_vp;
static struct viewport bitmap_vp;
static struct viewport text_vp;
static struct screen *display;

static int lineno;

static char *get_power_rating_str(int in_rating) 
{
    switch(in_rating) {
        case 125:
            return "1/8 Watt";
        case 250:
            return "1/4 Watt";
        case 500:
            return "1/2 Watt";
        case 1000:
            return "1 Watt";
        case 2000:
            return "2 Watt";
        case 3000:
            return "3 Watt";
        case 5000:
            return "5 Watt";
        case 10000:
            return "10 Watt";
        case 500000:
            return "50 Watt";
        default: 
            return "Unknown";
    }
}

static int powi(int num, int exp)
{
    int i, product = 1;
    for (i = 0; i < exp; i++) {
    product *= num; }

    return product;
}

static enum color get_band_rtoc(int in_val)
{
    int return_color = RES_INVALID;
    switch(in_val) {
        case 0:
            return_color = RES_BLACK;
            break;
        case 1:
            return_color = RES_BROWN;
            break;
        case 2:
            return_color = RES_RED;
            break;
        case 3:
            return_color = RES_ORANGE;
            break;
        case 4:
            return_color = RES_YELLOW;
            break;
        case 5:
            return_color = RES_GREEN;
            break;
        case 6:
            return_color = RES_BLUE;
            break;
        case 7:
            return_color = RES_VIOLET;
            break;
        case 8:
            return_color = RES_GREY;
            break;
        case 9:
            return_color = RES_WHITE;
            break;
    }
    return return_color;
}

static char *get_tolerance_str(enum color color)
{
    static char tolerance_str [14];
    rb->snprintf(tolerance_str, sizeof(tolerance_str), "%d%% tolerance",
                 band_data[color].tolerance);
    return tolerance_str;
}

#ifndef USE_TEXT_ONLY
static void draw_resistor(enum color firstband_color,
                   enum color secondband_color,
                   enum color thirdband_color,
                   enum color fourthband_color)
{
    unsigned int fg;

    rb->lcd_clear_display();
    display->set_viewport(&bitmap_vp);
    rb->lcd_bitmap_transparent(resistor, RESISTOR_BMP_X, 0, 
                               BMPWIDTH_resistor, BMPHEIGHT_resistor);

    fg = rb->lcd_get_foreground();
        
    if(firstband_color != RES_NONE) {
        rb->lcd_set_foreground(band_data[firstband_color].color_value);
        rb->lcd_fillrect(first_band_x, universal_y, band_width, band_height);
    } else {
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_drawrect(first_band_x, universal_y, band_width, band_height);
    }
          
    if(secondband_color != RES_NONE) {
        rb->lcd_set_foreground(band_data[secondband_color].color_value);
        rb->lcd_fillrect(second_band_x, universal_y, band_width, band_height);
    } else {
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_drawrect(second_band_x, universal_y, band_width, band_height);
    }
    
    if(thirdband_color != RES_NONE) {        
        rb->lcd_set_foreground(band_data[thirdband_color].color_value);
        rb->lcd_fillrect(third_band_x, universal_y, band_width, band_height);
    } else {
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_drawrect(third_band_x, universal_y, band_width, band_height);
    }
    
    if(fourthband_color != RES_NONE) {        
        rb->lcd_set_foreground(band_data[fourthband_color].color_value);
        rb->lcd_fillrect(fourth_band_x, universal_y, band_width, band_height);
    } else {
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_drawrect(fourth_band_x, universal_y, band_width, band_height);
    }
    
    rb->lcd_set_foreground(fg);
            
    rb->lcd_update();
    return;
}
#endif

static void draw_resistor_text(enum color firstband_color,
                        enum color secondband_color,
                        enum color thirdband_color,
                        enum color fourthband_color)
{
    char resistance_vals_str[64];
    display->set_viewport(&text_vp);
    rb->snprintf(resistance_vals_str, sizeof(resistance_vals_str),
                 "%s - %s - %s - %s", band_data[firstband_color].name,
                                      band_data[secondband_color].name,
                                      band_data[thirdband_color].name,
                                      band_data[fourthband_color].name);
    rb->lcd_puts_scroll(resistance_val_x, lineno++, resistance_vals_str);
    rb->lcd_update();
}
    

static int calculate_resistance(enum color first_band,
                         enum color second_band, 
                         enum color third_band)
{
    int tens = band_data[first_band].resistance_value;
    int units = band_data[second_band].resistance_value;
    int multiplier = band_data[third_band].multiplier;
    int total_resistance_centiunits = (10 * tens + units ) * multiplier;   
    
    unit_abbrev = band_data[third_band].unit;

    return total_resistance_centiunits;
}

static enum color do_first_band_menu(void)
{
    int band_selection = 0;
    enum color band_color_selection = 0;
            
    MENUITEM_STRINGLIST(colors_menu_first, "First band colour:", NULL, 
                        "Black", "Brown", "Red", "Orange", "Yellow",
                         "Green", "Blue", "Violet", "Grey", "White");
    band_selection = rb->do_menu(&colors_menu_first, &band_selection, NULL, 
                                false);
    switch(band_selection) {
        case 0: /* Black */
            band_color_selection = RES_BLACK;
            break;
        case 1: /* Brown */
            band_color_selection = RES_BROWN;
            break;
        case 2: /* Red */
            band_color_selection = RES_RED;
            break;
        case 3: /* Orange */
            band_color_selection = RES_ORANGE;
            break;
        case 4: /* Yellow */
            band_color_selection = RES_YELLOW;
            break;
        case 5: /* Green */
            band_color_selection = RES_GREEN;
            break;
        case 6: /* Blue */
            band_color_selection = RES_BLUE;
            break;
        case 7: /* Violet */
            band_color_selection = RES_VIOLET;
            break;
        case 8: /* Grey */
            band_color_selection = RES_GREY;
            break;
        case 9: /* White */
            band_color_selection = RES_WHITE;
            break;
        default:
            band_color_selection = RES_INVALID;
            break;
        }
    return band_color_selection;
}
            
static enum color do_second_band_menu(void) 
{
    int band_selection = 0;
    enum color band_color_selection = 0;
            
    MENUITEM_STRINGLIST(colors_menu_second, "Second band colour:", NULL, 
                        "Black", "Brown", "Red", "Orange", "Yellow", 
                        "Green", "Blue", "Violet", "Grey", "White");
    band_selection = rb->do_menu(&colors_menu_second, &band_selection, NULL, 
                                false);
    switch(band_selection) {
        case 0: /* Black */
            band_color_selection = RES_BLACK;
            break;
        case 1: /* Brown */
            band_color_selection = RES_BROWN;
            break;
        case 2: /* Red */
            band_color_selection = RES_RED;
            break;
        case 3: /* Orange */
            band_color_selection = RES_ORANGE;
            break;
        case 4: /* Yellow */
            band_color_selection = RES_YELLOW;
            break;
        case 5: /* Green */
            band_color_selection = RES_GREEN;
            break;
        case 6: /* Blue */
            band_color_selection = RES_BLUE;
            break;
        case 7: /* Violet */
            band_color_selection = RES_VIOLET;
            break;
        case 8: /* Grey */
            band_color_selection = RES_GREY;
            break;
        case 9: /* White */
            band_color_selection = RES_WHITE;
            break;
        default:
            band_color_selection = RES_INVALID;
            break;
    }
    return band_color_selection;
}
    
static enum color do_third_band_menu(void) 
{
    int band_selection = 0;
    enum color band_color_selection = 0;
            
    MENUITEM_STRINGLIST(colors_menu_third, "Third band colour:", NULL, 
                        "Black", "Brown", "Red", "Orange", "Yellow",
                         "Green", "Blue", "Violet", "Grey", "White",
                         "Silver", "Gold");
    band_selection = rb->do_menu(&colors_menu_third, &band_selection, NULL, 
                                false);
    switch(band_selection) {
        case 0: /* Black */
            band_color_selection = RES_BLACK;
            break;
        case 1: /* Brown */
            band_color_selection = RES_BROWN;
            break;
        case 2: /* Red */
            band_color_selection = RES_RED;
            break;
        case 3: /* Orange */
            band_color_selection = RES_ORANGE;
            break;
        case 4: /* Yellow */
            band_color_selection= RES_YELLOW;
            break;
        case 5: /* Green */
            band_color_selection = RES_GREEN;
            break;
        case 6: /* Blue */
            band_color_selection = RES_BLUE;
            break;
        case 7: /* Violet */
            band_color_selection = RES_VIOLET;
            break;
        case 8: /* Grey */
            band_color_selection = RES_GREY;
            break;
        case 9: /* White */
            band_color_selection = RES_WHITE;
            break;
        case 10: /* Silver */
            band_color_selection = RES_SILVER;
            break;
        case 11: /* Gold */
            band_color_selection= RES_GOLD;
            break;
        default:
            band_color_selection = RES_INVALID;
            break;
    }
    return band_color_selection;
}
            
static enum color do_fourth_band_menu(void) 
{
    int band_selection = 0;
    enum color band_color_selection = 0;
            
    MENUITEM_STRINGLIST(colors_menu_fourth, "Fourth band colour:", NULL, 
                        "Gold", "Brown", "Red", "Silver", "(none)");
    band_selection = rb->do_menu(&colors_menu_fourth, &band_selection, NULL, 
                                false);
    switch(band_selection) {
        case 0: /* Gold */
            band_color_selection = RES_GOLD;
            break;
        case 1: /* Brown */
            band_color_selection = RES_BROWN;
            break;
        case 2: /* Red */
            band_color_selection = RES_RED;
            break;
        case 3: /* Silver */
            band_color_selection = RES_SILVER;
            break;
        case 4: /* (none) */
            band_color_selection = RES_NONE;
            break;
        default:
            band_color_selection = RES_INVALID;
            break;
    }
    return band_color_selection;
}

static void display_helpfile(void)
{
    rb->lcd_clear_display();
    /* some information obtained from wikipedia */
    static char * helpfile_text[] = {
        "Resistor Calculator Helpfile", "", "",
        "About resistors:", "", /* 7 */
        /* -- */
        "A", "resistor", "is", "a ", "two-terminal", "electronic", 
        "component", "that", "produces", "a", "voltage", "across", "its", 
        "terminals", "that", "is", "proportional", "to", "the", "electric",
        "current", "passing", "through", "it", "in", "accordance", "to",
        "Ohm's", "Law:", "", /* 29 */
        /* -- */
        "", "V = IR",
        "", "I = V/R",
        "", "and",
        "", "R = V/I", "", "",
        "Where", "V", "=", "voltage, ", "I", "=", "current", "(in", "amps)", 
        "and", "R", "=", "resistance", "(measured", "in", "Ohms).", "", "",
         /* 28 */
        /* -- */
        "The", "primary", "characteristics", "of", "a", "resistor", "are",
        "the", "resistance,", "the", "tolerance,", "and", "the", "maximum",
        "working", "voltage", "and", "the", "power", "rating.", "At", 
        "this", "time,", "this", "calculator", "only", "utilises", "the",
        "resistance", "and", "tolerance.", "", "", /* 33 */
        /* -- */
        "The", "Ohm", "is", "the", "SI", "unit", "of", "resistance,", "and",
        "common", "multiples", "of", "that", "include", "the", "kiloohm", 
        "(KOhm", "-", "1x10^3)", "and", "the", "megaohm", "(MOhm",
        "-", "1x10^6),", "both", "of", "which", "are", "supported", "by",
        "this", "calculator.", "", "", /* 34 */
        /* -- */
        "Resistors", "in", "parallel:", "", /* 4 */
        /* -- */
        "1/Rtotal", "=", "1/R1", "+", "1/R2", "...", "+", "1/Rn", "", /* 9*/
        /* -- */
        "", "Resistors", "in", "series:", "", /* 5 */
        /* -- */
        "Rtotal", "=", "R1", "+", "R2", "...", "+", "Rn", "", /* 9 */
        /* -- */
        "", "How to use this calculator", "", /* 3 */
        /* -- */
        "This", "calculator", "has", "three", "modes:", "",
        "Resistance", "to", "colour", "codes,", "",
        "Colour", "codes", "to", "resistance", "",
        "and", "LED", "resistance", "calculator", "", "",
        /* -- */
        "At", "this", "time", "there", "is", "only", "support", "for",
        "four-", "band", "resistors.", "", "",
        /* -- */
        "In", "Colour", "to", "Resistance", "mode", "use", "the", "menus",
        "to", "input", "(in", "order)", "the", "bands", "of", "the",
        "resistor", "for", "which", "you", "would", "like", "to", "know",
        "the", "resistance.", "", "",
        /* -- */
        "In", "Resistance", "to", "Colour", "mode,", "use", "the", "menus", 
        "to", "select", "which", "unit", "to", "use", "(choose", "from", "Ohms,",
        "KiloOhms", "and", "MegaOhms)", "and", "the", "on-screen", "keyboard",
        "to", "input", "the", "value", "of", "the", "resistor", "that", "you",
        "would", "like", "to", "know", "the", "colour", "codes", "of.",
        "Output", "will", "be", "both", "graphical", "(with", "bands", "of",
        "the", "resistor", "shown", "in", "their", "corresponding", "colours", 
        "-", "colour", "targets", "only)", "and", "textually.", "","",
        /* -- */
        "LED", "resistor", "calculator", "mode", "is", "used", "to", "determine",
        "the", "resistor", "necessary", "to", "light", "a", "LED", "safely",
        "at", "a", "given", "voltage.", "First,", "select", "the", "voltage", 
        "that", "the", "LED", "will", "use", "(the", "first", "option", "is", 
        "the", "most", "common", "and", "is", "a", "safe", "guess)", "and", "the",
        "current", "that", "it", "will", "draw", "(likewise", "with", "the",
        "first", "option).", "Then", "use", "the", "onscreen", "keyboard", "to",
        "type", "in", "the", "supply", "voltage", "and,", "if", "selected,", 
        "the", "custom", "foreward", "current.", "", 
        "Disclaimer:", "this", 
        "calculator", "produces", "safe", "estimates,", "but", "use", "your",
        "own", "judgement", "when", "using", "these", "output", "values.",
        "Power", "rating", "and", "displayed", "resistance", "are", "rounded",
        "up", "to", "the", "nearest", "common", "value."
    };
    static struct style_text formatting[] = {
        { 0, TEXT_CENTER|TEXT_UNDERLINE },
        { 3, TEXT_UNDERLINE },
        { 159, TEXT_UNDERLINE },
        LAST_STYLE_ITEM
    };
            
    display_text(ARRAYLEN(helpfile_text), helpfile_text, formatting,
                 NULL, true);
    return;
}
    
static void led_resistance_calc(void)
{
    backlight_ignore_timeout();
    int voltage_menu_selection, button_press, j, k, l, foreward_current = 0;
    int fwd_current_selection = 0;
    bool quit = false;
    char kbd_buffer [5];
    char fwd_kbd_buffer [5];
    int input_voltage, led_voltage = 0;

    int resistance = 0;
    int rounded_resistance = 0;
    int temp;
    int power_rating_in = 0;
    int rounded_power_rating = 0;
    int out_int = 0;
    char current_out_str [16];
    char true_current_out_str [40];
    char rounded_resistance_out_str [40];
    char power_rating_out_str [40];
                         
    int power_ten, first_band_int, second_band_int = 0;
    
    enum color first_band;
    enum color second_band;
    enum color multiplier;
    enum color fourth_band = RES_NONE;
    
    rb->lcd_clear_display();
    
    MENUITEM_STRINGLIST(voltage_menu, "Select LED voltage:", NULL,
                        "2v (Common red, orange)", "1.5v (IR)", "2.1v (Yellow)",
                        "2.2v (Green)", "3.3v (True green, blue, white, UV)", 
                        "4.6v (Blue - 430nm)");
    MENUITEM_STRINGLIST(fwd_current_menu, "Select foreward current:", NULL,
                 "20mA - Most common for 5mm and 3mm LEDs - select if unsure.",
                 "Key in other (only if already known)");
    
    while(!quit) {
        int ret;
        ret = voltage_menu_selection = rb->do_menu(&voltage_menu, 
                      &voltage_menu_selection, NULL, false);
        if(ret<0) break;
        ret = fwd_current_selection = rb->do_menu(&fwd_current_menu, 
                      &fwd_current_selection,  NULL, false);
        if(ret<0) break;
        rb->lcd_clear_display();


        rb->splash(HZ*2, "(First) Input the supply voltage:");
        memset(kbd_buffer,0,sizeof(kbd_buffer));
        rb->kbd_input(kbd_buffer, sizeof(kbd_buffer));
        input_voltage = rb->atoi(kbd_buffer);
        if(input_voltage == 0) break;

        if(input_voltage != (int)input_voltage) {
            input_voltage *= 10;
            }
        else { input_voltage *= 100; }
                
        switch(voltage_menu_selection) {
            case 0: /* 2v */
                led_voltage = 200;
                break;
            case 1: /* 1.5v */
                led_voltage = 150;
                break;
            case 2: /* 2.1 */
                led_voltage = 210;
                break;
            case 3:
                led_voltage = 220;
                break;
            case 4:
                led_voltage = 330;
                break;
            case 5:
                led_voltage = 460;
                break;
        }
        switch(fwd_current_selection) {
            case 0: /* 20mA */
                foreward_current = 2; /* 20mA * 100 */
                break;
            case 1:
                rb->lcd_clear_display();
                rb->splash(HZ*2, "Input the foreward current, in mA");
                memset(fwd_kbd_buffer,0,sizeof(fwd_kbd_buffer));
                rb->kbd_input(fwd_kbd_buffer, sizeof(fwd_kbd_buffer));
                foreward_current = ((rb->atoi(fwd_kbd_buffer))/10);
                break;
        }
        
        if(foreward_current == 0) break;

        rb->lcd_clear_display();
        
        resistance = (input_voltage - led_voltage) / foreward_current;
        out_int = resistance;
        
        int total_common_values = 11;
        int total_power_values = 9;
        
        if(led_voltage > input_voltage) {
            rb->splash(HZ, "Problem: LED voltage is higher than the source.");
            break;
        }
        
        for(j = 0; j < total_common_values; j++) {
            for(k = 1; k < 5; k++) {
                if( resistance == (common_values[j] * powi(10, k))) {
                    rounded_resistance = (common_values[j] * powi(10, k)); 
                                          /* perfect match */
                    break;
                }
                else if(resistance >= (common_values[j] * powi(10, k)) && 
                        resistance <= (common_values[j+1] * powi(10, k))) {
                    rounded_resistance = (common_values[j+1] * powi(10, k));
                           /* the higher resistance, to be safe */
                    break;
                }
                else { break; }
            }
        }
            
        if(rounded_resistance == 0) 
        {
            rb->splash(HZ, "Problem: Input voltage too high.");
            break;
        }
        power_rating_in = ((input_voltage/100)*(input_voltage/100)*1000 / rounded_resistance);
        /* in mW */
        for(l = 0; l < total_power_values; l++) {
            if((int)power_rating_in == power_ratings[l]) {
                rounded_power_rating = (power_ratings[l]);
                break;
            }
            else if(power_rating_in >= power_ratings[l] && 
                    power_rating_in <= power_ratings[l+1]) {
                rounded_power_rating = power_ratings[l+1];
                break;
            }
            else { break; }
        }
        
        get_power_rating_str(rounded_power_rating);    
                 
        power_ten=0;
        temp=rounded_resistance;
        while(temp>=100) {
            temp/=10;
            power_ten++;
        }
        first_band_int=temp/10;
        second_band_int=temp%10;
        
        first_band = get_band_rtoc(first_band_int);
        second_band = get_band_rtoc(second_band_int);
        multiplier = get_band_rtoc(power_ten);
    
        rb->lcd_clear_display();
        lineno = INITIAL_TEXT_Y;
#ifndef USE_TEXT_ONLY
        draw_resistor(first_band, second_band, multiplier, fourth_band);
#endif
        draw_resistor_text(first_band, second_band, multiplier, fourth_band);
    
        rb->snprintf(current_out_str, sizeof(current_out_str), "%d mA", 
                         (foreward_current*10));
        
        rb->snprintf(true_current_out_str, sizeof(true_current_out_str), 
                     "Input: %dv, %d Ohms @ %s", (input_voltage/100), 
                     out_int, current_out_str);
        rb->snprintf(rounded_resistance_out_str, 
                     sizeof(rounded_resistance_out_str), 
                     "Rounded/displayed: [%d %s]", rounded_resistance, 
                     band_data[multiplier].unit);
        rb->snprintf(power_rating_out_str, sizeof(power_rating_out_str), 
                     "Recommended: %s or greater",
                     get_power_rating_str(rounded_power_rating));

        display->set_viewport(&text_vp);
        rb->lcd_puts_scroll(resistance_val_x, lineno++, true_current_out_str);
        rb->lcd_puts_scroll(resistance_val_x, lineno++, rounded_resistance_out_str);
        rb->lcd_puts_scroll(resistance_val_x, lineno++, power_rating_out_str);

        rb->lcd_update();
        
        button_press = rb->button_get(true);
        switch(button_press) {
            case PLA_SELECT:
                break;
            default:
                quit = true;
                backlight_use_settings();
                break;
        }
    }
    display->set_viewport(&text_vp);
    rb->lcd_stop_scroll();
    display->set_viewport(&screen_vp);
    rb->lcd_clear_display();
}

        
static void resistance_to_color(void) 
{
    backlight_ignore_timeout();
    int menu_selection;
    int menu_selection_tol;
    int button_press;
    int i;
    bool quit = false;
    char kbd_buffer [10];
    int kbd_input_int;
    int temp;
    int in_resistance_int;
    
    int power_ten=0;
    int first_band_int = 0;
    int second_band_int = 0;
    
    enum color first_band;
    enum color second_band;
    enum color multiplier;
    enum color fourth_band = 0;
    enum color units_used = 0;
    
    char out_str[20];
    
    for(i=0; i<=10; i++) { kbd_buffer[i] = 0; }
    /* This cleans out the mysterious garbage that appears */
    rb->lcd_clear_display();
    rb->splash(HZ/2, "Resistance to Colour");
    MENUITEM_STRINGLIST(r_to_c_menu, "Select unit to use:", NULL, 
                        "Ohms", "Kiloohms (KOhms)", "Megaohms (MOhms)",
                        "Gigaohms (GOhms)");
    MENUITEM_STRINGLIST(r_to_c_menu_tol, "Tolerance to display:", NULL,
                        "5%", "10%", "1%", "2%", "20%");
      
    while(!quit) {
        int ret;
        ret=menu_selection = rb->do_menu(&r_to_c_menu, &menu_selection,
                                     NULL, false);
        if(ret<0) break;
        
        rb->kbd_input(kbd_buffer, sizeof(kbd_buffer));
        /* As stated above somewhere, we (I) need to make a calculator-like
           keypad, that keyboard isn't all that fun to use. */
        ret = rb->do_menu(&r_to_c_menu_tol, &menu_selection_tol,
                                         NULL, false);
        if(ret<0) break;

        switch(menu_selection_tol) {
            case 0: /* 5% */
                fourth_band = RES_GOLD;
                break;
            case 1: /* 10% */
                fourth_band = RES_SILVER;
                break;
            case 2: /* 1% */
                fourth_band = RES_BROWN;
                break;
            case 3: /* 2% */
                fourth_band = RES_RED;
                break;
            case 4: /* 20% */
                fourth_band = RES_NONE;
                break;
        }
            
        kbd_input_int = rb->atoi(kbd_buffer);
        in_resistance_int = kbd_input_int;
        
        switch(menu_selection) {
            case 0:
                power_ten=0;
                units_used = RES_BLACK;
                break;
            case 1: /* KOhms */
                power_ten=3;
                units_used = RES_ORANGE;
                break;
            case 2: /* MOhms */
                power_ten=6;
                units_used = RES_BLUE;
                break;
            case 3: /* GOhms */
                power_ten=9;
                units_used = RES_WHITE;
                break;
        }

        temp=kbd_input_int;
        while(temp>=100) {
            temp/=10;
            power_ten++;
        }
        first_band_int=temp/10;
        second_band_int=temp%10;

        first_band = get_band_rtoc(first_band_int);
        second_band = get_band_rtoc(second_band_int);
        multiplier = get_band_rtoc(power_ten);

        if( first_band == RES_INVALID
         || second_band == RES_INVALID
         || multiplier == RES_INVALID) {
            rb->splashf(HZ, "%d %s can not be represented",
                        in_resistance_int,band_data[units_used].unit);
            return;
        }
        
        rb->lcd_clear_display();
        lineno = INITIAL_TEXT_Y;
#ifndef USE_TEXT_ONLY
        draw_resistor(first_band, second_band, multiplier, fourth_band);
#endif                     
        draw_resistor_text(first_band, second_band, multiplier, fourth_band);
        
        rb->snprintf(out_str, sizeof(out_str), "Input: %d %s", in_resistance_int,
                     band_data[units_used].unit);
        display->set_viewport(&text_vp);
        rb->lcd_puts_scroll(r_to_c_out_str_x, lineno++, out_str);
        rb->lcd_update();
        
        button_press = rb->button_get(true);
        switch(button_press) {
            case PLA_SELECT:
                break;
            default:
                quit = true;
                backlight_use_settings();
                break;
        }
    }
    display->set_viewport(&text_vp);
    rb->lcd_stop_scroll();
    display->set_viewport(&screen_vp);
    rb->lcd_clear_display();
}
    
static void color_to_resistance(void) 
{
    backlight_ignore_timeout();
    bool quit = false;
    int button_input = 0;
            
    /* The colors of the bands */
    enum color first_band = 0;
    enum color second_band = 0;
    enum color third_band = 0;
    enum color fourth_band = 0;
           
    int total_resistance_centiunits = 0;
    char total_resistance_str [35];
            
    rb->splash(HZ/2, "Colour to resistance");
    rb->lcd_clear_display();
            
    while(!quit) {
        first_band = do_first_band_menu();
        if(first_band==RES_INVALID) break;

        second_band = do_second_band_menu();
        if(second_band==RES_INVALID) break;

        third_band = do_third_band_menu();
        if(third_band==RES_INVALID) break;

        fourth_band = do_fourth_band_menu();
        if(third_band==RES_INVALID) break;
                    
        total_resistance_centiunits = calculate_resistance(first_band, 
                                                           second_band,
                                                           third_band);

        rb->lcd_clear_display();
        lineno = INITIAL_TEXT_Y;
#ifndef USE_TEXT_ONLY
        draw_resistor(first_band, second_band, third_band, fourth_band);
#endif                     
        draw_resistor_text(first_band, second_band, third_band, fourth_band);

        if(total_resistance_centiunits % 100 == 0) {
            /* No decimals */
            rb->snprintf(total_resistance_str, sizeof(total_resistance_str), 
                         "Resistance: %d %s",
                         total_resistance_centiunits/100,
                         unit_abbrev);
        }
        else {
            rb->snprintf(total_resistance_str, sizeof(total_resistance_str), 
                         "Resistance: %d.%2.2d %s",
                         total_resistance_centiunits/100,
                         total_resistance_centiunits%100,
                         unit_abbrev);
        }
        display->set_viewport(&text_vp);
        rb->lcd_puts_scroll(total_resistance_str_x, lineno++,
                            total_resistance_str);
        rb->lcd_puts_scroll(tolerance_str_x, lineno++,
                            get_tolerance_str(fourth_band));
        rb->lcd_update();
                    
        button_input = rb->button_get(true);
        switch(button_input) {
            case PLA_RIGHT:
                break;
            case PLA_EXIT:
            case PLA_SELECT:
            default:
                quit = true;
                backlight_use_settings();
                break;
        }                   
    }
    display->set_viewport(&text_vp);
    rb->lcd_stop_scroll();
    display->set_viewport(&screen_vp);
    rb->lcd_clear_display();
    return;
}

enum plugin_status plugin_start(const void* nothing) 
{
    (void)nothing;
    rb->lcd_clear_display();
    rb->lcd_update();
    int main_menu_selection = 0;
    bool menuquit = false;

    display = rb->screens[0];
    rb->viewport_set_defaults(&screen_vp,0);
    rb->viewport_set_defaults(&text_vp,0);
    rb->viewport_set_defaults(&bitmap_vp,0);
#ifndef USE_TEXT_ONLY
    bitmap_vp.y = RESISTOR_BMP_Y + screen_vp.y;
    bitmap_vp.height = BMPHEIGHT_resistor;
    text_vp.y = bitmap_vp.y + bitmap_vp.height;
    text_vp.height = screen_vp.height - text_vp.y;
#endif

    MENUITEM_STRINGLIST(main_menu, "Resistor Code Calculator:", NULL, 
                        "Colours -> Resistance", "Resistance -> Colours", 
                        "LED resistor calculator", "Help", "Exit");
    while (!menuquit) {
        display->set_viewport(&screen_vp);
        main_menu_selection = rb->do_menu(&main_menu, &main_menu_selection, 
                                          NULL, false);
        switch(main_menu_selection) {
            case 0:
                color_to_resistance();
                break;
            case 1:
                resistance_to_color();
                break;
            case 2:
                led_resistance_calc();
                break;
            case 3:
                display_helpfile();
                break;
            case 4:
                menuquit = true;
                break;
            case MENU_ATTACHED_USB:
                return PLUGIN_USB_CONNECTED;
        }
    }
    return PLUGIN_OK;
}
