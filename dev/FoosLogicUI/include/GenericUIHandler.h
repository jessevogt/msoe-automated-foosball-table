/**
 * File: GenericUIHandler.h
 *
 * Base class for any drawing library implementations
 *
 * Author: Jesse Vogt
 * Date: 5/1/2005
 */

#ifndef _GENERICUIHANDLER_H_
#define _GENERICUIHANDLER_H_

#include <string>

using std::string;

class GenericUIHandler
{
public:

   /**
    * Enumeration for the type of events
    */
   enum GUIHEventType { NONE = 0, INVALID, KEYDOWN, QUIT };
   
   /**
    * Enumeration for all the supported colors
    */
   enum GUIHColor {WHITE, BLACK, RED, GREEN, BLUE, YELLOW, PINK};
   
   /**
    * Enumeration for all supported text sizes
    */
   enum GUIHSize { SMALL, NORMAL, LARGE };
   
   /**
    * Inner class to wrap around any events used by the underlying graphics library
    */
   class GUIHEvent
   {
   public:
      
      /**
       * accessor for the type of event
       */
      inline GUIHEventType getType() const { return _type; }
      
      /**
       * accessor for any associated value of the event
       */
      inline string getValue() const { return _value; }
      
      /**
       * mutator for the type of the event
       */
      inline void setType( GUIHEventType type ) { _type = type; }
      
      /**
       * mutator for the value of the event
       */
      inline void setValue(const string& value ) { _value = value; }

   private:
   
      /**
       * type of event
       */
      GUIHEventType _type;
      
      /**
       * string value of event
       */
      string _value;
   };
 
   /**
    * mutator for the font size
    */
   virtual void setFontSize(GUIHSize size) { _fontSize = size; }
   
   /**
    * mutator for the color of the line
    */
   virtual void setLineColor(GUIHColor color) { _lineColor = color; }
   
   /**
    * mutator for the fill color
    */
   virtual void setFillColor(GUIHColor color) { _fillColor = color; }

   /**
    * perform any necessary initialization for the underlying graphics library
    *
    * Returns:
    *    true on successful initialization and false on a failure
    */
   virtual bool init() = 0;
   
   /**
    * Draw the rectangle specified by the corner coordinates of x1,y1 and x2,y2
    */
   virtual void drawRect(int x1, int y1, int x2, int y2) = 0;
   
   /**
    * Draw the circle specified by the radius and center xCenter, yCenter
    */
   virtual void drawCircle(int radius, int xCenter, int yCenter) = 0;
   
   /**
    * Draw line parallel to one of the axis specified by line with start and endpoints
    * of x1,y1 and x2,y2
    */
   virtual void drawAxisLine(int x1, int y1, int x2, int y2) = 0;
   
   /**
    * refresh display
    */
   virtual void paintDisplay() = 0;
   
   /**
    * get most recent event and store into passed guihEvent
    */
   virtual bool getEvent(GUIHEvent& guihEvent) const = 0;
   
   /**
    * draw integer specified by number with upper left coordinate of x,y
    */
   virtual void drawInt(int number, int x, int y) = 0;
   
   /**
    * draw text specified by text with upper left coordinate of x,y
    */
   virtual void drawText(const string & text, int x, int y) = 0;
   
   /**
    * clear screen
    */
   virtual void blankScreen() = 0;

protected:

   /**
    * Constructor
    */
   GenericUIHandler() : _fontSize(NORMAL), _lineColor(WHITE), _fillColor(BLACK) {}
  
   /**
    * number of avilable colors
    */
   static const unsigned int NUMBER_OF_COLORS = 7;

   /**
    * size of font for any drawn text
    */
   GUIHSize  _fontSize;
   
   /**
    * color of line for any drawn lines
    */
   GUIHColor _lineColor;
   
   /**
    * color fill for any filled object and text
    */
   GUIHColor _fillColor;
};

#endif //_GENERICUIHANDLER_H_
