#ifndef _LCDIF_RK27XX_H
#define _LCDIF_RK27XX_H

unsigned int lcd_data_transform(unsigned int data);

void lcd_cmd(unsigned int cmd);
void lcd_data(unsigned int data);
void lcd_write_reg(unsigned int reg, unsigned int val);
void lcd_display_init(void);

#endif /* _LCDIF_RK27XX_H */
