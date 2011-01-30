all: condvar.o 
include ../../../rtl.mk

help:
	@echo "/**************************************************************/"
	@echo "/**************************************************************/"
	@echo "/*  In this program a real-time thread sends a signal to      */"
	@echo "/* itself before becoming blocked on a condition variable.    */"
	@echo "/* After becoming blocked it executes the signal handler for  */"
	@echo "/* the previously generated signal. After this, it resumes    */"
	@echo "/* blocking on the condition variable.                        */"
	@echo "/**************************************************************/"	
	@echo "/**************************************************************/"	

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
	@insmod condvar.o
	@sleep 5
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod condvar
	@kill `pidof -s tail`

include ../../../Rules.make
