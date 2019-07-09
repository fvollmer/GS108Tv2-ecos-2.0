/*
 * Broadcom SiliconBackplane chipcommon serial flash interface
 *
 * $Copyright Open Broadcom Corporation$
 *
 * $Id: sflash.c,v 1.2 2009/02/04 10:45:18 cchao Exp $
 */

#include <cyg/hal/typedefs.h>
#include <cyg/hal/osl.h>
#include <cyg/hal/bcmutils.h>
#include <cyg/hal/siutils.h>
#include <cyg/hal/hndsoc.h>
#include <cyg/hal/sbchipc.h>
#include <cyg/hal/bcmdevs.h>
#include <cyg/hal/sflash.h>

#ifdef BCMDBG
#define	SFL_MSG(args)	printf args
#else
#define	SFL_MSG(args)
#endif	/* BCMDBG */
    
/* Private global state */
static struct sflash sflash;

/* Issue a serial flash command */
static INLINE void
sflash_cmd(osl_t *osh, chipcregs_t *cc, uint opcode)
{
    W_REG(osh, &cc->sflashcontrol, SFLASH_START | opcode);
    while (R_REG(osh, &cc->sflashcontrol) & SFLASH_BUSY);
}

/* Initialize serial flash access */
struct sflash *
sflash_init(si_t *sih, chipcregs_t *cc)
{
    uint32 id, id2;
    osl_t *osh;
    
    ASSERT(sih);
    
    osh = si_osh(sih);
    
    bzero(&sflash, sizeof(sflash));
    
    sflash.type = sih->cccaps & CC_CAP_FLASH_MASK;
    
    switch (sflash.type) {
    case SFLASH_ST:
          /* Probe for ST chips */

          W_REG(osh, &cc->sflashaddress, 0x00);
          sflash_cmd(osh, cc, SFLASH_ST_DEVID);
          id = R_REG(osh, &cc->sflashdata);
          sflash.vendor = id;

          /* read device id */
          W_REG(osh, &cc->sflashaddress, 0x01);
          sflash_cmd(osh, cc, SFLASH_ST_DEVID);
          id = R_REG(osh, &cc->sflashdata);
          sflash.devid = id;


          printf("vendor %x id %x\n",sflash.vendor,sflash.devid);



          switch (id) {
          case 0x11:
                /* ST M25P20 2 Mbit Serial Flash */
                sflash.blocksize = 64 * 1024;
                sflash.numblocks = 4;
                break;
          case 0x12:
                /* ST M25P40 4 Mbit Serial Flash */
                sflash.blocksize = 64 * 1024;
                sflash.numblocks = 8;
                break;
          case 0x13:
              /* ST M25P80 8 Mbit Serial Flash */
              sflash.blocksize = 64 * 1024;
              sflash.numblocks = 16;
              break;
          case 0x14:
              /* ST M25P16 16 Mbit Serial Flash */
              sflash.blocksize = 64 * 1024;
              sflash.numblocks = 32;
              break;
          case 0x15:
              /* ST M25P32 32 Mbit Serial Flash */
              sflash.blocksize = 64 * 1024;
              sflash.numblocks = 64;
              break;
          case 0x16:
              /* ST M25P64 64 Mbit Serial Flash */
              sflash.blocksize = 64 * 1024;
              sflash.numblocks = 128;
              break;
          case 0x17:
              /* ST M2L128 128 Mbit Serial Flash */
              sflash.blocksize = 64 * 1024;
              sflash.numblocks = 256;
              break;

          case 0xbf:
              W_REG(osh, &cc->sflashaddress, 1);
              sflash_cmd(osh, cc, SFLASH_ST_RES);
              id2 = R_REG(osh, &cc->sflashdata);
              if (id2 == 0x44) {
                  /* SST M25VF80 4 Mbit Serial Flash */
                  sflash.blocksize = 64 * 1024;
                  sflash.numblocks = 8;
              }
              break;
          case 0x1:
              /* ST S25FL128P00M 128 Mbit Serial Flash */
              sflash.blocksize = 64 * 1024;
              sflash.numblocks = 256;
              break;
          }
          break;
    
    case SFLASH_AT:
          /* Probe for Atmel chips */
          sflash_cmd(osh, cc, SFLASH_AT_STATUS);
          id = R_REG(osh, &cc->sflashdata) & 0x3c;
          switch (id) {
          case 0xc:
              /* Atmel AT45DB011 1Mbit Serial Flash */
              sflash.blocksize = 256;
              sflash.numblocks = 512;
              break;
          case 0x14:
              /* Atmel AT45DB021 2Mbit Serial Flash */
              sflash.blocksize = 256;
              sflash.numblocks = 1024;
              break;
          case 0x1c:
              /* Atmel AT45DB041 4Mbit Serial Flash */
              sflash.blocksize = 256;
              sflash.numblocks = 2048;
              break;
          case 0x24:
              /* Atmel AT45DB081 8Mbit Serial Flash */
              sflash.blocksize = 256;
              sflash.numblocks = 4096;
              break;
          case 0x2c:
              /* Atmel AT45DB161 16Mbit Serial Flash */
              sflash.blocksize = 512;
              sflash.numblocks = 4096;
              break;
          case 0x34:
              /* Atmel AT45DB321 32Mbit Serial Flash */
              sflash.blocksize = 512;
              sflash.numblocks = 8192;
              break;
          case 0x3c:
              /* Atmel AT45DB642 64Mbit Serial Flash */
              sflash.blocksize = 1024;
              sflash.numblocks = 8192;
              break;
          }
          break;
    }
    
    sflash.size = sflash.blocksize * sflash.numblocks;
    return sflash.size ? &sflash : NULL;
}

