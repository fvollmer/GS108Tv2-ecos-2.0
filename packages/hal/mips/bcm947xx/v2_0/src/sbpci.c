/*
 * Low-Level PCI and SB support for BCM47xx
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: sbpci.c,v 1.6.2.1 Exp $
 */

#include <cyg/hal/typedefs.h>
#include <cyg/hal/pcicfg.h>
#include <cyg/hal/bcmdevs.h>
#include <cyg/hal/sbconfig.h>
#include <cyg/hal/sbpci.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/bcmendian.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/sbutils.h>
#include <cyg/hal/bcmnvram.h>
#include <cyg/hal/hndmips.h>
#include <cyg/io/pci_cfg.h>

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/ecosbsp.h>

/* Can free sbpci_init() memory after boot */
#ifndef linux
#define __init
#endif

/* PCI spaces mapped by the core in Host mode. */

/* Emulated configuration space */
static pci_config_regs sb_config_regs[SB_MAXCORES];

/* Banned cores */
static uint16 pci_ban[32] = { 0 };
static uint pci_banned = 0;

/*
 * Functions for accessing external PCI configuration space
 */

/* Assume one-hot slot wiring */
#define PCI_SLOT_MAX 16

/* Note that READCFG/WRITECFG uses below can generate bus errors that
   are not caught; guard calls with these functions with pci_probe_tag. */
   
static int bus_error=0;

void pciscan_exception_handler(
    cyg_addrword_t data,
    cyg_code_t number,
    cyg_addrword_t info)
{ 
    HAL_SavedRegisters *savedreg;

    savedreg = (HAL_SavedRegisters *)info;
    savedreg->pc +=4;
    bus_error = 1;
} 

static int mem_probe(uint32 *reg, uint32* val)
{
    int ret = 0;
	
    bus_error = 0;
    *val = rreg32(reg);
    if (bus_error == 1) {
        ret = -1;
        bus_error = 0;
    }
    return ret;
}

static uint32
config_cmd(void *sbh, uint bus, uint dev, uint func, uint off)
{
    uint coreidx;
    sbpciregs_t *regs;
    uint32 addr = 0;

    coreidx = sb_coreidx(sbh);
    regs = (sbpciregs_t *) sb_setcore(sbh, SB_PCI, 0);

    /* Type 0 transaction */
    if (bus == 1) {
        /* Skip unwired slots */
        if (dev < PCI_SLOT_MAX) {
            /* Slide the PCI window to the appropriate slot */
            W_REG(&regs->sbtopci1, SBTOPCI_CFG0 |
                                  ((1 << (dev + 16)) & SBTOPCI1_MASK));
            addr = R_REG(&regs->sbtopci1); //push
            addr = SB_PCI_CFG | ( (1 << (dev + 16)) & ~SBTOPCI1_MASK) |
                                ((func << 8) | (off & ~3));
        }
    } else { /* Type 1 transaction */ 
        W_REG(&regs->sbtopci1, SBTOPCI_CFG1);
        addr = SB_PCI_CFG | ((bus - 1) << 16) |
               (dev << 11) | (func << 8) | (off & ~3);
    }

    sb_setcoreidx(sbh, coreidx);

    return addr;
}

static int
extpci_read_config(void *sbh, uint bus, uint dev, uint func, uint off, void *buf, int len)
{
    uint32 addr, val=0xffffffff;
    cyg_uint32* reg;
    int ret = 0;
    cyg_exception_handler_t *old_handler;

    cyg_addrword_t old_data;
	   
    if (dev < 16) {
        if (!(addr = config_cmd(sbh, bus, dev, func, off)) ||
            !(reg = (CYG_WORD32 *) REG_MAP(addr, len)) ) {
            val = 0xffffffff;
        }
        /* Install the exception handler for error output. */
        cyg_exception_set_handler(CYGNUM_HAL_VECTOR_DBE,
                               &pciscan_exception_handler,
                               0,
                               &old_handler,
                               &old_data);
        if (mem_probe(reg, &val) < 0) {
            val = 0xffffffff;
        }
        cyg_exception_clear_handler(CYGNUM_HAL_VECTOR_DBE);
    }
	
    val >>= 8 * (off & 3);
    if (len == 4)
        *((uint32 *) buf) = val;
    else if (len == 2)
        *((uint16 *) buf) = (uint16) val;
    else if (len == 1)
        *((uint8 *) buf) = (uint8) val;
    else
	       ret = -1;

    if (reg)
        REG_UNMAP(reg);
	
    return ret;
}

