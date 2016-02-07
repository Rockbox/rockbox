#include <stdint.h>

#define NR_BANKS        4
#define NR_FUNCTIONS    4
#define NR_PINS         32

struct pin_function_desc_t
{
    const char *name;
    unsigned group;
    unsigned block;
};

struct pin_desc_t
{
    struct pin_function_desc_t function[NR_FUNCTIONS];
};

struct bank_map_t
{
    struct pin_desc_t *pins;
};

struct soc_t
{
    const char *soc;
    const char *ver;
    struct bank_map_t *map;
};

#define BGA169  bga169
#define LQFP100 lqfp100

#define IMX233  imx233
#define STMP3700 stmp3700
#define STMP3600 stmp3600

#define BANK(soc, ver, bank) soc##_##ver##_bank##bank

#define PIN_GROUP_EMI       0
#define PIN_GROUP_GPIO      1
#define PIN_GROUP_I2C       2
#define PIN_GROUP_JTAG      3
#define PIN_GROUP_PWM       4
#define PIN_GROUP_SPDIF     5
#define PIN_GROUP_TIMROT    6
#define PIN_GROUP_AUART     7
#define PIN_GROUP_ETM       8
#define PIN_GROUP_GPMI      9
#define PIN_GROUP_IrDA      10
#define PIN_GROUP_LCD       11
#define PIN_GROUP_SAIF      12
#define PIN_GROUP_SSP       13
#define PIN_GROUP_DUART     14
#define PIN_GROUP_USB       15
#define PIN_GROUP_NONE      16

#define PIN_NO_BLOCK    (unsigned)(-1)

#define _STR(x) #x
#define STR(x)  _STR(x)

#define PIN_GROUP_PREFIX_EMI    "emi_"
#define PIN_GROUP_PREFIX_GPIO   ""
#define PIN_GROUP_PREFIX_I2C    "i2c_"
#define PIN_GROUP_PREFIX_JTAG   "jtag_"
#define PIN_GROUP_PREFIX_PWM    "pwm"
#define PIN_GROUP_PREFIX_SPDIF  "spdif"
#define PIN_GROUP_PREFIX_TIMROT "timrot"
#define PIN_GROUP_PREFIX_AUART  "auart"
#define PIN_GROUP_PREFIX_ETM    "etm_"
#define PIN_GROUP_PREFIX_GPMI   "gpmi_"
#define PIN_GROUP_PREFIX_IrDA   "ir_"
#define PIN_GROUP_PREFIX_LCD    "lcd_"
#define PIN_GROUP_PREFIX_SAIF   "saif"
#define PIN_GROUP_PREFIX_SSP    "ssp"
#define PIN_GROUP_PREFIX_DUART  "duart_"
#define PIN_GROUP_PREFIX_USB    "usb_"
#define PIN_GROUP_PREFIX_NONE   ""

#define R(group,name,block) {PIN_GROUP_PREFIX_##group name, PIN_GROUP_##group, block}
#define Q(group,block,name) R(group,STR(block) "_" name, block)
#define P(group,name) R(group,name,PIN_NO_BLOCK)

#define IO      P(GPIO,"gpio")
#define RES     {NULL,PIN_GROUP_NONE,PIN_NO_BLOCK}
#define DIS     P(GPIO,"disabled")

struct pin_desc_t BANK(IMX233,BGA169,0)[NR_PINS] =
{
    {{P(GPMI,"d0"), P(LCD,"d8"), Q(SSP,2,"d0"), IO}}, /* B0P00 */
    {{P(GPMI,"d1"), P(LCD,"d9"), Q(SSP,2,"d1"), IO}}, /* B0P01 */
    {{P(GPMI,"d2"), P(LCD,"d10"), Q(SSP,2,"d2"), IO}}, /* B0P02 */
    {{P(GPMI,"d3"), P(LCD,"d11"), Q(SSP,2,"d3"), IO}}, /* B0P03 */
    {{P(GPMI,"d4"), P(LCD,"d12"), Q(SSP,2,"d4"), IO}}, /* B0P04 */
    {{P(GPMI,"d5"), P(LCD,"d13"), Q(SSP,2,"d5"), IO}}, /* B0P05 */
    {{P(GPMI,"d6"), P(LCD,"d14"), Q(SSP,2,"d6"), IO}}, /* B0P06 */
    {{P(GPMI,"d7"), P(LCD,"d15"), Q(SSP,2,"d7"), IO}}, /* B0P07 */
    {{P(GPMI,"d8"), P(LCD,"d18"), Q(SSP,1,"d4"), IO}}, /* B0P08 */
    {{P(GPMI,"d9"), P(LCD,"d19"), Q(SSP,1,"d5"), IO}}, /* B0P09 */
    {{P(GPMI,"d10"), P(LCD,"d20"), Q(SSP,1,"d6"), IO}}, /* B0P10 */
    {{P(GPMI,"d11"), P(LCD,"d21"), Q(SSP,1,"d7"), IO}}, /* B0P11 */
    {{P(GPMI,"d12"), P(LCD,"d22"), RES, IO}}, /* B0P12 */
    {{P(GPMI,"d13"), P(LCD,"d23"), RES, IO}}, /* B0P13 */
    {{P(GPMI,"d14"), Q(AUART,2,"rx"), RES, IO}}, /* B0P14 */
    {{P(GPMI,"d15"), Q(AUART,2,"tx"), P(GPMI,"ce3n"), IO}}, /* B0P15 */
    {{P(GPMI,"cle"), P(LCD,"d16"), RES, IO}}, /* B0P16 */
    {{P(GPMI,"ale"), P(LCD,"d17"), RES, IO}}, /* B0P17 */
    {{P(GPMI,"ce2n"), P(GPMI,"a2"), RES, IO}}, /* B0P18 */
    {{P(GPMI,"rb0"), RES, Q(SSP,2,"det"), IO}}, /* B0P19 */
    {{P(GPMI,"rb1"), RES, Q(SSP,2,"cmd"), IO}}, /* B0P20 */
    {{P(GPMI,"rb2"), RES, RES, IO}}, /* B0P21 */
    {{P(GPMI,"rb3"), RES, RES, IO}}, /* B0P22 */
    {{P(GPMI,"wpn"), RES, RES, IO}}, /* B0P23 */
    {{P(GPMI,"wrn"), RES, Q(SSP,2,"sck"), IO}}, /* B0P24 */
    {{P(GPMI,"rdn"), RES, RES, IO}}, /* B0P25 */
    {{Q(AUART,1,"cts"), RES, Q(SSP,1,"d4"), IO}}, /* B0P26 */
    {{Q(AUART,1,"rts"), P(IrDA,"clk"), Q(SSP,1,"d5"), IO}}, /* B0P27 */
    {{Q(AUART,1,"rx"), P(IrDA,"in_data"), Q(SSP,1,"d6"), IO}}, /* B0P28 */
    {{Q(AUART,1,"tx"), P(IrDA,"out_data"), Q(SSP,1,"d7"), IO}}, /* B0P29 */
    {{P(I2C,"clk"), P(GPMI,"rb2"), Q(AUART,1,"tx"), IO}}, /* B0P30 */
    {{P(I2C,"sd"), P(GPMI,"ce2n"), Q(AUART,1,"rx"), IO}}, /* B0P31 */
};

