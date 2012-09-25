#ifndef _LCDIF_RK27XX_H
#define _LCDIF_RK27XX_H

enum lcdif_mode_t {
    LCDIF_16BIT,
    LCDIF_18BIT
};

void lcd_cmd(unsigned int cmd);
void lcd_data(unsigned int data);
void lcd_write_reg(unsigned int reg, unsigned int val);
void lcdctrl_bypass(unsigned int on_off);
void lcd_display_init(void);

void lcd_set_gram_area(int x, int y, int width, int height);

#endif /* _LCDIF_RK27XX_H */
