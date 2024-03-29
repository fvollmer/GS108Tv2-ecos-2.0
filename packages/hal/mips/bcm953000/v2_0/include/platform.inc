#ifndef CYGONCE_HAL_PLATFORM_INC
#define CYGONCE_HAL_PLATFORM_INC

##=============================================================================
##
##  platform.inc
##
##  BCM947xx board assembler header file
##
##=============================================================================
#####ECOSGPLCOPYRIGHTBEGIN####
## -------------------------------------------
## This file is part of eCos, the Embedded Configurable Operating System.
## Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
##
## eCos is free software; you can redistribute it and/or modify it under
## the terms of the GNU General Public License as published by the Free
## Software Foundation; either version 2 or (at your option) any later version.
##
## eCos is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
## for more details.
##
## You should have received a copy of the GNU General Public License along
## with eCos; if not, write to the Free Software Foundation, Inc.,
## 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
##
## As a special exception, if other files instantiate templates or use macros
## or inline functions from this file, or you compile this file and link it
## with other works to produce a work based on this file, this file does not
## by itself cause the resulting work to be covered by the GNU General Public
## License. However the source code for this file must still be made available
## in accordance with section (3) of the GNU General Public License.
##
## This exception does not invalidate any other reasons why a work based on
## this file might be covered by the GNU General Public License.
##
## Alternative licenses for eCos may be arranged by contacting Red Hat, Inc.
## at http://sources.redhat.com/ecos/ecos-license/
## -------------------------------------------
#####ECOSGPLCOPYRIGHTEND####
##=============================================================================
#######DESCRIPTIONBEGIN####
##
## Author(s):    
## Contributors: 
## Date:     
## Purpose:  BCM95300x board definitions.
## Description:  This file contains various definitions and macros that are
##               useful for writing assembly code for the BCM95300x reference
##               board.
## Usage:
##       #include <cyg/hal/platform.inc>
##       ...
##      
##
######DESCRIPTIONEND####
##
##=============================================================================

#include <cyg/hal/mips.inc>
#include <cyg/hal/mipsinc.h>

#------------------------------------------------------------------------------
# Macro for copying vectors to RAM if necessary.
#if !defined(CYGSEM_HAL_USE_ROM_MONITOR)

    .macro  hal_vectors_init

    # If we don~t play nice with a ROM monitor, copy the required
    # vectors into the proper location.
    la  t0,0x80000000       # dest addr
    la  t1,utlb_vector      # source addr
    la  t3,utlb_vector_end  # end dest addr
1:  
    lw  v0,0(t1)        # get word
    addi    t1,t1,4
    sw  v0,0(t0)        # write word
    addi    t0,t0,4
    bne t1,t3,1b
     nop

    la  t0,0x80000180       # dest addr
    la  t1,other_vector     # source addr
    la  t3,other_vector_end # end dest addr
1:  
    lw  v0,0(t1)        # get word
    addi    t1,t1,4
    sw  v0,0(t0)        # write word
    addi    t0,t0,4
    bne t1,t3,1b
     nop

    .set mips3                      # Set ISA to MIPS 3 to allow cache insns
    # Now clear the region in the caches
    la  t0,0x80000000       # dest addr
    ori t1,t0,0x200     # source addr
1:  cache   0x01,0x0000(t0)         # Flush word from data cache
    cache   0x01,0x1000(t0)
    cache   0x01,0x2000(t0)
    cache   0x01,0x3000(t0)
    nop
    cache   0x00,0x0000(t0)     # Invalidate icache for word
    cache   0x00,0x1000(t0)
    cache   0x00,0x2000(t0)
    cache   0x00,0x3000(t0)
    nop
    addi    t0,t0,0x20
    bne t0,t1,1b
     nop
    .set mips0                      # reset ISA to default

    .endm

#else

    .macro  hal_vectors_init
    .endm

#endif

#------------------------------------------------------------------------------
# Monitor initialization.
    
#ifndef CYGPKG_HAL_MIPS_MON_DEFINED

