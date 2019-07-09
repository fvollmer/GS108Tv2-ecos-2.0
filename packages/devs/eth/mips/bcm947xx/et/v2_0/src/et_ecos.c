//==========================================================================
// 
//      et_ecos.c
// 
//      Broadcom  ethernet driver
// 
//==========================================================================

//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    
// Contributors: 
// Date:         
// Purpose:
// Description:
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>
#ifdef CYGPKG_IO_ETH_DRIVERS
#include <pkgconf/io_eth_drivers.h>
#endif
#include <pkgconf/devs_eth_bcm947xx_et.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_if.h>
#include <cyg/hal/drv_api.h>
#include <cyg/io/eth/netdev.h>
#include <cyg/io/eth/eth_drv.h>

#ifdef CYGPKG_NET
#include <pkgconf/net.h>
#include <net/if.h>  /* Needed for struct ifnet */
#endif

#ifdef CYGPKG_IO_PCI
#include <cyg/io/pci.h>
#else
#error "Need PCI package here"
#endif

#include <cyg/kernel/kapi.h>

#include <cyg/hal/osl.h>
#include <cyg/hal/bcmendian.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/proto/ethernet.h>
#include <cyg/hal/bcmenetmib.h>
#include <cyg/hal/bcmenetrxh.h>
#include "et_dbg.h"
#include "etc.h"

#define DEBUG
#define MAX_TX_DESCRIPTORS 		128
#define dprintf		diag_printf		  
#define DATAHIWAT				50
#define ETH_STATS_INIT( p )

/* sgpkt queue */
struct sgpktq {
	struct sgpkt*  head;
	struct sgpkt*  tail;
	uint    len;
};
	

typedef struct et_info {
	etc_info_t	    *etc;		/* pointer to common os-independent data */
	void            *ndp;	    /* backpoint to network device tab */
	cyg_uint32      devid;		/* pci device id */
	cyg_pci_device  *pdev;      /* pointer to pci device structure */
	cyg_uint32 	    base;       /* PCI memory map base */
	int             index;
	int             active;
	struct sgpktq   txq;
	unsigned long   tx_keys[MAX_TX_DESCRIPTORS];
	                             /*keys for tx q management	*/
	struct sgpkt    *rxpkt;      /* pointer to current received packet */
	uint            tx_desc_add;
	uint            tx_desc_remove;
	int             events;    /* events received */
	cyg_vector_t		vector;     /* interrupt vector */
	cyg_handle_t  	interrupt_handle;
	cyg_interrupt	interrupt_object;
	ulong           flags;			/* current irq flags */
	cyg_handle_t    alarm_hdl;     /* handle to ecos alarm */
	cyg_alarm       alarm_obj;
	cyg_handle_t    counter_hdl;
	struct et_info  *next;		/* pointer to next et_info_t in chain */
} et_info_t;



#ifdef CYGPKG_DEVS_ETH_ET0

et_info_t et0_priv_data;

ETH_DRV_SC(et_sc0,
           &et0_priv_data,    // Driver specific data
           "eth0",             // Name for this interface
           et_start,
           et_stop,
           et_ioctl,
           et_can_send,
           et_send,
           et_recv,
           et_deliver,       // "pseudoDSR" called from fast net thread
           et_poll,
           et_int_vector);

NETDEVTAB_ENTRY(et_netdev0, 
                "dev_et0", 
                et_drv_init, 
                &et_sc0);

#endif

static et_info_t* et_info_array[CYGNUM_DEVS_ETH_ET_DEV_COUNT] = {
#ifdef CYGPKG_DEVS_ETH_ET0
	&et0_priv_data,
#endif
}; 

static et_info_t *et_list = NULL;

/* static functions */
static int et_isr(cyg_vector_t vector, cyg_addrword_t data);
static void et_dsr(cyg_vector_t vector, cyg_ucount32 count,
				   cyg_addrword_t data);
