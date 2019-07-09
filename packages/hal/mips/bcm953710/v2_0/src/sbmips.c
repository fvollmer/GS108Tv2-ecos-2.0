/*
 * BCM47XX Sonics SiliconBackplane MIPS core routines
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: sbmips.c,v 1.1.2.3 Exp $
 */

#include <cyg/hal/typedefs.h>
#include <cyg/hal/ecosbsp.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/bcmdevs.h>
#include <cyg/hal/bcmnvram.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/hndmips.h>
#include <cyg/hal/sbextif.h>
#include <cyg/hal/sbchipc.h>
#include <cyg/hal/sbmemc.h>

/*
 * Memory segments (32bit kernel mode addresses)
 */
#undef KUSEG
#undef KSEG0
#undef KSEG1
#undef KSEG2
#undef KSEG3
#define KUSEG		0x00000000
#define KSEG0		0x80000000
#define KSEG1		0xa0000000
#define KSEG2		0xc0000000
#define KSEG3		0xe0000000

/*
 * Map an address to a certain kernel segment
 */
#undef KSEG0ADDR
#undef KSEG1ADDR
#undef KSEG2ADDR
#undef KSEG3ADDR
#define KSEG0ADDR(a)		(((a) & 0x1fffffff) | KSEG0)
#define KSEG1ADDR(a)		(((a) & 0x1fffffff) | KSEG1)
#define KSEG2ADDR(a)		(((a) & 0x1fffffff) | KSEG2)
#define KSEG3ADDR(a)		(((a) & 0x1fffffff) | KSEG3)

/*
 * The following macros are especially useful for __asm__
 * inline assembler.
 */
#ifndef __STR
#define __STR(x) #x
#endif
#ifndef STR
#define STR(x) __STR(x)
#endif

/*  *********************************************************************
    *  CP0 Registers 
    ********************************************************************* */

#define C0_INX		0		/* CP0: TLB Index */
#define C0_RAND		1		/* CP0: TLB Random */
#define C0_TLBLO0	2		/* CP0: TLB EntryLo0 */
#define C0_TLBLO	C0_TLBLO0	/* CP0: TLB EntryLo0 */
#define C0_TLBLO1	3		/* CP0: TLB EntryLo1 */
#define C0_CTEXT	4		/* CP0: Context */
#define C0_PGMASK	5		/* CP0: TLB PageMask */
#define C0_WIRED	6		/* CP0: TLB Wired */
#define C0_BADVADDR	8		/* CP0: Bad Virtual Address */
#define C0_COUNT 	9		/* CP0: Count */
#define C0_TLBHI	10		/* CP0: TLB EntryHi */
#define C0_COMPARE	11		/* CP0: Compare */
#define C0_SR		12		/* CP0: Processor Status */
#define C0_STATUS	C0_SR		/* CP0: Processor Status */
#define C0_CAUSE	13		/* CP0: Exception Cause */
#define C0_EPC		14		/* CP0: Exception PC */
#define C0_PRID		15		/* CP0: Processor Revision Indentifier */
#define C0_CONFIG	16		/* CP0: Config */
#define C0_LLADDR	17		/* CP0: LLAddr */
#define C0_WATCHLO	18		/* CP0: WatchpointLo */
#define C0_WATCHHI	19		/* CP0: WatchpointHi */
#define C0_XCTEXT	20		/* CP0: XContext */
#define C0_DIAGNOSTIC	22		/* CP0: Diagnostic */
#define C0_BROADCOM	C0_DIAGNOSTIC	/* CP0: Broadcom Register */
#define C0_ECC		26		/* CP0: ECC */
#define C0_CACHEERR	27		/* CP0: CacheErr */
#define C0_TAGLO	28		/* CP0: TagLo */
#define C0_TAGHI	29		/* CP0: TagHi */
#define C0_ERREPC	30		/* CP0: ErrorEPC */

/*
 * Macros to access the system control coprocessor
 */

#define MFC0(source, sel)					\
({								\
	int __res;						\
	__asm__ __volatile__(					\
	".set\tnoreorder\n\t"					\
	".set\tnoat\n\t"					\
	".word\t"STR(0x40010000 | ((source)<<11) | (sel))"\n\t"	\
	"move\t%0,$1\n\t"					\
	".set\tat\n\t"						\
	".set\treorder"						\
	:"=r" (__res)						\
	:							\
	:"$1");							\
	__res;							\
})

