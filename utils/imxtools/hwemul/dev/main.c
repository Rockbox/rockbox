#include "stddef.h"
#include "protocol.h"
#include "logf.h"
#include "usb_ch9.h"

extern unsigned char oc_codestart[];
extern unsigned char oc_codeend[];
extern unsigned char oc_stackstart[];
extern unsigned char oc_stackend[];
extern unsigned char oc_bufferstart[];
extern unsigned char oc_bufferend[];

#define oc_codesize ((size_t)(oc_codeend - oc_codestart))
#define oc_stacksize ((size_t)(oc_stackend - oc_stackstart))
#define oc_buffersize ((size_t)(oc_bufferend - oc_bufferstart))

/**
 *
 * Common
 *
 */
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define __REG_SET(reg)  (*((volatile uint32_t *)(&reg + 1)))
#define __REG_CLR(reg)  (*((volatile uint32_t *)(&reg + 2)))
#define __REG_TOG(reg)  (*((volatile uint32_t *)(&reg + 3)))

#define __BLOCK_SFTRST  (1 << 31)
#define __BLOCK_CLKGATE (1 << 30)

#define __XTRACT(reg, field)    ((reg & reg##__##field##_BM) >> reg##__##field##_BP)
#define __XTRACT_EX(val, field)    (((val) & field##_BM) >> field##_BP)
#define __FIELD_SET(reg, field, val) reg = (reg & ~reg##__##field##_BM) | (val << reg##__##field##_BP)

/**
 *
 * Pin control
 *
 */

#define HW_PINCTRL_BASE         0x80018000

#define HW_PINCTRL_CTRL         (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x0))
#define HW_PINCTRL_MUXSEL(i)    (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x100 + (i) * 0x10))
#define HW_PINCTRL_DRIVE(i)     (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x200 + (i) * 0x10))
#ifdef HAVE_STMP3700
#define HW_PINCTRL_PULL(i)      (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x300 + (i) * 0x10))
#define HW_PINCTRL_DOUT(i)      (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x400 + (i) * 0x10))
#define HW_PINCTRL_DIN(i)       (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x500 + (i) * 0x10))
#define HW_PINCTRL_DOE(i)       (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x600 + (i) * 0x10))
#define HW_PINCTRL_PIN2IRQ(i)   (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x700 + (i) * 0x10))
#define HW_PINCTRL_IRQEN(i)     (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x800 + (i) * 0x10))
#define HW_PINCTRL_IRQLEVEL(i)  (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x900 + (i) * 0x10))
#define HW_PINCTRL_IRQPOL(i)    (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0xa00 + (i) * 0x10))
#define HW_PINCTRL_IRQSTAT(i)   (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0xb00 + (i) * 0x10))
#else
#define HW_PINCTRL_PULL(i)      (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x400 + (i) * 0x10))
#define HW_PINCTRL_DOUT(i)      (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x500 + (i) * 0x10))
#define HW_PINCTRL_DIN(i)       (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x600 + (i) * 0x10))
#define HW_PINCTRL_DOE(i)       (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x700 + (i) * 0x10))
#define HW_PINCTRL_PIN2IRQ(i)   (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x800 + (i) * 0x10))
#define HW_PINCTRL_IRQEN(i)     (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0x900 + (i) * 0x10))
#define HW_PINCTRL_IRQLEVEL(i)  (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0xa00 + (i) * 0x10))
#define HW_PINCTRL_IRQPOL(i)    (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0xb00 + (i) * 0x10))
#define HW_PINCTRL_IRQSTAT(i)   (*(volatile uint32_t *)(HW_PINCTRL_BASE + 0xc00 + (i) * 0x10))
#endif

#define PINCTRL_FUNCTION_MAIN   0
#define PINCTRL_FUNCTION_ALT1   1
#define PINCTRL_FUNCTION_ALT2   2
#define PINCTRL_FUNCTION_GPIO   3

#define PINCTRL_DRIVE_4mA       0
#define PINCTRL_DRIVE_8mA       1
#define PINCTRL_DRIVE_12mA      2
#define PINCTRL_DRIVE_16mA      3 /* not available on all pins */

typedef void (*pin_irq_cb_t)(int bank, int pin);

static inline void imx233_pinctrl_init(void)
{
    __REG_CLR(HW_PINCTRL_CTRL) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
}

static inline void imx233_set_pin_drive_strength(unsigned bank, unsigned pin, unsigned strength)
{
    __REG_CLR(HW_PINCTRL_DRIVE(4 * bank + pin / 8)) = 3 << (4 * (pin % 8));
    __REG_SET(HW_PINCTRL_DRIVE(4 * bank + pin / 8)) = strength << (4 * (pin % 8));
}

static inline void imx233_enable_gpio_output(unsigned bank, unsigned pin, bool enable)
{
    if(enable)
        __REG_SET(HW_PINCTRL_DOE(bank)) = 1 << pin;
    else
        __REG_CLR(HW_PINCTRL_DOE(bank)) = 1 << pin;
}

static inline void imx233_enable_gpio_output_mask(unsigned bank, uint32_t pin_mask, bool enable)
{
    if(enable)
        __REG_SET(HW_PINCTRL_DOE(bank)) = pin_mask;
    else
        __REG_CLR(HW_PINCTRL_DOE(bank)) = pin_mask;
}

static inline void imx233_set_gpio_output(unsigned bank, unsigned pin, bool value)
{
    if(value)
        __REG_SET(HW_PINCTRL_DOUT(bank)) = 1 << pin;
    else
        __REG_CLR(HW_PINCTRL_DOUT(bank)) = 1 << pin;
}

static inline void imx233_set_gpio_output_mask(unsigned bank, uint32_t pin_mask, bool value)
{
    if(value)
        __REG_SET(HW_PINCTRL_DOUT(bank)) = pin_mask;
    else
        __REG_CLR(HW_PINCTRL_DOUT(bank)) = pin_mask;
}

static inline uint32_t imx233_get_gpio_input_mask(unsigned bank, uint32_t pin_mask)
{
    return HW_PINCTRL_DIN(bank) & pin_mask;
}

static inline void imx233_set_pin_function(unsigned bank, unsigned pin, unsigned function)
{
    __REG_CLR(HW_PINCTRL_MUXSEL(2 * bank + pin / 16)) = 3 << (2 * (pin % 16));
    __REG_SET(HW_PINCTRL_MUXSEL(2 * bank + pin / 16)) = function << (2 * (pin % 16));
}

static inline void imx233_enable_pin_pullup(unsigned bank, unsigned pin, bool enable)
{
    if(enable)
        __REG_SET(HW_PINCTRL_PULL(bank)) = 1 << pin;
    else
        __REG_CLR(HW_PINCTRL_PULL(bank)) = 1 << pin;
}

static inline void imx233_enable_pin_pullup_mask(unsigned bank, uint32_t pin_msk, bool enable)
{
    if(enable)
        __REG_SET(HW_PINCTRL_PULL(bank)) = pin_msk;
    else
        __REG_CLR(HW_PINCTRL_PULL(bank)) = pin_msk;
}

/**
 *
 * USB subsystem
 *
 */

#define USB_BASE            0x80080000
#define USB_NUM_ENDPOINTS   2
#define MAX_PKT_SIZE        1024
#define MAX_PKT_SIZE_EP0    64

