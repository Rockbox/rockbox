#ifndef _LCDIF_RK27XX_H
#define _LCDIF_RK27XX_H

enum lcdif_mode_t {
    LCDIF_16BIT,
    LCDIF_18BIT
};

unsigned int lcd_data_transform(unsigned int data);

void lcd_cmd(unsigned int cmd);
void lcd_data(unsigned int data);
void lcd_write_reg(unsigned int reg, unsigned int val);
void lcdif_init(enum lcdif_mode_t mode);

#endif /* _LCDIF_RK27XX_H */