#define MTC0(source, sel, value)				\
do {								\
	__asm__ __volatile__(					\
	".set\tnoreorder\n\t"					\
	".set\tnoat\n\t"					\
	"move\t$1,%z0\n\t"					\
	".word\t"STR(0x40810000 | ((source)<<11) | (sel))"\n\t"	\
	".set\tat\n\t"						\
	".set\treorder"						\
	:							\
	:"Jr" (value)						\
	:"$1");							\
} while (0)

/*
 * R4x00 interrupt enable / cause bits
 */
#undef IE_SW0
#undef IE_SW1
#undef IE_IRQ0
#undef IE_IRQ1
#undef IE_IRQ2
#undef IE_IRQ3
#undef IE_IRQ4
#undef IE_IRQ5
#define IE_SW0		(1<< 8)
#define IE_SW1		(1<< 9)
#define IE_IRQ0		(1<<10)
#define IE_IRQ1		(1<<11)
#define IE_IRQ2		(1<<12)
#define IE_IRQ3		(1<<13)
#define IE_IRQ4		(1<<14)
#define IE_IRQ5		(1<<15)

/*
 * Bitfields in the R4xx0 cp0 status register
 */
#define ST0_IE			0x00000001
#define ST0_EXL			0x00000002
#define ST0_ERL			0x00000004
#define ST0_KSU			0x00000018
#  define KSU_USER		0x00000010
#  define KSU_SUPERVISOR	0x00000008
#  define KSU_KERNEL		0x00000000
#define ST0_UX			0x00000020
#define ST0_SX			0x00000040
#define ST0_KX 			0x00000080
#define ST0_DE			0x00010000
#define ST0_CE			0x00020000

/*
 * Status register bits available in all MIPS CPUs.
 */
#define ST0_IM			0x0000ff00
#define ST0_CH			0x00040000
#define ST0_SR			0x00100000
#define ST0_TS			0x00200000
#define ST0_BEV			0x00400000
#define ST0_RE			0x02000000
#define ST0_FR			0x04000000
#define ST0_CU			0xf0000000
#define ST0_CU0			0x10000000
#define ST0_CU1			0x20000000
#define ST0_CU2			0x40000000
#define ST0_CU3			0x80000000
#define ST0_XX			0x80000000	/* MIPS IV naming */

/*
 * Cache Operations
 */

#ifndef Fill_I
#define Fill_I			0x14
#endif

#define cache_unroll(base,op)			\
	__asm__ __volatile__("			\
		.set noreorder;			\
		.set mips3;			\
		cache %1, (%0);			\
		.set mips0;			\
		.set reorder"			\
		:				\
		: "r" (base),			\
		  "i" (op));

#if 1
extern void board_setleds(uint32 val);
extern void board_led_msg(char *msg);
#endif

/* 
 * These are the UART port assignments, expressed as offsets from the base
 * register.  These assignments should hold for any serial port based on
 * a 8250, 16450, or 16550(A).
 */

#define UART_MCR	4	/* Out: Modem Control Register */
#define UART_MSR	6	/* In:  Modem Status Register */
#define UART_MCR_LOOP	0x10	/* Enable loopback test mode */

/* IRQ defines */
#define SBIPS_INT_UART0     0
#define SBIPS_INT_UART1     1
#define SBIPS_INT_CMIC      2
#define SBIPS_INT_GPIO      3

/* 
 * Returns TRUE if an external UART exists at the given base
 * register.
 */
static bool
serial_exists(uint8 *regs)
{
	uint8 save_mcr, status1;

	save_mcr = R_REG(&regs[UART_MCR]);
	W_REG(&regs[UART_MCR], UART_MCR_LOOP | 0x0a);
	status1 = R_REG(&regs[UART_MSR]) & 0xf0;
	W_REG(&regs[UART_MCR], save_mcr);

	return (status1 == 0x90);
}

/* Current clock frequency dynamically detected, default is 133000000 */
uint32 sbclock = 133000000;

/* 
 * Initializes UART access. The callback function will be called once
 * per found UART.
*/
void
sb_serial_init(void *sbh, void (*add)(void *regs, uint irq, uint baud_base, uint reg_shift))
{
	 void *regs;
	 ulong baud_base;
	 uint irq;
	 int i, n;

  if ((regs = sb_setcore(sbh, SB_CC, 0))) {
    chipcregs_t *cc = (chipcregs_t *) regs;

    regs = (uint32 *)REG_MAP(BCM53710_REG_UARTS, 0);

#if 0 
    irq = INTNUM_UART0;
#endif
		
    baud_base = get_sb_clock();

    /* Probe the clock frequency */
    sbclock = sb_clock(sbh);

    if (add) {
      /* Determine IRQ */
      irq = sb_irq(sbh, SBIPS_INT_UART0);
	 	  add(regs, irq, baud_base, 0);
    }
  }
}

