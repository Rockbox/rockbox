/* -------------------- NOTES ------------------- */

/* I have a suspicion this isn't right for AXP_IRQ,
 * and it's not used right now anyway.  copied from m3k. */
/* DEFINE_GPIO(AXP_IRQ,            GPIO_PB(10),    GPIOF_INPUT) */

/* ---------------------------------------------- */

/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING */
/* ======================================================= */
/*         DO NOT CHANGE THE ORDER OF THESE DEFINES        */
/*         WITHOUT CONSIDERING GPIO-X1000.C!!              */
/* ======================================================= */
/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING */

/**************************/
/* PIN GROUPINGS          */
/************************ */
/*              Name            Port    Pins            Function */
DEFINE_PINGROUP(LCD_DATA,       GPIO_A, 0xffff <<  0,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(LCD_CONTROL,    GPIO_B,   0x1a << 16,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(MSC0,           GPIO_A,   0x3f << 20,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(SFC,            GPIO_A,   0x3f << 26,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(I2S,            GPIO_B,   0x1f <<  0,   GPIOF_DEVICE(1))
DEFINE_PINGROUP(I2C1,           GPIO_C,      3 << 26,   GPIOF_DEVICE(0))
DEFINE_PINGROUP(I2C2,           GPIO_D,      3 <<  0,   GPIOF_DEVICE(1))

/*          Name                Pin             Function */
/*************************/
/* AUDIO STUFF           */
/*************************/
/* mute DAC: 0 - mute, 1 - play */
/* Note: This seems to actually be power to the DAC in general,
 * at least on the ES9018K2M devices. Was "DAC_XMIT". */
DEFINE_GPIO(DAC_PWR,           GPIO_PB(12),    GPIOF_OUTPUT(0))

/* Headphone Amp power */
DEFINE_GPIO(HPAMP_POWER,        GPIO_PB(6),     GPIOF_OUTPUT(0))

/* DAC AVDD */
DEFINE_GPIO(DAC_ANALOG_PWR,     GPIO_PB(9),     GPIOF_OUTPUT(0))

/* SPDIF power? */
// gpio 53 --> port B, pin 21
DEFINE_GPIO(SPDIF_PWR,          GPIO_PB(21),    GPIOF_OUTPUT(0))

/* mute HP amp: 0 - mute, 1 - play */
DEFINE_GPIO(HPAMP_SHDN,         GPIO_PB(8),     GPIOF_OUTPUT(0))

/* mute audio mux: 0 - play, 1 - mute */
DEFINE_GPIO(STEREOSW_MUTE,      GPIO_PB(15),    GPIOF_OUTPUT(1))

/*
 * Original devices: switches HP on/off - 0 HP on, 1 HP off, no effect on LO.
 * Newer devices: switches between HP and LO - 0 HP, 1 LO.
 */
DEFINE_GPIO(STEREOSW_SEL,       GPIO_PB(5),     GPIOF_OUTPUT(0))

/**************/
/* SD and USB */
/**************/
/* SD card */
DEFINE_GPIO(MMC_PWR,            GPIO_PB(10),    GPIOF_OUTPUT(1))
DEFINE_GPIO(MSC0_CD,            GPIO_PB(11),    GPIOF_INPUT)

/* USB */
/* USB_DETECT D3 chosen by trial-and-error. */
DEFINE_GPIO(USB_DETECT,         GPIO_PD(3),     GPIOF_INPUT)
DEFINE_GPIO(USB_DRVVBUS,        GPIO_PB(25),    GPIOF_OUTPUT(0))

/* TCS1421_CFG (USB) CFG1 */
// set TCS1421_CFG0:1 to 00, Upstream Facing Port (tried others, they don't work)
// GPIO 62 --> PORT B, PIN 30
DEFINE_GPIO(TCS1421_CFG1,       GPIO_PB(30),    GPIOF_OUTPUT(0))

/* BL POWER */
// gpio 89 --> port c, pin 25
// oh, this is probably backlight power, isn't it?
DEFINE_GPIO(BL_PWR,             GPIO_PC(25),    GPIOF_OUTPUT(0))

/************/
/* LCD      */
/************/
/* LCD */
DEFINE_GPIO(LCD_RESET,          GPIO_PB(13),    GPIOF_OUTPUT(0))
DEFINE_GPIO(LCD_CE,             GPIO_PB(18),    GPIOF_OUTPUT(1))
DEFINE_GPIO(LCD_RD,             GPIO_PB(16),    GPIOF_OUTPUT(1))

/************/
/* BUTTONS  */
/************/
/* Buttons */
DEFINE_GPIO(BTN_PLAY,           GPIO_PA(16),    GPIOF_INPUT)
DEFINE_GPIO(BTN_VOL_UP,         GPIO_PA(17),    GPIOF_INPUT)
DEFINE_GPIO(BTN_VOL_DOWN,       GPIO_PA(19),    GPIOF_INPUT)
DEFINE_GPIO(BTN_MENU,           GPIO_PB(28),    GPIOF_INPUT)
DEFINE_GPIO(BTN_PREV,           GPIO_PD(4),     GPIOF_INPUT)
DEFINE_GPIO(BTN_NEXT,           GPIO_PC(24),    GPIOF_INPUT)
DEFINE_GPIO(BTN_SCROLL_A,       GPIO_PB(24),    GPIOF_INPUT)
DEFINE_GPIO(BTN_SCROLL_B,       GPIO_PB(23),    GPIOF_INPUT)

/**********************/
/* BLUETOOTH STUFF    */
/**********************/
/* BT Power */
// GPIO 86 --> Port C, Pin 22
DEFINE_GPIO(BT_PWR,             GPIO_PC(22),    GPIOF_OUTPUT(0))




/****************************** */
/* HW1/2/3-ONLY STUFF           */
/****************************** */
DEFINE_GPIO(HW1_START,              GPIO_PC(0),    GPIOF_OUTPUT(0))
/************/
/* BUTTONS  */
/************/
DEFINE_GPIO(BTN_BACK_HW1,           GPIO_PD(5),     GPIOF_INPUT)
DEFINE_GPIO(BTN_POWER_HW1,          GPIO_PB(7),     GPIOF_INPUT)

/**********************/
/* BLUETOOTH STUFF    */
/**********************/
// GPIO 83 --> port C, pin 19
DEFINE_GPIO(BT_REG_ON_HW1,          GPIO_PC(19),    GPIOF_OUTPUT(0))

/************/
/* LCD      */
/************/
/* LCD */
DEFINE_GPIO(LCD_PWR_HW1,            GPIO_PB(14),    GPIOF_OUTPUT(0))
DEFINE_GPIO(HW1_END,                GPIO_PC(0),    GPIOF_OUTPUT(0))




/****************************** */
/* HW4-ONLY STUFF               */
/****************************** */
DEFINE_GPIO(HW4_START,              GPIO_PC(0),    GPIOF_OUTPUT(0))
DEFINE_GPIO(BLUELIGHT_HW4,          GPIO_PB(7),    GPIOF_OUTPUT(1))
/************/
/* BUTTONS  */
/************/
DEFINE_GPIO(BTN_BACK_HW4,           GPIO_PA(18),     GPIOF_INPUT)
DEFINE_GPIO(BTN_POWER_HW4,          GPIO_PB(31),     GPIOF_INPUT)

/**********************/
/* BLUETOOTH STUFF    */
/**********************/
/* bt_wake_host */
// gpio 84 --> port C, pin 20
DEFINE_GPIO(BT_WAKE_HOST,       GPIO_PC(20),    GPIOF_INPUT)

/* host_wake_bt */
// gpio 83 --> port C, pin 19
DEFINE_GPIO(HOST_WAKE_BT,       GPIO_PC(19),    GPIOF_OUTPUT(0))

/**************/
/* USB        */
/**************/
/* USB_TCS1421_CFG0 */
// set TCS1421_CFG0:1 to 00, Upstream Facing Port (tried others, they don't work)
// gpio 101 --> port D, pin 5
DEFINE_GPIO(TCS1421_CFG0,       GPIO_PD(5),     GPIOF_OUTPUT(0))

/***************/
/* PLUG DETECT */
/***************/
/* HP_DETECT */
// gpio 46 --> port B, pin 14
DEFINE_GPIO(HP_DETECT_HW4,          GPIO_PB(14), GPIOF_INPUT)

/* LO_DETECT */
// gpio 54 --> Port B, pin 22
DEFINE_GPIO(LO_DETECT_HW4,          GPIO_PB(22), GPIOF_INPUT)
DEFINE_GPIO(HW4_END,                GPIO_PC(0),  GPIOF_OUTPUT(0))