#include <asm/io.h>
#include "IODriver.h"

static void write(int address, unsigned char data);
static unsigned char read(int);

io_driver IODriver = {&write,
                      &read};

void write(int address, unsigned char data)
{
   outb(address, data);
   return;
}

unsigned char read(int address)
{
   //return inb(address);
   return 0xaa;
}
