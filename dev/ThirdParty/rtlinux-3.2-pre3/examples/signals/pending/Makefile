all: pending.o

include ../../../rtl.mk


help:
	@echo "/**********************************************************************/"
	@echo "/**********************************************************************/"
	@echo "/*  A simple program to test signal delivery order and global         */"
	@echo "/* sigactions.                                                        */"
	@echo "/**********************************************************************/"	
	@echo "/**********************************************************************/"	

clean:
	rm -f *.o

test: all help
	@echo "First we remove any existing rtl-modules"
	@echo "You may see error warnings from \"make\" - ignore them"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../../; scripts/rmrtl)
	@echo "Now insert the fifo and scheduler"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../../; scripts/insrtl)
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@tail -f /var/log/messages -n 0 &
	@insmod pending.o
	@echo "Running real-time application for two seconds!"	
	@sleep 2
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod pending
	@kill `pidof -s tail`

include ../../../Rules.make
