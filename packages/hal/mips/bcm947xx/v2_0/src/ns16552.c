//=============================================================================
//
//      ns16552.c
//
//      Simple driver for the NS16652 serial controller
//
//=============================================================================
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
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   
// Contributors:
// Date:        
// Description: Simple HAL driver for the NS16552 serial controller
// Note:        To drop into a new HAL, you should only have to change the
//              few parameters here at the top.
//####DESCRIPTIONEND####
//
//=============================================================================

#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>         // base types

#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros

#include <cyg/hal/hal_arch.h>           // basic machine info
#include <cyg/hal/hal_intr.h>           // interrupt macros
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_if.h>             // interface API
#include <cyg/hal/hal_misc.h>           // Helper functions
#include <cyg/hal/typedefs.h>
#include <cyg/hal/ecosbsp.h>

/* Interrupt mode */
#undef UART_INTERRUPT_MODE
#define UART_INTERRUPT_RX_BUFFER_LEN    (512)

#ifndef NO_NETWORK_IO

/* Header files for network I/O */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Special characters used by Telnet - must be interpretted here */
#define TELNET_NUL    0x00 /* NULL (No operation) */
#define TELNET_IAC    0xFF /* Interpret as command (escape) */
#define TELNET_IP     0xF4 /* Interrupt process */
#define TELNET_WILL   0xFB /* I will */
#define TELNET_WONT   0xFC /* I Won't */
#define TELNET_DO     0xFD /* Will you do */
#define TELNET_DONT   0xFE /* Don't do it */
#define TELNET_ECHO   0x01 /* Option: echo */
#define TELNET_TM     0x06 /* Option: Time marker (special DO/WONT after IP) */
#define TELNET_SGA    0x03 /* Option: Suppress Go Ahead */

/*
 * Definitions for network I/O
 */
#define NETIO_DEFAULT_TIMEOUT      (180)        /* seconds */
#define NETIO_CHAR_ON_ERROR         ('\004')    /* Ctrl-D */

/*
 * Variables for network I/O
 */
static cyg_ucount32 netio_handle_thread_index = (cyg_ucount32)-1;
static unsigned int netio_timeout = NETIO_DEFAULT_TIMEOUT;

void
netio_associate_socket(int s)
{
    /*
     * Data stored in the per-thread-data:
     * -1   : UART console
     *  0   : non-associated thread
     *  > 0 : network I/O
     *
     */
    cyg_thread_set_data(
        netio_handle_thread_index, 
        (s >= 0) ? (cyg_addrword_t)(s + 1) : (cyg_addrword_t)-1
        );
}

void
netio_deassociate_socket(void)
{
    cyg_thread_set_data(netio_handle_thread_index, 0);
}

int
netio_get_assoicated_socket(void)
{
    int s;
    
    if (netio_handle_thread_index == (cyg_ucount32)-1) {
        return -1;
    }
    s = cyg_thread_get_data(netio_handle_thread_index);
    if (s <= 0) {
        return -1;
    }
    return s - 1;
}

void netio_set_timeout(int t)
{
    netio_timeout = t;
}

#endif /* !NO_NETWORK_IO */

//-----------------------------------------------------------------------------

#define NS_SERIAL_A_BASE 0xbd000020
#define NS_SERIAL_A_IRQ  CYGNUM_HAL_INTERRUPT_UART1
#define NS_SERIAL_B_BASE 0xbd000000
#define NS_SERIAL_B_IRQ  CYGNUM_HAL_INTERRUPT_UART2

#define NS_CLOCK /*1843200*/ 50000000

int num_tty;

#define CYGNUM_HAL_VIRTUAL_VECTOR_CHANNELS_DEFAULT_BAUD  115200

// Fill in extra code as required
#define NS_EXTRA_INIT()                         \
    CYG_MACRO_START                             \
    CYG_MACRO_END

//-----------------------------------------------------------------------------


#define CYG_DEVICE_SERIAL_BAUD_MSB (((NS_CLOCK / (16*(CYGNUM_HAL_VIRTUAL_VECTOR_CHANNELS_DEFAULT_BAUD))) >> 8) & 0xff)
#define CYG_DEVICE_SERIAL_BAUD_LSB  ((NS_CLOCK / (16*(CYGNUM_HAL_VIRTUAL_VECTOR_CHANNELS_DEFAULT_BAUD))) & 0xff)