/* Read len bytes starting at offset into buf. Returns number of bytes read. */
int
sflash_read(si_t *sih, chipcregs_t *cc, uint offset, uint len, uchar *buf)
{
    uint8 *from, *to;
    int cnt, i;
    osl_t *osh;
    
    ASSERT(sih);
    
    if (!len)
        return 0;
    
    if ((offset + len) > sflash.size)
        return -22;
    
    if ((len >= 4) && (offset & 3))
        cnt = 4 - (offset & 3);
    else if ((len >= 4) && ((uintptr)buf & 3))
        cnt = 4 - ((uintptr)buf & 3);
    else
        cnt = len;
    
    osh = si_osh(sih);
    
    from = (uint8 *)OSL_UNCACHED(SI_FLASH2 + offset);
    to = (uint8 *)buf;
    
    if (cnt < 4) {
        for (i = 0; i < cnt; i ++) {
            *to = R_REG(osh, from);
            from ++;
            to ++;
        }
        return cnt;
    }
    
    while (cnt >= 4) {
        *(uint32 *)to = R_REG(osh, (uint32 *)from);
        from += 4;
        to += 4;
        cnt -= 4;
    }
    
    return (len - cnt);
}

/* Poll for command completion. Returns zero when complete. */
int
sflash_poll(si_t *sih, chipcregs_t *cc, uint offset)
{
    osl_t *osh;
    
    ASSERT(sih);
    
    osh = si_osh(sih);
    
    if (offset >= sflash.size)
        return -22;
    
    switch (sflash.type) {
    case SFLASH_ST:
          /* Check for ST Write In Progress bit */
          sflash_cmd(osh, cc, SFLASH_ST_RDSR);
          return R_REG(osh, &cc->sflashdata) & SFLASH_ST_WIP;
    case SFLASH_AT:
          /* Check for Atmel Ready bit */
          sflash_cmd(osh, cc, SFLASH_AT_STATUS);
          return !(R_REG(osh, &cc->sflashdata) & SFLASH_AT_READY);
    }
    
    return 0;
}

/* Write len bytes starting at offset into buf. Returns number of bytes
 * written. Caller should poll for completion.
 */
#define	ST_RETRIES	3