static void et_sendup(et_info_t *et, struct sgpkt *sg);
static void et_enq(struct sgpktq *sgq, struct sgpkt *sg);
static struct sgpkt* et_deq(struct sgpktq *sgq);
static void et_txdone(et_info_t *et);
static void et_sendnext(et_info_t *et);
static void et_init_alarm(et_info_t *et);

void et_reset(et_info_t *et);

	
static bool et_find_match (cyg_uint16 v, cyg_uint16 d, cyg_uint32 c, void *p)
{
    if  (!etc_chipmatch ((cyg_uint32)v,  (cyg_uint32)d))
        return (FALSE);
    else
        return (TRUE);
}

static void
et_init_drv_priv(et_info_t *et)
{
    int i;

    et->tx_desc_add = 0;
    et->tx_desc_remove = 0;
        
    for (i=0; i < MAX_TX_DESCRIPTORS; i++)
        et->tx_keys[i] = 0;	
}

extern int nvram_init_ecos(void*);

static bool
et_drv_init(struct cyg_netdevtab_entry * ndp)
{
#if 0 /* alok */
    static int initialized = 0;
    cyg_pci_device_id devid;
    cyg_pci_device *dev_info;
    struct eth_drv_sc *sc;
    et_info_t *et;
    int found_devices = 0;
    int device_index;
	
    dprintf("et_init: device initialization start\n");
	
    /* device instance */
    sc = (struct eth_drv_sc *)ndp->device_instance;
    /* driver private structure */
    et = (et_info_t *) sc->driver_private;

    memset (et, '\0', sizeof(et_info_t));

    et->ndp = ndp;
	
    if (initialized == 0) {

    nvram_init_ecos(0);
    cyg_pci_init();

    devid = CYG_PCI_NULL_DEVID;
		
    for (device_index = 0; device_index < CYGNUM_DEVS_ETH_ET_DEV_COUNT;
         device_index++) {
        
        et_info_t *et_info = et_info_array[device_index];
        et_info->index = device_index;

        if (cyg_pci_find_matching (&et_find_match, NULL, &devid)) {
            dprintf("et_init: found device et%d\n", device_index);
				
            et_info->pdev = (cyg_pci_device*)malloc(sizeof(cyg_pci_device));
            {
            cyg_uint8 bus = CYG_PCI_DEV_GET_BUS(devid);
            cyg_uint8 devfn = CYG_PCI_DEV_GET_DEVFN(devid);
            cyg_uint16 cmd_val = CYG_PCI_CFG_COMMAND_MEMORY;
				
            cyg_pcihw_write_config_uint16(bus, devfn, 
            CYG_PCI_CFG_COMMAND, cmd_val);
            }
            cyg_pci_get_device_info(devid, et_info->pdev);
				
            et_info->interrupt_handle  = 0;

            if (cyg_pci_translate_interrupt(et_info->pdev, &et_info->vector))
            {
                dprintf("Wired to HAL vector %d\n", et_info->vector); 
                cyg_drv_interrupt_create(
                    et_info->vector,
                    0,
                    (CYG_ADDRWORD)et_info,
                    et_isr,
                    et_dsr,
                    &et_info->interrupt_handle,
                    &et_info->interrupt_object);
                cyg_drv_interrupt_attach(et_info->interrupt_handle);
            } else {
                et_info->vector = 0;
                dprintf("et_init: no interrupt generations\n");
            }
				
            if (cyg_pci_configure_device(et_info->pdev)) {
                dev_info = et_info->pdev;
#ifdef DEBUG
                int i;
                diag_printf("Found device on bus %d, devfn 0x%02x:\n",
                CYG_PCI_DEV_GET_BUS(devid),
                CYG_PCI_DEV_GET_DEVFN(devid));
					
                if (dev_info->command & CYG_PCI_CFG_COMMAND_ACTIVE) {
                    diag_printf(" Note that board is active. Probed"
                    " sizes and CPU addresses invalid!\n");
                }
                diag_printf(" Vendor    0x%04x", dev_info->vendor);
                diag_printf("\n Device    0x%04x", dev_info->device);
                diag_printf("\n Command   0x%04x, Status 0x%04x\n",
                            dev_info->command, dev_info->status);
					
                diag_printf(" Class/Rev 0x%08x", dev_info->class_rev);
                diag_printf("\n Header 0x%02x\n", dev_info->header_type);
					
                diag_printf(" SubVendor 0x%04x, Sub ID 0x%04x\n",
                             dev_info->header.normal.sub_vendor, 
                             dev_info->header.normal.sub_id);

                for(i = 0; i < CYG_PCI_MAX_BAR; i++) {
                    diag_printf(" BAR[%d]    0x%08x /", 
                                i, dev_info->base_address[i]);
                    diag_printf(" probed size 0x%08x / CPU addr 0x%08x\n",
                          dev_info->base_size[i], dev_info->base_map[i]);
                }
                diag_printf(" eth%d configured\n", device_index);
#endif
                found_devices++;
                et_info->active = 0;
                et_info->devid = devid;
                et_info->base = dev_info->base_map[0];
#ifdef DEBUG
                diag_printf(" memory address = 0x%08x\n",
                              dev_info->base_map[0]);
                diag_printf(" I/O address = 0x%08x\n",
                              dev_info->base_map[1]);
#endif
                if ((et_info->etc = etc_attach((void*)et_info,
                      dev_info->vendor,
                      dev_info->device,
                      device_index, dev_info,
                      et_info->base)) == NULL) {
                    diag_printf("et_init: etc_attach() %d failed\n",
                                 device_index);
                    free (et_info->pdev);
                    return 0;
                }
                dprintf("et_init: etc_attch passed for device %d\n",
                         device_index);
                if (et_info->interrupt_handle != 0)
                    cyg_drv_interrupt_acknowledge(et_info->vector);
					
            } else {
                et_info->active = 0;
                dprintf("et_init: failed to initize device %d\n",
                         device_index);
            }
        } else {
            et_info->active = 0;
            dprintf("et_init: failed to find device %d\n", device_index);
        }
    } /* for */

    if (found_devices == 0)
        return 0;
    }/* if (initialized == 0) */

    cyg_drv_interrupt_unmask(et->vector);

    et->next = et_list;
    et_list = et;
	
    et_init_drv_priv(et);
    et_init_alarm(et);
    /* Initialize upper level driver*/
    (sc->funs->eth_drv->init)(sc, &et->etc->cur_etheraddr);

    bcopy(&et->etc->cur_etheraddr, &sc->sc_arpcom.ac_enaddr, ETHER_ADDR_LEN);

    dprintf("et_init: device initialization end\n");
#endif	
    return 1;	
}

