#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

main(){
	char msg[32];
	int device_fd;
	int i;

	device_fd=open("/dev/junk",O_RDONLY);
	for(i=0;i<32;i++){
		read(device_fd,&msg,32);
		printf("got: %s\n",msg);
		sleep(1);
	}
	return 0;
}