int
sflash_write(si_t *sih, chipcregs_t *cc, uint offset, uint length, const uchar *buffer)
{
    struct sflash *sfl;
    uint off = offset, len = length;
    const uchar *buf = buffer;
    int ret = 0;
    int tryn = 0;
    uint32 page, byte, mask;
    osl_t *osh;
    unsigned int tempword;
    unsigned int tempword1;
    unsigned int i;
    unsigned int pagewritecommand;
    unsigned char tempbuf[20];
    unsigned char *tempptr;
    ASSERT(sih);
    
    osh = si_osh(sih);
    
    if (!len)
        return 0;
    sfl = &sflash;
    if ((off + len) > sfl->size)
        return -22;
    switch (sfl->type) {
    case SFLASH_ST:
          /* Enable writes */
retry: 
          if (sih->ccrev >= 20) {

              pagewritecommand = TRUE;
              while (len > 0) {
                if (pagewritecommand == TRUE)
                {
                  {
                    /* dummy flash read for Keystone serial flash and cs1 issue*/
                    tempptr = (unsigned char *)(0xbc000000 | off);
                    memcpy(tempbuf, tempptr, 8);
                  }
                  sflash_cmd(osh, cc, SFLASH_ST_WREN);
                  tempword1 = *(uint32*)&buf[0];
                  W_REG(osh, &cc->sflashaddress, off);
                  W_REG(osh, &cc->sflashdata, tempword1);
                  /* Issue a page program with CSA bit set */
                  sflash_cmd(osh, cc, (SFLASH_ST_CSA | 0x0402));
                  buf = buf + 4;
                  off = off + 4;
                  ret = ret + 4;
                  len = len - 4;
                  pagewritecommand = FALSE;
                }
                tempword = *(uint32*)&buf[0];
                W_REG(osh, &cc->sflashaddress, (tempword & 0xFFFFFF));

                tempword1 = *(uint32*)&buf[4];
                W_REG(osh, &cc->sflashdata, tempword1);
                                
                sflash_cmd(osh, cc, (SFLASH_ST_CSA | 0x400 |*buf));

                buf = buf + 8;
                off = off + 8;
                ret = ret + 8;
                len = len - 8;

                if (((off & 0xFF) >= 0xF8) || ( len == 0))
                {
                   W_REG(osh, &cc->sflashcontrol, 0);
                   OSL_DELAY(100);
                   for (i = 0; i <= 2000; i++)
                   {
                     sflash_cmd(osh, cc, SFLASH_ST_RDSR);
                     if ((R_REG(osh, &cc->sflashdata) & SFLASH_ST_WIP) != SFLASH_ST_WIP)
                     {
                       break;
                     }
                     if ( i == 2000)
                     {
                       printf("Flash page write failed\n");
                       return -22;
                     }
                     OSL_DELAY(2);
                  }

                  tempword1 = *(uint32*)&buf[0];
                  W_REG(osh, &cc->sflashaddress, off);
                  sflash_cmd(osh, cc, ( SFLASH_ST_WREN));
                  W_REG(osh, &cc->sflashdata, tempword1);

                  sflash_cmd(osh, cc,  0x80000402);
                  W_REG(osh, &cc->sflashcontrol, 0);
                  OSL_DELAY(1);

                  buf = buf + 4;
                  off = off + 4;
                  ret = ret + 4;
                  len = len - 4;

                  OSL_DELAY(100);
                  for (i = 0; i <= 2000; i++)
                  {
                    sflash_cmd(osh, cc, SFLASH_ST_RDSR);
                    if ((R_REG(osh, &cc->sflashdata) & SFLASH_ST_WIP) != SFLASH_ST_WIP)
                    {
                      break;
                    }
                    if ( i == 2000)
                    {
                      printf("Flash page write failed");
                      return -22;
                    }
                    OSL_DELAY(2);
                  }
                  pagewritecommand = TRUE;
                }
              }


          } else {
              ret = 1;
              W_REG(osh, &cc->sflashaddress, off);
              W_REG(osh, &cc->sflashdata, *buf);
              /* Page program */
              sflash_cmd(osh, cc, SFLASH_ST_PP);
          }
          break;
    case SFLASH_AT:
          mask = sfl->blocksize - 1;
          page = (off & ~mask) << 1;
          byte = off & mask;
          /* Read main memory page into buffer 1 */
          if (byte || (len < sfl->blocksize)) {
              W_REG(osh, &cc->sflashaddress, page);
              sflash_cmd(osh, cc, SFLASH_AT_BUF1_LOAD);
              /* 250 us for AT45DB321B */
              SPINWAIT(sflash_poll(sih, cc, off), 1000);
              ASSERT(!sflash_poll(sih, cc, off));
          }
          /* Write into buffer 1 */
          for (ret = 0; (ret < (int)len) && (byte < sfl->blocksize); ret++) {
              W_REG(osh, &cc->sflashaddress, byte++);
              W_REG(osh, &cc->sflashdata, *buf++);
              sflash_cmd(osh, cc, SFLASH_AT_BUF1_WRITE);
          }
          /* Write buffer 1 into main memory page */
          W_REG(osh, &cc->sflashaddress, page);
          sflash_cmd(osh, cc, SFLASH_AT_BUF1_PROGRAM);
          break;
    }
    
    return ret;
}