#if 0
// Define the serial registers.
#define CYG_DEV_RBR 0x00   // receiver buffer register, read, dlab = 0
#define CYG_DEV_THR 0x00   // transmitter holding register, write, dlab = 0
#define CYG_DEV_DLL 0x00   // divisor latch (LS), read/write, dlab = 1
#define CYG_DEV_IER 0x04   // interrupt enable register, read/write, dlab = 0
#define CYG_DEV_DLM 0x04   // divisor latch (MS), read/write, dlab = 1
#define CYG_DEV_IIR 0x08   // interrupt identification register, read, dlab = 0
#define CYG_DEV_FCR 0x08   // fifo control register, write, dlab = 0
#define CYG_DEV_LCR 0x0C   // line control register, read/write
#define CYG_DEV_MCR 0x10   // modem control register, read/write
#define CYG_DEV_LSR 0x14   // line status register, read
#define CYG_DEV_MSR 0x18   // modem status register, read
#else
#define CYG_DEV_RBR 0x00   // receiver buffer register, read, dlab = 0
#define CYG_DEV_THR 0x00   // transmitter holding register, write, dlab = 0
#define CYG_DEV_DLL 0x00   // divisor latch (LS), read/write, dlab = 1
#define CYG_DEV_IER 0x01   // interrupt enable register, read/write, dlab = 0
#define CYG_DEV_DLM 0x01   // divisor latch (MS), read/write, dlab = 1
#define CYG_DEV_IIR 0x02   // interrupt identification register, read, dlab = 0
#define CYG_DEV_FCR 0x02   // fifo control register, write, dlab = 0
#define CYG_DEV_LCR 0x03   // line control register, read/write
#define CYG_DEV_MCR 0x04   // modem control register, read/write
#define CYG_DEV_LSR 0x05   // line status register, read
#define CYG_DEV_MSR 0x06   // modem status register, read
#endif

// Interrupt Enable Register
#define SIO_IER_RCV 0x01
#define SIO_IER_XMT 0x02
#define SIO_IER_LS  0x04
#define SIO_IER_MS  0x08

// The line status register bits.
#define SIO_LSR_DR      0x01            // data ready
#define SIO_LSR_OE      0x02            // overrun error
#define SIO_LSR_PE      0x04            // parity error
#define SIO_LSR_FE      0x08            // framing error
#define SIO_LSR_BI      0x10            // break interrupt
#define SIO_LSR_THRE    0x20            // transmitter holding register empty
#define SIO_LSR_TEMT    0x40            // transmitter register empty
#define SIO_LSR_ERR     0x80            // any error condition

// The modem status register bits.
#define SIO_MSR_DCTS  0x01              // delta clear to send
#define SIO_MSR_DDSR  0x02              // delta data set ready
#define SIO_MSR_TERI  0x04              // trailing edge ring indicator
#define SIO_MSR_DDCD  0x08              // delta data carrier detect
#define SIO_MSR_CTS   0x10              // clear to send
#define SIO_MSR_DSR   0x20              // data set ready
#define SIO_MSR_RI    0x40              // ring indicator
#define SIO_MSR_DCD   0x80              // data carrier detect

// The line control register bits.
#define SIO_LCR_WLS0   0x01             // word length select bit 0
#define SIO_LCR_WLS1   0x02             // word length select bit 1
#define SIO_LCR_STB    0x04             // number of stop bits
#define SIO_LCR_PEN    0x08             // parity enable
#define SIO_LCR_EPS    0x10             // even parity select
#define SIO_LCR_SP     0x20             // stick parity
#define SIO_LCR_SB     0x40             // set break
#define SIO_LCR_DLAB   0x80             // divisor latch access bit

// Modem Control Register
#define SIO_MCR_DTR 0x01
#define SIO_MCR_RTS 0x02
#define SIO_MCR_INT 0x08   // Enable interrupts

#define	IIR_FIFO_MASK	0xc0	/* set if FIFOs are enabled */


#define wreg8(r,v)	(*(volatile CYG_BYTE *)(r) = (v))
#define rreg8(r)	(*(volatile CYG_BYTE *)(r))

