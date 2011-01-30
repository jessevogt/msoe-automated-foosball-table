#ifndef _BALLTRACKER_H_
#define _BALLTRACKER_H_

#include "FLUTypes.h"

typedef int (f_bt_setup_func)(const BallTrackerSetup *);
typedef void * (f_bt_thread_func)(void *);
typedef void (f_bt_stop_func)(void);
typedef int (f_bt_ready_func)(void);

typedef struct ball_tracker
{
   f_bt_setup_func * const setup;
   f_bt_thread_func * const mainThread;
   f_bt_stop_func * const stop;
   f_bt_ready_func * const isReady;
}
ball_tracker;

extern ball_tracker BallTracker;

#endif
