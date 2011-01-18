#ifndef _POINT_H_
#define _POINT_H_

#include "FLUTypes.h"

#ifndef __cplusplus
#include "time.h"
#endif

/**
 * Data structure to hold information needed to define a point in space and time
 */
typedef struct point
{
   /**
    * x coordinate
    */
   int x;
   
   /**
    * y coordinate
    */
   int y;

   /**
    * time at which the coordinates were recorded
    */
   #ifndef __cplusplus
	hrtime_t time;
	#else
	long long time;
	#endif
	
}
Point;

#endif // _POINT_H_