#if (CYG_BYTEORDER == CYG_MSBFIRST)
#define READREG(r,v) (v) = rreg8(base+((r)^0x3))
#define WRITEREG(r,v) wreg8(base+((r)^0x3),(v))
#else
#define READREG(r,v) (v) = rreg8(base+r)
#define WRITEREG(r,v) wreg8((base+r),(v))    
#endif

#define WRITECSR(r,v) \
     do { cyg_uint32 temp; WRITEREG((r),(v)); temp = rreg8(0xB8000000); } while (0)


//-----------------------------------------------------------------------------
typedef struct {
    cyg_uint8* base;
    cyg_int32 msec_timeout;
    int isr_vector;
    
#ifdef UART_INTERRUPT_MODE
    /* For interrupt mode */
    cyg_handle_t    interrupt_handle;
    cyg_interrupt   interrupt_object;
    cyg_sem_t       sem_received;
    cyg_uint32      rx_read_ptr;
    cyg_uint32      rx_write_ptr;
    cyg_uint8       rx_buffer[UART_INTERRUPT_RX_BUFFER_LEN];    
#endif /* UART_INTERRUPT_MODE */

} channel_data_t;

/*
 * We currently support only one UART.
 */
static channel_data_t ns_ser_channels[1];

//-----------------------------------------------------------------------------

#ifdef UART_INTERRUPT_MODE
static cyg_uint32
ns16552_uart_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_uint8 lsr, ch;
    channel_data_t* chan = (channel_data_t*)data;
    cyg_uint8* base = chan->base;
    
    /* Check if char available */
    READREG(CYG_DEV_LSR, lsr);   
    if (lsr & SIO_LSR_DR) {
        
        /* Read the char */
        READREG(CYG_DEV_RBR, ch);
        
        /* Check if buffer full */
        if (((chan->rx_write_ptr + 1) == chan->rx_read_ptr) ||
            ((chan->rx_write_ptr == UART_INTERRUPT_RX_BUFFER_LEN - 1) &&
             (chan->rx_read_ptr == 0))) {
                
            /* Buffer full; simply drop the char */
            cyg_drv_interrupt_acknowledge(chan->isr_vector);
            return CYG_ISR_HANDLED;
        }
        
        /* Put char to buffer and advance write pointer */
        chan->rx_buffer[chan->rx_write_ptr++] = ch;
        if (chan->rx_write_ptr == UART_INTERRUPT_RX_BUFFER_LEN) {
            chan->rx_write_ptr = 0;
        }
        
        /* We have got another char; schedule DSR */
        cyg_drv_interrupt_acknowledge(chan->isr_vector);
        return CYG_ISR_CALL_DSR;
    }
    
    /* No char available; should not happen */
    cyg_drv_interrupt_acknowledge(chan->isr_vector);
    return CYG_ISR_HANDLED;
}

static void
ns16552_uart_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    /*
     * How many times interrupt serviced = how many characters available
     */
    for( ; count > 0; count--) {
        /*
         * Increase amount of resources and wake up getc thread
         */
        cyg_semaphore_post(&((channel_data_t*)data)->sem_received);
    }
}
#endif /* UART_INTERRUPT_MODE */

static void
cyg_hal_plf_serial_init_channel(void* __ch_data)
{
    cyg_uint8* base = ((channel_data_t*)__ch_data)->base;
    cyg_uint8 lcr;
    cyg_uint32 i;
    int xtal = COM1_FREQ;
    int baud = CONSOLE_BAUD_RATE;
    int divisor = ((xtal + (8 * baud)) / (16 * baud));
    /* Set baud rate and LCR to 8-1-no parity */
    lcr = SIO_LCR_DLAB;
    WRITECSR(CYG_DEV_LCR, lcr);
    WRITECSR(CYG_DEV_DLL, divisor);
    WRITECSR(CYG_DEV_DLM, divisor>>8);
    WRITECSR(CYG_DEV_LCR, SIO_LCR_WLS0 | SIO_LCR_WLS1);
    
    /* Set options */
    WRITECSR(CYG_DEV_IER, 0);
    WRITECSR(CYG_DEV_FCR, 0x01);
    /* enable fifo, reset tx and rx firo, trigger at 1 char */
    WRITECSR(CYG_DEV_FCR, 0x07);
    hal_delay_us(100);
    READREG(CYG_DEV_IIR, lcr);
    if ((lcr & IIR_FIFO_MASK) != IIR_FIFO_MASK)
        WRITECSR(CYG_DEV_FCR, 0x00);

    NS_EXTRA_INIT();
}

