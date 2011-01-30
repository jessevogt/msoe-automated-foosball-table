/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 * CHANGELOG: this file has been modified by Sergio Perez Alcañiz <serpeal@disca.upv.es> 
 *            Departamento de Informática de Sistemas y Computadores          
 *            Universidad Politécnica de Valencia                             
 *            Valencia (Spain)    
 *            Date: April 2003                                          
 *
 */

#ifndef __LWIP_INET_H__
#define __LWIP_INET_H__

#include "lwip/arch.h"

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"

u16_t inet_chksum(void *dataptr, u16_t len);
u16_t inet_chksum_pbuf(struct pbuf *p);
u16_t inet_chksum_pseudo(struct pbuf *p,
			 struct ip_addr *src, struct ip_addr *dest,
			 u8_t proto, u16_t proto_len);

u32_t inet_addr(const char *cp);
int inet_aton(const char *cp, struct in_addr *addr);
char *inet_ntoa(struct in_addr in);

#ifdef HTONS
#undef HTONS
#endif /* HTONS */
#ifdef NTOHS
#undef NTOHS
#endif /* NTOHS */
#ifdef HTONL
#undef HTONL
#endif /* HTONL */
#ifdef NTOHL
#undef NTOHL
#endif /* NTOHL */

#ifndef HTONS
#   if BYTE_ORDER == BIG_ENDIAN
#      define HTONS(n) (n)
#      define htons(n) HTONS(n)
#   else /* BYTE_ORDER == BIG_ENDIAN */
#      define HTONS(n) (((((u16_t)(n) & 0xff)) << 8) | (((u16_t)(n) & 0xff00) >> 8))
#   endif /* BYTE_ORDER == BIG_ENDIAN */
#endif /* HTONS */

#ifdef NTOHS
#undef NTOHS
#endif /* NTOHS */

#ifdef ntohs
#undef ntohs
#endif /* ntohs */

#define NTOHS HTONS
#define ntohs htons


#ifndef HTONL
#   if BYTE_ORDER == BIG_ENDIAN
#      define HTONL(n) (n)
#      define htonl(n) HTONL(n)
#   else /* BYTE_ORDER == BIG_ENDIAN */
#      define HTONL(n) (((((u32_t)(n) & 0xff)) << 24) | \
                        ((((u32_t)(n) & 0xff00)) << 8) | \
                        ((((u32_t)(n) & 0xff0000)) >> 8) | \
                        ((((u32_t)(n) & 0xff000000)) >> 24))
#   endif /* BYTE_ORDER == BIG_ENDIAN */
#endif /* HTONL */

#ifdef ntohl
#undef ntohl
#endif /* ntohl */

#ifdef NTOHL
#undef NTOHL
#endif /* NTOHL */

#define NTOHL HTONL
#define ntohl htonl

#ifndef _MACHINE_ENDIAN_H_
#ifndef _NETINET_IN_H
#ifndef _LINUX_BYTEORDER_GENERIC_H

#if BYTE_ORDER == LITTLE_ENDIAN
#define htons HTONS
#define htonl HTONL
//uint16_t htons(u16_t n);
//uint32_t htonl(u32_t n);
//unsigned short int htons(u16_t n);
//unsigned int htonl(u32_t n);
//u16_t htons(u16_t n);
//u32_t htonl(u32_t n);
#else
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#endif /* _LINUX_BYTEORDER_GENERIC_H */
#endif /* _NETINET_IN_H */
#endif /* _MACHINE_ENDIAN_H_ */

#endif /* __LWIP_INET_H__ */

