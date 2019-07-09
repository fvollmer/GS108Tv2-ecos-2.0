//==========================================================================
//
//      pipelib.c
//
//      Pipe driver library
//
//==========================================================================
//####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2003 Waverider Communications Inc.
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
// Author(s): Alex Paulis   
// Contributors: 
// Date: Nov 8, 2004
// Purpose: Provide a simple pipe driver that allows data to transfered from
//          one thread to another via a file (implemented by the pipe driver).      
// Description: Access to the data in the driver is FIFO. Selects are provided
//              for both reads and writes. The read returns when there is at
//              one character in the buffer. The write returns when all of the
//              data has been written.
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <cyg/fileio/fileio.h>      // select - selinfo
/*#include <cyg/fileio/fileio_info.h> // getinfo ()*/
#include <cyg/io/config_keys.h>     // config_key definitions
#include <cyg/io/pipelib.h>
#include <string.h>

#include <cyg/infra/diag.h>

// ----------------------------------------------------------------------------
// Forward references.

static Cyg_ErrNo pipe_write (
    cyg_io_handle_t handle, 
    const void      *buf, 
    cyg_uint32      *len);
    
static Cyg_ErrNo pipe_read (
    cyg_io_handle_t handle, 
    void            *buf, 
    cyg_uint32      *len);
    
static Cyg_ErrNo pipe_select (
    cyg_io_handle_t handle, 
    cyg_uint32      which, 
    CYG_ADDRWORD    info);
    
static Cyg_ErrNo pipe_get_config (
    cyg_io_handle_t handle, 
    cyg_uint32      key, 
    void            *buf, 
    cyg_uint32      *len);
    
static Cyg_ErrNo pipe_set_config (
    cyg_io_handle_t handle, 
    cyg_uint32      key, 
    const void      *buf, 
    cyg_uint32      *len);

// ----------------------------------------------------------------------------
// Pipe driver device IO functions

DEVIO_TABLE (pipelibDevioTable,
             pipe_write,
             pipe_read,
             pipe_select,
             pipe_get_config,
             pipe_set_config);

// ----------------------------------------------------------------------------
// Discard any data that may be in the pipe

void pipelib_purge (
    int pipelibFd)     // the file descriptor of pipe to purge
{
    fsync(pipelibFd);
} 

// ----------------------------------------------------------------------------
// Initialize the Pipe Driver 

bool pipelib_init (
    struct cyg_devtab_entry *tab)
{
    PIPELIB_INFO_S          *pipeInfo_ps = (PIPELIB_INFO_S *)tab->priv;
    
    // initialize the buffer structure.
    CYG_ASSERT (pipeInfo_ps->storage_pv != NULL, "storage area is NULL");  
    CYG_ASSERT (pipeInfo_ps->storageSize_u32 != 0 , "storage size is zero");     
    
    pipeInfo_ps->writeIndex_u32 = 0;
    pipeInfo_ps->readIndex_u32 = 0;
    pipeInfo_ps->dataLength_u32 = 0;

    // create the buffer mutex
    cyg_mutex_init (&pipeInfo_ps->lock_t);
    cyg_cond_init (&pipeInfo_ps->wait_t, &pipeInfo_ps->lock_t);
    
    // Init the select structs.
    cyg_selinit (&pipeInfo_ps->selinfoRx);
    cyg_selinit (&pipeInfo_ps->selinfoTx);

    return true;
}

// ----------------------------------------------------------------------------
// Write the data into the buffer and wake up any readers waiting.

