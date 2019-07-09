#ifndef	FLASH_DRV_LIB_H
#define	FLASH_DRV_LIB_H

#define LOCAL static
#define OK 0
#define ERROR -1

#define	FLASH_BASE_ADDRESS_PLCC_BOOT    0xBFC00000
#define FLASH_BASE_ADDRESS_FLASH_BOOT   0xBb000000
#define FLASH_BASE_ADDRESS_ALIAS        0xBc000000

#define	FLASH_DEVICE_COUNT              1
#define FLASH_DEVICE_SECTOR_SIZE        flashDevSectorSize

#define FLASH_SECTOR_SIZE (FLASH_DEVICE_COUNT * FLASH_DEVICE_SECTOR_SIZE)

#define FLASH_SIZE                  flashSize
#define FLASH_SIZE_SECTORS          (FLASH_SIZE / FLASH_SECTOR_SIZE)

#define FLASH_BASE_ADDRESS	flashBaseAddress
#define FLASH_SECTOR_ADDRESS(sector) \
    (FLASH_BASE_ADDRESS + (sector) * FLASH_SECTOR_SIZE)
#define FLASH_SECTOR_NUMBER(address) \
    ((address-FLASH_BASE_ADDRESS )/FLASH_SECTOR_SIZE)
#define	FLASH_ERASE_TIMEOUT_COUNT	750
#define	FLASH_ERASE_TIMEOUT_TICKS	2

#define	FLASH_PROGRAM_TIMEOUT_POLLS	100000
#define	FLASH_PROGRAM_TIMEOUT_TICKS	0

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
    FLASH_2L081 = 0x38,
    FLASH_2L160 = 0x49,
    FLASH_2L320 = 0xF9,
    FLASH_2L640 = 0x7E,
    FLASH_2L800 = 0x5B,
    FLASH_28F640P33T = 0x881D,
    FLASH_28F128P33T = 0x881E,
    FLASH_28F256P33T = 0x881F,
    FLASH_28F640P33B = 0x8820,
    FLASH_28F128P33B = 0x8821,
    FLASH_28F256P33B = 0x8822,
    FLASH_29GL016T = 0x22C4,  /** TOP Boot **/
    FLASH_29GL016B = 0x2249, /** Bottom Boot **/
    FLASH_29GL032 = 0x221D,
    FLASH_29GL06402 = 0x220C, /* For Version 01, 02 */
    FLASH_29GL06404 = 0x2210, /* For Version 03, 04 */
    FLASH_29GL06407 = 0x2213, /* For Version 06, 07 */
    FLASH_29GL128 = 0x2221,
    FLASH_29GL256 = 0x2222,
    FLASH_29GL512 = 0x2223,
    FLASH_29GL01G = 0x2228,
		FLASH_29LV320DT = 0x22F6, /* 2MB, 0x10000, 32 */
		FLASH_29LV320DB = 0x22F9, /* 4MB, 0x10000, 64 */
		FLASH_29LV640D  = 0x22D7,  /* 8MB,0x10000,128 */
		FLASH_29LV128D  = 0x2212  /* 16MB,0x10000,256 */
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
extern struct flash_drv_funcs_s flash28f128p33t;
extern struct flash_drv_funcs_s flash29lxxx;
extern struct flash_drv_funcs_s flash29GLxxx;


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

