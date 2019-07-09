//====================================================================
//
//      bcm953000_flash.c
//
//      FLASH memory - platform support for BCM953000  flash memory
//
//====================================================================
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
//====================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):      
// Original data:  
// Contributors:   
// Date:           
//
//####DESCRIPTIONEND####
//
//====================================================================
#include <pkgconf/hal.h>
#include <cyg/io/flash_bcm953000.h>
#define  _FLASH_PRIVATE_
#include <cyg/io/flash.h>
#include <cyg/io/flashDrvLib.h>
#include <cyg/infra/diag.h>

void cyg_plf_bcm953000_flash_init(void)
{
    
}

int
flash_query(void *data)
{
    /* NOT implemented yet */
    return 0;
}

int
flash_hwr_init(void)
{
    int blocksize;

diag_printf("flashDrvLibInit \n");    
    CYGHWR_FLASH_CFI_PLF_INIT();
    
    if (flashDrvLibInit() == ERROR)
        diag_printf("flashDrvLibInit failed\n");
diag_printf(" after flashDrvLibInit\n");    
    flash_info.blocks = flashSectorCount;
    flash_info.block_size = FLASH_DEVICE_SECTOR_SIZE;
    flash_info.buffer_size = 0;
    flash_info.start = (void *)FLASH_BASE_ADDRESS;
    flash_info.end = (void *)(FLASH_BASE_ADDRESS + FLASH_SIZE); 

    return (FLASH_ERR_OK);
}

int
flash_hwr_map_error(int err)
{
    (*flash_info.pf)("Err = %x\n", err);
        return err;
}

int flash_erase_block(volatile unsigned short *block, unsigned int block_size)
{
    CYG_UNUSED_PARAM(unsigned int, block_size);
    int sector = FLASH_SECTOR_NUMBER((int)block);
    flashEraseBank(sector, 1);
    return (FLASH_ERR_OK);
}

int
flash_program_buf(volatile void *addr, void *data, int len,
                  unsigned long block_mask, int buffer_size)
{
    CYG_UNUSED_PARAM(unsigned long, block_mask);
    CYG_UNUSED_PARAM(int, buffer_size);
    int sector = FLASH_SECTOR_NUMBER((int)addr);
    int offset = ((int)addr - FLASH_BASE_ADDRESS) % FLASH_SECTOR_SIZE;
    
    if (ERROR == flashBlkWrite(sector, data, offset, len)) {
        return FLASH_ERR_PROGRAM;
    }
    
    return (FLASH_ERR_OK);
}