void
et_up(et_info_t *et)
{
    dprintf("et_up:  begin\n");
    etc_up(et->etc);
}


void
et_init(et_info_t *et)
{
    dprintf("et%d: et_init\n", et->etc->unit);
    et_reset(et);
    etc_init(et->etc);
}


void
et_reset(et_info_t *et)
{
    dprintf("et%d: et_reset\n", et->etc->unit);
    etc_reset(et->etc);
    et->events = 0;
}

static void
et_start(struct eth_drv_sc *sc, unsigned char *enaddr, int flags)
{
    et_info_t *et;

#ifdef CYGPKG_NET
    struct ifnet *ifp = &sc->sc_arpcom.ac_if;
#endif

    dprintf("et_start: device start begin\n");

    et = sc->driver_private;
#if 0 /* TEST */	
#ifdef CYGPKG_NET
    et->etc->promisc = (ifp->if_flags & IFF_PROMISC) ? TRUE:FALSE;
#endif
#ifdef ETH_DRV_FLAGS_PROMISC_MODE
    et->etc->promisc = (flags & ETH_DRV_FLAGS_PROMISC_MODE) ? TRUE:FLASE;
#endif
#endif				
    et_up(et);
	
    /*cyg_alarm_initialize (et->alarm_hdl, (cyg_current_time()+300), 300);*/
    dprintf("et_start: device start end\n");
}

void
et_down(et_info_t *et, int reset)
{
    etc_info_t *etc;

    dprintf("et_down: begin\n");
	
    etc = et->etc;
    etc_down(etc, reset);
	
    dprintf("et_down: end\n");
}

