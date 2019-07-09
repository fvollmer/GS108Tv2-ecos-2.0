#ifndef CYGONCE_PLF_CACHE_H
#define CYGONCE_PLF_CACHE_H
//=============================================================================
//
//      plf_cache.h
//
//      HAL cache control API
//
//=============================================================================
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
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    
// Contributors:
// Date:         
// Purpose:      Cache control API
// Description:  The macros defined here provide the HAL APIs for handling
//               cache control operations.
// Usage:
//               #include <cyg/hal/hal_cache.h>
//               ...
//              
//
//####DESCRIPTIONEND####
//
//=============================================================================

//-----------------------------------------------------------------------------
// Cache dimensions

// Data cache
#define HAL_DCACHE_SIZE                 (1024 * 32)   // Size of data cache in bytes
#define HAL_DCACHE_LINE_SIZE            32      // Size of a data cache line
#define HAL_DCACHE_WAYS                 4       // Associativity of the cache

// Instruction cache
#define HAL_ICACHE_SIZE                 (1024*32)   // Size of cache in bytes
#define HAL_ICACHE_LINE_SIZE            32      // Size of a cache line
#define HAL_ICACHE_WAYS                 4       // Associativity of the cache

#define HAL_DCACHE_SETS (HAL_DCACHE_SIZE/(HAL_DCACHE_LINE_SIZE*HAL_DCACHE_WAYS))
#define HAL_ICACHE_SETS (HAL_ICACHE_SIZE/(HAL_ICACHE_LINE_SIZE*HAL_ICACHE_WAYS))

#ifdef BCM_ROBO_SUPPORT
#define HAL_DCACHE_WRITETHRU_MODE       0
#define HAL_DCACHE_WRITEBACK_MODE       1
#else /* !BCM_ROBO_SUPPORT */
#define HAL_DCACHE_WRITETHRU_MODE       1
#define HAL_DCACHE_WRITEBACK_MODE       0
#endif /* BCM_ROBO_SUPPORT */

/* externals, defined in platform.S */
extern void reset_caches(void);
extern void FlushAll_Dcache(void);
extern void FlushAll_Icache(void);
extern void Invalidate_Icache(void * base_addr, CYG_WORD bcount);
extern void Invalidate_Dcache(void * base_addr, CYG_WORD bcount);
extern void Clear_Dcache(void * base_addr, CYG_WORD bcount);
extern void Flush_Dcache(void * base_addr, CYG_WORD bcount);
                       
//-----------------------------------------------------------------------------
// Global control of data cache

// Invalidate the entire cache
#define HAL_DCACHE_INVALIDATE_ALL_DEFINED
#define HAL_DCACHE_INVALIDATE_ALL()                                                    \
    CYG_MACRO_START                                                                    \
    FlushAll_Dcache();                                                                 \
    CYG_MACRO_END

// Synchronize the contents of the cache with memory.
__externC void hal_dcache_sync(void);
#define HAL_DCACHE_SYNC_DEFINED
#define HAL_DCACHE_SYNC() hal_dcache_sync()

#define HAL_DCACHE_FLUSH_DEFINED
#define HAL_DCACHE_FLUSH( _base_ , _asize_ )                                           \
    CYG_MACRO_START                                                                    \
    Flush_Dcache(_base_, _asize_);                                                     \
    CYG_MACRO_END


// Invalidate cache lines in the given range without writing to memory.
#define HAL_DCACHE_INVALIDATE_DEFINED
#define HAL_DCACHE_INVALIDATE( _base_ , _asize_ )                                       \
    CYG_MACRO_START                                                                     \
    Invalidate_Dcache(_base_, _asize_);                                                 \
    CYG_MACRO_END


//-----------------------------------------------------------------------------
// Global control of Instruction cache

// Invalidate the entire cache
#define HAL_ICACHE_INVALIDATE_ALL_DEFINED
#define HAL_ICACHE_INVALIDATE_ALL()                                                     \
    CYG_MACRO_START                                                                     \
    FlushAll_Icache();                                                                  \
    CYG_MACRO_END

//-----------------------------------------------------------------------------
// Instruction cache line control

// Invalidate cache lines in the given range without writing to memory.
#define HAL_ICACHE_INVALIDATE_DEFINED
#define HAL_ICACHE_INVALIDATE( _base_ , _asize_ )                                       \
    CYG_MACRO_START                                                                     \
    Invalidate_Icache(_base_, _asize_);                                                 \
    CYG_MACRO_END

//-----------------------------------------------------------------------------
#endif // ifndef CYGONCE_PLF_CACHE_H
// End of plf_cache.h