/* USB device mode registers (Little Endian) */
#define REG_USBCMD           (*(volatile unsigned int *)(USB_BASE+0x140))
#define REG_DEVICEADDR       (*(volatile unsigned int *)(USB_BASE+0x154))
#define REG_ENDPOINTLISTADDR (*(volatile unsigned int *)(USB_BASE+0x158))
#define REG_PORTSC1          (*(volatile unsigned int *)(USB_BASE+0x184))
#define REG_USBMODE          (*(volatile unsigned int *)(USB_BASE+0x1a8))
#define REG_ENDPTSETUPSTAT   (*(volatile unsigned int *)(USB_BASE+0x1ac))
#define REG_ENDPTPRIME       (*(volatile unsigned int *)(USB_BASE+0x1b0))
#define REG_ENDPTSTATUS      (*(volatile unsigned int *)(USB_BASE+0x1b8))
#define REG_ENDPTCOMPLETE    (*(volatile unsigned int *)(USB_BASE+0x1bc))
#define REG_ENDPTCTRL0       (*(volatile unsigned int *)(USB_BASE+0x1c0))
#define REG_ENDPTCTRL1       (*(volatile unsigned int *)(USB_BASE+0x1c4))
#define REG_ENDPTCTRL2       (*(volatile unsigned int *)(USB_BASE+0x1c8))
#define REG_ENDPTCTRL(_x_)   (*(volatile unsigned int *)(USB_BASE+0x1c0+4*(_x_)))

/* USB CMD  Register Bit Masks */
#define USBCMD_RUN                            (0x00000001)
#define USBCMD_CTRL_RESET                     (0x00000002)
#define USBCMD_PERIODIC_SCHEDULE_EN           (0x00000010)
#define USBCMD_ASYNC_SCHEDULE_EN              (0x00000020)
#define USBCMD_INT_AA_DOORBELL                (0x00000040)
#define USBCMD_ASP                            (0x00000300)
#define USBCMD_ASYNC_SCH_PARK_EN              (0x00000800)
#define USBCMD_SUTW                           (0x00002000)
#define USBCMD_ATDTW                          (0x00004000)
#define USBCMD_ITC                            (0x00FF0000)

/* Device Address bit masks */
#define USBDEVICEADDRESS_MASK                 (0xFE000000)
#define USBDEVICEADDRESS_BIT_POS              (25)

/* Endpoint Setup Status bit masks */
#define EPSETUP_STATUS_EP0                    (0x00000001)

/* PORTSCX  Register Bit Masks */
#define PORTSCX_CURRENT_CONNECT_STATUS         (0x00000001)
#define PORTSCX_CONNECT_STATUS_CHANGE          (0x00000002)
#define PORTSCX_PORT_ENABLE                    (0x00000004)
#define PORTSCX_PORT_EN_DIS_CHANGE             (0x00000008)
#define PORTSCX_OVER_CURRENT_ACT               (0x00000010)
#define PORTSCX_OVER_CURRENT_CHG               (0x00000020)
#define PORTSCX_PORT_FORCE_RESUME              (0x00000040)
#define PORTSCX_PORT_SUSPEND                   (0x00000080)
#define PORTSCX_PORT_RESET                     (0x00000100)
#define PORTSCX_LINE_STATUS_BITS               (0x00000C00)
#define PORTSCX_PORT_POWER                     (0x00001000)
#define PORTSCX_PORT_INDICTOR_CTRL             (0x0000C000)
#define PORTSCX_PORT_TEST_CTRL                 (0x000F0000)
#define PORTSCX_WAKE_ON_CONNECT_EN             (0x00100000)
#define PORTSCX_WAKE_ON_CONNECT_DIS            (0x00200000)
#define PORTSCX_WAKE_ON_OVER_CURRENT           (0x00400000)
#define PORTSCX_PHY_LOW_POWER_SPD              (0x00800000)
#define PORTSCX_PORT_FORCE_FULL_SPEED          (0x01000000)
#define PORTSCX_PORT_SPEED_MASK                (0x0C000000)
#define PORTSCX_PORT_WIDTH                     (0x10000000)
#define PORTSCX_PHY_TYPE_SEL                   (0xC0000000)

/* bit 11-10 are line status */
#define PORTSCX_LINE_STATUS_SE0                (0x00000000)
#define PORTSCX_LINE_STATUS_JSTATE             (0x00000400)
#define PORTSCX_LINE_STATUS_KSTATE             (0x00000800)
#define PORTSCX_LINE_STATUS_UNDEF              (0x00000C00)
#define PORTSCX_LINE_STATUS_BIT_POS            (10)

/* bit 15-14 are port indicator control */
#define PORTSCX_PIC_OFF                        (0x00000000)
#define PORTSCX_PIC_AMBER                      (0x00004000)
#define PORTSCX_PIC_GREEN                      (0x00008000)
#define PORTSCX_PIC_UNDEF                      (0x0000C000)
#define PORTSCX_PIC_BIT_POS                    (14)

/* bit 19-16 are port test control */
#define PORTSCX_PTC_DISABLE                    (0x00000000)
#define PORTSCX_PTC_JSTATE                     (0x00010000)
#define PORTSCX_PTC_KSTATE                     (0x00020000)
#define PORTSCX_PTC_SE0NAK                     (0x00030000)
#define PORTSCX_PTC_PACKET                     (0x00040000)
#define PORTSCX_PTC_FORCE_EN                   (0x00050000)
#define PORTSCX_PTC_BIT_POS                    (16)

/* bit 27-26 are port speed */
#define PORTSCX_PORT_SPEED_FULL                (0x00000000)
#define PORTSCX_PORT_SPEED_LOW                 (0x04000000)
#define PORTSCX_PORT_SPEED_HIGH                (0x08000000)
#define PORTSCX_PORT_SPEED_UNDEF               (0x0C000000)
#define PORTSCX_SPEED_BIT_POS                  (26)

/* bit 28 is parallel transceiver width for UTMI interface */
#define PORTSCX_PTW                            (0x10000000)
#define PORTSCX_PTW_8BIT                       (0x00000000)
#define PORTSCX_PTW_16BIT                      (0x10000000)

/* bit 31-30 are port transceiver select */
#define PORTSCX_PTS_UTMI                       (0x00000000)
#define PORTSCX_PTS_CLASSIC                    (0x40000000)
#define PORTSCX_PTS_ULPI                       (0x80000000)
#define PORTSCX_PTS_FSLS                       (0xC0000000)
#define PORTSCX_PTS_BIT_POS                    (30)

/* USB MODE Register Bit Masks */
#define USBMODE_CTRL_MODE_IDLE                (0x00000000)
#define USBMODE_CTRL_MODE_DEVICE              (0x00000002)
#define USBMODE_CTRL_MODE_HOST                (0x00000003)
#define USBMODE_CTRL_MODE_RSV                 (0x00000001)
#define USBMODE_SETUP_LOCK_OFF                (0x00000008)
#define USBMODE_STREAM_DISABLE                (0x00000010)

/* ENDPOINTCTRLx  Register Bit Masks */
#define EPCTRL_TX_ENABLE                       (0x00800000)
#define EPCTRL_TX_DATA_TOGGLE_RST              (0x00400000)    /* Not EP0 */
#define EPCTRL_TX_DATA_TOGGLE_INH              (0x00200000)    /* Not EP0 */
#define EPCTRL_TX_TYPE                         (0x000C0000)
#define EPCTRL_TX_DATA_SOURCE                  (0x00020000)    /* Not EP0 */
#define EPCTRL_TX_EP_STALL                     (0x00010000)
#define EPCTRL_RX_ENABLE                       (0x00000080)
#define EPCTRL_RX_DATA_TOGGLE_RST              (0x00000040)    /* Not EP0 */
#define EPCTRL_RX_DATA_TOGGLE_INH              (0x00000020)    /* Not EP0 */
#define EPCTRL_RX_TYPE                         (0x0000000C)
#define EPCTRL_RX_DATA_SINK                    (0x00000002)    /* Not EP0 */
#define EPCTRL_RX_EP_STALL                     (0x00000001)

/* bit 19-18 and 3-2 are endpoint type */
#define EPCTRL_TX_EP_TYPE_SHIFT                (18)
#define EPCTRL_RX_EP_TYPE_SHIFT                (2)

