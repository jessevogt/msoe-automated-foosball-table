include rtl.mk

DEBUG = -D_DEBUG_ 

all: FoosLogic.o

FoosLogic.o: RTInit.o BallTracker.o IODriver.o
	ld -m elf_i386 -r -o FoosLogic.o RTInit.o BallTracker.o IODriver.o

IODriver.o: IODriver.c IODriver.h RTDebug.h
	gcc $(INCLUDE) $(CFLAGS) $(DEBUG) -c IODriver.c
	
RTInit.o: RTInit.c BallTracker.h FLUTypes.h RTDebug.h
	gcc $(INCLUDE) $(CFLAGS) $(DEBUG) -c RTInit.c

BallTracker.o: BallTracker.c BallTracker.h FLUTypes.h FLUDefs.h RTDebug.h
	gcc $(INCLUDE) $(CFLAGS) $(DEBUG) -c BallTracker.c

clean:
	rm -rf *.o
	

