#ifndef _RTDEBUG_H_
#define _RTDEBUG_H_

#ifdef _DEBUG_
#define rt_debug_msg( text ) rtl_printf("%s:%d " text "\n", __FILE__, __LINE__);
#define rt_debug_msg_val( text, value ) rtl_printf("%s:%d " text "%d\n", __FILE__, __LINE__, value);
#else
#define rt_debug_msg( text )
#define rt_debug_msg_val( text, value )
#endif

#endif
