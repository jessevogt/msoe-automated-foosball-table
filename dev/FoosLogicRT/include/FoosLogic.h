/**
 * File: FoosLogic.h
 *
 * Prototypes for FoosLogic class
 * Contains the thread and methods for handling the gameplay strategy for the 
 * automated foosball system. Main system logic and homing routines are
 * are included.
 *
 * When starting the FoosLogic thread the use the following command order:
 *
 *    FoosLogic.setup();
 *    FoosLogic.run();
 *    FoosLogic.startPlay();
 *    ...
 *    FoosLogic.cleanup();
 *
 * Each of the above calls should return SUCCESS before the next one is executed.
 * After startPlay is called the system is in a functioning state. 
 *
 * When the system is preparing to exit by calling cleanup on each of the modules
 * the clean up for FoosLogic should be called prior to the cleanup for MotorController
 * since FoosLogic holds pointer to Motors controled by MotorController and will
 * become invalid once the MotorController method is called.
 *
 * This class is implemented as a Singleton as there will only ever be one instance
 * of it running. This is accompolished by the extern statement at the end of this
 * file and the static definitions in FoosLogic.c. For more information on this
 * implementation see http://www.csd.uwo.ca/%7ejamie/C/encapsulatedC.html
 *
 * Author: Jesse Vogt
 * Date: 4/25/2005
 */
 
#ifndef _FOOSLOGIC_H_
#define _FOOSLOGIC_H_

#include "FLUTypes.h"
#include "Motor.h"
#include "Rod.h"

/**
 * Typedefs for each of the methods that are implemented by FoosLogic
 */
typedef Status (fl_setup_func)(void);
typedef Status (fl_run_func)(void);
typedef Status (fl_startplay_func)(void);
typedef Status (fl_pause_func)(void);
typedef Status (fl_cleanup_func)(void);
typedef Status (fl_home_func)(RodId);
typedef Status (fl_usercommandmotor_func)(MotorId, int, int, Direction);
typedef Status (fl_userrotaterod_func)(RodId, int, int, Direction);

/**
 * FoosLogic class
 */
typedef struct foos_logic
{
   /**
    * Perform the necessary setup for the FoosLogic thread
    *
    * Prototype:
    *    Status setup( void )
    *
    * Parameters:
    *    void
    *
    * Returns:
    *    SUCCESS or FAILURE
    */
   fl_setup_func * const setup;
   
   /**
    * Start the FoosBall Logic thread
    *
    * Prototype
    *    Status run( void )
    *
    * Parameters:
    *    void
    *
    * Returns:
    *    SUCCESS or FAILURE
    */
   fl_run_func * const run;
   
   /**
    * Start the main program loop that start the game play for the system
    *
    * Prototype:
    *    Status startPlay( void )
    *
    * Parameters:
    *    void
    *
    * Returns:
    *    SUCCESS or FAILURE
    */
   fl_startplay_func * const startPlay;
   
   /**
    * Pause the logic portion of the game
    *
    * Prototype:
    *    Status pause( void )
    *
    * Parameters:
    *    void
    *
    * Returns:
    *    SUCCESS or FAILURE
    */
   fl_pause_func * const pause;
   
   /**
    * Perform any necessary clean-up for the FoosLogic thread
    *
    * Prototype:
    *    Status cleanup( void )
    *
    * Parameters:
    *    void
    *
    * Returns:
    *    SUCCESS or FAILURE
    */
   fl_cleanup_func * const cleanup;

   /**
    * Home the specified rod
    *
    * Prototype:
    *    Status home( RodId id )
    *
    * Parameters:
    *    id - the id of the rod to home (see Rod.h for RodId definition)
    *
    * Returns:
    *    SUCCESS or FAILURE
    */
   fl_home_func * const home;
   
   /**
    * Send a user generated command to the motors. This is used to command the
    * motors from the console or other source besides the logic thread itself.
    *
    * Prototype:
    *    Status userCommandMotor(MotorId motor, int steps, int wait, Direction dir)
    *
    * Parameters:
    *    motor - id of the motor to command (see Motor.h for MotorId definition)
    *    steps - number of steps to send the motor
    *    wait - number of motor thread cycles to wait before sending the command
    *          (see MotorController.c and FLUDefs.h for cycle timing information)
    *    dir - direction to turn the motor (see Motor.h for Direction definition)
    */
   fl_usercommandmotor_func * const userCommandMotor;

   /**
    * Prototype:
    *    Status userRotateRod(RodId id, int anglePosition, int timeToWait, Direction dir)
    *
    * Parameters:
    *    id - id of the rod to rotate
    *    anglePosition - angle to rotate the rod to
    *    timeToWait - number of cycles to wait before sending the command
    *    dir - direction to turn the rod
    *
    * Returns:
    *    SUCCESS or FAILURE
    */
   fl_userrotaterod_func * const userRotateRod; 
}
foos_logic;

/**
 * Single instance of FoosLogic used by the entire system
 */
extern foos_logic FoosLogic;

#endif // _FOOSLOGIC_H_:
