#ifndef _TABLE_H_
#define _TABLE_H_

#include "GenericUIHandler.h"

class Table
{
public:
	int _startX;
	int _startY;
	int _spacingX;
	int _spacingY;
	int _laserX;
	int _laserY;
	int _markLen;

	GenericUIHandler::GUIHColor _laserColor;
	GenericUIHandler::GUIHColor _lineColor;
};

#endif // _TABLE_H_
 
