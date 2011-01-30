
include ../../rtl.mk

#As an example:
RTLINUXPATH=$(RTL_DIR)
LINUXPATH=$(RTLINUX_DIR)


CFLAGS = -D__KERNEL__ -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -pipe -mpreferred-stack-boundary=2 -march=i586 -DMODULE -DMODVERSIONS -D__RTL__ -D_LOOSE_KERNEL_NAMES -O2 -I$(LINUXPATH)/include -I$(RTLINUXPATH)/include -I$(RTLINUXPATH)/include/compat -I$(RTLINUXPATH)/include/posix


CCDEP=gcc
CC=gcc
MODULEFLAGS= -Wall -O2 -fomit-frame-pointer -DMODULE -D__KERNEL__ 
RTLFLAGS= $(CFLAGS)
RTL_LWIPFLAGS= -Wall -DIPv4  
LDFLAGS=-lpcap
ARFLAGS=rs
RTL_LWIP_DIR=./src
TOOLSDIR= ./rtl_lwip_tools
COREDIR=$(RTL_LWIP_DIR)/core
RTLDIR=$(RTL_LWIP_DIR)/arch/RTLinux
ARESDIR=./ares-1.1.1-lwip


RTL_LWIPFLAGS:=$(RTL_LWIPFLAGS) \
	-I$(RTL_LWIP_DIR)/include -I$(RTL_LWIP_DIR)/arch/RTLinux/include \
	-I$(RTL_LWIP_DIR)/include/ipv4 	-Iapps -I. -I$(TOOLSDIR) -I$(ARESDIR)

COREFILES=$(COREDIR)/netif.c \
	$(COREDIR)/mem.c $(COREDIR)/memp.c \
	$(COREDIR)/pbuf.c $(COREDIR)/stats.c \
        $(COREDIR)/tcp.c $(COREDIR)/tcp_in.c \
        $(COREDIR)/tcp_out.c $(COREDIR)/udp.c $(COREDIR)/inet.c 
CORE4FILES=$(COREDIR)/ipv4/icmp.c $(COREDIR)/ipv4/ip.c \
	$(COREDIR)/ipv4/ip_addr.c $(COREDIR)/ipv4/ip_frag.c
TOOLFILES=$(TOOLSDIR)/gettimeofday.c $(TOOLSDIR)/memcopy.c \
	$(TOOLSDIR)/bzero.c \
	$(TOOLSDIR)/strtol.c \
	$(TOOLSDIR)/rt_pci.c $(TOOLSDIR)/time_debug.c
APIFILES=$(RTL_LWIP_DIR)/api/api_lib.c $(RTL_LWIP_DIR)/api/api_msg.c $(RTL_LWIP_DIR)/api/tcpip.c \
	$(RTL_LWIP_DIR)/api/err.c $(RTL_LWIP_DIR)/api/sockets.c 
NETIFFILES=$(RTL_LWIP_DIR)/netif/etharp.c $(RTL_LWIP_DIR)/netif/loopif.c 
ARCHFILES=$(RTLDIR)/sys_arch.c 
NETFILES=$(RTLDIR)/netif/rt_3c905x.c $(RTLDIR)/netif/rtfifoif.c 
ARESFILES=$(ARESDIR)/ares__close_sockets.c $(ARESDIR)/ares_compat.c \
	$(ARESDIR)/ares_fds.c $(ARESDIR)/ares_gethostbyaddr.c \
	$(ARESDIR)/ares_gethostbyname.c $(ARESDIR)/ares_init.c \
	$(ARESDIR)/ares_process.c $(ARESDIR)/ares_search.c \
	$(ARESDIR)/ares_strerror.c $(ARESDIR)/ares_timeout.c \
	$(ARESDIR)/ares_expand_name.c $(ARESDIR)/netdb.c \
	$(ARESDIR)/ares_query.c	$(ARESDIR)/ares_parse_a_reply.c \
	$(ARESDIR)/ares_destroy.c $(ARESDIR)/ares_send.c \
	$(ARESDIR)/ares_parse_ptr_reply.c $(ARESDIR)/ares_free_hostent.c \
	$(ARESDIR)/ares_mkquery.c $(ARESDIR)/ares_free_string.c \
	$(ARESDIR)/ares_free_errmem.c

COREFILESW=$(wildcard $(COREFILES))
CORE4FILESW=$(wildcard $(CORE4FILES))
TOOLFILESW=$(wildcard $(TOOLFILES))
APIFILESW=$(wildcard $(APIFILES))
NETIFFILESW=$(wildcard $(NETIFFILES))
ARCHFILESW=$(wildcard $(ARCHFILES))
NETFILESW=$(wildcard $(NETFILES))
ARESFILESW=$(wildcard $(ARESFILES))

COREOBJS=$(notdir $(COREFILESW:.c=.o))
CORE4OBJS=$(notdir $(CORE4FILESW:.c=.b))
TOOLOBJS=$(notdir $(TOOLFILESW:.c=.j))
APIOBJS=$(notdir $(APIFILESW:.c=.k))
NETIFOBJS=$(notdir $(NETIFFILESW:.c=.l))
ARCHOBJS=$(notdir $(ARCHFILESW:.c=.m))
NETOBJS=$(notdir $(NETFILESW:.c=.i))
ARESOBJS=$(notdir $(ARESFILESW:.c=.p))

