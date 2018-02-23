#include <stdint.h>

struct jz_pin_function_desc_t
{
    const char *name;
    int group;
    int block;
};

struct jz_pin_desc_t
{
    struct jz_pin_function_desc_t function[4];
};

struct jz_bank_map_t
{
    int nr_pins;
    struct jz_pin_desc_t *pins;
};

struct jz_soc_t
{
    const char *soc;
    int nr_functions;
    int nr_banks;
    struct jz_bank_map_t *map;
};

#define JZ_PIN_GROUP_NEMC   0
#define JZ_PIN_GROUP_MSC    1
#define JZ_PIN_GROUP_SSI    2
#define JZ_PIN_GROUP_OWI    3
#define JZ_PIN_GROUP_DMA    4
#define JZ_PIN_GROUP_CIM    5
#define JZ_PIN_GROUP_TSSI   6
#define JZ_PIN_GROUP_EPD    7
#define JZ_PIN_GROUP_LCD    8
#define JZ_PIN_GROUP_UART   9
#define JZ_PIN_GROUP_PCM    10
#define JZ_PIN_GROUP_PS2    11
#define JZ_PIN_GROUP_PWM    12
#define JZ_PIN_GROUP_SCC    13
#define JZ_PIN_GROUP_AIC    14
#define JZ_PIN_GROUP_BOOT   15
#define JZ_PIN_GROUP_I2C    16
#define JZ_PIN_GROUP_GPS    17
#define JZ_PIN_GROUP_NONE   18

#define JZ_PIN_NO_BLOCK     -1

#define _STR(x) #x
#define STR(x)  _STR(x)

#define JZ_PIN_GROUP_PREFIX_NEMC    "nemc_"
#define JZ_PIN_GROUP_PREFIX_MSC     "msc"
#define JZ_PIN_GROUP_PREFIX_SSI     "ssi"
#define JZ_PIN_GROUP_PREFIX_OWI     ""
#define JZ_PIN_GROUP_PREFIX_CIM     "cim_"
#define JZ_PIN_GROUP_PREFIX_TSSI    "ts_"
#define JZ_PIN_GROUP_PREFIX_EPD     "epd_"
#define JZ_PIN_GROUP_PREFIX_LCD     "lcd_"
#define JZ_PIN_GROUP_PREFIX_UART    "uart"
#define JZ_PIN_GROUP_PREFIX_PCM     "pcm_"
#define JZ_PIN_GROUP_PREFIX_PS2     "ps2_"
#define JZ_PIN_GROUP_PREFIX_PWM     "pwm"
#define JZ_PIN_GROUP_PREFIX_SCC     "scc_"
#define JZ_PIN_GROUP_PREFIX_AIC     "aic_"
#define JZ_PIN_GROUP_PREFIX_BOOT    "boot_"
#define JZ_PIN_GROUP_PREFIX_I2C     "i2c"
#define JZ_PIN_GROUP_PREFIX_GPS     "gps_"
#define JZ_PIN_GROUP_PREFIX_DMA     "dma_"
#define JZ_PIN_GROUP_PREFIX_NONE    ""

#define R(group,name,block) {JZ_PIN_GROUP_PREFIX_##group name, JZ_PIN_GROUP_##group, block}
#define Q(group,block,name) R(group,STR(block) "_" name, block)
#define P(group,name) R(group,name,JZ_PIN_NO_BLOCK)

#define DIS     P(NONE,"disabled")

#define JZ_BANK(soc,bank) soc##_port_##bank

