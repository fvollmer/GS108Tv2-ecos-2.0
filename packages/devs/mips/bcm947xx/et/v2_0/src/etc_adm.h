/*
 * ADMtek switch setup functions
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 *
 * $Id: etc_adm.h,v 1.1 Exp $
 */

#ifndef _adm_h_
#define _adm_h_

extern void * adm_attach(void *sbh, void *vars);
extern void adm_detach(void *adm);
extern void adm_enable_device(void *adm, void *vars);
extern int adm_config_vlan(void *adm, void *vars);

#endif /* _adm_h_ */

