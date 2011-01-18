/**
 * File: Motor.c
 *
 * Motor class implementation file
 *
 * Author: Jesse Vogt
 * Date: 4/18/2005
 */
 
#include <linux/slab.h>

#include "Motor.h"

#include "FLUDefs.h"
#include "FLUTypes.h"
#include "FLULogger.h"

/**
 * array of motor pointers to track the dynamic memory being used
 */
static Motor * _motors[NUMBER_OF_MOTORS];

/**
 * index into the array of motor pointers
 */
static unsigned int _motorIndex = 0;

inline Status motorStep(unsigned int numberOfSteps, Direction direction, unsigned long cyclesToWait, Motor * motor)
{  
   // make sure the state is not currently stop
   if( motor->state == COMPLETE )
   {
      if( numberOfSteps > 0 )
      {
         motor->direction = direction;

         // set the number of steps (edges * 2 = steps)
         motor->edgeToSend = numberOfSteps << 1;

         // set the number of cycles
         motor->cyclesToWait = cyclesToWait;

         // set the state to busy
         motor->state = BUSY;
      }
   
      return SUCCESS;
   }
   else
   {
      return FAILURE;
   }
}

Status stopMotors(void)
{
   int t;
   
   for( t = 0; t < _motorIndex; ++t )
   {  
      // save current state
      _motors[t]->prevState = _motors[t]->state;
      
      // set the current state to stop
      _motors[t]->state = STOP;
   }
   
   return SUCCESS;
}

Status resumeMotors(void)
{
   int t;
   
   for( t = 0; t < _motorIndex; ++t )
   {
      // restore previous state
      _motors[t]->state = _motors[t]->prevState;
   }
   
   return SUCCESS;
}

Motor * createNewMotor( unsigned char bitPosition )
{
   // make sure to not overstep bounds
   if( _motorIndex < NUMBER_OF_MOTORS )
   {
      // create new area of memory for the Motor instance
      Motor * mptr = (Motor *) kmalloc(sizeof(Motor),GFP_KERNEL);
      
      // create "normal" instance with all values initialized
      Motor temp = { PULL,          // motor direction
                     false,         // enabled
                     0,             // edges to send
                     0,             // cycles to wait
                     bitPosition,   // bit position
                     COMPLETE,       // state
                     COMPLETE };     // previous state
                     
      // copy the initialized array to the pointer
      memcpy(mptr,&temp,sizeof(Motor));
      
      // store the pointer in the array
      _motors[_motorIndex] = mptr;
      
      ++_motorIndex;
   
      return mptr;
   }
   else
   {
      // array over limit so return null
      return null;
   }
}

void freeAllMotorMem(void)
{
   int t;
   
   // loop through and free all motor memory
   for( t = 0; t < _motorIndex; ++t )
   {
      kfree(_motors[t]);
   }
}