#define QH_MULT_POS                            (30)
#define QH_ZLT_SEL                             (0x20000000)
#define QH_MAX_PKT_LEN_POS                     (16)
#define QH_IOS                                 (0x00008000)
#define QH_NEXT_TERMINATE                      (0x00000001)
#define QH_IOC                                 (0x00008000)
#define QH_MULTO                               (0x00000C00)
#define QH_STATUS_HALT                         (0x00000040)
#define QH_STATUS_ACTIVE                       (0x00000080)
#define EP_QUEUE_CURRENT_OFFSET_MASK         (0x00000FFF)
#define EP_QUEUE_HEAD_NEXT_POINTER_MASK      (0xFFFFFFE0)
#define EP_QUEUE_FRINDEX_MASK                (0x000007FF)
#define EP_MAX_LENGTH_TRANSFER               (0x4000)

#define DTD_NEXT_TERMINATE                   (0x00000001)
#define DTD_IOC                              (0x00008000)
#define DTD_STATUS_ACTIVE                    (0x00000080)
#define DTD_STATUS_HALTED                    (0x00000040)
#define DTD_STATUS_DATA_BUFF_ERR             (0x00000020)
#define DTD_STATUS_TRANSACTION_ERR           (0x00000008)
#define DTD_RESERVED_FIELDS                  (0x80007300)
#define DTD_ADDR_MASK                        (0xFFFFFFE0)
#define DTD_PACKET_SIZE                      (0x7FFF0000)
#define DTD_LENGTH_BIT_POS                   (16)
#define DTD_ERROR_MASK                       (DTD_STATUS_HALTED | \
                                               DTD_STATUS_DATA_BUFF_ERR | \
                                               DTD_STATUS_TRANSACTION_ERR)
/*-------------------------------------------------------------------------*/
/* manual: 32.13.2 Endpoint Transfer Descriptor (dTD) */
struct transfer_descriptor {
    unsigned int next_td_ptr;           /* Next TD pointer(31-5), T(0) set
                                           indicate invalid */
    unsigned int size_ioc_sts;          /* Total bytes (30-16), IOC (15),
                                           MultO(11-10), STS (7-0)  */
    unsigned int buff_ptr0;             /* Buffer pointer Page 0 */
    unsigned int buff_ptr1;             /* Buffer pointer Page 1 */
    unsigned int buff_ptr2;             /* Buffer pointer Page 2 */
    unsigned int buff_ptr3;             /* Buffer pointer Page 3 */
    unsigned int buff_ptr4;             /* Buffer pointer Page 4 */
    unsigned int reserved;
} __attribute__ ((packed));

static struct transfer_descriptor td_array[USB_NUM_ENDPOINTS*2]
    __attribute__((aligned(32)));

/* manual: 32.13.1 Endpoint Queue Head (dQH) */
struct queue_head {
    unsigned int max_pkt_length;    /* Mult(31-30) , Zlt(29) , Max Pkt len
                                       and IOS(15) */
    unsigned int curr_dtd_ptr;      /* Current dTD Pointer(31-5) */
    struct transfer_descriptor dtd; /* dTD overlay */
    unsigned int setup_buffer[2];   /* Setup data 8 bytes */
    unsigned int reserved;          /* for software use, pointer to the first TD */
    unsigned int status;            /* for software use, status of chain in progress */
    unsigned int length;            /* for software use, transfered bytes of chain in progress */
    unsigned int wait;              /* for softwate use, indicates if the transfer is blocking */
} __attribute__((packed));

static struct queue_head qh_array[USB_NUM_ENDPOINTS*2] __attribute__((aligned(2048)));

static const unsigned int pipe2mask[] = {
    0x01, 0x010000,
    0x02, 0x020000,
    0x04, 0x040000,
    0x08, 0x080000,
    0x10, 0x100000,
};

/* return transfered size if wait=true */
static int prime_transfer(int ep_num, void *ptr, int len, bool send, bool wait)
{
    int pipe = ep_num * 2 + (send ? 1 : 0);
    unsigned mask = pipe2mask[pipe];
    struct transfer_descriptor *td = &td_array[pipe];
    struct queue_head* qh = &qh_array[pipe];
    
    /* prepare TD */
    td->next_td_ptr = DTD_NEXT_TERMINATE;
    td->size_ioc_sts = (len<< DTD_LENGTH_BIT_POS) | DTD_STATUS_ACTIVE;
    td->buff_ptr0 = (unsigned int)ptr;
    td->buff_ptr1 = ((unsigned int)ptr & 0xfffff000) + 0x1000;
    td->buff_ptr2 = ((unsigned int)ptr & 0xfffff000) + 0x2000;
    td->buff_ptr3 = ((unsigned int)ptr & 0xfffff000) + 0x3000;
    td->buff_ptr4 = ((unsigned int)ptr & 0xfffff000) + 0x4000;
    td->reserved = 0;
    /* prime */
    qh->dtd.next_td_ptr = (unsigned int)td;
    qh->dtd.size_ioc_sts &= ~(QH_STATUS_HALT | QH_STATUS_ACTIVE);
    REG_ENDPTPRIME |= mask;
    /* wait for priming to be taken into account */
    while(!(REG_ENDPTSTATUS & mask));
    /* wait for completion */
    if(wait)
    {
        while(!(REG_ENDPTCOMPLETE & mask));
        REG_ENDPTCOMPLETE = mask;
        /* memory barrier */
        asm volatile("":::"memory");
        /* return transfered size */
        return len - (td->size_ioc_sts >> DTD_LENGTH_BIT_POS);
    }
    else
        return 0;
}

void usb_drv_set_address(int address)
{
    REG_DEVICEADDR = address << USBDEVICEADDRESS_BIT_POS;
}

/* endpoints */
#define EP_CONTROL 0

#define DIR_OUT 0
#define DIR_IN 1

#define EP_DIR(ep) (((ep) & USB_ENDPOINT_DIR_MASK) ? DIR_IN : DIR_OUT)
#define EP_NUM(ep) ((ep) & USB_ENDPOINT_NUMBER_MASK)

static int usb_drv_send_nonblocking(int endpoint, void* ptr, int length)
{
    return prime_transfer(EP_NUM(endpoint), ptr, length, true, false);
}

static int usb_drv_send(int endpoint, void* ptr, int length)
{
    return prime_transfer(EP_NUM(endpoint), ptr, length, true, true);
}

static int usb_drv_recv(int endpoint, void* ptr, int length)
{
    return prime_transfer(EP_NUM(endpoint), ptr, length, false, true);
}

static int usb_drv_recv_nonblocking(int endpoint, void* ptr, int length)
{
    return prime_transfer(EP_NUM(endpoint), ptr, length, false, false);
}

static int usb_drv_port_speed(void)
{
    return (REG_PORTSC1 & 0x08000000) ? 1 : 0;
}

static void usb_drv_stall(int endpoint, bool stall, bool in)
{
    int ep_num = EP_NUM(endpoint);

    if(in)
    {
        if(stall)
            REG_ENDPTCTRL(ep_num) |= EPCTRL_TX_EP_STALL;
        else
            REG_ENDPTCTRL(ep_num) &= ~EPCTRL_TX_EP_STALL;
    }
    else
    {
        if (stall)
            REG_ENDPTCTRL(ep_num) |= EPCTRL_RX_EP_STALL;
        else
            REG_ENDPTCTRL(ep_num) &= ~EPCTRL_RX_EP_STALL;
    }
}

static void usb_drv_configure_endpoint(int ep_num, int type)
{
    REG_ENDPTCTRL(ep_num) =
        EPCTRL_RX_DATA_TOGGLE_RST | EPCTRL_RX_ENABLE |
        EPCTRL_TX_DATA_TOGGLE_RST | EPCTRL_TX_ENABLE |
            (type << EPCTRL_RX_EP_TYPE_SHIFT) |
            (type << EPCTRL_TX_EP_TYPE_SHIFT);
}

/**
 *
 * Clock control
 *
 **/
#define __CLK_CLKGATE   (1 << 31)
#define __CLK_BUSY      (1 << 29)

#define HW_CLKCTRL_BASE     0x80040000

