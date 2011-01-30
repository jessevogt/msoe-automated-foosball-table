#ifndef _ROD_H_
#define _ROD_H_

#include <vector>
#include <string>
#include "FoosObject.h"

class Rod : public FoosObject
{
public:
   Rod( int id );
   virtual ~Rod();
   
   virtual void draw();
   
private:

   int id;
   int rotPosition;
   int x;
	int y;
   std::vector< int > players;
	int * yPosSMPtr;
	int * rotPosSMPtr;
	std::string yPosSMLabel;
	std::string rotPosSMLabel;
};

#endif // _ROD_H_