struct pin_desc_t BANK(IMX233,BGA169,1)[NR_PINS] =
{
    {{P(LCD,"d0"), P(ETM,"da8"), RES, IO}}, /* B1P00 */
    {{P(LCD,"d1"), P(ETM,"da9"), RES, IO}}, /* B1P01 */
    {{P(LCD,"d2"), P(ETM,"da10"), RES, IO}}, /* B1P02 */
    {{P(LCD,"d3"), P(ETM,"da11"), RES, IO}}, /* B1P03 */
    {{P(LCD,"d4"), P(ETM,"da12"), RES, IO}}, /* B1P04 */
    {{P(LCD,"d5"), P(ETM,"da13"), RES, IO}}, /* B1P05 */
    {{P(LCD,"d6"), P(ETM,"da14"), RES, IO}}, /* B1P06 */
    {{P(LCD,"d7"), P(ETM,"da15"), RES, IO}}, /* B1P07 */
    {{P(LCD,"d8"), P(ETM,"da0"), Q(SAIF,2,"d0"), IO}}, /* B1P08 */
    {{P(LCD,"d9"), P(ETM,"da1"), Q(SAIF,1,"d0"), IO}}, /* B1P09 */
    {{P(LCD,"d10"), P(ETM,"da2"), P(SAIF,"bitclk"), IO}}, /* B1P10 */
    {{P(LCD,"d11"), P(ETM,"da3"), P(SAIF,"lrclk"), IO}}, /* B1P11 */
    {{P(LCD,"d12"), P(ETM,"da4"), Q(SAIF,2,"d1"), IO}}, /* B1P12 */
    {{P(LCD,"d13"), P(ETM,"da5"), Q(SAIF,2,"d2"), IO}}, /* B1P13 */
    {{P(LCD,"d14"), P(ETM,"da6"), Q(SAIF,1,"d2"), IO}}, /* B1P14 */
    {{P(LCD,"d15"), P(ETM,"da7"), Q(SAIF,1,"d1"), IO}}, /* B1P15 */
    {{P(LCD,"d16"), RES, Q(SAIF,1,"alt_bitclk"), IO}}, /* B1P16 */
    {{P(LCD,"d17"), RES, RES, IO}}, /* B1P17 */
    {{P(LCD,"reset"), P(ETM,"tctl"), P(GPMI,"ce3n"), IO}}, /* B1P18 */
    {{P(LCD,"rs"), P(ETM,"tclk"), RES, IO}}, /* B1P19 */
    {{P(LCD,"wr"), RES, RES, IO}}, /* B1P20 */
    {{P(LCD,"cs"), RES, RES, IO}}, /* B1P21 */
    {{P(LCD,"dotclk"), P(GPMI,"rb3"), RES, IO}}, /* B1P22 */
    {{P(LCD,"enable"), P(I2C,"clk"), RES, IO}}, /* B1P23 */
    {{P(LCD,"hsync"), P(I2C,"sd"), RES, IO}}, /* B1P24 */
    {{P(LCD,"vsync"), P(LCD,"busy"), RES, IO}}, /* B1P25 */
    {{P(PWM,"0"), P(TIMROT,"1"), P(DUART,"rx"), IO}}, /* B1P26 */
    {{P(PWM,"1"), P(TIMROT,"2"), P(DUART,"tx"), IO}}, /* B1P27 */
    {{P(PWM,"2"), P(GPMI,"rb3"), RES, IO}}, /* B1P28 */
    {{P(PWM,"3"), P(ETM,"tctl"), Q(AUART,1,"cts"), IO}}, /* B1P29 */
    {{P(PWM,"4"), P(ETM,"tclk"), Q(AUART,1,"rts"), IO}}, /* B1P30 */
    {{RES, RES, RES, RES}}, /* B1P31 */
};

struct pin_desc_t BANK(IMX233,BGA169,2)[NR_PINS] =
{
    {{Q(SSP,1,"cmd"), RES, P(JTAG,"tdo"), IO}}, /* B2P00 */
    {{Q(SSP,1,"det"), P(GPMI,"ce3n"), P(USB,"otg_id"), IO}}, /* B2P01 */
    {{Q(SSP,1,"d0"), RES, P(JTAG,"tdi"), IO}}, /* B2P02 */
    {{Q(SSP,1,"d1"), P(I2C,"clk"), P(JTAG,"tck"), IO}}, /* B2P03 */
    {{Q(SSP,1,"d2"), P(I2C,"sd"), P(JTAG,"rtck"), IO}}, /* B2P04 */
    {{Q(SSP,1,"d3"), RES, P(JTAG,"tms"), IO}}, /* B2P05 */
    {{Q(SSP,1,"sck"), RES, P(JTAG,"trst_n"), IO}}, /* B2P06 */
    {{P(TIMROT,"1"), Q(AUART,2,"rts"), P(SPDIF,""), IO}}, /* B2P07 */
    {{P(TIMROT,"2"), Q(AUART,2,"cts"), P(GPMI,"ce3n"), IO}}, /* B2P08 */
    {{P(EMI,"a00"), RES, RES, IO}}, /* B2P09 */
    {{P(EMI,"a01"), RES, RES, IO}}, /* B2P10 */
    {{P(EMI,"a02"), RES, RES, IO}}, /* B2P11 */
    {{P(EMI,"a03"), RES, RES, IO}}, /* B2P12 */
    {{P(EMI,"a04"), RES, RES, IO}}, /* B2P13 */
    {{P(EMI,"a05"), RES, RES, IO}}, /* B2P14 */
    {{P(EMI,"a06"), RES, RES, IO}}, /* B2P15 */
    {{P(EMI,"a07"), RES, RES, IO}}, /* B2P16 */
    {{P(EMI,"a08"), RES, RES, IO}}, /* B2P17 */
    {{P(EMI,"a09"), RES, RES, IO}}, /* B2P18 */
    {{P(EMI,"a10"), RES, RES, IO}}, /* B2P19 */
    {{P(EMI,"a11"), RES, RES, IO}}, /* B2P20 */
    {{P(EMI,"a12"), RES, RES, IO}}, /* B2P21 */
    {{P(EMI,"ba0"), RES, RES, IO}}, /* B2P22 */
    {{P(EMI,"ba1"), RES, RES, IO}}, /* B2P23 */
    {{P(EMI,"casn"), RES, RES, IO}}, /* B2P24 */
    {{P(EMI,"ce0n"), RES, RES, IO}}, /* B2P25 */
    {{P(EMI,"ce1n"), RES, RES, IO}}, /* B2P26 */
    {{P(GPMI,"ce1n"), RES, RES, IO}}, /* B2P27 */
    {{P(GPMI,"ce0n"), RES, RES, IO}}, /* B2P28 */
    {{P(EMI,"cke"), RES, RES, IO}}, /* B2P29 */
    {{P(EMI,"rasn"), RES, RES, IO}}, /* B2P30 */
    {{P(EMI,"wen"), RES, RES, IO}}, /* B2P31 */
};

