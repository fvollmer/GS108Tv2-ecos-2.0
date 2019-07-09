/*
 * Copyright(c) 2006 Broadcom Corp.
 * All Rights Reserved.
 */

#ifndef __ECOSBSP_H
#define	__ECOSBSP_H

#include "sbconfig.h"
#include "sbutils.h"

#ifndef KSEG1ADDR
#define KSEG1ADDR(_a) ((unsigned long)(_a) | 0xA0000000)
#endif

/* Interrupt mappings */
#define DEF_SBINTVEC    0x00000001      /* MIPS_timer */ 
#define DEF_SBIPSFLAG   0x043F0201      /* 4=pci, 3=et1, 2=et0, 1=uart */

/* uart settings */
#define CONSOLE_BAUD_RATE     9600      /* console baud rate */
#define COM1_FREQ          1843200


extern uint32 get_sb_clock(void); /* routine to return sb clock */

#endif	/* __ECOSBSP_H */
