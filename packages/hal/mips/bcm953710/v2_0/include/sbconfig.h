/*
 * Broadcom SiliconBackplane hardware register definitions.
 *
 * Copyright 2004, Broadcom Corporation      
 * All Rights Reserved.      
 *       
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY      
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM      
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS      
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.      
 * $Id: sbconfig.h,v 1.1.2.1 Exp $
 */

#ifndef	_SBCONFIG_H
#define	_SBCONFIG_H

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif


#define BCM53710_SDRAM		0x00000000 /* 0-128MB Physical SDRAM */
#define BCM53710_SDRAM_SZ	0x08000000 /* 128MB Physical SDRAM */
#define	BCM53710_SDRAM_SWAPPED	0x10000000 /* Byteswapped Physical SDRAM */
#define BCM53710_SDRAM_SWAPPED_SZ	0x08000000 /* Byteswapped window size */
#define BCM53710_ENUM		0x18000000 /* Beginning of core enum space */

/* BCM4704 Core register space */
#define BCM53710_REG_CHIPC	0x18000000 /* Chipcommon  registers */
#define BCM53710_REG_MIPS33	0x18005000 /* MIPS core registers */
#define BCM53710_REG_MEMC	0x18008000 /* MEMC core registers */
#define BCM53710_REG_UARTS   (BCM53710_REG_CHIPC + 0x300) /* UART regs */
#define BCM53710_SDRAM_HIGH	0x80000000 /* 0-512MB Physical SDRAM */
#define BCM53710_SDRAM_HIGH_SZ	0x20000000 /* 0-512MB Physical SDRAM */
#define	BCM53710_EJTAG		0xff200000 /* MIPS EJTAG space (2M) */

/* COM Ports 1/2 */
#define	BCM53710_UART		(BCM53710_REG_UARTS)
#define BCM53710_UART_COM2	(BCM53710_REG_UARTS + 0x00000100)

/* Registers common to MIPS33 Core used in 4704 */
#define MIPS33_FLASH_REGION           0x1fc00000 /* Boot FLASH Region  */
#define MIPS33_EXTIF_REGION           0x1a000000 /* Chipcommon EXTIF region*/
#define MIPS33_FLASH_REGION_AUX       0x1c000000 /* FLASH Region 2*/

/* Internal Core Sonics Backplane Devices */
#define INTERNAL_UART_COM1            BCM53710_UART
#define INTERNAL_UART_COM2            BCM53710_UART_COM2
#define SB_REG_CHIPC                  BCM53710_REG_CHIPC
#define SB_REG_MIPS                   BCM53710_REG_MIPS33
#define SB_REG_MEMC                   BCM53710_REG_MEMC
#define SB_FLASH_SPACE                MIPS33_FLASH_REGION

/* RESET register */
#define ICS_SRESET_REG              0xb8000110

#define ICS_WATCHDOG_CTR_REG        0xb8000080

/* timer 0 registers */
#define ICS_TIMER0_INT_STATUS      (volatile uint32*) 0xb8005020
#define ICS_TIMER0_INT_MASK        (volatile uint32*) 0xb8005024
#define ICS_TIMER0_REG             (volatile uint32*) 0xb8005028
#define I_TO                        1

/* For Raptor/Raptor2(133MHz)/HawkEye (100MHz) platforms - 
 * Due to difference in clock frequencies in the above platforms,
 * changes are made to detect Clock frequency dynamically.
 * Modifying the below macro will not have any affect.
 * Please look into the packages/hal/mips/bcm953710/v2_0/src/sbutils.c 
 * for more info.
 */
#define ICS_SB_CLK_FREQ             133000000

/*
 * SiliconBackplane Address Map.
 * All regions may not exist on all chips.
 */
#define SB_SDRAM_BASE		0x00000000	/* Physical SDRAM */
#define SB_PCI_MEM		0x08000000	/* Host Mode PCI memory access space (64 MB) */
#define SB_PCI_CFG		0x0c000000	/* Host Mode PCI configuration space (64 MB) */
#define	SB_SDRAM_SWAPPED	0x10000000	/* Byteswapped Physical SDRAM */
#define SB_ENUM_BASE     0x18000000	/* Enumeration space base */
#define	SB_ENUM_LIM		0x18010000	/* Enumeration space limit */
#define	SB_EXTIF_BASE		0x1f000000	/* External Interface region base address */
#define SB_PCI_DMA		0x40000000	/* Client Mode PCI memory access space (1 GB) */
#define	SB_UART		(SB_ENUM_BASE + 0x300)

