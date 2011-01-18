/**
 * File: Table.h
 *
 * Class representing a drawable table
 *
 * Author: Jesse Vogt
 * Date: 5/1/2005
 */
 
#ifndef _TABLE_H_
#define _TABLE_H_

#include "FoosObject.h"
#include "FLUTypes.h"
#include "FLUDefs.h"
  
class Table : public FoosObject
{
private:

   /**
    * pointer to the x break share memory
    */
   int * xBreakSMPtr;
   
   /**
    * pointer to the y break share memory
    */
   int * yBreakSMPtr;
   
   /**
    * array of y breaks
    */
   int yBreaks[NUMBER_Y_RX + 1];
   
   /**
    * array of x breaks
    */
   int xBreaks[NUMBER_X_RX + 1];

public:

   /**
    * Constructor
    */
   Table();
   
   /**
    * draw the table
    */
   virtual void draw();
   
   /**
    * Deconstructor
    */
   virtual ~Table();
};

#endif // _TABLE_H_