static void
et_stop(struct eth_drv_sc *sc)
{
    et_info_t *et;
	
    dprintf("et_stop: begin\n");

    et = sc->driver_private;	

    /*	cyg_alarm_disable (et->alarm_hdl);*/
    et_down(et, 1);
	
    dprintf("et_stop: end\n");
}	
	

static int
et_set_mac_address(et_info_t *et, char *addr)

{
	   dprintf("et_set_mac_address: begin\n");

    if (et->etc->up)
        return 1;

    bcopy(addr, &et->etc->cur_etheraddr, ETHER_ADDR_LEN);

    dprintf("et_set_mac_address: end\n");

    return 0;
}

#ifdef ETH_DRV_GET_MAC_ADDRESS
static int
et_get_mac_address(et_info_t *et, char *addr)
{
    dprintf("et_get_mac_address: begin\n");

    memcpy( addr, (char *)(et->etc->cur_etheraddr), ETHER_ADDR_LEN);

    dprintf("et_get_mac_address: end\n");

    return 0;
}
#endif


static void
et_set_multicast_list(et_info_t *et, struct eth_drv_mc_list *mcl)
{
    int i;
    etc_info_t *etc = et->etc;
	
    if (etc->up) {
        for (i = 0; i < mcl->len; i++)
            etc->multicast[i] = *((struct ether_addr*) (mcl->addrs[i]));
    }
    etc->nmulticast = i;

    et_init(et);
}

static int
et_ioctl(struct eth_drv_sc *sc, unsigned long key,
             void *data, int data_length)
{
    et_info_t *et;
    int err=0;

    dprintf("et_ioctl: begin\n");

    et = (et_info_t *) sc->driver_private;
    struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	
    switch ( key ) {

#ifdef ETH_DRV_SET_MAC_ADDRESS
    case ETH_DRV_SET_MAC_ADDRESS:
        if ( 6 != data_length )
            return -2;
        return et_set_mac_address( et, data );
#endif

#ifdef ETH_DRV_GET_MAC_ADDRESS
    case ETH_DRV_GET_MAC_ADDRESS:
        return et_get_mac_address( et, data );
#endif

#ifdef ETH_DRV_GET_IF_STATS_UD
    case ETH_DRV_GET_IF_STATS_UD: // UD == UPDATE
        ETH_STATS_INIT( sc );    // so UPDATE the statistics structure
#endif
        // drop through
#ifdef ETH_DRV_GET_IF_STATS
    case ETH_DRV_GET_IF_STATS:
#endif

#ifdef ETH_DRV_SET_MC_LIST
    case ETH_DRV_SET_MC_LIST:    {
    struct eth_drv_mc_list *mcl = (struct eth_drv_mc_list *)data;
    et->etc->promisc = (ifp->if_flags & IFF_PROMISC) ? TRUE:FALSE;		
        etc_promisc (et->etc, et->etc->promisc);
    et->etc->allmulti = FALSE;
		
    et_set_multicast_list(et, mcl);
		
    return 0;
    }
#endif // ETH_DRV_SET_MC_LIST

#ifdef ETH_DRV_SET_MC_ALL
    case ETH_DRV_SET_MC_ALL:
    et->etc->allmulti = TRUE;
    et->etc->nmulticast = 0;		
    et_init(et);		
    return 0;
#endif // ETH_DRV_SET_MC_ALL
		
    default:
        break;
    }
    if (key == 110)
        err=etc_ioctl(et->etc, (key - 105), data) ? -1: 0;
    dprintf("et_ioctl: end\n");

    return err;
}



// This ISR is called when the ethernet interrupt occurs
static int
et_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    et_info_t *et = (et_info_t *)data;	
    uint events;

	
    if (!et->etc->up)
        return 0;

    events = (*et->etc->chops->getintrevents)(et->etc->ch);

    dprintf("et_isr: events 0x%x\n", events);
	
    if (!(events & INTR_NEW))
        return 0;

    cyg_drv_interrupt_acknowledge(et->vector);

    (*et->etc->chops->intrsoff)(et->etc->ch); /* disable interrupts */

    /* save intstatus bits */
    ASSERT(et->events == 0);
    et->events = events;
	
    return CYG_ISR_CALL_DSR; /* schedule DSR */
}