void
cyg_hal_plf_serial_putc(void *__ch_data, char c)
{
	cyg_uint8* base = ((channel_data_t*)__ch_data)->base;
    cyg_uint8 lsr;
    
    CYGARC_HAL_SAVE_GP();
    
#ifndef NO_NETWORK_IO
    int s = netio_get_assoicated_socket();
    if (s >= 0) {
        send(s, &c, 1, 0);
    } else
#endif /* !NO_NETWORK_IO */

    /* UART I/O */
    {
        do {
            //HAL_READ_UINT8(base+CYG_DEV_LSR, lsr);
         	READREG(CYG_DEV_LSR, lsr);   
        } while ((lsr & SIO_LSR_THRE) == 0);
    
        //HAL_WRITE_UINT8(base+CYG_DEV_THR, c);
        WRITECSR(CYG_DEV_THR, c);
    }
    
    CYGARC_HAL_RESTORE_GP();
}

#ifndef NO_NETWORK_IO
static cyg_bool
cyg_hal_plf_net_io_getc(int s, cyg_uint8* ch)
{
    struct timeval tm;
    fd_set rfds;
    tm.tv_sec = netio_timeout;
    tm.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(s, &rfds);
    if (select(FD_SETSIZE, &rfds, NULL, NULL, &tm) > 0) {
        if (FD_ISSET(s, &rfds)) {
            if (recv(s, ch, 1, 0) > 0) {
                return true;
            }
        }
    }
    
    *ch = NETIO_CHAR_ON_ERROR;
    return true;
}
#endif /* !NO_NETWORK_IO */

static cyg_bool
cyg_hal_plf_serial_getc_nonblock(void* __ch_data, cyg_uint8* ch)
{
    cyg_uint8* base = ((channel_data_t*)__ch_data)->base;
    cyg_uint8 lsr;

#ifndef NO_NETWORK_IO
    int s = netio_get_assoicated_socket();
    if (s >= 0) {
        cyg_bool r;
        r = cyg_hal_plf_net_io_getc(s, ch);
        
        if (r && *ch == TELNET_IAC) {
            
            /* Telnet escape - need to read/handle more */
            while(cyg_hal_plf_net_io_getc(s, ch) == false);
            
            switch(*ch) {

            case TELNET_IAC:
                /* The other special case - escaped escape */
                return true;

            case TELNET_IP:
                /* Special case for ^C == Interrupt Process */
                *ch = 0x03;  
                /* Just in case the other end needs synchronizing */
                cyg_hal_plf_serial_putc(__ch_data, TELNET_IAC);
                cyg_hal_plf_serial_putc(__ch_data, TELNET_WONT);
                cyg_hal_plf_serial_putc(__ch_data, TELNET_TM);
                return true;
                
            case TELNET_DO:
            case TELNET_DONT:
                /* Telnet DO option */
                while(cyg_hal_plf_net_io_getc(s, ch) == false);
                if (*ch == NETIO_CHAR_ON_ERROR) {
                    return true;
                }
                
                cyg_hal_plf_serial_putc(__ch_data, TELNET_IAC);
                if (*ch == TELNET_SGA || *ch == TELNET_ECHO) {
                    /* Agree following options in server side:
                     *  - Supress Go Ahead
                     *  - Echo
                     */  
                    cyg_hal_plf_serial_putc(__ch_data, TELNET_WILL);
                } else {
                    /* Respond with WONT option */
                    cyg_hal_plf_serial_putc(__ch_data, TELNET_WONT);
                }
                cyg_hal_plf_serial_putc(__ch_data, *ch);
                return false;  /* Ignore this whole thing! */
                
            case TELNET_WILL:
            case TELNET_WONT:
                /* Telnet WILL option */
                while(cyg_hal_plf_net_io_getc(s, ch) == false);
                if (*ch == NETIO_CHAR_ON_ERROR) {
                    return true;
                }
                
                cyg_hal_plf_serial_putc(__ch_data, TELNET_IAC);
                if (*ch == TELNET_SGA) {
                    /* Agree following options in client side:
                     *  - Supress Go Ahead
                     */  
                    cyg_hal_plf_serial_putc(__ch_data, TELNET_DO);
                } else {
                    /* Respond with DONT option */
                    cyg_hal_plf_serial_putc(__ch_data, TELNET_DONT);
                }
                cyg_hal_plf_serial_putc(__ch_data, *ch);
                return false;  /* Ignore this whole thing! */

            case NETIO_CHAR_ON_ERROR:
                return true;
                
            default:
                return false;
            }
        }
        
        /* 
         * This is most likely to happen followed by CR in UNIX mode.
         */
        if (*ch == TELNET_NUL) {
            return false;
        }
        
        return r;
    }
#endif /* !NO_NETWORK_IO */

#ifdef UART_INTERRUPT_MODE

    /* Interrupt mode */
    {
        channel_data_t* chan = (channel_data_t*)__ch_data;
        cyg_uint8 ier;
        /*
         * It seems that IER will be changed during init phase.
         * So we have to make sure it's set enabled here.
         */
       	READREG(CYG_DEV_IER, ier);   
        if ((ier & SIO_IER_RCV) == 0) {
            WRITECSR(CYG_DEV_IER, SIO_IER_RCV);
        }
        
        /*
         * Check if any character available, or wait if none.
         */
        cyg_semaphore_wait(&chan->sem_received);    
        
        if (chan->rx_write_ptr == chan->rx_read_ptr) {
            /* Buffer empty; should not happen. */
            return false;
        }
        
        /* 
         * Read char from buffer and advance read pointer 
         */
        *ch = chan->rx_buffer[chan->rx_read_ptr++];
        if (chan->rx_read_ptr == UART_INTERRUPT_RX_BUFFER_LEN) {
            chan->rx_read_ptr = 0;
        }
    }
   
#else /* !UART_INTERRUPT_MODE */

    READREG(CYG_DEV_LSR, lsr);   
    if ((lsr & SIO_LSR_DR) == 0)
        return false;

    READREG(CYG_DEV_RBR, *ch);   
    
#endif /* UART_INTERRUPT_MODE */

    return true;
}

