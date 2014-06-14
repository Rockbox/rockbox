#ifndef __USB_DESIGNWARE_REGS_H__
#define __USB_DESIGNWARE_REGS_H__


#include <inttypes.h>


union usb_dw_gotgctl
{
    uint32_t d32;
    struct
    {
        uint32_t sesreqscs :1;
        uint32_t sesreq :1;
        uint32_t reserved2_7 :6;
        uint32_t hstnegscs :1;
        uint32_t hnpreq :1;
        uint32_t hstsethnpen :1;
        uint32_t devhnpen :1;
        uint32_t reserved12_15 :4;
        uint32_t conidsts :1;
        uint32_t reserved17 :1;
        uint32_t asesvld :1;
        uint32_t bsesvld :1;
        uint32_t currmod :1;
        uint32_t reserved21_31 :11;
    } b;
};

union usb_dw_gotgint
{
    uint32_t d32;
    struct
    {
        uint32_t reserved0_1 :2;
        uint32_t sesenddet :1;
        uint32_t reserved3_7 :5;
        uint32_t sesreqsucstschng :1;
        uint32_t hstnegsucstschng :1;
        uint32_t reserver10_16 :7;
        uint32_t hstnegdet :1;
        uint32_t adevtoutchng :1;
        uint32_t debdone :1;
        uint32_t reserved31_20 :12;
    } b;
};

union usb_dw_gahbcfg
{
    uint32_t d32;
    struct
    {
        uint32_t glblintrmsk :1;
        uint32_t hburstlen :4;
        uint32_t dmaenable :1;
        uint32_t reserved :1;
        uint32_t nptxfemplvl_txfemplvl :1;
        uint32_t ptxfemplvl :1;
        uint32_t reserved9_31 :23;
    } b;
};

union usb_dw_gusbcfg
{
    uint32_t d32;
    struct
    {
        uint32_t toutcal :3;
        uint32_t phyif :1;
        uint32_t ulpi_utmi_sel :1;
        uint32_t fsintf :1;
        uint32_t physel :1;
        uint32_t ddrsel :1;
        uint32_t srpcap :1;
        uint32_t hnpcap :1;
        uint32_t usbtrdtim :4;
        uint32_t nptxfrwnden :1;
        uint32_t phylpwrclksel :1;
        uint32_t otgutmifssel :1;
        uint32_t ulpi_fsls :1;
        uint32_t ulpi_auto_res :1;
        uint32_t ulpi_clk_sus_m :1;
        uint32_t ulpi_ext_vbus_drv :1;
        uint32_t ulpi_int_vbus_indicator :1;
        uint32_t term_sel_dl_pulse :1;
        uint32_t reserved :6;
        uint32_t force_host :1;
        uint32_t force_dev :1;
        uint32_t corrupt_tx :1;
    } b;
};

union usb_dw_grstctl
{
    uint32_t d32;
    struct
    {
        uint32_t csftrst :1;
        uint32_t hsftrst :1;
        uint32_t hstfrm :1;
        uint32_t intknqflsh :1;
        uint32_t rxfflsh :1;
        uint32_t txfflsh :1;
        uint32_t txfnum :5;
        uint32_t reserved11_29 :19;
        uint32_t dmareq :1;
        uint32_t ahbidle :1;
    } b;
};

union usb_dw_gintmsk
{
    uint32_t d32;
    struct
    {
        uint32_t reserved0 :1;
        uint32_t modemismatch :1;
        uint32_t otgintr :1;
        uint32_t sofintr :1;
        uint32_t rxstsqlvl :1;
        uint32_t nptxfempty :1;
        uint32_t ginnakeff :1;
        uint32_t goutnakeff :1;
        uint32_t reserved8 :1;
        uint32_t i2cintr :1;
        uint32_t erlysuspend :1;
        uint32_t usbsuspend :1;
        uint32_t usbreset :1;
        uint32_t enumdone :1;
        uint32_t isooutdrop :1;
        uint32_t eopframe :1;
        uint32_t reserved16 :1;
        uint32_t epmismatch :1;
        uint32_t inepintr :1;
        uint32_t outepintr :1;
        uint32_t incomplisoin :1;
        uint32_t incomplisoout :1;
        uint32_t reserved22_23 :2;
        uint32_t portintr :1;
        uint32_t hcintr :1;
        uint32_t ptxfempty :1;
        uint32_t reserved27 :1;
        uint32_t conidstschng :1;
        uint32_t disconnect :1;
        uint32_t sessreqintr :1;
        uint32_t wkupintr :1;
    } b;
};

