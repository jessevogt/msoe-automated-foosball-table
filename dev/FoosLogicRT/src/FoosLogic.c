/**
 * File: FoosLogic.c
 *
 * Definition for the FoosLogic class
 * (see BallTracker.h for interface documentation)
 *
 * Author: Jesse Vogt
 * Date: 4/30/2005
 */

#include <mbuff.h>

#include "FoosLogic.h"

#include "Rod.h"
#include "MotorController.h"
#include "BallTracker.h"

#include "FLUTypes.h"
#include "FLUDefs.h"
#include "FLULogger.h"

/* public methods from header */
static Status setup(void);
static Status run(void);
static Status startPlay(void);
static Status pause(void);
static Status cleanup(void);
static Status home( RodId id );
static Status userCommandMotor(MotorId motor, int steps, int wait, Direction dir);
static Status userRotateRod(RodId id, int anglePosition, int timeToWait, Direction dir);

/*********************************************************************
 * PRIVATE METHODS
 *********************************************************************/
 
/**
 * The basic blocking strategy where the players will only be in front of the
 * ball. If ball is close to one of the commandable rods it will kick.
 *
 * Parameters:
 *    void
 *
 * Returns:
 *    void
 */
static inline void _basicStrategy(void);

/**
 * Function that the runs in FoosLogic thread. Handles the various states/modes
 * of the fooslogic thread.
 *
 * Parameters:
 *    param - void pointer to passed in data (not used)
 *
 * Returns:
 *    void pointer (not used)
 */
static void * _main( void * param );

/*********************************************************************
 * PRIVATE MEMBERS
 *********************************************************************/
 
/**
 * Enumeration for the modes allowed for the FoosLogic thread
 *    FL_HOME - home a specified motor
 *    FL_NORMAL - normal play mode
 *    FL_USER_COMMAND - accept commands from the user for the motors
 *    FL_USER_COMMAND_ROT - accept a command to rotate a rod to a certain
 *                          position
 *    FL_DO_NOTHING - do not send any commands or read from the ball tracker
 */
typedef enum { FL_HOME, FL_NORMAL, FL_USER_COMMAND, FL_USER_COMMAND_ROT, FL_DO_NOTHING } FoosLogicMode;

/**
 * Collection of rods on the table
 * (see Rod.h for rod doc and FLUDefs.h for NUMBER_OF_RODS)
 */
static Rod _rods[NUMBER_OF_RODS];

/**
 * Thread attribute structure for the fooslogic thread
 */
static pthread_attr_t _foosLogicAttr;

/**
 * schedule parameter struct for the fooslogic thread
 */
static struct sched_param _foosLogicSchedParam;

/**
 * FoosLogic thread handle
 */
static pthread_t _foosLogicThread;

/**
 * gameplay loop flag
 */
static bool _start;

/**
 * main progam loop flag
 */
static bool _mainLoop;

/**
 * current mode for the fooslogic thread
 */
static FoosLogicMode _mode;

/**
 * current rod - used for homing routine
 */
static int _currentRod;

/**
 * motor id for user command
 */
static MotorId _userCmdMotorId;

/**
 * number of steps to command the motor from the user
 */
static int _userCmdSteps;

/**
 * number of cycles to wait from the user
 */
static int _userCmdWait;

/**
 * direction to move the motor from the user
 */
static Direction _userCmdDir;

/**
 * current ball position
 */
static Point ballPos;

/**
 * flag if thread was started
 */
static bool _threadStarted;

typedef struct
{
   RodId id;
   int angle;
   int wait;
   Direction dir;
} UserCmdRot;

static UserCmdRot _userRot;

static int * cmdpos;
/**
 * create single instance for use by rest of program
 */
foos_logic FoosLogic = { &setup,
                         &run,
                         &startPlay,
                         &pause,
                         &cleanup,
                         &home,
                         &userCommandMotor,
                         &userRotateRod };