/* enumeration space related defs */
#define SB_CORE_SIZE    	0x1000		/* each core gets 4Kbytes for registers */
#define	SB_MAXCORES		((SB_ENUM_LIM - SB_ENUM_BASE)/SB_CORE_SIZE)
#define	SBCONFIGOFF		0xf00		/* core sbconfig regs are top 256bytes of regs */
#define	SBCONFIGSIZE		256		/* sizeof (sbconfig_t) */

/* mips address */
#define	SB_EJTAG		0xff200000	/* MIPS EJTAG space (2M) */

/*
 * Sonics Configuration Space Registers.
 */
#define SBIPSFLAG		0x08
#define SBTPSFLAG		0x18
#define	SBTMERRLOGA		0x48		/* sonics >= 2.3 */
#define	SBTMERRLOG		0x50		/* sonics >= 2.3 */
#define SBADMATCH3		0x60
#define SBADMATCH2		0x68
#define SBADMATCH1		0x70
#define SBIMSTATE		0x90
#define SBINTVEC		0x94
#define SBTMSTATELOW		0x98
#define SBTMSTATEHIGH		0x9c
#define SBBWA0			0xa0
#define SBIMCONFIGLOW		0xa8
#define SBIMCONFIGHIGH		0xac
#define SBADMATCH0		0xb0
#define SBTMCONFIGLOW		0xb8
#define SBTMCONFIGHIGH		0xbc
#define SBBCONFIG		0xc0
#define SBBSTATE		0xc8
#define SBACTCNFG		0xd8
#define	SBFLAGST		0xe8
#define SBIDLOW			0xf8
#define SBIDHIGH		0xfc

#ifndef _LANGUAGE_ASSEMBLY

typedef volatile struct _sbconfig {
	uint32	PAD[2];
	uint32	sbipsflag;		/* initiator port ocp slave flag */
	uint32	PAD[3];
	uint32	sbtpsflag;		/* target port ocp slave flag */
	uint32	PAD[11];
	uint32	sbtmerrloga;		/* (sonics >= 2.3) */
	uint32	PAD;
	uint32	sbtmerrlog;		/* (sonics >= 2.3) */
	uint32	PAD[3];
	uint32	sbadmatch3;		/* address match3 */
	uint32	PAD;
	uint32	sbadmatch2;		/* address match2 */
	uint32	PAD;
	uint32	sbadmatch1;		/* address match1 */
	uint32	PAD[7];
	uint32	sbimstate;		/* initiator agent state */
	uint32	sbintvec;		/* interrupt mask */
	uint32	sbtmstatelow;		/* target state */
	uint32	sbtmstatehigh;		/* target state */
	uint32	sbbwa0;			/* bandwidth allocation table0 */
	uint32	PAD;
	uint32	sbimconfiglow;		/* initiator configuration */
	uint32	sbimconfighigh;		/* initiator configuration */
	uint32	sbadmatch0;		/* address match0 */
	uint32	PAD;
	uint32	sbtmconfiglow;		/* target configuration */
	uint32	sbtmconfighigh;		/* target configuration */
	uint32	sbbconfig;		/* broadcast configuration */
	uint32	PAD;
	uint32	sbbstate;		/* broadcast state */
	uint32	PAD[3];
	uint32	sbactcnfg;		/* activate configuration */
	uint32	PAD[3];
	uint32	sbflagst;		/* current sbflags */
	uint32	PAD[3];
	uint32	sbidlow;		/* identification */
	uint32	sbidhigh;		/* identification */
} sbconfig_t;

#endif /* _LANGUAGE_ASSEMBLY */

/* sbipsflag */
#define	SBIPS_INT1_MASK		0x3f		/* which sbflags get routed to mips interrupt 1 */
#define	SBIPS_INT1_SHIFT	0
#define	SBIPS_INT2_MASK		0x3f00		/* which sbflags get routed to mips interrupt 2 */
#define	SBIPS_INT2_SHIFT	8
#define	SBIPS_INT3_MASK		0x3f0000	/* which sbflags get routed to mips interrupt 3 */
#define	SBIPS_INT3_SHIFT	16
#define	SBIPS_INT4_MASK		0x3f000000	/* which sbflags get routed to mips interrupt 4 */
#define	SBIPS_INT4_SHIFT	24

/* sbtpsflag */
#define	SBTPS_NUM0_MASK		0x3f		/* interrupt sbFlag # generated by this core */
#define	SBTPS_F0EN0		0x40		/* interrupt is always sent on the backplane */

/* sbtmerrlog */
#define	SBTMEL_CM		0x00000007	/* command */
#define	SBTMEL_CI		0x0000ff00	/* connection id */
#define	SBTMEL_EC		0x0f000000	/* error code */
#define	SBTMEL_ME		0x80000000	/* multiple error */

/* sbimstate */
#define	SBIM_PC			0xf		/* pipecount */
#define	SBIM_AP_MASK		0x30		/* arbitration policy */
#define	SBIM_AP_BOTH		0x00		/* use both timeslaces and token */
#define	SBIM_AP_TS		0x10		/* use timesliaces only */
#define	SBIM_AP_TK		0x20		/* use token only */
#define	SBIM_AP_RSV		0x30		/* reserved */
#define	SBIM_IBE		0x20000		/* inbanderror */
#define	SBIM_TO			0x40000		/* timeout */
#define	SBIM_BY			0x01800000	/* busy (sonics >= 2.3) */
#define	SBIM_RJ			0x02000000	/* reject (sonics >= 2.3) */

/* sbtmstatelow */
#define	SBTML_RESET		0x1		/* reset */
#define	SBTML_REJ		0x2		/* reject */
#define	SBTML_CLK		0x10000		/* clock enable */
#define	SBTML_FGC		0x20000		/* force gated clocks on */
#define	SBTML_FL_MASK		0x3ffc0000	/* core-specific flags */
#define	SBTML_PE		0x40000000	/* pme enable */
#define	SBTML_BE		0x80000000	/* bist enable */

/* sbtmstatehigh */
#define	SBTMH_SERR		0x1		/* serror */
#define	SBTMH_INT		0x2		/* interrupt */
#define	SBTMH_BUSY		0x4		/* busy */
#define	SBTMH_TO		0x00000020	/* timeout (sonics >= 2.3) */
#define	SBTMH_FL_MASK		0x1fff0000	/* core-specific flags */
#define	SBTMH_GCR		0x20000000	/* gated clock request */
#define	SBTMH_BISTF		0x40000000	/* bist failed */
#define	SBTMH_BISTD		0x80000000	/* bist done */

/* sbbwa0 */
#define	SBBWA_TAB0_MASK		0xffff		/* lookup table 0 */
#define	SBBWA_TAB1_MASK		0xffff		/* lookup table 1 */
#define	SBBWA_TAB1_SHIFT	16

/* sbimconfiglow */
#define	SBIMCL_STO_MASK		0x7		/* service timeout */
#define	SBIMCL_RTO_MASK		0x70		/* request timeout */
#define	SBIMCL_RTO_SHIFT	4
#define	SBIMCL_CID_MASK		0xff0000	/* connection id */
#define	SBIMCL_CID_SHIFT	16

/* sbimconfighigh */
#define	SBIMCH_IEM_MASK		0xc		/* inband error mode */
#define	SBIMCH_TEM_MASK		0x30		/* timeout error mode */
#define	SBIMCH_TEM_SHIFT	4
#define	SBIMCH_BEM_MASK		0xc0		/* bus error mode */
#define	SBIMCH_BEM_SHIFT	6

/* sbadmatch0 */
#define	SBAM_TYPE_MASK		0x3		/* address type */
#define	SBAM_AD64		0x4		/* reserved */
#define	SBAM_ADINT0_MASK	0xf8		/* type0 size */
#define	SBAM_ADINT0_SHIFT	3
#define	SBAM_ADINT1_MASK	0x1f8		/* type1 size */
#define	SBAM_ADINT1_SHIFT	3
#define	SBAM_ADINT2_MASK	0x1f8		/* type2 size */
#define	SBAM_ADINT2_SHIFT	3
#define	SBAM_ADEN		0x400		/* enable */
#define	SBAM_ADNEG		0x800		/* negative decode */
#define	SBAM_BASE0_MASK		0xffffff00	/* type0 base address */
#define	SBAM_BASE0_SHIFT	8
#define	SBAM_BASE1_MASK		0xfffff000	/* type1 base address for the core */
#define	SBAM_BASE1_SHIFT	12
#define	SBAM_BASE2_MASK		0xffff0000	/* type2 base address for the core */
#define	SBAM_BASE2_SHIFT	16