RTL_LWIPOBJS=$(COREOBJS) $(CORE4OBJS) $(TOOLOBJS) $(APIOBJS) $(NETIFOBJS) \
	$(ARCHOBJS) $(NETOBJS) $(ARESOBJS)


%.o:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(COREDIR)/$(@:.o=.c)
%.b:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(COREDIR)/ipv4/$(@:.b=.c)
%.j:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(TOOLSDIR)/$(@:.j=.c)
%.k:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(RTL_LWIP_DIR)/api/$(@:.k=.c)
%.l:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(RTL_LWIP_DIR)/netif/$(@:.l=.c)
%.m:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(RTLDIR)/$(@:.m=.c)
%.i:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(RTLDIR)/netif/$(@:.i=.c)
%.p:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(ARESDIR)/$(@:.p=.c)


all ipv4 compile: stack three_com tools
.PHONY: all

clean:
	@rm -f *.o 



tcpclient: ./examples/sequential_api/tcp_echo/tcpclient.c
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/sequential_api/tcp_echo/$@.c

sock_tcp_client: ./examples/sockets_api/sock_tcp_client.c
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/sockets_api/$@.c

httpd.c:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/raw_api/http/$@ -o http.o
fs:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/raw_api/http/$@.c
fsdata:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/http/raw_api/$@.c

httpd: fs httpd.c
	@ld -i -o httpd.o http.o fs.o; rm http.o fs.o; 

net_policy:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(RTL_LWIP_DIR)/arch/RTLinux/netif/net_policy/FIFO_policy.c

three_com: rt_3c905x_phys net_policy
	@ld -E -i -o rt_3c905x_phys.o three_com.o FIFO_policy.o;
	@rm three_com.o FIFO_policy.o;

rt_3c905x_phys:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $(RTL_LWIP_DIR)/arch/RTLinux/netif/physif/$@.c -o three_com.o

tcpserver: ./examples/sequential_api/tcp_echo/tcpserver.c
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/sequential_api/tcp_echo/$@.c

udpclient: ./examples/sequential_api/udp_echo/udpclient.c
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/sequential_api/udp_echo/$@.c

udpserver: ./examples/sequential_api/udp_echo/udpserver.c
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/sequential_api/udp_echo/$@.c

tcpthreaded: ./examples/sequential_api/tcp_echo_multithread/tcpthreaded.c
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/sequential_api/tcp_echo_multithread/$@.c

dnsclient: ./examples/dns/dnsclient.c
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c ./examples/dns/$@.c

mipro: 
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c rtl_lwip_stack.c -o mipro.o;	

stack:  $(RTL_LWIPOBJS) mipro
	@ld -E -i -o rtl_lwip_stack.o etharp.o icmp.o inet.o ip.o ip_addr.o loopif.o \
	netif.o pbuf.o sys_arch.o tcp.o tcp_in.o tcp_out.o udp.o tcpip.o \
	api_msg.o api_lib.o sockets.o err.o rtfifoif.o rt_3c905x.o \
	stats.o ip_frag.o mem.o memp.o \
	ares__close_sockets.o ares_compat.o ares_fds.o ares_gethostbyaddr.o \
	ares_gethostbyname.o ares_init.o ares_process.o ares_search.o \
	ares_strerror.o ares_timeout.o ares_expand_name.o ares_query.o \
	ares_parse_a_reply.o ares_destroy.o ares_send.o netdb.o \
	ares_parse_ptr_reply.o ares_free_hostent.o ares_mkquery.o \
	ares_free_string.o ares_free_errmem.o \
	mipro.o; \
	rm mipro.o etharp.o icmp.o inet.o ip.o ip_addr.o loopif.o netif.o \
	pbuf.o sys_arch.o tcp.o tcp_in.o tcp_out.o udp.o tcpip.o\
	api_msg.o api_lib.o sockets.o err.o rtfifoif.o rt_3c905x.o \
	ares__close_sockets.o ares_compat.o ares_fds.o ares_gethostbyaddr.o \
	ares_gethostbyname.o ares_init.o ares_process.o ares_search.o \
	ares_strerror.o ares_timeout.o ares_expand_name.o ares_query.o \
	ares_parse_a_reply.o ares_destroy.o ares_send.o netdb.o \
	ares_parse_ptr_reply.o ares_free_hostent.o ares_mkquery.o \
	ares_free_string.o ares_free_errmem.o \
	stats.o ip_frag.o mem.o memp.o;


DIDMA.c: 
	$(CC) $(RTLFLAGS) -c  $(TOOLSDIR)/$@ -o malloc.o

tools: DIDMA.c
	@ld -E -i -o rtl_lwip_tools.o malloc.o strtol.o bzero.o memcopy.o time_debug.o\
	gettimeofday.o rt_pci.o; \
	rm malloc.o strtol.o bzero.o memcopy.o time_debug.o gettimeofday.o rt_pci.o
prueba:
	$(CC) $(RTLFLAGS) $(RTL_LWIPFLAGS) -c $@.c
