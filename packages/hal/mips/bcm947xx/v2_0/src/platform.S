##=============================================================================
##
##	platform.S
##
##	MIPS BCM947xx platform code
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
## Purpose:	
## Description:	 MIPS BCM947xx platform code
##              
######DESCRIPTIONEND####
##
##=============================================================================

#include <pkgconf/system.h>	
#include <pkgconf/hal.h>

#ifdef CYGPKG_KERNEL
# include <pkgconf/kernel.h>	
#endif
			
#include <cyg/hal/arch.inc>	


/* 32-bit address map */
#define UBASE		0x00000000		/* user+ mapped */
#define USIZE		0x80000000
#define K0BASE 		0x80000000	/* kernel unmapped cached */
#define K0SIZE 		0x20000000
#define K1BASE 		0xa0000000	/* kernel unmapped uncached */
#define K1SIZE 		0x20000000
#define K2BASE 		0xc0000000	/* kernel mapped */
#define K2SIZE 		0x40000000

#define PHYS_TO_K1(pa)	(K1BASE | (pa))

#define MIPS33_EXTIF_REGION           0x1A000000 /* Chipcommon EXTIF region*/
#define BCM95836_CPCI_LED_ADDR        (MIPS33_EXTIF_REGION + 0xC000)

/* VxWorks-specific NVRAM addresses */
#define BCM95836_CPCI_VXWORKS_MAC_ADDR      (BCM95836_CPCI_NVRAM_ADDR + 0x400)

#define LED_DIGIT(n)  (((n<<3))^3)

#define WAR_ADDR	0xb8005f9c

#define CACHE_OP( code, type )			( ((code) << 2) | (type) )

#define ICACHE_INDEX_INVALIDATE			CACHE_OP(0x0, 0)
#define ICACHE_INDEX_LOAD_TAG			CACHE_OP(0x1, 0)
#define ICACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 0)
#define DCACHE_INDEX_WRITEBACK_INVALIDATE	CACHE_OP(0x0, 1)
#define DCACHE_INDEX_LOAD_TAG			CACHE_OP(0x1, 1)
#define DCACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 1)

#define ICACHE_ADDR_HIT_INVALIDATE		CACHE_OP(0x4, 0)
#define ICACHE_ADDR_FILL			CACHE_OP(0x5, 0)
#define ICACHE_ADDR_FETCH_LOCK			CACHE_OP(0x7, 0)
#define DCACHE_ADDR_HIT_INVALIDATE		CACHE_OP(0x4, 1)
#define DCACHE_ADDR_HIT_WRITEBACK_INVALIDATE	CACHE_OP(0x5, 1)
#define DCACHE_ADDR_HIT_WRITEBACK		CACHE_OP(0x6, 1)
#define DCACHE_ADDR_FETCH_LOCK			CACHE_OP(0x7, 1)


#define CACHELINE_SIZE                  16
#define INSTRUCTION_CACHE_SIZE          (16*1024)
#define DATA_CACHE_SIZE                 (16*1024)
#define KSEG0BASE                       0x80000000
#define KSEG1BASE	                0xA0000000
#define C0_TAGLO                        $28
#define C0_TAGHI                        $29
#define	ALIGN_4W		        0xf

/* R4K specific */
#define	SR_KX		0x00000080	/* enable kernel 64bit addresses */
#define	SR_SX		0x00000040	/* enable supervisor 64bit addresses */
#define	SR_UX		0x00000020	/* enable user 64bit addresses */
#define	SR_KSUMASK	0x00000018	/* mode mask */
#define SR_KSU_K	0x00000000	/* kernel mode */
#define SR_KSU_S	0x00000008	/* supervisor mode */
#define SR_KSU_U	0x00000010	/* user mode */
#define	SR_ERL		0x00000004	/* Error Level */
#define	SR_EXL		0x00000002	/* Exception Level */
#define	SR_IE		0x00000001	/* interrupt enable, 1 => enable */

