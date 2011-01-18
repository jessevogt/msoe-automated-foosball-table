/**
 * File: BallTracker.c
 *
 * Definition for BallTracker "class"
 * (see BallTracker.h for interface documentation)
 *
 * Author: Jesse Vogt
 * Date: 3/20/2005
 */

#include <rtl.h>
#include <time.h>
#include <mbuff.h>

#include "BallTracker.h"
#include "RxBank.h"
#include "FLULogger.h"
#include "FLUTypes.h"
#include "FLUDefs.h"
#include <asm/io.h>

/* public methods prototyped in header */
static Status setup(void);
static Status run(void);
static Status startPlay(void);
static Status pause(void);
static Status cleanup(void);
static inline void getBallPosition(Point * point);
static Status setBallTrackingMode(BallTrackingMode mode);

/*********************************************************************
 * PRIVATE METHODS
 *********************************************************************/

/**
 * BallTracking thread. This function is passed into the create thread and is
 * the function that is run periodically and makes the necessary calls to allow
 * for ball tracking.
 *
 * Parameters:
 *    param - pointer to any parameter information
 *
 * Returns:
 *    pointer
 */
static void * _main(void * param);

/**
 * Set all elements of array (size topIndex) to zero.
 *
 * Parameters:
 *    array - pointer to the first element of the array
 *    topIndex - index of element to zero up to
 *
 * Returns:
 *    void
 */
static inline void _zeroIntArray(int * array, int topIndex);

/**
 * Get all breaks that are detected in the specified bank and store in the
 * specified array.
 *
 * Parameters:
 *    currentBank - pointer to the current bank to get the break information from
 *    breakPtr - pointer to the array for storing the detected breaks
 *
 * Returns:
 *    void
 */
static inline void _getBreaks(RxBank * currentBank, int * breakPtr);

/**
 * Calculates the current ball position and performs additional filtering
 *
 * Parameters:
 *    xBreakCount - pointer to array that contains the number of times each laser
 *                  was broken in the x axis in the last checking cycle
 *    yBreakCount - pointer to array that contains the number of times each laser
 *                  was broken in the y axis in the last checking cycle
 *    calcXBreaksPtr - pointer to array that contains the actual breaks to be
 *                     reported to the UI in the x direction. This array is
 *                     terminated by a -1.
 *    calcYBreaksPtr - pointer to array that contains the actual breaks to be
 *                     reported to the UI in the Y direction. This array is
 *                     terminated by a -1.
 *
 * Returns:
 *    void
 */
static inline void _calcBallPosition(int * xBreakCount, int * yBreakCount, int * calcXBreaksPtr, int * calcYBreaksPtr);

/**
 * Publish the current break information and calculated ball position to shared
 * memory for use by any other programs. (The break arrays are passed in and the
 * ball position comes from a file scoped var)
 *
 * Parameters:
 *    calcXBreaks - the breaks to publish for the x axis
 *    calcYBreaks - the breaks to publish for the y axis
 *
 * Returns:
 *    void
 */
static inline void _publishBallTrackData(int * calcXBreaks, int * calcYBreaks);

/*********************************************************************
 * PRIVATE MEMBERS
 *********************************************************************/

/**
 * thread handle for the ball tracker thread
 */
static pthread_t _ballTrackerThread;

/**
 * thread attribute structure for the ball tracker thread
 */
static pthread_attr_t _ballTrackerAttr;

/**
 * schedule parameter structure for the ball tracker thread
 */
struct sched_param _ballTrackerSchedParam;

/**
 * flag to start the ball tracker loop
 */
static bool _start;

/**
 * flag for staying in the main program loop
 */
static bool _mainLoop;

/**
 * the current position of the ball on the table
 */
static Point _curPos;

/**
 * Pointer to the first y bank in the list
 */
static RxBank * startYBank;

/**
 * Pointer to the first x bank in the list
 */
static RxBank * startXBank;

