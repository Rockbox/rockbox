typedef unsigned int uint32_t;

#define PMU_BASE              0xB0000000
#define PMU_CTL               (*(volatile uint32_t *)(PMU_BASE + 0x00))
#define PMU_CTL_BL_EN         (1<<15)
#define PMU_LRADC             (*(volatile uint32_t *)(PMU_BASE + 0x04))
#define PMU_CHG               (*(volatile uint32_t *)(PMU_BASE + 0x08))
#define PMU_CHG_PBLS          (1<<15)
#define PMU_CHG_PBLS_PWM      (1<<15)
#define PMU_CHG_PBLS_BL_NDR   (0<<15)
#define PMU_CHG_PPHS          (1<<14)
#define PMU_CHG_PPHS_HIGH     (1<<14)
#define PMU_CHG_PPHS_LOW      (0<<14)
#define PMU_CHG_PDUT(x)       (((x) & 0x1f) << 8)
#define PMU_CHG_PDOUT_MASK    (0x1f << 8)

#define CMU_BASE              0xB0010000
#define CMU_COREPLL           (*(volatile uint32_t *)(CMU_BASE + 0x00))
#define CMU_DSPPLL            (*(volatile uint32_t *)(CMU_BASE + 0x04))
#define CMU_AUDIOPLL          (*(volatile uint32_t *)(CMU_BASE + 0x08))
#define CMU_BUSCLK            (*(volatile uint32_t *)(CMU_BASE + 0x0C))                                    
#define CMU_SDRCLK            (*(volatile uint32_t *)(CMU_BASE + 0x10))
#define CMU_ATACLK            (*(volatile uint32_t *)(CMU_BASE + 0x04))
#define CMU_NANDCLK           (*(volatile uint32_t *)(CMU_BASE + 0x18))
#define CMU_SDCLK             (*(volatile uint32_t *)(CMU_BASE + 0x1C))
#define CMU_MHACLK            (*(volatile uint32_t *)(CMU_BASE + 0x20))
#define CMU_BTCLK             (*(volatile uint32_t *)(CMU_BASE + 0x24))
#define CMU_IRCLK             (*(volatile uint32_t *)(CMU_BASE + 0x28))
#define CMU_UART2CLK          (*(volatile uint32_t *)(CMU_BASE + 0x2C))
#define CMU_DMACLK            (*(volatile uint32_t *)(CMU_BASE + 0x30))
#define CMU_FMCLK             (*(volatile uint32_t *)(CMU_BASE + 0x34))
#define CMU_FMCLK_BCKE        (1<<5)
#define CMI_FMCLK_BCKS        (1<<4)
#define CMU_FMCLK_BCKS_32K    (0<<4)
#define CMU_FMCLK_BCKS_3M     (1<<4)

#define CMU_FMCLK_BCLK_MASK   (CMI_FMCLK_BCKS | (3<<2))
#define CMU_FMCLK_BCLK_3M     (CMU_FMCLK_BCKS_3M | (0<<2))
#define CMU_FMCLK_BCLK_1_5M   (CMU_FMCLK_BCKS_3M | (1<<2))
#define CMU_FMCLK_BCLK_750K   (CMU_FMCLK_BCKS_3M | (2<<2))
#define CMU_FMCLK_BCLK_375K   (CMU_FMCLK_BCKS_3M | (3<<2))

#define CMU_FMCLK_BCLK_32K    (0<<2)
#define CMU_FMCLK_BCLK_16K    (1<<2)
#define CMU_FMCLK_BCLK_8K     (2<<2)
#define CMU_FMCLK_BCLK_4K     (3<<2)

#define CMU_MCACLK            (*(volatile uint32_t *)(CMU_BASE + 0x38))

#define CMU_DEVCLKEN          (*(volatile uint32_t *)(CMU_BASE + 0x80))
#define CMU_DEVRST            (*(volatile uint32_t *)(CMU_BASE + 0x84))

