#include <rtl.h>
#include <rtl_fifo.h>
#include <time.h>
#include <rtl_sched.h>
#include <rtl_sync.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
//#include <stdarg.h>
void user_printf(const char *fmt,...);
int OUT;
#define eprint rtl_printf  // so I could debug this code in user space
#define BUFSIZE 500
char  buf[BUFSIZE];

int init_module(void){
	
	rtf_destroy(0);
	if ( (OUT = open("/dev/rtf0",O_WRONLY | O_NONBLOCK | O_CREAT )) < 0)
	{
		rtl_printf("Example cannot open fifo\n");
		rtl_printf("Error number is %d\n",errno);
		return -1;
	}
	user_printf("Test from init code\n"); //DEBUG
	return 0;
}


void cleanup_module(void){ 
	close(OUT);
}



static int fixcopy(int space, char *dest, void *v,int sz){
	char *c = (char *)v;
	if(sz > space){
		eprint("RTL error: user_printf out of space\n");
		return -1;
       	}
	else{
		while(sz > 0){*dest++ = *c++; sz--;}
	}
	return 0;
}

static int safecopy(int space, char *dest, const char *src){
	int len = strlen(src)+1;
	if(len > space){
		eprint("RTL error: Can't user_printf very long strings\n");
		return -1;
       	}
	strncpy(dest,src,len);
	return len;
}
void user_printf(const char *fmt,...){
       	int len,i,d,l;
       	char *s;
       	double f;
       	va_list ap;
       	/* dump the character string */
       	if ( (len = safecopy(BUFSIZE, buf,fmt)) < 0)return;
       	/* then copy the characters */
       	va_start(ap, fmt);
       	for(i=0; i < strlen(fmt); i++){
	       	if(fmt[i] == '%') switch(fmt[i+1]) {
		       	case 's':           /* string */
			       	s = va_arg(ap, char *);
			       	l = safecopy(BUFSIZE -len, buf+len,s);
			       	if(l<0)return;
			       	len += l;

			       	break;
		       	case 'd':           /* int */
		       	case 'c':           /* char is in an int */
			       	d = va_arg(ap, int);
			       	if(fixcopy(BUFSIZE-len, buf+len, &d,sizeof(d)))
					return;
			       	len += sizeof(d);
			       	break;
		       	case 'f':           /* double */
			       	f = va_arg(ap, double);
			       	if(fixcopy(BUFSIZE-len, buf+len, &f,sizeof(f)))
					return;
			       	len += sizeof(f);
			       	break;
	       	}

	}
	rtl_printf("user printf writes %d bytes\n",len);//DEBUG
	write(OUT,&len,sizeof(len));
	write(OUT,buf,len);
}