/************************************************************************/
/* FlushAll_Dcache: invaldates the entire data cache 		        */
/*									*/
/*	Syntax: void Sys_Flush_Dcache(void)				*/
/*	  Note:								*/
/************************************************************************/
FUNC_START(FlushAll_Dcache)

	.set    noreorder
    
    mfc0    t7, status          # save SR 
    nop
    and     t0, t7, ~SR_IE      # dissable interrupts 
    mtc0    t0, status
    nop
        
    li      t0, DATA_CACHE_SIZE
    li      t1, CACHELINE_SIZE 
	
	mtc0    zero, C0_TAGLO
	mtc0    zero, C0_TAGHI  	    /* TagHi is not really used */
	
	/* Calc an address that will correspond to the first cache line */
	li	t2, KSEG0BASE

	/* Calc an address that will correspond to the last cache line  */
	addu	t3, t2, t0
	subu    t3, t1

	li	t0, WAR_ADDR

	/* Loop through all lines, invalidating each of them */
1:	
	lw	t4, 0(t0)
	.set mips3
	cache	DCACHE_INDEX_WRITEBACK_INVALIDATE, 0(t2)	/* clear tag */
	.set mips0
	bne	t2, t3, 1b
	addu	t2, t1
                
    mtc0    t7, status           # restore SR
    nop

	jr	ra
	nop
                                                 
    .set    reorder
FUNC_END(FlushAll_Dcache)

/************************************************************************/
/* FlushAll_Icache: invaldates the entire instruction  cache               */
/*									*/
/*	Syntax: void Flush_Icache(void)				        */
/*	  Note:								*/
/************************************************************************/
	
FUNC_START(FlushAll_Icache)	

    .set    noreorder
        
    mfc0    t7, status           # save SR 
    nop
    and     t0, t7, ~SR_IE      # dissable interrupts 
    mtc0    t0, status
    nop
        
    li      t0, INSTRUCTION_CACHE_SIZE
    li      t1, CACHELINE_SIZE 
	
	mtc0    zero, C0_TAGLO 
	mtc0    zero, C0_TAGHI 	    /* TagHi is not really used */
	
	/* Calc an address that will correspond to the first cache line */
	li	t2, KSEG0BASE

	/* Calc an address that will correspond to the last cache line  */
	addu	t3, t2, t0
	subu    t3, t1

	/* Loop through all lines, invalidating each of them */
1:
	.set mips3
	cache	ICACHE_INDEX_INVALIDATE, 0(t2)	/* clear tag */
	.set	mips0
	bne	t2, t3, 1b
	addu	t2, t1
        
    mtc0    t7, status           # restore SR
    nop
        
    j       ra
    nop
                
	.set	reorder
    
FUNC_END(FlushAll_Icache)

/************************************************************************/
/* Invalidate_Icache: Invaldates a portion of the Instruction Cache     */
/*								       	*/
/*	Syntax: void Invalidate_Icache(ulong base_addr, ulong bcount    */
/*	  Note:								*/
/************************************************************************/
FUNC_START(Invalidate_Icache)	
	.set	noreorder
	mfc0	t7, status	# save SR
    nop
    nop
	and	t0, t7, ~SR_IE	# disable interrupts
	mtc0	t0, status
	nop

	move	t5, a0
	li	t1, ~ALIGN_4W
	and 	t5, t1		# align start of cache 16 bytes (line)

	.set 	reorder
	li	t0, ~0xf0000000	# make sure that t5 is in Kseg0
	and	t5, t0
	li	t0, K0BASE
	or	t5, t0

	move	t4, a1

1:
	addu	t4, a0		# arg0(startaddr)+min(cachesize,arg1)
	li	t0, ~0xf0000000	# make sure that t4 is in Kseg0
	and	t4, t0
	li	t0, K0BASE
	or	t4, t0
	
	la	t0, 1f	       	# switch to Kseg1
	li	t1, K1BASE
	or	t0, t1
	j	t0
	nop

1:
	move	t0, t5		# move loop variable to t0

2:      
    .set    mips3        
	cache	ICACHE_ADDR_HIT_INVALIDATE, 0x0000(t0)	            
    .set    mips0
	addu	t0, t0, 16	# Add cacheline length
	bltu	t0, t4, 2b	# while index 
	nop

	# restore status register
	.set	noreorder
	mtc0	t7, status	# restore SR
	nop
	j	ra
	nop
	.set	reorder
FUNC_END(Invalidate_Icache)        

/************************************************************************/
FUNC_START(Clear_Dcache)                
	.set	noreorder
        
	mfc0	t7, status	# save SR
    nop
    nop
        
    and	t0, t7, ~SR_IE	# disable interrupts
	mtc0	t0, status
    nop
	
    .set 	reorder

	move	t5, a0
	li      t1, ~ALIGN_4W
	and 	t5, t1		# align start of region to 16 bytes

	li	t0, ~0xf0000000	# make sure that t5 is in Kseg0
	and	t5, t0
	li	t0, K0BASE
	or	t5, t0

	move	t4, a1