/**
 * pointer to the x break shared memory
 */
static int * xShareMem;

/**
 * pointer to the y break shared memory
 */
static int * yShareMem;

/**
 * pointer to the current ball position shared memory
 */
static Point * curPosShareMem;

/**
 * array used for filter that turns off broken lasers in x axis
 */
static int longCountX[NUMBER_X_RX];

/**
 * array used for filter that turns off broken lasers in y axis
 */
static int longCountY[NUMBER_Y_RX];

/**
 * flag for it the thread was started
 */
static bool _threadStarted;

/**
 * mode to use for tracking the ball
 */
static BallTrackingMode _btMode;

/**
 * BallTracker instance with populated function pointers for use by rest
 * of system
 */
ball_tracker BallTracker = { &setup,
                             &run,
                             &startPlay,
                             &pause,
                             &cleanup,
                             &getBallPosition,
                             &setBallTrackingMode };

static Status setup()
{
   Status status;
   
   DEBUG_MSG("BallTracker.setup()")
   
   status = SUCCESS;
   
   // init the ball tracker thread attribute structure
   pthread_attr_init(&_ballTrackerAttr);
   
   // set the scheduling priority for the ball tracker
   _ballTrackerSchedParam.sched_priority = BALL_TRACKER_PRIORITY;
   
   // set all scheduling parameters
   pthread_attr_setschedparam(&_ballTrackerAttr,&_ballTrackerSchedParam);
   
   // setup main looping vars
   _start = false;
   _mainLoop = true;
   _threadStarted = false;
   _btMode = BTMODE_LONGCOUNT;
   
   // init current ball position
   _curPos.x = -1;
   _curPos.y = -1;
   _curPos.time = 0;
   
   // Create and initialize RxBanks   
   {
      int t;
      RxBank * temp;
      int bankAddress;
      
      // START Create X RxBanks
      bankAddress = END_X_RX_ADDRESS;
      
      // create last bank
      temp = createNewRxBank( bankAddress, NUMBER_X_RX_BANKS - 1, null );
      
      // starting with second last bank, create all banks and add to list
      for( t = NUMBER_X_RX_BANKS - 2; t >= 0; --t )
      {
         // calculate next bank address to use
         bankAddress += DELTA_X_RX_ADDRESS;
         
         // create new bank
         temp = createNewRxBank( bankAddress, t, temp );
      }
      
      startXBank = temp;
      // END Create X RxBanks
      
      // START Create Y RxBanks
      bankAddress = END_Y_RX_ADDRESS;
      
      // create last bank
      temp = createNewRxBank( bankAddress, NUMBER_Y_RX_BANKS - 1, null );
      
      // starting with second last bank, create all banks
      for( t = NUMBER_Y_RX_BANKS - 2; t >= 0; --t )
      {
         // calculate next bank address to use
         bankAddress += DELTA_Y_RX_ADDRESS;
         
         // create new bank
         temp = createNewRxBank( bankAddress, t, temp );
      }
      
      startYBank = temp;
      // END Create Y RxBanks
      
      // output bank addressing information if debugging is turned on
      #ifdef _DEBUG_
      {
         RxBank * currentBank;
         
         currentBank = startXBank;
         
         while( currentBank != null )
         {
            rtl_printf("X Bank ID: %d Band Address: %x\n", currentBank->bankId, (unsigned int)currentBank->bankAddress);
            currentBank = currentBank->next;
         }
         
         rtl_printf("\n\n");
         
         currentBank = startYBank;
         
         while( currentBank != null )
         {
            rtl_printf("Y Bank ID: %d Band Address: %x\n", currentBank->bankId, currentBank->bankAddress);
            currentBank = currentBank->next;
         }
      }
      #endif // _DEBUG_ 
   }
   
   // create pointers to shared memory arrays for publishing breaks and ball position
   xShareMem = (int *) mbuff_alloc(SHAREMEM( x, 0), sizeof(int) * (NUMBER_X_RX + 1));
   yShareMem = (int *) mbuff_alloc(SHAREMEM( y, 0), sizeof(int) * (NUMBER_Y_RX + 1));
   curPosShareMem = (Point *) mbuff_alloc(BALL_POS_SMLABEL,sizeof(Point));
   
   // init the values of the shared memory
   *xShareMem = -1;
   *yShareMem = -1;
   *curPosShareMem = (Point){0,0,0};
   
   // clear long count arrays for filter
   _zeroIntArray(longCountX,NUMBER_X_RX);
   _zeroIntArray(longCountY,NUMBER_Y_RX);
   
   return SUCCESS;
}