#define HW_CLKCTRL_PLLCTRL0 (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x0))
#define HW_CLKCTRL_PLLCTRL0__POWER          (1 << 16)
#define HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS    (1 << 18)
#define HW_CLKCTRL_PLLCTRL0__DIV_SEL_BP     20
#define HW_CLKCTRL_PLLCTRL0__DIV_SEL_BM     (3 << 20)

#define HW_CLKCTRL_PLLCTRL1 (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x10))

#define HW_CLKCTRL_CPU      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x20))
#define HW_CLKCTRL_CPU__DIV_CPU_BP  0
#define HW_CLKCTRL_CPU__DIV_CPU_BM  0x3f
#define HW_CLKCTRL_CPU__INTERRUPT_WAIT  (1 << 12)
#define HW_CLKCTRL_CPU__DIV_XTAL_BP 16
#define HW_CLKCTRL_CPU__DIV_XTAL_BM (0x3ff << 16)
#define HW_CLKCTRL_CPU__DIV_XTAL_FRAC_EN    (1 << 26)
#define HW_CLKCTRL_CPU__BUSY_REF_CPU    (1 << 28)

#define HW_CLKCTRL_HBUS     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x30))
#define HW_CLKCTRL_HBUS__DIV_BP         0
#define HW_CLKCTRL_HBUS__DIV_BM         0x1f
#define HW_CLKCTRL_HBUS__DIV_FRAC_EN    (1 << 5)
#define HW_CLKCTRL_HBUS__SLOW_DIV_BP    16
#define HW_CLKCTRL_HBUS__SLOW_DIV_BM    (0x7 << 16)
#define HW_CLKCTRL_HBUS__AUTO_SLOW_MODE (1 << 20)

#define HW_CLKCTRL_XBUS     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x40))
#define HW_CLKCTRL_XBUS__DIV_BP     0
#define HW_CLKCTRL_XBUS__DIV_BM     0x3ff
#define HW_CLKCTRL_XBUS__BUSY       (1 << 31)

#define HW_CLKCTRL_XTAL     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x50))
#define HW_CLKCTRL_XTAL__TIMROT_CLK32K_GATE (1 << 26)
#define HW_CLKCTRL_XTAL__DRI_CLK24M_GATE    (1 << 28)
#define HW_CLKCTRL_XTAL__PWM_CLK24M_GATE    (1 << 29)
#define HW_CLKCTRL_XTAL__FILT_CLK24M_GATE   (1 << 30)

#define HW_CLKCTRL_PIX      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x60))
#define HW_CLKCTRL_PIX__DIV_BP  0
#define HW_CLKCTRL_PIX__DIV_BM  0xfff

#define HW_CLKCTRL_SSP      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x70))
#define HW_CLKCTRL_SSP__DIV_BP  0
#define HW_CLKCTRL_SSP__DIV_BM  0x1ff

#define HW_CLKCTRL_EMI      (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0xa0))
#define HW_CLKCTRL_EMI__DIV_EMI_BP  0
#define HW_CLKCTRL_EMI__DIV_EMI_BM  0x3f
#define HW_CLKCTRL_EMI__DIV_XTAL_BP 8
#define HW_CLKCTRL_EMI__DIV_XTAL_BM (0xf << 8)
#define HW_CLKCTRL_EMI__BUSY_REF_EMI    (1 << 28)
#define HW_CLKCTRL_EMI__SYNC_MODE_EN    (1 << 30)
#define HW_CLKCTRL_EMI__CLKGATE     (1 << 31)

#ifdef HAVE_STMP3770
#define HW_CLKCTRL_CLKSEQ   (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0xe0))
#else
#define HW_CLKCTRL_CLKSEQ   (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x110))
#endif
#define HW_CLKCTRL_CLKSEQ__BYPASS_PIX   (1 << 1)
#define HW_CLKCTRL_CLKSEQ__BYPASS_SSP   (1 << 5)
#define HW_CLKCTRL_CLKSEQ__BYPASS_EMI   (1 << 6)
#define HW_CLKCTRL_CLKSEQ__BYPASS_CPU   (1 << 7)

#ifdef HAVE_STMP3770
#define HW_CLKCTRL_FRAC     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0xd0))
#else
#define HW_CLKCTRL_FRAC     (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0xf0))
#endif
#define HW_CLKCTRL_FRAC_CPU (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf0))
#define HW_CLKCTRL_FRAC_EMI (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf1))
#define HW_CLKCTRL_FRAC_PIX (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf2))
#define HW_CLKCTRL_FRAC_IO  (*(volatile uint8_t *)(HW_CLKCTRL_BASE + 0xf3))
#define HW_CLKCTRL_FRAC_XX__XXDIV_BM    0x3f
#define HW_CLKCTRL_FRAC_XX__XX_STABLE   (1 << 6)
#define HW_CLKCTRL_FRAC_XX__CLKGATEXX   (1 << 7)

#define HW_CLKCTRL_RESET    (*(volatile uint32_t *)(HW_CLKCTRL_BASE + 0x120))
#define HW_CLKCTRL_RESET_CHIP   0x2
#define HW_CLKCTRL_RESET_DIG    0x1

/**
 *
 * DMA
 *
 */

/********
 * APHB *
 ********/

#define HW_APBH_BASE        0x80004000

/* APHB channels */
#define HW_APBH_SSP(ssp)    ssp

#define HW_APBH_CTRL0       (*(volatile uint32_t *)(HW_APBH_BASE + 0x0))
#define HW_APBH_CTRL0__FREEZE_CHANNEL(i)    (1 << (i))
#define HW_APBH_CTRL0__CLKGATE_CHANNEL(i)   (1 << ((i) + 8))
#define HW_APBH_CTRL0__RESET_CHANNEL(i)     (1 << ((i) + 16))
#define HW_APBH_CTRL0__APB_BURST4_EN        (1 << 28)
#define HW_APBH_CTRL0__APB_BURST8_EN        (1 << 29)

#define HW_APBH_CTRL1       (*(volatile uint32_t *)(HW_APBH_BASE + 0x10))
#define HW_APBH_CTRL1__CHx_CMDCMPLT_IRQ(i)      (1 << (i))
#define HW_APBH_CTRL1__CHx_CMDCMPLT_IRQ_EN(i)   (1 << ((i) + 16))

#define HW_APBH_CTRL2       (*(volatile uint32_t *)(HW_APBH_BASE + 0x20))
#define HW_APBH_CTRL2__CHx_ERROR_IRQ(i)         (1 << (i))
#define HW_APBH_CTRL2__CHx_ERROR_STATUS(i)      (1 << ((i) + 16))

#define HW_APBH_CHx_CURCMDAR(i) (*(volatile uint32_t *)(HW_APBH_BASE + 0x40 + 0x70 * (i)))

#define HW_APBH_CHx_NXTCMDAR(i) (*(volatile uint32_t *)(HW_APBH_BASE + 0x50 + 0x70 * (i)))

#define HW_APBH_CHx_CMD(i)      (*(volatile uint32_t *)(HW_APBH_BASE + 0x60 + 0x70 * (i)))

#define HW_APBH_CHx_BAR(i)      (*(volatile uint32_t *)(HW_APBH_BASE + 0x70 + 0x70 * (i)))

#define HW_APBH_CHx_SEMA(i)     (*(volatile uint32_t *)(HW_APBH_BASE + 0x80 + 0x70 * (i)))

#define HW_APBH_CHx_DEBUG1(i)   (*(volatile uint32_t *)(HW_APBH_BASE + 0x90 + 0x70 * (i)))

#define HW_APBH_CHx_DEBUG2(i)   (*(volatile uint32_t *)(HW_APBH_BASE + 0xa0 + 0x70 * (i)))
#define HW_APBH_CHx_DEBUG2__AHB_BYTES_BP    0
#define HW_APBH_CHx_DEBUG2__AHB_BYTES_BM    0xffff
#define HW_APBH_CHx_DEBUG2__APB_BYTES_BP    16
#define HW_APBH_CHx_DEBUG2__APB_BYTES_BM    0xffff0000

