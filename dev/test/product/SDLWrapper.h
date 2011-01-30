#ifndef _SDL_WRAPPER_H_
#define _SDL_WRAPPER_H_

#include "SDL.h"

class SDLWrapper
{
   private:
      SDL_Surface * _screen;
      
   public:
   
      SDLWrapper(SDL_Surface *screen);
      
      void drawRect(int x1, int y1, int x2, int y2, Uint32 lineColor, Uint32 fillColor) const;
      void drawAxisLine(int x1, int y1, int x2, int y2, Uint32 lineColor) const;
};

#endif
