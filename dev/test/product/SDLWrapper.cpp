#include "SDLWrapper.h"

#include "SDL.h"

SDLWrapper::SDLWrapper(SDL_Surface * screen) : _screen(screen)
{
   
}

void SDLWrapper::drawRect(int x1, int y1, int x2, int y2, Uint32 lineColor, Uint32 fillColor) const
{
   Uint8 *bufp;
   
   for(int x = x1; x <= x2; ++x)
   {
      for(int y = y1; y <= y2; ++y)
      {
         bufp = (Uint8 *) _screen->pixels + y * (_screen->pitch) + x;
         *bufp = lineColor;
      }
   }
   
   return;
}

void SDLWrapper::drawAxisLine(int x1, int y1, int x2, int y2, Uint32 lineColor) const
{
   Uint8 *bufp;
   
   if(x1 == x2)
   {
      for(int y = y1; y <= y2; ++y)
      {
         bufp = (Uint8 *) _screen->pixels + y * (_screen->pitch) + x1;
         *bufp = lineColor;
      }
   }
   else if(y1 == y2)
   {
      for(int x = x1; x <= x2; ++x)
      {
         bufp = (Uint8 *) _screen->pixels + y1 * (_screen->pitch) + x;
         *bufp = lineColor;
      }
   }
   
   return;
}