/********
 * APHX *
 ********/

/* APHX channels */
#define HW_APBX_AUDIO_ADC   0
#define HW_APBX_AUDIO_DAC   1
#define HW_APBX_I2C         3

#define HW_APBX_BASE        0x80024000

#define HW_APBX_CTRL0       (*(volatile uint32_t *)(HW_APBX_BASE + 0x0))

#define HW_APBX_CTRL1       (*(volatile uint32_t *)(HW_APBX_BASE + 0x10))
#define HW_APBX_CTRL1__CHx_CMDCMPLT_IRQ(i)      (1 << (i))
#define HW_APBX_CTRL1__CHx_CMDCMPLT_IRQ_EN(i)   (1 << ((i) + 16))

#define HW_APBX_CTRL2       (*(volatile uint32_t *)(HW_APBX_BASE + 0x20))
#define HW_APBX_CTRL2__CHx_ERROR_IRQ(i)         (1 << (i))
#define HW_APBX_CTRL2__CHx_ERROR_STATUS(i)      (1 << ((i) + 16))

#define HW_APBX_CHANNEL_CTRL    (*(volatile uint32_t *)(HW_APBX_BASE + 0x30))
#define HW_APBX_CHANNEL_CTRL__FREEZE_CHANNEL(i) (1 << (i))
#define HW_APBX_CHANNEL_CTRL__RESET_CHANNEL(i)  (1 << ((i) + 16))

#define HW_APBX_CHx_CURCMDAR(i) (*(volatile uint32_t *)(HW_APBX_BASE + 0x100 + (i) * 0x70))

#define HW_APBX_CHx_NXTCMDAR(i) (*(volatile uint32_t *)(HW_APBX_BASE + 0x110 + (i) * 0x70))

#define HW_APBX_CHx_CMD(i)      (*(volatile uint32_t *)(HW_APBX_BASE + 0x120 + (i) * 0x70))

#define HW_APBX_CHx_BAR(i)      (*(volatile uint32_t *)(HW_APBX_BASE + 0x130 + (i) * 0x70))

#define HW_APBX_CHx_SEMA(i)     (*(volatile uint32_t *)(HW_APBX_BASE + 0x140 + (i) * 0x70))

#define HW_APBX_CHx_DEBUG1(i)   (*(volatile uint32_t *)(HW_APBX_BASE + 0x150 + (i) * 0x70))

#define HW_APBX_CHx_DEBUG2(i)   (*(volatile uint32_t *)(HW_APBX_BASE + 0x160 + (i) * 0x70))
#define HW_APBX_CHx_DEBUG2__AHB_BYTES_BP    0
#define HW_APBX_CHx_DEBUG2__AHB_BYTES_BM    0xffff
#define HW_APBX_CHx_DEBUG2__APB_BYTES_BP    16
#define HW_APBX_CHx_DEBUG2__APB_BYTES_BM    0xffff0000

/**********
 * COMMON *
 **********/

struct apb_dma_command_t
{
    struct apb_dma_command_t *next;
    uint32_t cmd;
    void *buffer;
    /* PIO words follow */
};

#define APBH_DMA_CHANNEL(i)     i
#define APBX_DMA_CHANNEL(i)     ((i) | 0x10)
#define APB_IS_APBX_CHANNEL(x)  ((x) & 0x10)
#define APB_GET_DMA_CHANNEL(x)  ((x) & 0xf)

#define APB_SSP(ssp)        APBH_DMA_CHANNEL(HW_APBH_SSP(ssp))
#define APB_AUDIO_ADC       APBX_DMA_CHANNEL(HW_APBX_AUDIO_ADC)
#define APB_AUDIO_DAC       APBX_DMA_CHANNEL(HW_APBX_AUDIO_DAC)
#define APB_I2C             APBX_DMA_CHANNEL(HW_APBX_I2C)

#define HW_APB_CHx_CMD__COMMAND_BM         0x3
#define HW_APB_CHx_CMD__COMMAND__NO_XFER   0
#define HW_APB_CHx_CMD__COMMAND__WRITE     1
#define HW_APB_CHx_CMD__COMMAND__READ      2
#define HW_APB_CHx_CMD__COMMAND__SENSE     3
#define HW_APB_CHx_CMD__CHAIN              (1 << 2)
#define HW_APB_CHx_CMD__IRQONCMPLT         (1 << 3)
/* those two are only available on APHB */
#define HW_APBH_CHx_CMD__NANDLOCK          (1 << 4)
#define HW_APBH_CHx_CMD__NANDWAIT4READY    (1 << 5)
#define HW_APB_CHx_CMD__SEMAPHORE          (1 << 6)
#define HW_APB_CHx_CMD__WAIT4ENDCMD        (1 << 7)
/* An errata advise not to use it */
//#define HW_APB_CHx_CMD__HALTONTERMINATE    (1 << 8)
#define HW_APB_CHx_CMD__CMDWORDS_BM         0xf000
#define HW_APB_CHx_CMD__CMDWORDS_BP         12
#define HW_APB_CHx_CMD__XFER_COUNT_BM       0xffff0000
#define HW_APB_CHx_CMD__XFER_COUNT_BP       16
/* For software use */
#define HW_APB_CHx_CMD__UNUSED_BP           8
#define HW_APB_CHx_CMD__UNUSED_BM           (0xf << 8)
#define HW_APB_CHx_CMD__UNUSED_MAGIC        (0xa << 8)

#define HW_APB_CHx_SEMA__PHORE_BM           0xff0000
#define HW_APB_CHx_SEMA__PHORE_BP           16

/* A single descriptor cannot transfer more than 2^16 bytes */
#define IMX233_MAX_SINGLE_DMA_XFER_SIZE     (1 << 16)

static void imx233_dma_init(void)
{
    __REG_CLR(HW_APBH_CTRL0) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
    __REG_CLR(HW_APBX_CTRL0) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
}

static void imx233_dma_reset_channel(unsigned chan)
{
    volatile uint32_t *ptr;
    uint32_t bm;
    if(APB_IS_APBX_CHANNEL(chan))
    {
        ptr = &HW_APBX_CHANNEL_CTRL;
        bm = HW_APBX_CHANNEL_CTRL__RESET_CHANNEL(APB_GET_DMA_CHANNEL(chan));
    }
    else
    {
        ptr = &HW_APBH_CTRL0;
        bm = HW_APBH_CTRL0__RESET_CHANNEL(APB_GET_DMA_CHANNEL(chan));
    }
    __REG_SET(*ptr) = bm;
    /* wait for end of reset */
    while(*ptr & bm)
        ;
}

static void imx233_dma_start_command(unsigned chan, struct apb_dma_command_t *cmd)
{
    if(APB_IS_APBX_CHANNEL(chan))
    {
        HW_APBX_CHx_NXTCMDAR(APB_GET_DMA_CHANNEL(chan)) = (uint32_t)cmd;
        HW_APBX_CHx_SEMA(APB_GET_DMA_CHANNEL(chan)) = 1;
    }
    else
    {
        HW_APBH_CHx_NXTCMDAR(APB_GET_DMA_CHANNEL(chan)) = (uint32_t)cmd;
        HW_APBH_CHx_SEMA(APB_GET_DMA_CHANNEL(chan)) = 1;
    }
}

static void imx233_dma_wait_completion(unsigned chan)
{
    volatile uint32_t *sema;
    if(APB_IS_APBX_CHANNEL(chan))
        sema = &HW_APBX_CHx_SEMA(APB_GET_DMA_CHANNEL(chan));
    else
        sema = &HW_APBH_CHx_SEMA(APB_GET_DMA_CHANNEL(chan));

    while(*sema & HW_APB_CHx_SEMA__PHORE_BM)
        ;
}

/**
 *
 * Digctl
 *
 */

