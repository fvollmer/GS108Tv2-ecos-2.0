#ifndef FLASH_DRV_LIB_H
#define FLASH_DRV_LIB_H

#define LOCAL static
#define OK 0
#define ERROR -1

#define FLASH_BASE_ADDRESS_PLCC_BOOT    0xBFC00000
#define FLASH_BASE_ADDRESS_FLASH_BOOT   0xBFC00000
#define FLASH_BASE_ADDRESS_ALIAS        0xBC000000

#define FLASH_DEVICE_COUNT              1
#define FLASH_DEVICE_SECTOR_SIZE        flashDevSectorSize

#define FLASH_SECTOR_SIZE (FLASH_DEVICE_COUNT * FLASH_DEVICE_SECTOR_SIZE)

#define FLASH_SIZE                  flashSize
#define FLASH_SIZE_SECTORS          (FLASH_SIZE / FLASH_SECTOR_SIZE)

#define FLASH_BASE_ADDRESS  flashBaseAddress
#define FLASH_SECTOR_ADDRESS(sector) \
    (FLASH_BASE_ADDRESS + (sector) * FLASH_SECTOR_SIZE)
#define FLASH_SECTOR_NUMBER(address) \
    ((address-FLASH_BASE_ADDRESS )/FLASH_SECTOR_SIZE)
#define FLASH_ERASE_TIMEOUT_COUNT   750
#define FLASH_ERASE_TIMEOUT_TICKS   2

#define FLASH_PROGRAM_TIMEOUT_POLLS 100000
#define FLASH_PROGRAM_TIMEOUT_TICKS 0

typedef enum {
    AMD = 0x01,
    ALLIANCE = 0x52,
    INTEL = 0x89,
    MXIC = 0xc2
} FLASH_VENDORS;

typedef enum {
    FLASH_2F128 = 0x18,
    FLASH_2F640 = 0x17,
    FLASH_2F320 = 0x16,
    FLASH_2F040 = 0xA4,
    FLASH_2F080 = 0xD5,
    FLASH_2L081 = 0x38,
    FLASH_2L160 = 0x49,
    FLASH_2L320 = 0xF9,
    FLASH_2L640 = 0x7E,
    FLASH_2L017 = 0xC8,
    FLASH_2L800 = 0x5B,
    FLASH_29GL256 = 0x227E,
    FLASH_S29GL128P = 0x2101,
    FLASH_S29GL256P = 0x2201,
    FLASH_S29GL512P = 0x2301,
    FLASH_S29GL01GP = 0x2801,
    FLASH_25L12845 =0x18
} FLASH_TYPES;

struct flash_drv_funcs_s {
    FLASH_TYPES dev;
    FLASH_VENDORS vendor;
    void (*flashAutoSelect)(FLASH_TYPES *dev, FLASH_VENDORS *vendor);
    int (*flashEraseSector)(int sectorNum);
    int (*flashRead)(int sectorNum, char *buff,
          unsigned int offset, unsigned int count);
    int (*flashWrite)(int sectorNum, char *buff,
                unsigned int offset, unsigned int count);
    int (*flashFlushLoadedSector)(void);
};

extern int             flashVerbose; /* DEBUG */
extern int             flashSectorCount;
extern int             flashDevSectorSize;
extern int             flashSize;
extern unsigned int    flashBaseAddress;
extern struct flash_drv_funcs_s flash28f320;
extern struct flash_drv_funcs_s flash28f640;
extern struct flash_drv_funcs_s flash29l160;
extern struct flash_drv_funcs_s flash29l320;
extern struct flash_drv_funcs_s flash29l640;
extern struct flash_drv_funcs_s flash29l800;
extern struct flash_drv_funcs_s flash29GL256;
extern struct flash_drv_funcs_s flashs29gl256p;
extern struct flash_drv_funcs_s flash25L12845;
int             flashDrvLibInit(void);
int             flashGetSectorCount(void);
int             flashEraseBank(int firstSector, int nSectors);
int             flashBlkRead(int sectorNum, char *buff,
                unsigned int offset, unsigned int count);
int             flashBlkWrite(int sectorNum, char *buff,
                unsigned int offset, unsigned int count);
int             flashSyncFilesystem(void);
int             flashDiagnostic(void);

/* 
 * To access flash memory via windowed address (eg. 0xbc000000), 
 * flash driver must be locked. 
 */
void flashDrvLock(void);
void flashDrvUnlock(void);

#endif /* FLASH_DRV_LIB_H */

