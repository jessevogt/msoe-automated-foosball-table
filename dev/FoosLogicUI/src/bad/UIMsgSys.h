#ifndef _UIMSGSYS_
#define _UIMSGSYS_

#ifdef _DEBUG_

	#include <iostream>
	#define DEBUG_MSG( text ) std::cout << "DEBUG [" << __FILE__ << ":" << __LINE__ << "] " << text << std::endl;

#else

	#define DEBUG_MSG( text )

#endif

#ifdef _ERROR_

	#include <iostream>
	#define ERROR_MSG( text ) std::cout << "ERROR [" << __FILE__ << ":" << __LINE__ << "] " << text << std::endl;
	
#else

	#define ERROR_MSG( text )

#endif

#endif //_UIMSGSYS_
