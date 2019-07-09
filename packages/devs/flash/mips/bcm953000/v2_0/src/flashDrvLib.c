/*
 * $Id: flashDrvLib.c,v 1.5 2009/05/20 10:23:41 cchao Exp $
 * $Copyright: (c) 2001-2003, 2004 Broadcom Corp.
 * All Rights Reserved.$
 *
 * File:    flashDrvLib.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <cyg/hal/osl.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/siutils.h>
#include <cyg/hal/sbchipc.h>

#include <cyg/io/flashDrvLib.h>

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/siutils_priv.h> 

#define TOTAL_LOADED_SECS  3

int             flashVerbose = 0; /* DEBUG */
unsigned int    flashBaseAddress = 0xBC000000;
int             flashSize = 0x800000;
int             flashDevSectorSize = 0x10000;
int             flashSectorCount = 0;
LOCAL struct flash_drv_funcs_s *flashDrvFuncs = &flash28f320;

/* Flash driver functions should be thread-safe. */
LOCAL cyg_mutex_t *mutex_flashdrv = NULL;


static si_t *ksFlash_sih = NULL;
static chipcregs_t *cc;

#ifdef BUFFER_FLASH_SECTOR
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


#ifdef BUFFER_FLASH_SECTOR
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

int
flashDrvLibInit(void)
{
    FLASH_TYPES     dev;
    FLASH_VENDORS   vendor;
    int i;
diag_printf(" A1");                
    static si_t *ksFlash_sih = NULL;
    static chipcregs_t *cc;
    si_info_t *sii;
    
    ksFlash_sih = si_kattach(SI_OSH);
    sii = SI_INFO(ksFlash_sih);    
    
    cc = (chipcregs_t *)si_setcore(ksFlash_sih, CC_CORE_ID, 0); 

    if ((R_REG(osh, &cc->chipstatus) & 0x02) == 0x02)  
    {

        diag_printf("Serial flash present\n");
      /* Serial flash is boot device*/

        flashDrvFuncs = &flash25L12845;
        flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        flashSectorCount = 256;
        flashDevSectorSize = (64 * 1024);
        if (flashVerbose)
              printf("flashInit(): 25L12845  Found\n");


    }
    else
    {
    flashBaseAddress = FLASH_BASE_ADDRESS_ALIAS;
    
    flashDrvFuncs = &flashs29gl256p;

    flashDrvFuncs->flashAutoSelect(&dev, &vendor);
diag_printf(" ven %x dev %x ", vendor, dev);
    if ((vendor == 0xFF) && (dev == 0xFF)) {
    
      for(i = 0; i < 2; i++) {
        flashDrvFuncs = &flash29l160;
        flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash28f320;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash28f640;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash29l320;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash29l640;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash29l800;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashBaseAddress = FLASH_BASE_ADDRESS_FLASH_BOOT;
        } else {
diag_printf(" A2"); 
            break;
        }
      }
    }
    switch (vendor) {
        case AMD:
        case ALLIANCE:
        case MXIC:
        switch (dev) {
            case FLASH_2F040:
                flashSectorCount = 8;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    printf("flashInit(): 2F040 Found\n");
                break;
            case FLASH_2F080:
                flashSectorCount = 16;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    printf("flashInit(): 2F080 Found\n");
                break;

            case FLASH_2L081:
                flashSectorCount = 16;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    printf("flashInit(): 29LV081B Found\n");
                break;
            case FLASH_2L800:
                flashSectorCount = 16;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    printf("flashInit(): 2F800 Found\n");
                break;

            case FLASH_2L160:
            case FLASH_2L017:
                flashSectorCount = 32;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    printf("flashInit(): 29LV160D Found\n");
                break;

            case FLASH_2L320:
                flashSectorCount = 64;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    printf("flashInit(): 29LV320D Found\n");
                break;

            case FLASH_2L640:
                flashSectorCount = 128;
                flashDevSectorSize = 0x10000;
                if (flashVerbose)
                    printf("flashInit(): 29LV640M Found\n");
                break;
            
            case FLASH_29GL256:
                flashSectorCount = 256;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    printf("flashInit(): 29GL256 Found\n");
                break;
            
            case FLASH_S29GL128P:
                flashSectorCount = 128;
                flashDevSectorSize = 0x20000;
 					diag_printf("flashInit(): FLASH_S29GL128P Found\n");
                if (flashVerbose)
                    diag_printf("flashInit(): FLASH_S29GL128P Found\n");
                break;
                
            case FLASH_S29GL256P:
                flashSectorCount = 256;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    printf("flashInit(): S29GL256P Found\n");
                break;
                
            case FLASH_S29GL512P:
                flashSectorCount = 512;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    printf("flashInit(): FLASH_S29GL512P Found\n");
                break;
                
            case FLASH_S29GL01GP:
                flashSectorCount = 1024;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    printf("flashInit(): FLASH_S29GL01GP Found\n");
                break;
                
            default:
                printf("flashInit(): Unrecognized Device (0x%02X)\n", dev);
                return (ERROR);
        }
        break;

        case INTEL:
        switch (dev) {
            case FLASH_2F128:
                flashSectorCount = 128;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    printf("flashInit(): 28F128 Found\n");
                break;
            case FLASH_2F640:
                flashSectorCount = 64;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    printf("flashInit(): 28F640 Found\n");
                break;
            case FLASH_2F320:
                flashSectorCount = 32;
                flashDevSectorSize = 0x20000;
                if (flashVerbose)
                    printf("flashInit(): 28F320 Found\n");
                break;

            default:
                printf("flashInit(): Unrecognized Device (0x%02X)\n", dev);
                return (ERROR);
        }
        break;
        default:
            printf("flashInit(): Unrecognized Vendor (0x%02X)\n", vendor);
            return (ERROR);
    }

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
#endif
#ifdef BUFFER_FLASH_SECTOR
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
#if 0
return 0;/*   YRDR YRDR  REMOVE IT TESTING ONLY *************************************************************************************/
#endif     
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

#ifdef BUFFER_FLASH_SECTOR
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
    int             i,rc = OK;

    flashBuffPtr = (unsigned char *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);

    for (i = 0; i < count; i++) {
        if (flashBuffPtr[i] != 0xff) {
            if (flashVerbose)
	   diag_printf("\r\n sector not formatted,so erase before writing");

            rc = ERROR;
						break;
        }
    }
    if(ERROR == rc)
    {
	    if (flashVerbose)
      diag_printf("\n erase sectorNum:%d", sectorNum);
      flashEraseBank(sectorNum,1);
	    rc = OK;
	  }
    return (OK);
}

int
flashBlkWrite(int sectorNum, char *buff,
          unsigned int offset, unsigned int count)
{
    char *save;
    int i;
#if 0
return count;/*   YRDR YRDR  REMOVE IT TESTING ONLY *************************************************************************************/
#endif
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
#ifdef  BUFFER_FLASH_SECTOR
    /*** Critical Section BEGIN ***/
    flashDrvLock();

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

    if (flashCheckCanProgram(sectorNum, offset, count) != ERROR) {
#ifndef BUFFER_FLASH_SECTOR    
        flashDrvLock();
#endif
        int r = flashDrvFuncs->flashWrite(sectorNum, buff, offset, count);
#ifdef BUFFER_FLASH_SECTOR    
        /*** Critical Section END ***/
    flashDrvUnlock();
        
        return r;
#endif
		}
#ifdef BUFFER_FLASH_SECTOR     
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
        printf("flashBlkWrite(): load %d (and write to cache only)\n", sectorNum);
        printf("%d ", sectorNum);
    }

    return (OK);
}

#ifdef BUFFER_FLASH_SECTOR     
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
