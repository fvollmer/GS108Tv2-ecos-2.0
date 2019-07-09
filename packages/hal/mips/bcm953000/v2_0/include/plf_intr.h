#ifndef CYGONCE_HAL_PLF_INTR_H
#define CYGONCE_HAL_PLF_INTR_H
//==========================================================================
//
//      plf_intr.h
//
//      BCM947xx Interrupt and clock support
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
//              
// Date:         
// Purpose:      Define Interrupt support
// Description:  The macros defined here provide the HAL APIs for handling
//               interrupts and the clock for the BCM947xx board.
//              
// Usage:
//              #include <cyg/hal/plf_intr.h>
//              ...
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/hal/plf_io.h>

#define HAL_PLATFORM_RESET()            CYG_EMPTY_STATEMENT

#define HAL_PLATFORM_RESET_ENTRY        0xbfc00000

/* Virtual IRQ base, after last hardware IRQ */
#define SBMIPS_VIRTIRQ_BASE             6

//--------------------------------------------------------------------------
// Interrupt vectors.

#ifndef CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED
// the default for all MIPS variants is to use the 6 bits
// in the cause register.

#define CYGNUM_HAL_INTERRUPT_CHIPCOMMON       0
#define CYGNUM_HAL_INTERRUPT_GMAC1            1
#define CYGNUM_HAL_INTERRUPT_GMAC0            2
#define CYGNUM_HAL_INTERRUPT_PCIE_PORT0       3
#define CYGNUM_HAL_INTERRUPT_PCIE_PORT1       4
#define CYGNUM_HAL_INTERRUPT_TIMER            5

// shared mips int 0
#define CYGNUM_HAL_INT0_BIT0                  8
#define CYGNUM_HAL_INT0_BIT1                  9
#define CYGNUM_HAL_INT0_BIT2                  10
#define CYGNUM_HAL_INT0_BIT3                  11
#define CYGNUM_HAL_INT0_BIT4                  12
#define CYGNUM_HAL_INT0_BIT5                  13



// Min/Max ISR numbers and how many there are
#define CYGNUM_HAL_ISR_MIN                     0
#define CYGNUM_HAL_ISR_MAX                     19
#define CYGNUM_HAL_ISR_COUNT                   20

// The vector used by the Real time clock. The default here is to use
// interrupt 5, which is connected to the counter/comparator registers
// in many MIPS variants.

#ifndef CYGNUM_HAL_INTERRUPT_RTC
#define CYGNUM_HAL_INTERRUPT_RTC            CYGNUM_HAL_INTERRUPT_TIMER
#endif


#define CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED

#endif


//--------------------------------------------------------------------------
// Interrupt controller access
// The default code here simply uses the fields present in the CP0 status
// and cause registers to implement this functionality.
// Beware of nops in this code. They fill delay slots and avoid CP0 hazards
// that might otherwise cause following code to run in the wrong state or
// cause a resource conflict.

#ifndef CYGHWR_HAL_INTERRUPT_CONTROLLER_ACCESS_DEFINED

#define HAL_INTERRUPT_MASK( _vector_ )          \
{                                               \
    cyg_uint32 __vector = _vector_;             \
    if (__vector < 6) {                         \
        asm volatile (                              \
            "mfc0   $3,$12\n"                       \
            "la     $2,0x00000400\n"                \
            "sllv   $2,$2,%0\n"                     \
            "nor    $2,$2,$0\n"                     \
            "and    $3,$3,$2\n"                     \
            "mtc0   $3,$12\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            "nop; nop; nop\n"                       \
            :                                       \
            : "r"(__vector)                         \
            : "$2", "$3"                            \
            );                                      \
    } else {                                        \
        *(volatile unsigned int *)0xb8003014 &=     \
            ~(1 << (__vector - 6));                 \
    }                                               \
}

#define HAL_INTERRUPT_UNMASK( _vector_ )        \
{                                               \
    cyg_uint32 __vector = _vector_;             \
    if (__vector >= 6) {                        \
        *(volatile unsigned int *)0xb8003014 |= \
            (1 << (__vector - 6));              \
        __vector = 0;                           \
    }                                           \
    asm volatile (                              \
        "mfc0   $3,$12\n"                       \
        "la     $2,0x00000400\n"                \
        "sllv   $2,$2,%0\n"                     \
        "or     $3,$3,$2\n"                     \
        "mtc0   $3,$12\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        :                                       \
        : "r"(__vector)                         \
        : "$2", "$3"                            \
        );                                      \
}

#define HAL_INTERRUPT_ACKNOWLEDGE( _vector_ )   \
{                                               \
    cyg_uint32 __vector = _vector_;             \
    if (__vector >= 6) {                        \
        __vector = 0;                           \
    }                                           \
    asm volatile (                              \
        "mfc0   $3,$13\n"                       \
        "la     $2,0x00000400\n"                \
        "sllv   $2,$2,%0\n"                     \
        "nor    $2,$2,$0\n"                     \
        "and    $3,$3,$2\n"                     \
        "mtc0   $3,$13\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        "nop; nop; nop\n"                       \
        :                                       \
        : "r"(__vector)                         \
        : "$2", "$3"                            \
        );                                      \
}

#define HAL_INTERRUPT_CONFIGURE( _vector_, _level_, _up_ )

#define HAL_INTERRUPT_SET_LEVEL( _vector_, _level_ )

#define CYGHWR_HAL_INTERRUPT_CONTROLLER_ACCESS_DEFINED

#endif


#endif /* ifndef CYGONCE_HAL_PLF_INTR_H */
//--------------------------------------------------------------------------
// End of plf_intr.h