cyg_uint8
cyg_hal_plf_serial_getc(void* __ch_data)
{
    cyg_uint8 ch;
    CYGARC_HAL_SAVE_GP();

    while(!cyg_hal_plf_serial_getc_nonblock(__ch_data, &ch));

    CYGARC_HAL_RESTORE_GP();
    return ch;
}

static void
cyg_hal_plf_serial_write(void* __ch_data, const cyg_uint8* __buf, 
                         cyg_uint32 __len)
{
	CYGARC_HAL_SAVE_GP();
    
	while(__len-- > 0)
        cyg_hal_plf_serial_putc(__ch_data, *__buf++);

	CYGARC_HAL_RESTORE_GP();
}

static void
cyg_hal_plf_serial_read(void* __ch_data, cyg_uint8* __buf, cyg_uint32 __len)
{
    CYGARC_HAL_SAVE_GP();

    while(__len-- > 0)
        *__buf++ = cyg_hal_plf_serial_getc(__ch_data);

    CYGARC_HAL_RESTORE_GP();
}

cyg_bool
cyg_hal_plf_serial_getc_timeout(void* __ch_data, cyg_uint8* ch)
{
    int delay_count;
    channel_data_t* chan = (channel_data_t*)__ch_data;
    cyg_bool res;
    CYGARC_HAL_SAVE_GP();

    delay_count = chan->msec_timeout * 10; // delay in .1 ms steps

    for(;;) {
        res = cyg_hal_plf_serial_getc_nonblock(__ch_data, ch);
        if (res || 0 == delay_count--)
            break;
        
        CYGACC_CALL_IF_DELAY_US(100);
    }

    CYGARC_HAL_RESTORE_GP();
    return res;
}