/* sbtmconfiglow */
#define	SBTMCL_CD_MASK		0xff		/* clock divide */
#define	SBTMCL_CO_MASK		0xf800		/* clock offset */
#define	SBTMCL_CO_SHIFT		11
#define	SBTMCL_IF_MASK		0xfc0000	/* interrupt flags */
#define	SBTMCL_IF_SHIFT		18
#define	SBTMCL_IM_MASK		0x3000000	/* interrupt mode */
#define	SBTMCL_IM_SHIFT		24

/* sbtmconfighigh */
#define	SBTMCH_BM_MASK		0x3		/* busy mode */
#define	SBTMCH_RM_MASK		0x3		/* retry mode */
#define	SBTMCH_RM_SHIFT		2
#define	SBTMCH_SM_MASK		0x30		/* stop mode */
#define	SBTMCH_SM_SHIFT		4
#define	SBTMCH_EM_MASK		0x300		/* sb error mode */
#define	SBTMCH_EM_SHIFT		8
#define	SBTMCH_IM_MASK		0xc00		/* int mode */
#define	SBTMCH_IM_SHIFT		10

/* sbbconfig */
#define	SBBC_LAT_MASK		0x3		/* sb latency */
#define	SBBC_MAX0_MASK		0xf0000		/* maxccntr0 */
#define	SBBC_MAX0_SHIFT		16
#define	SBBC_MAX1_MASK		0xf00000	/* maxccntr1 */
#define	SBBC_MAX1_SHIFT		20

/* sbbstate */
#define	SBBS_SRD		0x1		/* st reg disable */
#define	SBBS_HRD		0x2		/* hold reg disable */

/* sbidlow */
#define	SBIDL_CS_MASK		0x3		/* config space */
#define	SBIDL_AR_MASK		0x38		/* # address ranges supported */
#define	SBIDL_AR_SHIFT		3
#define	SBIDL_SYNCH		0x40		/* sync */
#define	SBIDL_INIT		0x80		/* initiator */
#define	SBIDL_MINLAT_MASK	0xf00		/* minimum backplane latency */
#define	SBIDL_MINLAT_SHIFT	8
#define	SBIDL_MAXLAT		0xf000		/* maximum backplane latency */
#define	SBIDL_MAXLAT_SHIFT	12
#define	SBIDL_FIRST		0x10000		/* this initiator is first */
#define	SBIDL_CW_MASK		0xc0000		/* cycle counter width */
#define	SBIDL_CW_SHIFT		18
#define	SBIDL_TP_MASK		0xf00000	/* target ports */
#define	SBIDL_TP_SHIFT		20
#define	SBIDL_IP_MASK		0xf000000	/* initiator ports */
#define	SBIDL_IP_SHIFT		24
#define	SBIDL_RV_MASK		0xf0000000	/* sonics backplane revision code */
#define	SBIDL_RV_SHIFT		28

/* sbidhigh */
#define	SBIDH_RC_MASK		0xf		/* revision code*/
#define	SBIDH_CC_MASK		0xfff0		/* core code */
#define	SBIDH_CC_SHIFT		4
#define	SBIDH_VC_MASK		0xffff0000	/* vendor code */
#define	SBIDH_VC_SHIFT		16

#define	SB_COMMIT		0xfd8		/* update buffered registers value */

/* vendor codes */
#define	SB_VEND_BCM		0x4243		/* Broadcom's SB vendor code */

/* core codes */
#define	SB_CC			0x800		/* chipcommon core */
#define	SB_ILINE20		0x801		/* iline20 core */
#define	SB_SDRAM		0x803		/* sdram core */
#define	SB_PCI			0x804		/* pci core */
#define	SB_MIPS			0x805		/* mips core */
#define	SB_ENET			0x806		/* enet mac core */
#define	SB_CODEC		0x807		/* v90 codec core */
#define	SB_USB			0x808		/* usb 1.1 host/device core */
#define	SB_ILINE100		0x80a		/* iline100 core */
#define	SB_IPSEC		0x80b		/* ipsec core */
#define	SB_PCMCIA		0x80d		/* pcmcia core */
#define	SB_MEMC			0x80f		/* memc sdram core */
#define	SB_EXTIF		0x811		/* external interface core */
#define	SB_D11			0x812		/* 802.11 MAC core */
#define	SB_MIPS33		0x816		/* mips3302 core */
#define	SB_USB11H		0x817		/* usb 1.1 host core */
#define	SB_USB11D		0x818		/* usb 1.1 device core */
#define	SB_USB20H		0x819		/* usb 2.0 host core */
#define	SB_USB20D		0x81A		/* usb 2.0 device core */
#define	SB_SDIOH		0x81B		/* sdio host core */

#endif	/* _SBCONFIG_H */