struct jz_pin_desc_t JZ_BANK(jz4760b,A)[] =
{
    {{P(NEMC,"sd0"), DIS, DIS, DIS}}, /* PA0 */
    {{P(NEMC,"sd1"), DIS, DIS, DIS}}, /* PA1 */
    {{P(NEMC,"sd2"), DIS, DIS, DIS}}, /* PA2 */
    {{P(NEMC,"sd3"), DIS, DIS, DIS}}, /* PA3 */
    {{P(NEMC,"sd4"), DIS, DIS, DIS}}, /* PA4 */
    {{P(NEMC,"sd5"), DIS, DIS, DIS}}, /* PA5 */
    {{P(NEMC,"sd6"), DIS, DIS, DIS}}, /* PA6 */
    {{P(NEMC,"sd7"), DIS, DIS, DIS}}, /* PA7 */
    {{P(NEMC,"sd8"), DIS, DIS, DIS}}, /* PA8 */
    {{P(NEMC,"sd9"), DIS, DIS, DIS}}, /* PA9 */
    {{P(NEMC,"sd10"), DIS, DIS, DIS}}, /* PA10 */
    {{P(NEMC,"sd11"), DIS, DIS, DIS}}, /* PA11 */
    {{P(NEMC,"sd12"), DIS, DIS, DIS}}, /* PA12 */
    {{P(NEMC,"sd13"), DIS, DIS, DIS}}, /* PA13 */
    {{P(NEMC,"sd14"), DIS, DIS, DIS}}, /* PA14 */
    {{P(NEMC,"sd15"), DIS, DIS, DIS}}, /* PA15 */
    {{P(NEMC,"rd"), DIS, DIS, DIS}}, /* PA16 */
    {{P(NEMC,"we"), DIS, DIS, DIS}}, /* PA17 */
    {{P(NEMC,"fre"), Q(MSC,0,"clk"), Q(SSI,0,"clk"), DIS}}, /* PA18 */
    {{P(NEMC,"fwe"), Q(MSC,0,"cmd"), Q(SSI,0,"cmd"), DIS}}, /* PA19 */
    {{Q(MSC,0,"d0"), Q(SSI,0,"dr"), DIS, DIS}}, /* PA20 */
    {{P(NEMC,"cs1"), Q(MSC,0,"d1"), Q(SSI,0,"dt"), DIS}}, /* PA21 */
    {{P(NEMC,"cs2"), Q(MSC,0,"d2"), DIS, DIS}}, /* PA22 */
    {{P(NEMC,"cs3"), Q(MSC,0,"d3"), DIS, DIS}}, /* PA23 */
    {{P(NEMC,"cs4"), DIS, DIS, DIS}}, /* PA24 */
    {{P(NEMC,"cs5"), DIS, DIS, DIS}}, /* PA25 */
    {{P(NEMC,"cs6"), DIS, DIS, DIS}}, /* PA26 */
    {{P(NEMC,"wait"), DIS, DIS, DIS}}, /* PA27 */
    {{P(DMA,"dreq0"), DIS, DIS, DIS}}, /* PA28 */
    {{P(DMA,"dack0"), P(OWI,"owi"), DIS, DIS}}, /* PA29 */
    {{DIS, DIS, DIS, DIS}}, /* PA30 */
    {{DIS, DIS, DIS, DIS}}, /* PA31 */
};

