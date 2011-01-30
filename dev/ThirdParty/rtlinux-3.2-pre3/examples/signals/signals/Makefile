all: signals.o

include ../../../rtl.mk

help:
	@echo "/***********************************************************/"
	@echo "/***********************************************************/"
	@echo "/*  This is a test program in which there is a master      */"
	@echo "/* thread and a user defined number of slave-threads. The  */"
	@echo "/* master thread is periodic and sends a signal to all     */"
	@echo "/* slave threads each time it is woken up. Slave threads   */"
	@echo "/* are suspended and are woken up by their signal handlers.*/"
	@echo "/* After receiving a number of signals, a slave thread     */"
	@echo "/* blocks that signal with \"pthread_sigmask\". After this   */"
	@echo "/* no more signals are delivered.                          */"
	@echo "/***********************************************************/"
	@echo "/***********************************************************/"

	@echo "First we remove any existing rtl-modules"     

clean:
	rm -f *.o

test: all help
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
	@insmod signals.o
	@sleep 5
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod signals
	@kill `pidof -s tail`

include ../../../Rules.make
