//====================================================================
//
//      nvram_ecos.c
//
//      nvram memory - ecos support for  BCM953000  nvram
//
//====================================================================
//====================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):      
// Original data:  
// Contributors:   
// Date:           
//
//####DESCRIPTIONEND####
//
//====================================================================

#include <pkgconf/system.h>
#include <pkgconf/hal.h>
#include <cyg/io/flash.h>
#include <stdio.h>
#include <stdlib.h>

#include <cyg/hal/typedefs.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/bcmendian.h>
#include <cyg/hal/bcmnvram.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/sbsdram.h>
#include <cyg/hal/siutils.h>
#include <cyg/io/flashDrvLib.h>

static struct nvram_header *nvram_header = NULL;
static int dummy_printf( const char *fmt, ... ) {return 0;}
#ifdef LVL7_FIXUP
#define BCM953000_NVRAM_ADDR_A0         (0xBC000000+BOOT_SIZE+NVRAM_SPACE)
#endif
int nvram_init(void* sbh)
{
#ifndef LVL7_FIXUP
    uint idx;
    int stat;
    uint32 flash_start;
    uint32 flash_end;
    uint32 flash_block_size, flash_num_blocks;
    uint ret;
    uint32 off, lim;
    
    if (nvram_header != NULL) {
        /* Already initialized */
        return 0;
    }

    stat = flash_init(dummy_printf);

    if (stat != 0) {
        diag_printf("FLASH: flash init failed: %s\n", flash_errmsg(stat));
        return false;
    }

    flash_get_limits((void *)0, (void **)&flash_start, (void **)&flash_end);
    flash_get_block_info(&flash_block_size, &flash_num_blocks);

    lim = (flash_end - flash_start) - NVRAM_SPACE;
    off = 0;

    /*** Critical Section BEGIN ***/
    flashDrvLock();

    while (off <= lim) {

        /* Windowed flash access */
        nvram_header = (struct nvram_header *)(flash_start + off);
        
        if (nvram_header->magic == NVRAM_MAGIC) 
            break;

        off += flash_block_size;
    }   
    
    if (nvram_header->magic != NVRAM_MAGIC) {
        printf("nvram_init: cannot find NVRAM partition!\n");
        nvram_header = NULL; /* Indicate init failed */

        /*** Critical Section END ***/
        flashDrvUnlock();

        return FLASH_ERR_INVALID;
    }

    /*** Critical Section END ***/
    flashDrvUnlock();
    
    printf("nvram_init: nvram start address 0x%x and size 0x%x\n",
           nvram_header, NVRAM_SPACE);
    ret = _nvram_init();

    return ret;
#else

  uint ret;
  int stat;
  if (nvram_header != NULL)
  {
    /* Already initialized */
    return 0;
  }

  stat = flash_init(dummy_printf);

 if (stat != 0) {
    diag_printf("FLASH: flash init failed: %s\n", flash_errmsg(stat));
    return false;
 }
   
  /*** Critical Section BEGIN ***/
  flashDrvLock ();

  nvram_header = (struct nvram_header *) (BCM953000_NVRAM_ADDR_A0);

  if (nvram_header->magic != NVRAM_MAGIC)
  {
    diag_printf ("nvram_init: cannot find NVRAM partition!\n");
    nvram_header = NULL;        /* Indicate init failed */

    /*** Critical Section END ***/
    flashDrvUnlock ();

    return FLASH_ERR_INVALID;
  }
    /*** Critical Section END ***/
  flashDrvUnlock ();

  printf("nvram_init: nvram start address 0x%x and size 0x%x\n",
           nvram_header, NVRAM_SPACE);
  ret = _nvram_init ();

  return ret;
#endif
}


void nvram_exit(void *sih)
{
    if (nvram_header == NULL) {
        return;
    }
    
    _nvram_exit();
}


char* nvram_get(const char* name)
{
    char *value;

    if (nvram_header == NULL) {
        return "";
    }
    
    value = _nvram_get(name);

    return value;
}


int nvram_getall(char* buf, int count)
{
    int ret;

    if (nvram_header == NULL) {
        buf[0] = 0;
        /* 
         * Since NVRAM is not used anymore, 
         * we return with empty string instead of error (FLASH_ERR_NOT_INIT).
         * It also aligns with behavior of nvram_get().
         */
        return 0;
    }

    ret = _nvram_getall(buf, count);

    return ret;
}

int nvram_set(const char *name, const char *value)
{
    int ret;

    if (nvram_header == NULL) {
        return FLASH_ERR_NOT_INIT;
    }

    ret = _nvram_set(name, value);

    return ret;
}

int nvram_unset(const char *name)
{
    int ret;

    if (nvram_header == NULL) {
        return FLASH_ERR_NOT_INIT;
    }

    ret = _nvram_unset(name);

    return ret;
}

int nvram_commit(void)
{
    struct nvram_header *header;
    int ret;
    uint32 *src, *dst;
    uint i;
    uint32 err_addr;

    if (nvram_header == NULL) {
        return FLASH_ERR_NOT_INIT;
    }

    if (!(header = (struct nvram_header *) MALLOC(NULL, NVRAM_SPACE))) {
        printf("nvram_commit: out of memory\n");
        return -12;
    }

    ret = _nvram_commit(header);

    if (ret)
        goto done;

    src = (uint32 *) &header[1];
    dst = src;

    for (i = sizeof(struct nvram_header); i < header->len && i < NVRAM_SPACE;
         i += 4)
        *dst++ = htol32(*src++);

    
    ret = flash_erase((void*)nvram_header, header->len, (void**)&err_addr);

    if (ret)
    {
        printf("nvram_commit: Can't erase region at %p\n", err_addr);
        goto done;
    }

    ret = flash_program((void*)nvram_header, (void*)header, header->len,
                        (void**)&err_addr);

    if (ret)
    {
        printf("nvram_commit: Can't program region at %p\n", err_addr);
        goto done;
    }

done:
    MFREE(NULL, header, NVRAM_SPACE);
    return ret;
}

int _nvram_read(void *buf)
{
    volatile uint32 *src, *dst;
    uint i;
    
    src = (uint32 *) nvram_header;
    dst = (uint32 *) buf;

    printf ("_nvram_read: source address is 0x%x 0x%x\n", src, dst);

    /*** Critical Section BEGIN ***/
    flashDrvLock();

    for (i=0; i< sizeof(struct nvram_header); i +=4) {
        *dst++ = *src++;
    }

    for (;i < nvram_header->len && i < NVRAM_SPACE; i+= 4)
        *dst++ = ltoh32(*src++);

    /*** Critical Section END ***/
    flashDrvUnlock();

    return 0;
}

struct nvram_tuple *
_nvram_realloc(struct nvram_tuple *t, const char *name, const char *value)
{
    if (!(t = (struct nvram_tuple*)malloc(sizeof(struct nvram_tuple) +
                                          strlen(name) + 1 +
                                          strlen(value) +1)))
        return NULL;

    /* Copy name */
    t->name = (char *)&t[1];
    strcpy(t->name, name);

    /* Copy value */
    t->value = t->name + strlen(name) + 1;
    strcpy(t->value, value);

    return t;
}


void _nvram_free(struct nvram_tuple *t)
{
    if (t)
        free(t);
}
