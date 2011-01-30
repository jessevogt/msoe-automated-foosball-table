/**
 * rt_com
 * ======
 *
 * RT-Linux kernel module for communication across serial lines.
 *
 * Copyright (C) 1997 Jens Michaelsen
 * Copyright (C) 1997-1999 Jochen Küpper
 * Copyright (C) 1999 Hua Mao <hmao@nmt.edu>
 *
 * POSIX IO and new init functions by Michael Barabanov
 */

 

/* define this to be a RT-Linux kernel module */
#ifdef RTLINUX_V1
#define __KERNEL__
#define __RT__
#define MODULE
#endif


#include <linux/config.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include <asm/system.h>
#include <asm/io.h>


#ifdef RTLINUX_V1
#include <asm/rt_irq.h>
#include <asm/rt_time.h>
#else
#include <rtl_conf.h>
#include <rtl_core.h>
#include <rtl_sync.h>
#endif

#include "rt_com.h"
#include "rt_comP.h"



/**
 * Number and descriptions of serial ports to manage.
 * You also need to create an ISR ( rt_comN_isr() ) for each port.
 */
#define RT_COM_CNT 1
struct rt_com_struct rt_com_table[ RT_COM_CNT ] =
{
	{ 0, BASE_BAUD, 0x3f8, 4, STD_COM_FLAG, rt_com0_isr },
};




/**
 * Write data to a line.
 * @param com Port to use corresponding to internal numbering scheme.
 * @param ptr Address of data.
 * @param cnt Number of bytes to write.
 *
 * @author Jens Michaelsen, Jochen Küpper
 * @version 1999/07/20
 */
void rt_com_write( unsigned int com, char *ptr, int cnt )
{
	if( com < RT_COM_CNT ) {
		struct rt_com_struct *p = &( rt_com_table[ com ] );
		struct rt_buf_struct *b = &( p->obuf );
		long state;
		rt_com_irq_off( state );
		while( --cnt >= 0 ) {
			/* put byte into buffer, move pointers to next elements */
			b->buf[ b->head++ ] = *ptr++;
			/* if( head == RT_COM_BUF_SIZ ), wrap head to the first buffer element */
			b->head &= ( RT_COM_BUF_SIZ - 1 );
		}
		p->ier |= 0x02;
		outb( p->ier, p->port + RT_COM_IER );
		rt_com_irq_on( state );
	}
}



/**
 * Read data we got from a line.
 * @param com Port to use corresponding to internal numbering scheme.
 * @param ptr Address of data buffer. Needs to be of size > cnt !
 * @param cnt Number of bytes to read.
 * @return Number of bytes actually read.
 *
 * @author Jens Michaelsen, Jochen Küpper
 * @version 1999/07/20
 */
int rt_com_read( unsigned int com, char *ptr, int cnt )
{
	int done = 0;
	if( com < RT_COM_CNT ) {
		struct rt_com_struct *p = &( rt_com_table[ com ] );
		struct rt_buf_struct *b = &( p->ibuf );
		long state;
		rt_com_irq_off (state);
		while( ( b->head != b->tail ) && ( --cnt >= 0 ) ) {
			done++;
			*ptr++ = b->buf[ b->tail++ ];
			b->tail &= ( RT_COM_BUF_SIZ - 1 );
		}
		rt_com_irq_on (state);
	}
	return( done );
}



/**
 * Get first byte from the write buffer.
 * @param p rt_com_struct of the line we are writing to.
 * @param c Address to put the char in.
 * @return  Number of characters we actually got.
 * 
 * @author Jens Michaelsen, Jochen Küpper
 * @version 1999/07/20
 */
static inline int rt_com_irq_get( struct rt_com_struct *p, unsigned char *c )
{
	struct rt_buf_struct *b = &( p->obuf );
	long state;
	rt_com_irq_off(state);
	if( b->head != b->tail ) {
		*c = b->buf[ b->tail++ ];
		b->tail &= ( RT_COM_BUF_SIZ - 1 );
		rt_com_irq_on(state);
		return( 1 );
	}
	rt_com_irq_on(state);
	return( 0 );
}



/**
 * Concatenate a byte to the read buffer.
 * @param p  rt_com_struct of the line we are writing to.
 * @param ch Byte to put into buffer.
 *
 * @author Jens Michaelsen, Jochen Küpper
 * @version 1999/07/20
 */
static inline void rt_com_irq_put( struct rt_com_struct *p, unsigned char ch )
{
	struct rt_buf_struct *b = &( p->ibuf );
	long state;
	rt_com_irq_off(state);
	b->buf[ b->head++ ] = ch;
	b->head &= ( RT_COM_BUF_SIZ - 1 );
	rt_com_irq_on(state);
}



/**
 * Registered interrupt handler. Calls the general interrupt handler for
 * the current line to do the work.
 *
 * @author Jens Michaelsen, Jochen Küpper, Hua Mao
 * @version 1999/07/26
 */