/* Digital control */
#define HW_DIGCTL_BASE          0x8001C000
#define HW_DIGCTL_CTRL          (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0))
#define HW_DIGCTL_CTRL__USB_CLKGATE (1 << 2)

#define HW_DIGCTL_HCLKCOUNT     (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0x20))

#define HW_DIGCTL_MICROSECONDS  (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0xC0))

#define HW_DIGCTL_CHIPID        (*(volatile uint32_t *)(HW_DIGCTL_BASE + 0x310))
#define HW_DIGCTL_CHIPID__PRODUCT_CODE_BP   16
#define HW_DIGCTL_CHIPID__PRODUCT_CODE_BM   0xffff0000
#define HW_DIGCTL_CHIPID__REVISION_BP       0
#define HW_DIGCTL_CHIPID__REVISION_BM       0xff

static bool imx233_us_elapsed(uint32_t ref, unsigned us_delay)
{
    uint32_t cur = HW_DIGCTL_MICROSECONDS;
    if(ref + us_delay <= ref)
        return !(cur > ref) && !(cur < (ref + us_delay));
    else
        return (cur < ref) || cur >= (ref + us_delay);
}

static void udelay(unsigned us)
{
    uint32_t ref = HW_DIGCTL_MICROSECONDS;
    while(!imx233_us_elapsed(ref, us));
}

#define HZ  1000000

/**
 *
 * USB PHY
 *
 */
/* USB Phy */
#define HW_USBPHY_BASE          0x8007C000
#define HW_USBPHY_PWD           (*(volatile uint32_t *)(HW_USBPHY_BASE + 0))
#define HW_USBPHY_PWD__ALL      (7 << 10 | 0xf << 17)

#define HW_USBPHY_CTRL          (*(volatile uint32_t *)(HW_USBPHY_BASE + 0x30))

/**
 *
 * DCP
 *
 */
#define HW_DCP_BASE         0x80028000

#define HW_DCP_CTRL         (*(volatile unsigned long *)(HW_DCP_BASE + 0x0))

#define HW_DCP_STAT         (*(volatile unsigned long *)(HW_DCP_BASE + 0x10))
#define HW_DCP_STAT__IRQ(x) (1 << (x))

#define HW_DCP_CHANNELCTRL  (*(volatile unsigned long *)(HW_DCP_BASE + 0x20))
#define HW_DCP_CHANNELCTRL__ENABLE_CHANNEL(x)   (1 << (x))

#define HW_DCP_CH0CMDPTR    (*(volatile unsigned long *)(HW_DCP_BASE + 0x100))

#define HW_DCP_CH0SEMA      (*(volatile unsigned long *)(HW_DCP_BASE + 0x110))
#define HW_DCP_CH0SEMA__INCREMENT(x)    (x)
#define HW_DCP_CH0SEMA__VALUE_BP        16
#define HW_DCP_CH0SEMA__VALUE_BM        (0xff << 16)
#define HW_DCP_CH0STAT      (*(volatile unsigned long *)(HW_DCP_BASE + 0x120))

#define HW_DCP_CTRL0__INTERRUPT_ENABLE  (1 << 0)
#define HW_DCP_CTRL0__DECR_SEMAPHORE    (1 << 1)
#define HW_DCP_CTRL0__ENABLE_MEMCOPY    (1 << 4)
#define HW_DCP_CTRL0__ENABLE_CIPHER     (1 << 5)
#define HW_DCP_CTRL0__ENABLE_HASH       (1 << 6)
#define HW_DCP_CTRL0__CIPHER_ENCRYPT    (1 << 8)
#define HW_DCP_CTRL0__CIPHER_INIT       (1 << 9)
#define HW_DCP_CTRL0__OTP_KEY           (1 << 10)
#define HW_DCP_CTRL0__HASH_INIT         (1 << 12)
#define HW_DCP_CTRL0__HASH_TERM         (1 << 13)
#define HW_DCP_CTRL0__HASH_OUTPUT       (1 << 15)

#define HW_DCP_CTRL1__CIPHER_SELECT_BP  0
#define HW_DCP_CTRL1__CIPHER_SELECT_BM  0xf
#define HW_DCP_CTRL1__CIPHER_SELECT__AES128 0
#define HW_DCP_CTRL1__CIPHER_MODE_BP    4
#define HW_DCP_CTRL1__CIPHER_MODE_BM    0xf0
#define HW_DCP_CTRL1__CIPHER_MODE__CBC  (1 << 4)
#define HW_DCP_CTRL1__HASH_SELECT_BP    4
#define HW_DCP_CTRL1__HASH_SELECT_BM    0xf00

struct dcp_packet_t
{
    unsigned long next;
    unsigned long ctrl0;
    unsigned long ctrl1;
    unsigned long src_buf;
    unsigned long dst_buf;
    unsigned long buf_sz;
    unsigned long payload_ptr;
    unsigned long status;
} __attribute__((packed));

/**
 *
 * Misc
 *
 */

void memcpy(uint8_t *dst, const uint8_t *src, uint32_t length)
{
    for(uint32_t i = 0; i < length; i++)
        dst[i] = src[i];
}

void memset(uint8_t *dst, uint8_t fill, uint32_t length)
{
    for(uint32_t i = 0; i < length; i++)
        dst[i] = fill;
}

/**
 * 
 * USB stack
 * 
 */

static struct usb_device_descriptor __attribute__((aligned(2)))
    device_descriptor=
{
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = HWEMUL_USB_VID,
    .idProduct          = HWEMUL_USB_PID,
    .bcdDevice          = HWEMUL_VERSION_MAJOR << 8 | HWEMUL_VERSION_MINOR,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1
};

#define USB_MAX_CURRENT 200

static struct usb_config_descriptor __attribute__((aligned(2)))
                                    config_descriptor =
{
    .bLength             = sizeof(struct usb_config_descriptor),
    .bDescriptorType     = USB_DT_CONFIG,
    .wTotalLength        = 0, /* will be filled in later */
    .bNumInterfaces      = 1,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
    .bMaxPower           = (USB_MAX_CURRENT + 1) / 2, /* In 2mA units */
};

/* main interface */
static struct usb_interface_descriptor __attribute__((aligned(2)))
    interface_descriptor =
{
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 3,
    .bInterfaceClass    = HWEMUL_CLASS,
    .bInterfaceSubClass = HWEMUL_SUBCLASS,
    .bInterfaceProtocol = HWEMUL_PROTOCOL,
    .iInterface         = 4
};


static struct usb_endpoint_descriptor __attribute__((aligned(2)))
    endpoint_descriptor =
{
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = 0,
    .bmAttributes     = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize   = 0,
    .bInterval        = 0
};

static const struct usb_string_descriptor __attribute__((aligned(2)))
    usb_string_iManufacturer =
{
    24,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', '.', 'o', 'r', 'g'}
};

static const struct usb_string_descriptor __attribute__((aligned(2)))
    usb_string_iProduct =
{
    52,
    USB_DT_STRING,
    {'R', 'o', 'c', 'k', 'b', 'o', 'x', ' ',
     'h', 'a', 'r', 'd', 'w', 'a', 'r', 'e', ' ',
     'e', 'm', 'u', 'l', 'a', 't', 'e', 'r'}
};

static struct usb_string_descriptor __attribute__((aligned(2)))
    usb_string_iSerial =
{
    84,
    USB_DT_STRING,
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0',
     '0', '0', '0', '0', '0', '0', '0', '0'}
};

static struct usb_string_descriptor __attribute__((aligned(2)))
    usb_string_iInterface =
{
    28,
    USB_DT_STRING,
    {'A', 'c', 'i', 'd', ' ',
     '0' + (HWEMUL_VERSION_MAJOR >> 4), '0' + (HWEMUL_VERSION_MAJOR & 0xf), '.',
     '0' + (HWEMUL_VERSION_MINOR >> 4), '0' + (HWEMUL_VERSION_MINOR & 0xf), '.',
     '0' + (HWEMUL_VERSION_REV >> 4), '0' + (HWEMUL_VERSION_REV & 0xf) }
};

