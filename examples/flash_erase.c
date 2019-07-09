#include <pkgconf/hal.h>
#include <pkgconf/io_fileio.h>

#include <cyg/infra/cyg_trac.h>        // tracing macros
#include <cyg/infra/cyg_ass.h>         // assertion macros
#include <cyg/hal/bcmnvram.h>
#include <cyg/io/flash.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>

#include <cyg/fileio/fileio.h>
#include <cyg/io/io.h>
#include <cyg/infra/testcase.h>
#include <cyg/infra/diag.h>


char __ramfs_runtime_data[1024];
char flash_block_array[1024];

//==========================================================================
// main

int main( int argc, char **argv )
{
    cyg_io_handle_t nvram_hdl;
    cyg_uint8 *pos;
    cyg_uint32 err_addr;
//    pos=(cyg_uint8*)(0xbd000000);
    pos=(cyg_uint8*)(0xBC060000);
#if defined(CYGSEM_FILEIO_BLOCK_USAGE)
    struct cyg_fs_block_usage usage;
#endif

    CYG_TEST_INIT();
    diag_printf("\n test started ");

    flash_erase((void*)pos, (1024*1024), (void**)&err_addr);

    diag_printf("\n Test Finished");
    // --------------------------------------------------------------
    CYG_TEST_PASS_FINISH("flasherase");
}
