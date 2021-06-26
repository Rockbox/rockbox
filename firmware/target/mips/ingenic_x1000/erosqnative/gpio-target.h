/*              Name            Port    Pins            Function */
DEFINE_PINGROUP(LCD_DATA,       GPIO_A, 0xffff <<  0,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(LCD_CONTROL,    GPIO_B,   0x1a << 16,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(MSC0,           GPIO_A,   0x3f << 20,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(SFC,            GPIO_A,   0x3f << 26,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(I2S,            GPIO_B,   0x1f <<  0,   GPIOF_DEVICE(1))
//DEFINE_PINGROUP(DMIC,           GPIO_B,      3 << 21,   GPIOF_DEVICE(0)) // don't think we have this
//DEFINE_PINGROUP(I2C0,           GPIO_B,      3 << 23,   GPIOF_DEVICE(0)) // definitely reused as scrollwheel inputs
DEFINE_PINGROUP(I2C1,           GPIO_C,      3 << 26,   GPIOF_DEVICE(0)) // don't think we have a device here... reused?
DEFINE_PINGROUP(I2C2,           GPIO_D,      3 <<  0,   GPIOF_DEVICE(1))

/*          Name                Pin             Function */
/* audio control lines */
/* mute DAC - 0 - mute, 1 - play. Affects both HP and LO. */
DEFINE_GPIO(PCM5102A_XMIT,      GPIO_PB(12),    GPIOF_OUTPUT(0))

/* mute HP amp, no effect on LO. 0 - mute, 1 - play */
DEFINE_GPIO(MAX97220_SHDN,      GPIO_PB(8),     GPIOF_OUTPUT(0))

/* mute audio mux, which... just appears to mute the HP out?
 * doesn't seem to affect LO at all.
 * 0 - play, 1 - mute */
DEFINE_GPIO(ISL54405_MUTE,      GPIO_PB(15),    GPIOF_OUTPUT(1))

/* appears to switch HP in/out of circuit - 0 HP on, 1 hp off,
 * LO appears to always be in circuit? which seems a weird decision...
 * As best I can tell, it switches HP Out sources between HP amp and something
 * not implemented - there seem to be resistors missing. */
DEFINE_GPIO(ISL54405_SEL,       GPIO_PB(5),     GPIOF_OUTPUT(0))

/* DAC AVDD */
DEFINE_GPIO(ANALOG_POWER,       GPIO_PB(9),     GPIOF_OUTPUT(0))

/* Seems to power Headphone Amp */
DEFINE_GPIO(LO_POWER,           GPIO_PB(6),     GPIOF_OUTPUT(0))

/* def ok */
DEFINE_GPIO(MSC0_CD,            GPIO_PB(11),    GPIOF_INPUT)
/* OF has SD Card power listed as 0x2a - PB10, but apparently we don't need that? */

/* I think rtc32k is pin 0x2a - B26 */
/* I think BT power reg is pin 0x53 - C19 */

/* might be right? copied from m3k B10 */
DEFINE_GPIO(AXP_IRQ,            GPIO_PB(10),    GPIOF_INPUT)

/* other candidates for usb_detect that technically did
 * /something/ when I tested them:
 * B: 1, 2, 29, 31
 * C: 0, 2 */
/* seems to work - assume correct until proven otherwise */
DEFINE_GPIO(USB_DETECT,         GPIO_PD(3),    GPIOF_INPUT)
DEFINE_GPIO(USB_DRVVBUS,        GPIO_PB(25),    GPIOF_OUTPUT(0))

/* copied from m3k, not right - duplicate. don't think its used anyway. */
//DEFINE_GPIO(USB_ID,             GPIO_PB(7),     GPIOF_INPUT)

/* LCD - definitely ok. don't think it would work if these were wrong */
DEFINE_GPIO(LCD_PWR,            GPIO_PB(14),    GPIOF_OUTPUT(0))
DEFINE_GPIO(LCD_RESET,          GPIO_PB(13),    GPIOF_OUTPUT(0))
DEFINE_GPIO(LCD_CE,             GPIO_PB(18),    GPIOF_OUTPUT(1))
DEFINE_GPIO(LCD_RD,             GPIO_PB(16),    GPIOF_OUTPUT(1))

/* Buttons - all work */
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
