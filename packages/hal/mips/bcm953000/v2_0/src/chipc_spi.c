/*
 * The general level SPI driver for KeyStone SPI interface.
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: chipc_spi.c,v 1.2 2009/03/30 12:08:09 mflee Exp $
 */

#include <cyg/hal/typedefs.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/sbchipc.h>
#include <cyg/hal/hndsoc.h>
#include <cyg/hal/siutils.h>
#include <cyg/hal/pcicfg.h>
#include <cyg/hal/nicpci.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/bcmdevs.h>
#include <cyg/hal/chipc_spi.h>

#include "siutils_priv.h"

#define BCMDBG_ERR
#ifdef BCMDBG_ERR
#define	SPI_ERROR(args)	printf args
#else
#define	SPI_ERROR(args)
#endif	/* BCMDBG_ERR */

/*#define BCMDBG*/
#ifdef BCMDBG
#define	SPI_MSG(args)	printf args
#else
#define	SPI_MSG(args)
#endif	/* BCMDBG */

static si_t *spi_sih = NULL;

#define SPI_INIT_CHK  \
    if (spi_sih == NULL) {\
         SPI_MSG(("%s,SPI device not init yet!\n", __func__)); \
         return SPI_ERR_INTERNAL; \
    }

static chipc_spi_softc_t spi_softc;

/* --------- SPI register access level driver --------- 
 *  The drivers in this level read/write spi register directly.
 */

/* enable the SPI I/O port */
int
si_spi_select(si_t *sih, uint8 spi_id, int en)
{
    si_info_t *sii;
    uint    origidx;
    uint    intr_val = 0;
    uint32  reg_val = 0, temp;
    chipcregs_t *cc;
    
    /* select the I2C port */
    if (!CC_SPI_ID_IS_VALID(spi_id)){
        SI_ERROR(("%s: invalid spi_id\n", __func__));
        return -1;
    }
    
    sii = SI_INFO(sih);
    INTR_OFF(sii, intr_val);
    origidx = si_coreidx(sih);

    cc = (chipcregs_t *)si_setcoreidx(sih, SI_CC_IDX);
    ASSERT(cc);

    reg_val = R_REG(sii->osh, &cc->SERIAL_IO_SEL);
    temp = (spi_id == CC_SPI_SS0) ? CC_SERIAL_IO_SPI0_MASK : 
            (spi_id == CC_SPI_SS1) ? CC_SERIAL_IO_SPI1_MASK : 
            (spi_id == CC_SPI_SS2) ? CC_SERIAL_IO_SPI2_MASK : 0;
    if (en){
        reg_val |= temp;
    } else {
        reg_val &= ~temp;
    }
    W_REG(sii->osh, &cc->SERIAL_IO_SEL, reg_val);

    /* restore the original index */
    si_setcoreidx(sih, origidx);

    INTR_RESTORE(sii, intr_val);
    return 0;
    
}

/* enable CC lever interrupt on the SPI I/O port */
int
si_spi_ccint_enable(si_t *sih, bool en)
{
    si_info_t *sii;
    uint    origidx;
    uint    intr_val = 0;
    uint32  reg_val = 0, temp;
    chipcregs_t *cc;
    
    sii = SI_INFO(sih);
    INTR_OFF(sii, intr_val);
    origidx = si_coreidx(sih);

    cc = (chipcregs_t *)si_setcoreidx(sih, SI_CC_IDX);
    ASSERT(cc);

    reg_val = R_REG(sii->osh, &cc->SERIAL_IO_INTMASK);

    temp = 0x4;
    if (en){
        reg_val |= temp;
    } else {
        reg_val &= ~temp;
    }

    W_REG(sii->osh, &cc->SERIAL_IO_INTMASK, reg_val);

    /* restore the original index */
    si_setcoreidx(sih, origidx);

    INTR_RESTORE(sii, intr_val);
    return 0;
}

