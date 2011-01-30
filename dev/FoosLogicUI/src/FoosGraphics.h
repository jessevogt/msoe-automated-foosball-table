#ifndef _FOOSGRAPHICS_H_
#define _FOOSGRAPHICS_H_

#include "Table.h"
#include "GenericUIHandler.h"

class FoosGraphics
{
protected:

    GenericUIHandler* _guih;
    Table _table;
    
public:
	FoosGraphics(GenericUIHandler* guih, const Table & table);
	virtual ~FoosGraphics();
    
    void drawTable(void);
    void drawBall(int radius, int x, int y);
	 void drawXIntercept(int x);
	 void drawYIntercept(int y);

};

#endif //_FOOSGRAPHICS_H_
