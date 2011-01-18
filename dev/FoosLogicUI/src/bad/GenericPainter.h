#ifndef _GENERICPAINTER_H_
#define _GENERICPAINTER_H_

class GenericPainter
{
protected:
	
	GenericPainter() {}
	GenericPainter(const GenericPainter& orig) {}
    
    static const unsigned int NUMBER_OF_COLORS = 5;
    	
public:

	virtual ~GenericPainter() {}

	enum GPColor {WHITE, BLACK, RED, GREEN, BLUE};
	
	GenericPainter& operator=(const GenericPainter& rhs);
	
	virtual bool init() = 0;
	virtual void setLineColor(GPColor color) = 0;
    virtual void drawRect(int x1, int y1, int x2, int y2) = 0;
    virtual void drawAxisLine(int x1, int y1, int x2, int y2) = 0;
    virtual bool paintDisplay() = 0;
 
};

#endif //_GENERICPAINTER_H_