struct jz_pin_desc_t JZ_BANK(jz4760b,B)[] =
{
    {{P(NEMC,"sa0"), DIS, DIS, DIS}}, /* PB0 */
    {{P(NEMC,"sa1"), DIS, DIS, DIS}}, /* PB1 */
    {{P(NEMC,"sa2"), DIS, DIS, DIS}}, /* PB2 */
    {{P(NEMC,"sa3"), DIS, DIS, DIS}}, /* PB3 */
    {{P(NEMC,"sa4"), P(DMA,"dreq1"), DIS, DIS}}, /* PB4 */
    {{P(NEMC,"sa5"), P(DMA,"dack1"), DIS, DIS}}, /* PB5 */
    {{P(CIM,"pclk"), P(TSSI,"clk"), Q(SSI,1,"dr"), Q(MSC,2,"d0")}}, /* PB6 */
    {{P(CIM,"hsyn"), P(TSSI,"frm"), Q(SSI,1,"clk"), Q(MSC,2,"clk")}}, /* PB7 */
    {{P(CIM,"vsyn"), P(TSSI,"str"), Q(SSI,1,"ce0"), Q(MSC,2,"cmd")}}, /* PB8 */
    {{P(CIM,"mclk"), P(TSSI,"fail"), Q(SSI,1,"dt"), P(EPD,"pwc")}}, /* PB9 */
    {{P(CIM,"d0"), P(TSSI,"di0"), DIS, P(EPD,"pwr0")}}, /* PB10 */
    {{P(CIM,"d1"), P(TSSI,"di1"), DIS, P(EPD,"pwr1")}}, /* PB11 */
    {{P(CIM,"d2"), P(TSSI,"di2"), DIS, P(EPD,"sce2")}}, /* PB12 */
    {{P(CIM,"d3"), P(TSSI,"di3"), DIS, P(EPD,"sce3")}}, /* PB13 */
    {{P(CIM,"d4"), P(TSSI,"di4"), DIS, P(EPD,"sce4")}}, /* PB14 */
    {{P(CIM,"d5"), P(TSSI,"di5"), DIS, P(EPD,"sce5")}}, /* PB15 */
    {{P(CIM,"d6"), P(TSSI,"di6"), DIS, P(EPD,"pwr2")}}, /* PB16 */
    {{P(CIM,"d7"), P(TSSI,"di7"), DIS, P(EPD,"pwr3")}}, /* PB17 */
    {{DIS, DIS, DIS, DIS}}, /* PB18 */
    {{DIS, DIS, DIS, DIS}}, /* PB19 */
    {{Q(MSC,2,"d0"), Q(SSI,0,"dr"), Q(SSI,1,"dr"), P(TSSI,"di0")}}, /* PB20 */
    {{Q(MSC,2,"d1"), Q(SSI,0,"dt"), Q(SSI,1,"dt"), P(TSSI,"di1")}}, /* PB21 */
    {{P(TSSI,"di2"), DIS, DIS, DIS}}, /* PB22 */
    {{P(TSSI,"di3"), DIS, DIS, DIS}}, /* PB23 */
    {{P(TSSI,"di4"), DIS, DIS, DIS}}, /* PB24 */
    {{P(TSSI,"di5"), DIS, DIS, DIS}}, /* PB25 */
    {{P(TSSI,"di6"), DIS, DIS, DIS}}, /* PB26 */
    {{P(TSSI,"di7"), DIS, DIS, DIS}}, /* PB27 */
    {{Q(MSC,2,"clk"), Q(SSI,0,"clk"), Q(SSI,1,"clk"), P(TSSI,"clk")}}, /* PB28 */
    {{Q(MSC,2,"cmd"), Q(SSI,0,"ce0"), Q(SSI,1,"ce0"), P(TSSI,"str")}}, /* PB29 */
    {{Q(MSC,2,"d2"), Q(SSI,0,"gpc"), Q(SSI,1,"gpc"), P(TSSI,"fail")}}, /* PB30 */
    {{Q(MSC,2,"d3"), Q(SSI,0,"ce1"), Q(SSI,1,"ce1"), P(TSSI,"frm")}}, /* PB31 */
};

struct jz_pin_desc_t JZ_BANK(jz4760b,C)[] =
{
    {{P(LCD,"b0"), P(LCD,"rev"), DIS, DIS}}, /* PC0 */
    {{P(LCD,"b1"), P(LCD,"ps"), DIS, DIS}}, /* PC1 */
    {{P(LCD,"b2"), DIS, DIS, DIS}}, /* PC2 */
    {{P(LCD,"b3"), DIS, DIS, DIS}}, /* PC3 */
    {{P(LCD,"b4"), DIS, DIS, DIS}}, /* PC4 */
    {{P(LCD,"b5"), DIS, DIS, DIS}}, /* PC5 */
    {{P(LCD,"b6"), DIS, DIS, DIS}}, /* PC6 */
    {{P(LCD,"b7"), DIS, DIS, DIS}}, /* PC7 */
    {{P(LCD,"pclk"), DIS, DIS, DIS}}, /* PC8 */
    {{P(LCD,"de"), DIS, DIS, DIS}}, /* PC9 */
    {{P(LCD,"g0"), P(LCD,"spl"), DIS, DIS}}, /* PC10 */
    {{P(LCD,"g1"), DIS, DIS, DIS}}, /* PC11 */
    {{P(LCD,"g2"), DIS, DIS, DIS}}, /* PC12 */
    {{P(LCD,"g3"), DIS, DIS, DIS}}, /* PC13 */
    {{P(LCD,"g4"), DIS, DIS, DIS}}, /* PC14 */
    {{P(LCD,"g5"), DIS, DIS, DIS}}, /* PC15 */
    {{P(LCD,"g6"), DIS, DIS, DIS}}, /* PC16 */
    {{P(LCD,"g7"), DIS, DIS, DIS}}, /* PC17 */
    {{P(LCD,"hsyn"), DIS, DIS, DIS}}, /* PC18 */
    {{P(LCD,"vsyn"), DIS, DIS, DIS}}, /* PC19 */
    {{P(LCD,"r0"), P(LCD,"cls"), DIS, DIS}}, /* PC20 */
    {{P(LCD,"r1"), DIS, DIS, DIS}}, /* PC21 */
    {{P(LCD,"r2"), DIS, DIS, DIS}}, /* PC22 */
    {{P(LCD,"r3"), DIS, DIS, DIS}}, /* PC23 */
    {{P(LCD,"r4"), DIS, DIS, DIS}}, /* PC24 */
    {{P(LCD,"r5"), DIS, DIS, DIS}}, /* PC25 */
    {{P(LCD,"r6"), DIS, DIS, DIS}}, /* PC26 */
    {{P(LCD,"r7"), DIS, DIS, DIS}}, /* PC27 */
    {{Q(UART,2,"rxd"), DIS, DIS, DIS}}, /* PC28 */
    {{Q(UART,2,"cts"), DIS, DIS, DIS}}, /* PC29 */
    {{Q(UART,2,"txd"), DIS, DIS, DIS}}, /* PC30 */
    {{Q(UART,2,"rts"), DIS, DIS, DIS}}, /* PC31 */
};

