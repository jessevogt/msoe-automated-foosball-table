/**
 * Common data types
 *
 * Author: Jesse Vogt
 * Date: 4/15/2005
 */

#ifndef _FLUTYPES_H_
#define _FLUTYPES_H_

typedef short coord;

#ifndef __cplusplus
typedef unsigned char bool;
#endif

typedef enum {PUSH = 1, PULL = 0, CLOCKWISE = 1, COUNTERCLOCKWISE = 0} Direction;
typedef enum {BUSY,WAITING,STOP} MotorState;
typedef unsigned char byte;

/**
 * Enumeration for interpeting the result of method that returns a Status
 *
 * SETUP_SUCCESS - Setup was successful (memory allocated, motor init, etc)
 * SETUP_FAILED - Setup failed (memory alloc fail, missing motors, etc)
 * READY - Thread is started and ready to start processing
 * NOT_READY - Thread is not yet setup or ready for processing
 */
typedef enum {SETUP_SUCCESS, SETUP_FAILED, READY, NOT_READY} Status;

#endif // _FLUTYPES_H_
