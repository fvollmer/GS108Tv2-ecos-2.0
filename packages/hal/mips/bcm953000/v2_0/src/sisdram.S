/*
 * BCM947XX DRAM init & sizing
 *
 * Used by both cfe and shared/boot.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: sisdram.S,v 1.1 2009/01/05 07:19:05 cchao Exp $
 */

#include <cyg/hal/mipsinc.h>
#include <cyg/hal/hndsoc.h>
#include <cyg/hal/sbchipc.h>
#include <cyg/hal/bcmdevs.h>

/* Debug macro - write a number to memory - use it with caution,
 *  it changes k0 and k1 registers.
 */
#ifdef	BCMDBG
#define TRACEINIT(x) \
	li	k0,KSEG1ADDR(0x18000044); \
	li	k1,x; \
	sw	k1,0(k0)
	
#define TRACE(x) \
	li	k1,x; \
	sw	k1,0(k0)
#else
#define TRACEINIT(x)
#define TRACE(x)
#endif	/* BCMDBG */

	.text
	LEAF(board_draminit)
	.set	noreorder

	TRACEINIT(0x535301)
	move	gp,ra

	/* Figure out if we have an SB or AI chip */
	li	s2,KSEG1ADDR(SI_ENUM_BASE)	# s2 = SI_ENUM_BASE
	li	s3,KSEG1			# s3 = KSEG1
	li	t0,CID_TYPE_MASK
	lw	t1,CC_CHIPID(s2)
	and	t1,t0,t1
	srl	s7,t1,CID_TYPE_SHIFT		# s7 = ChipType (0 for SB, = 1 for AI)

	/* Check if we booted from flash, compute reloc for text addresses */
	bal	1f
	nop

1:	li	t0,PHYSADDR_MASK
	and	t0,t0,ra
	li	t1,SI_FLASH1
	blt	t0,t1,2f
	move	s5,zero
	la	t0,1b
	sub	s5,ra,t0			# s5: Relocation factor

	/* Call appropriate draminit for chip type */
2:	TRACE(0x535302)
	la	t2,sb_draminit
	beqz	s7,3f
	nop
	TRACE(0x535303)
	la	t2,ai_draminit
3:	add	t2,t2,s5
	jalr	t2
	nop

	/* Size memory if needed (Need to reinit TRACE after sb_draminit) */
	TRACEINIT(0x535304)
	beqz	v0,szmem
	nop
	
	li	a0,-1				# -1 means no controller
	bne	v0,a0,4f
	nop

	TRACE(0x5353ff)
sisdead:
	b	sisdead
	nop


4:	jr	gp
	nop

szmem:
  li      v0,(1 << 24)
  jr      gp
  nop

	TRACE(0x535305)
	li	s3,KSEG1			# s3 = KSEG1
	li	t2,0xaa55beef
	sw	zero,0x3c(s3)
	li	v0,(1 << 20)
	li	v1,(128 << 20)

5:	or	t0,v0,s3
	sw	t2,0x3c(t0)
	lw	t1,0x3c(s3)
	beq	t1,t2,6f
	nop

	sll	v0,v0,1
	bne	v0,v1,5b
	nop
	/* Didn't find an alias, must be 128MB */

6:	jr	gp
	nop

	END(board_draminit)
