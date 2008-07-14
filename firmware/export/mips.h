/**************************************************************************
*                                                                         *
*   PROJECT     : MIPS port for uC/OS-II                                  *
*                                                                         *
*   MODULE      : MIPS.h                                                  *
*                                                                         *
*   AUTHOR      : Michael Anburaj                                         *
*                 URL  : http://geocities.com/michaelanburaj/             *
*                 EMAIL: michaelanburaj@hotmail.com                       *
*                                                                         *
*   PROCESSOR   : MIPS 4Kc (32 bit RISC) - ATLAS board                    *
*                                                                         *
*   TOOL-CHAIN  : SDE & Cygnus                                            *
*                                                                         *
*   DESCRIPTION :                                                         *
*   MIPS processor definitions.                                           *
*   The basic CPU definitions are found in the file archdefs.h, which     *
*   is included by mips.h.                                                *
*                                                                         *
*   mips.h implements aliases for some of the definitions in archdefs.h   *
*   and adds various definitions.                                         *
*                                                                         *
**************************************************************************/


#ifndef __MIPS_H__
#define __MIPS_H__

#include "mips-archdefs.h"


/* ********************************************************************* */
/* Module configuration */


/* ********************************************************************* */
/* Interface macro & data definition */

#ifndef MSK
#define MSK(n)			  ((1 << (n)) - 1)
#endif

/* CPU registers */
#define SYS_CPUREG_ZERO	0
#define SYS_CPUREG_AT	1
#define SYS_CPUREG_V0	2
#define SYS_CPUREG_V1	3
#define SYS_CPUREG_A0	4
#define SYS_CPUREG_A1	5
#define SYS_CPUREG_A2	6
#define SYS_CPUREG_A3	7
#define SYS_CPUREG_T0	8
#define SYS_CPUREG_T1	9
#define SYS_CPUREG_T2	10
#define SYS_CPUREG_T3	11
#define SYS_CPUREG_T4	12
#define SYS_CPUREG_T5	13
#define SYS_CPUREG_T6	14
#define SYS_CPUREG_T7	15
#define SYS_CPUREG_S0	16
#define SYS_CPUREG_S1	17
#define SYS_CPUREG_S2	18
#define SYS_CPUREG_S3	19
#define SYS_CPUREG_S4	20
#define SYS_CPUREG_S5	21
#define SYS_CPUREG_S6	22
#define SYS_CPUREG_S7	23
#define SYS_CPUREG_T8	24
#define SYS_CPUREG_T9	25
#define SYS_CPUREG_K0	26
#define SYS_CPUREG_K1	27
#define SYS_CPUREG_GP	28
#define SYS_CPUREG_SP	29
#define SYS_CPUREG_S8	30
#define SYS_CPUREG_FP	SYS_CPUREG_S8		
#define SYS_CPUREG_RA	31


/* CPU register fp ($30) has an alias s8 */
#define s8		fp


/* Aliases for System Control Coprocessor (CP0) registers */
#define C0_INDEX	C0_Index
#define C0_RANDOM	C0_Random
#define C0_ENTRYLO0	C0_EntryLo0
#define C0_ENTRYLO1	C0_EntryLo1
#define C0_CONTEXT	C0_Context
#define C0_PAGEMASK	C0_PageMask
#define C0_WIRED	C0_Wired
#define C0_BADVADDR	C0_BadVAddr
#define C0_COUNT 	C0_Count
#define C0_ENTRYHI	C0_EntryHi
#define C0_COMPARE	C0_Compare
#define C0_STATUS	C0_Status
#define C0_CAUSE	C0_Cause

#ifdef C0_PRID	/* ArchDefs has an obsolete def. of C0_PRID */
#undef C0_PRID
#endif
#define C0_PRID		C0_PRId

#define C0_CONFIG	C0_Config
#define C0_CONFIG1	C0_Config1
#define C0_LLADDR	C0_LLAddr
#define C0_WATCHLO	C0_WatchLo
#define C0_WATCHHI	C0_WatchHi
#define C0_DEBUG        C0_Debug
#define C0_PERFCNT      C0_PerfCnt
#define C0_ERRCTL	C0_ErrCtl
#define C0_CACHEERR	C0_CacheErr
#define C0_TAGLO	C0_TagLo
#define C0_DATALO	C0_DataLo
#define C0_TAGHI	C0_TagHi
#define C0_DATAHI	C0_DataHi
#define C0_ERROREPC	C0_ErrorEPC
#if 0
#define C0_DESAVE	C0_DESAVE
#define C0_EPC		C0_EPC
#define C0_DEPC         C0_DEPC
#endif