static int
cyg_hal_plf_serial_control(void *__ch_data, __comm_control_cmd_t __func, ...)
{
    static int irq_state = 0;
    channel_data_t* chan = (channel_data_t*)__ch_data;
    cyg_uint8* base = ((channel_data_t*)__ch_data)->base;
    int ret = 0;
    CYGARC_HAL_SAVE_GP();

    switch (__func) {
    case __COMMCTL_IRQ_ENABLE:
        irq_state = 1;

        //HAL_WRITE_UINT8(chan->base+CYG_DEV_IER, SIO_IER_RCV);
        WRITECSR(CYG_DEV_IER, SIO_IER_RCV);
        //HAL_WRITE_UINT8(chan->base+CYG_DEV_MCR, SIO_MCR_INT|SIO_MCR_DTR|SIO_MCR_RTS);
		WRITECSR(CYG_DEV_MCR, SIO_MCR_INT|SIO_MCR_DTR|SIO_MCR_RTS);
        HAL_INTERRUPT_UNMASK(chan->isr_vector);
        break;
    case __COMMCTL_IRQ_DISABLE:
        ret = irq_state;
        irq_state = 0;

        //HAL_WRITE_UINT8(chan->base+CYG_DEV_IER, 0);
		WRITECSR(CYG_DEV_IER, 0);
        HAL_INTERRUPT_MASK(chan->isr_vector);
        break;
    case __COMMCTL_DBG_ISR_VECTOR:
        ret = chan->isr_vector;
        break;
    case __COMMCTL_SET_TIMEOUT:
    {
        va_list ap;

        va_start(ap, __func);

        ret = chan->msec_timeout;
        chan->msec_timeout = va_arg(ap, cyg_uint32);

        va_end(ap);
    }        
    default:
        break;
    }
    CYGARC_HAL_RESTORE_GP();
    return ret;
}

static int
cyg_hal_plf_serial_isr(void *__ch_data, int* __ctrlc, 
                       CYG_ADDRWORD __vector, CYG_ADDRWORD __data)
{
    int res = 0;
    channel_data_t* chan = (channel_data_t*)__ch_data;
    cyg_uint8* base = ((channel_data_t*)__ch_data)->base;
    char c;
    cyg_uint8 lsr;
    CYGARC_HAL_SAVE_GP();

    cyg_drv_interrupt_acknowledge(chan->isr_vector);

    *__ctrlc = 0;
    //HAL_READ_UINT8(chan->base+CYG_DEV_LSR, lsr);
    READREG(CYG_DEV_LSR, lsr);
    
    if ( (lsr & SIO_LSR_DR) != 0 ) {

        //HAL_READ_UINT8(chan->base+CYG_DEV_RBR, c);
        READREG(CYG_DEV_RBR, c);
        if( cyg_hal_is_break( &c , 1 ) )
            *__ctrlc = 1;

        res = CYG_ISR_HANDLED;
    }

    CYGARC_HAL_RESTORE_GP();
    return res;
}

static void
cyg_hal_plf_serial_init(void)
{
    hal_virtual_comm_table_t* comm;
    int cur = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);

#ifndef NO_NETWORK_IO
    netio_handle_thread_index = cyg_thread_new_data_index();
#endif /* !NO_NETWORK_IO */

    // Disable interrupts.
    HAL_INTERRUPT_MASK(ns_ser_channels[0].isr_vector);
#if 0
    HAL_INTERRUPT_MASK(ns_ser_channels[1].isr_vector);
#endif
    // Init channels
    cyg_hal_plf_serial_init_channel(&ns_ser_channels[0]);
#if 0
    cyg_hal_plf_serial_init_channel(&ns_ser_channels[1]);
#endif

#ifdef UART_INTERRUPT_MODE
    /* Initialize UART interrupt */
    {
        cyg_uint8* base = ns_ser_channels[0].base;

        /* Init channel-specific data */
        ns_ser_channels[0].rx_read_ptr = 0;
        ns_ser_channels[0].rx_write_ptr = 0;
        cyg_semaphore_init(&ns_ser_channels[0].sem_received, 0);

        /* Connect interrupt */
        cyg_drv_interrupt_create(
            ns_ser_channels[0].isr_vector,
            0,
            (CYG_ADDRWORD)&ns_ser_channels[0],
            ns16552_uart_isr,
            ns16552_uart_dsr,
            &ns_ser_channels[0].interrupt_handle,
            &ns_ser_channels[0].interrupt_object);
        cyg_drv_interrupt_attach(ns_ser_channels[0].interrupt_handle);
        cyg_drv_interrupt_unmask(ns_ser_channels[0].isr_vector);
        
        /* Start UART interrupt mode (RX only) */
        WRITECSR(CYG_DEV_IER, SIO_IER_RCV);
    }