static Cyg_ErrNo pipe_write (
    cyg_io_handle_t         handle, 
    const void              *buf, 
    cyg_uint32              *len)
{
    cyg_devtab_entry_t      *devtab_ps = (cyg_devtab_entry_t *)handle;
    PIPELIB_INFO_S          *pipeInfo_ps = (PIPELIB_INFO_S *)devtab_ps->priv;
    cyg_uint8               *dataDst_pu8 = (cyg_uint8 *)pipeInfo_ps->storage_pv;
    cyg_uint8               *dataSrc_pu8 = (cyg_uint8 *)buf;
    cyg_uint32              writeLength_u32 = *len;
    cyg_uint32              aLen_u32;
    cyg_uint32              wLen_u32;
    Cyg_ErrNo               returnResult = ENOERR;
    cyg_tick_count_t        endTime_t = cyg_current_time() + pipeInfo_ps->waitTime_t;

    CYG_ASSERT (buf != NULL, "data area is NULL");  

    cyg_mutex_lock (&pipeInfo_ps->lock_t);

    while (writeLength_u32 > 0) 
    {       
        if (pipeInfo_ps->dataLength_u32 < pipeInfo_ps->storageSize_u32) 
        {
            /* Find min of available space, space before end of buffer and data left to write */
            aLen_u32 = pipeInfo_ps->storageSize_u32 - pipeInfo_ps->dataLength_u32;
            wLen_u32 = pipeInfo_ps->storageSize_u32 - pipeInfo_ps->writeIndex_u32;
            wLen_u32 = wLen_u32 < aLen_u32 ? wLen_u32 : aLen_u32;
            wLen_u32 = wLen_u32 < writeLength_u32 ? wLen_u32 : writeLength_u32;

            /* Copy this portion (rest, if any, on next loop through while) */
            memcpy(dataDst_pu8 + pipeInfo_ps->writeIndex_u32, dataSrc_pu8, wLen_u32);
            
            pipeInfo_ps->dataLength_u32 += wLen_u32;
            dataSrc_pu8 += wLen_u32;
            pipeInfo_ps->writeIndex_u32 += wLen_u32;
            if (pipeInfo_ps->writeIndex_u32 >= pipeInfo_ps->storageSize_u32)
            { 
                pipeInfo_ps->writeIndex_u32 = 0;
            }
            writeLength_u32 -= wLen_u32;

            // wake up any readers waiting
            cyg_cond_broadcast (&pipeInfo_ps->wait_t);
            cyg_selwakeup(&pipeInfo_ps->selinfoRx); 
        }
        else if(pipeInfo_ps->waitTime_t == PIPE_TIMEOUT_NO_WAIT)
        {   
            // don't wait for space
            *len -= writeLength_u32;    
            returnResult = ENOERR;
            break;
        } 
        else if (pipeInfo_ps->waitTime_t == PIPE_TIMEOUT_WAIT_FOREVER)
        {
            if (!cyg_cond_wait (&pipeInfo_ps->wait_t)) 
            {
                // Unexpected exit!
                *len -= writeLength_u32;    
                returnResult = EINTR;
                break;
            }
        }
        else
        {
            if (!cyg_cond_timed_wait (&pipeInfo_ps->wait_t, endTime_t))
            {
                // Timeout!
                *len -= writeLength_u32;    
                returnResult = ETIMEDOUT;
                break;
            }
        }
    }

    cyg_mutex_unlock (&pipeInfo_ps->lock_t);
    return returnResult;
}

// ----------------------------------------------------------------------------
// Read the data from the buffer and wake up any writers waiting.

static Cyg_ErrNo pipe_read (
    cyg_io_handle_t         handle, 
    void                    *buf, 
    cyg_uint32              *len)
{
    cyg_devtab_entry_t      *devtab_ps = (cyg_devtab_entry_t *)handle;
    PIPELIB_INFO_S          *pipeInfo_ps = (PIPELIB_INFO_S *)devtab_ps->priv;
    cyg_uint8               *dataSrc_pu8 = (cyg_uint8 *)pipeInfo_ps->storage_pv;
    cyg_uint8               *dataDest_pu8 = (cyg_uint8 *)buf;
    cyg_uint32              readLength_u32 = 0;
    cyg_uint32              rLen_u32;
    cyg_uint32              bLen_u32;
    Cyg_ErrNo               returnResult = ENOERR;
    cyg_tick_count_t        endTime_t = cyg_current_time() + pipeInfo_ps->waitTime_t;

    CYG_ASSERT (pipeInfo_ps != NULL, "buffer object is NULL");  
    CYG_ASSERT (buf != NULL, "data area is NULL");  

    cyg_mutex_lock (&pipeInfo_ps->lock_t);

    while (readLength_u32 < *len) 
    {
        if (pipeInfo_ps->dataLength_u32 > 0) 
        {
            /* Find min of data left to read, data til end of buffer and output buffer space left */
            rLen_u32 = pipeInfo_ps->storageSize_u32 - pipeInfo_ps->readIndex_u32;
            bLen_u32 = *len - readLength_u32;
            rLen_u32 = rLen_u32 < pipeInfo_ps->dataLength_u32 ? rLen_u32 : pipeInfo_ps->dataLength_u32;
            rLen_u32 = rLen_u32 < bLen_u32 ? rLen_u32 : bLen_u32;

            /* Copy this portion of data (rest, if any, on next loop through while) */
            memcpy(dataDest_pu8, dataSrc_pu8 + pipeInfo_ps->readIndex_u32, rLen_u32);
            dataDest_pu8 += rLen_u32;
            pipeInfo_ps->readIndex_u32 += rLen_u32;
            if(pipeInfo_ps->readIndex_u32 >= pipeInfo_ps->storageSize_u32) 
            {
                pipeInfo_ps->readIndex_u32 = 0;
            }
            pipeInfo_ps->dataLength_u32 -= rLen_u32;
            readLength_u32 += rLen_u32;

            // wake up any writers waiting
            cyg_cond_broadcast (&pipeInfo_ps->wait_t);
            cyg_selwakeup (&pipeInfo_ps->selinfoTx); 
        }
        else if ((readLength_u32 > 0) || (pipeInfo_ps->waitTime_t == PIPE_TIMEOUT_NO_WAIT))
        {
            *len = readLength_u32;    
            returnResult = ENOERR;
            break;
        }
        else if (pipeInfo_ps->waitTime_t == PIPE_TIMEOUT_WAIT_FOREVER)
        {
            if (!cyg_cond_wait (&pipeInfo_ps->wait_t)) 
            {
                // Unexpected exit!
                *len = readLength_u32;    
                returnResult = EINTR;
                break;
            }
        }
        else
        {
           if (!cyg_cond_timed_wait (&pipeInfo_ps->wait_t, endTime_t))
            {
                *len = readLength_u32;    
                returnResult = ETIMEDOUT;
                break;
            }
        }
    }

    cyg_mutex_unlock (&pipeInfo_ps->lock_t);
    return returnResult;
}

