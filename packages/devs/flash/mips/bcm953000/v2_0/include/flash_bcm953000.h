#ifndef CYGONCE_DEVS_FLASH_BCM953000_H
#define CYGONCE_DEVS_FLASH_BCM953000_H
//==========================================================================
//
//      flash_bcm953000.h
//
//      Flash programming - device constants, etc.
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
// Purpose:      
// Description:  
//              
//####DESCRIPTIONEND####
//
//==========================================================================


extern void cyg_plf_bcm953000_flash_init(void);

#define  CYGHWR_FLASH_CFI_PLF_INIT     cyg_plf_bcm953000_flash_init

#define CYGNUM_CFI_BASE_ADDRESS        0xbc000000       
#define CYGNUM_CFI_FLASH_SIZE          0x400000
#define CYGNUM_CFI_BUSWIDTH            2 /* 1 = 8-bits data bus */ /* 2; 16-bits */

#endif  // CYGONCE_DEVS_FLASH_BCM953000_H