/* Erase a region. Returns number of bytes scheduled for erasure.
 * Caller should poll for completion.
 */
int
sflash_erase(si_t *sih, chipcregs_t *cc, uint offset)
{
    struct sflash *sfl;
    osl_t *osh;
    
    ASSERT(sih);
    osh = si_osh(sih);
    sfl = &sflash;
    if (offset >= sfl->size)
        return -22;
    
    switch (sfl->type) {
    case SFLASH_ST:
          sflash_cmd(osh, cc, SFLASH_ST_WREN);
          W_REG(osh, &cc->sflashaddress, offset);
          sflash_cmd(osh, cc, SFLASH_ST_SE);
/* remove while and check for some time and return failue*/
      while (sflash_poll(sih, cc, offset));
          return sfl->blocksize;
    case SFLASH_AT:
          W_REG(osh, &cc->sflashaddress, offset << 1);
          sflash_cmd(osh, cc, SFLASH_AT_PAGE_ERASE);
          return sfl->blocksize;
    }
    
    return 0;
}

/*
 * writes the appropriate range of flash, a NULL buf simply erases
 * the region of flash
 */
int
sflash_commit(si_t *sih, chipcregs_t *cc, uint offset, uint len, const uchar *buf)
{
    struct sflash *sfl;
    uchar *block = NULL, *cur_ptr, *blk_ptr;
    uint blocksize = 0, mask, cur_offset, cur_length, cur_retlen, remainder;
    uint blk_offset, blk_len, copied;
    int bytes, ret = 0;
    osl_t *osh;
    
    ASSERT(sih);
    
    osh = si_osh(sih);
    
    /* Check address range */
    if (len <= 0)
        return 0;
    
    sfl = &sflash;
    if ((offset + len) > sfl->size)
        return -1;
    
    blocksize = sfl->blocksize;
    mask = blocksize - 1;
    
    /* Allocate a block of mem */
    if (!(block = MALLOC(osh, blocksize)))
        return -1;
    
    while (len) {
        /* Align offset */
        cur_offset = offset & ~mask;
        cur_length = blocksize;
        cur_ptr = block;
        
        remainder = blocksize - (offset & mask);
        if (len < remainder)
            cur_retlen = len;
        else
            cur_retlen = remainder;
        
        /* buf == NULL means erase only */
        if (buf) {
            /* Copy existing data into holding block if necessary */
            if ((offset & mask) || (len < blocksize)) {
                blk_offset = cur_offset;
                blk_len = cur_length;
                blk_ptr = cur_ptr;
                
                /* Copy entire block */
                while (blk_len) {
                    copied = sflash_read(sih, cc, blk_offset, blk_len, blk_ptr);
                    blk_offset += copied;
                    blk_len -= copied;
                    blk_ptr += copied;
                }
            }
            
            /* Copy input data into holding block */
            memcpy(cur_ptr + (offset & mask), buf, cur_retlen);
        }
        
        /* Erase block */
        if ((ret = sflash_erase(sih, cc, (uint) cur_offset)) < 0)
            goto done;
        while (sflash_poll(sih, cc, (uint) cur_offset));
        
        /* buf == NULL means erase only */
        if (!buf) {
            offset += cur_retlen;
            len -= cur_retlen;
            continue;
        }
        
        /* Write holding block */
        while (cur_length > 0) {
            if ((bytes = sflash_write(sih, cc,
                                      (uint) cur_offset,
                                      (uint) cur_length,
                                      (uchar *) cur_ptr)) < 0) {
                ret = bytes;
                goto done;
            }
            while (sflash_poll(sih, cc, (uint) cur_offset));
            cur_offset += bytes;
            cur_length -= bytes;
            cur_ptr += bytes;
        }
        
        offset += cur_retlen;
        len -= cur_retlen;
        buf += cur_retlen;
    }
    
    ret = len;
done:
    if (block)
        MFREE(osh, block, blocksize);
    return ret;
}
