/*
 * Written by J.T. Conklin .
 * Public domain.
 *
 * CHANGELOG: this file has been modified by Sergio Perez Alcañiz <serpeal@disca.upv.es> 
 *            Departamento de Informática de Sistemas y Computadores          
 *            Universidad Politécnica de Valencia                             
 *            Valencia (Spain)    
 *            Date: March 2003                                          
 *            
 */

#include  <sys/types.h>
#include <rtl_sched.h>
#undef htonl

/*  */
/* htonl */
in_addr_t htonl(in_addr_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *)&x;
	return (in_addr_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
#else
	return x;
#endif
}