union usb_dw_gintsts
{
    uint32_t d32;
    struct
    {
        uint32_t curmode :1;
        uint32_t modemismatch :1;
        uint32_t otgintr :1;
        uint32_t sofintr :1;
        uint32_t rxstsqlvl :1;
        uint32_t nptxfempty :1;
        uint32_t ginnakeff :1;
        uint32_t goutnakeff :1;
        uint32_t reserved8 :1;
        uint32_t i2cintr :1;
        uint32_t erlysuspend :1;
        uint32_t usbsuspend :1;
        uint32_t usbreset :1;
        uint32_t enumdone :1;
        uint32_t isooutdrop :1;
        uint32_t eopframe :1;
        uint32_t intimerrx :1;
        uint32_t epmismatch :1;
        uint32_t inepintr :1;
        uint32_t outepintr :1;
        uint32_t incomplisoin :1;
        uint32_t incomplisoout :1;
        uint32_t reserved22_23 :2;
        uint32_t portintr :1;
        uint32_t hcintr :1;
        uint32_t ptxfempty :1;
        uint32_t reserved27 :1;
        uint32_t conidstschng :1;
        uint32_t disconnect :1;
        uint32_t sessreqintr :1;
        uint32_t wkupintr :1;
    } b;
};

union usb_dw_drxsts
{
    uint32_t d32;
    struct
    {
        uint32_t epnum :4;
        uint32_t bcnt :11;
        uint32_t dpid :2;
        uint32_t pktsts :4;
        uint32_t fn :4;
        uint32_t reserved :7;
    } b;
};

union usb_dw_grxfsts
{
    uint32_t d32;
    struct
    {
        uint32_t chnum :4;
        uint32_t bcnt :11;
        uint32_t dpid :2;
        uint32_t pktsts :4;
        uint32_t reserved :11;
    } b;
};

union usb_dw_txfsiz
{
    uint32_t d32;
    struct
    {
        uint32_t startaddr :16;
        uint32_t depth :16;
    } b;
};

union usb_dw_rxfsiz
{
    uint32_t d32;
    struct
    {
        uint32_t depth :16;
        uint32_t reserved :16;
    } b;
};

union usb_dw_hnptxsts
{
    uint32_t d32;
    struct
    {
        uint32_t nptxfspcavail :16;
        uint32_t nptxqspcavail :8;
        uint32_t nptxqtop_terminate :1;
        uint32_t nptxqtop_timer :2;
        uint32_t nptxqtop :2;
        uint32_t chnum :2;
        uint32_t reserved :1;
    } b;
};

union usb_dw_dtxfstsn
{
    uint32_t d32;
    struct
    {
        uint32_t txfspcavail :16;
        uint32_t reserved :16;
    } b;
};

union usb_dw_gi2cctl
{
    uint32_t d32;
    struct
    {
        uint32_t rwdata :8;
        uint32_t regaddr :8;
        uint32_t addr :7;
        uint32_t i2cen :1;
        uint32_t ack :1;
        uint32_t i2csuspctl :1;
        uint32_t i2cdevaddr :2;
        uint32_t dat_se0 :1;
        uint32_t reserved :1;
        uint32_t rw :1;
        uint32_t bsydne :1;
    } b;
};

