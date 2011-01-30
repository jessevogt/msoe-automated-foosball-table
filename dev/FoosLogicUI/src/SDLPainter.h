#ifndef _SDLPAINTER_H_
#define _SDLPAINTER_H_

#include "GenericPainter.h"
#include "SDL.h"

class SDLPainter : public GenericPainter
{
protected:

	SDL_Surface * _screen;
	Uint32 _curLineColor;
    Uint32 _colors[GenericPainter::NUMBER_OF_COLORS];
	
public:

	SDLPainter();
	SDLPainter(const SDLPainter& orig);
	virtual ~SDLPainter();
	SDLPainter& operator=(const SDLPainter& rhs);
	
	virtual bool init();
	virtual void setLineColor(GenericPainter::GPColor color);
    virtual void drawRect(int x1, int y1, int x2, int y2);
    virtual void drawAxisLine(int x1, int y1, int x2, int y2);
    virtual bool paintDisplay();
};

#endif //_SDLPAINTER_H_