static Status setup(void)
{
   Status status;

   status = SUCCESS;

   // setup loop flags
   _start = false;
   _mainLoop = true;
   _threadStarted = false;
   
   cmdpos = (int *) mbuff_alloc("cmdpos",sizeof(int));
   
   // Goalie Rod
   _rods[GOALIE].stepsToHome = 108;
   _rods[GOALIE].rotationalPosition = 0;
   _rods[GOALIE].y = 0;
   _rods[GOALIE].x = 76;
   _rods[GOALIE].id = 0;
   _rods[GOALIE].numberOfPlayers = 3; 
   _rods[GOALIE].upperLimit = 454;
   _rods[GOALIE].lowerLimit = 238;
   _rods[GOALIE].blockingPosition = 0;
   
   _rods[GOALIE].linearMotor = MotorController.getMotor(GOALIE_LIN);
   _rods[GOALIE].rotationalMotor = MotorController.getMotor(GOALIE_ROT);
   _rods[GOALIE].yPosSMPtr = (int *) mbuff_alloc(GOALIE_Y_SMLABEL, sizeof(int));
   _rods[GOALIE].rotPosSMPtr = (int *) mbuff_alloc(GOALIE_ROT_SMLABEL, sizeof(int));
   
   
      // Closest Player
      _rods[GOALIE].players[0].displacement = -184;
      _rods[GOALIE].players[0].lowerLimit = _rods[GOALIE].lowerLimit + _rods[GOALIE].players[0].displacement; // 42;
      _rods[GOALIE].players[0].upperLimit = _rods[GOALIE].upperLimit + _rods[GOALIE].players[0].displacement; // 270;
      _rods[GOALIE].players[0].y = 0;
      _rods[GOALIE].players[0].rod = &_rods[GOALIE];
      _rods[GOALIE].players[0].id = 0;

      // Middle Player 
      _rods[GOALIE].players[1].displacement = 0;
      _rods[GOALIE].players[1].lowerLimit = _rods[GOALIE].lowerLimit + _rods[GOALIE].players[1].displacement; // 226;
      _rods[GOALIE].players[1].upperLimit = _rods[GOALIE].upperLimit + _rods[GOALIE].players[1].displacement; // 454;
      _rods[GOALIE].players[1].y = 0;
      _rods[GOALIE].players[1].rod = &_rods[GOALIE];
      _rods[GOALIE].players[1].id = 1;

      // Far Player
      _rods[GOALIE].players[2].displacement = 184;
      _rods[GOALIE].players[2].lowerLimit = _rods[GOALIE].lowerLimit + _rods[GOALIE].players[2].displacement; // 410;
      _rods[GOALIE].players[2].upperLimit = _rods[GOALIE].upperLimit + _rods[GOALIE].players[2].displacement; // 638;
      _rods[GOALIE].players[2].y = 0;
      _rods[GOALIE].players[2].rod = &_rods[GOALIE];
      _rods[GOALIE].players[2].id = 2;
   
   // Fullback Rod
   _rods[FULLBACK].stepsToHome = 170;
   _rods[FULLBACK].rotationalPosition = 0;
   _rods[FULLBACK].y = 0;
   _rods[FULLBACK].x = 224;
   _rods[FULLBACK].id = 1;
   _rods[FULLBACK].numberOfPlayers = 2; 
   _rods[FULLBACK].upperLimit = 510;
   _rods[FULLBACK].lowerLimit = 171;
   _rods[FULLBACK].blockingPosition = 0;
   _rods[FULLBACK].linearMotor = MotorController.getMotor(FULLBACK_LIN);
   _rods[FULLBACK].rotationalMotor = MotorController.getMotor(FULLBACK_ROT);
   _rods[FULLBACK].yPosSMPtr = (int *) mbuff_alloc(FULLBACK_Y_SMLABEL, sizeof(int));
   _rods[FULLBACK].rotPosSMPtr = (int *) mbuff_alloc(FULLBACK_ROT_SMLABEL, sizeof(int));
   
      // Closest Player
      _rods[FULLBACK].players[0].displacement = -123;
      _rods[FULLBACK].players[0].lowerLimit = _rods[FULLBACK].lowerLimit + _rods[FULLBACK].players[0].displacement; // 44;
      _rods[FULLBACK].players[0].upperLimit = _rods[FULLBACK].upperLimit + _rods[FULLBACK].players[0].displacement; // 388;
      _rods[FULLBACK].players[0].y = 0;
      _rods[FULLBACK].players[0].rod = &_rods[FULLBACK];
      _rods[FULLBACK].players[0].id = 0;

      // Far Player 
      _rods[FULLBACK].players[1].displacement = 124;
      _rods[FULLBACK].players[1].lowerLimit = _rods[FULLBACK].lowerLimit + _rods[FULLBACK].players[1].displacement;
      _rods[FULLBACK].players[1].upperLimit = _rods[FULLBACK].upperLimit + _rods[FULLBACK].players[1].displacement;
      _rods[FULLBACK].players[1].y = 0;
      _rods[FULLBACK].players[1].rod = &_rods[FULLBACK];
      _rods[FULLBACK].players[1].id = 1;
   
   // init thread attribute structure
   pthread_attr_init( &_foosLogicAttr );
   
   // enable floating point operations for this thread
   pthread_attr_setfp_np( &_foosLogicAttr, 1 );
   
   // set the priority for this thread
   _foosLogicSchedParam.sched_priority = FOOS_LOGIC_PRIORITY;
   
   // set the schedule priority for the fooslogic thread
   pthread_attr_setschedparam( &_foosLogicAttr, &_foosLogicSchedParam );

   DEBUG_MSG("FoosLogic Setup Complete")
   return status;
}