/* mask&set spi_mode_ctrl bits */
int 
si_spi_modectrl(si_t *sih, uint32 mask, uint32 val)
{
    uint regoff = 0;

    regoff = OFFSETOF(chipcregs_t, spi_mode_ctrl);
    return (si_corereg(sih, SI_CC_IDX, regoff, mask, val));
}

/* mask&set spi_mode_ctrl bits */
int 
si_spi_config(si_t *sih, uint32 mask, uint32 val)
{
    uint regoff = 0;

    regoff = OFFSETOF(chipcregs_t, spi_config);
    return (si_corereg(sih, SI_CC_IDX, regoff, mask, val));
}

/* mask&set si_spi_fifo bits.
  * Note : do not read original value before writing data for SPI FIFO IO register,
  *           since this will cause abnormal FIFO in/out behavior.
  */
uint32
si_spi_fifo(si_t *sih, uint32 mask, uint32 val)
{
    si_info_t *sii;
    uint    origidx;
    uint    intr_val = 0;
    uint32 reg_val = 0;
    chipcregs_t *cc;
    
    sii = SI_INFO(sih);
    INTR_OFF(sii, intr_val);
    
    /* save current core index */
    origidx = si_coreidx(sih);

    cc = (chipcregs_t *)si_setcoreidx(sih, SI_CC_IDX);
    ASSERT(cc);

    /* mask and set */
    if (mask || val) {
        reg_val = val;
        W_REG(sii->osh, &cc->spi_fifo_io, reg_val);
    } else {
        /* readback */
        reg_val = R_REG(sii->osh, &cc->spi_fifo_io);
    }
    
    /* restore the original index */
    si_setcoreidx(sih, origidx);
    
    INTR_RESTORE(sii, intr_val);
    
    return reg_val;
}

/* mask&set spi_status bits. 
 *  write (0/1) to clear interrupt flag.
 */
int
si_spi_intr_clear(si_t *sih)
{
    si_info_t *sii;
    uint    origidx;
    uint    intr_val = 0;
    uint32    reg_val = 0;
    chipcregs_t *cc;
    
    sii = SI_INFO(sih);
    INTR_OFF(sii, intr_val);
    
    /* save current core index */
    origidx = si_coreidx(sih);

    cc = (chipcregs_t *)si_setcoreidx(sih, SI_CC_IDX);
    ASSERT(cc);

    reg_val = R_REG(sii->osh, &cc->spi_status);
    /* write (0/1) to clear interrupt flag */
    W_REG(sii->osh, &cc->spi_status, reg_val);
    
    /* restore the original index */
    si_setcoreidx(sih, origidx);
    
    INTR_RESTORE(sii, intr_val);
    
    return 0;
}

/* mask&set spi_status bits. */
int
si_spi_status(si_t *sih, uint32 mask, uint32 *val)
{
    si_info_t *sii;
    uint    origidx;
    uint    intr_val = 0;
    uint32    reg_val = 0;
    chipcregs_t *cc;
    
    sii = SI_INFO(sih);
    INTR_OFF(sii, intr_val);
    
    /* save current core index */
    origidx = si_coreidx(sih);

    cc = (chipcregs_t *)si_setcoreidx(sih, SI_CC_IDX);
    ASSERT(cc);

    reg_val = R_REG(sii->osh, &cc->spi_status);

    *val = (reg_val & mask);
    
    /* restore the original index */
    si_setcoreidx(sih, origidx);
    
    INTR_RESTORE(sii, intr_val);
    
    return 0;
}

/* --------- SPI Bus level(Low Level) driver --------- 
 *  The drivers in this level will combine the Keystone's register 
 *      configuration to proceed the requesting process.
 *  - interface between SPI Core driver and High level driver.
 */

/* Function : chipc_spi_mode_ctrl
 *  - Allow user to set SPI device mode (CPOL, CPHA).
 * Return : 
 * Note :
 *     flags = SPI_MODE_CTRL_MODE
 *                SPI_MODE_CTRL_ACKEN
 *                SPI_MODE_CTRL_ENDIAN
 *                SPI_MODE_CTRL_CLOCK
 *                SPI_MODE_CTRL_LSBEN
 */
