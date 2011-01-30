all: semaphores.o semaphores_timed.o

include ../../../rtl.mk


help:
	@echo "/********************************************************************/"
	@echo "/********************************************************************/"
	@echo "/* In this program, various threads wait on a semaphore. While they */"
	@echo "/* are blocked another thread sends a signal to half of the         */"
	@echo "/* semaphore-blocked threads. These threads are interrupted and get */"
	@echo "/* out of the semaphore. The other threads remain blocked.          */"
	@echo "/*  Note: There is also a test version for timed semaphores in the  */" 
	@echo "/* file semaphores_timed.c  It can be started using:                */" 
	@echo "/* make test_timed                                                  */" 
	@echo "/********************************************************************/"
	@echo "/********************************************************************/"

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

test: all help restart
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@tail -f /var/log/messages -n 0 &
	@insmod semaphores.o
	@sleep 3
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod semaphores
	@kill `pidof -s tail`

test_timed: all help restart
	@echo "Now start the real-time tasks  module"
	@echo "Type <return> to continue"
	@read junk
	@tail -f /var/log/messages -n 0 &
	@insmod semaphores_timed.o
	@sleep 3
	@echo "Now let's stop the application"
	@echo "Type <return> to finish"
	@read junk
	@rmmod semaphores_timed
	@kill `pidof -s tail`

include ../../../Rules.make