1:
	addu	t4, a0		# arg0(startaddr)+min(cachesize, arg1)
	li	t0, ~0xf0000000	# make sure that t4 is in Kseg0
	and	t4, t0
	li	t0, K0BASE
	or	t4, t0
 
	move	t0, t5		
        
    li      t2, WAR_ADDR

2:      # Clear out the Data Cache!!
    .set    mips3
    lw      t1, 0(t2)
	cache	DCACHE_ADDR_HIT_WRITEBACK_INVALIDATE, 0x0000(t0) 
    .set    mips0        
    addu	t0, t0, 16	# Add cacheline length
	bltu	t0, t4, 2b	# while index < endaddr
	nop
          
	# restore status register
	.set	noreorder
	mtc0	t7, status	# restore SR
	nop
	j	ra
	nop

	.set	reorder
FUNC_END(Clear_Dcache)


/************************************************************************/
/* Invalidate_Dcache: Invaldates a portion of the Data Cache            */
/*																		*/
/*	Syntax: void Invalidate_Dcache(ulong base_addr, ulong bcount)		*/
/*	Note:																*/
/************************************************************************/
FUNC_START(Invalidate_Dcache)
	.set	noreorder
        
	mfc0	t7, status	# save SR
    nop
        
    and	t0, t7, ~SR_IE	# disable interrupts
	mtc0	t0, status
    nop
	
    .set 	reorder

	move	t5, a0
	li      t1, ~ALIGN_4W
	and 	t5, t1		# align start of region to 16 bytes

	li	t0, ~0xf0000000	# make sure that t5 is in Kseg0
	and	t5, t0
	li	t0, K0BASE
	or	t5, t0

	move	t4, a1

1:
	addu	t4, a0		# arg0(startaddr)+min(cachesize, arg1)
	li	t0, ~0xf0000000	# make sure that t4 is in Kseg0
	and	t4, t0
	li	t0, K0BASE
	or	t4, t0

	move	t0, t5		        
       	
2:      # Clear out the Data Cache!!
    .set    mips3
	cache	DCACHE_ADDR_HIT_WRITEBACK_INVALIDATE, 0x0000(t0) 
    .set    mips0        
    addu	t0, t0, 16	# Add cacheline length
	bltu	t0, t4, 2b	# while index < endaddr
	nop
          
	# restore status register
	.set	noreorder
	mtc0	t7, status	# restore SR
	nop
	j	ra
	nop

	.set	reorder
FUNC_END(Invalidate_Dcache)

/************************************************************************/
/* Flush_Dcache: Invaldates a portion of the Data Cache             	*/
/*																		*/
/*	Syntax: void Flush_Dcache(ulong base_addr, ulong bcount)			*/
/*	  Note:																*/
/************************************************************************/
FUNC_START(Flush_Dcache)

	.set	noreorder
        
	mfc0	t7, status	# save SR
    nop
        
    and	t0, t7, ~SR_IE	# disable interrupts
	mtc0	t0, status
    nop
	
    .set 	reorder

	move	t5, a0
	li      t1, ~ALIGN_4W
	and 	t5, t1		# align start of region to 16 bytes

	li	t0, ~0xf0000000	# make sure that t5 is in Kseg0
	and	t5, t0
	li	t0, K0BASE
	or	t5, t0

	move	t4, a1

1:
	addu	t4, a0		# arg0(startaddr)+min(cachesize, arg1)
	li	t0, ~0xf0000000	# make sure that t4 is in Kseg0
	and	t4, t0
	li	t0, K0BASE
	or	t4, t0

	move	t0, t5	      

 li      t2, WAR_ADDR         
	
2:  # Clear out the Data Cache!!
 .set    mips3
 lw      t1, 0(t2)
	cache	DCACHE_ADDR_HIT_WRITEBACK_INVALIDATE, 0x0000(t0) 

 .set    mips0        
 addu	t0, t0, 16	# Add cacheline length
	bltu	t0, t4, 2b	# while index < endaddr
	nop
          
	# restore status register
	.set	noreorder
	mtc0	t7, status	# restore SR
	nop
	j	ra
	nop

	.set	reorder
