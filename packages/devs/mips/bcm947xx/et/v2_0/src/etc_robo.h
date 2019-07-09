/*
 * BCM53xx RoboSwitch utility functions
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 *
 * $Id: etc_robo.h,v 1.2 Exp $
 */

#ifndef _robo_h_
#define _robo_h_

#if defined(ROBO_OLD)
#define ROBO_CPU_PORT_PAGE 0x10
/* should set to 0x18, but 0x18 can't read the phy id value. */
#endif

extern void * robo_attach(void *sbh, uint32 ssl, uint32 clk, uint32 mosi, uint32 miso);
extern void robo_detach(void *robo);
extern void robo_switch_bus(void *robo,uint8 bustype);
extern void robo_rreg(void *robo, uint8 cid, uint8 page, uint8 addr, uint8 *buf, uint len);
extern void robo_wreg(void *robo, uint8 cid, uint8 page, uint8 addr, uint8 *buf, uint len);
extern void robo_rvmii(void *robo, uint8 cid);
extern void robo_i2c_rreg(void *robo, uint8 chipid, uint8 addr, uint8 *buf, uint len);
extern void robo_i2c_wreg(void *robo, uint8 chipid, uint8 addr, uint8 *buf, uint len);
extern void robo_i2c_rreg_intr(void *robo, uint8 chipid, uint8 *buf);
extern void robo_i2c_read_ARA(void *robo, uint8 *chipid);

#endif /* _robo_h_ */
