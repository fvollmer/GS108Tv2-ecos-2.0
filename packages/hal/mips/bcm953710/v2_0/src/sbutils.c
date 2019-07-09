/*
 * Misc utility routines for accessing chip-specific features
 * of the SiliconBackplane-based Broadcom chips.
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 * $Id: sbutils.c,v 1.1.2.1 Exp $
 */

#include <cyg/hal/typedefs.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/bcmdevs.h>
#include <cyg/hal/sbconfig.h>
#include <cyg/hal/sbchipc.h>
#include <cyg/hal/sbpci.h>
#include <cyg/hal/pcicfg.h>
#include <cyg/hal/sbpcmcia.h>
#include <cyg/hal/sbextif.h>
#include <cyg/hal/sbutils.h>

/* debug/trace */
#define	SB_ERROR(args)   printf args

typedef uint32 (*sb_intrsoff_t)(void *intr_arg);
typedef void (*sb_intrsrestore_t)(void *intr_arg, uint32 arg);
typedef bool (*sb_intrsenabled_t)(void *intr_arg);

/* misc sb info needed by some of the routines */
typedef struct sb_info {
	uint	chip;			/* chip number */
	uint	chiprev;		/* chip revision */
	uint	chippkg;		/* chip package option */
	uint	boardtype;		/* board type */
	uint	boardvendor;		/* board vendor id */
	uint	bus;			/* what bus type we are going through */

	void	*osh;			/* osl os handle */
	void	*sdh;			/* bcmsdh handle */

	void	*curmap;		/* current regs va */
	void	*regs[SB_MAXCORES];	/* other regs va */

	uint	curidx;			/* current core index */
	uint	dev_coreid;		/* the core provides driver functions */

	uint	ccrev;			/* chipc core rev */

	uint	gpioidx;		/* gpio control core index */
	uint	gpioid;			/* gpio control coretype */

	uint	numcores;		/* # discovered cores */
	uint	coreid[SB_MAXCORES];	/* id of each core */

	void	*intr_arg;		/* interrupt callback function arg */
	sb_intrsoff_t		intrsoff_fn;		/* function turns chip interrupts off */
	sb_intrsrestore_t	intrsrestore_fn;	/* function restore chip interrupts */
} sb_info_t;

/* local prototypes */
static void* sb_doattach(sb_info_t *si, uint devid, void *osh, void *regs, uint bustype, void *sdh, char **vars, int *varsz);
static void sb_scan(sb_info_t *si);
static uint sb_corereg(void *sbh, uint coreidx, uint regoff, uint mask, uint val);
static uint _sb_coreidx(void *sbh);
static uint sb_findcoreidx(void *sbh, uint coreid, uint coreunit);
static uint sb_pcidev2chip(uint pcidev);
static uint sb_chip2numcores(uint chip);
static int sb_map_cores(sb_info_t *si);

#define	SB_INFO(sbh)	(sb_info_t*)sbh
#define	SET_SBREG(sbh, r, mask, val)	W_SBREG((sbh), (r), ((R_SBREG((sbh), (r)) & ~(mask)) | (val)))
#define	GOODCOREADDR(x)	(((x) >= SB_ENUM_BASE) && ((x) <= SB_ENUM_LIM) && ISALIGNED((x), SB_CORE_SIZE))
#define	GOODREGS(regs)	(regs && ISALIGNED(regs, SB_CORE_SIZE))
#define	REGS2SB(va)	(sbconfig_t*) ((uint)(va) + SBCONFIGOFF)
#define	GOODIDX(idx)	(((uint)idx) < SB_MAXCORES)
#define	BADIDX		(SB_MAXCORES+1)

#define	R_SBREG(sbh, sbr)	sb_read_sbreg((sbh), (sbr))
#define	W_SBREG(sbh, sbr, v)	sb_write_sbreg((sbh), (sbr), (v))
#define	AND_SBREG(sbh, sbr, v)	W_SBREG((sbh), (sbr), (R_SBREG((sbh), (sbr)) & (v)))
#define	OR_SBREG(sbh, sbr, v)	W_SBREG((sbh), (sbr), (R_SBREG((sbh), (sbr)) | (v)))

/*
 * Macros to disable/restore function core(D11, ENET, ILINE20, etc) interrupts before/
 * after core switching to avoid invalid register accesss inside ISR.
 */
#define INTR_OFF(si, intr_val) \
	if ((si)->intrsoff_fn && (si)->coreid[(si)->curidx] == (si)->dev_coreid) {	\
		intr_val = (*(si)->intrsoff_fn)((si)->intr_arg); }