int
chipc_spi_mode_ctrl(si_t *sih, uint32 flags, uint32 value)
{    
    uint32 mask = 0, val = 0;

    ASSERT(sih);

    switch(flags) {
        case SPI_MODE_CTRL_MODE :
            if (!CC_SPI_MODE_IS_VALID(value)){
                SPI_ERROR(("%s: Invalid SPI device MODE!\n", __func__));
                return SPI_ERR_PARAM;
            }
            mask = CC_SPIMCTRL_MODE_MASK;

            switch(value) {
                case CC_SPI_MODE_CPOL_0_CPHA_0 :
                    val = CC_SPIMCTRL_MODE_0;
                    break;
                case CC_SPI_MODE_CPOL_0_CPHA_1 :
                    val = CC_SPIMCTRL_MODE_1;
                    break;
                case CC_SPI_MODE_CPOL_1_CPHA_0 :
                    val = CC_SPIMCTRL_MODE_2;
                    break;
                case CC_SPI_MODE_CPOL_1_CPHA_1 :
                    val = CC_SPIMCTRL_MODE_3;
                    break;
                default :
                    return SPI_ERR_PARAM;
            }
            
            break;
        case SPI_MODE_CTRL_ACKEN:
            mask = CC_SPIMCTRL_ACKEN_MASK;
            if (value) {
                val = CC_SPIMCTRL_ACKEN;
            } else {
                val = 0;
            }
            break;
        case SPI_MODE_CTRL_ENDIAN:
            mask = CC_SPIMCTRL_ENDIAN_MASK;
            if (value) {
                val = CC_SPIMCTRL_BE;
            } else {
                val = CC_SPIMCTRL_LE;
            }
            break;
        case SPI_MODE_CTRL_CLOCK:
            mask = CC_SPIMCTRL_CLK_MASK;
            val = value;
            break;
        case SPI_MODE_CTRL_LSBEN:
            mask = CC_SPIMCTRL_LSB_MASK;
            if (value) {
                val = CC_SPIMCTRL_LSB_FIRST;
            } else {
                val = CC_SPIMCTRL_MSB_FIRST;
            }
            break;
        default :
            return SPI_ERR_PARAM;
    }

    SPI_MSG(("%s: flags = 0x%x, value = 0x%x\n", __func__, flags, val));

    /* select the spi interface */
    si_spi_modectrl(sih, mask, val);

    return SPI_ERR_NONE;
}

/* Function : chipc_spi_io_select 
 *  - Enable Keystone's SPI port.
 * Return :
 * Note :
 */
int 
chipc_spi_io_select(si_t *sih, cc_spi_id_t id, int en) 
{
    ASSERT(sih);
    if (!CC_SPI_ID_IS_VALID(id)){
        SPI_ERROR(("%s: Invalid SPI device ID!\n", __func__));
        return SPI_ERR_PARAM;
    }
    
    /* select the spi interface */
    if (si_spi_select(sih, (uint8)id, (en) ? TRUE : FALSE)){
        SPI_ERROR(("%s: Failed on %s SPI_%d !", 
                    __func__, (en) ? "Enabling" : "Disabling", id));
        return SPI_ERR_INTERNAL;
    }

    SPI_MSG(("%s: %s SPI_%d is DONE!\n", 
            __func__, (en) ? "Enable" : "Disable", id));
    return SPI_ERR_NONE;
}

/* Function : chipc_spi_io_intr 
 *  - Enable/disable Keystone's SPI interrupt mode.
 * Return :
 * Note :
 */
int 
chipc_spi_io_intr(si_t *sih, int en) 
{
    ASSERT(sih);
    
    /* select the spi interface */
    if (si_spi_ccint_enable(sih, (en) ? TRUE : FALSE)){
        SPI_ERROR(("%s, Failed to %s SPI IO interrupt!", 
                    __func__, (en) ? "Enable" : "Disable"));
        return SPI_ERR_INTERNAL;
    }

    SPI_MSG(("%s: %s is DONE!\n", __func__, (en) ? "Enable" : "Disable"));
    return SPI_ERR_NONE;
}