#define RTC_BASE              0xB0018000
#define RTC_CTL               (*(volatile uint32_t *)(RTC_BASE + 0x00))
#define RTC_DHMS              (*(volatile uint32_t *)(RTC_BASE + 0x04))
#define RTC_YMD               (*(volatile uint32_t *)(RTC_BASE + 0x08))
#define RTC_DHMSALM           (*(volatile uint32_t *)(RTC_BASE + 0x0C))
#define RTC_YMDALM            (*(volatile uint32_t *)(RTC_BASE + 0x10))
#define RTC_WDCTL             (*(volatile uint32_t *)(RTC_BASE + 0x14))
#define RTC_WDCTL_CLR         (1<<0)

#define RTC_T0CTL             (*(volatile uint32_t *)(RTC_BASE + 0x18))
#define RTC_T0                (*(volatile uint32_t *)(RTC_BASE + 0x1C))
#define RTC_T1CTL             (*(volatile uint32_t *)(RTC_BASE + 0x20))
#define RTC_T1                (*(volatile uint32_t *)(RTC_BASE + 0x24))

#define INTC_BASE             0xB0020000
#define INTC_PD               (*(volatile uint32_t *)(INTC_BASE + 0x00))
#define INTC_MSK              (*(volatile uint32_t *)(INTC_BASE + 0x04))
#define INTC_CFG0             (*(volatile uint32_t *)(INTC_BASE + 0x08))
#define INTC_CFG1             (*(volatile uint32_t *)(INTC_BASE + 0x0C))
#define INTC_CFG2             (*(volatile uint32_t *)(INTC_BASE + 0x10))
#define INTC_EXTCTL           (*(volatile uint32_t *)(INTC_BASE + 0x14))

#define SRAMOC_BASE           0xB0030000
#define SRAMOC_CTL            (*(volatile uint32_t *)(SRAMOC_BASE + 0x00))
#define SRAMOC_STAT           (*(volatile uint32_t *)(SRAMOC_BASE + 0x04))

#define BOOT_BASE             0xB00380000
#define BOOT_NORCTL           (*(volatile uint32_t *)(BOOT_BASE + 0x00))
#define BOOT_BROMCTL          (*(volatile uint32_t *)(BOOT_BASE + 0x04))
#define BOOT_CHIPID           (*(volatile uint32_t *)(BOOT_BASE + 0x08))

#define PCNT_BASE             0xB0040000
#define PCNT_CTL              (*(volatile uint32_t *)(PCNT_BASE + 0x00))
#define PCNT_PC0              (*(volatile uint32_t *)(PCNT_BASE + 0x04))
#define PCNT_PC1              (*(volatile uint32_t *)(PCNT_BASE + 0x08))

#define DSP_BASE              0xB0050000
#define DSP_HDR0              (*(volatile uint32_t *)(DSP_BASE + 0x00))
#define DSP_HDR1              (*(volatile uint32_t *)(DSP_BASE + 0x04))
#define DSP_HDR2              (*(volatile uint32_t *)(DSP_BASE + 0x08))
#define DSP_HDR3              (*(volatile uint32_t *)(DSP_BASE + 0x0C))
#define DSP_HDR4              (*(volatile uint32_t *)(DSP_BASE + 0x10))
#define DSP_HDR5              (*(volatile uint32_t *)(DSP_BASE + 0x14))
#define DSP_HSR6              (*(volatile uint32_t *)(DSP_BASE + 0x18))
#define DSP_HSR7              (*(volatile uint32_t *)(DSP_BASE + 0x1C))
#define DSP_CTL               (*(volatile uint32_t *)(DSP_BASE + 0x20)) 

#define DMAC_BASE(n)          (0xB0060000 + (n<<5))
#define DMAC_CTL              (*(volatile uint32_t *)(DMAC_BASE(0) + 0x00))  
#define DMAC_IRQEN            (*(volatile uint32_t *)(DMAC_BASE(0) + 0x04))
#define DMAC_IRQPD            (*(volatile uint32_t *)(DMAC_BASE(0) + 0x08))