#define INTR_RESTORE(si, intr_val) \
	if ((si)->intrsrestore_fn && (si)->coreid[(si)->curidx] == (si)->dev_coreid) {	\
		(*(si)->intrsrestore_fn)((si)->intr_arg, intr_val); }

/* power control defines */
#define	LPOMINFREQ	25000			/* low power oscillator min */
#define	LPOMAXFREQ	43000			/* low power oscillator max */
#define	XTALMINFREQ	19800000		/* 20mhz - 1% */
#define	XTALMAXFREQ	20200000		/* 20mhz + 1% */
#define	PCIMINFREQ	25000000		/* 25mhz */
#define	PCIMAXFREQ	34000000		/* 33mhz + fudge */
#define SCC_DEF_DIV	0			/* default slow clock divider */

#define XTAL_ON_DELAY		1000	/* Xtal power on delay in us */

#define SCC_LOW2FAST_LIMIT	5000	/* turn on fast clock time, in unit of ms */

typedef struct sb_cores_map {
    char    *core_name;
    uint    core_base;
    uint    core_id;
} sb_cored_map_t;

static sb_cored_map_t bcm56218_cores_map[] = {
            { "Chipcommon", 0x18000000, SB_CC   },
            { "mips",       0x18005000, SB_MIPS },
            { "memc",       0x18008000, SB_MEMC },
            { NULL,           0,        0       },
                                             };
/*
 * Allocate a sb handle.
 * devid - pci device id (used to determine chip#)
 * osh - opaque OS handle
 * regs - virtual address of initial core registers
 * bustype - pci/pcmcia/sb/sdio/etc
 * vars - pointer to a pointer area for "environment" variables
 * varsz - pointer to int to return the size of the vars
 */
void*
sb_attach(uint devid, void *osh, void *regs, uint bustype, void *sdh, char **vars, int *varsz)
{
	sb_info_t *si;

	/* alloc sb_info_t */
	if ((si = (sb_info_t *)MALLOC(sizeof (sb_info_t))) == NULL) {
		SB_ERROR(("sb_attach: malloc failed!\n"));
		return (NULL);
	}

	return (sb_doattach(si, devid, osh, regs, bustype, sdh, vars, varsz));
}

/* global kernel resource */
static sb_info_t ksi;

/* generic kernel variant of sb_attach() */
void*
sb_kattach()
{
    uint32 *regs;
    char *unused;
    int varsz;

    if (ksi.curmap == NULL) {
        regs = (uint32 *)REG_MAP(SB_ENUM_BASE, SB_CORE_SIZE);
        sb_doattach(&ksi, BCM56218_DEVICE_ID, NULL, (void*)regs,
                    SB_BUS, NULL, &unused, &varsz);
    }
    return &ksi;
}

static void*
sb_doattach(sb_info_t *si, uint devid, void *osh, void *regs, uint bustype, void *sdh, char **vars, int *varsz)
{
  	uint origidx;
	chipcregs_t *cc;

	ASSERT(GOODREGS(regs));

	bzero((uchar*)si, sizeof (sb_info_t));

	si->gpioidx = BADIDX;

	si->osh = osh;
	si->curmap = regs;
	si->sdh = sdh;

	si->bus = bustype;

	/* initialize current core index value */
	si->curidx = _sb_coreidx((void*)si);

	/* keep and reuse the initial register mapping */
	origidx = si->curidx;
	if (si->bus == SB_BUS)
		si->regs[origidx] = regs;

	/* is core-0 a chipcommon core? */
	si->numcores = 1;
	cc = (chipcregs_t*) sb_setcoreidx((void*)si, 0);

	si->chip = BCM56218_DEVICE_ID;
        si->chiprev = (R_REG(&cc->chipid) & CID_REV_MASK) >> CID_REV_SHIFT;

        si->numcores = sb_chip2numcores(si->chip);

	/* return to original core */
	sb_setcoreidx((void*)si, origidx);

	/* sanity checks */
	ASSERT(si->chip);

        /*
         * Check if cores can be mapped statically. If not, do a scan.
         */
        if (sb_map_cores(si)) {
		SB_ERROR(("sb_attach: unable to map cores !!\n"));
                goto bad;
        }

	/* gpio control core is required */
	if (!GOODIDX(si->gpioidx)) {
		SB_ERROR(("sb_attach: gpio control core not found\n"));
		goto bad;
	}

	/* get boardtype and boardrev */
	si->boardvendor = VENDOR_BROADCOM;
	si->boardtype = 0xffff;

	if (si->boardtype == 0) {
		SB_ERROR(("sb_attach: unknown board type\n"));
		ASSERT(si->boardtype);
	}

	return ((void*)si);

bad:
	MFREE(si, sizeof (sb_info_t));
	return (NULL);
}

