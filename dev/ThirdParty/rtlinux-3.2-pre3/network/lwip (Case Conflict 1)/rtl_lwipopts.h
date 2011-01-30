/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * rtl_lwipopts.h
 *                     
 * Author : Adam Dunkels <adam@sics.se>                               
 *
 * CHANGELOG: this file has been modified by Sergio Perez Alcañiz <serpeal@disca.upv.es> 
 *            Departamento de Informática de Sistemas y Computadores          
 *            Universidad Politécnica de Valencia                             
 *            Valencia (Spain)    
 *            Date: March 2003                                          
 *            
 */

#ifndef __LWIP_OPTS_H__
#define __LWIP_OPTS_H__

#define LWIP 1


/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   RTL-lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
#define MEM_ALIGNMENT           2

/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
#define MEM_SIZE                5000

/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#define MEMP_NUM_PBUF           200
/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#define MEMP_NUM_UDP_PCB        4
/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#define MEMP_NUM_TCP_PCB        250
/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
   connections. */
#define MEMP_NUM_TCP_PCB_LISTEN 100
/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#define MEMP_NUM_TCP_SEG        200
/* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active
   timeouts. */
#define MEMP_NUM_SYS_TIMEOUT    10


/* The following four are used only with the sequential API and can be
   set to 0 if the application only will use the raw API. */
/* MEMP_NUM_NETBUF: the number of struct netbufs. */
#define MEMP_NUM_NETBUF         2
/* MEMP_NUM_NETCONN: the number of struct netconns. */
#define MEMP_NUM_NETCONN        7
/* MEMP_NUM_APIMSG: the number of struct api_msg, used for
   communication between the TCP/IP stack and the sequential
   programs. */
#define MEMP_NUM_API_MSG        8
/* MEMP_NUM_TCPIPMSG: the number of struct tcpip_msg, which is used
   for sequential API communication and incoming packets. Used in
   src/api/tcpip.c. */
#define MEMP_NUM_TCPIP_MSG      30

/* ---------- Pbuf options ---------- */
/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#define PBUF_POOL_SIZE          200

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
#define PBUF_POOL_BUFSIZE       256

/* PBUF_LINK_HLEN: the number of bytes that should be allocated for a
   link level header. */
#define PBUF_LINK_HLEN          16

/* ---------- TCP options ---------- */
#define LWIP_TCP                1
#define TCP_TTL                 255

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ         1

/* TCP Maximum segment size. */
#define TCP_MSS                 128

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF             256

/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
#define TCP_SND_QUEUELEN        8 * TCP_SND_BUF/TCP_MSS

/* TCP receive window. */
#define TCP_WND                 1024

/* Maximum number of retransmissions of data segments. */
#define TCP_MAXRTX              12

/* Maximum number of retransmissions of SYN segments. */
#define TCP_SYNMAXRTX           4

/* ---------- ARP options ---------- */
#define ARP_TABLE_SIZE 10

/* ---------- IP options ---------- */
/* Define IP_FORWARD to 1 if you wish to have the ability to forward
   IP packets across network interfaces. If you are going to run RTL-lwIP
   on a device with only one network interface, define this to 0. */
#define IP_FORWARD              0

/* If defined to 1, IP options are allowed (but not parsed). If
   defined to 0, all packets with IP options are dropped. */
#define IP_OPTIONS              1

/* ---------- ICMP options ---------- */
#define ICMP_TTL                255


/* ---------- DHCP options ---------- */
/* Define RTL_LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces. DHCP is not implemented in RTL-lwIP 0.5.1, however, so
   turning this on does currently not work. */
#define LWIP_DHCP               0

/* 1 if you want to do an ARP check on the offered address
   (recommended). */
#define DHCP_DOES_ARP_CHECK     1

/* ---------- UDP options ---------- */
#define LWIP_UDP                1
#define UDP_TTL                 255


/* ---------- TCPIP Thread period options ---------- */
//Nanoseconds
#define TCPIP_THREAD_PERIOD     (1000000LL)

/* ---------- Thread's stack options ---------- */
#define THREAD_STACK_SIZE 20000


/* ----------------------------------------------------------------------------- */
/*                                ETHERNET                                       */
/* ----------------------------------------------------------------------------- */
#ifndef __RTETHERNETOPTS__
#define __RTETHERNETOPTS__
#endif

#ifdef __RTETHERNETOPTS__

#define RT3COM905C_IP               "158.42.58.141"
#define RT3COM905C_GW               "158.42.1.10"
#define RT3COM905C_NETMASK          "255.0.0.0"
#define RT3COM905C_ISDEFAULTIF 1


#endif


/* ----------------------------------------------------------------------------- */
/*                                RTFIFO                                         */
/* ----------------------------------------------------------------------------- */
/* This line should be commented if no RTFIFO device is used */

//#ifndef __RTFIFOOPTS__
//#define __RTFIFOOPTS__
//#endif /* #ifndef __RTFIFOOPTS__ */

#ifdef __RTFIFOOPTS__
/* ---------- RTFIFO options ---------- */
/* Period of RTFIFO thread (nanoseconds) */
#define RTFIFO_THREAD_PERIOD    (1000000LL)
/* 
   Delays used by RTFIFO driver while waiting for a packet to arrive. (Reading from 
   RTFIFO is not a blocking operation) (units are microseconds)
*/
#define RTFIFO_DELAY            5
/* You must set a MAC address to the RTFIFO net interface, this one is default if not set */
#define RTFIFO_MAC              "02:02:03:03:05:06"
/* You must set an IP address to the RTFIFO net interface, this one is default if not set */
#define RTFIFO_IP               "192.170.0.5"
/* You must set a gateway IP address to the RTFIFO net interface,this one is default if not set */
#define RTFIFO_GW               "192.170.0.1"
/* You must set a net mask to the RTFIFO net interface, this one is default if not set */
#define RTFIFO_NETMASK          "255.0.0.0"
/* This flag indicates if RTFIFO interface should be set as default */
#define RTFIFO_ISDEFAULTIF       0   

#endif /* #ifdef __RTFIFOOPTS__ */




/* ---------- Statistics options ---------- */
//#define STATS

#ifdef STATS
#define LINK_STATS
#define IP_STATS
#define ICMP_STATS
#define UDP_STATS
#define TCP_STATS
#define MEM_STATS
#define MEMP_STATS
#define PBUF_STATS
#define SYS_STATS
#endif /* STATS */

#endif /* __LWIP_OPTS_H__ */