/* n in range 0-7 */
#define DMA_MODE(n)           (*(volatile uint32_t *)(DMAC_BASE(n) + 0x100))
#define DMA_SRC(n)            (*(volatile uint32_t *)(DMAC_BASE(n) + 0x104))
#define DMA_DST(n)            (*(volatile uint32_t *)(DMAC_BASE(n) + 0x108))
#define DMA_CNT(n)            (*(volatile uint32_t *)(DMAC_BASE(n) + 0x10C))
#define DMA_REM(n)            (*(volatile uint32_t *)(DMAC_BASE(n) + 0x110))
#define DMA_CMD(n)            (*(volatile uint32_t *)(DMAC_BASE(n) + 0x114))

#define SDR_BASE              0xB0070000
#define SDR_CTL               (*(volatile uint32_t *)(SDR_BASE + 0x00))    
#define SDR_ADDRCFG           (*(volatile uint32_t *)(SDR_BASE + 0x04))
#define SDR_EN                (*(volatile uint32_t *)(SDR_BASE + 0x08))     
#define SDR_CMD               (*(volatile uint32_t *)(SDR_BASE + 0x0C))    
#define SDR_STAT              (*(volatile uint32_t *)(SDR_BASE + 0x10))   
#define SDR_RFSH              (*(volatile uint32_t *)(SDR_BASE + 0x14))   
#define SDR_MODE              (*(volatile uint32_t *)(SDR_BASE + 0x18))   
#define SDR_MOBILE            (*(volatile uint32_t *)(SDR_BASE + 0x1C)) 

#define MCA_BASE              0xB0080000
#define MCA_CTL               (*(volatile uint32_t *)(MCA_BASE + 0x00))

#define ATA_BASE              0xB0090000
#define ATA_CONFIG            (*(volatile uint32_t *)(ATA_BASE + 0x00))
#define ATA_UDMACTL           (*(volatile uint32_t *)(ATA_BASE + 0x04))
#define ATA_DATA              (*(volatile uint32_t *)(ATA_BASE + 0x08))   
#define ATA_FEATURE           (*(volatile uint32_t *)(ATA_BASE + 0x0C))
#define ATA_SECCNT            (*(volatile uint32_t *)(ATA_BASE + 0x10)) 
#define ATA_SECNUM            (*(volatile uint32_t *)(ATA_BASE + 0x14)) 
#define ATA_CLDLOW            (*(volatile uint32_t *)(ATA_BASE + 0x18)) 
#define ATA_CLDHI             (*(volatile uint32_t *)(ATA_BASE + 0x1C))  
#define ATA_HEAD              (*(volatile uint32_t *)(ATA_BASE + 0x20))   
#define ATA_CMD               (*(volatile uint32_t *)(ATA_BASE + 0x24))    
#define ATA_BYTECNT           (*(volatile uint32_t *)(ATA_BASE + 0x28))
#define ATA_FIFOCTL           (*(volatile uint32_t *)(ATA_BASE + 0x2C))
#define ATA_FIFOCFG           (*(volatile uint32_t *)(ATA_BASE + 0x30))
#define ATA_ADDRDEC           (*(volatile uint32_t *)(ATA_BASE + 0x34))
#define ATA_IRQCTL            (*(volatile uint32_t *)(ATA_BASE + 0x38)) 

