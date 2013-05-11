#ifndef _LCDIF_RK27XX_H
#define _LCDIF_RK27XX_H

#define LCDIF_16BIT 16
#define LCDIF_18BIT 18

void lcd_cmd(unsigned int cmd);
void lcd_data(unsigned int data);
void lcd_write_reg(unsigned int reg, unsigned int val);
void lcdctrl_bypass(unsigned int on_off);
void lcd_display_init(void);

void lcd_set_gram_area(int x_start, int y_start, int x_end, int y_end);

#endif /* _LCDIF_RK27XX_H */