struct pin_desc_t BANK(IMX233,BGA169,3)[NR_PINS] =
{
    {{P(EMI,"d0"), RES, RES, DIS}}, /* B3P00 */
    {{P(EMI,"d1"), RES, RES, DIS}}, /* B3P01 */
    {{P(EMI,"d2"), RES, RES, DIS}}, /* B3P02 */
    {{P(EMI,"d3"), RES, RES, DIS}}, /* B3P03 */
    {{P(EMI,"d4"), RES, RES, DIS}}, /* B3P04 */
    {{P(EMI,"d5"), RES, RES, DIS}}, /* B3P05 */
    {{P(EMI,"d6"), RES, RES, DIS}}, /* B3P06 */
    {{P(EMI,"d7"), RES, RES, DIS}}, /* B3P07 */
    {{P(EMI,"d8"), RES, RES, DIS}}, /* B3P08 */
    {{P(EMI,"d9"), RES, RES, DIS}}, /* B3P09 */
    {{P(EMI,"d10"), RES, RES, DIS}}, /* B3P10 */
    {{P(EMI,"d11"), RES, RES, DIS}}, /* B3P11 */
    {{P(EMI,"d12"), RES, RES, DIS}}, /* B3P12 */
    {{P(EMI,"d13"), RES, RES, DIS}}, /* B3P13 */
    {{P(EMI,"d14"), RES, RES, DIS}}, /* B3P14 */
    {{P(EMI,"d15"), RES, RES, DIS}}, /* B3P15 */
    {{P(EMI,"dqm0"), RES, RES, DIS}}, /* B3P16 */
    {{P(EMI,"dqm1"), RES, RES, DIS}}, /* B3P17 */
    {{P(EMI,"dqs0"), RES, RES, DIS}}, /* B3P18 */
    {{P(EMI,"dqs1"), RES, RES, DIS}}, /* B3P19 */
    {{P(EMI,"clk"), RES, RES, DIS}}, /* B3P20 */
    {{P(EMI,"clkn"), RES, RES, DIS}}, /* B3P21 */
    {{RES, RES, RES, RES}}, /* B3P22 */
    {{RES, RES, RES, RES}}, /* B3P23 */
    {{RES, RES, RES, RES}}, /* B3P24 */
    {{RES, RES, RES, RES}}, /* B3P25 */
    {{RES, RES, RES, RES}}, /* B3P26 */
    {{RES, RES, RES, RES}}, /* B3P27 */
    {{RES, RES, RES, RES}}, /* B3P28 */
    {{RES, RES, RES, RES}}, /* B3P29 */
    {{RES, RES, RES, RES}}, /* B3P30 */
    {{RES, RES, RES, RES}}, /* B3P31 */
};

struct pin_desc_t BANK(STMP3700,BGA169,0)[NR_PINS] =
{
    {{P(GPMI,"d0"), RES, RES, IO}}, /* B0P00 */
    {{P(GPMI,"d1"), RES, Q(SSP,2,"d1"), IO}}, /* B0P01 */
    {{P(GPMI,"d2"), RES, Q(SSP,2,"d2"), IO}}, /* B0P02 */
    {{P(GPMI,"d3"), RES, Q(SSP,2,"d3"), IO}}, /* B0P03 */
    {{P(GPMI,"d4"), RES, RES, IO}}, /* B0P04 */
    {{P(GPMI,"d5"), RES, RES, IO}}, /* B0P05 */
    {{P(GPMI,"d6"), RES, RES, IO}}, /* B0P06 */
    {{P(GPMI,"d7"), RES, RES, IO}}, /* B0P07 */
    {{P(GPMI,"d8"), P(EMI,"a15"), RES, IO}}, /* B0P08 */
    {{P(GPMI,"d9"), P(EMI,"a16"), RES, IO}}, /* B0P09 */
    {{P(GPMI,"d10"), P(EMI,"a17"), RES, IO}}, /* B0P10 */
    {{P(GPMI,"d11"), P(EMI,"a18"), P(GPMI,"ce0n"), IO}}, /* B0P11 */
    {{P(GPMI,"d12"), P(EMI,"a19"), P(GPMI,"ce1n"), IO}}, /* B0P12 */
    {{P(GPMI,"d13"), P(EMI,"a20"), P(GPMI,"ce2n"), IO}}, /* B0P13 */
    {{P(GPMI,"d14"), P(EMI,"a21"), P(GPMI,"ce3n"), IO}}, /* B0P14 */
    {{P(GPMI,"d15"), P(EMI,"a22"), RES, IO}}, /* B0P15 */
    {{P(GPMI,"a0"), P(EMI,"a23"), RES, IO}}, /* B0P16 */
    {{P(GPMI,"a1"), P(EMI,"a24"), RES, IO}}, /* B0P17 */
    {{P(GPMI,"a2"), P(EMI,"a25"), P(IrDA,"dout"), IO}}, /* B0P18 */
    {{P(GPMI,"rb0"), RES, Q(SSP,2,"det"), IO}}, /* B0P19 */
    {{P(GPMI,"rb2"), Q(AUART,2,"rx"), Q(SSP,2,"cmd"), IO}}, /* B0P20 */
    {{P(GPMI,"rb3"), P(EMI,"oen"), P(IrDA,"din"), IO}}, /* B0P21 */
    {{P(GPMI,"resetn"), P(EMI,"rstn"), P(JTAG,"trst_n"), IO}}, /* B0P22 */
    {{P(GPMI,"irq"), Q(AUART,2,"tx"), Q(SSP,2,"sck"), IO}}, /* B0P23 */
    {{P(GPMI,"wrn"), RES, RES, IO}}, /* B0P24 */
    {{P(GPMI,"rdn"), RES, RES, IO}}, /* B0P25 */
    {{Q(AUART,2,"cts"), RES, Q(SSP,1,"d4"), IO}}, /* B0P26 */
    {{Q(AUART,2,"rts"), P(IrDA,"clk"), Q(SSP,1,"d5"), IO}}, /* B0P27 */
    {{Q(AUART,2,"rx"), P(IrDA,"din"), Q(SSP,1,"d6"), IO}}, /* B0P28 */
    {{Q(AUART,2,"tx"), P(IrDA,"dout"), Q(SSP,1,"d7"), IO}}, /* B0P29 */
    {{RES, RES, RES, RES}}, /* B0P30 */
    {{RES, RES, RES, RES}}, /* B0P31 */
};

