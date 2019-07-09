//==========================================================================
//
//      fileio1.c
//
//      Test fileio system
//
//==========================================================================
//####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with eCos; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//
// Alternative licenses for eCos may be arranged by contacting Red Hat, Inc.
// at http://sources.redhat.com/ecos/ecos-license/
// -------------------------------------------
//####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):           nickg
// Contributors:        nickg
// Date:                2000-05-25
// Purpose:             Test fileio system
// Description:         This test uses the testfs to check out the initialization
//                      and basic operation of the fileio system
//                      
//                      
//                      
//                      
//                      
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal.h>
#include <pkgconf/kernel.h>
#include <pkgconf/io_fileio.h>

#include <cyg/kernel/ktypes.h>         // base kernel types
#include <cyg/infra/cyg_trac.h>        // tracing macros
#include <cyg/infra/cyg_ass.h>         // assertion macros
#include <pkgconf/fs_ram.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL

#include <cyg/kernel/kapi.h>


#include <cyg/hal/hal_intr.h>           // exception ranges

#include <cyg/fileio/fileio.h>

#include <cyg/infra/testcase.h>
#include <cyg/infra/diag.h>            // HAL polled output

typedef cyg_uint8 ramfs_block[CYGNUM_RAMFS_BLOCK_SIZE];

struct ramfs_node;
typedef struct ramfs_node ramfs_node;

struct ramfs_runtime_struct
{
   ramfs_block *block_free_list;
   cyg_bool initialized;
   ramfs_node *root_node;
   unsigned long ulgMagicNumber;
   unsigned char uchVersion;
   unsigned long ulgCRC;
   cyg_uint8 pad[232];
};

typedef struct ramfs_runtime_struct ramfs_runtime_struct;

/* store RAM File system in flash, need fixed ram address */
ramfs_runtime_struct __ramfs_runtime_data __attribute__ ((section (".ramfsdata")));
ramfs_block flash_block_array[CYGNUM_FS_RAM_BLOCKS_ARRAY_SIZE] __attribute__ ((section (".ramfsdata")));

//==========================================================================

#if 0
MTAB_ENTRY( ramfs_mte1,
                   "/",
                   "ramfs",
                   "",
                   0);
#endif

//==========================================================================

