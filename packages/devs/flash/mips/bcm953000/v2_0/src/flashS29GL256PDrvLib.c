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

#define FLASH_16BIT 1   /* Actually effective only for programming */
#define DELAY() do { int i; for(i=0;i<32768; i++); } while(0)

#define FLASH_ADDR(dev, addr) \
    ((volatile cyg_uint8 *)((int)(dev) + (int)(addr)))

#define FLASH_WRITE(dev, addr, value) \
    (*FLASH_ADDR(dev, addr) = (value))

#define FLASH_READ(dev, addr) \
    (*FLASH_ADDR(dev, addr))

extern struct flash_drv_funcs_s flashs29gl256p;

LOCAL void
flashReadReset(void)
{
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0xaaa, 0xf0);
}

LOCAL void
flashAutoSelect(FLASH_TYPES *dev, FLASH_VENDORS *vendor)
{
    uint32 mask = 0xff; /* 8 bit mode */

    flashReadReset();   

    /* We use 8bit mode when autoselecting for better compatibility */
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0xaaa, 0xaa);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x555, 0x55);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0xaaa, 0x90);
    
    /* Since BCM5300x runs too fast, we need some delay in between */
    DELAY();
    
    *vendor = FLASH_READ(FLASH_BASE_ADDRESS, 0);
    *dev = FLASH_READ(FLASH_BASE_ADDRESS, 2);

    /*
      * If device ID is 0x227e(16 bits) or 0x7e(8 bits), 
      * this is a device with 3-bytes device ID. 
      * Read the other 2 bytes and set those 2-bytes as device ID
      */
    if (((int)*vendor == 1) && (((int)*dev & mask) == (FLASH_29GL256 & mask))) {
        *dev = ((((FLASH_TYPES)FLASH_READ(FLASH_BASE_ADDRESS, 0x1c) & mask) << 8) |
            ((FLASH_TYPES)FLASH_READ(FLASH_BASE_ADDRESS, 0x1e) & mask));
    }
    if (flashVerbose)
        printf("flashAutoSelect(): dev = 0x%x, vendor = 0x%x\n",
               (int)*dev, (int)*vendor);
    flashReadReset();   

    if ((*dev != FLASH_S29GL128P && *dev != FLASH_S29GL256P && 
         *dev != FLASH_S29GL512P && *dev != FLASH_S29GL01GP) ||
        ((*vendor != AMD) && (*vendor != ALLIANCE) && *vendor != MXIC)) {
        *vendor = *dev = 0xFF;
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

    /* We use 8bit mode when autoselecting for better compatibility */
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
#if FLASH_16BIT
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

    *addr = val;

    for (polls = 0; polls < FLASH_PROGRAM_TIMEOUT_POLLS; polls++) {
        tmp = *addr;

#if FLASH_16BIT
        if ((tmp & 0x8080) == (val & 0x8080)) {
#else
        if ((tmp & 0x80) == (val & 0x80)) {
#endif
            if (flashVerbose > 2)
                printf("flashProgramDevices(): devices programmed\n");
            return (OK);
        }
    }

#if FLASH_16BIT
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

    flashReadReset();
    return (ERROR);
}

LOCAL int
flashWrite(int sectorNum, char *buff, unsigned int offset, unsigned int count)
{
#if FLASH_16BIT
    unsigned short *curBuffPtr, *flashBuffPtr;
#else
    unsigned char   *curBuffPtr, *flashBuffPtr;
#endif
    int             i;

#if FLASH_16BIT
    curBuffPtr = (unsigned short *)buff;
    flashBuffPtr = (unsigned short *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);

    for (i = 0; i < (count+1)/2; i++) {
#else
    curBuffPtr = (unsigned char *)buff;
    flashBuffPtr = (unsigned char *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);

    for (i = 0; i < count; i++) {
#endif
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

struct flash_drv_funcs_s flashs29gl256p = {
    FLASH_S29GL256P, AMD,
    flashAutoSelect,
    flashEraseSector,
    flashRead,
    flashWrite
};

