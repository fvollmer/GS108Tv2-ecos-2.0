/*
 * SiliconBackplane Chipcommon core hardware definitions.
 *
 * The chipcommon core provides chip identification, SB control,
 * jtag, 0/1/2 uarts, clock frequency control, a watchdog interrupt timer,
 * gpio interface, extbus, and support for serial and parallel flashes.
 *
 * Copyright (C) 2004 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef	_SBCHIPC_H
#define	_SBCHIPC_H

#define CONFIG_BCM56218
/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */

#if defined(CONFIG_BCM56218)

#define	CID_ID_MASK		0x0000ffff		/* Chip Id mask */
#define	CID_REV_MASK		0x00ff0000		/* Chip Revision mask */
#define	CID_REV_SHIFT		16			/* Chip Revision shift */

/* Prefered flash window in chipcommon */
#define CC_FLASH_BASE       0xbc000000  /* Chips with chipcommon cores */
#define CC_FLASH_MAX        0x02000000  /* Maximum flash size with chipc */

#ifndef _LANGUAGE_ASSEMBLY
typedef volatile struct {
	uint32	chipid;			/* FIXME (not sure if its there*/   
	uint32	capabilities;
	uint32	PAD[6];

	/* Interrupt control */
	uint32	intstatus;		/* 0x20 */
	uint32	PAD[15];

	/* gpio - cleared only by power-on-reset */
	uint32	gpioin;			/* 0x60 */
	uint32	gpioout;
	uint32	gpioouten;
	uint32	gpiocontrol;
	uint32	gpiointpolarity;
	uint32	gpiointmask;
	uint32	PAD[2];

	/* Watchdog timer */
	uint32	watchdog;		/* 0x80 */
	uint32	PAD[3];

	/* PLL control */
	uint32	pllcontrol1;            /* 0x90 */
	uint32	pllcontrol2;
	uint32	pllstatus;
	uint32	PAD[25];

	/* 0x100 */
	uint32	sbipsflag;
	uint32	sbintvec;
	uint32	serrconfig;
	uint32	memcclkcontrol;
	uint32	icsreset;
	uint32	peripheralreset;
	uint32	PAD[2];

	/* 0x120 */
	uint32	membist;
	uint32	membiststatus;
	uint32	memcbist;
	uint32	parallelflashwaitcnt;
	uint32	PAD[116];

	/* uarts */
	uint8	uart0data;		/* 0x300 */
	uint8	uart0imr;
	uint8	uart0fcr;
	uint8	uart0lcr;
	uint8	uart0mcr;
	uint8	uart0lsr;
	uint8	uart0msr;
	uint8	uart0scratch;
	uint8	PAD[248];		/* corerev >= 1 */

	uint8	uart1data;		/* 0x400 */
	uint8	uart1imr;
	uint8	uart1fcr;
	uint8	uart1lcr;
	uint8	uart1mcr;
	uint8	uart1lsr;
	uint8	uart1msr;
	uint8	uart1scratch;
} chipcregs_t;
#endif

#endif /* CONFIG_BCM56218 */
#endif	/* _SBCHIPC_H */