struct pin_desc_t BANK(STMP3700,BGA169,1)[NR_PINS] =
{
    {{P(LCD,"d0"), P(ETM,"da0"), RES, IO}}, /* B1P00 */
    {{P(LCD,"d1"), P(ETM,"da1"), RES, IO}}, /* B1P01 */
    {{P(LCD,"d2"), P(ETM,"da2"), RES, IO}}, /* B1P02 */
    {{P(LCD,"d3"), P(ETM,"da3"), RES, IO}}, /* B1P03 */
    {{P(LCD,"d4"), P(ETM,"da4"), RES, IO}}, /* B1P04 */
    {{P(LCD,"d5"), P(ETM,"da5"), RES, IO}}, /* B1P05 */
    {{P(LCD,"d6"), P(ETM,"da6"), RES, IO}}, /* B1P06 */
    {{P(LCD,"d7"), P(ETM,"da7"), RES, IO}}, /* B1P07 */
    {{P(LCD,"d8"), P(ETM,"da0"), Q(SAIF,2,"d0"), IO}}, /* B1P08 */
    {{P(LCD,"d9"), P(ETM,"da1"), Q(SAIF,1,"d0"), IO}}, /* B1P09 */
    {{P(LCD,"d10"), P(ETM,"da2"), P(SAIF,"mlclk_bitclk"), IO}}, /* B1P10 */
    {{P(LCD,"d11"), P(ETM,"da3"), P(SAIF,"lrclk"), IO}}, /* B1P11 */
    {{P(LCD,"d12"), P(ETM,"da4"), Q(SAIF,2,"d1"), IO}}, /* B1P12 */
    {{P(LCD,"d13"), P(ETM,"da5"), Q(SAIF,2,"d2"), IO}}, /* B1P13 */
    {{P(LCD,"d14"), P(ETM,"da6"), Q(SAIF,1,"d2"), IO}}, /* B1P14 */
    {{P(LCD,"d15"), P(ETM,"da7"), P(LCD,"vsync"), IO}}, /* B1P15 */
    {{P(LCD,"reset"), RES, RES, IO}}, /* B1P16 */
    {{P(LCD,"rs"), RES, RES, IO}}, /* B1P17 */
    {{P(LCD,"wr"), RES, RES, IO}}, /* B1P18 */
    {{P(LCD,"rd"), RES, RES, IO}}, /* B1P19 */
    {{P(LCD,"enable"), RES, RES, IO}}, /* B1P20 */
    {{P(LCD,"busy"), P(LCD,"vsync"), Q(SAIF,1,"d1"), IO}}, /* B1P21 */
    {{Q(SSP,1,"cmd"), RES, RES, IO}}, /* B1P22 */
    {{Q(SSP,1,"sck"), RES, RES, IO}}, /* B1P23 */
    {{Q(SSP,1,"d0"), RES, RES, IO}}, /* B1P24 */
    {{Q(SSP,1,"d1"), P(GPMI,"ce2n"), P(JTAG,"tclk"), IO}}, /* B1P25 */
    {{Q(SSP,1,"d2"), Q(AUART,1,"cts"), P(JTAG,"rtclk"), IO}}, /* B1P26 */
    {{Q(SSP,1,"d3"), Q(AUART,1,"rts"), P(JTAG,"tms"), IO}}, /* B1P27 */
    {{Q(SSP,1,"det"), P(ETM,"psa2"), P(USB,"otg_id"), IO}}, /* B1P28 */
    {{RES, RES, RES, RES}}, /* B1P29 */
    {{RES, RES, RES, RES}}, /* B1P30 */
    {{RES, RES, RES, RES}}, /* B1P31 */
};

struct pin_desc_t BANK(STMP3700,BGA169,2)[NR_PINS] =
{
    {{P(PWM,"0"), P(ETM,"tsynca"), P(DUART,"rx"), IO}}, /* B2P00 */
    {{P(PWM,"1"), P(ETM,"psa1"), P(DUART,"tx"), IO}}, /* B2P01 */
    {{P(PWM,"2"), P(SPDIF,""), P(SAIF,"alt_bitclk"), IO}}, /* B2P02 */
    {{P(PWM,"3"), P(ETM,"psa0"), Q(AUART,2,"cts"), IO}}, /* B2P03 */
    {{P(PWM,"4"), P(ETM,"tclk"), Q(AUART,2,"rts"), IO}}, /* B2P04 */
    {{P(I2C,"scl"), P(GPMI,"rb3"), RES, IO}}, /* B2P05 */
    {{P(I2C,"sd"), P(GPMI,"ce3n"), RES, IO}}, /* B2P06 */
    {{P(TIMROT,"1"), Q(AUART,1,"tx"), P(JTAG,"tdo"), IO}}, /* B2P07 */
    {{P(TIMROT,"2"), Q(AUART,1,"rx"), P(JTAG,"tdi"), IO}}, /* B2P08 */
    {{P(EMI,"cke"), RES, RES, IO}}, /* B2P09 */
    {{P(EMI,"rasn"), RES, RES, IO}}, /* B2P10 */
    {{P(EMI,"casn"), RES, RES, IO}}, /* B2P11 */
    {{P(EMI,"ce0n"), P(GPMI,"ce2n"), RES, IO}}, /* B2P12 */
    {{P(EMI,"ce1n"), P(GPMI,"ce3n"), P(IrDA,"clk"), IO}}, /* B2P13 */
    {{P(EMI,"ce2n"), P(GPMI,"ce1n"), Q(SSP,2,"d0"), IO}}, /* B2P14 */
    {{P(EMI,"ce3n"), P(GPMI,"ce0n"), RES, IO}}, /* B2P15 */
    {{P(EMI,"a00"), RES, RES, IO}}, /* B2P16 */
    {{P(EMI,"a01"), RES, RES, IO}}, /* B2P17 */
    {{P(EMI,"a02"), RES, RES, IO}}, /* B2P18 */
    {{P(EMI,"a03"), RES, RES, IO}}, /* B2P19 */
    {{P(EMI,"a04"), RES, RES, IO}}, /* B2P20 */
    {{P(EMI,"a05"), RES, RES, IO}}, /* B2P21 */
    {{P(EMI,"a06"), RES, RES, IO}}, /* B2P22 */
    {{P(EMI,"a07"), RES, RES, IO}}, /* B2P23 */
    {{P(EMI,"a08"), RES, RES, IO}}, /* B2P24 */
    {{P(EMI,"a08"), RES, RES, IO}}, /* B2P25 */
    {{P(EMI,"a10"), RES, RES, IO}}, /* B2P26 */
    {{P(EMI,"a11"), RES, RES, IO}}, /* B2P27 */
    {{P(EMI,"a12"), RES, RES, IO}}, /* B2P28 */
    {{P(EMI,"a13"), RES, RES, IO}}, /* B2P29 */
    {{P(EMI,"a14"), RES, RES, IO}}, /* B2P30 */
    {{P(EMI,"wen"), RES, RES, IO}}, /* B2P31 */
};

