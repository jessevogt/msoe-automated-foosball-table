#ifndef _FLUTYPES_H_
#define _FLUTYPES_H_

#include <time.h>
#include "FLUDefs.h"

typedef short coord;

typedef struct point
{
   coord x;
   coord y;
   
#ifdef __cplusplus 
   long time;
#else
   hrtime_t time;
#endif
}
Point;

typedef struct ball_tracker_setup
{
   int BallTrackerPriority;
   int BallTrackerPeriod;
   
   int BankSelectAddress;
   int BankResultAddress;
   
   unsigned char StartXBank;
   unsigned char EndXBank;
   unsigned char StartYBank;
   unsigned char EndYBank;
   
   volatile coord * XBreaksSM[NUM_SHARE_MEM];
   volatile coord * YBreaksSM[NUM_SHARE_MEM];
   
   volatile int * BreakSMIndex;
   volatile int * UserBreakSMIndex;
}
BallTrackerSetup;

#endif