static int
extpci_write_config(void *sbh, uint bus, uint dev, uint func, uint off, void *buf, int len)
{
    uint32 addr, *reg = NULL, val;
    int ret = 0;

    if (!(addr = config_cmd(sbh, bus, dev, func, off)) ||
        !(reg = (CYG_WORD32 *) REG_MAP(addr, len)) ||
        BUSPROBE(val, reg)) {
        goto done;
    }

    if (len == 4)
        val = *((uint32 *) buf);
    else if (len == 2) {
        val &= ~(0xffff << (8 * (off & 3)));
        val |= *((uint16 *) buf) << (8 * (off & 3));
    } else if (len == 1) {
        val &= ~(0xff << (8 * (off & 3)));
        val |= *((uint8 *) buf) << (8 * (off & 3));
    } else
        ret = -1;

    W_REG(reg, val);

done:
    if (reg)
        REG_UNMAP(reg);

    return ret;
}

/*
 * Functions for accessing translated SB configuration space
 */

static int
sb_read_config(void *sbh, uint bus, uint dev, uint func, uint off, void *buf, int len)
{
    pci_config_regs *cfg;

    if (dev >= SB_MAXCORES || (off + len) > sizeof(pci_config_regs))
        return -1;
    cfg = &sb_config_regs[dev];

    ASSERT(ISALIGNED(off, len));   
    ASSERT(ISALIGNED(buf, len));

    if (len == 4)
        *((uint32 *) buf) = ltoh32(*((uint32 *)((ulong) cfg + off)));
    else if (len == 2)
        *((uint16 *) buf) = ltoh16(*((uint16 *)((ulong) cfg + off)));
    else if (len == 1)
        *((uint8 *) buf) = *((uint8 *)((ulong) cfg + off));
    else
        return -1;

    return 0;
}

static int
sb_write_config(void *sbh, uint bus, uint dev, uint func, uint off, void *buf, int len)
{
    uint coreidx, n;
    void *regs;
    sbconfig_t *sb;
    pci_config_regs *cfg;

    if (dev >= SB_MAXCORES || (off + len) > sizeof(pci_config_regs))
        return -1;
    cfg = &sb_config_regs[dev];

    ASSERT(ISALIGNED(off, len));
    ASSERT(ISALIGNED(buf, len));

    /* Emulate BAR sizing */
    if (off >= OFFSETOF(pci_config_regs, base[0]) && 
        off <= OFFSETOF(pci_config_regs, base[3]) &&
        len == 4 && *((uint32 *) buf) == ~0) {
        coreidx = sb_coreidx(sbh);
        if ((regs = sb_setcoreidx(sbh, dev))) {
            sb = (sbconfig_t *)((ulong) regs + SBCONFIGOFF);
            /* Highest numbered address match register */
            n = (R_REG(&sb->sbidlow) & SBIDL_AR_MASK) >> SBIDL_AR_SHIFT;
            if (off == OFFSETOF(pci_config_regs, base[0]))
                cfg->base[0] = ~(sb_size(R_REG(&sb->sbadmatch0)) - 1);
            else if (off == OFFSETOF(pci_config_regs, base[1]) && n >= 1)
                cfg->base[1] = ~(sb_size(R_REG(&sb->sbadmatch1)) - 1);
            else if (off == OFFSETOF(pci_config_regs, base[2]) && n >= 2)
                cfg->base[2] = ~(sb_size(R_REG(&sb->sbadmatch2)) - 1);
            else if (off == OFFSETOF(pci_config_regs, base[3]) && n >= 3)
                cfg->base[3] = ~(sb_size(R_REG(&sb->sbadmatch3)) - 1);
        }
        sb_setcoreidx(sbh, coreidx);
        return 0;
    }

    if (len == 4)
        *((uint32 *)((ulong) cfg + off)) = htol32(*((uint32 *) buf));
    else if (len == 2)
        *((uint16 *)((ulong) cfg + off)) = htol16(*((uint16 *) buf));
    else if (len == 1)
        *((uint8 *)((ulong) cfg + off)) = *((uint8 *) buf);
    else
        return -1;

    return 0;
}

int
sbpci_read_config(void *sbh, uint bus, uint dev, uint func, uint off, void *buf, int len)
{
    if (bus == 0)
        return sb_read_config(sbh, bus, dev, func, off, buf, len);
    else
        return extpci_read_config(sbh, bus, dev, func, off, buf, len);
}

int
sbpci_write_config(void *sbh, uint bus, uint dev, uint func, uint off, void *buf, int len)
{
    if (bus == 0)
        return sb_write_config(sbh, bus, dev, func, off, buf, len);
#ifndef BCM_ROBO_SUPPORT
    else
        return extpci_write_config(sbh, bus, dev, func, off, buf, len);
#else
    return 0;
#endif
}