struct pin_desc_t BANK(STMP3700,BGA169,3)[NR_PINS] =
{
    {{P(EMI,"d0"), RES, RES, DIS}}, /* B3P00 */
    {{P(EMI,"d1"), RES, RES, DIS}}, /* B3P01 */
    {{P(EMI,"d2"), RES, RES, DIS}}, /* B3P02 */
    {{P(EMI,"d3"), RES, RES, DIS}}, /* B3P03 */
    {{P(EMI,"d4"), RES, RES, DIS}}, /* B3P04 */
    {{P(EMI,"d5"), RES, RES, DIS}}, /* B3P05 */
    {{P(EMI,"d6"), RES, RES, DIS}}, /* B3P06 */
    {{P(EMI,"d7"), RES, RES, DIS}}, /* B3P07 */
    {{P(EMI,"d8"), RES, RES, DIS}}, /* B3P08 */
    {{P(EMI,"d9"), RES, RES, DIS}}, /* B3P09 */
    {{P(EMI,"d10"), RES, RES, DIS}}, /* B3P10 */
    {{P(EMI,"d11"), RES, RES, DIS}}, /* B3P11 */
    {{P(EMI,"d12"), RES, RES, DIS}}, /* B3P12 */
    {{P(EMI,"d13"), RES, RES, DIS}}, /* B3P13 */
    {{P(EMI,"d14"), RES, RES, DIS}}, /* B3P14 */
    {{P(EMI,"d15"), RES, RES, DIS}}, /* B3P15 */
    {{P(EMI,"dqm0"), RES, RES, DIS}}, /* B3P16 */
    {{P(EMI,"dqm1"), RES, RES, DIS}}, /* B3P17 */
    {{P(EMI,"dqs0"), RES, RES, DIS}}, /* B3P18 */
    {{P(EMI,"dqs1"), RES, RES, DIS}}, /* B3P19 */
    {{P(EMI,"clk"), RES, RES, DIS}}, /* B3P20 */
    {{P(EMI,"clkn"), RES, RES, DIS}}, /* B3P21 */
    {{RES, RES, RES, RES}}, /* B3P22 */
    {{RES, RES, RES, RES}}, /* B3P23 */
    {{RES, RES, RES, RES}}, /* B3P24 */
    {{RES, RES, RES, RES}}, /* B3P25 */
    {{RES, RES, RES, RES}}, /* B3P26 */
    {{RES, RES, RES, RES}}, /* B3P27 */
    {{RES, RES, RES, RES}}, /* B3P28 */
    {{RES, RES, RES, RES}}, /* B3P29 */
    {{RES, RES, RES, RES}}, /* B3P30 */
    {{RES, RES, RES, RES}}, /* B3P31 */
};

/* B0P{26-29}: might be ssp1_d{4-7} */
struct pin_desc_t BANK(STMP3700,LQFP100,0)[NR_PINS] =
{
    {{RES, RES, RES, RES}}, /* B0P00 */
    {{RES, RES, Q(SSP,2,"d1"), IO}}, /* B0P01 */
    {{RES, RES, Q(SSP,2,"d2"), IO}}, /* B0P02 */
    {{RES, RES, Q(SSP,2,"d3"), IO}}, /* B0P03 */
    {{RES, RES, Q(SSP,2,"d4"), IO}}, /* B0P04 */
    {{RES, RES, Q(SSP,2,"d5"), IO}}, /* B0P05 */
    {{RES, RES, Q(SSP,2,"d6"), IO}}, /* B0P06 */
    {{RES, RES, Q(SSP,2,"d7"), IO}}, /* B0P07 */
    {{RES, RES, RES, RES}}, /* B0P08 */
    {{RES, RES, RES, RES}}, /* B0P09 */
    {{RES, RES, RES, RES}}, /* B0P10 */
    {{RES, RES, RES, RES}}, /* B0P11 */
    {{RES, RES, RES, RES}}, /* B0P12 */
    {{RES, RES, RES, RES}}, /* B0P13 */
    {{RES, RES, RES, RES}}, /* B0P14 */
    {{RES, RES, RES, RES}}, /* B0P15 */
    {{RES, RES, RES, RES}}, /* B0P16 */
    {{RES, RES, RES, RES}}, /* B0P17 */
    {{RES, RES, RES, RES}}, /* B0P18 */
    {{RES, RES, RES, RES}}, /* B0P19 */
    {{RES, RES, Q(SSP,2,"cmd"), IO}}, /* B0P20 */
    {{RES, RES, RES, RES}}, /* B0P21 */
    {{RES, RES, RES, RES}}, /* B0P22 */
    {{RES, RES, RES, RES}}, /* B0P23 */
    {{RES, RES, Q(SSP,2,"sck"), IO}}, /* B0P24 */
    {{RES, RES, RES, RES}}, /* B0P25 */
    {{RES, RES, RES, RES}}, /* B0P26 */
    {{RES, RES, RES, RES}}, /* B0P27 */
    {{RES, RES, RES, RES}}, /* B0P28 */
    {{RES, RES, RES, RES}}, /* B0P29 */
    {{RES, RES, RES, RES}}, /* B0P30 */
    {{RES, RES, RES, RES}}, /* B0P31 */
};

