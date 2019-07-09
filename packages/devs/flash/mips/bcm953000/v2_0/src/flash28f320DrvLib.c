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

#define FLASH_16BIT 0

#define xor_val 0
/*
 * INTEL Flash commands
 */
#define INTEL_FLASH_READ_MODE     0xFF
#define INTEL_FLASH_ERASE_BLOCK   0x20
#define INTEL_FLASH_ERASE_CONFIRM 0xD0
#define INTEL_FLASH_PROGRAM       0x40
#define INTEL_FLASH_RD_SR         0x70
#define INTEL_FLASH_CLR_SR        0x50
#define INTEL_FLASH_WB            0xE8
#define INTEL_FLASH_WB_CONFIRM    0xD0
#if FLASH_16BIT
#define INTEL_FLASH_WBSIZE        16
#else
#define INTEL_FLASH_WBSIZE        32 
#endif

#define FLASH_ADDR(dev, addr) \
    ((volatile cyg_uint8 *) (((int)(dev) + (int)(addr)) ^ xor_val))

#define FLASH_WRITE(dev, addr, value) \
    (*FLASH_ADDR(dev, addr) = (value))

#define FLASH_READ(dev, addr) \
    (*FLASH_ADDR(dev, addr))

LOCAL void
flashReadReset(void)
{
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x000, INTEL_FLASH_READ_MODE);
}

LOCAL void
flashAutoSelect(FLASH_TYPES *dev, FLASH_VENDORS *vendor)
{
    flashReadReset();
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x000, 0x90);

    *vendor = FLASH_READ(FLASH_BASE_ADDRESS, 0);
    *dev = FLASH_READ(FLASH_BASE_ADDRESS, 2);

    if (flashVerbose) {
        printf("\nflashAutoSelect(): dev = 0x%x, vendor = 0x%x\n",
               (int)*dev, (int)*vendor);
    }
    flashReadReset();
    if (((*dev != FLASH_2F320) &&
         (*dev != FLASH_2F640) &&
         (*dev != FLASH_2F128)) ||
        (*vendor != INTEL)) {
        *vendor = *dev = 0xFF;
    }
}

LOCAL int
flashEraseDevices(volatile cyg_uint8 *sectorBasePtr)
{
    int             i;
    unsigned int    status = 0;

    if (flashVerbose) {
        diag_printf("Erasing Sector @ 0x%08x\n",(cyg_uint32)sectorBasePtr);
    }
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x0, INTEL_FLASH_READ_MODE);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x0, INTEL_FLASH_CLR_SR);
    FLASH_WRITE(sectorBasePtr, 0x000, INTEL_FLASH_ERASE_BLOCK);
    FLASH_WRITE(sectorBasePtr, 0x000, INTEL_FLASH_ERASE_CONFIRM);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x0, INTEL_FLASH_RD_SR);

    for (i = 0; i < FLASH_ERASE_TIMEOUT_COUNT; i++) {
           cyg_thread_delay((cyg_tick_count_t)FLASH_ERASE_TIMEOUT_TICKS);
        status = FLASH_READ(FLASH_BASE_ADDRESS, 0x0);

        if ((status & 0x80) == 0x80) {
            break;
        }
    }

    flashReadReset();
    if ((status & 0xa0) != 0x80) {
        printf("flashEraseDevices(): addr 0x%08x erase failed\n",
           (int)sectorBasePtr);
        return (ERROR);
    }
    return (OK);
}

LOCAL int
flashEraseSector(int sectorNum)
{
    cyg_uint8 *sectorBasePtr = (cyg_uint8 *)FLASH_SECTOR_ADDRESS(sectorNum);

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
flashRead(int sectorNum, char *buff,
    unsigned int offset, unsigned int count)
{
    flashReadReset();
    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashRead(): Illegal sector %d\n", sectorNum);
        return (ERROR);
    }

    bcopy((char *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset), buff, count);

    return (0);
}

LOCAL int
#if FLASH_16BIT
flashProgramBlock(volatile cyg_uint16 *addr, cyg_uint16 *val)
#else
flashProgramBlock(volatile cyg_uint8 *addr, cyg_uint8 *val)
#endif
{
    int                 polls;
    int                 i;
    unsigned char       status;

    FLASH_WRITE(addr, 0x000, INTEL_FLASH_WB);
    /*
     * XSR7 transitions to a 1, the buffer is ready for loading.
     */
    while (((status = FLASH_READ(FLASH_BASE_ADDRESS, 0x0)) & 0x80) == 0) {
        cyg_thread_delay((cyg_tick_count_t)FLASH_PROGRAM_TIMEOUT_TICKS);
    }
    FLASH_WRITE(addr, 0x000, INTEL_FLASH_WBSIZE -  1);
    for(i = 0; i < INTEL_FLASH_WBSIZE; i++) {
#if FLASH_16BIT
        addr[i ^ xor_val] = val[i ^ xor_val];
#else
        FLASH_WRITE(addr, i, val[i ^ xor_val]);
#endif
    }
    FLASH_WRITE(addr, 0x000, INTEL_FLASH_WB_CONFIRM);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x000, INTEL_FLASH_RD_SR);

    for (polls = 0; polls < FLASH_PROGRAM_TIMEOUT_POLLS; polls++) {
        status = FLASH_READ(FLASH_BASE_ADDRESS, 0x0);

        if ((status & 0x80) == 0x80) {
            break;
        }
        cyg_thread_delay((cyg_tick_count_t)FLASH_PROGRAM_TIMEOUT_TICKS);
    }

    if ((status & 0x90) != 0x80) {
        printf("flashProgramDevices(): Address 0x%08x program failed\n",
               (int)addr);
        return (ERROR);
    } else {
        return (OK);
    }

}