void
sbpci_ban(uint16 core)
{
    if (pci_banned < ARRAYSIZE(pci_ban))
        pci_ban[pci_banned++] = core;
}

int __init
sbpci_init(void *sbh)
{
    uint chip, chiprev, chippkg, coreidx, host, i;
    uint32 boardflags;
    sbpciregs_t *pci;
    sbconfig_t *sb;
    pci_config_regs *cfg;
    void *regs;
    char varname[8];
    uint wlidx = 0;
    uint16 vendor, core;
    uint8 class, subclass, progif;
    uint32 val;
    uint32 sbips_int_mask[] = { 0, SBIPS_INT1_MASK, SBIPS_INT2_MASK, SBIPS_INT3_MASK, SBIPS_INT4_MASK };
    uint32 sbips_int_shift[] = { 0, 0, SBIPS_INT2_SHIFT, SBIPS_INT3_SHIFT, SBIPS_INT4_SHIFT };
    uint32 pci_control, pci_arbcontrol;
    int ext_pci_arb = 0;
    int ext_pci_clk = 0;
    
    chip = sb_chip(sbh);
    chiprev = sb_chiprev(sbh);
    chippkg = sb_chippkg(sbh);
    coreidx = sb_coreidx(sbh);
    
    if (!(pci = (sbpciregs_t *) sb_setcore(sbh, SB_PCI, 0)))
        return -1;
    
    pci_control = R_REG(&pci->control);
    pci_arbcontrol = R_REG(&pci->arbcontrol);
    
    sb_core_reset(sbh, 0);
    
    boardflags = (uint32) getintvar(NULL, "boardflags");
    
    host = !BUSPROBE(val, &pci->control);
    
    if (!host) {
        /* Disable PCI interrupts in client mode */
        sb = (sbconfig_t *)((ulong) pci + SBCONFIGOFF);
        W_REG(&sb->sbintvec, 0);
    
        /* Disable the PCI bridge in client mode */
        sbpci_ban(SB_PCI);
        printf("PCI: Disabled\n");
    } else {
    	   /* Assume CFE already do the job */
    	   if (pci_control & 0x5) {
           if (!(pci_control & 0x8)) {
               ext_pci_clk = 1;
           }
        }
        ext_pci_arb = pci_arbcontrol & PCI_EXT_ARB;
    	   /*
         * Init the pci interface 
         */
        if (ext_pci_clk) {
        	 W_REG(&pci->control, 0x1); /* PCI reset, no clock */
          OSL_DELAY(100);
          W_REG(&pci->control, 0x3);
        } else {
        	 W_REG(&pci->control, 0x5);				/* enable the tristate drivers */
          OSL_DELAY(100);					    /* delay 100 us */
          W_REG(&pci->control, 0xd);				/* enable the PCI clock */
          OSL_DELAY(100);					    /* delay 100 us */
          W_REG(&pci->control, 0xf);				/* deassert PCI reset */
        }
        W_REG(&pci->arbcontrol, (ext_pci_arb) ? PCI_EXT_ARB : PCI_INT_ARB);
        OSL_DELAY(1);				/* delay 1 us */
    
    #define MAPPED_ADDR	0xA8000000
        /* 64 MB Memory access window */
        W_REG(&pci->sbtopci0, SBTOPCI_MEM | MAPPED_ADDR| 0x0C);OSL_DELAY(2);
        /* 64 MB configuration access window */
        W_REG(&pci->sbtopci1, SBTOPCI_CFG0);OSL_DELAY(2);
    
        /* stop the servicing of any further PCI */
    
      	 val = 0x0;
        sbpci_write_config(sbh, 1, 0, 0, PCI_CFG_CMD, &val, sizeof(val));
        OSL_DELAY(150);
        
        /* Set up the BARs (BAR0 for cores, BAR1 for SDRAM) */
        val = 0x18000000;
        sbpci_write_config(sbh, 1, 0, 0, 0x80, &val, sizeof(val));
        OSL_DELAY(2);
        
        val = 0x10000000;
        sbpci_write_config(sbh, 1, 0, 0, CYG_PCI_CFG_BAR_0, &val, sizeof(val));
        OSL_DELAY(2);
        
        val = 0x00000000;
        sbpci_write_config(sbh, 1, 0, 0, 0x84, &val, sizeof(val));
        OSL_DELAY(2);
        
        val = 0xA0000000;
        sbpci_write_config(sbh, 1, 0, 0, CYG_PCI_CFG_BAR_1, &val, sizeof(val));
        OSL_DELAY(2);
        
      	 val = 0x146;
      	 sbpci_write_config(sbh, 1, 0, 0, PCI_CFG_CMD, &val, sizeof(val));
        
        /* push */
        sbpci_read_config(sbh, 1, 0, 0, CYG_PCI_CFG_VENDOR, &val, sizeof(val));
          
        W_REG(&pci->intmask,  PCI_INTA);
    
#define SB_MIPS33_CORE_BA   0xb8005000
#define SB_CHIPC_CORE_BA    0xb8000000

        *((volatile unsigned long*)(SB_MIPS33_CORE_BA + 0xfa8)) = 0x00050076; /* Bump up the timeout */
        *((volatile unsigned long*)(SB_CHIPC_CORE_BA + 0x50)) = 0xfd8;
        *((volatile unsigned long*)(SB_CHIPC_CORE_BA + 0x54)) = 0x0;
   
    }

    /* Scan the SB bus */
    bzero(sb_config_regs, sizeof(sb_config_regs));
    for (cfg = sb_config_regs; cfg < &sb_config_regs[SB_MAXCORES]; cfg++) {
        cfg->vendor = 0xffff;
        if (!(regs = sb_setcoreidx(sbh, cfg - sb_config_regs)))
            continue;
        sb = (sbconfig_t *)((ulong) regs + SBCONFIGOFF);
     
        /* Read ID register and parse vendor and core */
        val = R_REG(&sb->sbidhigh);
        vendor = (val & SBIDH_VC_MASK) >> SBIDH_VC_SHIFT;
        core = (val & SBIDH_CC_MASK) >> SBIDH_CC_SHIFT;
        progif = 0;
        
        /* Check if this core is banned */
        for (i = 0; i < pci_banned; i++)
            if (core == pci_ban[i])
                break;
        if (i < pci_banned)
            continue;
        
        /* Known vendor translations */
        switch (vendor) {
        case SB_VEND_BCM:
            vendor = VENDOR_BROADCOM;
            break;
        }
        
        /* Determine class based on known core codes */
        switch (core) {
        case SB_ILINE20:
            class = PCI_CLASS_NET;
            subclass = PCI_NET_ETHER;
            core = BCM47XX_ILINE_ID;
            break;
        case SB_ILINE100:
            class = PCI_CLASS_NET;
            subclass = PCI_NET_ETHER;
            core = BCM4610_ILINE_ID;
            break;
        case SB_ENET:
            class = PCI_CLASS_NET;
            subclass = PCI_NET_ETHER;
            core = BCM47XX_ENET_ID;
            break;
        case SB_SDRAM:
        case SB_MEMC:
            class = PCI_CLASS_MEMORY;
            subclass = PCI_MEMORY_RAM;
            break;
        case SB_PCI:
            class = PCI_CLASS_BRIDGE;
            subclass = PCI_BRIDGE_PCI;
            break;
        case SB_MIPS:
        case SB_MIPS33:
            class = PCI_CLASS_CPU;
            subclass = PCI_CPU_MIPS;
            break;
        case SB_CODEC:
            class = PCI_CLASS_COMM;
            subclass = PCI_COMM_MODEM;
            core = BCM47XX_V90_ID;
            break;
        case SB_USB:
            class = PCI_CLASS_SERIAL;
            subclass = PCI_SERIAL_USB;
            progif = 0x10; /* OHCI */
            core = BCM47XX_USB_ID;
            break;
        case SB_USB11H:
            class = PCI_CLASS_SERIAL;
            subclass = PCI_SERIAL_USB;
            progif = 0x10; /* OHCI */
            core = BCM47XX_USBH_ID;
            break;
        case SB_USB11D:
            class = PCI_CLASS_SERIAL;
            subclass = PCI_SERIAL_USB;
            core = BCM47XX_USBD_ID;
            break;
        case SB_IPSEC:
            class = PCI_CLASS_CRYPT;
            subclass = PCI_CRYPT_NETWORK;
            core = BCM47XX_IPSEC_ID;
            break;
        case SB_EXTIF:
        case SB_CC:
            class = PCI_CLASS_MEMORY;
            subclass = PCI_MEMORY_FLASH;
            break;
        case SB_D11:
            class = PCI_CLASS_NET;
            subclass = PCI_NET_OTHER;
            /*Let an nvram variable override this */
            /*sprintf(varname, "wl%did", wlidx);*/
            wlidx++;
            core = 0x4320;
            break;
        default:
            class = subclass = progif = 0xff;
            break;
        }
        /* Supported translations */
        cfg->vendor = htol16(vendor);
        cfg->device = htol16(core);
        cfg->rev_id = chiprev;
        cfg->prog_if = progif;
        cfg->sub_class = subclass;
        cfg->base_class = class;
        cfg->base[0] = htol32(sb_base(R_REG(&sb->sbadmatch0)));
        cfg->base[1] = htol32(sb_base(R_REG(&sb->sbadmatch1)));
        cfg->base[2] = htol32(sb_base(R_REG(&sb->sbadmatch2)));
        cfg->base[3] = htol32(sb_base(R_REG(&sb->sbadmatch3)));
        cfg->base[4] = 0;
        cfg->base[5] = 0;
        if (class == PCI_CLASS_BRIDGE && subclass == PCI_BRIDGE_PCI)
        	   cfg->header_type = PCI_HEADER_BRIDGE;
        else
        	   cfg->header_type = PCI_HEADER_NORMAL;
        /* Save core interrupt flag */
        cfg->int_pin = R_REG(&sb->sbtpsflag) & SBTPS_NUM0_MASK;
        /* Default to MIPS shared interrupt 0 */
        cfg->int_line = 0;
        /* MIPS sbipsflag maps core interrupt flags to interrupts 1 through 4 */
        if ((regs = sb_setcore(sbh, SB_MIPS, 0)) ||
            (regs = sb_setcore(sbh, SB_MIPS33, 0))) {
        	   sb = (sbconfig_t *)((ulong) regs + SBCONFIGOFF);
        	   val = R_REG(&sb->sbipsflag);
        	   for (cfg->int_line = 1; cfg->int_line <= 4; cfg->int_line++) {
        		      if (((val & sbips_int_mask[cfg->int_line]) >> sbips_int_shift[cfg->int_line]) == cfg->int_pin)
        			         break;
            }
            if (cfg->int_line > 4)
                cfg->int_line = 0;
        }
        /* Emulated core */
        *((uint32 *) &cfg->sprom_control) = 0xffffffff;
    } /* for */
    sb_setcoreidx(sbh, coreidx);
    return 0;
}