#define NAND_BASE             0xB00A0000
#define NAND_CTL              (*(volatile uint32_t *)(NAND_BASE + 0x00))       
#define NAND_STATUS           (*(volatile uint32_t *)(NAND_BASE + 0x04))    
#define NAND_FIFOTIM          (*(volatile uint32_t *)(NAND_BASE + 0x08))   
#define NAND_CLKCTL           (*(volatile uint32_t *)(NAND_BASE + 0x0C))    
#define NAND_BYTECNT          (*(volatile uint32_t *)(NAND_BASE + 0x10))   
#define NAND_ADDRLO1234       (*(volatile uint32_t *)(NAND_BASE + 0x14))
#define NAND_ADDRLO56         (*(volatile uint32_t *)(NAND_BASE + 0x18))  
#define NAND_ADDRHI1234       (*(volatile uint32_t *)(NAND_BASE + 0x1C))
#define NAND_ADDRHI56         (*(volatile uint32_t *)(NAND_BASE + 0x20))  
#define NAND_BUF0             (*(volatile uint32_t *)(NAND_BASE + 0x24))      
#define NAND_BUF1             (*(volatile uint32_t *)(NAND_BASE + 0x28))      
#define NAND_CMD              (*(volatile uint32_t *)(NAND_BASE + 0x2C))       
#define NAND_ECCCTL           (*(volatile uint32_t *)(NAND_BASE + 0x30))    
#define NAND_HAMECC0          (*(volatile uint32_t *)(NAND_BASE + 0x34))   
#define NAND_HAMECC1          (*(volatile uint32_t *)(NAND_BASE + 0x38))   
#define NAND_HAMECC2          (*(volatile uint32_t *)(NAND_BASE + 0x3C))   
#define NAND_HAMCEC           (*(volatile uint32_t *)(NAND_BASE + 0x40))    
#define NAND_RSE0             (*(volatile uint32_t *)(NAND_BASE + 0x44))      
#define NAND_RSE1             (*(volatile uint32_t *)(NAND_BASE + 0x48))      
#define NAND_RSE2             (*(volatile uint32_t *)(NAND_BASE + 0x4C))      
#define NAND_RSE3             (*(volatile uint32_t *)(NAND_BASE + 0x50))      
#define NAND_RSPS0            (*(volatile uint32_t *)(NAND_BASE + 0x54))     
#define NAND_RSPS1            (*(volatile uint32_t *)(NAND_BASE + 0x58))     
#define NAND_RSPS2            (*(volatile uint32_t *)(NAND_BASE + 0x5C))     
#define NAND_FIFODATA         (*(volatile uint32_t *)(NAND_BASE + 0x60))  
#define NAND_DEBUG            (*(volatile uint32_t *)(NAND_BASE + 0x70))     

#define SD_BASE               0xB00B0000
#define SD_CTL                (*(volatile uint32_t *)(SD_BASE + 0x00))    
#define SD_CMDRSP             (*(volatile uint32_t *)(SD_BASE + 0x04)) 
#define SD_RW                 (*(volatile uint32_t *)(SD_BASE + 0x08))     
#define SD_FIFOCTL            (*(volatile uint32_t *)(SD_BASE + 0x0C))
#define SD_CMD                (*(volatile uint32_t *)(SD_BASE + 0x10))    
#define SD_ARG                (*(volatile uint32_t *)(SD_BASE + 0x14))    
#define SD_CRC7               (*(volatile uint32_t *)(SD_BASE + 0x18))   
#define SD_RSPBUF0            (*(volatile uint32_t *)(SD_BASE + 0x1C))
#define SD_RSPBUF1            (*(volatile uint32_t *)(SD_BASE + 0x20))
#define SD_RSPBUF2            (*(volatile uint32_t *)(SD_BASE + 0x24))
#define SD_RSPBUF3            (*(volatile uint32_t *)(SD_BASE + 0x28))
#define SD_RSPBUF4            (*(volatile uint32_t *)(SD_BASE + 0x2C))
#define SD_DAT                (*(volatile uint32_t *)(SD_BASE + 0x30))    
#define SD_CLK                (*(volatile uint32_t *)(SD_BASE + 0x34))    
#define SD_BYTECNT            (*(volatile uint32_t *)(SD_BASE + 0x38))

