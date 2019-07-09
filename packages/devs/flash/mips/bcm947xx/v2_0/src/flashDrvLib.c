/*
 * $Id: flashDrvLib.c,v 1.3.2.2.2.2 Exp $
 * $Copyright: Copyright 2006, Broadcom Corporation All Rights Reserved.
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES
 * OF ANY KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.
 * BROADCOM SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.$
 *
 * File:    flashDrvLib.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <cyg/io/flashDrvLib.h>

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>

#define TOTAL_LOADED_SECS  3

int             flashVerbose = 0; /* DEBUG */
unsigned int    flashBaseAddress = 0xBC000000;
int             flashSize = 0x400000;
int             flashDevSectorSize = 0x10000;
int             flashSectorCount = 64;
LOCAL struct flash_drv_funcs_s *flashDrvFuncs = &flash28f320;

/* Flash driver functions should be thread-safe. */
LOCAL cyg_mutex_t *mutex_flashdrv = NULL;

#if 0
LOCAL struct flashLoadedSectorInfo {
    cyg_mutex_t  fsSemID;
    int    sector;
    int    dirty;
    char  *buffer;
} flashLoadedSectors[TOTAL_LOADED_SECS];

#define FS_CACHE_LOCK(_x_) \
    cyg_mutex_lock(&(flashLoadedSectors[(_x_)].fsSemID))
    //semTake(flashLoadedSectors[(_x_)].fsSemID, WAIT_FOREVER)
#define FS_CACHE_UNLOCK(_x_) \
    cyg_mutex_unlock(&(flashLoadedSectors[(_x_)].fsSemID))
    //semGive(flashLoadedSectors[(_x_)].fsSemID)
#endif

void
flashDrvLock(void)
{
    if (mutex_flashdrv) {
        cyg_mutex_lock(mutex_flashdrv);
    }
}

void
flashDrvUnlock(void)
{
    if (mutex_flashdrv) {
        cyg_mutex_unlock(mutex_flashdrv);
    }
}

#if 0
LOCAL int
flashFlushLoadedSector(int number, int reason)
{
    if (flashLoadedSectors[number].sector < 0 ||
        !flashLoadedSectors[number].dirty) {
        if (flashVerbose)
            printf("flashFlushLoadedSector(%d): not dirty\n", number);
        return (OK);
    }

    if (flashVerbose) {
        printf("flashFlushLoadedSector(%d): Flushing %d %s\n", number,
                flashLoadedSectors[number].sector,
                (reason == 0 ? "<- flashSync" : "<- flashBlkWrite"));
    }

    if (flashDrvFuncs->flashEraseSector(flashLoadedSectors[number].sector)==ERROR) {
        return (ERROR);
    }

    if (flashDrvFuncs->flashWrite(flashLoadedSectors[number].sector,
            flashLoadedSectors[number].buffer, 0, FLASH_SECTOR_SIZE) == ERROR) {
        return (ERROR);
    }

    if (flashVerbose) {
        printf("                           Flushing %d done\n",
                flashLoadedSectors[number].sector);
    }

    flashLoadedSectors[number].sector = -1;
    flashLoadedSectors[number].dirty = 0;

    return (OK);
}
#endif


struct flash_drv_funcs_s *flash_drv_funcs_list[] = {
    &flash28f128m29ewh,
    &flash29GL128,
    &flash28f320,
    &flash28f640,
    &flash28f128p33t,
    &flash29lxxx,
    &flash29GLxxx
};

int flash_drv_funcs_list_size = sizeof(flash_drv_funcs_list) / sizeof(struct flash_drv_funcs_s *);