static const uint32 sbips_int_mask[] = {
	0,
	SBIPS_INT1_MASK,
	SBIPS_INT2_MASK,
	SBIPS_INT3_MASK,
	SBIPS_INT4_MASK
};

static const uint32 sbips_int_shift[] = {
	0,
	0,
	SBIPS_INT2_SHIFT,
	SBIPS_INT3_SHIFT,
	SBIPS_INT4_SHIFT
};

/* 
 * Returns the MIPS IRQ assignment of the current core. If unassigned,
 * 0 is returned.
 */

uint
sb_irq(void *sbh, uint irq_src_id)
{
 uint idx;
 uint32 flag, sbipsflag = 0;
 uint irq = 0;
 chipcregs_t *cc;

 idx = sb_coreidx(sbh);

 flag = irq_src_id;
 if ((cc = sb_setcore(sbh, SB_CC, 0))) {
            sbipsflag = R_REG(&cc->sbipsflag);
         }

 for (irq = 1; irq <= 4; irq++) {
         if (((sbipsflag & sbips_int_mask[irq]) >> sbips_int_shift[irq])
                                                == flag)
           break;
 }
/* No match, return Zer. */
 if (irq == 5)
     irq = 0;

 sb_setcoreidx(sbh, idx);

 return irq;  
}

/* Clears the specified MIPS IRQ. */
static void
sb_clearirq(void *sbh, uint irq)
{
	chipcregs_t *cc;
	uint idx;

	idx = sb_coreidx(sbh);
	if ((cc = sb_setcore(sbh, SB_CC, 0))) {
            if (irq == 0)
                W_REG(&cc->sbintvec, 0);
            else
                OR_REG(&cc->sbipsflag, sbips_int_mask[irq]);

        }
	sb_setcoreidx(sbh, idx);
}

/* 
 * Assigns the specified MIPS IRQ to the specified core. Shared MIPS
 * IRQ 0 may be assigned more than once.
 */
static void
sb_setirq(void *sbh, uint irq, uint sb_irq_id)
{
  	chipcregs_t *cc;
	uint32 flag, idx;

	flag = sb_irq_id;

	idx = sb_coreidx(sbh);
	if ((cc = sb_setcore(sbh, SB_CC, 0))) {
            if (irq == 0)
                OR_REG(&cc->sbintvec, 1 << flag);
            else {
                flag <<= sbips_int_shift[irq];
                ASSERT(!(flag & ~sbips_int_mask[irq]));
                flag |= R_REG(&cc->sbipsflag) & ~sbips_int_mask[irq];
                W_REG(&cc->sbipsflag, flag);

                /* Setup GPIO interrupts */
                if (sb_irq_id == SBIPS_INT_GPIO) {
                    W_REG(&cc->gpiocontrol, 0xffffffff);
                    W_REG(&cc->gpiointmask, 3);
                    W_REG(&cc->gpiointpolarity, 3);
                    W_REG(&cc->gpioouten, 3);
                }
            }
        }
	sb_setcoreidx(sbh, idx);
}	

/* 
 * Initializes clocks and interrupts. SB and NVRAM access must be
 * initialized prior to calling.
 */
void
sb_mips_init(void *sbh)
{
    uint irq, idx;
	chipcregs_t *cc;

	/* Chip specific initialization */
                /* reset uart */
	        idx = sb_coreidx(sbh);
	        if ((cc = sb_setcore(sbh, SB_CC, 0))) {
                    W_REG(&cc->peripheralreset, 1);
                    W_REG(&cc->peripheralreset, 3);
                }

		/* Clear interrupt map */
		for (irq = 0; irq <= 4; irq++)
			sb_clearirq(sbh, irq);
		sb_setirq(sbh, 1, SBIPS_INT_UART0);
		sb_setirq(sbh, 2, SBIPS_INT_UART1);
                sb_setirq(sbh, 3, SBIPS_INT_CMIC);
		/* sb_setirq(sbh, 4, SBIPS_INT_GPIO); */

	    sb_setcoreidx(sbh, idx);
}