FUNC_END(Flush_Dcache)

/************************************************************************/
/* reset_caches: Init the D & I cache                                   */
/*									*/
/************************************************************************/
FUNC_START(reset_caches)
    /* init the data cache */
    .set noreorder

    li      t0, DATA_CACHE_SIZE
    li      t1, CACHELINE_SIZE 
	
	mtc0    zero, C0_TAGLO 
	mtc0    zero, C0_TAGHI 	    /* TagHi is not really used */
	
	/* Calc an address that will correspond to the first cache line */
	li	t2, KSEG0BASE

	/* Calc an address that will correspond to the last cache line  */
	addu	t3, t2, t0
	subu    t3, t1

	/* Loop through all lines, invalidating each of them */
1:	
	.set mips3
	cache	DCACHE_INDEX_STORE_TAG, 0(t2)	/* clear tag */
	.set mips0
	bne	t2, t3, 1b
	addu	t2, t1
        
    /* init the instr. cache */
        
    li      t0, INSTRUCTION_CACHE_SIZE
    li      t1, CACHELINE_SIZE 
	
	mtc0    zero, C0_TAGLO 
	mtc0    zero, C0_TAGHI 	    /* TagHi is not really used */
	
	/* Calc an address that will correspond to the first cache line */
	li	t2, KSEG0BASE

	/* Calc an address that will correspond to the last cache line  */
	addu	t3, t2, t0
	subu    t3, t1

	/* Loop through all lines, invalidating each of them */
1:
	.set mips3
	cache	ICACHE_INDEX_STORE_TAG, 0(t2)	/* clear tag */
	.set	mips0
	bne	t2, t3, 1b
	addu	t2, t1

	jr	ra
	nop

    .set reorder
FUNC_END(reset_caches)

##FUNC_START(hal_setTlbEntry)
##         .set noreorder
##		mtc0 a0,C0_INX
##		mtc0 a1,C0_TLBHI
##		mtc0 a2,C0_TLBLO0
##		mtc0 a3,C0_TLBLO1
##		nop
##		nop
##		tlbwi
##		nop
##.set reorder
##
##		j ra
##         nop
##FUNC_END(hal_setTlbEntry)
##
##FUNC_START(hal_setPageSize)
##        .set noreorder
##        mtc0  a0,C0_PAGEMASK
##        nop
##        nop
##        .set reorder
##        j    ra
##        nop
##FUNC_END(hal_setPageSize)




FUNC_START (hal_sysConfigOutByte)
      .set noreorder
      li   t0,0xb8002cf8
      li   t1,0xb8002cfc
      li   t2,0x3
      and  a2,t2
      sw   a0,0x0(t0)
      xori a2,0x3
      or   t1,a2
      sb   a1,0x0(t1)
      j    ra
      nop
      .set reorder
FUNC_END(hal_sysConfigOutByte)



FUNC_START(hal_sysConfigOutHalfWord)
      .set noreorder
      li   t0,0xb8002cf8
      li   t1,0xb8002cfc
      li   t2,0x3
      and  a2,t2
      sw   a0,0x0(t0)
      xori a2,0x2
      or   t1,a2
      sh   a1,0x0(t1)
      j    ra
      nop
      .set reorder
FUNC_END (hal_sysConfigOutHalfWord)

FUNC_START (hal_sysConfigOutWord)
      .set noreorder
      li   t0,0xb8002cf8
      li   t1,0xb8002cfc
      sw   a0,0x0(t0)
      sw   a1,0x0(t1)
      j    ra
      nop
      .set reorder
FUNC_END (hal_sysConfigOutWord)
     

/*
 * Function:
 *	unsigned char sysConfigInByte (int pciConfigAddress) ;
 * Inputs:
 *	a0 - pciConfigAddress
 * Outputs:
 *	v0 - byte read from PCI Configuration space.
 */ 
 FUNC_START (hal_sysConfigInByte)
