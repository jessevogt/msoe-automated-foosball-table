/*
 * Written by J.T. Conklin .
 * Public domain.
 *
 * CHANGELOG: this file has been modified by Sergio Perez Alca�iz <serpeal@disca.upv.es> 
 *            Departamento de Inform�tica de Sistemas y Computadores          
 *            Universidad Polit�cnica de Valencia                             
 *            Valencia (Spain)    
 *            Date: March 2003                                          
 *            
 */

#undef htons

/*  */
/* htons */
in_port_t
htons(x)
	in_port_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (in_port_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