#if defined(CYG_HAL_STARTUP_ROM) ||         \
    (   defined(CYG_HAL_STARTUP_RAM) &&     \
        !defined(CYGSEM_HAL_USE_ROM_MONITOR))
    
    # If we are starting up from ROM, or we are starting in
    # RAM and NOT using a ROM monitor, initialize the VSR table.

    .macro  hal_mon_init
    hal_vectors_init
    
    # Set default exception VSR for all vectors
    ori     a0,zero,16
    la  a1,__default_exception_vsr
    la  a2,hal_vsr_table
1:  sw      a1,0(a2)
    addi    a2,a2,4
    addi    a0,a0,-1
    bne     a0,zero,1b
     nop

    # Now set special VSRs
    la  a0,hal_vsr_table
    # Set interrupt VSR
    la  a1,__default_interrupt_vsr
    sw      a1,0*4(a0)
    # Add special handler on breakpoint vector to allow GDB and
    # GCC to both use 'break' without conflicts.
    la  a1,__break_vsr_springboard
    sw  a1,9*4(a0)
    # Set exception handler on special vectors
    # FIXME: Should use proper definitions
    la  a1,__default_exception_vsr
    sw  a1,32*4(a0)             # debug
    sw  a1,33*4(a0)             # utlb
    sw  a1,34*4(a0)             # nmi
    .endm
    
#elif defined(CYG_HAL_STARTUP_RAM) && defined(CYGSEM_HAL_USE_ROM_MONITOR)

    # Initialize the VSR table entries
    # We only take control of the interrupt vector,
    # the rest are left to the ROM for now...

    .macro  hal_mon_init
    hal_vectors_init
        # Set interrupt VSR
        la  a0,hal_vsr_table
        la  a1,__default_interrupt_vsr
        sw      a1,0*4(a0)
    .endm

#else

    .macro  hal_mon_init
    hal_vectors_init
    .endm

#endif

#define CYGPKG_HAL_MIPS_MON_DEFINED
#endif  


#--------------------------------------------------------------
#  
# Define hal_intc_translate for shared IRQ0 
#
#----------------------------------------------------------------------
#ifndef CYGPKG_HAL_MIPS_INTC_TRANSLATE_DEFINED

    .macro  hal_intc_translate inum,vnum,ists
    
    #
    # TODO: flexible assignment
    #
    
    # If not IRQ0, vector == interrupt number
    move    a0, \inum
    bne     a0, zero, 1f
    nop
    
    # AND CPU core INTSTATUS and INTMASK[0]
    li      v0, 0xb8003010
    lw      v1, 0(v0)
    li      v0, 0xb8003014
    lw      a1, 0(v0)
    and     v1, v1, a1
    move    \ists, v1
    
    # Check which core asserts interrupt
    li      a0, 6             
    li      a1, 0x00000001
2:
    and     t1, v1, a1
    beq     t1, a1, 1f
    nop

    addi    a0, a0, 1
    sll     a1, a1, 1
    j       2b
    
1:  
    move    \vnum,a0
    .endm

#define CYGPKG_HAL_MIPS_INTC_TRANSLATE_DEFINED
#endif 


#--------------------------------------------------------------
# Decide whether the VSR table is defined externally, or is to be defined
# here.

## ISR tables are defined in platform.S
#define CYG_HAL_MIPS_ISR_TABLES_DEFINED

## VSR table is at a fixed RAM address defined by the linker script
#define CYG_HAL_MIPS_VSR_TABLE_DEFINED

#----------------------------------------------------------------------
#ifndef CYGPKG_HAL_MIPS_MEMC_DEFINED

.macro  hal_memc_init
        .extern board_draminit
        lar     t0,board_draminit
        jalr    t0
        nop

.endm
#define CYGPKG_HAL_MIPS_MEMC_DEFINED
#endif

##-----------------------------------------------------------------------------
#ifdef CYG_STARTUP_ROM

## Initial SR value for use in ROM:
## CP0 usable
## Vectors in RAM
## FP registers are 32 bit
## All hw ints disabled
#define INITIAL_SR  0x30000000

#else

## Initial SR value for use standalone:
## CP0 usable
## Vectors to RAM
## FP registers are 32 bit
## All hw ints disabled
#define INITIAL_SR  0x30000000

#endif                   

#------------------------------------------------------------------------------
#endif // ifndef CYGONCE_HAL_PLATFORM_INC
# end of platform.inc