int
flashDrvLibInit(void)
{
    FLASH_TYPES     dev;
    FLASH_VENDORS   vendor;
    int i;

diag_printf(" in flashDrvLibInit 47xx\n");				
flashBaseAddress = FLASH_BASE_ADDRESS_ALIAS;

    i=0;
    vendor = 0xFF;
    dev = 0xFF;
    while ( (i<flash_drv_funcs_list_size) && (vendor == 0xFF) && (dev == 0xFF) ) {
        flashDrvFuncs = flash_drv_funcs_list[i];
    flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        i++;
    }
    switch (vendor) {
        case AMD:
        case ALLIANCE:
        case MXIC:
        switch (dev) {
            case FLASH_2L800:
                flashSectorCount = 16;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    diag_printf("flashInit(): 2F800 Found\n");
                break;

            case FLASH_2L160:
                flashSectorCount = 32;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29LV160D Found\n");
                break;

            case FLASH_2L320:
                flashSectorCount = 64;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29LV320D Found\n");
                break;

            case FLASH_2L640:
                flashSectorCount = 128;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29LV640M Found\n");
                break;
            
            case FLASH_29GL016T:
            case FLASH_29GL016B:
                flashSectorCount = 32;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29GL016AT/B Found\n");
                break;

            case FLASH_29GL032:
            case FLASH_29LV320DT:
            case FLASH_29LV320DB:
                flashSectorCount = 64;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29GL032P Found\n");
                break;

            case FLASH_29GL06402:
            case FLASH_29GL06404:
            case FLASH_29GL06407:
            case FLASH_29LV640D:
                flashSectorCount = 128;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29GL064P Found\n");
                break;

            case FLASH_29GL128:
            case FLASH_29LV128D:
                flashSectorCount = 128;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29GL128 Found\n");
                break;

            case FLASH_29GL256:
                flashSectorCount = 256;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29GL256 Found\n");
                break;

            case FLASH_29GL512:
                flashSectorCount = 512;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29GL512 Found\n");
                break;
            
            case FLASH_29GL01G:
                flashSectorCount = 1024;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29GL01G Found\n");
                break;

            default:
                diag_printf("flashInit(): Unrecognized Device (0x%02X)\n", dev);
                return (ERROR);
        }
        break;

        case INTEL:
        switch (dev) {
            case FLASH_28F128P33T:
                flashSectorCount = 127;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 28F128 Found\n");
                break;
            case FLASH_29GL128:
                flashSectorCount = 128;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 29GL128 Found\n");
                break;
            case FLASH_2F128:
                flashSectorCount = 128;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 28F128 Found\n");
                break;
            case FLASH_2F640:
                flashSectorCount = 64;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 28F640 Found\n");
                break;
            case FLASH_2F320:
                flashSectorCount = 32;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    diag_printf("flashInit(): 28F320 Found\n");
                break;

            default:
                diag_printf("flashInit(): Unrecognized Device (0x%02X)\n", dev);
                return (ERROR);
        }
        break;
        default:
            diag_printf("flashInit(): Unrecognized Vendor (0x%02X)\n", vendor);
            return (ERROR);
    }


    flashSize = flashDevSectorSize * flashSectorCount;

#if 0
    if (flashBaseAddress == FLASH_BASE_ADDRESS_FLASH_BOOT && 
        flashSize >= 0x800000) {
        /* 
         * Only 8 MB of the flash is visible through this window, so we
         * have to drop the flash size and sector count accordingly.
         */
        flashSize = 0x800000;
        flashSectorCount = flashSize / flashDevSectorSize;
    }
    for (i = 0; i < TOTAL_LOADED_SECS; i++) {
        flashLoadedSectors[i].buffer = malloc(FLASH_SECTOR_SIZE);
        if (flashLoadedSectors[i].buffer == NULL) {
            printf("flashInit(): malloc() failed\n");
            for (; i > 0; i--) {
                free(flashLoadedSectors[i-1].buffer);
            }
            return (ERROR);
        }
        flashLoadedSectors[i].sector = -1;
        flashLoadedSectors[i].dirty = 0;
        cyg_mutex_init(&(flashLoadedSectors[i].fsSemID));
        /*
        flashLoadedSectors[i].fsSemID =
            semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);
        */
    }
#endif
    
    /* Initialize mutex for flash access */
    if (mutex_flashdrv == NULL) {
        mutex_flashdrv = malloc(sizeof(cyg_mutex_t));
        if (mutex_flashdrv) {
            cyg_mutex_init(mutex_flashdrv);
        }
    }

    return (OK);
}

int
flashGetSectorCount(void)
{
    return (flashSectorCount);
}

int
flashEraseBank(int firstSector, int nSectors)
{
    int             sectorNum, errCnt = 0;
    
    if (firstSector < 0 || firstSector + nSectors > flashSectorCount) {
        diag_printf("flashEraseBank(): Illegal parms %d, %d\n",
           firstSector, nSectors);
        return ERROR;
    }

    /*** Critical Section BEGIN ***/
    flashDrvLock();
    
    for (sectorNum = firstSector;
         sectorNum < firstSector + nSectors; sectorNum++) {
        if (flashDrvFuncs->flashEraseSector(sectorNum))
            errCnt++;
    }
    
    /*** Critical Section END ***/
    flashDrvUnlock();

    if (errCnt)
        return (ERROR);
    else
        return (OK);
}