static Status run()
{
   // create thread
   pthread_create(&_ballTrackerThread, &_ballTrackerAttr, _main, 0);
   _threadStarted = true;
   return SUCCESS;
}

static Status startPlay()
{
   DEBUG_MSG("BallTracker.startPlay()");
   
   // start main program loop
   _start = true;
   
   return SUCCESS;
}

static Status pause()
{
   // stop main program loop
   _start = false;
   return SUCCESS;
}

static Status cleanup()
{
   _mainLoop = false;
   _start = false;
   
   if( _threadStarted == true )
   {
      // delete thread
      pthread_delete_np(_ballTrackerThread);
   }
   
   // free all RxBank memory
   freeAllRxBankMem();
   
   // free all shared memory
   mbuff_free( SHAREMEM( x, 0 ) , (void *)xShareMem );
   mbuff_free( SHAREMEM( y, 0 ) , (void *)yShareMem );
   mbuff_free( BALL_POS_SMLABEL , (void*)curPosShareMem );

   return SUCCESS;
}

static inline void getBallPosition(Point * point)
{ 
   point->x = _curPos.x;
   point->y = _curPos.y;
   point->time = _curPos.time;
}

static void * _main(void * param)
{
   // pointer into break array
   int * breakPtr;
   
   // count used to track the number of times the filter loop has run
   int filterCount = 0;
   
   // arrays to track the number of times each laser was broken during the jitter
   // filter loop
   int yBreakCount[NUMBER_Y_RX];
   int xBreakCount[NUMBER_X_RX];
   
   // arrays of breaks found (terminated by -1)
   int _xBreaks[NUMBER_X_RX + 1];
   int _yBreaks[NUMBER_Y_RX + 1];
   
   // arrays of breaks found to report after filtering (terminated by -1)
   int calcXBreaks[NUMBER_X_RX + 1];
   int calcYBreaks[NUMBER_Y_RX + 1];
   
   // clear break count array
   _zeroIntArray(yBreakCount,NUMBER_Y_RX);
   _zeroIntArray(xBreakCount,NUMBER_X_RX);
      
   // setup thread to run periodically at BALL_TRACKER_PERIOD intervals
   // (see FLUDefs.h)
   pthread_make_periodic_np(pthread_self(), gethrtime(), BALL_TRACKER_PERIOD);
   
   while( _mainLoop == true )
   {      
      while( _start == true )
      {
         // wait for next cycle
         pthread_wait_np();
         
         // get the current breaks in x and y axis
         _getBreaks(startXBank,_xBreaks);
         _getBreaks(startYBank,_yBreaks);
 
         // test if still in jitter filter
         if( filterCount < FILTER_COUNT )
         {
            // store pointer to the beginning of x breaks array
            breakPtr = _xBreaks;
            
            // loop through array until end if found
            while( *breakPtr != -1 )
            {
               // increment the break count for the index of the broken laser
               ++xBreakCount[*breakPtr];
               
               // increment pointer in break array
               ++breakPtr;
            }
            
            // store pointer to beginning of y breaks array
            breakPtr = _yBreaks;
            
            // loop until end of array is found
            while( *breakPtr != -1 )
            {
               // increment the break count for the index of the broken laser
               ++yBreakCount[*breakPtr];
               
               // increment the pointer of the break array
               ++breakPtr;
            }
            
            // incremement the filter count value
            ++filterCount;
         }
         else
         {
            //filter has run FILTER_COUNT times to ready to publish data
            
            _calcBallPosition(xBreakCount,yBreakCount,calcXBreaks,calcYBreaks); 
            _publishBallTrackData(calcXBreaks,calcYBreaks);
            
            // clear break counts
            _zeroIntArray(yBreakCount,NUMBER_Y_RX);
            _zeroIntArray(xBreakCount,NUMBER_X_RX);
                        
            // reset filter count
            filterCount = 0;
         }
      }
      
      // wait until next interval
      pthread_wait_np();
   }
   
   return 0;
}

