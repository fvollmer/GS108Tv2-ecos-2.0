/*
 * eCos OS Independent Layer
 *
 */

#define ECOS_OSL

#include <cyg/hal/typedefs.h>
#include <cyg/hal/bcmendian.h>
#include <cyg/hal/ecos_osl.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/pcicfg.h>

#ifdef CYGPKG_IO_PCI
#include <pkgconf/io_pci.h>
#include <cyg/io/pci_hw.h>
#include <cyg/io/pci.h>
#endif

#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_arch.h>

#define PCI_CFG_RETRY 10	

#define BUF_SIZE    (1024*4)

#undef bzero
void
bzero(void *b, int len)
{
	memset(b, '\0', len);
}

void*
osl_pktget(uint len)
{
	uchar *buf;
	struct sgpkt *pkt;
	
	if (len > (BUF_SIZE - sizeof(struct sgpkt)))
		return (NULL);
	
	if ((buf = (uchar*)malloc(BUF_SIZE)) == NULL)
		return (NULL);

	pkt = (struct sgpkt*)&buf[BUF_SIZE - sizeof(struct sgpkt)];
    
	bzero(pkt, sizeof(struct sgpkt));
	pkt->head = pkt->data = buf;
	pkt->len = len;
	
	return ((void*)pkt);
}

void
osl_pktfree(void *p)
{

	struct sgpkt *next;

	for (; (struct sgpkt*)p; (struct sgpkt*)p = next)
	{
		next = ((struct sgpkt*)p)->next;
		free (CYGARC_CACHED_ADDRESS(CYGARC_PHYSICAL_ADDRESS(((struct sgpkt*)p)->head)));
	}
}

void *
osl_pktdup(void *m)
{
	struct sgpkt *dup;

	if (!(dup = (struct sgpkt *)osl_pktget(((struct sgpkt*)m)->len)))
		return NULL;

	bcopy(((struct sgpkt *)m)->head, dup->head, ((struct sgpkt *)m)->len);

	return ((void*)dup);
}

uchar *
osl_pktpush(void *m, int bytes)
{
        ASSERT((((struct sgpkt*)m)->data - bytes) >= ((struct sgpkt*)m)->head);

	((struct sgpkt*)m)->data -= bytes;
	((struct sgpkt*)m)->len += bytes;

	return ((struct sgpkt*)m)->data;
}

uchar *
osl_pktpull(void *m, int bytes)
{
        ASSERT((((struct sgpkt*)m)->data + bytes) <= ((struct sgpkt*)m)->end);
        ASSERT(((struct sgpkt*)m)->len >= bytes);

        ((struct sgpkt*)m)->data += bytes;
        ((struct sgpkt*)m)->len -= bytes;

       return ((struct sgpkt*)m)->data;
}
#ifdef CYGPKG_IO_PCI    
uint32
osl_pci_read_config(void *loc, uint offset, uint size)
{
	uint devid;
	uint16 devfn;
	uint val = 0;
	uint retry=PCI_CFG_RETRY;	 

	devid = ((cyg_pci_device *)loc)->devid;

	devfn = CYG_PCI_DEV_GET_DEVFN(devid);
        
	/* only 4byte access supported */
	ASSERT(size == 4);

	do {
        cyg_pcihw_read_config_uint32(CYG_PCI_DEV_GET_BUS(devid),
                                     devfn, offset, &val);
		if (val != 0xffffffff)
			break;
	} while (retry--);

	return (val);
	
}

void
osl_pci_write_config(void *loc, uint offset, uint size, uint val)
{

	uint devid;
	uint16 devfn;
	uint retry=PCI_CFG_RETRY;	 

	/* only 4byte access supported */
	ASSERT(size == 4);

	devid = ((cyg_pci_device *)loc)->devid;
	
	devfn = CYG_PCI_DEV_GET_DEVFN(devid);
        

	do {
		cyg_pcihw_write_config_uint32(CYG_PCI_DEV_GET_BUS(devid),
                                              devfn, offset, val);
		if (offset!=PCI_BAR0_WIN)
			break;
		if (osl_pci_read_config(loc,offset,size) == val) 
			break;
	} while (retry--);

}
#else
uint32
osl_pci_read_config(void *loc, uint offset, uint size)
{
    return (0xffffffff);
}

void
osl_pci_write_config(void *loc, uint offset, uint size, uint val)
{
}
#endif

static void
osl_pcmcia_attr(void *osh, uint offset, char *buf, int size, bool write)
{
}

void
osl_pcmcia_read_attr(void *osh, uint offset, void *buf, int size)
{
	osl_pcmcia_attr(osh, offset, (char *) buf, size, FALSE);
}

void
osl_pcmcia_write_attr(void *osh, uint offset, void *buf, int size)
{
	osl_pcmcia_attr(osh, offset, (char *) buf, size, TRUE);
}

void*
osl_dma_alloc_consistent(void *dev, uint size, ulong *pap)
{
	void *va;
	
	if ((va = malloc(size)) == NULL)
		return (NULL);
	*pap = (ulong)CYGARC_PHYSICAL_ADDRESS(va);
	va = CYGARC_UNCACHED_ADDRESS(va);
	return (va);
}

void
osl_dma_free_consistent(void *dev, uint size, void *va, ulong pa)
{
	va = ((void*)(CYGARC_CACHED_ADDRESS(CYGARC_PHYSICAL_ADDRESS(va))));
	free(va);
}

void*
osl_dma_map(void *dev, void *va, uint size, uint direction)
{
#if 1	
	if (direction == DMA_TX)
		HAL_DCACHE_FLUSH( va, size);
	else
		HAL_DCACHE_INVALIDATE(va, size);
#endif
	return ((void*)CYGARC_PHYSICAL_ADDRESS(va));
}
