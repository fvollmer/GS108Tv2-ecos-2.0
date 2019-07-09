/*
 * $Id: chipc_spi.h,v 1.3 2009/04/20 08:36:09 cchao Exp $
 * $Copyright Open Broadcom Corporation$
 */

#ifndef _CHIPC_SPI_H_
#define _CHIPC_SPI_H_

#include "typedefs.h"
#include <cyg/hal/sbchipc.h>

typedef struct chipc_spi_softc_s {
    uint id;        /* SPI device ID */
    uint8 buf[32];        /* Software control info */
    uint buf_index;        /* the buffer length */
    uint states;        /* start ready to do read/write transaction */
} chipc_spi_softc_t;

#define SPI_FIFO_MAX_SIZE 32
#define SPI_INTFLAG_TIMEOUT 100

#define SPI_STATES_DISABLE 0x0
#define SPI_STATES_ENABLE 0x1
#define SPI_STATES_WRITE 0x2
#define SPI_STATES_READ 0x4

/* SPI mode control definition */
#define SPI_MODE_CTRL_MODE     0x0        /* SPI Mode (CPOL, CPHA) */
#define SPI_MODE_CTRL_ACKEN     0x1        /* SPI RACK enable */
#define SPI_MODE_CTRL_ENDIAN     0x2        /* SPI Big Endian enable */
#define SPI_MODE_CTRL_CLOCK     0x4        /* SPI Clock divider parameter */
#define SPI_MODE_CTRL_LSBEN     0x10        /* SPI LSB first enable */

#define SPI_CCD_MAX      0xf     /* max N value for spi clock divider parameter(CCD) */

/* reutrn value for SPI driver */
#define SPI_ERR_NONE          0
#define SPI_ERR_TIMEOUT       -1
#define SPI_ERR_INTERNAL      -2
#define SPI_ERR_PARAM         -3
#define SPI_ERR_UNAVAIL       -4
#define SPI_ERR_UNKNOW        -5

#define _DD_MAKEMASK1(n) (1 << (n))
#define _DD_MAKEMASK(v,n) ((((1)<<(v))-1) << (n))
#define _DD_MAKEVALUE(v,n) ((v) << (n))
#define _DD_GETVALUE(v,n,m) (((v) & (m)) >> (n))

/* SPICONFIG: SPI Configuration Register (0x284, R/W) */
#define S_SPICFG_SS             0                       /* SPI SS (device(n)) enable */
#define V_SPICFG_SS(x)         _DD_MAKEVALUE(x,S_SPICFG_SS)

#define S_SPICFG_RDC           3                       /* SPI Read byte count */
#define V_SPICFG_RDC(x)        _DD_MAKEVALUE(x,S_SPICFG_RDC)

#define S_SPICFG_WDC           13                       /* SPI Write data byte count */
#define V_SPICFG_WDC(x)        _DD_MAKEVALUE(x,S_SPICFG_WDC)

#define S_SPICFG_WCC           23                       /* SPI Write command byte count */
#define V_SPICFG_WCC(x)        _DD_MAKEVALUE(x,S_SPICFG_WCC)

#define V_SPICFG_START         _DD_MAKEVALUE(1,31) /* Start SPI transfer */


/* spi register access level function prototype */
extern int si_spi_select(si_t * sih, uint8 spi_id, int en);
extern int si_spi_ccint_enable(si_t * sih, bool en);
extern int si_spi_modectrl(si_t * sih, uint32 mask, uint32 val);
extern int si_spi_config(si_t * sih, uint32 mask, uint32 val);
extern uint32 si_spi_fifo(si_t * sih, uint32 mask, uint32 val);
extern int si_spi_intr_clear(si_t *sih);
extern int si_spi_status(si_t *sih, uint32 mask, uint32 *val);

/* spi low level function prototype */
extern int chipc_spi_mode_ctrl(si_t * sih, uint32 flags, uint32 value);
extern int chipc_spi_io_select(si_t * sih, cc_spi_id_t id, int en);
extern int chipc_spi_io_intr(si_t * sih, int en);
extern int chipc_spi_set_freq(si_t * sih, cc_spi_id_t id, uint32 speed_mhz);
extern uint32 chipc_spi_fifo_rw(si_t * sih, uint32 mask, uint32 val);
extern int chipc_spi_write_fifo(si_t * sih, uint8 * data, int len);
extern uint32 chipc_spi_status(si_t * sih);
extern int chipc_spi_intr_clear(si_t * sih);
extern int chipc_spi_wait(si_t * sih);
extern int chipc_spi_config(si_t * sih, cc_spi_id_t id, uint rdc, uint wdc, uint wcc);

/* spi high level function prototype */
extern int chipc_spi_attach(cc_spi_id_t id);
extern int chipc_spi_init(void);
extern int chipc_spi_enable(cc_spi_id_t id);
extern int chipc_spi_disable(cc_spi_id_t id);
extern int chipc_spi_read(uint8_t * buf, int len, uint8_t data_out);
extern int chipc_spi_write(uint8_t * buf, int len);

#endif /* _CHIPC_SPI_H_ */

