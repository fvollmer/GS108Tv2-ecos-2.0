/*
 * Minimal debug/trace/assert driver definitions for
 * Broadcom Home Networking Division 10/100 Mbit/s Ethernet
 * Device Driver.
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 * $Id: et_dbg.h,v 1.1 Exp $
 */

#ifndef _et_dbg_
#define _et_dbg_

#define ecos_build 1

#define	ET_ERROR(args)		
#define	ET_TRACE(args)      
#define	ET_PRHDR(msg, eh, len, unit)
#define	ET_PRPKT(msg, buf, len, unit)
#define EB_TRACE(args)


extern int et_msg_level;

#define	ET_LOG(fmt, a1, a2)

/* include port-specific tunables */
#ifdef NDIS
#include <et_ndis.h>
#elif linux
#include <et_linux.h>
#elif PMON
#include <et_pmon.h>
#elif _CFE_
#include <et_cfe.h>
#elif ecos_build
#include "et_ecos.h"
#else
#error
#endif

#endif /* _et_dbg_ */
