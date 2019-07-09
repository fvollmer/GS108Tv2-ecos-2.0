/*
 * Linux device driver tunables for
 * Broadcom BCM44XX and BCM47XX 10/100Mbps Ethernet Device Driver
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 * $Id: et_ecos.h,v 1.1 Exp $
 */

#ifndef _et_ecos_h_
#define _et_ecos_h_

/* tunables */
#define	NTXD		64	/* # tx dma ring descriptors (must be ^2) */
#define	NRXD		512     /* # rx dma ring descriptors (must be ^2) changed from 512 */
#define	NRXBUFPOST	32		/* try to keep this # rbufs posted to the chip */
#define	BUFSZ		2048		/* packet data buffer size */
#define	RXBUFSZ		(2048 - 256)	/* receive buffer size */


#endif	/* _et_ecos_h_ */