void
sbpci_check(void *sbh)
{
    uint coreidx;
    sbpciregs_t *pci;
    uint32 sbtopci1;
    uint32 buf[64], *ptr, i;
    ulong pa;
    volatile uint j;
    
    coreidx = sb_coreidx(sbh);
    pci = (sbpciregs_t *) sb_setcore(sbh, SB_PCI, 0);
    
    /* Clear the test array */
    pa = (ulong) DMA_MAP(NULL, buf, sizeof(buf), DMA_RX, NULL);
    ptr = (uint32 *) OSL_UNCACHED(&buf[0]);
    memset(ptr, 0, sizeof(buf));
    
    /* Point PCI window 1 to memory */
    sbtopci1 = R_REG(&pci->sbtopci1);
    W_REG(&pci->sbtopci1, SBTOPCI_MEM | (pa & SBTOPCI1_MASK));
    
    /* Fill the test array via PCI window 1 */
    ptr = (uint32 *) REG_MAP(SB_PCI_CFG + (pa & ~SBTOPCI1_MASK), sizeof(buf));
    for (i = 0; i < ARRAYSIZE(buf); i++) {
    	   for (j = 0; j < 2; j++);
    	       W_REG(&ptr[i], i);
    }
    REG_UNMAP(ptr);
    
    /* Restore PCI window 1 */
    W_REG(&pci->sbtopci1, sbtopci1);
    
    /* Check the test array */
    DMA_UNMAP(NULL, pa, sizeof(buf), DMA_RX, NULL);
    ptr = (uint32 *) OSL_UNCACHED(&buf[0]);
    for (i = 0; i < ARRAYSIZE(buf); i++) {
    	   if (ptr[i] != i)
    		      break;
    }
    
    /* Change the clock if the test fails */
    if (i < ARRAYSIZE(buf)) {
        uint32 req, cur;
    
    	   cur = sb_clock(sbh);
    	   printf("PCI: Test failed at %d MHz\n", (cur + 500000) / 1000000);
    	   for (req = 104000000; req < 176000000; req += 4000000) {
    		      printf("PCI: Resetting to %d MHz\n", (req + 500000) / 1000000);
    		      /* This will only reset if the clocks are valid and have changed */
    		      sb_mips_setclock(sbh, req, 0, 0);
    	   }
        /* Should not reach here */
        ASSERT(0);
    }
    sb_setcoreidx(sbh, coreidx);
}