uint
sb_coreidx(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	return (si->curidx);
}

/* return current index of core */
static uint
_sb_coreidx(void *sbh)
{
  	sb_info_t *si;
        int idx;

	si = SB_INFO(sbh);
	ASSERT(si);

        for (idx = 0; idx < SB_MAXCORES; idx++)
            if (si->regs[idx] == si->curmap)
                return idx;

        return 0; 
}

#define	SBTML_ALLOW	(SBTML_PE | SBTML_FGC | SBTML_FL_MASK)

/*
 * Switch to 'coreidx', issue a single arbitrary 32bit register mask&set operation,
 * switch back to the original core, and return the new value.
 */
static uint
sb_corereg(void *sbh, uint coreidx, uint regoff, uint mask, uint val)
{
  	sb_info_t *si;
	uint origidx;
	uint32 *r;
	uint w = 0;
	uint intr_val = 0;

	ASSERT(GOODIDX(coreidx));
	ASSERT(regoff < SB_CORE_SIZE);
	ASSERT((val & ~mask) == 0);

	si = SB_INFO(sbh);

	/* save current core index */
	origidx = sb_coreidx(sbh);

	/* switch core */
	INTR_OFF(si, intr_val);
	r = (uint32*) ((uint) sb_setcoreidx(sbh, coreidx) + regoff);

	/* mask and set */
	if (mask || val) {
		w = (R_REG(r) & ~mask) | val;
		W_REG(r, w);
	}

	/* restore core index */
	if (origidx != coreidx)
		sb_setcoreidx(sbh, origidx);

	INTR_RESTORE(si, intr_val);
	return (w);  
}

/* may be called with core in reset */
void
sb_detach(void *sbh)
{
	sb_info_t *si;
	uint idx;

	si = SB_INFO(sbh);

	if (si == NULL)
		return;

	if (si->bus == SB_BUS)
		for (idx = 0; idx < SB_MAXCORES; idx++)
			if (si->regs[idx]) {
				REG_UNMAP(si->regs[idx]);
				si->regs[idx] = NULL;
			}

	MFREE(si, sizeof (sb_info_t));
}

/* use pci dev id to determine chip id for chips not having a chipcommon core */
static uint
sb_pcidev2chip(uint devid)
{
   if (devid == BCM56218_DEVICE_ID)
	return (BCM56218_DEVICE_ID);
   return (0);
}

/* convert chip number to number of i/o cores */
static uint
sb_chip2numcores(uint chip)
{
  if (chip == BCM56218_DEVICE_ID)
	return (9);

  SB_ERROR(("sb_chip2numcores: unsupported chip 0x%x\n", chip));
  ASSERT(0);
  return (1);
}

/* return index of coreid or BADIDX if not found */
static uint
sb_findcoreidx(void *sbh, uint coreid, uint coreunit)
{
	sb_info_t *si;
	uint found;
	uint i;

	si = SB_INFO(sbh);
	found = 0;

	for (i = 0; i < si->numcores; i++)
		if (si->coreid[i] == coreid) {
			if (found == coreunit)
				return (i);
			found++;
		}

	return (BADIDX);
}

/* 
 * this function changes logical "focus" to the indiciated core, 
 * must be called with interrupt off.
 * Moreover, callers should keep interrupts off during switching out of and back to d11 core
 */
void*
sb_setcoreidx(void *sbh, uint coreidx)
{
 	sb_info_t *si;
	uint32 sbaddr;

	si = SB_INFO(sbh);

	if (coreidx >= si->numcores)
		return (NULL);

	sbaddr = SB_ENUM_BASE + (coreidx * SB_CORE_SIZE);

        /* map new one */
        if (!si->regs[coreidx]) {
            si->regs[coreidx] = (void*)REG_MAP(sbaddr, SB_CORE_SIZE);
            ASSERT(GOODREGS(si->regs[coreidx]));
        }
        si->curmap = si->regs[coreidx];

	si->curidx = coreidx;

	return (si->curmap); 	
}

/* 
 * this function changes logical "focus" to the indiciated core, 
 * must be called with interrupt off.
 * Moreover, callers should keep interrupts off during switching out of and back to d11 core
 */