union usb_dw_gccfg
{
    uint32_t d32;
    struct
    {
        uint32_t reserved_in :16;
        uint32_t pwdn :1;
        uint32_t i2cifen :1;
        uint32_t vbussensinga :1;
        uint32_t vbussensingb :1;
        uint32_t sofouten :1;
        uint32_t disablevbussensing :1;
        uint32_t reserved_out :10;
    } b;
};

union usb_dw_ghwcfg1
{
    uint32_t d32;
    struct
    {
        uint32_t ep0 :2;
        uint32_t ep1 :2;
        uint32_t ep2 :2;
        uint32_t ep3 :2;
        uint32_t ep4 :2;
        uint32_t ep5 :2;
        uint32_t ep6 :2;
        uint32_t ep7 :2;
        uint32_t ep8 :2;
        uint32_t ep9 :2;
        uint32_t ep10 :2;
        uint32_t ep11 :2;
        uint32_t ep12 :2;
        uint32_t ep13 :2;
        uint32_t ep14 :2;
        uint32_t ep15 :2;
    } b;
};

union usb_dw_ghwcfg2
{
    uint32_t d32;
    struct
    {
        uint32_t otgmode :3;
        uint32_t otgarch :2;
        uint32_t singpnt :1;
        uint32_t hsphytype :2;
        uint32_t fsphytype :2;
        uint32_t numdeveps :4;
        uint32_t numhstchnl :4;
        uint32_t periosupport :1;
        uint32_t dynfifosizing :1;
        uint32_t reserved_20_21 :2;
        uint32_t nptxqdepth :2;
        uint32_t ptxqdepth :2;
        uint32_t tknqdepth :5;
        uint32_t reserved_31_31 :1;
    } b;
};

union usb_dw_ghwcfg3
{
    uint32_t d32;
    struct
    {
        uint32_t xfersizewidth :4;
        uint32_t pktsizewidth :3;
        uint32_t otgen :1;
        uint32_t i2c_selection :1;
        uint32_t vendor_control_interface_support :1;
        uint32_t optfeature :1;
        uint32_t rsttype :1;
        uint32_t ahbphysync :1;
        uint32_t reserved_13_15 :3;
        uint32_t dfifodepth :16;
    } b;
};

union usb_dw_ghwcfg4
{
    uint32_t d32;
    struct
    {
        uint32_t numdevperioeps :4;
        uint32_t enablepwropt :1;
        uint32_t ahbfreq :1;
        uint32_t reserved_6_13 :8;
        uint32_t phydatawidth :2;
        uint32_t numctleps :4;
        uint32_t iddgfltr :1;
        uint32_t vbusvalidfltr :1;
        uint32_t avalidfltr :1;
        uint32_t bvalidfltr :1;
        uint32_t sessendfltr :1;
        uint32_t endedtrfifo :1;
        uint32_t numdevmodinend :4;
        uint32_t reserved_30_31 :2;
    } b;
};

union usb_dw_dcfg
{
    uint32_t d32;
    struct
    {
        uint32_t devspd :2;
        uint32_t nzstsouthshk :1;
        uint32_t reserved3 :1;
        uint32_t devaddr :7;
        uint32_t perfrint :2;
        uint32_t reserved13_17 :5;
        uint32_t epmscnt :4;
    } b;
};

union usb_dw_dctl
{
    uint32_t d32;
    struct
    {
        uint32_t rmtwkupsig :1;
        uint32_t sftdiscon :1;
        uint32_t gnpinnaksts :1;
        uint32_t goutnaksts :1;
        uint32_t tstctl :3;
        uint32_t sgnpinnak :1;
        uint32_t cgnpinnak :1;
        uint32_t sgoutnak :1;
        uint32_t cgoutnak :1;
        uint32_t reserved :21;
    } b;
};

union usb_dw_dsts
{
    uint32_t d32;
    struct
    {
        uint32_t suspsts :1;
        uint32_t enumspd :2;
        uint32_t errticerr :1;
        uint32_t reserved4_7 :4;
        uint32_t soffn :14;
        uint32_t reserved22_31 :10;
    } b;
};

