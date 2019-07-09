/*
 * eCos 2.0 OS Independent Layer
 *
 * $Copyright Open Broadcom Corporation$
 * $Id: ecos_osl.h,v 1.4 2009/07/07 13:23:26 jimhuang Exp $
 */
#ifndef __ECOS_OSL_H__
#define __ECOS_OSL_H__

#include "typedefs.h"
#include <sys/param.h>

#define ASSERT(exp)             do {} while (0)

#define OSL_PCMCIA_READ_ATTR(osh, offset, buf, size)    bzero(buf, size)
#define OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size)   do {} while (0)

/* PCI configuration space access macros */
#define OSL_PCI_READ_CONFIG(osh, offset, size)  \
    osl_pci_read_config((osh), (offset), (size))
#define OSL_PCI_WRITE_CONFIG(osh, offset, size, val)    \
    osl_pci_write_config((osh), (offset), (size), (val))
extern uint32 osl_pci_read_config(osl_t *osh, uint offset, uint size);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

/* PCI device bus # and slot # */
#define OSL_PCI_BUS(osh)    (0)
#define OSL_PCI_SLOT(osh)   (0)

/* Host/Bus architecture specific swap. Noop for little endian systems, 
 *  possible swap on big endian
*/

#define BUS_SWAP32(v)           (v)


#undef free
#undef malloc

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef MALLOC
#undef MALLOC
#endif /* MALLOC */
#define MALLOC(osh, size)   malloc(size)
#ifdef MFREE
#undef MFREE
#endif /* MFREE */
#define MFREE(osh, addr, size)  free(addr)
#define MALLOCED(osh)       (0)
#define MALLOC_FAILED(osh)  (0)
#define MALLOC_DUMP(osh, b)

#ifdef WL_NVRAM
extern char *CFG_wlget(char *name);
extern int CFG_wlset(char *name, char *value);

#ifdef nvram_safe_get
#undef nvram_safe_get
#endif /* nvram_safe_get */
#define nvram_safe_get CFG_wlget

#ifdef nvram_get
#undef nvram_get
#endif /* nvram_get */
#define nvram_get CFG_wlget

#ifdef nvram_set
#undef nvram_set
#endif /* nvram_set */
#define nvram_set CFG_wlset
#endif /* WL_NVRAM */

/* register access macros */
#define wreg32(r, v)    (*(volatile uint32 *)(r) = (v))
#define rreg32(r)   (*(volatile uint32 *)(r))
#define wreg16(r, v)    (*(volatile uint16 *)(r) = (v))
#define rreg16(r)   (*(volatile uint16 *)(r))
#define wreg8(r, v) (*(volatile uint8 *)(r) = (v))
#define rreg8(r)    (*(volatile uint8 *)(r))

/* register access macros */
#define R_REG(osh, r) (\
    sizeof(*(r)) == sizeof(uint8) ? rreg8((volatile uint8*)(r)) : \
    sizeof(*(r)) == sizeof(uint16) ? rreg16((volatile uint16*)(r)) : \
    rreg32((volatile uint32*)(r)) \
)
#define W_REG(osh, r, v) do { \
    switch (sizeof(*(r))) { \
    case sizeof(uint8):  wreg8((volatile uint8*)(r), (uint8)(v)); break;       \
    case sizeof(uint16): wreg16((volatile uint16*)(r), (uint16)(v)); break;     \
    case sizeof(uint32): wreg32((volatile uint32*)(r), (uint32)(v)); break;     \
    } \
} while (0)

#define AND_REG(osh, r, v)  W_REG(osh, (r), R_REG(osh, r) & (v))
#define OR_REG(osh, r, v)   W_REG(osh, (r), R_REG(osh, r) | (v))


/* bcopy, bcmp, and bzero */
#define bcopy(src, dst, len)    bcopy((char*)(src), (char*)(dst), (len))
#define bcmp(b1, b2, len)   bcmp((char*)(b1), (char*)(b2), (len))
#define bzero(b, len)       bzero((char*)b, (len))

/* uncached virtual address */
#ifdef __mips__
#define OSL_UNCACHED(va)        (((uint32)(va)) | 0xa0000000)
#define IS_KSEG2(_x)            (((_x) & 0xC0000000) == 0xC0000000)
#else
#define IS_KSEG2(_x)            0
#define OSL_UNCACHED(va)        (va)
#endif /* __mips__ */