#endif /* UART_INTERRUPT_MODE */    

    // Set channel 0
    CYGACC_CALL_IF_SET_CONSOLE_COMM(0);
    comm = CYGACC_CALL_IF_CONSOLE_PROCS();
    CYGACC_COMM_IF_CH_DATA_SET(*comm, &ns_ser_channels[0]);
    CYGACC_COMM_IF_WRITE_SET(*comm, cyg_hal_plf_serial_write);
    CYGACC_COMM_IF_READ_SET(*comm, cyg_hal_plf_serial_read);
    CYGACC_COMM_IF_PUTC_SET(*comm, cyg_hal_plf_serial_putc);
    CYGACC_COMM_IF_GETC_SET(*comm, cyg_hal_plf_serial_getc);
    CYGACC_COMM_IF_CONTROL_SET(*comm, cyg_hal_plf_serial_control);
    CYGACC_COMM_IF_DBG_ISR_SET(*comm, cyg_hal_plf_serial_isr);
    CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, cyg_hal_plf_serial_getc_timeout);

#if 0    
    // Set channel 1
    CYGACC_CALL_IF_SET_CONSOLE_COMM(1);
    comm = CYGACC_CALL_IF_CONSOLE_PROCS();
    CYGACC_COMM_IF_CH_DATA_SET(*comm, &ns_ser_channels[1]);
    CYGACC_COMM_IF_WRITE_SET(*comm, cyg_hal_plf_serial_write);
    CYGACC_COMM_IF_READ_SET(*comm, cyg_hal_plf_serial_read);
    CYGACC_COMM_IF_PUTC_SET(*comm, cyg_hal_plf_serial_putc);
    CYGACC_COMM_IF_GETC_SET(*comm, cyg_hal_plf_serial_getc);
    CYGACC_COMM_IF_CONTROL_SET(*comm, cyg_hal_plf_serial_control);
    CYGACC_COMM_IF_DBG_ISR_SET(*comm, cyg_hal_plf_serial_isr);
    CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, cyg_hal_plf_serial_getc_timeout);
#endif

    // Restore original console
    CYGACC_CALL_IF_SET_CONSOLE_COMM(cur);
}

void
cyg_hal_plf_serial_add(void *regs, cyg_uint32 irq, cyg_uint32 baud_base, cyg_uint32 reg_shift)
{
	ns_ser_channels[num_tty].base = regs;
    ns_ser_channels[num_tty].isr_vector = irq;
    num_tty++;
}

void
cyg_hal_plf_comms_init(void)
{
    static int initialized = 0;

    if (initialized)
        return;

    initialized = 1;

    cyg_hal_plf_serial_init();
}

//-----------------------------------------------------------------------------

//=============================================================================
// Compatibility with older stubs
//=============================================================================

#ifndef CYGSEM_HAL_VIRTUAL_VECTOR_DIAG

#ifdef CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS
#include <cyg/hal/hal_stub.h>           // cyg_hal_gdb_interrupt
#endif

#if CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL==0
// This is the base address of the A-channel
#define CYG_DEV_SERIAL_BASE      0x1d000020
#define CYG_DEVICE_SERIAL_INT    CYGNUM_HAL_INTERRUPT_SERIALA
#else
// This is the base address of the B-channel
#define CYG_DEV_SERIAL_BASE      0x1d000000
#define CYG_DEVICE_SERIAL_INT    CYGNUM_HAL_INTERRUPT_SERIALB
#endif

static channel_data_t ns_ser_channel = {
    (cyg_uint8*)CYG_DEV_SERIAL_BASE, 0, 0 
};

// Assumption: all diagnostic output must be GDB packetized unless this is a ROM (i.e.
// totally stand-alone) system.

#if defined(CYG_HAL_STARTUP_ROM) || !defined(CYGDBG_HAL_DIAG_TO_DEBUG_CHAN)
#define HAL_DIAG_USES_HARDWARE
#endif


#ifndef HAL_DIAG_USES_HARDWARE
#if (CYGNUM_HAL_VIRTUAL_VECTOR_CONSOLE_CHANNEL != CYGNUM_HAL_VIRTUAL_VECTOR_DEBUG_CHANNEL)
#define HAL_DIAG_USES_HARDWARE
#endif
#endif

#ifdef HAL_DIAG_USES_HARDWARE