static inline void _getBreaks(RxBank * currentBank, int * breakPtr)
{
   // loop through list until end if found
   while( currentBank != null )
   {
      // call that checks current bank for any breaks and if any found increments
      // the pointer to the break array as needed
      breakPtr = bankGetBreaks(breakPtr,currentBank);
      
      // store the next bank to check
      currentBank = currentBank->next;     
   }
   
   // set the element after the last one found to -1 to terminate the array
   *breakPtr = -1;
}

static inline void _zeroIntArray(int * array, int topIndex)
{
   int t = 0;
   
   for( ; t < topIndex; ++t )
   {
      array[t] = 0;
   }
}

static inline void _calcBallPosition(int * xBreakCount, int * yBreakCount, int * calcXBreaksPtr, int * calcYBreaksPtr)
{
   // number of times the laser has to be on to count as a valid break from the
   // jitter loop
   static int testCount = FILTER_COUNT / 2;
   
   // switch between tracking counts (currently hardcoded to only use LONG_COUNT
   switch ( _btMode )
   {
      case BTMODE_NORMAL:
         {
            int t;

            // loop through all x banks
            for( t = 0; t < NUMBER_X_RX; ++t )
            {
               // if break was detected for current bank greater than half the time
               // of the filter then pick as candidate for break
               if( xBreakCount[t] > testCount )
               {
                  // set index for current bank
                  *calcXBreaksPtr = t;
                  
                  // increment the pointer to the array
                  ++calcXBreaksPtr;
               }
            }

            // set last element to -1 to terminate
            *calcXBreaksPtr = -1;

            // loop through all y banks
            for( t = 0; t < NUMBER_Y_RX; ++t )
            {
               // if break was detected for current bank greater than half the time
               // of the filter then pick as candidate for break
               if( yBreakCount[t] > testCount )
               {
                  // store index for current bank
                  *calcYBreaksPtr = t;
                  
                  // increment the pointer to the array
                  ++calcYBreaksPtr;
               }
            }

            // set last element to -1 to terminate
            *calcYBreaksPtr = -1;

            break;
         }
      case BTMODE_LONGCOUNT:
         {
            int t;

            // loop through all x banks
            for( t = 0; t < NUMBER_X_RX; ++t )
            {
               // if break was detected for current bank greater than half the time
               // of the filter then pick as candidate for break
               if( xBreakCount[t] > testCount )
               {
                  // test that laser is not current flagged ( >= 0 ) and has not
                  // been broken for last 500 checks
                  if( longCountX[t] < 500 && longCountX[t] >= 0 )
                  {
                     // set break pointer to index of broken laser
                     *calcXBreaksPtr = t;
                     
                     // increment pointer to array
                     ++calcXBreaksPtr;
                     
                     // increment count of how many times the laser was broken
                     ++longCountX[t];
                  }
                  else
                  {
                     // if the laser has been off for more than 500 times flag
                     // laser to ignore
                     longCountX[t] = -1;
                  }
               }
               else
               {
                  // laser has turned back on so reset ignore flag and use in
                  // next check
                  longCountX[t] = 0;
               }
            }

            // terminate array
            *calcXBreaksPtr = -1;

            // loop through all y banks
            for( t = 0; t < NUMBER_Y_RX; ++t )
            {
               // if break was detected for current bank greater than half the time
               // of the filter then pick as candidate for break
               if( yBreakCount[t] > testCount )
               {
                  // test that laser is not current flagged ( >= 0 ) and has not
                  // been broken for last 500 checks
                  if( longCountY[t] < 500 && longCountY[t] >= 0 )
                  {
                     // set break pointer to index of broken laser
                     *calcYBreaksPtr = t;
                     
                     // increment pointer to the array
                     ++calcYBreaksPtr;
                     
                     // increment the count of how many times the laser was broken
                     ++longCountY[t];
                  }
                  else
                  {
                     // if the laser has been off for more than 500 times flag the
                     // lase to igore
                     longCountY[t] = -1;
                  }
               }
               else
               {
                  // laser has turned back on so reset ingore flag and use in next
                  // check
                  longCountY[t] = 0;
               }
            }

            // terminate array
            *calcYBreaksPtr = -1;

            break;
         }
      default:
         break;
   }
}

