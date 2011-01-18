/**
 * File: main.cpp
 *
 * Main entry point for the user interface
 *
 * Author: Jesse Vogt
 * Date: 5/1/2005
 */
 
#include "SDLHandler.h"
#include "GenericUIHandler.h"
#include "Table.h"
#include "FLULogger.h"
#include "Rod.h"
#include "Ball.h"
#include "mbuff.h"
#include <iostream>

int main(int argc, char** argv)
{
   // create GenericUIHandler using the SDL implementation
   GenericUIHandler * guih = new SDLHandler();
   
   // create new table dimensions and use default values
   TableDim * tableDim = new TableDim();

   // set the table dimensions and ui handler
   FoosObject::setTableDim(tableDim);
   FoosObject::setUIHandler(guih);
   
   // create table
   Table table;
   
   // create the goalie and fullback rods and the ball
   FoosObject * goalie = new Rod(0);
   FoosObject * fullback = new Rod(1);
   FoosObject * ball = new Ball();
   
   // add all the object to the table
   table.add(goalie);
   table.add(fullback);
   table.add(ball);

   if( guih->init() )
   {
      bool done = false;
      bool paused = false;
        
      // main program loop
      while( !done )
      {
         GenericUIHandler::GUIHEvent event;
         
         // get the event from SDL
         guih->getEvent(event);
   
         // check the event values
         if( event.getValue() == "q" )
         {
            // exit the program
            done = true;
         }
         else if( event.getValue() == "space" )
         {
            // pause the game
            
            FILE * file;
            file = fopen("/dev/rtf8","w");
            fwrite("pause",5,1,file);
            fclose(file);
            
            paused = true;
         }
         else if( event.getValue() == "s" )
         {
            // restart the game after a pause
            
            FILE * file;
            file = fopen("/dev/rtf9","w");
            fwrite("start",5,1,file);
            fclose(file);
            
            paused = false;
         }
         
         guih->blankScreen();
         
         if( paused )
         {
            // draw the paused text
            guih->drawText("PAUSED",900,700);
         }
         
         // draw table and all contained objects
         table.draw();
         guih->paintDisplay();
      }
   }
   
   delete tableDim;
   delete guih;

   return 0;
}