static Status run(void)
{ 
   // start thread
   pthread_create( &_foosLogicThread, &_foosLogicAttr, _main, 0 );
   _threadStarted = true;
   return SUCCESS;
}

static Status startPlay(void)
{
   // start gameplay
   _start = true;
   _mode = FL_NORMAL;
   return SUCCESS;
}

static Status pause(void)
{
   _start = false;
   return SUCCESS;
}

static Status cleanup(void)
{
   // stop program loops
   _start = false;
   _mainLoop = false;
   
   if( _threadStarted == true )
   {
      // delete threads
      pthread_delete_np( _foosLogicThread );
   }

   // release all shared memory
   mbuff_free( GOALIE_Y_SMLABEL, (void *) _rods[GOALIE].yPosSMPtr );
   mbuff_free( GOALIE_ROT_SMLABEL, (void *) _rods[GOALIE].rotPosSMPtr );
   mbuff_free( FULLBACK_Y_SMLABEL, (void *) _rods[FULLBACK].yPosSMPtr );
   mbuff_free( FULLBACK_ROT_SMLABEL, (void *) _rods[FULLBACK].rotPosSMPtr );
   mbuff_free( "cmdpos", (void*)cmdpos);

   return SUCCESS;
}

static Status home( RodId id )
{
   // loop var
   int t = 0;
   
   // flag for if rod was found
   bool rodFound = false;

   // stop normal looping
   _mode = FL_DO_NOTHING;

   // search through all rods for the user specified rod
   while( !rodFound && t < NUMBER_OF_RODS )
   {
      if( _rods[t].id == id )
      {
         rodFound = true;
      }
      else
      {
         ++t;
      }
   }

   // test if rod was found
   if( rodFound == true )
   {
      DEBUG_MSG_VAL("Rod to home found", t)
      
      // set mode for homing
      _mode = FL_HOME;
      
      // set the current rod
      _currentRod = t;
      
      // start program loop
      _start = true;
      
      return SUCCESS;
   }
   else
   {
      DEBUG_MSG_VAL("Invalid rod", id)
      return FAILURE;
   }
}

static void * _main( void * param )
{
   // make thread periodic (see FLUDefs.h for FOOS_LOGIC_PERIOD)
   pthread_make_periodic_np(pthread_self(), gethrtime(), FOOS_LOGIC_PERIOD);

   while( _mainLoop == true )
   {
      pthread_wait_np();

      while( _start == true )
      {
         pthread_wait_np();
         
         // switch on the current mode being used for the FoosLogic
         switch( _mode )
         {
            // homing routine
            case FL_HOME:
            {
               // call the rod homing function for the specified rod
               rodHome( &_rods[_currentRod] );
               
               // go back to doing nothing
               _mode = FL_DO_NOTHING;
               break;
            }
            // normal gameplay mode
            case FL_NORMAL:
            {
               // get the current ball position
               BallTracker.getBallPosition( &ballPos );

               // convert y laser coord into table coordinates
               
               // test for odd or even coordinate
               if( ballPos.y % 2 == 1 )
               {
                  // decrement coordinate since this is odd (will be added back later in the offset)
                  --ballPos.y;
                  
                  // 15 -> mm spacing between each laser
                  // 24 -> spacing between wall and first laser
                  // 7 -> offset since this was an odd coordinate
                  ballPos.y = (ballPos.y / 2) * 15 + 24 + 7;
               }
               else
               {
                  // 15 -> mm spacing between each laser
                  // 24 -> spacing between wall and first laser
                  ballPos.y = (ballPos.y / 2) * 15 + 24;
               }

               //convert x laser coord into table coordinates
               
               // test for odd or even coordinate
               if( ballPos.x % 2 == 1 )
               {
                  // decrement coordinate since this is odd (will be added back later in the offset)
                  --ballPos.x;
                  
                  // 20 -> mm spacing between each laser
                  // 23 -> spacing between wall and first laser
                  // 10 -> offset since the ball was an odd coordinate
                  ballPos.x = (ballPos.x / 2) * 20 + 23 + 10;
               }
               else
               {
                  // 20 -> mm spacing between each laser
                  // 23 -> spacing between wall and first laser
                  ballPos.x = (ballPos.x / 2) * 20 + 23;
               }
               
               // get rid of breaks that we cannot handle
               if( ballPos.y < 60 )
               {
                  ballPos.y = 60;
               }
               else if( ballPos.y > (TABLE_WIDTH - 60) )
               {
                  ballPos.y = TABLE_WIDTH - 60;
               }
 
               // execute basic strategy
               _basicStrategy();
               
               break;
            }
            // user command mode
            case FL_USER_COMMAND:
            {
               // call motor step command based on parameters from the user
               motorStep( _userCmdSteps,
                          _userCmdDir,
                          (unsigned long)_userCmdWait,
                          MotorController.getMotor(_userCmdMotorId) );
                          
               // reset to safe mode after execution
               _mode = FL_DO_NOTHING;
               break;
            }
            case FL_USER_COMMAND_ROT:
            {
               rodRotate( _userRot.angle, &_rods[_userRot.id] );
               _mode = FL_DO_NOTHING;
               break;
            }
            // do nothing
            case FL_DO_NOTHING:
            {
               break;
            }
               
            default:
               break;
         }
      }
   }
   
   return 0;
}

