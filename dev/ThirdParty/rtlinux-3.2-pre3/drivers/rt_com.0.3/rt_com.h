/**
 * rt_com
 * ======
 *
 * RT-Linux kernel module for communication across serial lines.
 *
 * Copyright (C) 1997 Jens Michaelsen
 * Copyright (C) 1997-1999 Jochen Küpper
 */



#ifndef RT_COM_H
#define RT_COM_H



extern void cleanup_module( void );
extern int  init_module( void );
extern int  rt_com_read( unsigned int, char *, int );
extern void rt_com_setup( unsigned int, int, unsigned int, unsigned int, unsigned int );
extern void rt_com_write( unsigned int, char *, int );


#define RT_COM_PARITY_EVEN  0x18
#define RT_COM_PARITY_NONE  0x00
#define RT_COM_PARITY_ODD   0x08


/* size of internal queues */
#define RT_COM_BUF_SIZ 256



#endif /* RT_COM_H */


/**
 * Local Variables:
 * mode: C
 * c-file-style: "Stroustrup"
 * End:
 */
