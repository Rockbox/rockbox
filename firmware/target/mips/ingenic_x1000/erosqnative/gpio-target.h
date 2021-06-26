/* -------------------- NOTES ------------------- */

/* I don't think we have any devices on I2C1, the pins /may/ be reused. */
/* DEFINE_PINGROUP(I2C1,           GPIO_C,      3 << 26,   GPIOF_DEVICE(0)) */

/* OF has SD Card power listed as 0x2a - PB10, but it seems to work without. */

/* I think BT power reg is pin 0x53 - C19 */

/* USB_DETECT D3 chosen by trial-and-error. */

/* I have a suspicion this isn't right for AXP_IRQ,
 * and it's not used right now anyway.  copied from m3k. */
/* DEFINE_GPIO(AXP_IRQ,            GPIO_PB(10),    GPIOF_INPUT) */

/* ---------------------------------------------- */

/*              Name            Port    Pins            Function */
DEFINE_PINGROUP(LCD_DATA,       GPIO_A, 0xffff <<  0,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(LCD_CONTROL,    GPIO_B,   0x1a << 16,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(MSC0,           GPIO_A,   0x3f << 20,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(SFC,            GPIO_A,   0x3f << 26,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(I2S,            GPIO_B,   0x1f <<  0,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(I2C2,           GPIO_D,      3 <<  0,   GPIOF_DEVICE(1))

/*          Name                Pin             Function */
/* mute DAC - 0 - mute, 1 - play. Affects both HP and LO. */
DEFINE_GPIO(PCM5102A_XMIT,      GPIO_PB(12),    GPIOF_OUTPUT(0))

/* mute HP amp, no effect on LO. 0 - mute, 1 - play */
DEFINE_GPIO(MAX97220_SHDN,      GPIO_PB(8),     GPIOF_OUTPUT(0))

/* mute audio mux, only affects Headphone out.
 * 0 - play, 1 - mute */
DEFINE_GPIO(ISL54405_MUTE,      GPIO_PB(15),    GPIOF_OUTPUT(1))

/* switches HP on/off - 0 HP on, 1 hp off, has no effect on LO.
 * As best I can tell, it switches HP Out sources between HP amp and something
 * not implemented - there seem to be resistors missing. */
DEFINE_GPIO(ISL54405_SEL,       GPIO_PB(5),     GPIOF_OUTPUT(0))

/* DAC AVDD */
DEFINE_GPIO(PCM5102A_ANALOG_PWR,       GPIO_PB(9),     GPIOF_OUTPUT(0))

/* Headphone Amp power */
DEFINE_GPIO(MAX97220_POWER,     GPIO_PB(6),     GPIOF_OUTPUT(0))

/* SD card */
DEFINE_GPIO(MSC0_CD,            GPIO_PB(11),    GPIOF_INPUT)

/* USB */
DEFINE_GPIO(USB_DETECT,         GPIO_PD(3),     GPIOF_INPUT)
DEFINE_GPIO(USB_DRVVBUS,        GPIO_PB(25),    GPIOF_OUTPUT(0))

/* LCD */
DEFINE_GPIO(LCD_PWR,            GPIO_PB(14),    GPIOF_OUTPUT(0))
DEFINE_GPIO(LCD_RESET,          GPIO_PB(13),    GPIOF_OUTPUT(0))
DEFINE_GPIO(LCD_CE,             GPIO_PB(18),    GPIOF_OUTPUT(1))
DEFINE_GPIO(LCD_RD,             GPIO_PB(16),    GPIOF_OUTPUT(1))

/* Buttons */
DEFINE_GPIO(BTN_PLAY,           GPIO_PA(16),    GPIOF_INPUT)
DEFINE_GPIO(BTN_VOL_UP,         GPIO_PA(17),    GPIOF_INPUT)
DEFINE_GPIO(BTN_VOL_DOWN,       GPIO_PA(19),    GPIOF_INPUT)
DEFINE_GPIO(BTN_POWER,          GPIO_PB(7),     GPIOF_INPUT)
DEFINE_GPIO(BTN_MENU,           GPIO_PB(28),    GPIOF_INPUT)
DEFINE_GPIO(BTN_BACK,           GPIO_PD(5),     GPIOF_INPUT)
DEFINE_GPIO(BTN_PREV,           GPIO_PD(4),     GPIOF_INPUT)
DEFINE_GPIO(BTN_NEXT,           GPIO_PC(24),    GPIOF_INPUT)
DEFINE_GPIO(BTN_SCROLL_A,       GPIO_PB(24),    GPIOF_INPUT)
DEFINE_GPIO(BTN_SCROLL_B,       GPIO_PB(23),    GPIOF_INPUT)
