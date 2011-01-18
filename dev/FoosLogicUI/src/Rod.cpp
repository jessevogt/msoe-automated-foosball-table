/**
 * File: Rod.cpp
 *
 * Implementation for the drawable Rod
 *
 * Author: Jesse Vogt
 * Date: 5/1/2005
 */

#include "Rod.h"
#include "mbuff.h"
#include "FLUDefs.h"
#include "FLUTypes.h"
#include "FLULogger.h"

#define ROD_WIDTH 10 
#define GOALIE_ROD_X 57
#define PLAYER_WIDTH 28
#define PLAYER_HEIGHT 52

Rod::Rod( int id )
{
   rotPosition = 0;
   y = 258;
   
   switch (id)
   {
      case 0: // goalie
      {
         // set the player positions for the goalie
         players.push_back( -142 );
         players.push_back(    0 );
         players.push_back(  142 );
         
         // set middle of the goalie rod
         x = GOALIE_ROD_X;
         
         // setup the shared memory labels
         yPosSMLabel = GOALIE_Y_SMLABEL;
         rotPosSMLabel = GOALIE_ROT_SMLABEL;
         
         // set the position to draw the rod position
         yDrawPos = 650;
         break;
      }
      case 1: // fullback
      {
         // set the player positions for the fullback
         players.push_back(-68);
         players.push_back(68);
         
         // set the middle of the fulback rod
         x = GOALIE_ROD_X + 125;
         
         // setup the shared memory labels
         yPosSMLabel = FULLBACK_Y_SMLABEL;
         rotPosSMLabel = FULLBACK_ROT_SMLABEL;
         
         // set the position to draw the rod position
         yDrawPos = 680;
         break;
      }
      case 2:
      {
         x = GOALIE_ROD_X + (3 * 125);
         break;
      }
      case 3:
      {
         x = GOALIE_ROD_X + (5 * 125);
         break;
      }
      default:
      {
         break;
      }
   }
   
   // create shared memory for the rods linear and rotational position
   yPosSMPtr = (int *)mbuff_alloc(yPosSMLabel.c_str(),sizeof(int));
   rotPosSMPtr = (int *)mbuff_alloc(rotPosSMLabel.c_str(),sizeof(int));
}

Rod::~Rod()
{
   // free all the shared memory from the user space side
   mbuff_free(yPosSMLabel.c_str(),(void*)yPosSMPtr);
   mbuff_free(rotPosSMLabel.c_str(),(void*)rotPosSMPtr);
}

void Rod::draw()
{
   int position;
   
   // get the position of the ball from shared memory
   memcpy((void*)&position,(void*)yPosSMPtr,sizeof(int));
   
   // set the fill color, size, and number for the rod center position
   _guih->setFillColor(GenericUIHandler::WHITE);
   _guih->setFontSize(GenericUIHandler::LARGE);
   _guih->drawInt(position,0,yDrawPos);
   
   // set the fill color for the rods
   _guih->setFillColor(GenericUIHandler::YELLOW);
   {
      // setup coordinates for the current rod
      int x1 = _tableDim->getStartX() + x - 2;
      int y1 = _tableDim->getStartY();
      int x2 = _tableDim->getStartX() + x + 3;
      int y2 = _tableDim->getStartY() + ( _tableDim->getLaserY() * _tableDim->getSpacingY());
      
      // draw the rod on the table
      _guih->drawRect(x1,y1,x2,y2);
   }

   {
      // calculate the relpos offset for each player
      int relpos = (int)(((double)position * 0.7713) + 0.5);
      
      for( unsigned int t = 0; t < players.size(); ++t )
      {
         // setup x and y coordinates
         int x1 = x - 12 + _tableDim->getStartX();
         int y1 = players[t] + relpos - 12 + _tableDim->getStartY();
         
         int x2 = x + 13 + _tableDim->getStartX();
         int y2 = players[t] + relpos + 12 + _tableDim->getStartY(); 
   
         // draw the player
         _guih->drawRect(x1,y1,x2,y2);
      }
   }
   
   FoosObject::drawContained();
}
