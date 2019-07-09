/*
 * HND SiliconBackplane MIPS/ARM cores software interface.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: hndcpu.h,v 1.1 2009/01/05 07:19:05 cchao Exp $
 */

#ifndef _hndcpu_h_
#define _hndcpu_h_

#if defined(mips)
#include "hndmips.h"
#elif defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
#include <hndarm.h>
#endif

extern uint si_irq(si_t *sih);
extern uint32 si_cpu_clock(si_t *sih);
extern uint32 si_mem_clock(si_t *sih);
extern void hnd_cpu_wait(si_t *sih);
extern void hnd_cpu_jumpto(void *addr);
extern void hnd_cpu_reset(si_t *sih);

#endif /* _hndcpu_h_ */
