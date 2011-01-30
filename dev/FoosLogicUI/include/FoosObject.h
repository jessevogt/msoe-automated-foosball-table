/**
 * File: FoosObject.h
 *
 * Base class for any drawable object on the UI screen
 *
 * Author: Jesse Vogt
 * Date: 5/1/2005
 */

#ifndef _FOOSOBJECT_H_
#define _FOOSOBJECT_H_

#include <list>
#include "TableDim.h"
#include "GenericUIHandler.h"

class FoosObject
{
protected:

   /**
    * single instance of the GenericUIHandler used by all derived and contained
    * classes
    */
   static GenericUIHandler * _guih;
   
   /**
    * any drawable children objects
    */
   std::list< FoosObject * > contained;
   
   /**
    * flag for whether or not this object should be drawn
    */
   bool _drawThisObject;
   
   /**
    * a structure containing all of the table dimensions
    */
   static TableDim * _tableDim;
   
   /**
    * Draw all contained objects
    */
   inline void drawContained()
   {
      for( std::list< FoosObject * >::iterator itr = contained.begin();
           itr != contained.end();
           ++itr )
      {
         (*itr)->draw();
      }
   }
   
public:

   /**
    * Constructor
    */
   FoosObject() : _drawThisObject(true) {}
   
   /**
    * Deconstructor
    */
   virtual ~FoosObject()
   {
      // delete any contained objects
      std::list< FoosObject * >::iterator itr;
      
      for( itr = contained.begin(); itr != contained.end(); ++itr )
      {
         delete *itr;
      }
   }
   
   /**
    * Draw this object
    */
   virtual void draw() = 0;
   
   /**
    * add child object to container
    *
    * Parameters:
    *    foosObject - new drawable object to add
    */
   virtual void add( FoosObject * foosObject ) { contained.push_back( foosObject ); }
   
   /**
    * set whether or not this object is drawable
    *
    * Parameters:
    *    allow - boolean for whether or not to allow drawing;
    */
   inline void allowDraw( bool allow ) { _drawThisObject = allow; }
   
   /**
    * class (static) method to set the table dimensions used by the all drawable objects
    *
    * Parameters:
    *    tableDim - table dimension object
    */
   static void setTableDim( TableDim * tableDim ) { _tableDim = tableDim; }
   
   /**
    * class (static) method to set the UI Handler for all drawing by all drawable objects
    *
    * Parameters:
    *    guih - pointer to generic ui handler for drawing anything on screen
    */
   static void setUIHandler( GenericUIHandler * guih ) { _guih = guih; }
};

#endif // _FOOSOBJECT_H_