static inline void _basicStrategy(void)
{  
   // make sure ball is within the proper range
   if( ballPos.y > 60 && ballPos.y < (TABLE_WIDTH - 60) )
   {
      // player to command
      Player * player;
      
      // make sure the rods linear motor has completed the last command
      if( _rods[0].linearMotor->state == COMPLETE )
      {
         // get the closest player to the ball
         player = rodGetClosestPlayer( &ballPos, &_rods[GOALIE] );
         
         // move the player in front of the ball
         playerMove(ballPos.y,player);
      }
      
      // make sure the fullback linear rod has completed the last command
      if( _rods[1].linearMotor->state == COMPLETE )
      {
         // get the closest player to the rod
         player = rodGetClosestPlayer( &ballPos, &_rods[FULLBACK] );
         
         // move that player in front of the ball
         playerMove(ballPos.y,player);
      }
     
      // test if the ball is near the goalie rod
      if( ballPos.x > 95 && ballPos.x < 115 )
      {
         int t;
         
         // kick the ball
         rodRotate(90,&_rods[GOALIE]);
 
         // wait for 10 ms
         for( t = 0; t < 10; ++t )
         {
            pthread_wait_np();
         }
         
         // move rod back to original position
         rodRotate(0,&_rods[GOALIE]);

         // wait for anothe 10 ms to make sure ball has cleared
         for( t = 0; t < 500; ++t )
         {
            pthread_wait_np();
         }
         
      }
   
      // test if ball is near the fullback rod
      if( ballPos.x > 250 && ballPos.x < 270 && _rods[1].rotationalMotor->state == COMPLETE )
      {
         int t;
         
         // kick the ball
         rodRotate(90,&_rods[FULLBACK]);
         
         // wait for 10 ms
         for( t = 0; t < 10; ++t )
         {
            pthread_wait_np();
         }
         
         // return rod back to original position
         rodRotate(0,&_rods[FULLBACK]);

         // wait for 10 ms to allow  ball to clear
         for( t = 0; t < 500; ++t )
         {
            pthread_wait_np();
         }
      }
      else if( ballPos.x < 160 && _rods[1].rotationalMotor->state == COMPLETE )
      {
         rodRotate(-90,&_rods[FULLBACK]);
      }
      else if( _rods[1].rotationalMotor->state == COMPLETE )
      {
         rodRotate(0,&_rods[FULLBACK]);
      }
   }
}

static Status userCommandMotor(MotorId motor, int steps, int wait, Direction dir)
{
   // set the class vars to the values from the user
   _userCmdMotorId = motor;
   _userCmdSteps = steps;
   _userCmdWait = wait;
   _userCmdDir = dir;

   // set the current mode to use the user command
   _mode = FL_USER_COMMAND; 
   
   return SUCCESS;
}

static Status userRotateRod(RodId id, int anglePosition, int timeToWait, Direction dir)
{
   _userRot.id = id;
   _userRot.angle = anglePosition;
   _userRot.wait = timeToWait;
   _userRot.dir = dir;
   
   _mode = FL_USER_COMMAND_ROT;
   return SUCCESS;
}