LOCAL int
#if FLASH_16BIT
flashProgramDevices(volatile cyg_uint16 *addr, cyg_uint16 val)
#else
flashProgramDevices(volatile cyg_uint8 *addr, cyg_uint8 val)
#endif
{
    int             polls;
    unsigned char    status;

    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x000, INTEL_FLASH_CLR_SR);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x000, INTEL_FLASH_READ_MODE);
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x000, INTEL_FLASH_PROGRAM);
    *addr = val; /* FLASH_WRITE(addr, 0x0, val);*/
    FLASH_WRITE(FLASH_BASE_ADDRESS, 0x000, INTEL_FLASH_RD_SR);

    for (polls = 0; polls < FLASH_PROGRAM_TIMEOUT_POLLS; polls++) {
        status = FLASH_READ(FLASH_BASE_ADDRESS, 0x0);

        if ((status & 0x80) == 0x80) {
            break;
        }
        cyg_thread_delay((cyg_tick_count_t)FLASH_PROGRAM_TIMEOUT_TICKS);
    }

    flashReadReset();
    if ((status & 0x90) != 0x80) {
        printf("flashProgramDevices(): Address 0x%08x program failed\n",
               (int)addr);
        return (ERROR);
    } else {
        return (OK);
    }

}

LOCAL int
flashWrite(int sectorNum, char *buff,
            unsigned int offset, unsigned int count)
{
    int             i;
#if FLASH_16BIT
    cyg_uint16  *curBuffPtr, *flashBuffPtr;

    curBuffPtr = (cyg_uint16 *)buff;
    flashBuffPtr = (cyg_uint16 *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);

    for (i = 0; i < (count+1)/2; i++) {
        if (((offset + i) % INTEL_FLASH_WBSIZE) == 0) {
            break;
        }
        if (flashProgramDevices(flashBuffPtr, *curBuffPtr) == ERROR) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
               sectorNum, (int)flashBuffPtr);
            return (ERROR);
        }

        flashBuffPtr++; 
        curBuffPtr++;
    }
    for (; i < (count+1)/2; i += INTEL_FLASH_WBSIZE) {
        if ((i + INTEL_FLASH_WBSIZE) >= (count+1)/2) {
            break;
        }
        if (flashProgramBlock(flashBuffPtr, curBuffPtr) != OK) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
               sectorNum, (int)flashBuffPtr);
            return (ERROR);
        }
        flashBuffPtr += INTEL_FLASH_WBSIZE;
        curBuffPtr += INTEL_FLASH_WBSIZE;
    }
    for (; i < (count+1)/2; i++) {
        if (flashProgramDevices(flashBuffPtr, *curBuffPtr) == ERROR) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
               sectorNum, (int)flashBuffPtr);
            return (ERROR);
        }

        flashBuffPtr++;
        curBuffPtr++;
    }
#else
    cyg_uint8   *curBuffPtr, *flashBuffPtr;

    curBuffPtr = (cyg_uint8 *)buff;
    flashBuffPtr = (cyg_uint8 *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);

    for (i = 0; i < count; i++) {
        if (((offset + i) % INTEL_FLASH_WBSIZE) == 0) {
            break;
        }
        if (flashProgramDevices(flashBuffPtr, *curBuffPtr) == ERROR) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
               sectorNum, (int)flashBuffPtr);
            return (ERROR);
        }

        flashBuffPtr++;
        curBuffPtr++;
    }
    for (; i < count; i += INTEL_FLASH_WBSIZE) {
        if ((i + INTEL_FLASH_WBSIZE) >= count) {
            break;
        }
        if (flashProgramBlock(flashBuffPtr, curBuffPtr) != OK) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
               sectorNum, (int)flashBuffPtr);
            return (ERROR);
        }
        flashBuffPtr += INTEL_FLASH_WBSIZE;
        curBuffPtr += INTEL_FLASH_WBSIZE;
    }
    for (; i < count; i++) {
        if (flashProgramDevices(flashBuffPtr, *curBuffPtr) == ERROR) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
               sectorNum, (int)flashBuffPtr);
            return (ERROR);
        }

        flashBuffPtr++;
        curBuffPtr++;
    }
#endif
    return (0);
}

struct flash_drv_funcs_s flash28f320 = {
    FLASH_2F320, INTEL,
    flashAutoSelect,
    flashEraseSector,
    flashRead,
    flashWrite
};

struct flash_drv_funcs_s flash28f640 = {
    FLASH_2F640, INTEL,
    flashAutoSelect,
    flashEraseSector,
    flashRead,
    flashWrite
};