int
flashBlkRead(int sectorNum, char *buff,
         unsigned int offset, unsigned int count)
{
    int i;

    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashBlkRead(): Sector %d invalid\n", sectorNum);
        return (ERROR);
    }

    if (offset < 0 || offset >= FLASH_SECTOR_SIZE) {
        printf("flashBlkRead(): Offset 0x%x invalid\n", offset);
        return (ERROR);
    }

    if (count < 0 || count > FLASH_SECTOR_SIZE - offset) {
        printf("flashBlkRead(): Count 0x%x invalid\n", count);
        return (ERROR);
    }

    /*** Critical Section BEGIN ***/
    flashDrvLock();

#if 0
    /*
     * If the sector is loaded, read from it.  Else, read from the
     * flash itself (slower).
     */
    for (i = 0; i < TOTAL_LOADED_SECS; i++) {
        if (flashLoadedSectors[i].sector == sectorNum) {
            if (flashVerbose)
                printf("flashBlkRead(): from loaded sector %d\n", sectorNum);
            bcopy(&flashLoadedSectors[i].buffer[offset], buff, count);

            /*** Critical Section END ***/
            flashDrvUnlock();

            return (OK);
        }
    }
#endif
    flashDrvFuncs->flashRead(sectorNum, buff, offset, count);

    /*** Critical Section END ***/
    flashDrvUnlock();

    return (OK);
}

/*
 * Check if we can program this part of the flash.  All
 * the data has to be all "1" to be programmed.  Because
 * the flash has to be init to all 1's and change from 1 to 0
 */
LOCAL int
flashCheckCanProgram(int sectorNum, unsigned int offset, unsigned int count)
{
    unsigned char   *flashBuffPtr;
    int             i, rc=OK;

    flashBuffPtr = (unsigned char *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);

    for (i = 0; i < count; i++) {
        if (flashBuffPtr[i] != 0xff) {
            rc =ERROR;
			break;
        }
    }

	if(rc ==ERROR)
	{
      if (flashVerbose)
        diag_printf("\n erase sectorNum:%d", sectorNum);
      flashEraseBank(sectorNum,1);
	  rc = OK;
	}

    return (rc);
}

int
flashBlkWrite(int sectorNum, char *buff,
          unsigned int offset, unsigned int count)
{
    char *save;
    int i;

    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashBlkWrite(): Sector %d invalid\n", sectorNum);
        return (ERROR);
    }

    if (offset < 0 || offset >= FLASH_SECTOR_SIZE) {
        printf("flashBlkWrite(): Offset 0x%x invalid\n", offset);
        return (ERROR);
    }

    /* 
     * Count must be within range and must be a long word multiple, as
     * we always program long words.
     */

    if ((count < 0) || 
        (count > ((flashSectorCount - sectorNum) * FLASH_SECTOR_SIZE - offset))) {
        printf("flashBlkWrite(): Count 0x%x invalid\n", count);
        return (ERROR);
    }

    /*** Critical Section BEGIN ***/
    flashDrvLock();

#if 0
    /*
     * If the Sector is loaded, write it to buffer.  Else check to see
     * if we can program the sector; if so, program it.  Else, flush the
     * first loaded sector (if loaded and dirty), push loaded sectors
     * up by one, load the new one and copy the data into the last one.
     */

    for (i = 0; i < TOTAL_LOADED_SECS; i++) {
        FS_CACHE_LOCK(i);
        if (flashLoadedSectors[i].sector == sectorNum) {
            if (flashVerbose)
                printf("%d ", sectorNum);
            bcopy(buff, &flashLoadedSectors[i].buffer[offset], count);
            flashLoadedSectors[i].dirty = 1;
            FS_CACHE_UNLOCK(i);
            
            /*** Critical Section END ***/
            flashDrvUnlock();
            
            return (OK);
        }
        FS_CACHE_UNLOCK(i);
    }
#endif
    flashDrvUnlock();
    flashCheckCanProgram(sectorNum, offset, count);
    flashDrvLock();
    int r = flashDrvFuncs->flashWrite(sectorNum, buff, offset, count);
     