#define MHA_BASE              0xB00C0000
#define MHA_CTL               (*(volatile uint32_t *)(MHA_BASE + 0x00))
#define MHA_CFG               (*(volatile uint32_t *)(MHA_BASE + 0x04))
#define MHA_DCSCL01           (*(volatile uint32_t *)(MHA_BASE + 0x10))
#define MHA_DCSCL23           (*(volatile uint32_t *)(MHA_BASE + 0x14))
#define MHA_DCSCL45           (*(volatile uint32_t *)(MHA_BASE + 0x18))
#define MHA_DCSCL67           (*(volatile uint32_t *)(MHA_BASE + 0x1C))
#define MHA_QSCL              (*(volatile uint32_t *)(MHA_BASE + 0x20))

#define BT_BASE               0xB00D0000
#define BT_MODESEL            (*(volatile uint32_t *)(BT_BASE + 0x00))
#define BT_FIFODAT            (*(volatile uint32_t *)(BT_BASE + 0x04))

/* video Encoder */
#define BT_VEICTL             (*(volatile uint32_t *)(BT_BASE + 0x08))
#define BT_VEIVSEPOF          (*(volatile uint32_t *)(BT_BASE + 0x14))
#define BT_VEIVSEPEF          (*(volatile uint32_t *)(BT_BASE + 0x18))
#define BT_VEIFTP             (*(volatile uint32_t *)(BT_BASE + 0x24))
#define BT_VEIFIFOCTL         (*(volatile uint32_t *)(BT_BASE + 0x30))
                                     
/* Video Decoder */
#define BT_VDICTL             (*(volatile uint32_t *)(BT_BASE + 0x08))
#define BT_VDIHSPOS           (*(volatile uint32_t *)(BT_BASE + 0x0C))
#define BT_VDIHEPOS           (*(volatile uint32_t *)(BT_BASE + 0x10))
#define BT_VDIVSEPOF          (*(volatile uint32_t *)(BT_BASE + 0x14))
#define BT_VDIVSEPEF          (*(volatile uint32_t *)(BT_BASE + 0x18))
#define BT_VDIIRQSTA          (*(volatile uint32_t *)(BT_BASE + 0x28))
#define BT_VDIXYDAT           (*(volatile uint32_t *)(BT_BASE + 0x2C))
#define BT_VDIFIFOCTL         (*(volatile uint32_t *)(BT_BASE + 0x30))
                                     
/* CMOS Sensor Interface */
#define BT_CSICTL             (*(volatile uint32_t *)(BT_BASE + 0x08))
#define BT_CSIHSPOS           (*(volatile uint32_t *)(BT_BASE + 0x0C))
#define BT_CSIHEPOS           (*(volatile uint32_t *)(BT_BASE + 0x10))
#define BT_CSIVSPOS           (*(volatile uint32_t *)(BT_BASE + 0x1C))
#define BT_CSIVEPOS           (*(volatile uint32_t *)(BT_BASE + 0x20))
#define BT_CSIIRQSTA          (*(volatile uint32_t *)(BT_BASE + 0x28))
#define BT_CSIXYDAT           (*(volatile uint32_t *)(BT_BASE + 0x2C))
#define BT_CSIFIFOCTL         (*(volatile uint32_t *)(BT_BASE + 0x30))
                                     
/* TS */
#define  BT_TSICTL            (*(volatile uint32_t *)(BT_BASE + 0x08))
#define  BT_TSIFIFOCTL        (*(volatile uint32_t *)(BT_BASE + 0x30))
                                     