/* unsure about precise lcd functions */
struct pin_desc_t BANK(STMP3700,LQFP100,1)[NR_PINS] =
{
    {{P(LCD,"d0"), RES, RES, IO}}, /* B1P00 */
    {{P(LCD,"d1"), RES, RES, IO}}, /* B1P01 */
    {{P(LCD,"d2"), RES, RES, IO}}, /* B1P02 */
    {{P(LCD,"d3"), RES, RES, IO}}, /* B1P03 */
    {{P(LCD,"d4"), RES, RES, IO}}, /* B1P04 */
    {{P(LCD,"d5"), RES, RES, IO}}, /* B1P05 */
    {{P(LCD,"d6"), RES, RES, IO}}, /* B1P06 */
    {{P(LCD,"d7"), RES, RES, IO}}, /* B1P07 */
    {{RES, RES, RES, RES}}, /* B1P08 */
    {{RES, RES, RES, RES}}, /* B1P09 */
    {{RES, RES, RES, RES}}, /* B1P10 */
    {{RES, RES, RES, RES}}, /* B1P11 */
    {{RES, RES, RES, RES}}, /* B1P12 */
    {{RES, RES, RES, RES}}, /* B1P13 */
    {{RES, RES, RES, RES}}, /* B1P14 */
    {{RES, RES, RES, RES}}, /* B1P15 */
    {{P(LCD,"reset"), RES, RES, IO}}, /* B1P16 */
    {{P(LCD,"rs"), RES, RES, IO}}, /* B1P17 */
    {{P(LCD,"wr"), RES, RES, IO}}, /* B1P18 */
    {{P(LCD,"rs"), RES, RES, IO}}, /* B1P19 */
    {{P(LCD,"enable"), RES, RES, IO}}, /* B1P20 */
    {{RES, P(LCD,"busy"), RES, RES}}, /* B1P21 */
    {{Q(SSP,1,"cmd"), RES, RES, IO}}, /* B1P22 */
    {{Q(SSP,1,"sck"), RES, RES, IO}}, /* B1P23 */
    {{Q(SSP,1,"d0"), RES, RES, IO}}, /* B1P24 */
    {{Q(SSP,1,"d1"), RES, RES, IO}}, /* B1P25 */
    {{Q(SSP,1,"d2"), RES, RES, IO}}, /* B1P26 */
    {{Q(SSP,1,"d3"), RES, RES, IO}}, /* B1P27 */
    {{Q(SSP,1,"det"), RES, RES, IO}}, /* B1P28 */
    {{RES, RES, RES, RES}}, /* B1P29 */
    {{RES, RES, RES, RES}}, /* B1P30 */
    {{RES, RES, RES, RES}}, /* B1P31 */
};

struct pin_desc_t BANK(STMP3700,LQFP100,2)[NR_PINS] =
{
    {{RES, RES, RES, IO}}, /* B2P00 */
    {{RES, RES, RES, IO}}, /* B2P01 */
    {{RES, RES, RES, IO}}, /* B2P02 */
    {{RES, RES, RES, IO}}, /* B2P03 */
    {{RES, RES, RES, IO}}, /* B2P04 */
    {{RES, RES, RES, IO}}, /* B2P05 */
    {{RES, RES, RES, IO}}, /* B2P06 */
    {{RES, RES, RES, IO}}, /* B2P07 */
    {{RES, RES, RES, RES}}, /* B2P08 */
    {{RES, RES, RES, RES}}, /* B2P09 */
    {{RES, RES, RES, RES}}, /* B2P10 */
    {{RES, RES, RES, RES}}, /* B2P11 */
    {{RES, RES, RES, RES}}, /* B2P12 */
    {{RES, RES, RES, RES}}, /* B2P13 */
    {{RES, RES, Q(SSP,2,"d0"), IO}}, /* B2P14 */
    {{RES, RES, RES, RES}}, /* B2P15 */
    {{RES, RES, RES, RES}}, /* B2P16 */
    {{RES, RES, RES, RES}}, /* B2P17 */
    {{RES, RES, RES, RES}}, /* B2P18 */
    {{RES, RES, RES, RES}}, /* B2P19 */
    {{RES, RES, RES, RES}}, /* B2P20 */
    {{RES, RES, RES, RES}}, /* B2P21 */
    {{RES, RES, RES, RES}}, /* B2P22 */
    {{RES, RES, RES, RES}}, /* B2P23 */
    {{RES, RES, RES, RES}}, /* B2P24 */
    {{RES, RES, RES, RES}}, /* B2P25 */
    {{RES, RES, RES, RES}}, /* B2P26 */
    {{RES, RES, RES, RES}}, /* B2P27 */
    {{RES, RES, RES, RES}}, /* B2P28 */
    {{RES, RES, RES, RES}}, /* B2P29 */
    {{RES, RES, RES, RES}}, /* B2P30 */
    {{RES, RES, RES, RES}}, /* B2P31 */
};

struct pin_desc_t BANK(STMP3700,LQFP100,3)[NR_PINS] =
{
    {{RES, RES, RES, DIS}}, /* B3P00 */
    {{RES, RES, RES, DIS}}, /* B3P01 */
    {{RES, RES, RES, DIS}}, /* B3P02 */
    {{RES, RES, RES, DIS}}, /* B3P03 */
    {{RES, RES, RES, DIS}}, /* B3P04 */
    {{RES, RES, RES, DIS}}, /* B3P05 */
    {{RES, RES, RES, DIS}}, /* B3P06 */
    {{RES, RES, RES, DIS}}, /* B3P07 */
    {{RES, RES, RES, DIS}}, /* B3P08 */
    {{RES, RES, RES, DIS}}, /* B3P09 */
    {{RES, RES, RES, DIS}}, /* B3P10 */
    {{RES, RES, RES, DIS}}, /* B3P11 */
    {{RES, RES, RES, DIS}}, /* B3P12 */
    {{RES, RES, RES, DIS}}, /* B3P13 */
    {{RES, RES, RES, DIS}}, /* B3P14 */
    {{RES, RES, RES, DIS}}, /* B3P15 */
    {{RES, RES, RES, DIS}}, /* B3P16 */
    {{RES, RES, RES, DIS}}, /* B3P17 */
    {{RES, RES, RES, DIS}}, /* B3P18 */
    {{RES, RES, RES, DIS}}, /* B3P19 */
    {{RES, RES, RES, DIS}}, /* B3P20 */
    {{RES, RES, RES, DIS}}, /* B3P21 */
    {{RES, RES, RES, RES}}, /* B3P22 */
    {{RES, RES, RES, RES}}, /* B3P23 */
    {{RES, RES, RES, RES}}, /* B3P24 */
    {{RES, RES, RES, RES}}, /* B3P25 */
    {{RES, RES, RES, RES}}, /* B3P26 */
    {{RES, RES, RES, RES}}, /* B3P27 */
    {{RES, RES, RES, RES}}, /* B3P28 */
    {{RES, RES, RES, RES}}, /* B3P29 */
    {{RES, RES, RES, RES}}, /* B3P30 */
    {{RES, RES, RES, RES}}, /* B3P31 */
};

