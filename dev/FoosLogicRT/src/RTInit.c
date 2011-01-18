#include <rtl.h>
#include <rtl_fifo.h>
#include <asm/io.h>
#include <pthread.h>

#include "FLULogger.h"
#include "FLUDefs.h"
#include "FLUTypes.h"

#include "IODriver.h"

#include "BallTracker.h"
#include "MotorController.h"
#include "FoosLogic.h"

static int command_handler(unsigned int argv);
static int pause_handler(unsigned int argv);
static int start_handler(unsigned int argv);
static int init_module(void);
static void cleanup_module(void);

int fd;

/**
 * Interrupt handler for normal commands sent from the shell
 *
 * Parameters and return values for this method are in place only to meet the requirements
 * of the function pointer and are not being used
 */
static int command_handler(unsigned int argv)
{
   // character array for holding the incoming meesage
   char msg[100];
   
   // character array for the parsed command
   char command[9];
   
   // character array for the parsed values
   int values[32];
   
   // flag if command was found
   bool cmdFound = false;
   
   // index of the current value
   int valIndex = 0;
   
   // loop index
   int index = 0;
   
   // flag for if value is negative
   bool minus = false;
   
   // loop flag
   bool loop = true;
   
   values[valIndex] = 0;
   
   // pull message from pipe
   rtf_get(7, msg, sizeof(msg));
      
   while( loop == true )
   {
      // test if command has already been found
      if( cmdFound == false )
      {
         // loop until max size of command reached (9) or space found
         if( msg[index] == ' ' && index < 9 )
         {
            // set cmdFound flag
            cmdFound = true;
            
            // make command a null terminated string
            command[index] = 0;
         }
         // max size of command reached
         else if( msg[index] == 10 && index < 9 )
         {
            cmdFound = true;
            loop = false;
            command[index] = 0;
         }
         else if( index < 8 )
         {
            // add character to command string
            command[index] = msg[index];
         }
         else
         {
            DEBUG_MSG_VAL("Error parsing command at index ",index)
            loop = false;
         }
      }
      else
      {
         // command found so start parsing values
         
         // test for space between values
         if( msg[index] == ' ' )
         {
            // if space then test if last value was negative
            if( minus == true )
            {
               values[valIndex] = -values[valIndex];
            }
            
            // increase val index to next element in array
            ++valIndex;
            
            // rest minus flag
            minus = false;
            
            // clear next value in value array
            values[valIndex] = 0;
         }
         // test for line feed
         else if( msg[index] == 10 )
         {
            // test for minus flag for last value
            if( minus == true )
            {
               values[valIndex] = -values[valIndex];
            }
            
            ++valIndex;
            
            // done with this command so clear loop flag
            loop = false;
         }
         // test for minus char
         else if( msg[index] == '-' )
         {
            minus = true;
         }
         else
         {
            // create value in array
            values[valIndex] *= 10;
            values[valIndex] += msg[index] - '0';
         }
      }
      
      // next character
      ++index;
   }
   
   // test for home command
   if( strcmp("home", command) == 0 )
   {
      DEBUG_MSG_VAL("Begin Home Motor", values[0])
      FoosLogic.home(values[0]);
   }
   // test for start command
   else if( strcmp("start", command) == 0 )
   {
      DEBUG_MSG("Starting gameplay...")
      FoosLogic.startPlay();
   }
   // test for motorcmd (lets user command individual motors)
   else if( strcmp("motorcmd", command) == 0 )
   {
      DEBUG_MSG("Motor Command")
      DEBUG_MSG_VAL("id", values[0])
      DEBUG_MSG_VAL("direction", values[3])
      DEBUG_MSG_VAL("steps", values[1])
      DEBUG_MSG_VAL("wait", values[2])
      
      FoosLogic.userCommandMotor(values[0], values[1], values[2], values[3]);
   }
   // change the ball tracking mode
   else if( strcmp("btmode", command) == 0 )
   {
      switch( values[0] )
      {
         case BTMODE_NORMAL:
         {
            BallTracker.setBallTrackingMode(BTMODE_NORMAL);
            break;
         }
         case BTMODE_LONGCOUNT:
         {
            BallTracker.setBallTrackingMode(BTMODE_LONGCOUNT);
            break;
         }
         default:
         {
            rtl_printf("Invalid Ball Tracking mode: %d\n", values[0]);
            break;
         }
      }
   }
   else if( strcmp("rotate", command) == 0 )
   {
      FoosLogic.userRotateRod(values[0],values[1],0,values[2]);
   }
   // anything else is an invalid command
   else
   {
      rtl_printf("Invalid command: %s\n", command);
   }

   return 0;
}

/**
 * Handles any input on pipe (rtf8) which is being used to capture the pause command.
 * any values written into this pipe will pause the system.
 *
 * Parameters and return values for this method are in place only to meet the requirements
 * of the function pointer and are not being used
 */
static int pause_handler(unsigned int argv)
{
   char msg[100];
   DEBUG_MSG("In Pause Handler!")
   rtf_get(8, msg, sizeof(msg));
   MotorController.pause();
   FoosLogic.pause();
   BallTracker.pause();
   return 0;
}

/**
 * Handles any input on pipe (rtf9) which is being used to capture the start command
 * after the pause command has been hit. Any values written to this pipe will
 * start all the major tasks.
 *
 * Parameters and return values for this method are in place only to meet the requirements
 * of the function pointer and are not being used
 */
static int start_handler(unsigned int argv)
{
   char msg[100];
   DEBUG_MSG("In Start Handler!")
   rtf_get(9, msg, sizeof(msg));
   MotorController.startPlay();
   BallTracker.startPlay();
   FoosLogic.startPlay();
   return 0;
}

/**
 * Main entry point for the real-time system.
 *
 * Parameters and return values for this method are in place only to meet the requirements
 * of the function pointer and are not being used
 */ 
static int init_module(void)
{ 
   // set the correct data directions for all the ports on the I/O board
   IODriver.write(CONFIG0,0x03);
   IODriver.write(CONFIG1,0x00);
   
   // create the handler for the command pipe    
   rtf_destroy(7);
   rtf_create(7,100);
   rtf_create_handler(7,command_handler);
   
   // create the handler for the pause pipe
   rtf_destroy(8);
   rtf_create(8,100);
   rtf_create_handler(8, pause_handler);
   
   // create the handler for the start pipe
   rtf_destroy(9);
   rtf_create(9,100);
   rtf_create_handler(9,start_handler);   

   // setup each of the threads
   BallTracker.setup();
   MotorController.setup();
   FoosLogic.setup();
   
   // start all the thread
   MotorController.run();
   BallTracker.run();
   FoosLogic.run();
   
   // begin main program loops for the ball tracker and motor controller
   BallTracker.startPlay();
   MotorController.startPlay();
   
   return 0;
}

/**
 * When the real-time portion is ready to exit this method is called to allow
 * for everyone to clean up memory and release resources
 *
 * Parameters and return values for this method are in place only to meet the requirements
 * of the function pointer and are not being used
 */
static void cleanup_module(void)
{
   // call cleanup for each of the threads
   FoosLogic.cleanup();
   MotorController.cleanup();
   BallTracker.cleanup();
   
   // destroy all the pipes
   rtf_destroy(7);
   rtf_destroy(8);
   rtf_destroy(9);
}
