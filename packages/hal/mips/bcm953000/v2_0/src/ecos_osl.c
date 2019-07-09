/*
 * eCos OS Independent Layer
 *
 * $Copyright Open Broadcom Corporation$
 * $Id: ecos_osl.c,v 1.3 2009/07/07 13:23:26 jimhuang Exp $
 */

#include <pkgconf/system.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>
#include <cyg/io/pci.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_arch.h>

#include <cyg/hal/typedefs.h>
#include <cyg/hal/bcmendian.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/ecos_osl.h>
#include <cyg/hal/pcicfg.h>
#include <cyg/hal/bcmallocache.h>

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_arch.h>

#define PCI_CFG_RETRY 10

#define BUF_SIZE    (1024*4)

static int data_bus_error = 0;

struct osl_info {
	void 		*devinfo;
	bcmcache_t	*pkttag_pool;	/* Working set of allocated memory pool */
	uint32		alloced;		/* Total allocated pkttags */
	void 		*tx_ctx;
	pktfree_cb_fn_t tx_fn;
};

void*
osl_attach(void *pdev, uint bustype, bool pkttag)
{
	static bool init_done = FALSE;
	osl_t *osh;

	osh = (osl_t *)malloc(sizeof(osl_t));

	if (osh) {
		bzero(osh, sizeof(osl_t));
		osh->devinfo = pdev;
		if (pkttag) {
			osh->pkttag_pool = bcmcache_create(osh, "pkttag", OSL_PKTTAG_SZ);
		}
	}
	return (void*)osh;
}

void
osl_detach(osl_t *osh)
{
	if (osh == NULL)
		return;

	/* Destroy working set cache if exists */
	if (osh->pkttag_pool) {
#ifdef BCMDBG
		if (osh->alloced != 0)
			printf("Pkttag leak of %d pkttags. Packets leaked\n", osh->alloced);
#endif /* BCMDBG */
		ASSERT(osh->alloced == 0);
		bcmcache_destroy(osh->pkttag_pool);
	}

	free(osh);
}

uint32
osl_pci_read_config(osl_t *osh, uint offset, uint size)
{
	uint devid;
	uint16 devfn;
	uint val = 0;
	uint retry = PCI_CFG_RETRY;
	devid = ((cyg_pci_device *)osh->devinfo)->devid;

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
osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val)
{

	uint devid;
	uint16 devfn;
	uint retry = PCI_CFG_RETRY;

	/* only 4byte access supported */
	ASSERT(size == 4);

	devid = ((cyg_pci_device *)osh->devinfo)->devid;
	devfn = CYG_PCI_DEV_GET_DEVFN(devid);
	do {
		cyg_pcihw_write_config_uint32(CYG_PCI_DEV_GET_BUS(devid), devfn, offset, val);
		if (offset != PCI_BAR0_WIN)
			break;
		if (osl_pci_read_config(osh, offset, size) == val)
			break;
	} while (retry--);

}

void
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
osl_dma_alloc_consistent(osl_t *osh, uint size, ulong *pap)
{
	void *va;
	if ((va = malloc(size)) == NULL)
		return (NULL);

	HAL_DCACHE_FLUSH(va, size);
	*pap = (ulong)CYGARC_PHYSICAL_ADDRESS(va);
	va = (void *)CYGARC_UNCACHED_ADDRESS(va);
	return (va);
}

void
osl_dma_free_consistent(osl_t *osh, uint size, void *va, ulong pa)
{
	va = ((void*)(CYGARC_CACHED_ADDRESS(CYGARC_PHYSICAL_ADDRESS(va))));
	free(va);
}

void*
osl_dma_map(osl_t *osh, void *va, uint size, uint direction)
{
	if (direction == DMA_TX)
		HAL_DCACHE_STORE((CYG_ADDRESS)va, size);	/* write back */
	else
		HAL_DCACHE_FLUSH(va, size);	/* write back and invalidate */

	return ((void*)CYGARC_PHYSICAL_ADDRESS(va));
}

void
osl_assert(char *exp, char *file, int line)
{
	char tempbuf[255];

	sprintf(tempbuf, "\nassertion \"%s\" failed: file \"%s\", line %d\n", exp, file, line);
	printf("ASSERT: %s\n", tempbuf);
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
        free ((void *)CYGARC_CACHED_ADDRESS(CYGARC_PHYSICAL_ADDRESS(((struct sgpkt*)p)->head)));
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

static void
dbe_exception_handler(
    cyg_addrword_t data,
    cyg_code_t number,
    cyg_addrword_t info)
{ 
    HAL_SavedRegisters *savedreg;

    savedreg = (HAL_SavedRegisters *)info;
    
    /* Wherever it stops, we let it continue */
    savedreg->pc += 4;
    
    data_bus_error = 1;
} 

int
bus_probe(void *reg, void *val, int len)
{
    cyg_exception_handler_t *old_handler;
    cyg_addrword_t old_data;
    uint32 *r32;
    uint16 *r16;
    uint8 *r8;

    /* XXX: Disable non-32bit (len != 4) access
     *      since we encountered a strange crashing problem with -O2.
     *      Will try to fix it later.
     */
    if (val == NULL || reg == NULL || len != 4) {
        return -1;
    }

    switch(len) {
        case 1: r8 = (uint8 *)reg; 
            break;
        case 2: r16 = (uint16 *)reg;
            break;
        case 4: r32 = (uint32 *)reg;
            break;
        default:
           return -1;
    }

    /* XXX: critical section */
    data_bus_error = 0;

    /* Install the exception handler to capture and ignore data bus error */
    cyg_exception_set_handler(CYGNUM_HAL_VECTOR_DBE,
                           &dbe_exception_handler,
                           0,
                           &old_handler,
                           &old_data);

#if 1 /* XXX: Only 32bit access allowed */
    /* This could fire DBE exception */
    *(uint32 *)val = R_REG(NULL, r32);
#else 
    /* This could fire DBE exception */
    switch(len) {
        case 1: 
            *(uint8 *)val = R_REG(NULL, r8); break;
        case 2: 
            *(uint16 *)val = R_REG(NULL, r16); break;
        case 4: 
            *(uint32 *)val = R_REG(NULL, r32); break;
    }
#endif
                           
    /* Restore exception handler */
    cyg_exception_set_handler(CYGNUM_HAL_VECTOR_DBE,
                           old_handler,
                           old_data,
                           NULL,
                           NULL);
             
    /* Check if DBE exception fired */
    if (data_bus_error) {
        return -1;
    }
    
    return 0;
}
