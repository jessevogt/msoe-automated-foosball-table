/**
 * File: Ball.cpp
 *
 * Ball Implementation
 *
 * Author: Jesse Vogt
 * Date: 5/1/2005
 */
 
#include "Ball.h"
#include "mbuff.h"
#include "FLUDefs.h"

Ball::Ball()
{
   // get pointer to the shared memory used for accessing the position of the ball
   ballPosSMPtr = (Point*) mbuff_alloc(BALL_POS_SMLABEL,sizeof(Point));
}

Ball::~Ball()
{
   // free memory used for accessing ball position
   mbuff_free(BALL_POS_SMLABEL,(void*)ballPosSMPtr);
}

void Ball::draw()
{
   // copy the position of the ball from shared memory
   memcpy((void*)&point,(void*)ballPosSMPtr,sizeof(Point));
   
   // set the fill color of the ball to white
   _guih->setFillColor(GenericUIHandler::WHITE);
   
   // convert the x position of the ball coordinates to screen coordinates
   int x = point.x * _tableDim->getSpacingX() / 2 + _tableDim->getStartX();
   
   // convert the y position of the ball coordinates to screen coordinates
   int y = _tableDim->getStartY() + (_tableDim->getSpacingY() / 2) + (point.y * _tableDim->getSpacingY() / 2); 
   
   // draw the ball on the screen
   _guih->drawCircle(10, x, y);
   
   // set the text size to large for printing the position of the ball
   _guih->setFontSize(GenericUIHandler::LARGE);
   
   // draw the x position of the ball
   _guih->drawInt(point.x, 0, 0);
   
   // draw the y position of the ball
   _guih->drawInt(point.y,40, 0);
   
   // draw any contained objects
   FoosObject::drawContained();
}