#define SHOW_RESULT( _fn, _res ) \
diag_printf("<FAIL>: " #_fn "() returned %d %s\n", _res, _res<0?strerror(errno):"");

//==========================================================================

#define IOSIZE  100

#define LONGNAME1       "long_file_name_that_should_take_up_more_than_one_directory_entry_1"
#define LONGNAME2       "long_file_name_that_should_take_up_more_than_one_directory_entry_2"


//==========================================================================

#ifndef CYGPKG_LIBC_STRING

char *strcat( char *s1, const char *s2 )
{
    char *s = s1;
    while( *s1 ) s1++;
    while( (*s1++ = *s2++) != 0);
    return s;
}

#endif

//==========================================================================

static void listdir( char *name, int statp, int numexpected, int *numgot )
{
    int err;
    DIR *dirp;
    int num=0;
    
    diag_printf("<INFO>: reading directory %s\n",name);
    
    dirp = opendir( name );
    if( dirp == NULL ) SHOW_RESULT( opendir, -1 );

    for(;;)
    {
        struct dirent *entry = readdir( dirp );
        
        if( entry == NULL )
            break;
        num++;
        diag_printf("<INFO>: entry %14s",entry->d_name);
        if( statp )
        {
            char fullname[PATH_MAX];
            struct stat sbuf;

            if( name[0] )
            {
                strcpy(fullname, name );
                if( !(name[0] == '/' && name[1] == 0 ) )
                    strcat(fullname, "/" );
            }
            else fullname[0] = 0;
            
            strcat(fullname, entry->d_name );
            
            err = stat( fullname, &sbuf );
            if( err < 0 )
            {
                if( errno == ENOSYS )
                    diag_printf(" <no status available>");
                else SHOW_RESULT( stat, err );
            }
            else
            {
                diag_printf(" [mode %08x ino %08x nlink %d size %d]",
                            sbuf.st_mode,sbuf.st_ino,sbuf.st_nlink,sbuf.st_size);
            }
        }

        diag_printf("\n");
    }

    err = closedir( dirp );
    if( err < 0 ) SHOW_RESULT( stat, err );
    if (numexpected >= 0 && num != numexpected)
        CYG_TEST_FAIL("Wrong number of dir entries\n");
    if ( numgot != NULL )
        *numgot = num;
}

//==========================================================================

static void createfile( char *name, size_t size )
{
    char buf[IOSIZE];
    int fd;
    ssize_t wrote;
    int i;
    int err;

    diag_printf("<INFO>: create file %s size %d\n",name,size);

    err = access( name, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );
    
    for( i = 0; i < IOSIZE; i++ ) buf[i] = i%256;
 
    fd = open( name, O_WRONLY|O_CREAT );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    while( size > 0 )
    {
        ssize_t len = size;
        if ( len > IOSIZE ) len = IOSIZE;
        
        wrote = write( fd, buf, len );
        if( wrote != len ) SHOW_RESULT( write, wrote );        

        size -= wrote;
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

//==========================================================================

#if 0
static void maxfile( char *name )
{
    char buf[IOSIZE];
    int fd;
    ssize_t wrote;
    int i;
    int err;
    size_t size = 0;
    
    diag_printf("<INFO>: create maximal file %s\n",name);

    err = access( name, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );
    
    for( i = 0; i < IOSIZE; i++ ) buf[i] = i%256;
 
    fd = open( name, O_WRONLY|O_CREAT );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    do
    {
        wrote = write( fd, buf, IOSIZE );
        if( wrote < 0 ) SHOW_RESULT( write, wrote );        

        size += wrote;
        
    } while( wrote == IOSIZE );

    diag_printf("<INFO>: file size == %d\n",size);

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}
#endif

//==========================================================================

static void checkfile( char *name )
{
    char buf[IOSIZE];
    int fd;
    ssize_t done;
    int i;
    int err;
    off_t pos = 0;

    diag_printf("<INFO>: check file %s\n",name);
    
    err = access( name, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );

    fd = open( name, O_RDONLY );
    if( fd < 0 ) SHOW_RESULT( open, fd );

    for(;;)
    {
        done = read( fd, buf, IOSIZE );
        if( done < 0 ) SHOW_RESULT( read, done );

        if( done == 0 ) break;

        for( i = 0; i < done; i++ )
            if( buf[i] != i%256 )
            {
                diag_printf("buf[%d+%d](%02x) != %02x\n",pos,i,buf[i],i%256);
                CYG_TEST_FAIL("Data read not equal to data written\n");
            }
        
        pos += done;
    }

    err = close( fd );
    if( err < 0 ) SHOW_RESULT( close, err );
}

//==========================================================================

static void copyfile( char *name2, char *name1 )
{

    int err;
    char buf[IOSIZE];
    int fd1, fd2;
    ssize_t done, wrote;

    diag_printf("<INFO>: copy file %s -> %s\n",name2,name1);

    err = access( name1, F_OK );
    if( err < 0 && errno != EACCES ) SHOW_RESULT( access, err );

    err = access( name2, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );
    
    fd1 = open( name1, O_WRONLY|O_CREAT );
    if( fd1 < 0 ) SHOW_RESULT( open, fd1 );

    fd2 = open( name2, O_RDONLY );
    if( fd2 < 0 ) SHOW_RESULT( open, fd2 );
    
    for(;;)
    {
        done = read( fd2, buf, IOSIZE );
        if( done < 0 ) SHOW_RESULT( read, done );

        if( done == 0 ) break;

        wrote = write( fd1, buf, done );
        if( wrote != done ) SHOW_RESULT( write, wrote );

        if( wrote != done ) break;
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
}

//==========================================================================

static void comparefiles( char *name2, char *name1 )
{
    int err;
    char buf1[IOSIZE];
    char buf2[IOSIZE];
    int fd1, fd2;
    ssize_t done1, done2;
    int i;

    diag_printf("<INFO>: compare files %s == %s\n",name2,name1);

    err = access( name1, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );

    err = access( name1, F_OK );
    if( err != 0 ) SHOW_RESULT( access, err );
    
    fd1 = open( name1, O_RDONLY );
    if( fd1 < 0 ) SHOW_RESULT( open, fd1 );

    fd2 = open( name2, O_RDONLY );
    if( fd2 < 0 ) SHOW_RESULT( open, fd2 );
    
    for(;;)
    {
        done1 = read( fd1, buf1, IOSIZE );
        if( done1 < 0 ) SHOW_RESULT( read, done1 );

        done2 = read( fd2, buf2, IOSIZE );
        if( done2 < 0 ) SHOW_RESULT( read, done2 );

        if( done1 != done2 )
            diag_printf("Files different sizes\n");
        
        if( done1 == 0 ) break;

        for( i = 0; i < done1; i++ )
            if( buf1[i] != buf2[i] )
            {
                diag_printf("buf1[%d](%02x) != buf1[%d](%02x)\n",i,buf1[i],i,buf2[i]);
                CYG_TEST_FAIL("Data in files not equal\n");
            }
    }

    err = close( fd1 );
    if( err < 0 ) SHOW_RESULT( close, err );

    err = close( fd2 );
    if( err < 0 ) SHOW_RESULT( close, err );
    
}

//==========================================================================

void checkcwd( const char *cwd )
{
    static char cwdbuf[PATH_MAX];
    char *ret;

    ret = getcwd( cwdbuf, sizeof(cwdbuf));
    if( ret == NULL ) SHOW_RESULT( getcwd, ret );    

    if( strcmp( cwdbuf, cwd ) != 0 )
    {
        diag_printf( "cwdbuf %s cwd %s\n",cwdbuf, cwd );
        CYG_TEST_FAIL( "Current directory mismatch");
    }
}

#include <cyg/io/flash.h>
#include <cyg/io/flashDrvLib.h>
#define CYGNUM_FS_RAM_BASE_ADDRESS 0xbd000000
#define CYGNUM_FLASH_SECTOR_NUM    2
#define CYGNUM_FLASH_SECTOR_SIZE   0x20000 

int flashFsSync(void)
{
  cyg_uint8 *pos, *header;
   cyg_uint32 err_addr;
      int ret, i;

     pos = (cyg_uint8 *)CYGNUM_FS_RAM_BASE_ADDRESS;
      header = (cyg_uint8 *)&__ramfs_runtime_data;

     for (i = 0; i< CYGNUM_FLASH_SECTOR_NUM; i++) {
         pos += i*CYGNUM_FLASH_SECTOR_SIZE;

        header += i*CYGNUM_FLASH_SECTOR_SIZE;
        ret = flash_program((void*)pos, (void*)header, CYGNUM_FLASH_SECTOR_SIZE,
        (void**)&err_addr);
         if (ret) {
                diag_printf("flashFsSync: Can't program region at %d\n", err_addr);
			                   break;
              }
     }
								         return ret;
}

int flashFsRestore(void)
{
	    int ret = 0;
		    cyg_uint8 *pos, *header;
			    int i,j;

			   flashDrvLock();
			   /* should check if MAGIC NUMBER exist and CRC ok or not */
			      pos = (cyg_uint8 *)CYGNUM_FS_RAM_BASE_ADDRESS;
			      header = (cyg_uint8 *)&__ramfs_runtime_data;
			       /* DON'T use memcpy here, cause failure */
			      for (i = 0; i< CYGNUM_FLASH_SECTOR_NUM; i++) {
			          j = CYGNUM_FLASH_SECTOR_SIZE;
			            while(--j > 0) {
			                *header++ = *pos++;
						           }
			         }
			       flashDrvUnlock();
 return ret;
}


void osNativeMemShow(void)
{
  struct mallinfo meminfo;

  meminfo = mallinfo();

  diag_printf("============================================\n");
  diag_printf("ECOS Memory Information:\n");
  diag_printf("============================================\n");
  diag_printf("  TOTAL size of memory arena  = %d\n", meminfo.arena);
  diag_printf("  number of ordinary memory blocks = %d\n", meminfo.ordblks);
  diag_printf("  number of small memory blocks = %d\n", meminfo.smblks);
  diag_printf("  number of mmapped regions = %d\n", meminfo.hblks);
  diag_printf("  total space in mmapped regions = %d\n", meminfo.hblkhd);
  diag_printf("  space used by small memory blocks = %d\n", meminfo.usmblks);
  diag_printf("  space available for small memory blocks = %d\n", meminfo.fsmblks);
  diag_printf("  space used by ordinary memory blocks = %d\n", meminfo.uordblks);
  diag_printf("  space FREE for ordinary blocks = %d\n", meminfo.fordblks);
  diag_printf("  top-most, releasable (via malloc_trim) space = %d\n", meminfo.keepcost);
  diag_printf("  size of LARGEST free block = %d\n", meminfo.maxfree);
  diag_printf("============================================\n");
}
//==========================================================================

static void handler0(cyg_addrword_t data, cyg_code_t number, cyg_addrword_t info)
{
    CYG_TEST_INFO("handler 0 called");

    CYG_TEST_CHECK((cyg_addrword_t)123 == data, "handler given wrong data");

    // ignore machine specific stuff
    CYG_UNUSED_PARAM(cyg_code_t, number);
    CYG_UNUSED_PARAM(cyg_addrword_t, info);

     diag_printf("\n**Exception ALOK ");
    CYG_TEST_PASS_FINISH("Except 1 OK");
}

#if 0
void cyg_user_start(void)
{
  cyg_code_t n;
  cyg_exception_handler_t *old_handler, *old_handler1;
  cyg_addrword_t old_data, old_data1;

  #ifdef HAL_VSR_SET_TO_ECOS_HANDLER
    // Reclaim the VSR off CygMon possibly
#ifdef CYGNUM_HAL_EXCEPTION_DATA_ACCESS
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_DATA_ACCESS, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_DATA_TLBMISS_ACCESS
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_DATA_TLBMISS_ACCESS, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_DATA_UNALIGNED_ACCESS
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_DATA_UNALIGNED_ACCESS, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_ILLEGAL_INSTRUCTION
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_ILLEGAL_INSTRUCTION, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_DIV_BY_ZERO
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_DIV_BY_ZERO, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_FPU
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_FPU, NULL );
#endif
#ifdef CYGNUM_HAL_EXCEPTION_FPU_DIV_BY_ZERO
    HAL_VSR_SET_TO_ECOS_HANDLER( CYGNUM_HAL_EXCEPTION_FPU_DIV_BY_ZERO, NULL );
#endif
#endif

  for(n = CYGNUM_HAL_EXCEPTION_MIN; n <= CYGNUM_HAL_EXCEPTION_MAX; n++) {
  cyg_exception_clear_handler(n);
        cyg_exception_set_handler(
            n,
            handler0,
            (cyg_addrword_t)123,
            &old_handler1,
            &old_data1);
    }

  diag_printf("cyg_user_start called\n");

}
#endif


// main

int main( int argc, char **argv )
{
    int err;
    int existingdirents=-1;

    CYG_TEST_INIT();

	diag_printf("test started \n");

    // --------------------------------------------------------------
#if 0
    err = mount( "", "/", "ramfs" );
    if( err < 0 ) SHOW_RESULT( mount, err );    

    err = chdir( "/" );
    if( err < 0 ) SHOW_RESULT( chdir, err );

    checkcwd( "/" );
    
    listdir( "/", true, -1, &existingdirents );
    if ( existingdirents < 2 )
        CYG_TEST_FAIL("Not enough dir entries\n");

    // --------------------------------------------------------------

    createfile( "foo", 202 );
    createfile( "alokone", 202 );
    checkfile( "foo" );
    copyfile( "foo", "fee");
    checkfile( "fee" );
    comparefiles( "foo", "fee" );
    diag_printf("<INFO>: mkdir bar\n");
    err = mkdir( "bar", 0 );
    if( err < 0 ) SHOW_RESULT( mkdir, err );

    listdir( "/" , true, existingdirents+3, NULL );

	flashFsSync();
	unlink("foo");
	unlink("fee");
	rmdir("bar");

    listdir( "/" , true, existingdirents, NULL );
    diag_printf("<INFO>: umount /ram\n");
    err = umount( "/" );
	    if( err < 0 ) SHOW_RESULT( umount, err );
#endif

    osNativeMemShow();
      
    CYG_TEST_PASS_FINISH("fileio1");
}

// -------------------------------------------------------------------------
// EOF fileio1.c
