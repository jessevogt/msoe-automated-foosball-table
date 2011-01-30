#ifndef _TABLEDIM_H_
#define _TABLEDIM_H_

class TableDim
{
public:
   TableDim( int startX = 30, int startY = 30,
             int spacingX = 16, int spacingY = 12,
             int laserX = 58, int laserY = 43,
             int markLen = 10 ) :
      _startX(startX), _startY(startY), _spacingX(spacingX), _spacingY(spacingY),
      _laserX(laserX), _laserY(laserY), _markLen(markLen) {}
      
   inline int getStartX() const { return _startX; }
   inline int getStartY() const { return _startY; }
   inline int getSpacingX() const { return _spacingX; }
   inline int getSpacingY() const { return _spacingY; }
   inline int getLaserX() const { return _laserX; }
   inline int getLaserY() const { return _laserY; }
   inline int getMarkLen() const { return _markLen; }
   
private:
   int _startX;
   int _startY;
   int _spacingX;
   int _spacingY;
   int _laserX;
   int _laserY;
   int _markLen;
};

#endif // _TABLEDIM_H_
