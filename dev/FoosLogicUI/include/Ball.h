/**
 * File: Ball.h
 *
 * Ball FoosObject for displaying the ball on screen
 *
 * Author: Jesse Vogt
 * Date: 5/1/2005
 */

#ifndef _BALL_H_
#define _BALL_H_

#include "FoosObject.h"
#include "Point.h"

class Ball : public FoosObject
{
public:

   /**
    * Default Constructor
    */
   Ball();
   
   /**
    * Desconstructor
    */
   virtual ~Ball();
   
   /**
    * Draw the ball on the screen
    */
   virtual void draw();
   
private:

   /**
    * Current position of the ball
    */
   Point point;
   
   /**
    * Pointer to the shared memory used to read the ball position
    */
   Point * ballPosSMPtr;
};

#endif // _BALL_H_