/* System Control Coprocessor (CP0) registers select fields */
#define C0_INDEX_SEL	0		/* TLB Index */
#define C0_RANDOM_SEL	0		/* TLB Random */
#define C0_TLBLO0_SEL	0		/* TLB EntryLo0 */
#define C0_TLBLO1_SEL	0		/* TLB EntryLo1 */
#define C0_CONTEXT_SEL	0		/* Context */
#define C0_PAGEMASK_SEL	0		/* TLB PageMask */
#define C0_WIRED_SEL	0		/* TLB Wired */
#define C0_BADVADDR_SEL	0		/* Bad Virtual Address */
#define C0_COUNT_SEL	0		/* Count */
#define C0_ENTRYHI_SEL	0		/* TLB EntryHi */
#define C0_COMPARE_SEL	0		/* Compare */
#define C0_STATUS_SEL	0		/* Processor Status */
#define C0_CAUSE_SEL	0		/* Exception Cause */
#define C0_EPC_SEL	0		/* Exception PC */
#define C0_PRID_SEL	0		/* Processor Revision Indentifier */
#define C0_CONFIG_SEL	0		/* Config */
#define C0_CONFIG1_SEL	1		/* Config1 */
#define C0_LLADDR_SEL	0		/* LLAddr */
#define C0_WATCHLO_SEL	0		/* WatchpointLo */
#define C0_WATCHHI_SEL	0		/* WatchpointHi */
#define C0_DEBUG_SEL    0		/* EJTAG Debug Register */
#define C0_DEPC_SEL     0		/* Program counter at last EJTAG debug exception */
#define C0_PERFCNT_SEL  0		/* Performance counter interface */
#define C0_ERRCTL_SEL	0		/* ERRCTL */
#define C0_CACHEERR_SEL	0		/* CacheErr */
#define C0_TAGLO_SEL	0		/* TagLo */
#define C0_DATALO_SEL	1		/* DataLo */
#define C0_DTAGLO_SEL	2		/* DTagLo */
#define C0_TAGHI_SEL	0		/* TagHi */
#define C0_DATAHI_SEL	1		/* DataHi */
#define C0_DTAGHI_SEL	2		/* DTagHi */
#define C0_ERROREPC_SEL	0		/* ErrorEPC */
#define C0_DESAVE_SEL	0		/* EJTAG dbg exc. save register */


/* C0_CONFIG register encoding */

#define C0_CONFIG_M_SHF			S_ConfigMore
#define C0_CONFIG_M_MSK     		M_ConfigMore
#define C0_CONFIG_M_BIT			C0_CONFIG_M_MSK

#define C0_CONFIG_BE_SHF		S_ConfigBE
#define C0_CONFIG_BE_MSK    		M_ConfigBE
#define C0_CONFIG_BE_BIT		C0_CONFIG_BE_MSK

#define C0_CONFIG_AT_SHF		S_ConfigAT
#define C0_CONFIG_AT_MSK		M_ConfigAT
#define C0_CONFIG_AT_MIPS32		K_ConfigAT_MIPS32
#define C0_CONFIG_AT_MIPS64_32ADDR	K_ConfigAT_MIPS64S
#define C0_CONFIG_AT_MIPS64		K_ConfigAT_MIPS64

#define C0_CONFIG_AR_SHF		S_ConfigAR
#define C0_CONFIG_AR_MSK		M_ConfigAR

#define C0_CONFIG_MT_SHF		S_ConfigMT
#define C0_CONFIG_MT_MSK		M_ConfigMT
#define C0_CONFIG_MT_NONE		K_ConfigMT_NoMMU
#define C0_CONFIG_MT_TLB		K_ConfigMT_TLBMMU
#define C0_CONFIG_MT_BAT		K_ConfigMT_BATMMU
#define C0_CONFIG_MT_NON_STD		K_ConfigMT_FMMMU

#define C0_CONFIG_K0_SHF		S_ConfigK0
#define C0_CONFIG_K0_MSK		M_ConfigK0
#define C0_CONFIG_K0_WTHRU_NOALLOC	K_CacheAttrCWTnWA
#define C0_CONFIG_K0_WTHRU_ALLOC	K_CacheAttrCWTWA
#define C0_CONFIG_K0_UNCACHED		K_CacheAttrU
#define C0_CONFIG_K0_NONCOHERENT	K_CacheAttrCN
#define C0_CONFIG_K0_COHERENTXCL	K_CacheAttrCCE
#define C0_CONFIG_K0_COHERENTXCLW	K_CacheAttrCCS
#define C0_CONFIG_K0_COHERENTUPD	K_CacheAttrCCU
#define C0_CONFIG_K0_UNCACHED_ACCEL	K_CacheAttrUA


/*  WC field.
 *
 *  This feature is present specifically to support configuration
 *  testing of the core in a lead vehicle, and is not supported
 *  in any other environment.  Attempting to use this feature
 *  outside of the scope of a lead vehicle is a violation of the
 *  MIPS Architecture, and may cause unpredictable operation of
 *  the processor.
 */
#define C0_CONFIG_WC_SHF		19
#define C0_CONFIG_WC_MSK    		(MSK(1) << C0_CONFIG_WC_SHF)
#define C0_CONFIG_WC_BIT		C0_CONFIG_WC_MSK


/* C0_CONFIG1 register encoding */

#define C0_CONFIG1_MMUSIZE_SHF		S_Config1MMUSize
#define C0_CONFIG1_MMUSIZE_MSK		M_Config1MMUSize

#define C0_CONFIG1_IS_SHF		S_Config1IS
#define C0_CONFIG1_IS_MSK		M_Config1IS

#define C0_CONFIG1_IL_SHF		S_Config1IL
#define C0_CONFIG1_IL_MSK		M_Config1IL

#define C0_CONFIG1_IA_SHF		S_Config1IA
#define C0_CONFIG1_IA_MSK		M_Config1IA

#define C0_CONFIG1_DS_SHF		S_Config1DS
#define C0_CONFIG1_DS_MSK		M_Config1DS

#define C0_CONFIG1_DL_SHF		S_Config1DL
#define C0_CONFIG1_DL_MSK		M_Config1DL

#define C0_CONFIG1_DA_SHF		S_Config1DA
#define C0_CONFIG1_DA_MSK		M_Config1DA

#define C0_CONFIG1_WR_SHF		S_Config1WR
#define C0_CONFIG1_WR_MSK		M_Config1WR
#define C0_CONFIG1_WR_BIT		C0_CONFIG1_WR_MSK

#define C0_CONFIG1_CA_SHF		S_Config1CA
#define C0_CONFIG1_CA_MSK		M_Config1CA
#define C0_CONFIG1_CA_BIT		C0_CONFIG1_CA_MSK

#define C0_CONFIG1_EP_SHF		S_Config1EP
#define C0_CONFIG1_EP_MSK		M_Config1EP
#define C0_CONFIG1_EP_BIT		C0_CONFIG1_EP_MSK

#define C0_CONFIG1_FP_SHF		S_Config1FP
#define C0_CONFIG1_FP_MSK		M_Config1FP
#define C0_CONFIG1_FP_BIT		C0_CONFIG1_FP_MSK


/* C0_STATUS register encoding */

#define C0_STATUS_CU3_SHF		S_StatusCU3
#define C0_STATUS_CU3_MSK		M_StatusCU3
#define C0_STATUS_CU3_BIT		C0_STATUS_CU3_MSK

#define C0_STATUS_CU2_SHF		S_StatusCU2
#define C0_STATUS_CU2_MSK		M_StatusCU2
#define C0_STATUS_CU2_BIT		C0_STATUS_CU2_MSK

#define C0_STATUS_CU1_SHF		S_StatusCU1
#define C0_STATUS_CU1_MSK		M_StatusCU1
#define C0_STATUS_CU1_BIT		C0_STATUS_CU1_MSK

#define C0_STATUS_CU0_SHF		S_StatusCU1
#define C0_STATUS_CU0_MSK		M_StatusCU1
#define C0_STATUS_CU0_BIT		C0_STATUS_CU0_MSK

#define C0_STATUS_RP_SHF		S_StatusRP
#define C0_STATUS_RP_MSK		M_StatusRP
#define C0_STATUS_RP_BIT		C0_STATUS_RP_MSK

#define C0_STATUS_FR_SHF		S_StatusFR
#define C0_STATUS_FR_MSK		M_StatusFR
#define C0_STATUS_FR_BIT		C0_STATUS_FR_MSK

#define C0_STATUS_RE_SHF		S_StatusRE
#define C0_STATUS_RE_MSK		M_StatusRE
#define C0_STATUS_RE_BIT		C0_STATUS_RE_MSK

#define C0_STATUS_BEV_SHF		S_StatusBEV
#define C0_STATUS_BEV_MSK		M_StatusBEV
#define C0_STATUS_BEV_BIT		C0_STATUS_BEV_MSK

#define C0_STATUS_TS_SHF		S_StatusTS
#define C0_STATUS_TS_MSK		M_StatusTS
#define C0_STATUS_TS_BIT		C0_STATUS_TS_MSK

#define C0_STATUS_SR_SHF		S_StatusSR
#define C0_STATUS_SR_MSK		M_StatusSR
#define C0_STATUS_SR_BIT		C0_STATUS_SR_MSK

#define C0_STATUS_NMI_SHF		S_StatusNMI
#define C0_STATUS_NMI_MSK		M_StatusNMI
#define C0_STATUS_NMI_BIT		C0_STATUS_NMI_MSK

#define C0_STATUS_IM_SHF		S_StatusIM
#define C0_STATUS_IM_MSK		M_StatusIM
/*  Note that the the definitions below indicate the interrupt number
 *  rather than the mask.
 *  (0..1 for SW interrupts and 2...7 for HW interrupts)
 */
