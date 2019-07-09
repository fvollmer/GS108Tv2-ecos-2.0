#include <pkgconf/system.h>
#include <pkgconf/hal.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <cyg/io/flashDrvLib.h>

#include <cyg/hal/osl.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>

#define xor_val 0x0

#define FLASH_WIDTH     cyg_uint16*

#define FLASH_ADDR(dev, addr) \
    ((volatile cyg_uint16 *) ((FLASH_WIDTH)dev + addr))

#define FLASH_WRITE(dev, addr, value) \
    (*FLASH_ADDR(dev, addr) = (value))

#define FLASH_READ(dev, addr) \
    (*FLASH_ADDR(dev, addr))

extern struct flash_drv_funcs_s flash29GL256;

LOCAL void
flashReadReset(void)
{
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0xf0);
}

LOCAL void
flashAutoSelect(FLASH_TYPES *dev, FLASH_VENDORS *vendor)
{
    flashReadReset();   
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0xaa);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x2aa, 0x55);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0x90);

    *vendor = FLASH_READ(FLASH_BASE_ADDRESS, 0);
    *dev = FLASH_READ(FLASH_BASE_ADDRESS, 1);

    if (flashVerbose)
        printf("flashAutoSelect(): dev = 0x%x, vendor = 0x%x\n",
               (int)*dev, (int)*vendor);
    flashReadReset();   

    if ((*dev != FLASH_29GL256) || ((*vendor != AMD) && (*vendor != ALLIANCE))) {
        *vendor = *dev = 0xFF;
    } else {
        /*
         * Special case for 29GL series:
         * It shares the same device ID as 29VL640 series,
         * so we do one step further to make sure which one we're using.
         */
        FLASH_WRITE(FLASH_BASE_ADDRESS, 0x55, 0x98); /* CFI query */
        if (FLASH_READ(FLASH_BASE_ADDRESS, 0x27) != 0x19) {
            /* 29GL256 series have a device size == 0x19 */
            *vendor = *dev = 0xFF;
        }
        flashReadReset();   
    }
}

LOCAL int
flashEraseDevices(volatile unsigned char *sectorBasePtr)
{
    int             i;
    unsigned int    tmp;

    if (flashVerbose) {
        printf("Erasing Sector @ 0x%08x\n",(unsigned int)sectorBasePtr);
    }
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0xaa);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x2aa, 0x55);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0x80);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0xaa);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x2aa, 0x55);
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

    flashReadReset();
    return (ERROR);
}

LOCAL int
flashEraseSector(int sectorNum)
{
    unsigned char   *sectorBasePtr =
    (unsigned char *)FLASH_SECTOR_ADDRESS(sectorNum);

    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashEraseSector(): Sector %d invalid\n", sectorNum);
        return (ERROR);
    }

    if (flashEraseDevices(sectorBasePtr) == ERROR) {
        printf("flashEraseSector(): erase devices failed sector=%d\n",
               sectorNum);
        return (ERROR);
    }

     if (flashVerbose)
        printf("flashEraseSector(): Sector %d erased\n", sectorNum);

    return (OK);
}

LOCAL int
flashRead(int sectorNum, char *buff, unsigned int offset, unsigned int count)
{
    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashRead(): Illegal sector %d\n", sectorNum);
        return (ERROR);
    }

    bcopy((char *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset), buff, count);

    return (0);
}


LOCAL int
flashProgramDevices(volatile unsigned short *addr, unsigned short val)
{
    int             polls;
    unsigned char    tmp;

    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0xaa);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x2aa, 0x55);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0xa0);
    /* FLASH_WRITE(addr, 0x0, val);*/
    *addr = val;

    for (polls = 0; polls < FLASH_PROGRAM_TIMEOUT_POLLS; polls++) {
        tmp = *addr;

        if ((tmp & 0x80) == (val & 0x80)) {
            if (flashVerbose > 2)
                printf("flashProgramDevices(): devices programmed\n");
            return (OK);
        }
    }

    if ((tmp & 0x20) != 0) {
    /* 
     * We've already waited so long that chances are nil that the
     * 0x80 bits will change again.  Don't bother re-checking them.
     */
        printf("flashProgramDevices(): Address 0x%08x program failed\n",
               (int)addr);
    } else {
        printf("flashProgramDevices(): timed out\n");
    }

    flashReadReset();
    return (ERROR);
}

LOCAL int
flashWrite(int sectorNum, char *buff, unsigned int offset, unsigned int count)
{
    unsigned short *curBuffPtr, *flashBuffPtr;
    int             i;

    curBuffPtr = (unsigned short *)buff;
    flashBuffPtr = (unsigned short *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);

    count = (count + 1)/2;
    for (i = 0; i < count; i++) {
        if (flashProgramDevices(flashBuffPtr, *curBuffPtr) == ERROR) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
               sectorNum, (int)flashBuffPtr);
            return (ERROR);
        }

        flashBuffPtr++;
        curBuffPtr++;
    }

    return (0);
}

struct flash_drv_funcs_s flash29GL256 = {
    FLASH_29GL256, AMD,
    flashAutoSelect,
    flashEraseSector,
    flashRead,
    flashWrite
};