/* this is stringid #0: languages supported */
static const struct usb_string_descriptor __attribute__((aligned(2)))
    lang_descriptor =
{
    4,
    USB_DT_STRING,
    {0x0409} /* LANGID US English */
};

#define USB_NUM_STRINGS 5

static const struct usb_string_descriptor* const usb_strings[USB_NUM_STRINGS] =
{
   &lang_descriptor,
   &usb_string_iManufacturer,
   &usb_string_iProduct,
   &usb_string_iSerial,
   &usb_string_iInterface
};

uint8_t *usb_buffer = oc_bufferstart;
uint32_t usb_buffer_size = 0;

#define EP_BULK 1
#define EP_INT  2

static void set_config(void)
{
    usb_drv_configure_endpoint(EP_BULK, USB_ENDPOINT_XFER_BULK);
    usb_drv_configure_endpoint(EP_INT, USB_ENDPOINT_XFER_INT);
}

static void handle_std_dev_desc(struct usb_ctrlrequest *req)
{
    int size;
    const void* ptr = NULL;
    unsigned index = req->wValue & 0xff;

    switch(req->wValue >> 8)
    {
        case USB_DT_DEVICE:
            ptr = &device_descriptor;
            size = sizeof(struct usb_device_descriptor);
            break;
        case USB_DT_OTHER_SPEED_CONFIG:
        case USB_DT_CONFIG:
        {
            int max_packet_size;

            /* config desc */
            if((req->wValue >> 8) ==USB_DT_CONFIG)
            {
                max_packet_size = (usb_drv_port_speed() ? 512 : 64);
                config_descriptor.bDescriptorType = USB_DT_CONFIG;
            }
            else
            {
                max_packet_size=(usb_drv_port_speed() ? 64 : 512);
                config_descriptor.bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;
            }
            size = sizeof(struct usb_config_descriptor);

            /* interface */
            memcpy(usb_buffer + size, (void *)&interface_descriptor,
                sizeof(interface_descriptor));
            size += sizeof(interface_descriptor);
            /* endpoint 1: bulk out */
            endpoint_descriptor.bEndpointAddress = EP_BULK | USB_DIR_OUT;
            endpoint_descriptor.bmAttributes = USB_ENDPOINT_XFER_BULK;
            endpoint_descriptor.wMaxPacketSize = 512;
            memcpy(usb_buffer + size, (void *)&endpoint_descriptor,
                sizeof(endpoint_descriptor));
            size += sizeof(endpoint_descriptor);
            /* endpoint 2: bulk in */
            endpoint_descriptor.bEndpointAddress = EP_BULK | USB_DIR_IN;
            endpoint_descriptor.bmAttributes = USB_ENDPOINT_XFER_BULK;
            endpoint_descriptor.wMaxPacketSize = 512;
            memcpy(usb_buffer + size, (void *)&endpoint_descriptor,
                sizeof(endpoint_descriptor));
            size += sizeof(endpoint_descriptor);
            /* endpoint 3: int in */
            endpoint_descriptor.bEndpointAddress = EP_INT | USB_DIR_IN;
            endpoint_descriptor.bmAttributes = USB_ENDPOINT_XFER_INT;
            endpoint_descriptor.wMaxPacketSize = 1024;
            memcpy(usb_buffer + size, (void *)&endpoint_descriptor,
                sizeof(endpoint_descriptor));
            size += sizeof(endpoint_descriptor);

            /* fix config descriptor */
            config_descriptor.bNumInterfaces = 1;
            config_descriptor.wTotalLength = size;
            memcpy(usb_buffer, (void *)&config_descriptor, sizeof(config_descriptor));

            ptr = usb_buffer;
            break;
        }
        case USB_DT_STRING:
            if(index < USB_NUM_STRINGS)
            {
                size = usb_strings[index]->bLength;
                ptr = usb_strings[index];
            }
            else
                usb_drv_stall(EP_CONTROL, true, true);
            break;
        default:
            break;
    }

    if(ptr)
    {
        int length = MIN(size, req->wLength);

        if(ptr != usb_buffer)
            memcpy(usb_buffer, ptr, length);

        usb_drv_send(EP_CONTROL, usb_buffer, length);
        usb_drv_recv(EP_CONTROL, NULL, 0);
    }
    else
        usb_drv_stall(EP_CONTROL, true, true);
}