/* Function : chipc_spi_set_freq 
 *  - set Keystone's SPI device frequency
 * Return :
 * Note :
 *  1. SPI clock is Keystone backplane
 *  2. the unit of the speed is hz.
 *  3. spi_freq = sys_freq / (2^(N+1)),  N= clock divider parameter (from 1 ~ 15)
 *  4. System Clock will be retrieved from SI interface and a proper spi 
 *      clock value will be auto-generated and selected to match user's 
 *      request frequency.
 */
int 
chipc_spi_set_freq(si_t *sih, cc_spi_id_t id, uint32 speed_hz) 
{
    uint  sys_freq, spi_freq;
    int  n;
    uint32  value;

    ASSERT(sih);

    if (!CC_SPI_ID_IS_VALID(id)){
        SPI_ERROR(("%s: Invalid SPI Device ID!\n", __func__));
        return SPI_ERR_PARAM;
    }
        
    /* -------- set spi clock --------- */
    /* get Fsys */
    sys_freq = si_clock(sih);

    SPI_MSG(("%s: sys_clock = %d, speed_hz = %d\n", __func__, sys_freq, speed_hz));

    /* retrive the N value */
    /* spi clock counting formula  : spi_freq = sys_freq / (2^(N+1)) */
    for (n = 0; n < SPI_CCD_MAX; n++){
        spi_freq = sys_freq / (2 << n);

        if (spi_freq <= speed_hz) {
            break;
        }
    }

    SPI_MSG(("%s: spi_freq = %d, clock divider parameters N = 0x%x\n", 
        __func__, spi_freq, n));

    /* Set the SPI clock (clock divider parameters n : [7:4] in SPI Moder Control register) */
    value = n << 4;
    if (chipc_spi_mode_ctrl(sih, SPI_MODE_CTRL_CLOCK, value)) {
        SPI_ERROR(("%s: Failed to set device clock frequence at SPI_%d!\n", __func__, id));
        return SPI_ERR_PARAM;
    }

    return SPI_ERR_NONE;
}

/* Function : chipc_spi_fifo_rw 
 *  - Read/write data via FIFO for Keystone's SPI device.
 * Return :
 * Note :
 *     Read data from FIFO : if mask = val = 0
 *     Write data to FIFO : if (mask | val)  != 0
 */
uint32
chipc_spi_fifo_rw(si_t *sih, uint32 mask, uint32 val) 
{
    uint32 reg_val;

    ASSERT(sih);

    reg_val = si_spi_fifo(sih, mask, val);

    SPI_MSG(("%s: %s fifo reg_val = 0x%x\n", __func__, 
        (!mask && !val) ? "Read" : "Write", reg_val));

    return reg_val;
}

/* Function : chipc_spi_write_fifo 
 *  - Write data to Keystone's SPI device via FIFO.
 * Return :
 * Note :
 */
int
chipc_spi_write_fifo(si_t *sih, uint8 *data, int len) 
{
    int idx, m;
    uint32 value;

    ASSERT(sih);
    /* len < 32 bytes */
    if (len > SPI_FIFO_MAX_SIZE) {
        SPI_ERROR(("%s: Write failed : can not exceed 32 bytes!\n", __func__));
        return SPI_ERR_PARAM;
    }

    /* Write command/data to Keystone's SPI FIFO */
#ifndef IL_BIGENDIAN
    value = 0;
    for (idx = 0 ; idx < len ; idx++) {
        m = idx % 4;
        value |= (*data << (m*8));
        data++;
        if ((m == 3) || (idx == (len - 1))) {
            chipc_spi_fifo_rw(sih, CC_SPIFIFOIO_MASK, value);
            value = 0;
        }
    }
#else /* Big Endiag */
    value = 0;
    for (idx = 0 ; idx < len ; idx++) {
        m = idx % 4;
        value |= (*data << ((3-m)*8));
        data++;
        if ((m == 3) || (idx == (len - 1))) {
            chipc_spi_fifo_rw(sih, CC_SPIFIFOIO_MASK, value);
            value = 0;
        }
    }
#endif /* CFG_LITTLE */

    return SPI_ERR_NONE;
}

 /* Function : chipc_spi_status 
 *     - Get spi device status (offset 0x28c in chipcommon).
 * Return :
 *     - Return the current SPI status.
 * Note :
 */
