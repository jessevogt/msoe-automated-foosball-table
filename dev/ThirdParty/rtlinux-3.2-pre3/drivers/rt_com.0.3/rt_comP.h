/**
 * rt_com
 * ======
 *
 * RT-Linux kernel module for communication across serial lines.
 *
 * Copyright (C) 1997 Jens Michaelsen
 * Copyright (C) 1997-1999 Jochen Küpper
 * Copyright (C) 1999 Hua Mao <hmao@nmt.edu>
 */



#ifndef RT_COM_P_H
#define RT_COM_P_H



/* private functions */
#ifdef RTLINUX_V1
static void rt_com0_isr( void );
static inline void rt_com_isr( unsigned int );
#else
static unsigned int rt_com0_isr( unsigned int, struct pt_regs * );
static unsigned int rt_com_isr( unsigned int, struct pt_regs * );
#endif


/* number of bytes in port FIFO when a DATA_READY interrupt shall occur */
#define FIFO_TRIGGER 8


/* status masks */
#define DATA_READY   0x01     /* not an error               */
#define OVERRUN      0x02     /* error detected by hardware */
#define PARITY       0x04     /* error detected by hardware */
#define FRAME        0x08     /* error detected by hardware */
#define BREAK        0x10     /* not an error               */
#define BUFFER_FULL  0x80     /* error detected by software */
#define TXB_EMPTY    0x20     /* not an error               */
#define HARD_ERROR   (OVERRUN | PARITY | FRAME | BREAK)


/* port register offsets */
#define RT_COM_RXB  0x00
#define RT_COM_TXB  0x00
#define RT_COM_IER  0x01
#define RT_COM_IIR  0x02
#define RT_COM_FCR  0x02
#define RT_COM_LCR  0x03
#define RT_COM_MCR  0x04
#define RT_COM_LSR  0x05
#define RT_COM_MSR  0x06
#define RT_COM_DLL  0x00
#define RT_COM_DLM  0x01


/* bit masks which may be written to the MCR using ModemControl */
#define RT_COM_DTR          0x01     /* data Terminal Ready        */
#define RT_COM_RTS          0x02     /* Request To Send            */
#define RT_COM_Out1         0x04
#define RT_COM_Out2         0x08
#define RT_COM_LoopBack     0x10


/* data buffer - organized as a FIFO */
struct rt_buf_struct{
	int  head;
	int  tail;
	char buf[ RT_COM_BUF_SIZ ];
};


#ifdef CONFIG_SMP
spinlock_t rt_com_spinlock = SPIN_LOCK_UNLOCKED;
#define RTL_SPIN_LOCK rt_com_spinlock
#endif

#define rt_com_irq_off( state )         rtl_critical( state )
#define rt_com_irq_on(state)            rtl_end_critical( state )

/* information about handled ports */
struct rt_com_struct{
	int magic;
	int baud_base;
	int port;
	int irq;
	int flag;
#ifdef RTLINUX_V1
	void (*isr)(void);
#else
	unsigned int (*isr)( unsigned int num, struct pt_regs *r);
#endif
	int type;
  	int ier;  /* copy of chip register */
	struct rt_buf_struct ibuf;
	struct rt_buf_struct obuf;
};



/*-----------------------------------------------------------------------*/
#define BASE_BAUD 115200
#define STD_COM_FLAG 0


#endif /* RT_COM_P_H */



/**
 * Local Variables:
 * mode: C
 * c-file-style: "Stroustrup"
 * End:
 */