#define C0_STATUS_IM_SW0		(S_StatusIM0 - S_StatusIM)
#define C0_STATUS_IM_SW1		(S_StatusIM1 - S_StatusIM)
#define C0_STATUS_IM_HW0		(S_StatusIM2 - S_StatusIM)
#define C0_STATUS_IM_HW1		(S_StatusIM3 - S_StatusIM)
#define C0_STATUS_IM_HW2		(S_StatusIM4 - S_StatusIM)
#define C0_STATUS_IM_HW3		(S_StatusIM5 - S_StatusIM)
#define C0_STATUS_IM_HW4		(S_StatusIM6 - S_StatusIM)
#define C0_STATUS_IM_HW5		(S_StatusIM7 - S_StatusIM)

/* Max interrupt code */
#define C0_STATUS_IM_MAX		C0_STATUS_IM_HW5

#define C0_STATUS_KSU_SHF		S_StatusKSU
#define C0_STATUS_KSU_MSK		M_StatusKSU

#define C0_STATUS_UM_SHF		S_StatusUM
#define C0_STATUS_UM_MSK		M_StatusUM
#define C0_STATUS_UM_BIT		C0_STATUS_UM_MSK

#define C0_STATUS_ERL_SHF		S_StatusERL
#define C0_STATUS_ERL_MSK		M_StatusERL
#define C0_STATUS_ERL_BIT		C0_STATUS_ERL_MSK

#define C0_STATUS_EXL_SHF		S_StatusEXL
#define C0_STATUS_EXL_MSK		M_StatusEXL
#define C0_STATUS_EXL_BIT		C0_STATUS_EXL_MSK

#define C0_STATUS_IE_SHF		S_StatusIE
#define C0_STATUS_IE_MSK		M_StatusIE
#define C0_STATUS_IE_BIT		C0_STATUS_IE_MSK


/* C0_PRID register encoding */

#define C0_PRID_OPT_SHF			S_PRIdCoOpt
#define C0_PRID_OPT_MSK			M_PRIdCoOpt

#define C0_PRID_COMP_SHF		S_PRIdCoID
#define C0_PRID_COMP_MSK		M_PRIdCoID
#define C0_PRID_COMP_MIPS		K_PRIdCoID_MIPS
#define C0_PRID_COMP_NOT_MIPS32_64	0

#define C0_PRID_PRID_SHF		S_PRIdImp
#define C0_PRID_PRID_MSK		M_PRIdImp

/* Jade */
#define C0_PRID_PRID_4Kc		K_PRIdImp_Jade
#define C0_PRID_PRID_4Kmp		K_PRIdImp_JadeLite  /* 4Km/4Kp */
/* Emerald */
#define C0_PRID_PRID_4KEc		K_PRIdImp_4KEc
#define C0_PRID_PRID_4KEmp		K_PRIdImp_4KEmp
/* Coral */
#define C0_PRID_PRID_4KSc	        K_PRIdImp_4KSc	
/* Opal */
#define C0_PRID_PRID_5K			K_PRIdImp_Opal
/* Ruby */
#define C0_PRID_PRID_20Kc		K_PRIdImp_Ruby
/* Other CPUs */
#define C0_PRID_PRID_R4000  		K_PRIdImp_R4000
#define C0_PRID_PRID_RM52XX  		K_PRIdImp_R5200
#define C0_PRID_PRID_RM70XX  		0x27

#define C0_PRID_REV_SHF			S_PRIdRev
#define C0_PRID_REV_MSK			M_PRIdRev


#define MIPS_4Kc			( (C0_PRID_COMP_MIPS <<   \
					      C0_PRID_COMP_SHF) | \
					  (C0_PRID_PRID_4Kc  <<   \
					      C0_PRID_PRID_SHF)   \
					)

#define MIPS_4Kmp			( (C0_PRID_COMP_MIPS <<   \
					      C0_PRID_COMP_SHF) | \
					  (C0_PRID_PRID_4Kmp  <<  \
					      C0_PRID_PRID_SHF)   \
					)

#define MIPS_4KEc			( (C0_PRID_COMP_MIPS <<   \
					      C0_PRID_COMP_SHF) | \
					  (C0_PRID_PRID_4KEc  <<  \
					      C0_PRID_PRID_SHF)   \
					)

#define MIPS_4KEmp			( (C0_PRID_COMP_MIPS <<   \
					      C0_PRID_COMP_SHF) | \
					  (C0_PRID_PRID_4KEmp  << \
					      C0_PRID_PRID_SHF)   \
					)

#define MIPS_4KSc			( (C0_PRID_COMP_MIPS <<   \
					      C0_PRID_COMP_SHF) | \
					  (C0_PRID_PRID_4KSc  <<  \
					      C0_PRID_PRID_SHF)   \
					)

#define MIPS_5K				( (C0_PRID_COMP_MIPS <<   \
					      C0_PRID_COMP_SHF) | \
					  (C0_PRID_PRID_5K  <<    \
					      C0_PRID_PRID_SHF)   \
					)