/* dereference and address that may cause a bus exception */
extern int bus_probe(void *reg, void *val, int len);
#define BUSPROBE(x, y) bus_probe((void *)(y), (void *)&(x), sizeof(*(y)))

/* map/unmap physical to virtual */
/*  - assume a 1:1 mapping if KSEG2 addresses are used */
#define REG_MAP(pa, size)   ((void *)(IS_KSEG2(pa) ? (pa) : OSL_UNCACHED(pa)))
#define REG_UNMAP(va)       do {} while (0) /* nop */


#define DMA_CONSISTENT_ALIGN    sizeof(int)

#define DMA_ALLOC_CONSISTENT(osh, size, pap, dmah) osl_dma_alloc_consistent(osh, size, pap)
#define DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
    osl_dma_free_consistent(osh, size, (void*)va, pa)
extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, ulong *pap);
extern void osl_dma_free_consistent(osl_t *osh, uint size, void *va, ulong pa);

#define DMA_TX          0       /* TX direction */
#define DMA_RX          1       /* RX direction */
#define DMA_MAP(osh, va, size, direction, p, dmah) \
    osl_dma_map(osh, (void*)va, size, direction)
#define DMA_UNMAP(osh, pa, size, direction, p, dmah)    /* nop */
extern void *osl_dma_map(osl_t *osh, void *va, uint size, uint direction);

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth) do {} while (0)

/* microsecond delay */
#define OSL_DELAY(us)   udelay(us)
extern void udelay(int delay);

/* shared memory access macros */
#define R_SM(a)     *(a)
#define W_SM(a, v)  (*(a) = (v))
#define BZERO_SM(a, len)    bzero((char*)a, len)

void*   osl_attach(void *pdev, uint bustype, bool pkttag);
void    osl_detach(osl_t *osh);


/* packet structure */
struct sgpkt {  
    struct sgpkt    *next;      /* pointer to next sgpkt in a chain */
    struct sgpkt    *link;      /* pointer to next sgpkt in a list */
    uchar       *head;      /* start of buffer */
    uchar       *data;      /* start of data */
    uint        len;        /* nbytes of data */
    void        *cookie;    /* generic cookie */
};

/* Memory types for packet primitives */
#define MEMORY_DDRRAM 0
#define MEMORY_SOCRAM 1
#define MEMORY_PCIMEM 2 

/* packet primitives */
#define PKTGET(drv, len, send, mt)      osl_pktget(len)
#define PKTFREE(drv, m, send, mt)       osl_pktfree(m)
#define PKTDATA(drv, m)                 ((struct sgpkt*)m)->data
#define PKTLEN(drv, m)              ((struct sgpkt*)m)->len
#define PKTHEADROOM(drv, m)     (0)
#define PKTTAILROOM(drv, m)     (0)
#define PKTNEXT(drv, m)         ((struct sgpkt*)m)->next
#define PKTSETNEXT(m, n)        ((struct sgpkt*)m)->next = ((struct sgpkt*)n)
#define PKTSETLEN(drv, m, dlen)     ((struct sgpkt*)m)->len = dlen
#define PKTPUSH(drv, m, nbytes)     osl_pktpush(m, nbytes)
#define PKTPULL(drv, m, nbytes)     osl_pktpull(m, nbytes)
#define PKTDUP(drv, m)          osl_pktdup(m)
#define PKTCOOKIE(m)            ((struct sgpkt*)m)->cookie
#define PKTSETCOOKIE(m, x)      ((struct sgpkt*)m)->cookie = (void *)x
#define PKTLINK(m)              ((struct sgpkt*)m)->link
#define PKTSETLINK(m, x)        ((struct sgpkt*)m)->link = ((struct sgpkt*)x)
#define PKTPRIO(x)              (0)
#define PKTSETPRIO(m, p)        do { } while(0)
#define PKTFLAGTS(osh, lb)      (0)

extern void *osl_pktget(uint len);
extern void osl_pktfree(void* m);
extern uchar *osl_pktpush(void* m, int bytes);
extern uchar *osl_pktpull(void* m, int bytes);
extern void *osl_pktdup(void* m);

/* get system up time in miliseconds */
#define OSL_SYSUPTIME()         (0)

#endif /* __ECOS_OSL_H__ */
