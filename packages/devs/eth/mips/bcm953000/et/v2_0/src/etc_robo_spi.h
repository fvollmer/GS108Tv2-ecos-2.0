/*
 * BCM53xx RoboSwitch utility functions
 *
 * Copyright (C) 2002 Broadcom Corporation
 *
 * $Id: etc_robo_spi.h,v 1.3 2009/07/28 09:35:43 jimhuang Exp $
 */

#ifndef _robo_spi_h_
#define _robo_spi_h_

#if defined(ROBO_OLD)
#define ROBO_CPU_PORT_PAGE  0x10
/* should set to 0x18, but 0x18 can't read the phy id value. */
#endif

#define SPI_FIFO_MAX_SIZE  32
#define SPI_INTFLAG_TIMEOUT  1000

#define SPI_STATES_DISABLE  0x0
#define SPI_STATES_ENABLE  0x1
#define SPI_STATES_WRITE  0x2
#define SPI_STATES_READ  0x4

/* SPI mode control definition */
#define SPI_MODE_CTRL_MODE  0x0        /* SPI Mode (CPOL, CPHA) */
#define SPI_MODE_CTRL_ACKEN  0x1        /* SPI RACK enable */
#define SPI_MODE_CTRL_ENDIAN  0x2        /* SPI Big Endian enable */
#define SPI_MODE_CTRL_CLOCK  0x4        /* SPI Clock divider parameter */
#define SPI_MODE_CTRL_LSBEN  0x10        /* SPI LSB first enable */

#define SPI_CCD_MAX  0xf        /* max N value for spi clock divider parameter(CCD) */

/* reutrn value for SPI driver */
#define SPI_ERR_NONE  0
#define SPI_ERR_TIMEOUT  -1
#define SPI_ERR_INTERNAL  -2
#define SPI_ERR_PARAM  -3
#define SPI_ERR_UNAVAIL  -4
#define SPI_ERR_UNKNOW  -5

#define _DD_MAKEMASK1(n) (1 << (n))
#define _DD_MAKEMASK(v,n) ((((1)<<(v))-1) << (n))
#define _DD_MAKEVALUE(v,n) ((v) << (n))
#define _DD_GETVALUE(v,n,m) (((v) & (m)) >> (n))

/* SPICONFIG: SPI Configuration Register (0x284, R/W) */
#define S_SPICFG_SS        0        /* SPI SS (device(n)) enable */
#define V_SPICFG_SS(x)    _DD_MAKEVALUE(x,S_SPICFG_SS)

#define S_SPICFG_RDC      3        /* SPI Read byte count */
#define V_SPICFG_RDC(x)   _DD_MAKEVALUE(x,S_SPICFG_RDC)

#define S_SPICFG_WDC      13        /* SPI Write data byte count */
#define V_SPICFG_WDC(x)   _DD_MAKEVALUE(x,S_SPICFG_WDC)

#define S_SPICFG_WCC      23        /* SPI Write command byte count */
#define V_SPICFG_WCC(x)   _DD_MAKEVALUE(x,S_SPICFG_WCC)

#define V_SPICFG_START    _DD_MAKEVALUE(1,31)        /* Start SPI transfer */

extern void * robo_attach(void *sih, 
    uint32 ssl, uint32 clk, uint32 mosi, uint32 miso, uint8 ss);
extern void robo_detach(void *robo);
extern void robo_switch_bus(void *robo,uint8 bustype);
extern void robo_rreg(void *robo, uint8 cid, uint8 page, uint8 addr, uint8 *buf, uint len);
extern void robo_wreg(void *robo, uint8 cid, uint8 page, uint8 addr, uint8 *buf, uint len);
extern void robo_rvmii(void *robo, uint8 cid);
extern void robo_i2c_rreg(void *robo, uint8 chipid, uint8 addr, uint8 *buf, uint len);
extern void robo_i2c_wreg(void *robo, uint8 chipid, uint8 addr, uint8 *buf, uint len);
extern void robo_i2c_rreg_intr(void *robo, uint8 chipid, uint8 *buf);
extern void robo_i2c_read_ARA(void *robo, uint8 *chipid);
#endif /* _robo_spi_h_ */