void hal_diag_init(void)
{
    static int init = 0;
    char *msg = "\n\rARM eCos\n\r";

    if (init++) return;

    cyg_hal_plf_serial_init_channel(&ns_ser_channel);

    while (*msg) cyg_hal_plf_serial_putc(&ns_ser_channel, *msg++);
}

#ifdef DEBUG_DIAG
#if defined(CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS)
#define DIAG_BUFSIZE 32
#else
#define DIAG_BUFSIZE 2048
#endif
static char diag_buffer[DIAG_BUFSIZE];
static int diag_bp = 0;
#endif

void hal_diag_write_char(char c)
{
    hal_diag_init();

    cyg_hal_plf_serial_putc(&ns_ser_channel, c);

#ifdef DEBUG_DIAG
    diag_buffer[diag_bp++] = c;
    if (diag_bp == DIAG_BUFSIZE) diag_bp = 0;
#endif
}

void hal_diag_read_char(char *c)
{
    *c = cyg_hal_plf_serial_getc(&ns_ser_channel);
}

#else // HAL_DIAG relies on GDB

// Initialize diag port - assume GDB channel is already set up
void hal_diag_init(void)
{
    if (0) cyg_hal_plf_serial_init_channel(&ns_ser_channel); // avoid warning
}

// Actually send character down the wire
static void
hal_diag_write_char_serial(char c)
{
    hal_diag_init();

    cyg_hal_plf_serial_putc(&ns_ser_channel, c);
}

static bool
hal_diag_read_serial(char *c)
{
    long timeout = 1000000000;  // A long time...
    while (!cyg_hal_plf_serial_getc_nonblock(&ns_ser_channel, c))
        if (0 == --timeout) return false;

    return true;
}

void 
hal_diag_read_char(char *c)
{
    while (!hal_diag_read_serial(c)) ;
}

void 
hal_diag_write_char(char c)
{
    static char line[100];
    static int pos = 0;
    
    // No need to send CRs
    if( c == '\r' ) return;

    line[pos++] = c;

    if( c == '\n' || pos == sizeof(line) )
    {
        CYG_INTERRUPT_STATE old;

        // Disable interrupts. This prevents GDB trying to interrupt us
        // while we are in the middle of sending a packet. The serial
        // receive interrupt will be seen when we re-enable interrupts
        // later.
        
#ifdef CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS
        CYG_HAL_GDB_ENTER_CRITICAL_IO_REGION(old);
#else
        HAL_DISABLE_INTERRUPTS(old);
#endif

        while(1)
        {
            static char hex[] = "0123456789ABCDEF";
            cyg_uint8 csum = 0;
            int i;
            char c1;
        
            hal_diag_write_char_serial('$');
            hal_diag_write_char_serial('O');
            csum += 'O';
            for( i = 0; i < pos; i++ )
            {
                char ch = line[i];
                char h = hex[(ch>>4)&0xF];
                char l = hex[ch&0xF];
                hal_diag_write_char_serial(h);
                hal_diag_write_char_serial(l);
                csum += h;
                csum += l;
            }
            hal_diag_write_char_serial('#');
            hal_diag_write_char_serial(hex[(csum>>4)&0xF]);
            hal_diag_write_char_serial(hex[csum&0xF]);

            // Wait for the ACK character '+' from GDB here and handle
            // receiving a ^C instead.  This is the reason for this clause
            // being a loop.
            if (!hal_diag_read_serial(&c1))
                continue;   // No response - try sending packet again

            if( c1 == '+' )
                break;              // a good acknowledge

#ifdef CYGDBG_HAL_DEBUG_GDB_BREAK_SUPPORT
            cyg_drv_interrupt_acknowledge(CYG_DEVICE_SERIAL_INT);
            if( c1 == 3 ) {
                // Ctrl-C: breakpoint.
                cyg_hal_gdb_interrupt ((target_register_t)__builtin_return_address(0));
                break;
            }
#endif
            // otherwise, loop round again
        }
        
        pos = 0;

        // And re-enable interrupts
#ifdef CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS
        CYG_HAL_GDB_LEAVE_CRITICAL_IO_REGION(old);
#else
        HAL_RESTORE_INTERRUPTS(old);
#endif
        
    }
}
#endif

#endif // CYGSEM_HAL_VIRTUAL_VECTOR_DIAG

//-----------------------------------------------------------------------------
// End of ns16552.c
