#ifndef _IODRIVER_H_
#define _IODRIVER_H_

typedef void (f_drv_write_func)(int, unsigned char);
typedef unsigned char (f_drv_read_func)(int);

typedef struct io_driver
{
   f_drv_write_func * const write;
   f_drv_read_func * const read;
}
io_driver;

extern io_driver IODriver;

#endif
