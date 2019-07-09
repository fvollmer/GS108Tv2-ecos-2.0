#ifndef _ecos_osl_h_
#define _ecos_osl_h_

#include <cyg/infra/cyg_type.h>
#include <cyg/hal/typedefs.h>
#include <cyg/hal/basetype.h>

#define	ASSERT(exp)             do {} while(0)

#define	OSL_UNCACHED(va)	(((uint32)(va) & 0x1fffffff) | 0xa0000000)
#define IS_KSEG2(_x)		(((_x) & 0xC0000000) == 0xC0000000)

#define	OSL_PCMCIA_READ_ATTR(osh, offset, buf, size) \
	osl_pcmcia_read_attr((osh), (offset), (buf), (size))
#define	OSL_PCMCIA_WRITE_ATTR(osh, offset, buf, size) \
	osl_pcmcia_write_attr((osh), (offset), (buf), (size))
extern void osl_pcmcia_read_attr(void *osh, uint offset, void *buf, int size);
extern void osl_pcmcia_write_attr(void *osh, uint offset, void *buf, int size);

/* PCI configuration space access macros */
#define	OSL_PCI_READ_CONFIG(loc, offset, size) \
	osl_pci_read_config((loc), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(loc, offset, size, val) \
	osl_pci_write_config((loc), (offset), (size), (val))
extern uint32 osl_pci_read_config(void *loc, uint size, uint offset);
extern void osl_pci_write_config(void *loc, uint offset, uint size, uint val);

/* OSL initialization */
#define osl_init()		do {} while (0)

/* Host/Bus architecture specific swap. Noop for little endian systems, possible swap on big endian */
#define BUS_SWAP32(v)	        (v)


#ifdef MALLOC
#undef MALLOC
#endif 
#define	MALLOC(size)		malloc(size)
#ifdef MFREE
#undef MFREE
#endif 
#define	MFREE(addr, size)	free(addr)

/* register access macros */
#define wreg32(r,v)	(*(volatile uint32 *)(r) = (v))
#define rreg32(r)	(*(volatile uint32 *)(r))

// Little-endian version
#if (CYG_BYTEORDER == CYG_LSBFIRST)

#define wreg16(r,v)	(*(volatile uint16 *)(r) = (v))
#define rreg16(r)	(*(volatile uint16 *)(r))
#define wreg8(r,v)	(*(volatile uint8 *)(r) = (v))
#define rreg8(r)	(*(volatile uint8 *)(r))

#else // Big-endian version

#define wreg8(r,v)	(*(volatile uint8 *)((uint32)r^3) = (v))
#define rreg8(r)	(*(volatile uint8 *)((uint32)r^3))
#define wreg16(r,v)	(*(volatile uint16 *)((uint32)r^2) = (v))
#define rreg16(r)	(*(volatile uint16 *)((uint32)r^2))

#endif

/* register access macros */
#define R_REG(r) ( \
	sizeof(*(r)) == sizeof(uint8) ? rreg8 ((volatile uint8*)(r)) : \
	sizeof(*(r)) == sizeof(uint16) ? rreg16 ((volatile uint16*)(r)) : \
	rreg32 ((volatile uint32*)(r)) \
)
#define W_REG(r, v) do { \
	switch (sizeof(*(r))) { \
	case sizeof(uint8):  wreg8((volatile uint8*)(r), (uint8)(v)); break;       \
	case sizeof(uint16): wreg16((volatile uint16*)(r), (uint16)(v)); break;     \
	case sizeof(uint32): wreg32((volatile uint32*)(r), (uint32)(v)); break;     \
	} \
} while (0)

#define	AND_REG(r, v)	W_REG((r), R_REG(r) & (v))
#define	OR_REG(r, v)	W_REG((r), R_REG(r) | (v))


/* bcopy, bcmp, and bzero */
#define	bcopy(src, dst, len)	bcopy((char*)(src), (char*)(dst), (len))
#define	bcmp(b1, b2, len)	bcmp((char*)(b1), (char*)(b2), (len))
#define	bzero(b, len)		bzero((char*)b, (len))