#ifdef RTLINUX_V1
static void rt_com0_isr( void )
{
	rt_com_isr( 0 );
}
#else
static unsigned int rt_com0_isr( unsigned int num, struct pt_regs *r)
{
	return rt_com_isr(0, NULL);
}
#endif



/**
 * Real interrupt handler.
 * Called by the registered ISRs to do the actual work.
 * @param com Port to use corresponding to internal numbering scheme.
 *
 * @author Jens Michaelsen, Jochen Küpper, Hua Mao
 * @version 1999/07/26
 */
#ifdef RTLINUX_V1
static inline void rt_com_isr( unsigned int com )
#else
unsigned int rt_com_isr( unsigned int com, struct pt_regs *r )
#endif
{
	struct rt_com_struct *p = &(rt_com_table[com]);
	unsigned int B = p->port;
	unsigned char data, isr, sta;
	char loop = 4;
  
	do {
		/* get available data from port */
		sta = inb(B+RT_COM_LSR);
		while(sta & DATA_READY) {
			data = inb(B+RT_COM_RXB);
			rt_com_irq_put(p,data);
			sta = inb(B+RT_COM_LSR);
		};
		/* if possible, put data to port */
/* 		sta = inb(B+RT_COM_MSR); */
/* 		rtl_printf("%x\n", sta); */
		if(sta & 0x20) {
			/* Data Set Ready */
			if( rt_com_irq_get( p, &data ) ) {
				/* data in output buffer */
				do {
					outb(data,B+RT_COM_TXB);
					sta = inb(B+RT_COM_LSR);
				} while(--loop>0 && rt_com_irq_get(p,&data));
			} else {
				/* no data in output buffer, disable Transmitter Holding Register Empty Interrupt */
				p->ier &= ~0x02;
				outb( p->ier, B + RT_COM_IER );
			}
			sta = inb(B+RT_COM_LSR);
		};
		isr = inb(B+RT_COM_IIR) & 0x0F;
		/* check the low nibble of IIR wether there is another pending interrupt */
	} while( isr && ( --loop > 0 ) );

#ifdef RTLINUX_V1
	return;
#else	
	rtl_hard_enable_irq(p->irq);
	return 0;
#endif
}



/**
 * Enable hardware fifo buffers. This requires an 16550A or better !
 * @param base    Port base address to work on.
 * @param trigger Bytecount to buffer before an interrupt.
 *
 * @author Jochen Küpper
 * @version 1999/07/20
 */
static inline void enable_fifo( int base, int trigger )
{
	switch ( trigger ) {
	case  0:
		outb( 0x00, base + RT_COM_FCR );
		break;
	case  1:
		outb( 0x01, base + RT_COM_FCR );
		break;
	case  4:
		outb( 0x41, base + RT_COM_FCR );
		break;
	case  8:
		outb( 0x81, base + RT_COM_FCR );
		break;
	case 14:
		outb( 0xC1, base + RT_COM_FCR );
		break;
	default:
		break;
	}
	return;
}



/**
 * Calls from init_module + cleanup_module have baud == 0; in these cases we
 * only do some cleanup.
 *
 * To allocate a port, give usefull setup parameter, to deallocate give negative
 * baud.
 * @param com        Number corresponding to internal port numbering scheme.
 *                   This is esp. the index of the rt_com_table to use.
 * @param baud       Data transmission rate to use [Byte/s].
 * @param parity     Parity for transmission protocol.
 *                   (RT_COM_PARITY_EVEN, RT_COM_PARITY_ODD or RT_COM_PARITY_NONE)
 * @param stopbits   Number of stopbits to use. 1 gives you one stopbit, 2
 *                   actually gives really two stopbits for wordlengths of
 *                   6 - 8 bit, but 1.5 stopbits for a wordlength of 5 bits.
 * @param wordlength Number of bits per word (5 - 8 bits).
 *
 * @author Jens Michaelsen, Jochen Küpper
 * @version 1999/07/20
 */
void rt_com_setup( unsigned int com, int baud, unsigned int parity,
				   unsigned int stopbits, unsigned int wordlength )
{
	struct rt_com_struct  *p = &( rt_com_table[ com ] );
	unsigned int base = p->port, divider, par = parity;

	/* Stop everything, set DLAB */
	outb( 0x00, base + RT_COM_IER );
	outb( 0x80, base + RT_COM_LCR );

	/* clear irq */
	inb( base + RT_COM_IIR );
	inb( base + RT_COM_LSR );
	inb( base + RT_COM_RXB );
	inb( base + RT_COM_MSR );

	if( 0 == baud ) {
		/* return */
	} else if( 0 > baud ) {
		MOD_DEC_USE_COUNT;
	} else {
		MOD_INC_USE_COUNT;
		divider = p->baud_base / baud;
		outb( divider % 256, base + RT_COM_DLL );
		outb( divider / 256, base + RT_COM_DLM );
		/* bits 3,4 + 5 determine parity */
		if( par & 0xC7 )
			par = RT_COM_PARITY_NONE;
		/* set transmission parameters and clear DLAB */
		outb( ( wordlength - 5 ) + ( ( stopbits - 1 ) << 2 ) + par, base + RT_COM_LCR );
		outb( RT_COM_DTR + RT_COM_RTS +  RT_COM_Out1 + RT_COM_Out2, base + RT_COM_MCR );
		p->ier = 0x01;
		outb( p->ier, base + RT_COM_IER );
		enable_fifo( base, FIFO_TRIGGER );
	}
	return;
}