void*
sb_setcore(void *sbh, uint coreid, uint coreunit)
{
	sb_info_t *si;
	uint idx;

	si = SB_INFO(sbh);

	idx = sb_findcoreidx(sbh, coreid, coreunit);
	if (!GOODIDX(idx))
		return (NULL);

	return (sb_setcoreidx(sbh, idx));
}

/* return chip number */
uint
sb_chip(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	return (si->chip);
}

/* return chip revision number */
uint
sb_chiprev(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	return (si->chiprev);
}

/* return chip common revision number */
uint
sb_chipcrev(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	return (si->ccrev);
}

/* return chip package option */
uint
sb_chippkg(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	return (si->chippkg);
}


/* return board vendor id */
uint
sb_boardvendor(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	return (si->boardvendor);
}

/* return boardtype */
uint
sb_boardtype(void *sbh)
{
  	sb_info_t *si;
	char *var;

	si = SB_INFO(sbh);

	if (si->bus == SB_BUS && si->boardtype == 0xffff) {
		/* boardtype format is a hex string */
		si->boardtype = getintvar(NULL, "boardtype");

		/* backward compatibility for older boardtype string format */
		if ((si->boardtype == 0) && (var = getvar(NULL, "boardtype"))) {
			if (!strcmp(var, "bcm56218qt"))
				si->boardtype = 0x11;
		}
	}

	return (si->boardtype);
}

/* return board bus style */
uint
sb_boardstyle(void *sbh)
{
  	sb_info_t *si;

	si = SB_INFO(sbh);

	if (si->bus == SB_BUS)
		return (BOARDSTYLE_SOC);

	return (BOARDSTYLE_SOC);  
}

/* return boolean if sbh device is in pci hostmode or client mode */
uint
sb_bus(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	return (si->bus);
}

/* return list of found cores */
uint
sb_corelist(void *sbh, uint coreid[])
{
	sb_info_t *si;

	si = SB_INFO(sbh);

	bcopy((uchar*)si->coreid, (uchar*)coreid, (si->numcores * sizeof (uint)));
	return (si->numcores);
}

/* return current register mapping */
void *
sb_coreregs(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	ASSERT(GOODREGS(si->curmap));

	return (si->curmap);
}


/* reset and re-enable a core */
void
sb_core_reset(void *sbh, uint32 bits)
{
   /* FIXME : add code here */
    return;
}

void
sb_core_disable(void *sbh, uint32 bits)
{
  return;
}

void
sb_watchdog(void *sbh, uint ticks)
{
	sb_info_t *si = SB_INFO(sbh);

	/* instant NMI */
	switch (si->gpioid) {
	case SB_CC:
		sb_corereg(sbh, si->gpioidx, OFFSETOF(chipcregs_t, watchdog), ~0, ticks);
		break;
        default:
		break;
	}
}


/* return the core-type instantiation # of the current core */
uint
sb_coreunit(void *sbh)
{
	sb_info_t *si;
	uint idx;
	uint coreid;
	uint coreunit;
	uint i;

	si = SB_INFO(sbh);
	coreunit = 0;

	idx = si->curidx;

	ASSERT(GOODREGS(si->curmap));
	coreid = si->coreid[idx];

	/* count the cores of our type */
	for (i = 0; i < idx; i++)
		if (si->coreid[i] == coreid)
			coreunit++;

	return (coreunit);
}

/* returns the current speed the SB is running at */
uint32
sb_clock(void *sbh)
{
#if 0
  return 133000000;
#else
    /* Dynamically detect the clock frequency */
    uint32_t cap;
    uint16_t chip_id;
    chipcregs_t *cc;

#   define M_CORECAP_CORE_CLK_FREQ_SEL ( 1 << 22 )

    /*
     * Read cap register to determine the system clks for c31x.
     */
    if ((cc = (chipcregs_t *) sb_setcore(sbh, SB_CC, 0)))
    {
      if((R_REG(&cc->chipid) & 0x0ff0)== 0x0310)
      {
        cap = R_REG(&cc->capabilities);
        if (cap & M_CORECAP_CORE_CLK_FREQ_SEL)
        {
          return 133000000;
        }
        else
        {
          return 100000000;
        }
      }
    }
    return 133000000;

#endif
}

/* change logical "focus" to the gpio core for optimized access */
void*
sb_gpiosetcore(void *sbh)
{
	sb_info_t *si;

	si = SB_INFO(sbh);

	return (sb_setcoreidx(sbh, si->gpioidx));
}

