/*
 * OS Independent Layer
 * 
 * Copyright 2004, Broadcom Corporation      
 * All Rights Reserved.      
 *       
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY      
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM      
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS      
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.      
 * $Id: osl.h,v 1.1.2.1 Exp $
 */

#ifndef _osl_h_
#define _osl_h_

#include <cyg/hal/ecos_osl.h>

/* handy */
#define	SET_REG(r, mask, val)	W_REG((r), ((R_REG(r) & ~(mask)) | (val)))

#endif	/* _osl_h_ */
