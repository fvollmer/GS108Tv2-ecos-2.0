
#include <pkgconf/system.h>
#include <pkgconf/hal.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/siutils.h>
#include <cyg/hal/sbchipc.h>
#include <cyg/io/flashDrvLib.h>
#include <cyg/hal/sflash.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/siutils_priv.h>
#define FLASH_16BIT 1

#define xor_val 0



    static si_t *sFlash_sih = NULL;
    static chipcregs_t *cc;
    static struct sflash *flashDevice;
    extern struct flash_drv_funcs_s flash25L12845;



#define FLASH_ADDR(dev, addr) \
    ((volatile cyg_uint8 *) (((int)(dev) + (int)(addr)) ^ xor_val))

#define FLASH_WRITE(dev, addr, value) \
    (*FLASH_ADDR(dev, addr) = (value))

#define FLASH_READ(dev, addr) \
    (*FLASH_ADDR(dev, addr))


LOCAL void
flashAutoSelect(FLASH_TYPES *dev, FLASH_VENDORS *vendor)
{

    si_info_t *sii;
    sFlash_sih = si_kattach(SI_OSH);
    sii = SI_INFO(sFlash_sih);

    cc = (chipcregs_t *)si_setcore(sFlash_sih, CC_CORE_ID, 0);
/* Read dev and vendor from flash */

    flashDevice = sflash_init(sFlash_sih, cc);
    flashDevSectorSize = flashDevice->blocksize;
    flashSectorCount = flashDevice->numblocks;

    if (flashVerbose)
        printf("flashAutoSelect(): dev = 0x%x, vendor = 0x%x\n",
               flashDevice->devid, flashDevice->vendor);
    
}
#if 0
LOCAL int
flashEraseDevices(volatile unsigned char *sectorBasePtr)
{
    int             i;
    unsigned int    tmp;

    if (flashVerbose) {
        printf("Erasing Sector @ 0x%08x\n",(unsigned int)sectorBasePtr);
    }
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0xaaa, 0xaa);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0x55);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0xaaa, 0x80);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0xaaa, 0xaa);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0x55);
    FLASH_WRITE(sectorBasePtr, 0x0, 0x30);

    for (i = 0; i < FLASH_ERASE_TIMEOUT_COUNT; i++) {
           cyg_thread_delay((cyg_tick_count_t)FLASH_ERASE_TIMEOUT_TICKS);

        tmp = FLASH_READ(sectorBasePtr, 0x0);

        if ((tmp & 0x80) == 0x80) {
            if (flashVerbose > 1)
                printf("flashEraseDevices(): all devices erased\n");
            return (OK);
        }
    }

    if ((tmp & 0x20) == 0x20) {
        printf("flashEraseDevices(): addr 0x%08x erase failed\n",
           (int)sectorBasePtr);
    } else {
        printf("flashEraseDevices(): addr 0x%08x erase timed out\n",
           (int)sectorBasePtr);
    }

    return (ERROR);
}
#endif
LOCAL int
flashEraseSector(int sectorNum)
{
    unsigned int   sectorBasePtr;

    sectorBasePtr = FLASH_SECTOR_ADDRESS(sectorNum);
    sectorBasePtr = sectorBasePtr - FLASH_BASE_ADDRESS;
    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashEraseSector(): Sector %d invalid\n", sectorNum);
        return (ERROR);
    }
    if (sflash_erase(sFlash_sih, cc, sectorBasePtr) == ERROR) {
        printf("flashEraseSector(): erase devices failed sector=%d\n",
               sectorNum);
        return (ERROR);
    }

    if (flashVerbose)
        printf("flashEraseSector(): sectorBasePtr %x Sector %d erased\n",sectorBasePtr, sectorNum);

    return (OK);
}

LOCAL int
flashRead(int sectorNum, char *buff, unsigned int offset, unsigned int count)
{
    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashRead(): Illegal sector %d\n", sectorNum);
        return (ERROR);
    }

    if (sflash_read(sFlash_sih, cc, offset, count, buff) == ERROR) {
        printf("flashEraseSector(): erase devices failed sector=%d\n",
               sectorNum);
        return (ERROR);
    }

    return (0);
}


LOCAL int
#ifdef FLASH_16BIT
flashProgramDevices(volatile unsigned short *addr, unsigned short val)
#else
flashProgramDevices(volatile unsigned char *addr, unsigned char val)
#endif
{
    int             polls;
    unsigned int    tmp;

    FLASH_WRITE(FLASH_BASE_ADDRESS, 0xaaa, 0xaa);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0x55);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0xaaa, 0xa0);
    /* FLASH_WRITE(addr, 0x0, val);*/
    *addr = val;

    for (polls = 0; polls < FLASH_PROGRAM_TIMEOUT_POLLS; polls++) {
        tmp = *addr;

#ifdef FLASH_16BIT
        if ((tmp & 0x8080) == (val & 0x8080)) {
#else
        if ((tmp & 0x80) == (val & 0x80)) {
#endif
            if (flashVerbose > 2)
                printf("flashProgramDevices(): devices programmed\n");
            return (OK);
        }
    }

#ifdef FLASH_16BIT
    if ((tmp & 0x2020) != 0) {
#else
    if ((tmp & 0x20) != 0) {
#endif
    /* 
     * We've already waited so long that chances are nil that the
     * 0x80 bits will change again.  Don't bother re-checking them.
     */
        printf("flashProgramDevices(): Address 0x%08x program failed\n",
               (int)addr);
    } else {
        printf("flashProgramDevices(): timed out\n");
    }

    return (ERROR);
}

LOCAL int
flashWrite(int sectorNum, char *buff, unsigned int offset, unsigned int count)
{
    unsigned char   *curBuffPtr, *flashBuffPtr;
    int             i;
    unsigned int    flashPointer;

    curBuffPtr = (unsigned char *)buff;
    flashBuffPtr = (unsigned char *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);
    flashBuffPtr = flashBuffPtr - FLASH_BASE_ADDRESS;

    flashPointer = (FLASH_SECTOR_ADDRESS(sectorNum) + offset);
    flashPointer = flashPointer - FLASH_BASE_ADDRESS;

        if (sflash_write(sFlash_sih, cc, flashPointer, count, curBuffPtr) < 0) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
               sectorNum,flashPointer);
            return (ERROR);
        }

    return (0);
}


struct flash_drv_funcs_s flash25L12845 = {
    FLASH_25L12845, MXIC,
    flashAutoSelect,
    flashEraseSector,
    flashRead,
    flashWrite
};