#define MIPS_20Kc			( (C0_PRID_COMP_MIPS <<   \
					      C0_PRID_COMP_SHF) | \
					  (C0_PRID_PRID_20Kc  <<  \
					      C0_PRID_PRID_SHF)   \
					)

#define QED_RM52XX			( (C0_PRID_COMP_NOT_MIPS32_64 << \
					      C0_PRID_COMP_SHF) |	 \
					  (C0_PRID_PRID_RM52XX  <<	 \
					      C0_PRID_PRID_SHF)		 \
					)

#define QED_RM70XX			( (C0_PRID_COMP_NOT_MIPS32_64 << \
					      C0_PRID_COMP_SHF) |	 \
					  (C0_PRID_PRID_RM70XX  <<	 \
					      C0_PRID_PRID_SHF)		 \
					)

/* C0_ENTRYHI register encoding */

#define C0_ENTRYHI_VPN2_SHF		S_EntryHiVPN2
#define C0_ENTRYHI_VPN2_MSK		M_EntryHiVPN2

#define C0_ENTRYHI_ASID_SHF		S_EntryHiASID
#define C0_ENTRYHI_ASID_MSK		M_EntryHiASID


/* C0_CAUSE register encoding */

#define C0_CAUSE_BD_SHF			S_CauseBD
#define C0_CAUSE_BD_MSK			M_CauseBD
#define C0_CAUSE_BD_BIT			C0_CAUSE_BD_MSK

#define C0_CAUSE_CE_SHF			S_CauseCE
#define C0_CAUSE_CE_MSK			M_CauseCE

#define C0_CAUSE_IV_SHF			S_CauseIV
#define C0_CAUSE_IV_MSK			M_CauseIV
#define C0_CAUSE_IV_BIT			C0_CAUSE_IV_MSK

#define C0_CAUSE_WP_SHF			S_CauseWP
#define C0_CAUSE_WP_MSK			M_CauseWP
#define C0_CAUSE_WP_BIT			C0_CAUSE_WP_MSK

#define C0_CAUSE_IP_SHF			S_CauseIP
#define C0_CAUSE_IP_MSK			M_CauseIP

#define C0_CAUSE_CODE_SHF		S_CauseExcCode
#define C0_CAUSE_CODE_MSK		M_CauseExcCode

#define C0_CAUSE_CODE_INT		EX_INT
#define C0_CAUSE_CODE_MOD		EX_MOD
#define C0_CAUSE_CODE_TLBL		EX_TLBL
#define C0_CAUSE_CODE_TLBS		EX_TLBS
#define C0_CAUSE_CODE_ADEL		EX_ADEL
#define C0_CAUSE_CODE_ADES		EX_ADES
#define C0_CAUSE_CODE_IBE		EX_IBE
#define C0_CAUSE_CODE_DBE		EX_DBE
#define C0_CAUSE_CODE_SYS		EX_SYS
#define C0_CAUSE_CODE_BP		EX_BP
#define C0_CAUSE_CODE_RI		EX_RI
#define C0_CAUSE_CODE_CPU		EX_CPU
#define C0_CAUSE_CODE_OV		EX_OV
#define C0_CAUSE_CODE_TR		EV_TR
#define C0_CAUSE_CODE_FPE		EX_FPE
#define C0_CAUSE_CODE_WATCH		EX_WATCH
#define C0_CAUSE_CODE_MCHECK		EX_MCHECK

/* Max cause code */
#define C0_CAUSE_CODE_MAX		EX_MCHECK


/* C0_PAGEMASK register encoding */
#define C0_PAGEMASK_MASK_SHF		S_PageMaskMask
#define C0_PAGEMASK_MASK_MSK		M_PageMaskMask
#define C0_PAGEMASK_MASK_4K		K_PageMask4K
#define C0_PAGEMASK_MASK_16K		K_PageMask16K
#define C0_PAGEMASK_MASK_64K		K_PageMask64K
#define C0_PAGEMASK_MASK_256K		K_PageMask256K
#define C0_PAGEMASK_MASK_1M		K_PageMask1M
#define C0_PAGEMASK_MASK_4M		K_PageMask4M
#define C0_PAGEMASK_MASK_16M		K_PageMask16M


/* C0_ENTRYLO0 register encoding (equiv. to C0_ENTRYLO1) */
#define C0_ENTRYLO0_PFN_SHF		S_EntryLoPFN
#define C0_ENTRYLO0_PFN_MSK		M_EntryLoPFN

#define C0_ENTRYLO0_C_SHF		S_EntryLoC
#define C0_ENTRYLO0_C_MSK		M_EntryLoC

#define C0_ENTRYLO0_D_SHF		S_EntryLoD
#define C0_ENTRYLO0_D_MSK		M_EntryLoD