union usb_dw_diepintn
{
    uint32_t d32;
    struct
    {
        uint32_t xfercompl :1;
        uint32_t epdisabled :1;
        uint32_t ahberr :1;
        uint32_t timeout :1;
        uint32_t intktxfemp :1;
        uint32_t intknepmis :1;
        uint32_t inepnakeff :1;
        uint32_t emptyintr :1;
        uint32_t txfifoundrn :1;
        uint32_t reserved08_31 :23;
    } b;
};

union usb_dw_doepintn
{
    uint32_t d32;
    struct
    {
        uint32_t xfercompl :1;
        uint32_t epdisabled :1;
        uint32_t ahberr :1;
        uint32_t setup :1;
        uint32_t reserved04_31 :28;
    } b;
};

union usb_dw_daint
{
    uint32_t d32;
    struct
    {
        uint32_t in :16;
        uint32_t out :16;
    } ep;
};

union usb_dw_dthrctl
{
    uint32_t d32;
    struct
    {
        uint32_t non_iso_thr_en :1;
        uint32_t iso_thr_en :1;
        uint32_t tx_thr_len :9;
        uint32_t reserved11_15 :5;
        uint32_t rx_thr_en :1;
        uint32_t rx_thr_len :9;
        uint32_t reserved26 :1;
        uint32_t arb_park_en :1;
        uint32_t reserved28_31 :4;
    } b;
};

union usb_dw_depctl
{
    uint32_t d32;
    struct
    {
        uint32_t mps :11;
        uint32_t nextep :4;
        uint32_t usbactep :1;
        uint32_t dpid :1;
        uint32_t naksts :1;
        uint32_t eptype :2;
        uint32_t snp :1;
        uint32_t stall :1;
        uint32_t txfnum :4;
        uint32_t cnak :1;
        uint32_t snak :1;
        uint32_t setd0pid :1;
        uint32_t setd1pid :1;
        uint32_t epdis :1;
        uint32_t epena :1;
    } b;
};
union usb_dw_depxfrsiz
{
    uint32_t d32;
    struct
    {
        uint32_t xfersize :19;
        uint32_t pktcnt :10;
        uint32_t mc :2;
        uint32_t reserved :1;
    } b;
};

union usb_dw_dep0xfrsiz
{
    uint32_t d32;
    struct
    {
        uint32_t xfersize :7;
        uint32_t reserved7_18 :12;
        uint32_t pktcnt :2;
        uint32_t reserved21_28 :8;
        uint32_t supcnt :2;
        uint32_t reserved31;
    } b;
};

union usb_dw_hcfg
{
    uint32_t d32;
    struct
    {
        uint32_t fslspclksel :2;
        uint32_t fslssupp :1;
    } b;
};

union usb_dw_hfrmintrvl
{
    uint32_t d32;
    struct
    {
        uint32_t frint :16;
        uint32_t reserved :16;
    } b;
};

union usb_dw_hfnum
{
    uint32_t d32;
    struct
    {
        uint32_t frnum :16;
        uint32_t frrem :16;
    } b;
};

union usb_dw_hptxsts
{
    uint32_t d32;
    struct
    {
        uint32_t ptxfspcavail :16;
        uint32_t ptxqspcavail :8;
        uint32_t ptxqtop_terminate :1;
        uint32_t ptxqtop_timer :2;
        uint32_t ptxqtop :2;
        uint32_t chnum :2;
        uint32_t ptxqtop_odd :1;
    } b;
};

union usb_dw_hprt0
{
    uint32_t d32;
    struct
    {
        uint32_t prtconnsts :1;
        uint32_t prtconndet :1;
        uint32_t prtena :1;
        uint32_t prtenchng :1;
        uint32_t prtovrcurract :1;
        uint32_t prtovrcurrchng :1;
        uint32_t prtres :1;
        uint32_t prtsusp :1;
        uint32_t prtrst :1;
        uint32_t reserved9 :1;
        uint32_t prtlnsts :2;
        uint32_t prtpwr :1;
        uint32_t prttstctl :4;
        uint32_t prtspd :2;
        uint32_t reserved19_31 :13;
    } b;
};