/* Integrated Video Encoder */
#define BT_IVECTL             (*(volatile uint32_t *)(BT_BASE + 0x34))
#define BT_IVEOUTCTL          (*(volatile uint32_t *)(BT_BASE + 0x38))
#define BT_IVECOTCTL          (*(volatile uint32_t *)(BT_BASE + 0x3C))
#define BT_IVEBRGCTL          (*(volatile uint32_t *)(BT_BASE + 0x40))
#define BT_IVECSATCTL         (*(volatile uint32_t *)(BT_BASE + 0x44))
#define BT_IVECBURCTL         (*(volatile uint32_t *)(BT_BASE + 0x48))
#define BT_IVESYNCAMCTL       (*(volatile uint32_t *)(BT_BASE + 0x4C))

#define OTG_BASE              0xB00E0000
#define OTG_OUT0BC            (*(volatile uint8_t *)(OTG_BASE + 0x00)) // ok (byte count?)
#define OTG_IN0BC             (*(volatile uint8_t *)(OTG_BASE + 0x01)) // ok (byte count?)
#define OTG_EP0CS             (*(volatile uint8_t *)(OTG_BASE + 0x02)) // ok
#define EP_NAK                (1<<1) // from rt source
#define EP0_IN_BUSY           (1<<2)
#define EP0_OUT_BUSY          (1<<3)

#define OTG_OUT1CON           (*(volatile uint8_t *)(OTG_BASE + 0x0A)) // ok
#define OTG_OUT1CS            (*(volatile uint8_t *)(OTG_BASE + 0x0B)) // missing in rt

#define OTG_OUT2CON           (*(volatile uint8_t *)(OTG_BASE + 0x12)) // missing in sdk
#define OTG_OUT2CS            (*(volatile uint8_t *)(OTG_BASE + 0x13)) // deduced

#define OTG_IN2BCL            (*(volatile uint8_t *)(OTG_BASE + 0x14)) // missing in rt
#define OTG_IN2BCH            (*(volatile uint8_t *)(OTG_BASE + 0x15)) // missing in rt
#define OTG_IN2CON            (*(volatile uint8_t *)(OTG_BASE + 0x16)) // ok
#define OTG_IN2CS             (*(volatile uint8_t *)(OTG_BASE + 0x17)) // 

#define OTG_FIFO1DAT          (*(volatile uint32_t *)(OTG_BASE + 0x84)) // missing in rt
#define OTG_FIFO2DAT          (*(volatile uint32_t *)(OTG_BASE + 0x88)) // missing in rt

#define OTG_EP0INDAT          (*(volatile uint8_t *)(OTG_BASE + 0x100) // ok

#define OTG_EP0OUTDAT         (*(volatile uint8_t *)(OTG_BASE + 0x140) // ok

#define OTG_SETUPDAT          (*(volatile uint8_t *)(OTG_BASE + 0x180) // ok
#define OTG_USBIRQ            (*(volatile uint8_t *)(OTG_BASE + 0x18C) // ok

#define OTG_USBIEN            (*(volatile uint8_t *)(OTG_BASE + 0x198)) // ok

#define OTG_IVECT             (*(volatile uint8_t *)(OTG_BASE + 0x1A0)) // missing in rt
#define OTG_ENDPRST           (*(volatile uint8_t *)(OTG_BASE + 0x1A2)) // ok
#define OTG_USBCS             (*(volatile uint8_t *)(OTG_BASE + 0x1A3)) // ok
#define SOFT_DISCONN          (1<<6) // set for soft disconnect

#define OTG_FIFOCTL           (*(volatile uint8_t *)(OTG_BASE + 0x1A8)) // ok


#define OTG_OTGIRQ            (*(volatile uint8_t *)(OTG_BASE + 0x1BC)) 
#define OTG_FSMSTAT           (*(volatile uint8_t *)(OTG_BASE + 0x1BD))
#define OTG_CTRL              (*(volatile uint8_t *)(OTG_BASE + 0x1BE))
#define OTG_STAT              (*(volatile uint8_t *)(OTG_BASE + 0x1BF))
#define OTG_OTGIEN            (*(volatile uint8_t *)(OTG_BASE + 0x1C0))
                                    