#define C0_ENTRYLO0_V_SHF		S_EntryLoV
#define C0_ENTRYLO0_V_MSK		M_EntryLoV

#define C0_ENTRYLO0_G_SHF		S_EntryLoG
#define C0_ENTRYLO0_G_MSK		M_EntryLoG


/* FPU (CP1) FIR register encoding */
#define C1_FIR_3D_SHF		S_FIRConfig3D
#define C1_FIR_3D_MSK		M_FIRConfig3D

#define C1_FIR_PS_SHF		S_FIRConfigPS
#define C1_FIR_PS_MSK		M_FIRConfigPS

#define C1_FIR_D_SHF		S_FIRConfigD
#define C1_FIR_D_MSK		M_FIRConfigD

#define C1_FIR_S_SHF		S_FIRConfigS
#define C1_FIR_S_MSK		M_FIRConfigS

#define C1_FIR_PRID_SHF		S_FIRImp
#define C1_FIR_PRID_MSK		M_FIRImp

#define C1_FIR_REV_SHF		S_FIRRev
#define C1_FIR_REV_MSK		M_FIRRev


/* FPU (CP1) FCSR control/status register */
#define C1_FCSR_FCC_SHF		S_FCSRFCC7_1
#define C1_FCSR_FCC_MSK		M_FCSRFCC7_1

#define C1_FCSR_FS_SHF		S_FCSRFS
#define C1_FCSR_FS_MSK		M_FCSRFS
#define C1_FCSR_FS_BIT		C1_FCSR_FS_MSK

#define C1_FCSR_CC_SHF		S_FCSRCC
#define C1_FCSR_CC_MSK		M_FCSRCC

#define C1_FCSR_IMPL_SHF	S_FCSRImpl
#define C1_FCSR_IMPL_MSK	M_FCSRImpl

#define C1_FCSR_EXC_SHF		S_FCSRExc
#define C1_FCSR_EXC_MSK		M_FCSRExc

#define C1_FCSR_ENA_SHF		S_FCSREna
#define C1_FCSR_ENA_MSK		M_FCSREna

#define C1_FCSR_FLG_SHF		S_FCSRFlg
#define C1_FCSR_FLG_MSK		M_FCSRFlg

#define C1_FCSR_RM_SHF		S_FCSRRM
#define C1_FCSR_RM_MSK		M_FCSRRM
#define C1_FCSR_RM_RN		K_FCSRRM_RN
#define C1_FCSR_RM_RZ		K_FCSRRM_RZ
#define C1_FCSR_RM_RP		K_FCSRRM_RP
#define C1_FCSR_RM_RM		K_FCSRRM_RM



/* cache operations */

#define CACHE_OP( code, type )			( ((code) << 2) | (type) )

#define ICACHE_INDEX_INVALIDATE			CACHE_OP(0x0, 0)
#define ICACHE_INDEX_LOAD_TAG			CACHE_OP(0x1, 0)
#define ICACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 0)
#define DCACHE_INDEX_WRITEBACK_INVALIDATE	CACHE_OP(0x0, 1)
#define DCACHE_INDEX_LOAD_TAG			CACHE_OP(0x1, 1)
#define DCACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 1)
#define SCACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 3)

#define ICACHE_ADDR_HIT_INVALIDATE		CACHE_OP(0x4, 0)
#define ICACHE_ADDR_FILL			CACHE_OP(0x5, 0)
#define ICACHE_ADDR_FETCH_LOCK			CACHE_OP(0x7, 0)
#define DCACHE_ADDR_HIT_INVALIDATE		CACHE_OP(0x4, 1)
#define DCACHE_ADDR_HIT_WRITEBACK_INVALIDATE	CACHE_OP(0x5, 1)
#define DCACHE_ADDR_HIT_WRITEBACK		CACHE_OP(0x6, 1)
#define DCACHE_ADDR_FETCH_LOCK			CACHE_OP(0x7, 1)

#define SCACHE_ADDR_HIT_WRITEBACK_INVALIDATE	CACHE_OP(0x5, 3)

/*  Workaround for bug in early revisions of MIPS 4K family of 
 *  processors. Only relevant in early engineering samples of test
 *  chips (RTL revision <= 3.0).
 *
 *  The bug is described in :
 *
 *  MIPS32 4K(tm) Processor Core Family RTL Errata Sheet
 *  MIPS Document No: MD00003
 *
 *  The bug is identified as : C16
 */
#ifndef SET_MIPS0
#define SET_MIPS0()
#define SET_PUSH()
#define SET_POP()
#endif
#define ICACHE_INVALIDATE_WORKAROUND(reg) \
SET_PUSH();				  \
SET_MIPS0();				  \
	la     reg, 999f;		  \
SET_POP();				  \
	cache  ICACHE_ADDR_FILL, 0(reg);  \
	sync;				  \
	nop; nop; nop; nop;		  \
999:

/*  EMPTY_PIPELINE is used for the below cache invalidation operations.
 *  When $I is invalidated, there will still be operations in the
 *  pipeline. We make sure these are 'nop' operations.
 */
#define EMPTY_PIPELINE		nop; nop; nop; nop

#define ICACHE_INDEX_INVALIDATE_OP(index,scratch)		  \
	    ICACHE_INVALIDATE_WORKAROUND(scratch);		  \
	    cache ICACHE_INDEX_INVALIDATE, 0(index);		  \
	    EMPTY_PIPELINE

#define ICACHE_ADDR_INVALIDATE_OP(addr,scratch)			  \
	    ICACHE_INVALIDATE_WORKAROUND(scratch);		  \
	    cache ICACHE_ADDR_HIT_INVALIDATE, 0(addr);		  \
	    EMPTY_PIPELINE

/*  The sync used in the below macro is there in case we are installing
 *  a new instruction (flush $D, sync, invalidate $I sequence).
 */
#define SCACHE_ADDR_HIT_WB_INVALIDATE_OP(reg)			  \
	    cache   SCACHE_ADDR_HIT_WRITEBACK_INVALIDATE, 0(reg); \
	    sync;						  \
	    EMPTY_PIPELINE

/* Config1 cache field decoding */
#define CACHE_CALC_SPW(s)	( 64 << (s) )
#define CACHE_CALC_LS(l)	( (l) ? 2 << (l) : 0 )
#define CACHE_CALC_BPW(l,s)	( CACHE_CALC_LS(l) * CACHE_CALC_SPW(s) )
#define CACHE_CALC_ASSOC(a)	( (a) + 1 )


/**** Move from/to Coprocessor operations ****/

/*  We use ssnop instead of nop operations in order to handle 
 *  superscalar CPUs.
 *  The "sll zero,zero,1" notation is compiler backwards compatible.
 */
#define SSNOP   sll zero,zero,1
#define NOPS	SSNOP; SSNOP; SSNOP; SSNOP

#define MFLO(dst)        \
		mflo dst;\
 	  	NOPS

/*  Workaround for bug in early revisions of MIPS 4K family of 
 *  processors.
 *
 *  This concerns the nop instruction before mtc0 in the 
 *  MTC0 macro below.
 *
 *  The bug is described in :
 *
 *  MIPS32 4K(tm) Processor Core Family RTL Errata Sheet
 *  MIPS Document No: MD00003
 *
 *  The bug is identified as : C27
 */

#define MTC0(src, dst)       \
		nop;	     \
	    mtc0 src,dst;\
		NOPS

#define DMTC0(src, dst)       \
		nop;	      \
	    dmtc0 src,dst;\
		NOPS

#define MFC0(dst, src)       \
	  	mfc0 dst,src;\
		NOPS

#define DMFC0(dst, src)       \
	  	dmfc0 dst,src;\
		NOPS

#define MFC0_SEL_OPCODE(dst, src, sel)\
	  	.##word (0x40000000 | ((dst)<<16) | ((src)<<11) | (sel));\
		NOPS

#define MTC0_SEL_OPCODE(dst, src, sel)\
	  	.##word (0x40800000 | ((dst)<<16) | ((src)<<11) | (sel));\
		NOPS

#define LDC1(dst, src, offs)\
		.##word (0xd4000000 | ((src)<<21) | ((dst)<<16) | (offs))

#define SDC1(src, dst, offs)\
		.##word (0xf4000000 | ((dst)<<21) | ((src)<<16) | (offs))


/* Instruction opcode fields */
#define OPC_SPECIAL   0x0
#define OPC_REGIM     0x1
#define OPC_J         0x2
#define OPC_JAL	      0x3
#define OPC_BEQ	      0x4
#define OPC_BNE	      0x5
#define OPC_BLEZ      0x6
#define OPC_BGTZ      0x7
#define OPC_COP1      0x11
#define OPC_JALX      0x1D
#define OPC_BEQL      0x14
#define OPC_BNEL      0x15
#define OPC_BLEZL     0x16
#define OPC_BGTZL     0x17

/* Instruction function fields */
#define FUNC_JR	      0x8
#define FUNC_JALR     0x9

/* Instruction rt fields */
#define RT_BLTZ	      0x0
#define RT_BGEZ	      0x1
#define RT_BLTZL      0x2
#define RT_BGEZL      0x3
#define RT_BLTZAL     0x10
#define RT_BGEZAL     0x11
#define RT_BLTZALL    0x12
#define RT_BGEZALL    0x13

/* Instruction rs fields */
#define RS_BC1	      0x08

/* Access macros for instruction fields */
#define MIPS_OPCODE( instr)	((instr) >> 26)
#define MIPS_FUNCTION(instr)	((instr) & MSK(6))
#define MIPS_RT(instr)		(((instr) >> 16) & MSK(5))
#define MIPS_RS(instr)		(((instr) >> 21) & MSK(5))
#define MIPS_OFFSET(instr)	((instr) & 0xFFFF)
#define MIPS_TARGET(instr)	((instr) & MSK(26))