union usb_dw_haint
{
    uint32_t d32;
    struct
    {
        uint32_t chint :16;
        uint32_t reserved :16;
    } b;
};

union usb_dw_haintmsk
{
    uint32_t d32;
    struct
    {
        uint32_t chint :16;
        uint32_t reserved :16;
    } b;
};

union usb_dw_hcchar
{
    uint32_t d32;
    struct
    {
        uint32_t mps :11;
        uint32_t epnum :4;
        uint32_t epdir :1;
        uint32_t reserved :1;
        uint32_t lspddev :1;
        uint32_t eptype :2;
        uint32_t multicnt :2;
        uint32_t devaddr :7;
        uint32_t oddfrm :1;
        uint32_t chdis :1;
        uint32_t chen :1;
    } b;
};

union usb_dw_hcsplt
{
    uint32_t d32;
    struct
    {
        uint32_t prtaddr :7;
        uint32_t hubaddr :7;
        uint32_t xactpos :2;
        uint32_t compsplt :1;
        uint32_t reserved :14;
        uint32_t spltena :1;
    } b;
};

union usb_dw_hcintn
{
    uint32_t d32;
    struct
    {
        uint32_t xfercompl :1;
        uint32_t chhltd :1;
        uint32_t ahberr :1;
        uint32_t stall :1;
        uint32_t nak :1;
        uint32_t ack :1;
        uint32_t nyet :1;
        uint32_t xacterr :1;
        uint32_t bblerr :1;
        uint32_t frmovrun :1;
        uint32_t datatglerr :1;
        uint32_t reserved :21;
    } b;
};

union usb_dw_hctsizn
{
    uint32_t d32;
    struct
    {
        uint32_t xfersize :19;
        uint32_t pktcnt :10;
        uint32_t pid :2;
        uint32_t dopng :1;
    } b;
};

union usb_dw_hcgintmsk
{
    uint32_t d32;
    struct
    {
        uint32_t xfercompl :1;
        uint32_t chhltd :1;
        uint32_t ahberr :1;
        uint32_t stall :1;
        uint32_t nak :1;
        uint32_t ack :1;
        uint32_t nyet :1;
        uint32_t xacterr :1;
        uint32_t bblerr :1;
        uint32_t frmovrun :1;
        uint32_t datatglerr :1;
        uint32_t reserved :21;
    } b;
};

union usb_dw_pcgcctl
{
    uint32_t d32;
    struct
    {
        uint32_t stoppclk :1;
        uint32_t gatehclk :1;
        uint32_t reserved :30;
    } b;
};

struct usb_dw_gregs
{
    volatile union usb_dw_gotgctl gotgctl;
    volatile union usb_dw_gotgint gotgint;
    volatile union usb_dw_gahbcfg gahbcfg;
    volatile union usb_dw_gusbcfg gusbcfg;
    volatile union usb_dw_grstctl grstctl;
    volatile union usb_dw_gintsts gintsts;
    volatile union usb_dw_gintmsk gintmsk;
    volatile union usb_dw_grxfsts grxstsr;
    volatile union usb_dw_grxfsts grxstsp;
    volatile union usb_dw_rxfsiz grxfsiz;
    volatile union usb_dw_txfsiz dieptxf0_hnptxfsiz;
    volatile union usb_dw_hnptxsts hnptxsts;
    volatile union usb_dw_gi2cctl gi2cctl;
    volatile uint32_t reserved34;
    volatile union usb_dw_gccfg gccfg;
    volatile uint32_t cid;
    volatile uint32_t gsnpsid;
    volatile union usb_dw_ghwcfg1 ghwcfg1;
    volatile union usb_dw_ghwcfg2 ghwcfg2;
    volatile union usb_dw_ghwcfg3 ghwcfg3;
    volatile union usb_dw_ghwcfg4 ghwcfg4;
    volatile uint32_t glpmcfg;
    volatile uint32_t reserved58[42];
    volatile union usb_dw_txfsiz hptxfsiz;
    volatile union usb_dw_txfsiz dieptxf[15];
};