struct jz_pin_desc_t JZ_BANK(jz4760b,D)[] =
{
    {{P(PCM,"dout"), DIS, DIS, DIS}}, /* PD0 */
    {{P(PCM,"clk"), DIS, DIS, DIS}}, /* PD1 */
    {{P(PCM,"syn"), DIS, DIS, DIS}}, /* PD2 */
    {{P(PCM,"din"), DIS, DIS, DIS}}, /* PD3 */
    {{P(PS2,"mclk"), DIS, DIS, DIS}}, /* PD4 */
    {{P(PS2,"mdata"), DIS, DIS, DIS}}, /* PD5 */
    {{P(PS2,"kclk"), DIS, DIS, DIS}}, /* PD6 */
    {{P(PS2,"kdata"), DIS, DIS, DIS}}, /* PD7 */
    {{P(SCC,"data"), DIS, DIS, DIS}}, /* PD8 */
    {{P(SCC,"clk"), DIS, DIS, DIS}}, /* PD9 */
    {{P(PWM,"6"), DIS, DIS, DIS}}, /* PD10 */
    {{P(PWM,"7"), DIS, DIS, DIS}}, /* PD11 */
    {{Q(UART,3,"rxd"), P(AIC,"bclk"), Q(SSI,1,"dt"), P(EPD,"pwr4")}}, /* PD12 */
    {{P(AIC,"sync"), Q(MSC,2,"d0"), Q(SSI,1,"dr"), P(EPD,"pwr5")}}, /* PD13 */
    {{DIS, DIS, DIS, DIS}}, /* PD14 */
    {{DIS, DIS, DIS, DIS}}, /* PD15 */
    {{DIS, DIS, DIS, DIS}}, /* PD16 */
    {{P(BOOT,"sel0"), DIS, DIS, DIS}}, /* PD17 */
    {{P(BOOT,"sel1"), DIS, DIS, DIS}}, /* PD18 */
    {{P(BOOT,"sel2"), DIS, DIS, DIS}}, /* PD19 */
    {{Q(MSC,1,"d0"), Q(SSI,0,"dr"), Q(SSI,1,"dr"), DIS}}, /* PD20 */
    {{Q(MSC,1,"d1"), Q(SSI,0,"dt"), Q(SSI,1,"dt"), DIS}}, /* PD21 */
    {{Q(MSC,1,"d2"), Q(SSI,0,"gpc"), Q(SSI,1,"gpc"), DIS}}, /* PD22 */
    {{Q(MSC,1,"d3"), Q(SSI,0,"ce1"), Q(SSI,1,"ce1"), DIS}}, /* PD23 */
    {{Q(MSC,1,"clk"), Q(SSI,0,"clk"), Q(SSI,1,"clk"), DIS}}, /* PD24 */
    {{Q(MSC,1,"cmd"), Q(SSI,0,"ce0"), Q(SSI,1,"ce0"), DIS}}, /* PD25 */
    {{Q(UART,1,"rxd"), DIS, DIS, DIS}}, /* PD26 */
    {{Q(UART,1,"cts"), DIS, DIS, DIS}}, /* PD27 */
    {{Q(UART,1,"txd"), DIS, DIS, DIS}}, /* PD28 */
    {{Q(UART,1,"rts"), DIS, DIS, DIS}}, /* PD29 */
    {{Q(I2C,0,"sda"), DIS, DIS, DIS}}, /* PD30 */
    {{Q(I2C,0,"sck"), DIS, DIS, DIS}}, /* PD31 */
};

struct jz_pin_desc_t JZ_BANK(jz4760b,E)[] =
{
    {{P(PWM,"0"), DIS, DIS, DIS}}, /* PE0 */
    {{P(PWM,"1"), DIS, DIS, DIS}}, /* PE1 */
    {{P(PWM,"2"), DIS, DIS, DIS}}, /* PE2 */
    {{P(PWM,"3"), DIS, DIS, DIS}}, /* PE3 */
    {{P(PWM,"4"), DIS, DIS, DIS}}, /* PE4 */
    {{P(PWM,"5"), Q(UART,3,"txd"), P(AIC,"sclk"), DIS}}, /* PE5 */
    {{P(AIC,"dati"), Q(MSC,2,"cmd"), Q(SSI,1,"ce0"), P(EPD,"pwr6")}}, /* PE6 */
    {{P(AIC,"dato"), Q(MSC,2,"clk"), Q(SSI,1,"clk"), P(EPD,"pwr7")}}, /* PE7 */
    {{Q(UART,3,"cts"), DIS, DIS, DIS}}, /* PE8 */
    {{Q(UART,3,"rts"), DIS, DIS, DIS}}, /* PE9 */
    {{P(NONE,"otg_vbus"), DIS, DIS, DIS}}, /* PE10 */
    {{P(AIC,"dato1"), DIS, DIS, DIS}}, /* PE11 */
    {{P(AIC,"dato2"), DIS, DIS, DIS}}, /* PE12 */
    {{P(AIC,"dato3"), DIS, DIS, DIS}}, /* PE13 */
    {{Q(SSI,0,"dr"), Q(SSI,1,"dr"), DIS, DIS}}, /* PE14 */
    {{Q(SSI,0,"clk"), Q(SSI,1,"clk"), DIS, DIS}}, /* PE15 */
    {{Q(SSI,0,"ce0"), Q(SSI,1,"ce0"), DIS, DIS}}, /* PE16 */
    {{Q(SSI,0,"dt"), Q(SSI,1,"dt"), DIS, DIS}}, /* PE17 */
    {{Q(SSI,0,"ce1"), Q(SSI,1,"ce1"), DIS, DIS}}, /* PE18 */
    {{Q(SSI,0,"gpc"), Q(SSI,1,"gpc"), DIS, DIS}}, /* PE19 */
    {{Q(MSC,0,"d0"), Q(MSC,1,"d0"), Q(MSC,2,"d0"), DIS}}, /* PE20 */
    {{Q(MSC,0,"d1"), Q(MSC,1,"d1"), Q(MSC,2,"d1"), DIS}}, /* PE21 */
    {{Q(MSC,0,"d2"), Q(MSC,1,"d2"), Q(MSC,2,"d2"), DIS}}, /* PE22 */
    {{Q(MSC,0,"d3"), Q(MSC,1,"d3"), Q(MSC,2,"d3"), DIS}}, /* PE23 */
    {{Q(MSC,0,"d4"), Q(MSC,1,"d4"), Q(MSC,2,"d4"), DIS}}, /* PE24 */
    {{Q(MSC,0,"d5"), Q(MSC,1,"d5"), Q(MSC,2,"d5"), DIS}}, /* PE25 */
    {{Q(MSC,0,"d6"), Q(MSC,1,"d6"), Q(MSC,2,"d6"), DIS}}, /* PE26 */
    {{Q(MSC,0,"d7"), Q(MSC,1,"d7"), Q(MSC,2,"d7"), DIS}}, /* PE27 */
    {{Q(MSC,0,"clk"), Q(MSC,1,"clk"), Q(MSC,2,"clk"), DIS}}, /* PE28 */
    {{Q(MSC,0,"cmd"), Q(MSC,1,"cmd"), Q(MSC,2,"cmd"), DIS}}, /* PE29 */
    {{Q(I2C,1,"sda"), DIS, DIS, DIS}}, /* PE30 */
    {{Q(I2C,1,"sck"), DIS, DIS, DIS}}, /* PE31 */
};