#ifdef CONFIG_RTL_POSIX_IO

#define RT_COM_MAJOR 4

#include <rtl_posixio.h>

static int rtl_rt_com_open (struct rtl_file *filp)
{
	if (!(filp->f_flags & O_NONBLOCK)) {
		return -EACCES; /* TODO: implement blocking IO */
	}
	if ((unsigned) RTL_MINOR_FROM_FILEPTR(filp) >= RT_COM_CNT) {
		return -ENODEV;
	}
	return 0;
}

static int rtl_rt_com_release (struct rtl_file *filp)
{
	return 0;
}

static ssize_t rtl_rt_com_write(struct rtl_file *filp, const char *buf, size_t count, loff_t* ppos)
{
/*	if (rt_com_table[RTL_MINOR_FROM_FILEPTR(filp)].type == 0) {
		return -ENODEV;
	} */
	rt_com_write(RTL_MINOR_FROM_FILEPTR(filp), (char *) buf, count);
	return count;
	
}

static ssize_t rtl_rt_com_read(struct rtl_file *filp, char *buf, size_t count, loff_t* ppos)
{
/*	if (rt_com_table[RTL_MINOR_FROM_FILEPTR(filp)].type == 0) {
		return -ENODEV;
	} */
	return rt_com_read(RTL_MINOR_FROM_FILEPTR(filp), buf, count);
	
}

static struct rtl_file_operations rtl_rt_com_fops = {
       	NULL,
	rtl_rt_com_read,
	rtl_rt_com_write,
	NULL,
	NULL,
	rtl_rt_com_open,
	rtl_rt_com_release
};
#endif


/**
 * Request port memory and register ISRs, if we cannot get the memory of all
 * ports, release all already requested ports and return an error.
 * @return Success status, zero on success.
 *
 * @author Jochen Küpper, Hua Mao
 * @version 1999/07/28
 */
int init_module( void )
{
	struct rt_com_struct *p;
	int errorcode = 0, i, j;
	for( i=0; i<RT_COM_CNT; i++ ) {
		p = &( rt_com_table[ i ] );
		if(-EBUSY == check_region( p->port, 8 ) ) {
			errorcode = 1;
			break;
		}
		request_region( p->port, 8, "rt_com" );
#ifdef RTLINUX_V1
		request_RTirq( p->irq,p->isr );
#else
		rtl_request_irq( p->irq, p->isr );
		rtl_hard_enable_irq( p->irq );
#endif
		rt_com_setup( i, 0, 0, 0, 0 );
	}
	if( 0 == errorcode ) {
		printk( KERN_INFO "rt_com sucessfully loaded.\n" );
	} else {
		printk( KERN_WARNING "rt_com: cannot request all port regions,\nrt_com: giving up.\n" );
		for( j=0; j<i; j++ ) {
#ifdef RTLINUX_V1
			free_RTirq( p->irq );
#else
			rtl_free_irq(p->irq);
#endif
			release_region( p->port, 8 );
		}
	}
#ifdef CONFIG_RTL_POSIX_IO
	if (rtl_register_chrdev (RT_COM_MAJOR, "ttyS", &rtl_rt_com_fops)) {
		printk ("RT-COM: unable to get RTL major %d\n", RT_COM_MAJOR);
		return -EIO;
	}
#endif
	return( errorcode );
}



/**
 * Unregister ISR and releases memory for all ports
 *
 * @author Jochen Küpper, Hua Mao
 * @version 1999/07/26
 */ 
void cleanup_module( void )
{
	struct rt_com_struct *p;
	int i;
#ifdef CONFIG_RTL_POSIX_IO
	rtl_unregister_chrdev(RT_COM_MAJOR, "ttyS");
#endif
	for( i=0; i<RT_COM_CNT; i++ ) {
		p = &( rt_com_table[ i ] );
#ifdef RTLINUX_V1
		free_RTirq( p->irq );
#else
		rtl_free_global_irq(p->irq);
#endif
		rt_com_setup( i, 0, 0, 0, 0 );
		release_region( p->port, 8 );
	}
	printk( KERN_INFO "rt_com unloaded.\n" );
	return;
}




/**
 * Local Variables:
 * mode: C
 * c-file-style: "Stroustrup"
 * End:
 */