struct usb_dw_dregs
{
    volatile union usb_dw_dcfg dcfg;
    volatile union usb_dw_dctl dctl;
    volatile union usb_dw_dsts dsts;
    volatile uint32_t reserved0c;
    volatile union usb_dw_diepintn diepmsk;
    volatile union usb_dw_doepintn doepmsk;
    volatile union usb_dw_daint daint;
    volatile union usb_dw_daint daintmsk;
    volatile uint32_t reserved20;
    volatile uint32_t reserved9;
    volatile uint32_t dvbusdis;
    volatile uint32_t dvbuspulse;
    volatile union usb_dw_dthrctl dthrctl;
    volatile union usb_dw_daint diepempmsk;
    volatile union usb_dw_daint deachint;
    volatile union usb_dw_daint deachmsk;
    volatile union usb_dw_diepintn dinep1msk;
    volatile uint32_t reserved44[15];
    volatile union usb_dw_doepintn doutep1msk;
};

struct usb_dw_inepregs
{
    volatile union usb_dw_depctl diepctl;
    volatile uint32_t reserved04;
    volatile union usb_dw_diepintn diepint;
    volatile uint32_t reserved0c;
    volatile union usb_dw_depxfrsiz dieptsiz;
    volatile const void* diepdma;
    volatile union usb_dw_dtxfstsn dtxfsts;
    volatile uint32_t reserved18;
};

struct usb_dw_outepregs
{
    volatile union usb_dw_depctl doepctl;
    volatile uint32_t doutepfrm;
    volatile union usb_dw_doepintn doepint;
    volatile uint32_t reserved0c;
    volatile union usb_dw_depxfrsiz doeptsiz;
    volatile void* doepdma;
    volatile uint32_t reserved18[2];
};

struct usb_dw_hregs
{
    volatile union usb_dw_hcfg hcfg;
    volatile uint32_t hfir;
    volatile union usb_dw_hfnum hfnum;
    volatile uint32_t reserved40c;
    volatile union usb_dw_hptxsts hptxsts;
    volatile union usb_dw_haint haint;
    volatile union usb_dw_haintmsk haintmsk;
};

struct usb_dw_hc_regs
{
    volatile union usb_dw_hcchar hcchar;
    volatile union usb_dw_hcsplt hcsplt;
    volatile union usb_dw_hcintn hcint;
    volatile union usb_dw_hcintn hcgintmsk;
    volatile union usb_dw_hctsizn hctsiz;
    volatile void* hcdma;
    volatile uint32_t reserved[2];
};

struct usb_dw_core_regs
{
    volatile struct usb_dw_gregs gregs;
    volatile uint32_t reserved140[176];
    volatile struct usb_dw_hregs hregs;
    volatile uint32_t reserved41c[9];
    volatile union usb_dw_hprt0 hprt0;
    volatile uint32_t reserved444[47];
    volatile struct usb_dw_hc_regs hc_regs[16];
    volatile uint32_t reserved700[64];
    volatile struct usb_dw_dregs dregs;
    volatile uint32_t reserved884[31];
    volatile struct usb_dw_inepregs inep_regs[16];
    volatile struct usb_dw_outepregs outep_regs[16];
    volatile uint32_t reservedd00[64];
    volatile union usb_dw_pcgcctl pcgcctl;
    volatile uint32_t reservede04[127];
    volatile uint32_t dfifo[16][1024];
    volatile uint32_t reserved11000[15360];
    volatile uint32_t rawfifo[32768];
};


#endif