static void handle_std_dev_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequest)
    {
        case USB_REQ_GET_CONFIGURATION:
            usb_buffer[0] = 1;
            usb_drv_send(EP_CONTROL, usb_buffer, 1);
            usb_drv_recv(EP_CONTROL, NULL, 0);
            break;
        case USB_REQ_SET_CONFIGURATION:
            usb_drv_send(EP_CONTROL, NULL, 0);
            set_config();
            break;
        case USB_REQ_GET_DESCRIPTOR:
            handle_std_dev_desc(req);
            break;
        case USB_REQ_SET_ADDRESS:
            usb_drv_send(EP_CONTROL, NULL, 0);
            usb_drv_set_address(req->wValue);
            break;
        case USB_REQ_GET_STATUS:
            usb_buffer[0] = 0;
            usb_buffer[1] = 0;
            usb_drv_send(EP_CONTROL, usb_buffer, 2);
            usb_drv_recv(EP_CONTROL, NULL, 0);
            break;
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

static void handle_std_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequestType & USB_RECIP_MASK)
    {
        case USB_RECIP_DEVICE:
            return handle_std_dev_req(req);
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

struct usb_resp_info_version_t g_version =
{
    .major = HWEMUL_VERSION_MAJOR,
    .minor = HWEMUL_VERSION_MINOR,
    .revision = HWEMUL_VERSION_REV
};

struct usb_resp_info_layout_t g_layout;

struct usb_resp_info_stmp_t g_stmp;

struct usb_resp_info_features_t g_features =
{
    .feature_mask = HWEMUL_FEATURE_LOG | HWEMUL_FEATURE_MEM |
        HWEMUL_FEATURE_CALL | HWEMUL_FEATURE_JUMP | HWEMUL_FEATURE_AES_OTP
};

static void fill_layout_info(void)
{
    g_layout.oc_code_start = (uint32_t)oc_codestart;
    g_layout.oc_code_size = oc_codesize;
    g_layout.oc_stack_start = (uint32_t)oc_stackstart;
    g_layout.oc_stack_size = oc_stacksize;
    g_layout.oc_buffer_start = (uint32_t)oc_bufferstart;
    g_layout.oc_buffer_size = oc_buffersize;
}

static void fill_stmp_info(void)
{
    g_stmp.chipid = __XTRACT(HW_DIGCTL_CHIPID, PRODUCT_CODE);
    g_stmp.rev = __XTRACT(HW_DIGCTL_CHIPID, REVISION);
    g_stmp.is_supported = g_stmp.chipid == 0x3780 || g_stmp.chipid == 0x3700 ||
        g_stmp.chipid == 0x3b00;
}

static void handle_get_info(struct usb_ctrlrequest *req)
{
    void *ptr = NULL;
    int size = 0;
    switch(req->wIndex)
    {
        case HWEMUL_INFO_VERSION:
            ptr = &g_version;
            size = sizeof(g_version);
            break;
        case HWEMUL_INFO_LAYOUT:
            fill_layout_info();
            ptr = &g_layout;
            size = sizeof(g_layout);
            break;
        case HWEMUL_INFO_STMP:
            fill_stmp_info();
            ptr = &g_stmp;
            size = sizeof(g_stmp);
            break;
        case HWEMUL_INFO_FEATURES:
            ptr = &g_features;
            size = sizeof(g_features);
            break;
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }

    if(ptr)
    {
        int length = MIN(size, req->wLength);
        
        if(ptr != usb_buffer)
            memcpy(usb_buffer, ptr, length);
        usb_drv_send(EP_CONTROL, usb_buffer, length);
        usb_drv_recv(EP_CONTROL, NULL, 0);
    }
}

static void handle_get_log(struct usb_ctrlrequest *req)
{
    enable_logf(false);
    int length = logf_readback(usb_buffer, MIN(req->wLength, usb_buffer_size));
    usb_drv_send(EP_CONTROL, usb_buffer, length);
    usb_drv_recv(EP_CONTROL, NULL, 0);
    enable_logf(true);
}

static void handle_rw_mem(struct usb_ctrlrequest *req)
{
    uint32_t addr = req->wValue | req->wIndex << 16;
    uint16_t length = req->wLength;
    
    if(req->bRequestType & USB_DIR_IN)
    {
        memcpy(usb_buffer, (void *)addr, length);
        asm volatile("nop" : : : "memory");
        usb_drv_send(EP_CONTROL, usb_buffer, length);
        usb_drv_recv(EP_CONTROL, NULL, 0);
    }
    else
    {
        int size = usb_drv_recv(EP_CONTROL, usb_buffer, length);
        asm volatile("nop" : : : "memory");
        if(size != length)
            usb_drv_stall(EP_CONTROL, true, true);
        else
        {
            memcpy((void *)addr, usb_buffer, length);
            usb_drv_send(EP_CONTROL, NULL, 0);
        }
    }
}

static void handle_call_jump(struct usb_ctrlrequest *req)
{
    uint32_t addr = req->wValue | req->wIndex << 16;

    if(req->bRequest == HWEMUL_CALL)
        ((void (*)(void))addr)();
    else
        asm volatile("bx %0\n" : : "r" (addr) : "memory");
}

static void do_aes_otp(void *buffer, unsigned length, unsigned params)
{
    static struct dcp_packet_t dcp_packet;

    bool encrypt = !!(params & HWEMUL_AES_OTP_ENCRYPT);
    /* reset DCP */
    __REG_SET(HW_DCP_CTRL) = 0x80000000;
    /* clear clock gate */
    __REG_CLR(HW_DCP_CTRL) = 0xc0000000;
    /* enable dma for channel 0 */
    __REG_SET(HW_DCP_CHANNELCTRL) = HW_DCP_CHANNELCTRL__ENABLE_CHANNEL(0);
    /* prepare packet */
    dcp_packet.next = 0;

    dcp_packet.ctrl0 = HW_DCP_CTRL0__INTERRUPT_ENABLE |
        HW_DCP_CTRL0__DECR_SEMAPHORE | HW_DCP_CTRL0__CIPHER_INIT |
        HW_DCP_CTRL0__ENABLE_CIPHER | HW_DCP_CTRL0__OTP_KEY |
        (encrypt ? HW_DCP_CTRL0__CIPHER_ENCRYPT : 0);
    dcp_packet.ctrl1 = HW_DCP_CTRL1__CIPHER_SELECT__AES128 |
        HW_DCP_CTRL1__CIPHER_MODE__CBC;
    dcp_packet.src_buf = (unsigned long)buffer + 16;
    dcp_packet.dst_buf = (unsigned long)buffer + 16;
    dcp_packet.buf_sz = length - 16;
    dcp_packet.payload_ptr = (unsigned long)buffer;
    dcp_packet.status = 0;

    asm volatile("":::"memory");
    /* kick */
    HW_DCP_CH0CMDPTR = (unsigned long)&dcp_packet;
    HW_DCP_CH0SEMA = HW_DCP_CH0SEMA__INCREMENT(1);
    /* wait */
    while(!(HW_DCP_STAT & HW_DCP_STAT__IRQ(0)));

    usb_drv_send_nonblocking(EP_INT, buffer, length);
}

static void handle_aes_otp(struct usb_ctrlrequest *req)
{
    uint16_t length = req->wLength;

    int size = usb_drv_recv(EP_CONTROL, usb_buffer, length);
    if(size != length)
        usb_drv_stall(EP_CONTROL, true, true);
    else
        usb_drv_send(EP_CONTROL, NULL, 0);
    do_aes_otp(usb_buffer, length, req->wValue);
}

static void handle_class_dev_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequest)
    {
        case HWEMUL_GET_INFO:
            handle_get_info(req);
            break;
        case HWEMUL_GET_LOG:
            handle_get_log(req);
            break;
        case HWEMUL_RW_MEM:
            handle_rw_mem(req);
            break;
        case HWEMUL_CALL:
        case HWEMUL_JUMP:
            handle_call_jump(req);
            break;
        case HWEMUL_AES_OTP:
            handle_aes_otp(req);
            break;
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

static void handle_class_req(struct usb_ctrlrequest *req)
{
    switch(req->bRequestType & USB_RECIP_MASK)
    {
        case USB_RECIP_DEVICE:
            return handle_class_dev_req(req);
        default:
            usb_drv_stall(EP_CONTROL, true, true);
    }
}

/**
 * 
 * Main
 * 
 */

void main(uint32_t arg)
{
    usb_buffer_size = oc_buffersize;
    
    logf("hwemul %d.%d.%d\n", HWEMUL_VERSION_MAJOR, HWEMUL_VERSION_MINOR,
         HWEMUL_VERSION_REV);
    logf("argument: 0x%08x\n", arg);

    /* we don't know if USB was connected or not. In USB recovery mode it will
     * but in other cases it might not be. In doubt, disconnect */
    REG_USBCMD &= ~USBCMD_RUN;
    /* enable USB PHY PLL */
    __REG_SET(HW_CLKCTRL_PLLCTRL0) = HW_CLKCTRL_PLLCTRL0__EN_USB_CLKS;
    /* power up USB PHY */
    __REG_CLR(HW_USBPHY_CTRL) = __BLOCK_CLKGATE | __BLOCK_SFTRST;
    //__REG_CLR(HW_USBPHY_PWD) = HW_USBPHY_PWD__ALL;
    HW_USBPHY_PWD = 0;
    /* enable USB controller */
    __REG_CLR(HW_DIGCTL_CTRL) = HW_DIGCTL_CTRL__USB_CLKGATE;
    /* reset the controller */
    REG_USBCMD |= USBCMD_CTRL_RESET;
    while (REG_USBCMD & USBCMD_CTRL_RESET);
    /* put it in device mode */
    REG_USBMODE = USBMODE_CTRL_MODE_DEVICE;
    /* reset address */
    REG_DEVICEADDR = 0;
    /* prepare qh array */
    qh_array[0].max_pkt_length = 1 << 29 | MAX_PKT_SIZE_EP0 << 16;
    qh_array[1].max_pkt_length = 1 << 29 | MAX_PKT_SIZE_EP0 << 16;
    qh_array[2].max_pkt_length = 1 << 29 | MAX_PKT_SIZE << 16;
    qh_array[3].max_pkt_length = 1 << 29 | MAX_PKT_SIZE << 16;
    /* setup qh */
    REG_ENDPOINTLISTADDR = (unsigned int)qh_array;
    /* clear setup status */
    REG_ENDPTSETUPSTAT = EPSETUP_STATUS_EP0;
    /* run! */
    REG_USBCMD |= USBCMD_RUN;
    
    while(1)
    {
        /* wait for setup */
        while(!(REG_ENDPTSETUPSTAT & EPSETUP_STATUS_EP0))
            ;
        /* clear setup status */
        REG_ENDPTSETUPSTAT = EPSETUP_STATUS_EP0;
        /* check request */
        asm volatile("":::"memory");
        struct usb_ctrlrequest *req = (void *)&qh_array[0].setup_buffer[0];
        
        switch(req->bRequestType & USB_TYPE_MASK)
        {
            case USB_TYPE_STANDARD:
                handle_std_req(req);
                break;
            case USB_TYPE_CLASS:
                handle_class_req(req);
                break;
            default:
                usb_drv_stall(EP_CONTROL, true, true);
        }
    }
}
