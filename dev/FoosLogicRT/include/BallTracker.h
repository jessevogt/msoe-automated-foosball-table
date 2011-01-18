/**
 * File: BallTracker.h
 *
 * Prototypes for BallTracker "class"
 * Contains thread used that handles all ball tracking tasks. Provides accessor
 * into thread to obtain current ball position and is able to publish ball
 * position as well as all laser break information to shared memory for use by
 * user space programs. Implements seveal filtering methods to help eliminate
 * jumpy lasers.
 *
 * When starting the BallTracker thread the use the following command order:
 *
 *    BallTracker.setup();
 *    BallTracker.run();
 *    BallTracker.startPlay();
 *    ...
 *    BallTracker.cleanup();
 *
 * Each of the above method calls should return SUCCESS before the next is called.
 *
 * This class is implemented as a Singleton as there will only ever be one instance
 * of it running. This is accompolished by the extern statement at the end of this
 * file and the static definitions in FoosLogic.c. For more information on this
 * implementation see http://www.csd.uwo.ca/%7ejamie/C/encapsulatedC.html
 *
 * Author: Jesse Vogt
 * Date: 3/20/2005
 */

#ifndef _BALLTRACKER_H_
#define _BALLTRACKER_H_

#include "FLUTypes.h"
#include "Point.h"

typedef enum { BTMODE_NORMAL, BTMODE_LONGCOUNT } BallTrackingMode;

/**
 * Typedefs for the function pointers used in the BallTracker structure
 */
typedef Status (bt_setup_func)(void);
typedef Status (bt_run_func)(void);
typedef Status (bt_startplay_func)(void);
typedef Status (bt_pause_func)(void);
typedef Status (bt_cleanup_func)(void);

typedef inline void (bt_getballposition_func)(Point *);
typedef Status (bt_setballtrackingmode_func)(BallTrackingMode);

typedef struct ball_tracker
{
   /**
    * Setup and init all necessary values and allocate all dynamic memory for the
    * Ball Tracker thread
    *
    * Prototype:
    *    Status setup(void);
    * Params:
    *    void
    * Returns:
    *    SUCCESS or FAILURE
    */
   bt_setup_func * const setup;
   
   /**
    * Cause the ball tracking thread to begin running (but do not start play)
    *
    * Prototype:
    *    void run(void)
    * Params:
    *    void
    * Returns:
    *    SUCCESS or FAILURE
    */
   bt_run_func * const run;
   
   /**
    * Start the main program loop to begin play
    *
    * Prototype:
    *    void start(void)
    * Params:
    *    void
    * Returns:
    *    SUCCESS or FAILURE
    */
   bt_startplay_func * const startPlay;
   
   /**
    * Prevent the ball tracking from polling the lasers and calculating ball
    * positions
    *
    * Prototype:
    *    void pause(void)
    * Params:
    *    void
    * Returns:
    *    SUCCESS or FAILURE
    */
   bt_pause_func * const pause;
   
   /**
    * Perform necessary cleanup after program is done (dealloc memory, stop loop, etc)
    *
    * Prototype:
    *    void cleanup(void)
    * Params:
    *    void
    * Returns:
    *    SUCCESS or FAILURE
    */
   bt_cleanup_func * const cleanup;
   
   /**
    * Store the current ball position into the memory specified by the point param
    *
    * Prototype:
    *    inline void getBallPosition(Point * point)
    * Params:
    *    point - pointer to Point in which to store the curren position
    * Returns:
    *    SUCCESS or FAILURE
    */
   bt_getballposition_func * const getBallPosition;

   /**
    * Set the ball tracking mode used for tracking the ball on the table.
    * BTMODE_LONGCOUNT uses two types of filtering. The first takes out jitter and
    *    and the second attempts to remove broken lasers.
    * BTMODE_NORMAL uses only the jitter filter
    *
    * Prototype:
    *    Status setBallTrackingMode(BallTrackingMode mode)
    * Params:
    *    BTMODE_LONGCOUNT or BTMODE_NORMAL
    * Returns:
    *    SUCCESS or FAILURE
    */
   bt_setballtrackingmode_func * const setBallTrackingMode;
}
ball_tracker;

/**
 * Single instance to be used by entire system
 */
extern ball_tracker BallTracker;

#endif // _BALLTRACKER_H_

