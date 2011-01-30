all: mutex.o mutex_timed.o

include ../../../rtl.mk

help:
	@echo "/**************************************************************/"
	@echo "/**************************************************************/"
	@echo "/*  In this program, various threads are blocked by a mutex.  */"
	@echo "/* While they are blocked by the mutex, the master thread     */"
	@echo "/* sends signals to them. At this point blocked threads       */"
	@echo "/* execute their signal handlers, but they remain blocked.    */"
	@echo "/*  There is also a timed-version that can be started with:   */"
	@echo "/*  make test_timed                                           */"
	@echo "/**************************************************************/"	
	@echo "/**************************************************************/"	

clean:
	rm -f *.o

restart:
	@echo "First we remove any existing rtl-modules"
	@echo "You may see error warnings from \"make\" - ignore them"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../../; scripts/rmrtl)
	@echo "Now insert the fifo and scheduler"
	@echo "Type <return> to continue"
	@read junk
	(cd ../../../; scripts/insrtl)

test_timed: all help restart
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@tail -f /var/log/messages -n 0 &
	@insmod mutex_timed.o
	@sleep 4
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod mutex_timed
	@kill `pidof -s tail`

test: all help restart
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@tail -f /var/log/messages -n 0 &
	@insmod mutex.o
	@sleep 4
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod mutex
	@kill `pidof -s tail`

include ../../../Rules.make