uint32
chipc_spi_status(si_t *sih) 
{
    uint32 spi_status;

    ASSERT(sih);

    if (si_spi_status(sih, CC_SPISTS_MASK, &spi_status)){
        SPI_ERROR(("%s: Retrive SPI status error\n", __func__));
        return SPI_ERR_INTERNAL;
    }

    return spi_status;

}

 /* Function : chipc_spi_intr_clear 
 *     - Clear interrupt flag after transcation done.
 * Return :
 * Note :
 */
int
chipc_spi_intr_clear(si_t *sih) 
{
    ASSERT(sih);

    if (si_spi_intr_clear(sih)){
        SPI_ERROR(("%s: Clear SPI interrupt flag fails!\n", __func__));
        return SPI_ERR_INTERNAL;
    }

    return SPI_ERR_NONE;

}

/* Function : _chipc_spi_wait_for_iflg_set 
 *  - Wait for the spi interrupt flag is up.
 * Return :
 * -  SPI_ERR_NONE or SPI_ERR_TIMEOUT
 * Note :
 */
static int  
_chipc_spi_wait_for_iflg_set(si_t *sih) 
{
    uint32 spireg_val = 0;
    int retry = SPI_INTFLAG_TIMEOUT;

    ASSERT(sih);

    /* check if the spi IFLG is set */
    while((retry--) > 0) {

        spireg_val = chipc_spi_status(sih);

        if (spireg_val & CC_SPISTS_INTFLAG) {
            return SPI_ERR_NONE;
        }
        OSL_DELAY(1);
    }
    SPI_MSG(("%s: %d, spireg_val=%x, retry=%d\n",
            __func__,__LINE__,spireg_val,retry));
    
    return (retry > 0) ? SPI_ERR_NONE : SPI_ERR_TIMEOUT;
}

/* Function : chipc_spi_wait 
 *  - Wait for the spi interrupt flag.
 * Return :
 * Note :
 */
int  
chipc_spi_wait(si_t *sih) 
{
    int rv = SPI_ERR_NONE;
#ifdef BCMDBG 
    uint32 spireg_val = 0;
#endif

    ASSERT(sih); 

    rv = _chipc_spi_wait_for_iflg_set(sih);

#ifdef BCMDBG    
    /* Get SPI status for Debug */
    spireg_val = chipc_spi_status(sih);
#endif
    
    SPI_MSG(("%s: current SPI status=0x%x\n", __func__, spireg_val));

    return rv;
    
}

/* Function : chipc_spi_config 
 *  - Start to do Read/write transaction on FIFO to Keystone's SPI device.
 * Return :
 * Note :
 */