static void
et_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    et_info_t *et = (et_info_t *)data;
    struct cyg_netdevtab_entry *ndp = (struct cyg_netdevtab_entry *)(et->ndp);
    struct eth_drv_sc *sc = (struct eth_drv_sc *)(ndp->device_instance);

    eth_drv_dsr(vector, count, (cyg_addrword_t)sc);
}


void
et_deliver(struct eth_drv_sc *sc)
{
    et_info_t *et;
    struct chops *chops;
    void *ch;
    struct sgpkt *spkt;

    et = (et_info_t *)sc->driver_private;	
    chops = et->etc->chops;
    ch = et->etc->ch;

    if (!et->etc->up)
        goto done;

    if (et->events & INTR_RX) {
        while ((spkt = (struct sgpkt*) (*chops->rx)(ch)))
            et_sendup(et, spkt);
		
        /* post more rx bufs */
        (*chops->rxfill)(ch);
    }

    if (et->events & INTR_TX) {
        (*chops->txreclaim)(ch, FALSE);
        /*et_txdone (et);*/ /* moved to et_send */
    }
	
    if (et->events & INTR_ERROR)
        if ((*chops->errors)(ch))
            et_init(et);

#if 0
	/* run the tx queue */
    if ((et->txq.len) > 0)
        et_sendnext(et);
#endif
done:	
    /* clear this before re-enabling interrupts */
    et->events = 0;

    /* re-enable interrupts */
    if (et->etc->up)
        (*chops->intrson)(ch);	
}

static void
et_sendnext(et_info_t *et)
{
    struct sgpkt *sg;
    etc_info_t *etc;

    etc = et->etc;

    dprintf("et%d: et_sendnext\n", etc->unit, 0);

    /* dequeue and send each packet */
    while ((*(etc->txavail) > 0)
    && (etc->pioactive == NULL)
    && (sg = et_deq(&et->txq))) {
    /* transmit the frame */
        (*etc->chops->tx)(etc->ch, (void*)sg);

        etc->txframe++;
        etc->txbyte += sg->len;
    }
    dprintf("et%d: et_sendnext ret\n", etc->unit, 0);
}

static void
et_sendup(et_info_t *et, struct sgpkt *sg)
{
    bcmenetrxh_t *rxh;
    uint16 flags;
    uchar eabuf[32];
    struct cyg_netdevtab_entry *ndp;
    struct eth_drv_sc *sc;

    ndp = (struct cyg_netdevtab_entry *)(et->ndp);
    sc = (struct eth_drv_sc *)(ndp->device_instance);
	
    /* packet buffer starts with rxh */
    rxh = (bcmenetrxh_t*) sg->data;

    /* strip off rxh */
    sg->data += HWRXOFF;
    sg->len -= HWRXOFF;

    et->etc->rxframe++;
    et->etc->rxbyte += sg->len;

    /* eh should now be aligned 2-mod-4 */
    ASSERT(((uint)sgpkt.data & 3) == 2);

    sg->len -= ETHER_CRC_LEN;	
	
    /* check for reported frame errors */
    flags = ltoh16(rxh->flags);
    if (flags & (RXF_NO | RXF_RXER | RXF_CRC | RXF_OV)) {
        goto err;
    }

    et->rxpkt = sg;
	
    (sc->funs->eth_drv->recv)( sc, sg->len);	

    dprintf("et%d: et_sendup ret\n", et->etc->unit, 0);

    return;

err:
    bcm_ether_ntoa(((struct ether_header*)sg->data)->ether_shost, eabuf);
    if (flags & RXF_NO) {
        dprintf("et%d: rx: crc error (odd nibbles) from %s\n",
                 et->etc->unit, eabuf);
    }
    if (flags & RXF_RXER) {
        dprintf("et%d: rx: symbol error from %s\n", et->etc->unit, eabuf);
    }
    if ((flags & RXF_CRC) == RXF_CRC) {
        dprintf("et%d: rx: crc error from %s\n", et->etc->unit, eabuf);
    }
    if (flags & RXF_OV) {
        dprintf("et%d: rx: fifo overflow\n", et->etc->unit);
    }
	
    PKTFREE(0,sg,0);
    return;
}