#if 0	
    FS_CACHE_LOCK(0);
    if (flashFlushLoadedSector(0, 1) == ERROR) {
        FS_CACHE_UNLOCK(0);

        /*** Critical Section END ***/
        flashDrvUnlock();

        return (ERROR);
    }
    FS_CACHE_UNLOCK(0);

    save = flashLoadedSectors[0].buffer;
    for (i = 1; i < TOTAL_LOADED_SECS; i++) {
        FS_CACHE_LOCK(i-1);
        flashLoadedSectors[i-1].sector = flashLoadedSectors[i].sector;
        flashLoadedSectors[i-1].dirty  = flashLoadedSectors[i].dirty;
        flashLoadedSectors[i-1].buffer = flashLoadedSectors[i].buffer;
        FS_CACHE_UNLOCK(i-1);
    }
    i--;
    flashLoadedSectors[i].buffer = save;

    FS_CACHE_LOCK(i);
    if (flashDrvFuncs->flashRead(sectorNum, flashLoadedSectors[i].buffer,
                                 0, FLASH_SECTOR_SIZE) == ERROR) {
        flashLoadedSectors[i].sector = -1;
        FS_CACHE_UNLOCK(i);

        /*** Critical Section END ***/
        flashDrvUnlock();

        return (ERROR);
    }

    flashLoadedSectors[i].sector = sectorNum;
    bcopy(buff, &flashLoadedSectors[i].buffer[offset], count);
    flashLoadedSectors[i].dirty = 1;
    FS_CACHE_UNLOCK(i);
#endif
    /*** Critical Section END ***/
    flashDrvUnlock();

    if (flashVerbose) {
        printf("flashBlkWrite(): write complete to sector %d\n", sectorNum);
    }

    return (OK);
}

#if 0
int
flashDiagnostic(void)
{
    unsigned int   *flashSectorBuff;
    int             sectorNum, i;

    /*
     * Probe flash; allocate flashLoadedSector Buffer
     */

    flashDrvLibInit();        /* Probe; clear loaded sector */

    flashSectorBuff = (unsigned int *) flashLoadedSectors[0].buffer;

    if (flashVerbose)
        printf("flashDiagnostic(): Executing. Erasing %d Sectors\n",
                flashSectorCount);

    if (flashEraseBank(0, flashSectorCount) == ERROR) {
    if (flashVerbose)
        printf("flashDiagnostic(): flashEraseBank() #1 failed\n");

    return (ERROR);
    }

    /* Write unique counting pattern to each sector. */
    for (sectorNum = 0; sectorNum < flashSectorCount; sectorNum++) {
    if (flashVerbose)
        printf("flashDiagnostic(): writing sector %d\n", sectorNum);

    for (i = 0; i < FLASH_SECTOR_SIZE / sizeof (unsigned int); i++)
        flashSectorBuff[i] = (i + sectorNum);

    if (flashDrvFuncs->flashWrite(sectorNum, (char *)flashSectorBuff,
               0, FLASH_SECTOR_SIZE) == ERROR) {
        if (flashVerbose)
        printf("flashDiagnostic(): flashWrite() failed on %d\n",
               sectorNum);

        return (ERROR);
    }
    }

    /* Verify each sector. */
    for (sectorNum = 0; sectorNum < flashSectorCount; sectorNum++) {
    if (flashVerbose)
        printf("flashDiagnostic(): verifying sector %d\n", sectorNum);

    if (flashDrvFuncs->flashRead(sectorNum, (char *)flashSectorBuff,
              0, FLASH_SECTOR_SIZE) == ERROR) {
        if (flashVerbose)
        printf("flashDiagnostic(): flashRead() failed on %d\n",
               sectorNum);

        return (ERROR);
    }

    for (i = 0; i < FLASH_SECTOR_SIZE / sizeof (unsigned int); i++) {
        if (flashSectorBuff[i] != (i + sectorNum)) {
        if (flashVerbose) {
            printf("flashDiagnostic(): verification failed\n");
            printf("flashDiagnostic(): sector %d, offset 0x%x\n",
               sectorNum, (i * sizeof(unsigned int)));
            printf("flashDiagnostic(): expected 0x%x, got 0x%x\n",
               (i + sectorNum), (int)flashSectorBuff[i]);
        }

        return (ERROR);
        }
    }
    }

    if (flashEraseBank(0, flashSectorCount) == ERROR) {
    if (flashVerbose)
        printf("flashDiagnostic(): flashEraseBank() #2 failed\n");

        return (ERROR);
    }

    if (flashVerbose)
        printf("flashDiagnostic(): Completed without error\n");

    return (OK);
}

int
flashSyncFilesystem(void)
{
    int i;

    /*** Critical Section BEGIN ***/
    flashDrvLock();

    for (i = 0; i < TOTAL_LOADED_SECS; i++) {
        FS_CACHE_LOCK(i);
        if (flashFlushLoadedSector(i, 0) != OK) {
            FS_CACHE_UNLOCK(i);

            /*** Critical Section END ***/
            flashDrvUnlock();

            return (ERROR);
        }
        FS_CACHE_UNLOCK(i);
    }

    /*** Critical Section END ***/
    flashDrvUnlock();

    return (OK);
}
#endif