int
chipc_spi_config(si_t *sih, cc_spi_id_t id, uint rdc, uint wdc, uint wcc)
{
    int rv = SPI_ERR_NONE;
    uint32 reg_val = 0;

    if (!CC_SPI_ID_IS_VALID(id)){
        SPI_ERROR(("%s: Invalid SPI device ID!\n", __func__));
        return SPI_ERR_PARAM;
    }

    /* (RdDatCnt/WrDatCnt/WrCmdCnt) <= 32 bytes */
    if ((rdc > SPI_FIFO_MAX_SIZE) || 
        (wdc > SPI_FIFO_MAX_SIZE) || 
        (wcc > SPI_FIFO_MAX_SIZE)) {
        SPI_ERROR(("%s: Config failed : can not exceed 32 bytes!\n", __func__));
        return SPI_ERR_PARAM;
    }

    /* (WrCmdCnt + WrDatCnt) <= 32 bytes */
    if ((wcc + wdc) > SPI_FIFO_MAX_SIZE) {
        SPI_ERROR(("%s: Config failed : (WrCmdCnt+WrDatCnt) can not exceed 32 bytes!\n",
            __func__));
        return SPI_ERR_PARAM;
    }
    
    reg_val = (V_SPICFG_SS(id) |
                 V_SPICFG_RDC(rdc) |
                 V_SPICFG_WDC(wdc) |
                 V_SPICFG_WCC(wcc) |
                 V_SPICFG_START);

    SPI_MSG(("%s: reg_val = 0x%x\n", __func__, reg_val));

    si_spi_config(sih, CC_SPICFG_MASK, reg_val);

    /* Polling SPI status until SPI Interrupt flag is set after done */
    if (chipc_spi_wait(sih)){
        /* timeout occurred */
        SPI_ERROR(("%s: TIMEOUT after START at SPI\n", __func__));
        return SPI_ERR_TIMEOUT;
    }

    /* Write (0/1) to clear Interrupt flag after done */
    rv = chipc_spi_intr_clear(sih);

    return rv;
}


/* --------- SPI High level driver --------- 
 * The drivers in this level also know as general SPI driver. 
 *  - Request the SPI read/write on SPI device through SPI driver.
 * Function : chipc_spi_attach 
 * Function : chipc_spi_init 
 * Function : chipc_spi_enable 
 * Function : chipc_spi_disable 
 * Function : chipc_spi_read
 * Function : chipc_spi_write
 */

/* Function : chipc_spi_attach
 *  - Attach the Keystone's SPI device SPI0/SPI1/SPI2.
 * Return :
 * Note : 
 *     Use parameter id as device id(store in softc->id first).
 */
int
chipc_spi_attach(cc_spi_id_t id)
{
    chipc_spi_softc_t *s = &spi_softc;

    if (!CC_SPI_ID_IS_VALID(id)){
        SPI_ERROR(("%s: Invalid SPI device ID!\n", __func__));
        return SPI_ERR_PARAM;
    }

    memset(s, 0, sizeof(s));
    s->id = id;

    return SPI_ERR_NONE;
}

/* Function : chipc_spi_init
 *  - Init Keystone's SPI device SPI0/SPI1/SPI2.
 * Return :
 * Note : 
 *     Enable the serial IO select register (offset 0x2f4 in chipcommon) for SPI devices.
 *     Set the SPI mode control register (offset 0x280 in chipcommon) for SPI.
 *     
 */
int
chipc_spi_init(void)
{
    int rv = SPI_ERR_NONE;
    chipc_spi_softc_t *s = &spi_softc;
    uint32 value = 0;
    
    spi_sih = si_kattach(SI_OSH);

    memset(s->buf, 0, sizeof(s->buf));
    s->buf_index = 0;
    s->states = SPI_STATES_DISABLE;

    /* Disable the SPI serial IO interrupt by default */
    if (chipc_spi_io_intr(spi_sih, FALSE)) {
        SPI_ERROR(("%s: Failed to disable SPI IO interrupt!\n", __func__));
        return SPI_ERR_INTERNAL;
    }

    /* Select the SPI interface for SPI device id */
    if (chipc_spi_io_select(spi_sih, s->id, TRUE)) {
        SPI_ERROR(("%s: Failed on enabling SPI_%d!\n", __func__, s->id));
        return SPI_ERR_INTERNAL;
    }

    /* Set the SPI mode */
    value = CC_SPI_MODE_CPOL_1_CPHA_1;
    if (chipc_spi_mode_ctrl(spi_sih, SPI_MODE_CTRL_MODE, value)) {
        SPI_ERROR(("%s: Failed to set device mode at SPI_%d!\n", __func__, s->id));
        return SPI_ERR_PARAM;
    }

#ifndef IL_BIGENDIAN
    value = CC_SPIMCTRL_LE;
#else /* Big Endiag */
    value = CC_SPIMCTRL_BE;
#endif /* CFG_LITTLE */

    /* Set the SPI endian */
    if (chipc_spi_mode_ctrl(spi_sih, SPI_MODE_CTRL_ENDIAN, value)) {
        SPI_ERROR(("%s: Failed to set device endian at SPI_%d!\n", __func__, s->id));
        return SPI_ERR_PARAM;
    }

    /* Set the SPI clodk */
    value = 0x60;
    if (chipc_spi_mode_ctrl(spi_sih, SPI_MODE_CTRL_CLOCK, value)) {
        SPI_ERROR(("%s: Failed to set device clock frequence at SPI_%d!\n", __func__, s->id));
        return SPI_ERR_PARAM;
    }

    /* Initialize SPI's interrupt flag */
    if (chipc_spi_status(spi_sih) & CC_SPISTS_INTFLAG) {
        /* Write (0/1) to clear Interrupt flag for initialization */
        rv = chipc_spi_intr_clear(spi_sih);
    }

    return rv;
}