struct pin_desc_t BANK(STMP3600,BGA169,0)[NR_PINS] =
{
    {{P(GPMI,"d0"), RES, RES, IO}}, /* B0P00 */
    {{P(GPMI,"d1"), RES, RES, IO}}, /* B0P01 */
    {{P(GPMI,"d2"), RES, RES, IO}}, /* B0P02 */
    {{P(GPMI,"d3"), RES, RES, IO}}, /* B0P03 */
    {{P(GPMI,"d4"), RES, RES, IO}}, /* B0P04 */
    {{P(GPMI,"d5"), RES, RES, IO}}, /* B0P05 */
    {{P(GPMI,"d6"), RES, RES, IO}}, /* B0P06 */
    {{P(GPMI,"d7"), RES, RES, IO}}, /* B0P07 */
    {{P(GPMI,"d8"), P(EMI,"a15"), RES, IO}}, /* B0P08 */
    {{P(GPMI,"d9"), P(EMI,"a16"), RES, IO}}, /* B0P09 */
    {{P(GPMI,"d10"), P(EMI,"a17"), RES, IO}}, /* B0P10 */
    {{P(GPMI,"d11"), P(EMI,"a18"), RES, IO}}, /* B0P11 */
    {{P(GPMI,"d12"), P(EMI,"a19"), P(GPMI,"ce0n"), IO}}, /* B0P12 */
    {{P(GPMI,"d13"), P(EMI,"a20"), P(GPMI,"ce1n"), IO}}, /* B0P13 */
    {{P(GPMI,"d14"), P(EMI,"a21"), P(GPMI,"ce2n"), IO}}, /* B0P14 */
    {{P(GPMI,"d15"), P(EMI,"a22"), P(GPMI,"ce3n"), IO}}, /* B0P15 */
    {{P(GPMI,"irq"), RES, RES, IO}}, /* B0P16 */
    {{P(GPMI,"rdn"), RES, RES, IO}}, /* B0P17 */
    {{P(GPMI,"rdy"), RES, RES, IO}}, /* B0P18 */
    {{P(GPMI,"rdy3"), P(EMI,"oen"), RES, IO}}, /* B0P19 */
    {{P(GPMI,"rdy2"), RES, RES, IO}}, /* B0P20 */
    {{P(GPMI,"wrn"), RES, RES, IO}}, /* B0P21 */
    {{P(GPMI,"a0"), P(EMI,"a23"), RES, IO}}, /* B0P22 */
    {{P(GPMI,"a1"), P(EMI,"a24"), RES, IO}}, /* B0P23 */
    {{P(GPMI,"a2"), P(EMI,"a25"), RES, IO}}, /* B0P24 */
    {{P(SSP,"det"), RES, P(ETM,"rtck"), IO}}, /* B0P25 */
    {{P(SSP,"cmd"), RES, RES, IO}}, /* B0P26 */
    {{P(SSP,"sck"), RES, RES, IO}}, /* B0P27 */
    {{P(SSP,"d0"), RES, RES, IO}}, /* B0P28 */
    {{P(SSP,"d1"), RES, RES, IO}}, /* B0P29 */
    {{P(SSP,"d2"), RES, RES, IO}}, /* B0P30 */
    {{P(SSP,"d3"), RES, RES, IO}}, /* B0P31 */
};

struct pin_desc_t BANK(STMP3600,BGA169,1)[NR_PINS] =
{
    {{P(LCD,"d0"), P(ETM,"da0"), RES, IO}}, /* B1P00 */
    {{P(LCD,"d1"), P(ETM,"da1"), RES, IO}}, /* B1P01 */
    {{P(LCD,"d2"), P(ETM,"da2"), RES, IO}}, /* B1P02 */
    {{P(LCD,"d3"), P(ETM,"da3"), RES, IO}}, /* B1P03 */
    {{P(LCD,"d4"), P(ETM,"da4"), RES, IO}}, /* B1P04 */
    {{P(LCD,"d5"), P(ETM,"da5"), RES, IO}}, /* B1P05 */
    {{P(LCD,"d6"), P(ETM,"da6"), RES, IO}}, /* B1P06 */
    {{P(LCD,"d7"), P(ETM,"da7"), RES, IO}}, /* B1P07 */
    {{P(LCD,"d8"), P(ETM,"db0"), RES, IO}}, /* B1P08 */
    {{P(LCD,"d9"), P(ETM,"db1"), RES, IO}}, /* B1P09 */
    {{P(LCD,"d10"), P(ETM,"db2"), RES, IO}}, /* B1P10 */
    {{P(LCD,"d11"), P(ETM,"db3"), RES, IO}}, /* B1P11 */
    {{P(LCD,"d12"), P(ETM,"db4"), RES, IO}}, /* B1P12 */
    {{P(LCD,"d13"), P(ETM,"db5"), RES, IO}}, /* B1P13 */
    {{P(LCD,"d14"), P(ETM,"db6"), RES, IO}}, /* B1P14 */
    {{P(LCD,"d15"), P(ETM,"db7"), P(ETM,"rtck"), IO}}, /* B1P15 */
    {{P(LCD,"reset"), P(ETM,"psa1"), RES, IO}}, /* B1P16 */
    {{P(LCD,"rs"), P(ETM,"psa0"), RES, IO}}, /* B1P17 */
    {{P(LCD,"wr"), P(ETM,"psa2"), RES, IO}}, /* B1P18 */
    {{P(LCD,"cs"), P(ETM,"tclk"), RES, IO}}, /* B1P19 */
    {{P(GPMI,"resetn"), P(EMI,"reset"), RES, IO}}, /* B1P20 */
    {{P(LCD,"busy"), RES, RES, IO}}, /* B1P21 */
    {{P(AUART,"cts"), RES, RES, IO}}, /* B1P22 */
    {{P(AUART,"rts"), RES, P(IrDA,"clk"), IO}}, /* B1P23 */
    {{P(AUART,"rx"), RES, RES, IO}}, /* B1P24 */
    {{P(AUART,"tx"), RES, RES, IO}}, /* B1P25 */
    {{RES, RES, RES, IO}}, /* B1P26 */
    {{RES, RES, RES, IO}}, /* B1P27 */
    {{RES, RES, RES, IO}}, /* B1P28 */
    {{RES, RES, RES, IO}}, /* B1P29 */
    {{RES, RES, RES, IO}}, /* B1P30 */
    {{RES, RES, RES, IO}}, /* B1P31 */
};