struct jz_pin_desc_t JZ_BANK(jz4760b,F)[] =
{
    {{Q(UART,0,"rxd"), P(GPS,"clk"), Q(SSI,1,"dr"), Q(MSC,2,"d0")}}, /* PF0 */
    {{Q(UART,0,"cts"), P(GPS,"mag"), Q(SSI,1,"ce0"), Q(MSC,2,"cmd")}}, /* PF1 */
    {{Q(UART,0,"rts"), P(GPS,"sig"), Q(SSI,1,"clk"), Q(MSC,2,"clk")}}, /* PF2 */
    {{Q(UART,0,"txd"), DIS, Q(SSI,1,"dt"), DIS}}, /* PF3 */
    {{DIS, DIS, DIS, DIS}}, /* PF4 */
    {{DIS, DIS, DIS, DIS}}, /* PF5 */
    {{DIS, DIS, DIS, DIS}}, /* PF6 */
    {{DIS, DIS, DIS, DIS}}, /* PF7 */
    {{DIS, DIS, DIS, DIS}}, /* PF8 */
    {{DIS, DIS, DIS, DIS}}, /* PF9 */
    {{DIS, DIS, DIS, DIS}}, /* PF10 */
    {{DIS, DIS, DIS, DIS}}, /* PF11 */
};

#undef P
#undef Q
#undef R
#undef DIS

#define B(soc,bank) {sizeof(JZ_BANK(soc,bank)) / sizeof(JZ_BANK(soc,bank)[0]), JZ_BANK(soc,bank)}

struct jz_bank_map_t jz4760b_ports[] =
{
    B(jz4760b, A), B(jz4760b, B), B(jz4760b, C), B(jz4760b, D), B(jz4760b, E), B(jz4760b, F),
};

#undef BANK
#undef B

#define SOC(soc, functions, ports) {soc, functions, sizeof(ports) / sizeof(ports[0]), ports}

struct jz_soc_t jz4760b_soc = SOC("JZ4760B", 4, jz4760b_ports);

#undef SOC