/* Function : chipc_spi_enable
 *  - Enable the SPI device :
 *       initialize the sw_info : get ready to do read/write transaction on FIFO.
 * Return :
 * Note : 
 */
int
chipc_spi_enable(cc_spi_id_t id)
{
    chipc_spi_softc_t *s = &spi_softc;

    SPI_INIT_CHK;

    if (!CC_SPI_ID_IS_VALID(id)){
        SPI_ERROR(("%s: Invalid SPI device ID!\n", __func__));
        return SPI_ERR_PARAM;
    }

    memset(s->buf, 0, sizeof(s->buf));
    s->id = id;
    s->buf_index = 0;
    s->states = SPI_STATES_ENABLE;

    return SPI_ERR_NONE;
}

/* Function : chipc_spi_disable
 *  - Disable the SPI device : 
 *       finished read/write transaction on FIFO (set states = SPI_STATES_DISABLE).
 * Return :
 * Note : 
 *     For SPI write operation (while s->states == SPI_STATES_WRITE), 
 *     we start to do really write transaction on FIFO 
 *     when we call function chipc_spi_disable. 
 *
 *     For SPI read operation, we do really read transaction on FIFO 
 *     when we call function chipc_spi_read.
 */
int
chipc_spi_disable(cc_spi_id_t id)
{
    chipc_spi_softc_t *s = &spi_softc;

    SPI_INIT_CHK;

    if (!CC_SPI_ID_IS_VALID(id)){
        SPI_ERROR(("%s: Invalid SPI device ID!\n", __func__));
        return SPI_ERR_PARAM;
    }

    /* 
     *  if s->states == SPI_STATES_WRITE, start to do write transaction on FIFO
     *  ReadDataCnt = 0, WriteDataCnt = s->buf_index, WriteCmdCnt = 0
     */
    if (s->states == SPI_STATES_WRITE) {
        if (chipc_spi_write_fifo(spi_sih, s->buf, s->buf_index)) {
            SPI_ERROR(("%s: Failed to write data to SPI FIFO at SPI_%d!\n", __func__, s->id));
            return SPI_ERR_PARAM;
        }
        if (chipc_spi_config(spi_sih, s->id, 0, s->buf_index, 0)) {
            SPI_ERROR(("%s: Failed to start write transaction at SPI_%d!\n", __func__, s->id));
            return SPI_ERR_PARAM;
        }
    }

    s->states = SPI_STATES_DISABLE;
    
    return SPI_ERR_NONE;
}

/* Function : chipc_spi_read
 *  - Read operation through Keystone's SPI.
 * Return :
 * Note : 
 *     For SPI read operation, we start to do really read transaction on FIFO 
 *     when we call function chipc_spi_read. 
 */