/* uncached virtual address */
#define	OSL_UNCACHED(va)	(((uint32)(va) & 0x1fffffff) | 0xa0000000)

/* dereference and address that may cause a bus exception */
#define BUSPROBE(x, y)		({ (x) = R_REG((y)); 0; })

/* map/unmap physical to virtual */
/* 	- assume a 1:1 mapping if KSEG2 addresses are used */
#define	REG_MAP(pa, size)	(IS_KSEG2(pa) ? (pa) : OSL_UNCACHED(pa))
#define	REG_UNMAP(va)		do {} while (0) /* nop */


#define	DMA_ALLOC_CONSISTENT(dev, size, pap)	osl_dma_alloc_consistent(dev, size, pap)
#define	DMA_FREE_CONSISTENT(dev, va, size, pa)	osl_dma_free_consistent(dev, size, (void*)va, pa)
extern void *osl_dma_alloc_consistent(void *dev, uint size, ulong *pap);
extern void osl_dma_free_consistent(void *dev, uint size, void *va, ulong pa);

/* map/unmap direction */
#define	DMA_TX	0
#define	DMA_RX	1

#define	DMA_MAP(dev, va, size, direction, p)	osl_dma_map(dev, (void*)va, size, direction)
#define	DMA_UNMAP(dev, pa, size, direction, p)	/* nop */
extern void *osl_dma_map(void *dev, void *va, uint size, uint direction);

/* microsecond delay */
#define	OSL_DELAY(us)	udelay(us)
extern void udelay(int delay);

/* shared memory access macros */
#define	R_SM(a)		*(a)
#define	W_SM(a, v)	(*(a) = (v))
#define	BZERO_SM(a, len)	bzero((char*)a, len)

/* packet structure */
struct sgpkt {	
	struct sgpkt	*next;		/* pointer to next sgpkt in a chain */
	struct sgpkt	*link;		/* pointer to next sgpkt in a list */
	uchar		*head;		/* start of buffer */
	uchar		*data;		/* start of data */
	uint		len;		/* nbytes of data */
	void		*cookie;	/* generic cookie */
};

/* packet primitives */
#define	PKTGET(drv, len, send)		osl_pktget(len)
#define	PKTFREE(drv, m, send)		osl_pktfree(m)
#define	PKTDATA(drv, m)                 ((struct sgpkt*)m)->data
#define	PKTLEN(drv, m)		        ((struct sgpkt*)m)->len
#define PKTHEADROOM(drv, m)		(0)
#define PKTTAILROOM(drv, m)		(0)
#define	PKTNEXT(drv, m)			((struct sgpkt*)m)->next
#define	PKTSETNEXT(m, n)		((struct sgpkt*)m)->next = ((struct sgpkt*)n)
#define	PKTSETLEN(drv, m, dlen)		((struct sgpkt*)m)->len = dlen
#define	PKTPUSH(drv, m, nbytes)		osl_pktpush(m, nbytes)
#define	PKTPULL(drv, m, nbytes)		osl_pktpull(m, nbytes)
#define	PKTDUP(drv, m)			osl_pktdup(m)
#define	PKTCOOKIE(m)			((struct sgpkt*)m)->cookie
#define	PKTSETCOOKIE(m, x)		((struct sgpkt*)m)->cookie = (void *)x
#define	PKTLINK(m)			((struct sgpkt*)m)->link
#define	PKTSETLINK(m, x)		((struct sgpkt*)m)->link = ((struct sgpkt*)x)
#define PKTPRIO(x)               0
#define PKTSETPRIO(m, x)        do {} while(0)

extern void *osl_pktget(uint len);
extern void osl_pktfree(void* m);
extern uchar *osl_pktpush(void* m, int bytes);
extern uchar *osl_pktpull(void* m, int bytes);
extern void *osl_pktdup(void* m);
#endif	/* _ecos_osl_h_ */