#define OTG_TAAIDLBDIS        (*(volatile uint8_t *)(OTG_BASE + 0x1C1))
#define OTG_TAWAITBCON        (*(volatile uint8_t *)(OTG_BASE + 0x1C2))
#define OTG_TBVBUSPLS         (*(volatile uint8_t *)(OTG_BASE + 0x1C3))
#define OTG_TBVBUSDISPLS      (*(volatile uint8_t *)(OTG_BASE + 0x1C7))

#define OTG_HCIN1MAXPCKL      (*(volatile uint8_t *)(OTG_BASE + 0x1E2))
#define OTG_HCIN1MAXPCKH      (*(volatile uint8_t *)(OTG_BASE + 0x1E3))

#define OTG_OUT1STADDR        ((*(volatile uint8_t *)(OTG_BASE + 0x304))

#define OTG_IN2STADDR         ((*(volatile uint8_t *)(OTG_BASE + 0x348))

#define OTG_HCOUT2MAXPCKL     ((*(volatile uint8_t *)(OTG_BASE + 0x3E4))
#define OTG_HCOUT2MAXPCKH     ((*(volatile uint8_t *)(OTG_BASE + 0x3E5))

#define OTG_USBEIRQ           ((*(volatile uint8_t *)(OTG_BASE + 0x400))

#define OTG_DMAEPSEL          ((*(volatile uint8_t *)(OTG_BASE + 0x40C))

#define YUV2RGB_BASE          0xB00F0000
#define YUV2RGB_CTL           (*(volatile uint32_t *)(YUV2RGB_BASE + 0x00))
#define YUV2RGB_FIFODATA      (*(volatile uint32_t *)(YUV2RGB_BASE + 0x04))
#define YUV2RGB_CLKCTL        (*(volatile uint32_t *)(YUV2RGB_BASE + 0x08))
#define YUV2RGB_FRAMECOUNT    (*(volatile uint32_t *)(YUV2RGB_BASE + 0x0C))

#define DAC_BASE              0xB0100000
#define DAC_CTL               (*(volatile uint32_t *)(DAC_BASE + 0x00))
#define DAC_FIFOCTL           (*(volatile uint32_t *)(DAC_BASE + 0x04))
#define DAC_DAT               (*(volatile uint32_t *)(DAC_BASE + 0x08))
#define DAC_DEBUG             (*(volatile uint32_t *)(DAC_BASE + 0x0C))
#define DAC_ANALOG            (*(volatile uint32_t *)(DAC_BASE + 0x10))

#define ADC_BASE              0xB0110000
#define ADC_CTL               (*(volatile uint32_t *)(ADC_BASE + 0x00))
#define ADC_FIFOCTL           (*(volatile uint32_t *)(ADC_BASE + 0x04))
#define ADC_DAT               (*(volatile uint32_t *)(ADC_BASE + 0x08))
#define ADC_DEBUG             (*(volatile uint32_t *)(ADC_BASE + 0x0C))
#define ADC_ANALOG            (*(volatile uint32_t *)(ADC_BASE + 0x10))

#define TP_BASE               0xB0120000
#define TP_CTL                (*(volatile uint32_t *)(TP_BASE + 0x00))
#define TP_DAT                (*(volatile uint32_t *)(TP_BASE + 0x04))

#define SPDIF_BASE            0xB0140000
#define SPDIF_CTL             (*(volatile uint32_t *)(SPDIF_BASE + 0x00))
#define SPDIF_STAT            (*(volatile uint32_t *)(SPDIF_BASE + 0x04))
#define SPDIF_TXDAT           (*(volatile uint32_t *)(SPDIF_BASE + 0x08))
#define SPDIF_RXDAT           (*(volatile uint32_t *)(SPDIF_BASE + 0x0C))
#define SPDIF_TXCSTAT         (*(volatile uint32_t *)(SPDIF_BASE + 0x10))
#define SPDIF_RXCSTAT         (*(volatile uint32_t *)(SPDIF_BASE + 0x14))

