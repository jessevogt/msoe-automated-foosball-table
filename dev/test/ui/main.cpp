#include <iostream>
#include "mbuff.h"
#include <stdio.h>

volatile char * buf;

int main(int argc, char** argv) {
   std::cout << "Hello world" << std::endl;

   buf = (volatile char *)mbuff_alloc("ballPos",0x100000);
   
   printf("whote %s\n",(char*)buf);

   mbuff_free("buf",(void*)buf);
   return 0;
}