sysConfigInByte:
	li	t0, 0xb8000000	/* t0 = Upper 16 bits of PCI config */
					/*	space address register */
	andi	t1, a0, 0x00000003	/* t1 = PCI config space byte offset. */
	xor	t2, t1, a0		/* t2 = PCI config address of	*/
					/*	longword containing byte. */
	xori	t1, 0x00000003	/* t1 = big endian byte offset. */
					/*	offset 0 -> 3 */
					/*	offset 1 -> 2 */
					/*	offset 2 -> 1 */
					/*	offset 3 -> 0 */
	or	t1, t0			/* t1 = byte corrected PCI config */
					/*	data address. */
	.set	noreorder		/* Instructions must not be moved ! */
	sw	t2, 0x2cf8 (t0)		/* Write PCI config space address reg */
	lw	$0, 0x2cf8 (t0)		/* wbflush() insures write-then-read */

	lbu	v0, 0x2cfc (t1)		/* v0 = PCI config space byte read */

	j	ra			/* Return to caller. */
	sw	$0, 0x2cf8 (t0)		/* BDSLOT - Park PCI bus */

	.set	reorder
FUNC_END (hal_sysConfigInByte)

/*
 * Function:
 *	unsigned short sysConfigInHalfWord (int pciConfigAddress) ;
 *
 * Inputs:
 *	a0 - pciConfigAddress
 *
 * Outputs:
 *	v0 - half word read from PCI Configuration space.
 */ 

FUNC_START (hal_sysConfigInHalfWord)
sysConfigInHalfWord:
	li	t0, 0xb8000000	/* t0 = Upper 16 bits of PCI config */
					/*	space address register */
	andi	t1, a0, 0x00000003	/* t1 = Config space halfword offset. */
	xor	t2, t1, a0		/* t2 = PCI config address of	*/
					/*	longword containing halfword. */

	xori	t1, 0x00000002	/* t1 = big endian halfword offset. */
					/*	offset 0 -> 2 */
					/*	offset 2 -> 0 */

	or	t1, t0			/* t1 = halfword corrected PCI config */
					/*	data address. */
	.set	noreorder		/* Instructions must not be moved ! */
	sw	t2, 0x2cf8 (t0)		/* Write PCI config space address reg */
	lw	$0, 0x2cf8 (t0)		/* wbflush() insures write-then-read */

	lhu	v0, 0x2cfc (t1)		/* v0 = config space halfword read */

	j	ra			/* Return to caller. */
	sw	$0, 0x2cf8 (t0)		/* BDSLOT - Park PCI bus */

	.set	reorder
FUNC_END (hal_sysConfigInHalfWord)

/*
 * Function:
 *	unsigned short sysConfigInWord (int pciConfigAddress) ;
 *
 * Inputs:
 *	a0 - pciConfigAddress
 *
 * Outputs:
 *	v0 - word read from PCI Configuration space.
 */ 

FUNC_START (hal_sysConfigInWord)
	li	t0, 0xb8000000		/* t0 = Upper 16 bits of PCI config */
					/*	space address register */
	.set	noreorder		/* Instructions must not be moved ! */
	sw	a0, 0x2cf8 (t0)		/* Write PCI config space address reg */
	lw	$0, 0x2cf8 (t0)		/* wbflush() insures write-then-read */

	lw	v0, 0x2cfc (t0)		/* v0 = config space word read */

	j	ra			/* Return to caller. */
	sw	$0, 0x2cf8 (t0)		/* BDSLOT - Park PCI bus */

	.set	reorder
FUNC_END(hal_sysConfigInWord)


FUNC_START(board_setleds)

		li	t0,PHYS_TO_K1(BCM95836_CPCI_LED_ADDR)
		rol	a0,a0,8
		sb	a0,LED_DIGIT(3)(t0)
		rol	a0,a0,8
		sb	a0,LED_DIGIT(2)(t0)
		rol	a0,a0,8
		sb	a0,LED_DIGIT(1)(t0)
		rol	a0,a0,8
		sb	a0,LED_DIGIT(0)(t0)
		j	ra

FUNC_END(board_setleds)


##-----------------------------------------------------------------------------
# Interrupt vector tables.
# These tables contain the isr, data and object pointers used to deliver
# interrupts to user code.
		
	.extern hal_default_isr
	.data

	.globl	hal_interrupt_handlers
hal_interrupt_handlers:
	.rept	20
	.long	hal_default_isr
	.endr

	.globl	hal_interrupt_data
hal_interrupt_data:
	.rept	20
	.long	0
	.endr

	.globl	hal_interrupt_objects
hal_interrupt_objects:
	.rept	20
	.long	0
	.endr
	
        .globl  cyg_hal_interrupt_level
cyg_hal_interrupt_level:
        .rept   20
        .byte   0
        .endr

	
##-----------------------------------------------------------------------------
## end of platform.S
	