/* Instructions */
#define OPCODE_DERET		0x4200001f
#define OPCODE_BREAK	  	0x0005000d
#define OPCODE_NOP		0
#define OPCODE_JUMP(addr)	( (OPC_J << 26) | (((addr) >> 2) & 0x3FFFFFF) )

#define DERET			.##word OPCODE_DERET

/* MIPS16e opcodes and instruction field access macros */

#define MIPS16E_OPCODE(inst)		(((inst) >> 11) & 0x1f)
#define MIPS16E_I8_FUNCTION(inst)	(((inst) >>  8) & 0x7)
#define MIPS16E_X(inst) 		(((inst) >> 26) & 0x1)
#define MIPS16E_RR_FUNCTION(inst)	(((inst) >>  0) & 0x1f)
#define MIPS16E_RY(inst)		(((inst) >>  5) & 0x3)
#define MIPS16E_OPC_EXTEND		0x1e
#define MIPS16E_OPC_JAL_X		0x03
#define MIPS16E_OPC_B			0x02
#define MIPS16E_OPC_BEQZ		0x04
#define MIPS16E_OPC_BNEZ		0x05
#define MIPS16E_OPC_I8			0x0c
#define MIPS16E_I8_FUNC_BTEQZ		0x00
#define MIPS16E_I8_FUNC_BTNEZ		0x01
#define MIPS16E_X_JALX			0x01
#define MIPS16E_OPC_RR			0x1d
#define MIPS16E_RR_FUNC_JALRC		0x00
#define MIPS16E_RR_RY_JRRX		0x00
#define MIPS16E_RR_RY_JRRA		0x01
#define MIPS16E_RR_RY_JALR		0x02
#define MIPS16E_RR_RY_JRCRX		0x04
#define MIPS16E_RR_RY_JRCRA		0x05
#define MIPS16E_RR_RY_JALRC		0x06

#define MIPS16E_OPCODE_BREAK		0xE805
#define MIPS16E_OPCODE_NOP		0x6500

/* MIPS reset vector */
#define MIPS_RESET_VECTOR       0x1fc00000

/* Clock periods per count register increment */
#define MIPS4K_COUNT_CLK_PER_CYCLE	2
#define MIPS5K_COUNT_CLK_PER_CYCLE	2
#define MIPS20Kc_COUNT_CLK_PER_CYCLE	1


/**** MIPS 4K/5K families specific fields of CONFIG register ****/

#define C0_CONFIG_MIPS4K5K_K23_SHF   S_ConfigK23
#define C0_CONFIG_MIPS4K5K_K23_MSK   (MSK(3) << C0_CONFIG_MIPS4K5K_K23_SHF)

#define C0_CONFIG_MIPS4K5K_KU_SHF    S_ConfigKU
#define C0_CONFIG_MIPS4K5K_KU_MSK    (MSK(3) << C0_CONFIG_MIPS4K5K_KU_SHF)


/**** MIPS 20Kc specific fields of CONFIG register ****/

#define C0_CONFIG_MIPS20KC_EC_SHF    28
#define C0_CONFIG_MIPS20KC_EC_MSK    (MSK(3) << C0_CONFIG_MIPS20KC_EC_SHF)

#define C0_CONFIG_MIPS20KC_DD_SHF    27
#define C0_CONFIG_MIPS20KC_DD_MSK    (MSK(1) << C0_CONFIG_MIPS20KC_DD_SHF)
#define C0_CONFIG_MIPS20KC_DD_BIT    C0_CONFIG_MIPS20KC_DD_MSK

#define C0_CONFIG_MIPS20KC_LP_SHF    26
#define C0_CONFIG_MIPS20KC_LP_MSK    (MSK(1) << C0_CONFIG_MIPS20KC_LP_SHF)
#define C0_CONFIG_MIPS20KC_LP_BIT    C0_CONFIG_MIPS20KC_LP_MSK

#define C0_CONFIG_MIPS20KC_SP_SHF    25
#define C0_CONFIG_MIPS20KC_SP_MSK    (MSK(1) << C0_CONFIG_MIPS20KC_SP_SHF)
#define C0_CONFIG_MIPS20KC_SP_BIT    C0_CONFIG_MIPS20KC_SP_MSK

#define C0_CONFIG_MIPS20KC_TI_SHF    24
#define C0_CONFIG_MIPS20KC_TI_MSK    (MSK(1) << C0_CONFIG_MIPS20KC_TI_SHF)
#define C0_CONFIG_MIPS20KC_TI_BIT    C0_CONFIG_MIPS20KC_TI_MSK


/* ********************************************************************* */
/* Interface function definition */


/* ********************************************************************* */

#endif /* #ifndef __MIPS_H__ */
