all: sig_intr.o

include ../../../rtl.mk

help:
	@echo "/****************************************************************/"
	@echo "/****************************************************************/"	
	@echo "/*  This program shows what happens when a non-periodic thread  */"
	@echo "/* is executing a handler and is interrupted by a higher        */"
	@echo "/* priority thread.                                             */"
	@echo "/****************************************************************/"	
	@echo "/****************************************************************/"	

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
	@insmod sig_intr.o
	@sleep 3
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod sig_intr
	@kill `pidof -s tail`

include ../../../Rules.make