/* mask&set gpiocontrol bits */
uint32
sb_gpiocontrol(void *sbh, uint32 mask, uint32 val)
{
	sb_info_t *si;
	uint regoff;

	si = SB_INFO(sbh);
	regoff = 0;

	switch (si->gpioid) {
	case SB_CC:
		regoff = OFFSETOF(chipcregs_t, gpiocontrol);
		break;
        default:
		return (0);
	}

	return (sb_corereg(sbh, si->gpioidx, regoff, mask, val));
}

/* mask&set gpio output enable bits */
uint32
sb_gpioouten(void *sbh, uint32 mask, uint32 val)
{
	sb_info_t *si;
	uint regoff;

	si = SB_INFO(sbh);
	regoff = 0;

	switch (si->gpioid) {
	case SB_CC:
		regoff = OFFSETOF(chipcregs_t, gpioouten);
		break;

        default:
		return (0);
	}

	return (sb_corereg(sbh, si->gpioidx, regoff, mask, val));
}

/* mask&set gpio output bits */
uint32
sb_gpioout(void *sbh, uint32 mask, uint32 val)
{
	sb_info_t *si;
	uint regoff;

	si = SB_INFO(sbh);
	regoff = 0;

	switch (si->gpioid) {
	case SB_CC:
		regoff = OFFSETOF(chipcregs_t, gpioout);
		break;
        default:
		return (0);
	}

	return (sb_corereg(sbh, si->gpioidx, regoff, mask, val));
}

/* return the current gpioin register value */
uint32
sb_gpioin(void *sbh)
{
	sb_info_t *si;
	uint regoff;

	si = SB_INFO(sbh);
	regoff = 0;

	switch (si->gpioid) {
	case SB_CC:
		regoff = OFFSETOF(chipcregs_t, gpioin);
		break;
        default:
		return (0);
	}

	return (sb_corereg(sbh, si->gpioidx, regoff, 0, 0));
}

/* mask&set gpio interrupt polarity bits */
uint32
sb_gpiointpolarity(void *sbh, uint32 mask, uint32 val)
{
	sb_info_t *si;
	uint regoff;

	si = SB_INFO(sbh);
	regoff = 0;

	switch (si->gpioid) {
	case SB_CC:
		regoff = OFFSETOF(chipcregs_t, gpiointpolarity);
		break;
        default:
		return (0);
	}

	return (sb_corereg(sbh, si->gpioidx, regoff, mask, val));
}

/* mask&set gpio interrupt mask bits */
uint32
sb_gpiointmask(void *sbh, uint32 mask, uint32 val)
{
	sb_info_t *si;
	uint regoff;

	si = SB_INFO(sbh);
	regoff = 0;

	switch (si->gpioid) {
	case SB_CC:
		regoff = OFFSETOF(chipcregs_t, gpiointmask);
		break;
        default:
		return (0);
	}

	return (sb_corereg(sbh, si->gpioidx, regoff, mask, val));
}


/* register driver interrupt disabling and restoring callback functions */
void
sb_register_intr_callback(void *sbh, void *intrsoff_fn, void *intrsrestore_fn, void *intrsenabled_fn, void *intr_arg)
{
	sb_info_t *si;

	si = SB_INFO(sbh);
	si->intr_arg = intr_arg;
	si->intrsoff_fn = (sb_intrsoff_t)intrsoff_fn;
	si->intrsrestore_fn = (sb_intrsrestore_t)intrsrestore_fn;
	/* save current core id.  when this function called, the current core
	 * must be the core which provides driver functions(il, et, wl, etc.)
	 */
	si->dev_coreid = si->coreid[si->curidx];
}

static int
sb_map_cores(sb_info_t *si)
{
    sb_cored_map_t  *core_map = NULL;
    int              core_idx, origidx;
    void            *sbh;

    sbh = (void*) si;

    /* save current core index */
    origidx = sb_coreidx(sbh);

    if (si->chip == BCM56218_DEVICE_ID)
        core_map = bcm56218_cores_map;

    if (core_map == NULL)
        return -1;

    /*
     * Map the cores.
     */
    while (core_map->core_name) {
        core_idx = (core_map->core_base - SB_ENUM_BASE)/ SB_CORE_SIZE;
        sb_setcoreidx(sbh, core_idx);
        si->coreid[core_idx] = core_map->core_id;
        core_map++;
    }
    
    if (GOODIDX(sb_findcoreidx(sbh, SB_CC, 0))) {
        si->gpioidx = sb_findcoreidx(sbh, SB_CC, 0);
        si->gpioid = SB_CC;
    } 

    /* return to original core index */
    sb_setcoreidx(sbh, origidx);

    return 0;
}
