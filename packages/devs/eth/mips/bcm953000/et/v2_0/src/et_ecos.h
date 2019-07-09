/*
 * Copyright (C) 2009 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,USA.
 *
 *
 */

#ifndef _et_ecos_h_
#define _et_ecos_h_

/* tunables */
#define NTXD        64  /* # tx dma ring descriptors (must be ^2) */
#define NRXD        512     /* # rx dma ring descriptors (must be ^2) changed from 512 */
#define NRXBUFPOST  32      /* try to keep this # rbufs posted to the chip */
#define BUFSZ       2048        /* packet data buffer size */
#define RXBUFSZ     (2048 - 256)    /* receive buffer size */


#endif  /* _et_ecos_h_ */
