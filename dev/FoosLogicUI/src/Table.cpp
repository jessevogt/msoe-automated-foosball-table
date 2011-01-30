/**
 * File: Table.cpp
 *
 * Implementation of the drawable Table code
 *
 * Author: Jesse Vogt
 * Date: 5/1/2005
 */

#include "Table.h"
#include "GenericUIHandler.h"
#include "FLULogger.h"
#include "FLUDefs.h"
#include "mbuff.h"
#include "FoosObject.h"

Table::Table()
{
   // intiliaze pointers for using the shared memory from the realtime
   xBreakSMPtr = ( int * ) mbuff_alloc(SHAREMEM(x,0), sizeof(int) * (NUMBER_X_RX + 1));
   yBreakSMPtr = ( int * ) mbuff_alloc(SHAREMEM(y,0), sizeof(int) * (NUMBER_Y_RX + 1));
}

Table::~Table()
{
   // free shared memory from user space side
   mbuff_free(SHAREMEM(y,0),(void*)yBreakSMPtr);
   mbuff_free(SHAREMEM(x,0),(void*)xBreakSMPtr);
}

void Table::draw()
{
   if( FoosObject::_drawThisObject )
   {
      // put all table dimensions in shorter var names  
      FoosObject::_guih->setFillColor(GenericUIHandler::WHITE);
      int spacingX = FoosObject::_tableDim->getSpacingX();
      int spacingY = FoosObject::_tableDim->getSpacingY();
      int startX = FoosObject::_tableDim->getStartX();
      int startY = FoosObject::_tableDim->getStartY();
      int laserX = FoosObject::_tableDim->getLaserX();
      int laserY = FoosObject::_tableDim->getLaserY();
      int markLen = FoosObject::_tableDim->getMarkLen();
      
      int length = laserX * spacingX;
      int height = laserY * spacingY;
      
      int endY = startY + height;
      
      int laserStartY = startY + height;
      int laserEndY = laserStartY + markLen;
      
      int laserStartX = startX + length;
      int laserEndX = laserStartX + markLen;
      
      int bottomTextX = startY - 8;
      
      int laser_loc = 0;
      int line_loc = 0;
      
      FoosObject::_guih->setFontSize(GenericUIHandler::SMALL);
      
      int displayNumber = 0;
   
      // loop through all x lasers
      for(int x=0; x<laserX; ++x)
      {
         laser_loc = (startX + (x * spacingX)) + spacingX / 2;
         line_loc = startX + (x * spacingX);
         
         // draw laser
         FoosObject::_guih->setLineColor(GenericUIHandler::RED);
         FoosObject::_guih->drawAxisLine(laser_loc, laserStartY, laser_loc, laserEndY);
         
         // draw laser number
         FoosObject::_guih->drawInt(x, laser_loc - 4, laserEndY + 1);
         
         // draw line
         FoosObject::_guih->setLineColor(GenericUIHandler::WHITE);
         FoosObject::_guih->drawAxisLine(line_loc, startY, line_loc, endY);
           
         // draw coordinate
         if( displayNumber < 10 )
         {
           FoosObject::_guih->drawInt(displayNumber,line_loc-2,bottomTextX);
         }
         else
         {
           FoosObject::_guih->drawInt(displayNumber,line_loc - 5, bottomTextX);
         }
         
         displayNumber += 2;
      }
      
      // draw the last laser and label
      line_loc = startX + (laserX * spacingX);
      FoosObject::_guih->drawAxisLine(line_loc, startY, line_loc, startY + height);
      FoosObject::_guih->drawInt(displayNumber,line_loc - 5, bottomTextX);   
      
      displayNumber = 0;
      
      for(int y=0; y<laserY; ++y)
      {
         laser_loc = (startY + (y * spacingY)) + spacingY / 2;
         line_loc = startY + (y * spacingY);
         
         // draw laser
         FoosObject::_guih->setLineColor(GenericUIHandler::RED);
         FoosObject::_guih->drawAxisLine(laserStartX, laser_loc, laserEndX, laser_loc);
         
         // draw the laser label
         FoosObject::_guih->drawInt(y, laserEndX + 1, laser_loc - 3);
         
         // draw line
         FoosObject::_guih->setLineColor(GenericUIHandler::WHITE);
         FoosObject::_guih->drawAxisLine(startX, line_loc, startX + length, line_loc);
         
         // draw the line label
         if( displayNumber < 10 )
         {
           FoosObject::_guih->drawInt(displayNumber, startX - 6, line_loc - 3);
         }
         else if( displayNumber < 100 )
         {
           FoosObject::_guih->drawInt(displayNumber, startX - 11, line_loc - 3);
         }
         else
         {
           FoosObject::_guih->drawInt(displayNumber, startX - 16, line_loc - 3);
         }
         
         displayNumber += 2;
      }
      
      // draw the last line and label
      line_loc = startY + (laserY * spacingY);
      FoosObject::_guih->drawAxisLine(startX, line_loc, startX + length, line_loc);
      FoosObject::_guih->drawInt(displayNumber, startX - 16, line_loc - 3);
      
      int index = 0;
      
      // copy the x breaks from shared memory
      memcpy((void*)xBreaks,(void*)xBreakSMPtr,sizeof(coord) * (NUMBER_X_RX + 1));
      
      // loop through all x breaks
      while( xBreaks[index] != -1 )
      {
         // use green for the breaks in the x direction
         FoosObject::_guih->setFillColor(GenericUIHandler::GREEN);
   
         // calculate the coordinates for the rectange to put over the breaks
         int x1 = FoosObject::_tableDim->getStartX() + (xBreaks[index] * FoosObject::_tableDim->getSpacingX());
         int y1 = FoosObject::_tableDim->getStartY();
         int x2 = FoosObject::_tableDim->getStartX() + (xBreaks[index] * FoosObject::_tableDim->getSpacingX()) + FoosObject::_tableDim->getSpacingX();
         int y2 = FoosObject::_tableDim->getStartY() + (FoosObject::_tableDim->getLaserY() * FoosObject::_tableDim->getSpacingY());
         
         // draw the rectangle
         FoosObject::_guih->drawRect(x1,y1,x2,y2);
         ++index;
      }
      
      index = 0;
      
      // copy all the y breaks from shared memory
      memcpy((void*)yBreaks,(void*)yBreakSMPtr,sizeof(coord) * (NUMBER_Y_RX + 1));
      
      // loop though all y breaks
      while( yBreaks[index] != -1 )
      {  
         // use green for the y breaks
         FoosObject::_guih->setFillColor(GenericUIHandler::RED);
   
         // calculate the coordinates for the rectangle that shows the break
         int x1 = FoosObject::_tableDim->getStartY();
         int y1 = FoosObject::_tableDim->getStartX() + (yBreaks[index] * FoosObject::_tableDim->getSpacingY());
         int x2 = FoosObject::_tableDim->getStartY() + (FoosObject::_tableDim->getLaserX() * FoosObject::_tableDim->getSpacingX());
         int y2 = FoosObject::_tableDim->getStartY() + (yBreaks[index] * FoosObject::_tableDim->getSpacingY()) + FoosObject::_tableDim->getSpacingY();
   
         // draw the rectangle
         FoosObject::_guih->drawRect(x1,y1,x2,y2);
         ++index;
      }
   
      FoosObject::drawContained();
   }
      
   return;
}