static void 
et_send(struct eth_drv_sc *sc,
            struct eth_drv_sg *sg_list, int sg_len, int total_len,
            unsigned long key)
{
    et_info_t *et;
    struct sgpkt *pkt;
    struct eth_drv_sg *last_sg;
    cyg_uint8 *to_p;
	
    et = (et_info_t *) sc->driver_private;
    
    dprintf("et_send: Tx %d: %d sg's, %d bytes, KEY %x\n", et->index,
             sg_len, total_len, key);

    /* discard if transmit queue is too full */
    if (et->txq.len > DATAHIWAT)
        goto qfull;

    et->tx_keys[et->tx_desc_add] = key;

    pkt = PKTGET (0, total_len, 0);
    to_p = (cyg_uint8 *)(pkt->data);
    for ( last_sg = &sg_list[sg_len]; sg_list < last_sg; sg_list++ ) {
        cyg_uint8 *from_p;
        int l;
        
        from_p = (cyg_uint8 *)(sg_list->buf);
        l = sg_list->len;
		
        if ( l > total_len )
            l = total_len;
		
        memcpy(to_p, from_p, l );
        to_p += l;
        total_len -= l;
		
        if ( 0 > total_len ) 
            break; /* Should exit via sg_last normally */
    }	
	
    /* Next descriptor */
    if ( ++(et->tx_desc_add) >= MAX_TX_DESCRIPTORS)
        et->tx_desc_add = 0;
		
    et_enq(&et->txq, pkt);
    et_sendnext(et);
    et_txdone(et); /* moved from deliver ack immediately*/
    return;

qfull:
    et->etc->txnobuf++;
    et->etc->txerror++;
    et_txdone(et);
    return;	
}

/* enqueue sgpkt on queue */
static void
et_enq(struct sgpktq *sgq, struct sgpkt *sg)
{
    ASSERT(sg->next == NULL);

    if (sgq->tail == NULL) {
        ASSERT(sgq->head == NULL);
        sgq->head = sgq->tail = sg;
    }
    else {
        ASSERT(sgq->head);
        ASSERT(sgq->tail->next == NULL);
        sgq->tail->next = sg;
        sgq->tail = sg;
    }
    sgq->len++;
}

/* dequeue sgpkt from queue */
static struct sgpkt*
et_deq(struct sgpktq *sgq)
{
    struct sgpkt *sg;

    if ((sg = sgq->head)) {
        ASSERT(sgq->tail);
        sgq->head = sg->next;
        sg->next = NULL;
        sgq->len--;
        if (sgq->head == NULL)
            sgq->tail = NULL;
    } else {
        ASSERT(sgq->tail == NULL);
    }
    return (sg);
}

static void
et_txdone(et_info_t *et)
{
    struct cyg_netdevtab_entry *ndp;
    struct eth_drv_sc *sc;

    ndp = (struct cyg_netdevtab_entry *)(et->ndp);
    sc = (struct eth_drv_sc *)(ndp->device_instance);

    while ( 1 ) {
        unsigned long key = et->tx_keys[ et->tx_desc_remove ];

        if ( (et->tx_desc_remove == et->tx_desc_add))
            break;
		
        // Zero the key in global state before the callback:
        et->tx_keys[et->tx_desc_remove ] = 0;

        dprintf("TxDone %d : KEY %x \n", et->index, key);
        // tx_done() can now cope with a NULL key, no guard needed here
        (sc->funs->eth_drv->tx_done)( sc, key, 1 /* status */ );
        
        if ( ++(et->tx_desc_remove) >= MAX_TX_DESCRIPTORS )
            et->tx_desc_remove = 0;
    }	
}