#define PCM_BASE              0xB0150000
#define PCM_CTL               (*(volatile uint32_t *)(PCM_BASE + 0x00))
#define PCM_STAT              (*(volatile uint32_t *)(PCM_BASE + 0x04))
#define PCM_RXDAT             (*(volatile uint32_t *)(PCM_BASE + 0x08))
#define PCM_TXDAT             (*(volatile uint32_t *)(PCM_BASE + 0x0C))

/* n = 0,1 */
#define UART_BASE(n)          (0xB0160000 + (n<<5))
#define UART_CTL(n)           (*(volatile uint32_t *)(UART_BASE(n) + 0x00))
#define UART_RXDAT(n)         (*(volatile uint32_t *)(UART_BASE(n) + 0x04))
#define UART_TXDAT(n)         (*(volatile uint32_t *)(UART_BASE(n) + 0x08))
#define UART_STAT(n)          (*(volatile uint32_t *)(UART_BASE(n) + 0x0C))

#define IR_PL                 (*(volatile uint32_t *)(UART_BASE(0) + 0x10))
#define IR_RBC                (*(volatile uint32_t *)(UART_BASE(0) + 0x14))

/* n = 0,1 */
#define I2C_BASE(n)           (0xB0180000 + (n<<5))
#define I2C_CTL(n)            (*(volatile uint32_t *)(I2C_BASE(n) + 0x00))
#define I2C_CLKDIV(n)         (*(volatile uint32_t *)(I2C_BASE(n) + 0x04))
#define I2C_STAT(n)           (*(volatile uint32_t *)(I2C_BASE(n) + 0x08))
#define I2C_ADDR(n)           (*(volatile uint32_t *)(I2C_BASE(n) + 0x0C))
#define I2C_DAT(n)            (*(volatile uint32_t *)(I2C_BASE(n) + 0x10))

#define SPI_BASE              0xB0190000
#define SPI_CTL               (*(volatile uint32_t *)(SPI_BASE + 0x00))
#define SPI_CLKDIV            (*(volatile uint32_t *)(SPI_BASE + 0x04))
#define SPI_STAT              (*(volatile uint32_t *)(SPI_BASE + 0x08))
#define SPI_RXDAT             (*(volatile uint32_t *)(SPI_BASE + 0x0C))
#define SPI_TXDAT             (*(volatile uint32_t *)(SPI_BASE + 0x10))

#define KEY_BASE              0xB01A0000
#define KEY_CTL               (*(volatile uint32_t *)(KEY_BASE + 0x00))
#define KEY_DAT0              (*(volatile uint32_t *)(KEY_BASE + 0x04))
#define KEY_DAT1              (*(volatile uint32_t *)(KEY_BASE + 0x08))
#define KEY_DAT2              (*(volatile uint32_t *)(KEY_BASE + 0x0C))
#define KEY_DAT3              (*(volatile uint32_t *)(KEY_BASE + 0x10))

#define GPIO_BASE             0xB01C0000
#define GPIO_AOUTEN           (*(volatile uint32_t *)(GPIO_BASE + 0x00))
#define GPIO_AINEN            (*(volatile uint32_t *)(GPIO_BASE + 0x04))
#define GPIO_ADAT             (*(volatile uint32_t *)(GPIO_BASE + 0x08))
#define GPIO_BOUTEN           (*(volatile uint32_t *)(GPIO_BASE + 0x0C))
#define GPIO_BINEN            (*(volatile uint32_t *)(GPIO_BASE + 0x10))
#define GPIO_BDAT             (*(volatile uint32_t *)(GPIO_BASE + 0x14))
#define GPIO_MFCTL0           (*(volatile uint32_t *)(GPIO_BASE + 0x18))
#define GPIO_MFCTL1           (*(volatile uint32_t *)(GPIO_BASE + 0x1C))