static inline void _publishBallTrackData(int * calcXBreaks, int * calcYBreaks)
{
   /**
    * the table coordinate system is as follows:
    *
    * Table Coord: 0 1 2 3 4 5 6 ...
    * Laser:       |   |   |   | ...
    * Laser Id:    0   1   2   3 ...
    */
   
   // check that the first two breaks are valid breaks and that the difference
   // between is 1 (broken lasers are next to each other
   if( calcXBreaks[0] != -1 && calcXBreaks[1] != -1 && calcXBreaks[1] - calcXBreaks[0] == 1 )
   {
      // set x position to id of laser * 2 + 1 to get the coordinate between
      // the two broken lasers
      _curPos.x = (calcXBreaks[0] * 2) + 1;
   }
   // check if only first break is broken
   else if( calcXBreaks[0] != -1 )
   {
      // set x postion to the coordinate value of the broken laser
      _curPos.x = calcXBreaks[0] * 2;
   }

   // check that the first two breaks are valid breaks and that the difference
   // between is 1 (broken lasers are next to each other
   if( calcYBreaks[0] != -1 && calcYBreaks[1] != -1 && calcYBreaks[1] - calcYBreaks[0] == 1 )
   {
      // set y position to id of laser * 2 + 1 to get the coordinate between
      // the two broken lasers
      _curPos.y = (calcYBreaks[0] * 2) + 1;
   }
   // check if only first break is broken
   else if( calcYBreaks[0] != -1 )
   {
      // set y postion to the coordinate value of the broken laser
      _curPos.y = calcYBreaks[0] * 2;
   }
      
   // copy ball position to shared memory
   memcpy((void*)curPosShareMem,(void*)&_curPos,sizeof(Point));
   
   // copy break information to shared memory
   memcpy((void*)yShareMem,(void*)calcYBreaks,sizeof(int) * (NUMBER_Y_RX + 1));
   memcpy((void*)xShareMem,(void*)calcXBreaks,sizeof(int) * (NUMBER_X_RX + 1));   
}

static Status setBallTrackingMode(BallTrackingMode mode)
{
   Status result = SUCCESS;

   switch (mode)
   {
      case BTMODE_NORMAL:
         {
            _btMode = BTMODE_NORMAL;

            // clear long count arrays for filter
            _zeroIntArray(longCountX,NUMBER_X_RX);
            _zeroIntArray(longCountY,NUMBER_Y_RX);
            break;
         }
      case BTMODE_LONGCOUNT:
         {
            // clear long count arrays for filter
            _zeroIntArray(longCountX,NUMBER_X_RX);
            _zeroIntArray(longCountY,NUMBER_Y_RX);

            _btMode = BTMODE_LONGCOUNT;
            break;
         }
      default:
         {
            result = FAILURE;
            break;
         }
   }

   return result; 
}

