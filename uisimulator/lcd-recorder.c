/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * Hardware-specific implementations for the Archos Recorder LCD.
 *
 * Archos Jukebox Recorder LCD functions.
 * Solomon SSD1815Z controller and Shing Yih Technology G112064-30 LCD.
 *
 */

/* LCD command codes */
#define	LCD_CNTL_RESET		0xe2	/* Software reset */
#define	LCD_CNTL_POWER		0x2f	/* Power control */
#define	LCD_CNTL_CONTRAST	0x81	/* Contrast */
#define	LCD_CNTL_OUTSCAN	0xc8	/* Output scan direction */
#define	LCD_CNTL_SEGREMAP	0xa1	/* Segment remap */
#define	LCD_CNTL_DISPON		0xaf	/* Display on */

#define	LCD_CNTL_PAGE		0xb0	/* Page address */
#define	LCD_CNTL_HIGHCOL	0x10	/* Upper column address */
#define	LCD_CNTL_LOWCOL		0x00	/* Lower column address */


/*
 * Initialize LCD
 */
void lcd_init (void)
{
  /* Initialize PB0-3 as output pins */
  PBCR2 &= 0xff00; /* MD = 00 */
  PBIOR |= 0x000f; /* IOR = 1 */

  /* Initialize LCD */
  lcd_write (TRUE, LCD_CNTL_RESET);
  lcd_write (TRUE, LCD_CNTL_POWER);
  lcd_write (TRUE, LCD_CNTL_SEGREMAP);
  lcd_write (TRUE, LCD_CNTL_OUTSCAN);
  lcd_write (TRUE, LCD_CNTL_CONTRAST);
  lcd_write (TRUE, 0x30); /* contrast parameter */
  lcd_write (TRUE, LCD_CNTL_DISPON);

  lcd_clear();
  lcd_update();
}

/*
 * Update the display.
 * This must be called after all other LCD funtions that change the display.
 */
void lcd_update (void)
{
  int row, col;

  /* Copy display bitmap to hardware */
  for (row = 0; row < DISP_Y/8; row++) {
    lcd_write (TRUE, LCD_CNTL_PAGE | (row & 0xf));
    lcd_write (TRUE, LCD_CNTL_HIGHCOL);
    lcd_write (TRUE, LCD_CNTL_LOWCOL);

    for (col = 0; col < DISP_X; col++)
      lcd_write (FALSE, display[row][col]);
  }
}

#if 0
/*
 * Display a string at current position
 */
void lcd_string (const char *text, BOOL invert)
{
  int ch;

  while ((ch = *text++) != '\0') {
    if (lcd_y > DISP_Y-CHAR_Y) {
      /* Scroll (8 pixels) */
      memcpy (display[0], display[1], DISP_X*(DISP_Y/8-1));
      lcd_y -= 8;
    }

    if (ch == '\n')
      lcd_x = DISP_X;
    else {
      lcd_char (ch, invert);
      lcd_x += CHAR_X;
    }

    if (lcd_x > DISP_X-CHAR_X) {
      /* Wrap to next line */
      lcd_x = 0;
      lcd_y += CHAR_Y;
    }
  }
}
#endif


/*
 * Write a byte to LCD controller.
 * command is TRUE if value is a command byte.
 */
static void lcd_write (BOOL command, int value)
{
  int i;

  /* Read PBDR, clear LCD bits */
  int pbdr = PBDR & ~(PBDR_LCD_CS1|PBDR_LCD_DC|PBDR_LCD_SDA|PBDR_LCD_SCK);
  if (!command)
    pbdr |= PBDR_LCD_DC; /* set data indicator */

  /* Send each bit, starting with MSB */
  for (i = 0; i < 8; i++) {
    if (value & 0x80) {
      /* Set data, toggle clock */
      PBDR = pbdr | PBDR_LCD_SDA;
      PBDR = pbdr | PBDR_LCD_SDA | PBDR_LCD_SCK;
    }
    else {
      /* Clear data, toggle clock */
      PBDR = pbdr;
      PBDR = pbdr | PBDR_LCD_SCK;
    }
    value <<= 1;
  }
}