int
chipc_spi_read(uint8_t * buf, int len, uint8_t data_out)
{
    chipc_spi_softc_t *s = &spi_softc;
    int idx, m;
    uint32_t value;

    SPI_INIT_CHK;

    /* 
     * (Reading data length) <= 32 bytes
     * - The Maximum size of SPI's FIFO is 32 bytes(once time).
     */
    if (len > SPI_FIFO_MAX_SIZE) {
        SPI_ERROR(("%s: Read failed : can not exceed 32 bytes!\n",__func__));
        return SPI_ERR_PARAM;
    }

    /* 
     *  if s->states == SPI_STATES_WRITE, start to do Read transaction on FIFO
     *  ReadDataCnt = len, WriteDataCnt = s->buf_index, WriteCmdCnt = 0
     */
    if (s->states == SPI_STATES_WRITE) {
        if (chipc_spi_write_fifo(spi_sih, s->buf, s->buf_index)) {
            SPI_ERROR(("%s: Failed to write data to SPI FIFO at SPI_%d!\n", __func__, s->id));
            return SPI_ERR_PARAM;
        }
        if (chipc_spi_config(spi_sih, s->id, (uint)len, s->buf_index, 0)) {
            SPI_ERROR(("%s: Failed to start read transaction at SPI_%d!\n", __func__, s->id));
            return SPI_ERR_PARAM;
        }
        s->states = SPI_STATES_READ;
    } else {
        SPI_ERROR(("%s: Failed to do SPI read operation!\n",__func__));
        return SPI_ERR_PARAM;
    }
 
    /* Get spi status */
    value = chipc_spi_status(spi_sih);

    /* Check FIFO is not empty for reading */
    if (value & CC_SPISTS_FIFOE) {
        SPI_ERROR(("%s: Read failed at SPI: FIFO is empty for reading!\n", __func__));
        return SPI_ERR_INTERNAL;
    }

    /* Read data from FIFO IO register until FIFO is empty(read 4 bytes each time) */
#ifndef IL_BIGENDIAN 
    value = 0;
    for (idx = 0 ; idx < len ; idx++) {
        m = idx % 4;
        if (m == 0) {
            value = chipc_spi_fifo_rw(spi_sih, 0, 0);
        }
        *buf++ = (uint8)(value >> (m*8));
    }
#else /* Big Endiag */
    value = 0;
    for (idx = 0 ; idx < len ; idx++) {
        m = idx % 4;
        if (m == 0) {
            value = chipc_spi_fifo_rw(spi_sih, 0, 0);
        }
        *buf++ = (uint8)(value >> ((3-m)*8));
    }
#endif /* CFG_LITTLE */

    return SPI_ERR_NONE;
}

/* Function : chipc_spi_write 
 *  - Write operation through Keystone's SPI.
 * Return :
 * Note : 
 *     Write the SPI device command(or data) and keep in software buffer first.
 *     For SPI write operation (while s->states == SPI_STATES_WRITE), 
 *     we start to do really write transaction on FIFO 
 *     when we call function chipc_spi_disable. 
 */
int
chipc_spi_write(uint8_t *buf, int len)
{
    chipc_spi_softc_t *s = &spi_softc;
    int i = 0;

    SPI_INIT_CHK;

    if (s->states != SPI_STATES_ENABLE && s->states != SPI_STATES_WRITE) {
        SPI_ERROR(("%s: Failed to do SPI read/write operation!\n", __func__));
        return SPI_ERR_PARAM;
    }

    /* The Maximum size of SPI's FIFO is 32 bytes */
    if ((s->buf_index + len) > SPI_FIFO_MAX_SIZE) {
        SPI_ERROR(("%s: Failed to write data to FIFO : the size of FIFO > 32 bytes!\n", 
            __func__));
        return SPI_ERR_PARAM;
    }

    /* Write SPI command or data and keep in software first */
    while (len) {
        s->buf[s->buf_index++] = buf[i++];
        len--;
    }

    s->states = SPI_STATES_WRITE;

    return SPI_ERR_NONE;
}

