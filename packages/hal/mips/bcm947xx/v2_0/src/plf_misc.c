//==========================================================================
//
//      plf_misc.c
//
//      HAL platform miscellaneous functions
//
//==========================================================================
//####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with eCos; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//
// Alternative licenses for eCos may be arranged by contacting Red Hat, Inc.
// at http://sources.redhat.com/ecos/ecos-license/
// -------------------------------------------
//####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    
// Contributors: 
// Date:         
// Purpose:      HAL miscellaneous functions
// Description:  This file contains miscellaneous functions provided by the
//               HAL.
//
//####DESCRIPTIONEND####
//
//========================================================================*/

#include <pkgconf/hal.h>
#include <pkgconf/system.h>

#include CYGBLD_HAL_PLATFORM_H

#include <cyg/infra/cyg_type.h>         // Base types
#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros
#include <cyg/hal/hal_arch.h>           // architectural definitions
#include <cyg/hal/hal_intr.h>           // Interrupt handling
#include <cyg/hal/hal_cache.h>          // Cache handling

#include <cyg/hal/typedefs.h>
#include <cyg/hal/bcmdevs.h>
#include <cyg/hal/bcmnvram.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/sbconfig.h>
#include <cyg/hal/sbextif.h>
#include <cyg/hal/sbchipc.h>
#include <cyg/hal/sbmips.h>
#include <cyg/hal/sbmemc.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/sbutils.h>
#include <cyg/hal/sbmips.h>
#include <cyg/hal/sbpci.h>

#ifdef CYGPKG_IO_PCI
#include <cyg/io/pci_hw.h>
#endif

extern void cyg_hal_plf_serial_add(void *regs, uint irq, uint baud_base, uint reg_shift);
extern void pc_serial_info0_add(void *regs, uint irq, uint baud_base, uint reg_shift);
extern void pc_serial_info1_add(void *regs, uint irq, uint baud_base, uint reg_shift);

static void *sbh;
static void *sbpci;
static chipcregs_t *cc;
static ser_count = 0;

/*------------------------------------------------------------------------*/
    
static void
plf_serial_add(void *regs, uint irq, uint baud_base, uint reg_shift)
{
    if (ser_count > 1)
        return;
#if 0
    if (ser_count == 0)
        pc_serial_info0_add( regs, (irq -1), baud_base, reg_shift);
    else if (ser_count == 1)
        pc_serial_info1_add( regs, (irq -1), baud_base, reg_shift);
#endif
    cyg_hal_plf_serial_add( regs, irq, baud_base, reg_shift);

    ser_count++;
}

/* this is called from the kernel */
void hal_platform_init(void)
{

    sbh = sb_kattach();
    cc = sb_setcore(sbh, SB_CC, 0);

    sb_mips_init(sbh);

    sb_serial_init(sbh, plf_serial_add);

    hal_if_init();
}

#ifdef CYGPKG_IO_PCI
void cyg_hal_plf_pci_init()
{

    sbpci = sb_kattach();
    sbpci_init(sbpci);
}
#endif



/* PCI read and write functions */

cyg_uint8 cyg_hal_plf_pci_cfg_read_byte(int busNo, 
                                        int devFnNo,
                                        int regOffset)
{
      cyg_uint8 config_byte;

      sbpci_read_config (sbpci, busNo, CYG_PCI_DEV_GET_DEV(devFnNo),
                         CYG_PCI_DEV_GET_FN(devFnNo), regOffset,
                         &config_byte, sizeof(config_byte));
      return (config_byte);
                   
}


cyg_uint16 cyg_hal_plf_pci_cfg_read_word(int busNo, 
                                         int devFnNo,
                                         int regOffset)
{
      cyg_uint16 config_byte;

      sbpci_read_config (sbpci, busNo, CYG_PCI_DEV_GET_DEV(devFnNo),
                         CYG_PCI_DEV_GET_FN(devFnNo), regOffset,
                         &config_byte, sizeof(config_byte));
      return (config_byte);
                   
}

cyg_uint32 cyg_hal_plf_pci_cfg_read_dword(int busNo, 
                                          int devFnNo,
                                          int regOffset)
{
      cyg_uint32 config_byte;
	  
      sbpci_read_config (sbpci, busNo, CYG_PCI_DEV_GET_DEV(devFnNo),
                         CYG_PCI_DEV_GET_FN(devFnNo), regOffset,
                         &config_byte, sizeof(config_byte));
      return (config_byte);
                   
}

void cyg_hal_plf_pci_cfg_write_byte(int busNo, 
                                    int devFnNo,
                                    int regOffset,
                                    cyg_uint8 data)
{

      sbpci_write_config (sbpci, busNo, CYG_PCI_DEV_GET_DEV(devFnNo),
                         CYG_PCI_DEV_GET_FN(devFnNo), regOffset,
                         &data, sizeof (data));
                   
}

void cyg_hal_plf_pci_cfg_write_word(int busNo, 
                                    int devFnNo,
                                    int regOffset,
                                    cyg_uint16 data)
{

      sbpci_write_config (sbpci, busNo, CYG_PCI_DEV_GET_DEV(devFnNo),
                         CYG_PCI_DEV_GET_FN(devFnNo), regOffset,
                         &data, sizeof (data));
}

void cyg_hal_plf_pci_cfg_write_dword(int busNo, 
                                     int devFnNo,
                                     int regOffset,
                                     cyg_uint32 data)
{


      sbpci_write_config (sbpci, busNo, CYG_PCI_DEV_GET_DEV(devFnNo),
                         CYG_PCI_DEV_GET_FN(devFnNo), regOffset,
                         &data, sizeof (data));
                   
}

void
udelay(int delay)
{
    hal_delay_us (delay);
    return;
}

void sysReboot(void)
{
    sbh = sb_kattach();
    sb_watchdog(sbh, 1);
}
/*------------------------------------------------------------------------*/
/* End of plf_misc.c                                                      */

