/*
 * Copyright(c) 2006 Broadcom Corp.
 * All Rights Reserved.
 */

#ifndef __ECOSBSP_H
#define	__ECOSBSP_H

#include "sbconfig.h"
#include "sbutils.h"


/* Interrupt mappings */
#define DEF_SBINTVEC    0x00000010  /* Shared interrupts for timer */
#define DEF_SBIPSFLAG   0x03020100  /* 1=uart0, 2=uart1 and 3=cmic 4=gpio */

#define INTNUM_UART0    0x1
#define INTNUM_UART1    0x2
#define INTNUM_CMIC     0x3
#define INTNUM_GPIO     0x4

/* uart settings */
#define CONSOLE_BAUD_RATE     9600      /* console baud rate */
#define COM1_FREQ  ICS_SB_CLK_FREQ

#define SYS_HARD_RESET()   \
    { { *(volatile uint32 *)(ICS_SRESET_REG) = 0;   \
          *(volatile uint32 *)(ICS_SRESET_REG) = 1; \
      }}
      
extern uint32 get_sb_clock(void); /* routine to return sb clock */

#endif	/* __ECOSBSP_H */