static void 
et_recv( struct eth_drv_sc *sc, struct eth_drv_sg *sg_list, int sg_len )
{
    et_info_t *et;
    struct eth_drv_sg *last_sg;
    volatile cyg_uint8 *from_p;
    unsigned long total_len;

    et = (et_info_t *) sc->driver_private;

    from_p = (cyg_uint8*)(et->rxpkt->data);
    total_len = et->rxpkt->len;

    for ( last_sg = &sg_list[sg_len]; sg_list < last_sg; sg_list++ ) {
        cyg_uint8 *to_p;
        int l;
            
        to_p = (cyg_uint8 *)(sg_list->buf);
        l = sg_list->len;

        CYG_ASSERT( 0 <= l, "sg length -ve" );

        if ( 0 == to_p ) {
            PKTFREE(0, et->rxpkt,0);            
            return; // caller was out of mbufs
        }
        if ( l > total_len )
            l = total_len;

        memcpy( to_p, (unsigned char *)from_p, l );
        from_p += l;
        total_len -= l;
    }
    PKTFREE(0,et->rxpkt,0);	
}

//
// This routine is called to see if it is possible to send another packet.
// It will return non-zero if a transmit is possible, zero otherwise.
//
static int
et_can_send(struct eth_drv_sc *sc)
{
    return 1;
}

void
et_poll(struct eth_drv_sc *sc)
{
    return;
}

// ------------------------------------------------------------------------
// Determine interrupt vector used by a device - for attaching GDB stubs
// packet handler.
static int
et_int_vector(struct eth_drv_sc *sc)
{
    et_info_t *et;
    et = (et_info_t *)sc->driver_private;
    return (et->vector);
}


cyg_alarm_t et_watchdog;

static void
et_init_alarm(et_info_t *et)
{
    cyg_handle_t sys_clk;

    sys_clk = cyg_real_time_clock();

    cyg_clock_to_counter(sys_clk, &et->counter_hdl);

    cyg_alarm_create (et->counter_hdl, et_watchdog,
					  (cyg_addrword_t)et, &et->alarm_hdl, &et->alarm_obj);

}

void
et_watchdog(cyg_handle_t alarm_handle, cyg_addrword_t data)
{
    et_info_t *et;

    et = (et_info_t*)data;

    etc_watchdog (et->etc);
	
}


void
et_link_up(et_info_t *et)
{
    dprintf("et%d: link up (%d%s)\n",
    et->etc->unit, et->etc->speed, (et->etc->duplex? "FD" : "HD"));
}

void
et_link_down(et_info_t *et)
{
    dprintf("et%d: link down\n", et->etc->unit);
}

/*
 * 47XX-specific shared mdc/mdio contortion:
 * Find the et associated with the same chip as <et>
 * and coreunit matching <coreunit>.
 */
void*
et_phyfind(et_info_t *et, uint coreunit)
{
    et_info_t *tmp;

    /* walk the list et's */
    for (tmp = et_list; tmp; tmp = tmp->next) {
        if (et->etc == NULL)
            continue;
        if (CYG_PCI_DEV_GET_BUS(tmp->devid) != CYG_PCI_DEV_GET_BUS(et->devid))
            continue;  
        if (tmp->etc->coreunit != coreunit)
            continue;
		      break;
    }
    return (tmp);
}

/* shared phy read entry point */
uint16
et_phyrd(et_info_t *et, uint phyaddr, uint reg)
{
    uint16 val;

    val = et->etc->chops->phyrd(et->etc->ch, phyaddr, reg);

    return (val);
}

/* shared phy write entry point */
void
et_phywr(et_info_t *et, uint phyaddr, uint reg, uint16 val)
{
    et->etc->chops->phywr(et->etc->ch, phyaddr, reg, val);
}

void
printPkt(char* buf, int len)
{
    int i;
    diag_printf ("pkt buffer: ");
    for (i=0; i< len ; i++)
        diag_printf("0x%x ", (uint8)buf[i]);
    diag_printf ("\n\r ");	
}