struct pin_desc_t BANK(STMP3600,BGA169,2)[NR_PINS] =
{
    {{P(EMI,"d0"), RES, RES, IO}}, /* B2P00 */
    {{P(EMI,"d1"), RES, RES, IO}}, /* B2P01 */
    {{P(EMI,"d2"), RES, RES, IO}}, /* B2P02 */
    {{P(EMI,"d3"), RES, RES, IO}}, /* B2P03 */
    {{P(EMI,"d4"), RES, RES, IO}}, /* B2P04 */
    {{P(EMI,"d5"), RES, RES, IO}}, /* B2P05 */
    {{P(EMI,"d6"), RES, RES, IO}}, /* B2P06 */
    {{P(EMI,"d7"), RES, RES, IO}}, /* B2P07 */
    {{P(EMI,"d8"), RES, RES, IO}}, /* B2P08 */
    {{P(EMI,"d9"), RES, RES, IO}}, /* B2P09 */
    {{P(EMI,"d10"), RES, RES, IO}}, /* B2P10 */
    {{P(EMI,"d11"), RES, RES, IO}}, /* B2P11 */
    {{P(EMI,"d12"), RES, RES, IO}}, /* B2P12 */
    {{P(EMI,"d13"), RES, RES, IO}}, /* B2P13 */
    {{P(EMI,"d14"), RES, RES, IO}}, /* B2P14 */
    {{P(EMI,"d15"), RES, RES, IO}}, /* B2P15 */
    {{P(EMI,"a0"), RES, RES, IO}}, /* B2P16 */
    {{P(EMI,"a1"), RES, RES, IO}}, /* B2P17 */
    {{P(EMI,"a2"), RES, RES, IO}}, /* B2P18 */
    {{P(EMI,"a3"), RES, RES, IO}}, /* B2P19 */
    {{P(EMI,"a4"), RES, RES, IO}}, /* B2P20 */
    {{P(EMI,"a5"), RES, RES, IO}}, /* B2P21 */
    {{P(EMI,"a6"), RES, RES, IO}}, /* B2P22 */
    {{P(EMI,"a7"), RES, RES, IO}}, /* B2P23 */
    {{P(EMI,"a8"), RES, RES, IO}}, /* B2P24 */
    {{P(EMI,"a9"), RES, RES, IO}}, /* B2P25 */
    {{P(EMI,"a10"), RES, RES, IO}}, /* B2P26 */
    {{P(EMI,"a11"), RES, RES, IO}}, /* B2P27 */
    {{P(EMI,"a12"), RES, RES, IO}}, /* B2P28 */
    {{P(EMI,"a13"), RES, RES, IO}}, /* B2P29 */
    {{P(EMI,"a14"), RES, RES, IO}}, /* B2P30 */
    {{P(EMI,"rasn"), RES, RES, IO}}, /* B2P31 */
};

struct pin_desc_t BANK(STMP3600,BGA169,3)[NR_PINS] =
{
    {{P(EMI,"ce0n"), P(GPMI,"ce0n"), RES, IO}}, /* B3P00 */
    {{P(EMI,"ce1n"), P(GPMI,"ce1n"), RES, IO}}, /* B3P01 */
    {{P(EMI,"ce2n"), P(GPMI,"ce2n"), RES, IO}}, /* B3P02 */
    {{P(EMI,"ce3n"), P(GPMI,"ce3n"), RES, IO}}, /* B3P03 */
    {{P(EMI,"clk"), RES, RES, IO}}, /* B3P04 */
    {{P(EMI,"cke"), RES, RES, IO}}, /* B3P05 */
    {{P(EMI,"casn"), RES, RES, IO}}, /* B3P06 */
    {{P(EMI,"dqm0"), RES, RES, IO}}, /* B3P07 */
    {{P(EMI,"dqm1"), RES, RES, IO}}, /* B3P08 */
    {{P(EMI,"wen"), RES, RES, IO}}, /* B3P09 */
    {{P(PWM,"0"), P(ETM,"tsynca"), P(DUART,"rx"), IO}}, /* B3P10 */
    {{P(PWM,"1"), P(ETM,"tsyncb"), P(DUART,"tx"), IO}}, /* B3P11 */
    {{P(PWM,"2"), P(ETM,"psb2"), P(ETM,"rtclk"), IO}}, /* B3P12 */
    {{P(PWM,"3"), P(ETM,"psb0"), P(SPDIF,""), IO}}, /* B3P13 */
    {{P(PWM,"4"), P(ETM,"psb1"), RES, IO}}, /* B3P14 */
    {{P(TIMROT,"rotarya"), RES, RES, IO}}, /* B3P15 */
    {{P(TIMROT,"rotaryb"), RES, RES, IO}}, /* B3P16 */
    {{P(I2C,"scl"), RES, RES, IO}}, /* B3P17 */
    {{P(I2C,"sda"), RES, RES, IO}}, /* B3P18 */
    {{RES, RES, RES, IO}}, /* B3P19 */
    {{RES, RES, RES, IO}}, /* B3P20 */
    {{RES, RES, RES, IO}}, /* B3P21 */
    {{RES, RES, RES, IO}}, /* B3P22 */
    {{RES, RES, RES, IO}}, /* B3P23 */
    {{RES, RES, RES, IO}}, /* B3P24 */
    {{RES, RES, RES, IO}}, /* B3P25 */
    {{RES, RES, RES, IO}}, /* B3P26 */
    {{RES, RES, RES, IO}}, /* B3P27 */
    {{RES, RES, RES, IO}}, /* B3P28 */
    {{RES, RES, RES, IO}}, /* B3P29 */
    {{RES, RES, RES, IO}}, /* B3P30 */
    {{RES, RES, RES, IO}}, /* B3P31 */
};

#define SOC(soc, ver) soc##_##ver

struct bank_map_t SOC(IMX233,BGA169)[NR_BANKS] =
{
    {BANK(IMX233,BGA169,0)},
    {BANK(IMX233,BGA169,1)},
    {BANK(IMX233,BGA169,2)},
    {BANK(IMX233,BGA169,3)},
};

struct bank_map_t SOC(STMP3700,BGA169)[NR_BANKS] =
{
    {BANK(STMP3700,BGA169,0)},
    {BANK(STMP3700,BGA169,1)},
    {BANK(STMP3700,BGA169,2)},
    {BANK(STMP3700,BGA169,3)},
};

struct bank_map_t SOC(STMP3700,LQFP100)[NR_BANKS] =
{
    {BANK(STMP3700,LQFP100,0)},
    {BANK(STMP3700,LQFP100,1)},
    {BANK(STMP3700,LQFP100,2)},
    {BANK(STMP3700,LQFP100,3)},
};

struct bank_map_t SOC(STMP3600,BGA169)[NR_BANKS] =
{
    {BANK(STMP3600,BGA169,0)},
    {BANK(STMP3600,BGA169,1)},
    {BANK(STMP3600,BGA169,2)},
    {BANK(STMP3600,BGA169,3)},
};

#undef P
#undef Q
#undef R
#undef IO
#undef DIS
#undef RES

struct soc_t socs [] =
{
    {"imx233", "bga169", SOC(IMX233,BGA169)},
    {"stmp3700", "bga169", SOC(STMP3700,BGA169)},
    {"stmp3700", "lqfp100", SOC(STMP3700,LQFP100)},
    {"stmp3600", "bga169", SOC(STMP3600,BGA169)},
};

#define NR_SOCS (sizeof(socs) / sizeof(socs[0]))