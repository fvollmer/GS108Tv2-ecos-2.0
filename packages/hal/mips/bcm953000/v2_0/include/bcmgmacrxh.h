/*
 * Hardware-specific Receive Data Header for the
 * Broadcom Home Networking Division
 * BCM47XX GbE cores.
 *
 * $Copyright Open Broadcom Corporation$
 * $Id: bcmgmacrxh.h,v 1.3 2009/07/07 13:23:26 jimhuang Exp $
 */

#ifndef _bcmgmacrxh_h_
#define	_bcmgmacrxh_h_

/*
 * The Ethernet GMAC core returns an 8-byte Receive Frame Data Header
 * with every frame consisting of
 * 16 bits of frame length, followed by
 * 16 bits of GMAC rx descriptor info, followed by 32bits of undefined.
 */
typedef volatile struct {
	uint16	len;
	uint16	flags;
	uint16	pad[14];
} bcmgmacrxh_t;

#define	RXHDR_LEN	32	/* Header length */

#define	GRXF_DT_MASK	((uint16)0xf)		/* data type */
#define	GRXF_DT_SHIFT	12
#define	GRXF_DC_MASK	((uint16)0xf)	/* (num descr to xfer the frame) - 1 */
#define	GRXF_DC_SHIFT	8
#define	GRXF_OVF	((uint16)1 << 7)	/* overflow error occured */
#define	GRXF_OVERSIZE	((uint16)1 << 2)	/* frame size > rxmaxlength */
#define	GRXF_CRC	((uint16)1 << 3)	/* crc error */
#define	GRXF_PT_MASK	((uint16)3)		/* packet type 0 - Unicast,
						 * 1 - Multicast, 2 - Broadcast
						 */

#endif	/* _bcmgmacrxh_h_ */