// ----------------------------------------------------------------------------
// Select: file reads or file writes only

static cyg_bool pipe_select (
    cyg_io_handle_t         handle, 
    cyg_uint32              which, 
    CYG_ADDRWORD            info)
{
    cyg_devtab_entry_t       *devtab_ps = (cyg_devtab_entry_t *)handle;
    PIPELIB_INFO_S           *pipeInfo_ps = (PIPELIB_INFO_S *)devtab_ps->priv;
    cyg_bool                 dataAvailable_b = false;

    switch (which)
    {
    case CYG_FREAD:     // Check for data in the buffer. If there is none,
                        // register the select operation, otherwise return true.
        cyg_mutex_lock (&pipeInfo_ps->lock_t);
        if (pipeInfo_ps->dataLength_u32 > 0)
        { 
            dataAvailable_b = true;
        }
        else
        {
            cyg_selrecord (info, &pipeInfo_ps->selinfoRx);
        }
        cyg_mutex_unlock (&pipeInfo_ps->lock_t);
    break;
        
    case CYG_FWRITE:    // Check for space in the buffer. If there is none,
                        // register the select operation, otherwise return true.
        cyg_mutex_lock (&pipeInfo_ps->lock_t);
        if ((pipeInfo_ps->storageSize_u32 - pipeInfo_ps->dataLength_u32) > 0)
        { 
            dataAvailable_b = true;
        }
        else 
        {
            cyg_selrecord (info, &pipeInfo_ps->selinfoTx);
        }
        cyg_mutex_unlock (&pipeInfo_ps->lock_t);
    break;

    case 0: // exceptions - none supported
    default:
        break;
    }
    
    return dataAvailable_b;
}

// ----------------------------------------------------------------------------
// Implement Flush/Drain controls

static Cyg_ErrNo pipe_get_config (
    cyg_io_handle_t         handle, 
    cyg_uint32              key, 
    void                    *buf, 
    cyg_uint32              *len)
{
    cyg_devtab_entry_t      *devtab_ps = (cyg_devtab_entry_t *)handle;
    PIPELIB_INFO_S          *pipeInfo_ps = (PIPELIB_INFO_S *)devtab_ps->priv;
    Cyg_ErrNo               retVal = ENOERR;

    diag_printf("pipe get config %s ", devtab_ps->name);
    switch (key)
    {
        // clear the buffer and signal any threads waiting.
    case CYG_IO_GET_CONFIG_SERIAL_INPUT_FLUSH:
    case CYG_IO_GET_CONFIG_SERIAL_OUTPUT_FLUSH:
    case CYG_IO_GET_CONFIG_SERIAL_OUTPUT_DRAIN:
        diag_printf("pipe drain %s \n", devtab_ps->name);
        cyg_mutex_lock (&pipeInfo_ps->lock_t);
        pipeInfo_ps->writeIndex_u32 = 0;
        pipeInfo_ps->readIndex_u32 = 0;
        pipeInfo_ps->dataLength_u32 = 0;
        cyg_cond_broadcast (&pipeInfo_ps->wait_t);
        cyg_selwakeup (&pipeInfo_ps->selinfoTx); 
        cyg_mutex_unlock (&pipeInfo_ps->lock_t);
        retVal = ENOERR;
        break;
            
    default:
        retVal = -EINVAL;
        break;
    }
    
    return (retVal);
}

// ----------------------------------------------------------------------------
// Nothing yet

static Cyg_ErrNo pipe_set_config (
    cyg_io_handle_t         handle, 
    cyg_uint32              key, 
    const void              *buf, 
    cyg_uint32              *len)
{
    return -EINVAL;
}