uint32
sb_mips_clock(void *sbh)
{
  uint idx;
  uint32 rate, ratio;
  chipcregs_t *cc;

  rate = 133000000;
  idx = sb_coreidx(sbh);

  if ((cc = (chipcregs_t *) sb_setcore(sbh, SB_CC, 0))) {
      ratio = (R_REG(&cc->capabilities) >> 4) & 3;

  switch (ratio) {
        case 0:
          rate = rate*2;
          break;
        case 1:
           rate = (rate*9)/4;
           break;
        case 2:
           rate = (rate*6)/4;
           break;
        case 3:
           break;
       }
     }

  sb_setcoreidx(sbh, idx);

  return rate; 
}

#if 0
static void
icache_probe(int *size, int *lsize)
{
	uint32 config1;
	uint sets, ways;

	config1 = MFC0(C0_CONFIG, 1);

	/* Instruction Cache Size = Associativity * Line Size * Sets Per Way */
	if ((*lsize = ((config1 >> 19) & 7)))
		*lsize = 2 << *lsize;
	sets = 64 << ((config1 >> 22) & 7);
	ways = 1 + ((config1 >> 16) & 7);
	*size = *lsize * sets * ways;
}

#define ALLINTS (IE_IRQ0 | IE_IRQ1 | IE_IRQ2 | IE_IRQ3 | IE_IRQ4)

static void
handler(void)
{
	/* Step 11 */
	__asm__ (
		".set\tmips32\n\t"
		"ssnop\n\t"
		"ssnop\n\t"
	/* Disable interrupts */
	/*	MTC0(C0_STATUS, 0, MFC0(C0_STATUS, 0) & ~(ALLINTS | STO_IE)); */
		"mfc0 $15, $12\n\t"
		"and $15, $15, -31746\n\t"
		"mtc0 $15, $12\n\t"
		"eret\n\t"
		"nop\n\t"
		"nop\n\t"
		".set\tmips0"
	);
}

/* The following MUST come right after handler() */
static void
afterhandler(void)
{
}
#endif
/*
 * Set the MIPS, backplane and PCI clocks as closely as possible.
 */
bool
sb_mips_setclock(void *sbh, uint32 mipsclock, uint32 sbclock, uint32 pciclock)
{
  return FALSE;
}

/* returns the ncdl value to be programmed into sdram_ncdl for calibration */
uint32
sb_memc_get_ncdl(void *sbh)
{
 	sbmemcregs_t *memc;
	uint32 ret = 0;
	uint32 config, rd, wr, misc, dqsg, cd, sm, sd;
	uint idx;

	idx = sb_coreidx(sbh);

	memc = (sbmemcregs_t *)sb_setcore(sbh, SB_MEMC, 0);
	if (memc == 0)
		goto out;

	config = R_REG(&memc->config);
	wr = R_REG(&memc->wrncdlcor);
	rd = R_REG(&memc->rdncdlcor);
	misc = R_REG(&memc->miscdlyctl);
	dqsg = R_REG(&memc->dqsgatencdl);

	rd &= MEMC_RDNCDLCOR_RD_MASK;
	wr &= MEMC_WRNCDLCOR_WR_MASK; 
	dqsg &= MEMC_DQSGATENCDL_G_MASK;

	if (config & MEMC_CONFIG_DDR) {
		ret = (wr << 16) | (rd << 8) | dqsg;
	} else {
		cd = (rd == MEMC_CD_THRESHOLD) ? rd : (wr + MEMC_CD_THRESHOLD);
		sm = (misc & MEMC_MISC_SM_MASK) >> MEMC_MISC_SM_SHIFT;
		sd = (misc & MEMC_MISC_SD_MASK) >> MEMC_MISC_SD_SHIFT;
		ret = (sm << 16) | (sd << 8) | cd;
	}

out:
	/* switch back to previous core */
	sb_setcoreidx(sbh, idx);

	return ret;  
}

void sysReboot(void)
{
    SYS_HARD_RESET();
}

static uint32 sbclk=133000000;

/* routine to return the sb clock speed (discovered during init ) */
uint32 get_sb_clock(void) {
    ASSERT(sbclk);
    return(sbclk);
}

#define MIPS_PLL_CTRL_REG2  0x18000094

uint32 get_sb_mips_clock(void) {
    uint32   core;
    volatile uint32  reg;

    core = 133000000;

    reg = *(volatile uint32*)REG_MAP(MIPS_PLL_CTRL_REG2, 0);
    switch (reg & 3) {
        case 0: core = core * 2; break;
        case 1: core = (core * 9)/4; break;
        case 2: core = (core * 6)/4; break;
        case 3: core = core; break;
    }

    return core;
}
